/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#ifndef __WINE_WGL_H
#define __WINE_WGL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <sys/types.h>

#ifdef WINE_UNIX_LIB
#define GL_NO_PROTOTYPES
#define GLAPIENTRY
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY __stdcall
#endif

#ifndef EGL_CAST
#define EGL_CAST(t,x) ((t)(x))
#endif

typedef void *EGLNativeDisplayType;
typedef void *EGLNativePixmapType;
typedef void *EGLNativeWindowType;
typedef int EGLint;
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef unsigned char GLubyte;
typedef short GLshort;
typedef unsigned short GLushort;
typedef int GLint;
typedef unsigned int GLuint;
typedef int GLclampx;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double DECLSPEC_ALIGN(8) GLdouble;
typedef double DECLSPEC_ALIGN(8) GLclampd;
typedef void *GLeglClientBufferEXT;
typedef void *GLeglImageOES;
typedef char GLchar;
typedef char GLcharARB;
typedef unsigned short GLhalf;
typedef unsigned short GLhalfARB;
typedef int GLfixed;
typedef INT_PTR GLintptr;
typedef INT_PTR GLintptrARB;
typedef INT_PTR GLsizeiptr;
typedef INT_PTR GLsizeiptrARB;
typedef INT64 GLint64;
typedef INT64 GLint64EXT;
typedef UINT64 GLuint64;
typedef UINT64 GLuint64EXT;
typedef struct __GLsync *GLsync;
struct _cl_context;
struct _cl_event;
typedef void *GLDEBUGPROC;
typedef void *GLDEBUGPROCARB;
typedef void *GLDEBUGPROCKHR;
typedef void *GLDEBUGPROCAMD;
typedef unsigned short GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;
typedef void *GLVULKANPROCNV;
struct _GPU_DEVICE {
    DWORD  cb;
    CHAR   DeviceName[32];
    CHAR   DeviceString[128];
    DWORD  Flags;
    RECT   rcVirtualScreen;
};
DECLARE_HANDLE(HPBUFFERARB);
DECLARE_HANDLE(HPBUFFEREXT);
DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);
DECLARE_HANDLE(HPVIDEODEV);
DECLARE_HANDLE(HPGPUNV);
DECLARE_HANDLE(HGPUNV);
DECLARE_HANDLE(HVIDEOINPUTDEVICENV);
typedef struct _GPU_DEVICE GPU_DEVICE;
typedef struct _GPU_DEVICE *PGPU_DEVICE;
struct AHardwareBuffer;
struct wl_buffer;
struct wl_display;
struct wl_resource;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef intptr_t EGLAttribKHR;
typedef intptr_t EGLAttrib;
typedef void *EGLClientBuffer;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDeviceEXT;
typedef void *EGLDisplay;
typedef void *EGLImage;
typedef void *EGLImageKHR;
typedef void *EGLLabelKHR;
typedef void *EGLObjectKHR;
typedef void *EGLOutputLayerEXT;
typedef void *EGLOutputPortEXT;
typedef void *EGLStreamKHR;
typedef void *EGLSurface;
typedef void *EGLSync;
typedef void *EGLSyncKHR;
typedef void *EGLSyncNV;
typedef void (*__eglMustCastToProperFunctionPointerType)(void);
typedef unsigned __int64 EGLTimeKHR;
typedef unsigned __int64 EGLTime;
typedef unsigned __int64 EGLTimeNV;
typedef unsigned __int64 EGLuint64NV;
typedef unsigned __int64 EGLuint64KHR;
typedef __int64 EGLnsecsANDROID;
typedef int EGLNativeFileDescriptorKHR;
typedef ssize_t EGLsizeiANDROID;
typedef void (*EGLSetBlobFuncANDROID) (const void *key, EGLsizeiANDROID keySize, const void *value, EGLsizeiANDROID valueSize);
typedef EGLsizeiANDROID (*EGLGetBlobFuncANDROID) (const void *key, EGLsizeiANDROID keySize, void *value, EGLsizeiANDROID valueSize);
struct EGLClientPixmapHI {
    void  *pData;
    EGLint iWidth;
    EGLint iHeight;
    EGLint iStride;
};
typedef void ( *EGLDEBUGPROCKHR)(EGLenum error,const char *command,EGLint messageType,EGLLabelKHR threadLabel,EGLLabelKHR objectLabel,const char* message);
#define PFNEGLBINDWAYLANDDISPLAYWL PFNEGLBINDWAYLANDDISPLAYWLPROC
#define PFNEGLUNBINDWAYLANDDISPLAYWL PFNEGLUNBINDWAYLANDDISPLAYWLPROC
#define PFNEGLQUERYWAYLANDBUFFERWL PFNEGLQUERYWAYLANDBUFFERWLPROC
#define PFNEGLCREATEWAYLANDBUFFERFROMIMAGEWL PFNEGLCREATEWAYLANDBUFFERFROMIMAGEWLPROC
typedef unsigned int GLhandleARB;

#define EGL_ALPHA_FORMAT                                              0x3088
#define EGL_ALPHA_FORMAT_NONPRE                                       0x308B
#define EGL_ALPHA_FORMAT_PRE                                          0x308C
#define EGL_ALPHA_MASK_SIZE                                           0x303E
#define EGL_ALPHA_SIZE                                                0x3021
#define EGL_BACK_BUFFER                                               0x3084
#define EGL_BAD_ACCESS                                                0x3002
#define EGL_BAD_ALLOC                                                 0x3003
#define EGL_BAD_ATTRIBUTE                                             0x3004
#define EGL_BAD_CONFIG                                                0x3005
#define EGL_BAD_CONTEXT                                               0x3006
#define EGL_BAD_CURRENT_SURFACE                                       0x3007
#define EGL_BAD_DISPLAY                                               0x3008
#define EGL_BAD_MATCH                                                 0x3009
#define EGL_BAD_NATIVE_PIXMAP                                         0x300A
#define EGL_BAD_NATIVE_WINDOW                                         0x300B
#define EGL_BAD_PARAMETER                                             0x300C
#define EGL_BAD_SURFACE                                               0x300D
#define EGL_BIND_TO_TEXTURE_RGB                                       0x3039
#define EGL_BIND_TO_TEXTURE_RGBA                                      0x303A
#define EGL_BLUE_SIZE                                                 0x3022
#define EGL_BUFFER_DESTROYED                                          0x3095
#define EGL_BUFFER_PRESERVED                                          0x3094
#define EGL_BUFFER_SIZE                                               0x3020
#define EGL_CLIENT_APIS                                               0x308D
#define EGL_CL_EVENT_HANDLE                                           0x309C
#define EGL_COLORSPACE                                                0x3087
#define EGL_COLORSPACE_LINEAR                                         0x308A
#define EGL_COLORSPACE_sRGB                                           0x3089
#define EGL_COLOR_BUFFER_TYPE                                         0x303F
#define EGL_COLOR_COMPONENT_TYPE_EXT                                  0x3339
#define EGL_COLOR_COMPONENT_TYPE_FIXED_EXT                            0x333A
#define EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT                            0x333B
#define EGL_CONDITION_SATISFIED                                       0x30F6
#define EGL_CONFIG_CAVEAT                                             0x3027
#define EGL_CONFIG_ID                                                 0x3028
#define EGL_CONFORMANT                                                0x3042
#define EGL_CONTEXT_CLIENT_TYPE                                       0x3097
#define EGL_CONTEXT_CLIENT_VERSION                                    0x3098
#define EGL_CONTEXT_FLAGS_KHR                                         0x30FC
#define EGL_CONTEXT_LOST                                              0x300E
#define EGL_CONTEXT_MAJOR_VERSION                                     0x3098
#define EGL_CONTEXT_MAJOR_VERSION_KHR                                 0x3098
#define EGL_CONTEXT_MINOR_VERSION                                     0x30FB
#define EGL_CONTEXT_MINOR_VERSION_KHR                                 0x30FB
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT                  0x00000002
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR              0x00000002
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT                           0x00000001
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR                       0x00000001
#define EGL_CONTEXT_OPENGL_DEBUG                                      0x31B0
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR                              0x00000001
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE                         0x31B1
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR                 0x00000002
#define EGL_CONTEXT_OPENGL_NO_ERROR_KHR                               0x31B3
#define EGL_CONTEXT_OPENGL_PROFILE_MASK                               0x30FD
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR                           0x30FD
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY                0x31BD
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR            0x31BD
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS                              0x31B2
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR                      0x00000004
#define EGL_CORE_NATIVE_ENGINE                                        0x305B
#define EGL_DEFAULT_DISPLAY                                           EGL_CAST(EGLNativeDisplayType,0)
#define EGL_DEPTH_SIZE                                                0x3025
#define EGL_DISPLAY_SCALING                                           10000
#define EGL_DONT_CARE                                                 EGL_CAST(EGLint,-1)
#define EGL_DRAW                                                      0x3059
#define EGL_EXTENSIONS                                                0x3055
#define EGL_FALSE                                                     0
#define EGL_FOREVER                                                   0xFFFFFFFFFFFFFFFF
#define EGL_GL_COLORSPACE                                             0x309D
#define EGL_GL_COLORSPACE_LINEAR                                      0x308A
#define EGL_GL_COLORSPACE_SRGB                                        0x3089
#define EGL_GL_RENDERBUFFER                                           0x30B9
#define EGL_GL_TEXTURE_2D                                             0x30B1
#define EGL_GL_TEXTURE_3D                                             0x30B2
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X                            0x30B4
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y                            0x30B6
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z                            0x30B8
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X                            0x30B3
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y                            0x30B5
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z                            0x30B7
#define EGL_GL_TEXTURE_LEVEL                                          0x30BC
#define EGL_GL_TEXTURE_ZOFFSET                                        0x30BD
#define EGL_GREEN_SIZE                                                0x3023
#define EGL_HEIGHT                                                    0x3056
#define EGL_HORIZONTAL_RESOLUTION                                     0x3090
#define EGL_IMAGE_PRESERVED                                           0x30D2
#define EGL_LARGEST_PBUFFER                                           0x3058
#define EGL_LEVEL                                                     0x3029
#define EGL_LOSE_CONTEXT_ON_RESET                                     0x31BF
#define EGL_LOSE_CONTEXT_ON_RESET_KHR                                 0x31BF
#define EGL_LUMINANCE_BUFFER                                          0x308F
#define EGL_LUMINANCE_SIZE                                            0x303D
#define EGL_MATCH_NATIVE_PIXMAP                                       0x3041
#define EGL_MAX_PBUFFER_HEIGHT                                        0x302A
#define EGL_MAX_PBUFFER_PIXELS                                        0x302B
#define EGL_MAX_PBUFFER_WIDTH                                         0x302C
#define EGL_MAX_SWAP_INTERVAL                                         0x303C
#define EGL_MIN_SWAP_INTERVAL                                         0x303B
#define EGL_MIPMAP_LEVEL                                              0x3083
#define EGL_MIPMAP_TEXTURE                                            0x3082
#define EGL_MULTISAMPLE_RESOLVE                                       0x3099
#define EGL_MULTISAMPLE_RESOLVE_BOX                                   0x309B
#define EGL_MULTISAMPLE_RESOLVE_BOX_BIT                               0x0200
#define EGL_MULTISAMPLE_RESOLVE_DEFAULT                               0x309A
#define EGL_NATIVE_RENDERABLE                                         0x302D
#define EGL_NATIVE_VISUAL_ID                                          0x302E
#define EGL_NATIVE_VISUAL_TYPE                                        0x302F
#define EGL_NONE                                                      0x3038
#define EGL_NON_CONFORMANT_CONFIG                                     0x3051
#define EGL_NOT_INITIALIZED                                           0x3001
#define EGL_NO_CONFIG_KHR                                             EGL_CAST(EGLConfig,0)
#define EGL_NO_CONTEXT                                                EGL_CAST(EGLContext,0)
#define EGL_NO_DISPLAY                                                EGL_CAST(EGLDisplay,0)
#define EGL_NO_IMAGE                                                  EGL_CAST(EGLImage,0)
#define EGL_NO_RESET_NOTIFICATION                                     0x31BE
#define EGL_NO_RESET_NOTIFICATION_KHR                                 0x31BE
#define EGL_NO_SURFACE                                                EGL_CAST(EGLSurface,0)
#define EGL_NO_SYNC                                                   EGL_CAST(EGLSync,0)
#define EGL_NO_TEXTURE                                                0x305C
#define EGL_OPENGL_API                                                0x30A2
#define EGL_OPENGL_BIT                                                0x0008
#define EGL_OPENGL_ES2_BIT                                            0x0004
#define EGL_OPENGL_ES3_BIT                                            0x00000040
#define EGL_OPENGL_ES3_BIT_KHR                                        0x00000040
#define EGL_OPENGL_ES_API                                             0x30A0
#define EGL_OPENGL_ES_BIT                                             0x0001
#define EGL_OPENVG_API                                                0x30A1
#define EGL_OPENVG_BIT                                                0x0002
#define EGL_OPENVG_IMAGE                                              0x3096
#define EGL_PBUFFER_BIT                                               0x0001
#define EGL_PIXEL_ASPECT_RATIO                                        0x3092
#define EGL_PIXMAP_BIT                                                0x0002
#define EGL_PLATFORM_ANDROID_KHR                                      0x3141
#define EGL_PLATFORM_SURFACELESS_MESA                                 0x31DD
#define EGL_PLATFORM_WAYLAND_KHR                                      0x31D8
#define EGL_PLATFORM_X11_KHR                                          0x31D5
#define EGL_PLATFORM_X11_SCREEN_KHR                                   0x31D6
#define EGL_PRESENT_OPAQUE_EXT                                        0x31DF
#define EGL_READ                                                      0x305A
#define EGL_RED_SIZE                                                  0x3024
#define EGL_RENDERABLE_TYPE                                           0x3040
#define EGL_RENDER_BUFFER                                             0x3086
#define EGL_RGB_BUFFER                                                0x308E
#define EGL_SAMPLES                                                   0x3031
#define EGL_SAMPLE_BUFFERS                                            0x3032
#define EGL_SIGNALED                                                  0x30F2
#define EGL_SINGLE_BUFFER                                             0x3085
#define EGL_SLOW_CONFIG                                               0x3050
#define EGL_STENCIL_SIZE                                              0x3026
#define EGL_SUCCESS                                                   0x3000
#define EGL_SURFACE_TYPE                                              0x3033
#define EGL_SWAP_BEHAVIOR                                             0x3093
#define EGL_SWAP_BEHAVIOR_PRESERVED_BIT                               0x0400
#define EGL_SYNC_CL_EVENT                                             0x30FE
#define EGL_SYNC_CL_EVENT_COMPLETE                                    0x30FF
#define EGL_SYNC_CONDITION                                            0x30F8
#define EGL_SYNC_FENCE                                                0x30F9
#define EGL_SYNC_FLUSH_COMMANDS_BIT                                   0x0001
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE                              0x30F0
#define EGL_SYNC_STATUS                                               0x30F1
#define EGL_SYNC_TYPE                                                 0x30F7
#define EGL_TEXTURE_2D                                                0x305F
#define EGL_TEXTURE_FORMAT                                            0x3080
#define EGL_TEXTURE_RGB                                               0x305D
#define EGL_TEXTURE_RGBA                                              0x305E
#define EGL_TEXTURE_TARGET                                            0x3081
#define EGL_TIMEOUT_EXPIRED                                           0x30F5
#define EGL_TRANSPARENT_BLUE_VALUE                                    0x3035
#define EGL_TRANSPARENT_GREEN_VALUE                                   0x3036
#define EGL_TRANSPARENT_RED_VALUE                                     0x3037
#define EGL_TRANSPARENT_RGB                                           0x3052
#define EGL_TRANSPARENT_TYPE                                          0x3034
#define EGL_TRUE                                                      1
#define EGL_UNKNOWN                                                   EGL_CAST(EGLint,-1)
#define EGL_UNSIGNALED                                                0x30F3
#define EGL_VENDOR                                                    0x3053
#define EGL_VERSION                                                   0x3054
#define EGL_VERTICAL_RESOLUTION                                       0x3091
#define EGL_VG_ALPHA_FORMAT                                           0x3088
#define EGL_VG_ALPHA_FORMAT_NONPRE                                    0x308B
#define EGL_VG_ALPHA_FORMAT_PRE                                       0x308C
#define EGL_VG_ALPHA_FORMAT_PRE_BIT                                   0x0040
#define EGL_VG_COLORSPACE                                             0x3087
#define EGL_VG_COLORSPACE_LINEAR                                      0x308A
#define EGL_VG_COLORSPACE_LINEAR_BIT                                  0x0020
#define EGL_VG_COLORSPACE_sRGB                                        0x3089
#define EGL_WIDTH                                                     0x3057
#define EGL_WINDOW_BIT                                                0x0004
#define ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB                        0x2054
#define ERROR_INVALID_PIXEL_TYPE_ARB                                  0x2043
#define ERROR_INVALID_PROFILE_ARB                                     0x2096
#define ERROR_INVALID_VERSION_ARB                                     0x2095
#define GL_1PASS_EXT                                                  0x80A1
#define GL_1PASS_SGIS                                                 0x80A1
#define GL_2D                                                         0x0600
#define GL_2PASS_0_EXT                                                0x80A2
#define GL_2PASS_0_SGIS                                               0x80A2
#define GL_2PASS_1_EXT                                                0x80A3
#define GL_2PASS_1_SGIS                                               0x80A3
#define GL_2X_BIT_ATI                                                 0x00000001
#define GL_2_BYTES                                                    0x1407
#define GL_2_BYTES_NV                                                 0x1407
#define GL_3D                                                         0x0601
#define GL_3D_COLOR                                                   0x0602
#define GL_3D_COLOR_TEXTURE                                           0x0603
#define GL_3_BYTES                                                    0x1408
#define GL_3_BYTES_NV                                                 0x1408
#define GL_422_AVERAGE_EXT                                            0x80CE
#define GL_422_EXT                                                    0x80CC
#define GL_422_REV_AVERAGE_EXT                                        0x80CF
#define GL_422_REV_EXT                                                0x80CD
#define GL_4D_COLOR_TEXTURE                                           0x0604
#define GL_4PASS_0_EXT                                                0x80A4
#define GL_4PASS_0_SGIS                                               0x80A4
#define GL_4PASS_1_EXT                                                0x80A5
#define GL_4PASS_1_SGIS                                               0x80A5
#define GL_4PASS_2_EXT                                                0x80A6
#define GL_4PASS_2_SGIS                                               0x80A6
#define GL_4PASS_3_EXT                                                0x80A7
#define GL_4PASS_3_SGIS                                               0x80A7
#define GL_4X_BIT_ATI                                                 0x00000002
#define GL_4_BYTES                                                    0x1409
#define GL_4_BYTES_NV                                                 0x1409
#define GL_8X_BIT_ATI                                                 0x00000004
#define GL_ABGR_EXT                                                   0x8000
#define GL_ACCUM                                                      0x0100
#define GL_ACCUM_ADJACENT_PAIRS_NV                                    0x90AD
#define GL_ACCUM_ALPHA_BITS                                           0x0D5B
#define GL_ACCUM_BLUE_BITS                                            0x0D5A
#define GL_ACCUM_BUFFER_BIT                                           0x00000200
#define GL_ACCUM_CLEAR_VALUE                                          0x0B80
#define GL_ACCUM_GREEN_BITS                                           0x0D59
#define GL_ACCUM_RED_BITS                                             0x0D58
#define GL_ACTIVE_ATOMIC_COUNTER_BUFFERS                              0x92D9
#define GL_ACTIVE_ATTRIBUTES                                          0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH                                0x8B8A
#define GL_ACTIVE_PROGRAM                                             0x8259
#define GL_ACTIVE_PROGRAM_EXT                                         0x8B8D
#define GL_ACTIVE_RESOURCES                                           0x92F5
#define GL_ACTIVE_STENCIL_FACE_EXT                                    0x8911
#define GL_ACTIVE_SUBROUTINES                                         0x8DE5
#define GL_ACTIVE_SUBROUTINE_MAX_LENGTH                               0x8E48
#define GL_ACTIVE_SUBROUTINE_UNIFORMS                                 0x8DE6
#define GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS                        0x8E47
#define GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH                       0x8E49
#define GL_ACTIVE_TEXTURE                                             0x84E0
#define GL_ACTIVE_TEXTURE_ARB                                         0x84E0
#define GL_ACTIVE_UNIFORMS                                            0x8B86
#define GL_ACTIVE_UNIFORM_BLOCKS                                      0x8A36
#define GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH                       0x8A35
#define GL_ACTIVE_UNIFORM_MAX_LENGTH                                  0x8B87
#define GL_ACTIVE_VARIABLES                                           0x9305
#define GL_ACTIVE_VARYINGS_NV                                         0x8C81
#define GL_ACTIVE_VARYING_MAX_LENGTH_NV                               0x8C82
#define GL_ACTIVE_VERTEX_UNITS_ARB                                    0x86A5
#define GL_ADD                                                        0x0104
#define GL_ADD_ATI                                                    0x8963
#define GL_ADD_SIGNED                                                 0x8574
#define GL_ADD_SIGNED_ARB                                             0x8574
#define GL_ADD_SIGNED_EXT                                             0x8574
#define GL_ADJACENT_PAIRS_NV                                          0x90AE
#define GL_AFFINE_2D_NV                                               0x9092
#define GL_AFFINE_3D_NV                                               0x9094
#define GL_ALIASED_LINE_WIDTH_RANGE                                   0x846E
#define GL_ALIASED_POINT_SIZE_RANGE                                   0x846D
#define GL_ALLOW_DRAW_FRG_HINT_PGI                                    0x1A210
#define GL_ALLOW_DRAW_MEM_HINT_PGI                                    0x1A211
#define GL_ALLOW_DRAW_OBJ_HINT_PGI                                    0x1A20E
#define GL_ALLOW_DRAW_WIN_HINT_PGI                                    0x1A20F
#define GL_ALL_ATTRIB_BITS                                            0xFFFFFFFF
#define GL_ALL_BARRIER_BITS                                           0xFFFFFFFF
#define GL_ALL_BARRIER_BITS_EXT                                       0xFFFFFFFF
#define GL_ALL_COMPLETED_NV                                           0x84F2
#define GL_ALL_PIXELS_AMD                                             0xFFFFFFFF
#define GL_ALL_SHADER_BITS                                            0xFFFFFFFF
#define GL_ALL_SHADER_BITS_EXT                                        0xFFFFFFFF
#define GL_ALL_STATIC_DATA_IBM                                        103060
#define GL_ALPHA                                                      0x1906
#define GL_ALPHA12                                                    0x803D
#define GL_ALPHA12_EXT                                                0x803D
#define GL_ALPHA16                                                    0x803E
#define GL_ALPHA16F_ARB                                               0x881C
#define GL_ALPHA16I_EXT                                               0x8D8A
#define GL_ALPHA16UI_EXT                                              0x8D78
#define GL_ALPHA16_EXT                                                0x803E
#define GL_ALPHA16_SNORM                                              0x9018
#define GL_ALPHA32F_ARB                                               0x8816
#define GL_ALPHA32I_EXT                                               0x8D84
#define GL_ALPHA32UI_EXT                                              0x8D72
#define GL_ALPHA4                                                     0x803B
#define GL_ALPHA4_EXT                                                 0x803B
#define GL_ALPHA8                                                     0x803C
#define GL_ALPHA8I_EXT                                                0x8D90
#define GL_ALPHA8UI_EXT                                               0x8D7E
#define GL_ALPHA8_EXT                                                 0x803C
#define GL_ALPHA8_SNORM                                               0x9014
#define GL_ALPHA_BIAS                                                 0x0D1D
#define GL_ALPHA_BITS                                                 0x0D55
#define GL_ALPHA_FLOAT16_APPLE                                        0x881C
#define GL_ALPHA_FLOAT16_ATI                                          0x881C
#define GL_ALPHA_FLOAT32_APPLE                                        0x8816
#define GL_ALPHA_FLOAT32_ATI                                          0x8816
#define GL_ALPHA_INTEGER                                              0x8D97
#define GL_ALPHA_INTEGER_EXT                                          0x8D97
#define GL_ALPHA_MAX_CLAMP_INGR                                       0x8567
#define GL_ALPHA_MAX_SGIX                                             0x8321
#define GL_ALPHA_MIN_CLAMP_INGR                                       0x8563
#define GL_ALPHA_MIN_SGIX                                             0x8320
#define GL_ALPHA_REF_COMMAND_NV                                       0x000F
#define GL_ALPHA_SCALE                                                0x0D1C
#define GL_ALPHA_SNORM                                                0x9010
#define GL_ALPHA_TEST                                                 0x0BC0
#define GL_ALPHA_TEST_FUNC                                            0x0BC1
#define GL_ALPHA_TEST_REF                                             0x0BC2
#define GL_ALPHA_TO_COVERAGE_DITHER_DEFAULT_NV                        0x934D
#define GL_ALPHA_TO_COVERAGE_DITHER_DISABLE_NV                        0x934F
#define GL_ALPHA_TO_COVERAGE_DITHER_ENABLE_NV                         0x934E
#define GL_ALPHA_TO_COVERAGE_DITHER_MODE_NV                           0x92BF
#define GL_ALREADY_SIGNALED                                           0x911A
#define GL_ALWAYS                                                     0x0207
#define GL_ALWAYS_FAST_HINT_PGI                                       0x1A20C
#define GL_ALWAYS_SOFT_HINT_PGI                                       0x1A20D
#define GL_AMBIENT                                                    0x1200
#define GL_AMBIENT_AND_DIFFUSE                                        0x1602
#define GL_AND                                                        0x1501
#define GL_AND_INVERTED                                               0x1504
#define GL_AND_REVERSE                                                0x1502
#define GL_ANY_SAMPLES_PASSED                                         0x8C2F
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE                            0x8D6A
#define GL_ARC_TO_NV                                                  0xFE
#define GL_ARRAY_BUFFER                                               0x8892
#define GL_ARRAY_BUFFER_ARB                                           0x8892
#define GL_ARRAY_BUFFER_BINDING                                       0x8894
#define GL_ARRAY_BUFFER_BINDING_ARB                                   0x8894
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT                               0x81A9
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT                               0x81A8
#define GL_ARRAY_OBJECT_BUFFER_ATI                                    0x8766
#define GL_ARRAY_OBJECT_OFFSET_ATI                                    0x8767
#define GL_ARRAY_SIZE                                                 0x92FB
#define GL_ARRAY_STRIDE                                               0x92FE
#define GL_ASYNC_DRAW_PIXELS_SGIX                                     0x835D
#define GL_ASYNC_HISTOGRAM_SGIX                                       0x832C
#define GL_ASYNC_MARKER_SGIX                                          0x8329
#define GL_ASYNC_READ_PIXELS_SGIX                                     0x835E
#define GL_ASYNC_TEX_IMAGE_SGIX                                       0x835C
#define GL_ATOMIC_COUNTER_BARRIER_BIT                                 0x00001000
#define GL_ATOMIC_COUNTER_BARRIER_BIT_EXT                             0x00001000
#define GL_ATOMIC_COUNTER_BUFFER                                      0x92C0
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS               0x92C5
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES        0x92C6
#define GL_ATOMIC_COUNTER_BUFFER_BINDING                              0x92C1
#define GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE                            0x92C4
#define GL_ATOMIC_COUNTER_BUFFER_INDEX                                0x9301
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER         0x90ED
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_FRAGMENT_SHADER        0x92CB
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_GEOMETRY_SHADER        0x92CA
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_MESH_SHADER_NV         0x959E
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TASK_SHADER_NV         0x959F
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_CONTROL_SHADER    0x92C8
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_EVALUATION_SHADER 0x92C9
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_VERTEX_SHADER          0x92C7
#define GL_ATOMIC_COUNTER_BUFFER_SIZE                                 0x92C3
#define GL_ATOMIC_COUNTER_BUFFER_START                                0x92C2
#define GL_ATTACHED_MEMORY_OBJECT_NV                                  0x95A4
#define GL_ATTACHED_MEMORY_OFFSET_NV                                  0x95A5
#define GL_ATTACHED_SHADERS                                           0x8B85
#define GL_ATTENUATION_EXT                                            0x834D
#define GL_ATTRIBUTE_ADDRESS_COMMAND_NV                               0x0009
#define GL_ATTRIB_ARRAY_POINTER_NV                                    0x8645
#define GL_ATTRIB_ARRAY_SIZE_NV                                       0x8623
#define GL_ATTRIB_ARRAY_STRIDE_NV                                     0x8624
#define GL_ATTRIB_ARRAY_TYPE_NV                                       0x8625
#define GL_ATTRIB_STACK_DEPTH                                         0x0BB0
#define GL_AUTO_GENERATE_MIPMAP                                       0x8295
#define GL_AUTO_NORMAL                                                0x0D80
#define GL_AUX0                                                       0x0409
#define GL_AUX1                                                       0x040A
#define GL_AUX2                                                       0x040B
#define GL_AUX3                                                       0x040C
#define GL_AUX_BUFFERS                                                0x0C00
#define GL_AUX_DEPTH_STENCIL_APPLE                                    0x8A14
#define GL_AVERAGE_EXT                                                0x8335
#define GL_AVERAGE_HP                                                 0x8160
#define GL_BACK                                                       0x0405
#define GL_BACK_LEFT                                                  0x0402
#define GL_BACK_NORMALS_HINT_PGI                                      0x1A223
#define GL_BACK_PRIMARY_COLOR_NV                                      0x8C77
#define GL_BACK_RIGHT                                                 0x0403
#define GL_BACK_SECONDARY_COLOR_NV                                    0x8C78
#define GL_BEVEL_NV                                                   0x90A6
#define GL_BGR                                                        0x80E0
#define GL_BGRA                                                       0x80E1
#define GL_BGRA_EXT                                                   0x80E1
#define GL_BGRA_INTEGER                                               0x8D9B
#define GL_BGRA_INTEGER_EXT                                           0x8D9B
#define GL_BGR_EXT                                                    0x80E0
#define GL_BGR_INTEGER                                                0x8D9A
#define GL_BGR_INTEGER_EXT                                            0x8D9A
#define GL_BIAS_BIT_ATI                                               0x00000008
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV                               0x8541
#define GL_BINORMAL_ARRAY_EXT                                         0x843A
#define GL_BINORMAL_ARRAY_POINTER_EXT                                 0x8443
#define GL_BINORMAL_ARRAY_STRIDE_EXT                                  0x8441
#define GL_BINORMAL_ARRAY_TYPE_EXT                                    0x8440
#define GL_BITMAP                                                     0x1A00
#define GL_BITMAP_TOKEN                                               0x0704
#define GL_BLACKHOLE_RENDER_INTEL                                     0x83FC
#define GL_BLEND                                                      0x0BE2
#define GL_BLEND_ADVANCED_COHERENT_KHR                                0x9285
#define GL_BLEND_ADVANCED_COHERENT_NV                                 0x9285
#define GL_BLEND_COLOR                                                0x8005
#define GL_BLEND_COLOR_COMMAND_NV                                     0x000B
#define GL_BLEND_COLOR_EXT                                            0x8005
#define GL_BLEND_DST                                                  0x0BE0
#define GL_BLEND_DST_ALPHA                                            0x80CA
#define GL_BLEND_DST_ALPHA_EXT                                        0x80CA
#define GL_BLEND_DST_RGB                                              0x80C8
#define GL_BLEND_DST_RGB_EXT                                          0x80C8
#define GL_BLEND_EQUATION                                             0x8009
#define GL_BLEND_EQUATION_ALPHA                                       0x883D
#define GL_BLEND_EQUATION_ALPHA_EXT                                   0x883D
#define GL_BLEND_EQUATION_EXT                                         0x8009
#define GL_BLEND_EQUATION_RGB                                         0x8009
#define GL_BLEND_EQUATION_RGB_EXT                                     0x8009
#define GL_BLEND_OVERLAP_NV                                           0x9281
#define GL_BLEND_PREMULTIPLIED_SRC_NV                                 0x9280
#define GL_BLEND_SRC                                                  0x0BE1
#define GL_BLEND_SRC_ALPHA                                            0x80CB
#define GL_BLEND_SRC_ALPHA_EXT                                        0x80CB
#define GL_BLEND_SRC_RGB                                              0x80C9
#define GL_BLEND_SRC_RGB_EXT                                          0x80C9
#define GL_BLOCK_INDEX                                                0x92FD
#define GL_BLUE                                                       0x1905
#define GL_BLUE_BIAS                                                  0x0D1B
#define GL_BLUE_BITS                                                  0x0D54
#define GL_BLUE_BIT_ATI                                               0x00000004
#define GL_BLUE_INTEGER                                               0x8D96
#define GL_BLUE_INTEGER_EXT                                           0x8D96
#define GL_BLUE_MAX_CLAMP_INGR                                        0x8566
#define GL_BLUE_MIN_CLAMP_INGR                                        0x8562
#define GL_BLUE_NV                                                    0x1905
#define GL_BLUE_SCALE                                                 0x0D1A
#define GL_BOLD_BIT_NV                                                0x01
#define GL_BOOL                                                       0x8B56
#define GL_BOOL_ARB                                                   0x8B56
#define GL_BOOL_VEC2                                                  0x8B57
#define GL_BOOL_VEC2_ARB                                              0x8B57
#define GL_BOOL_VEC3                                                  0x8B58
#define GL_BOOL_VEC3_ARB                                              0x8B58
#define GL_BOOL_VEC4                                                  0x8B59
#define GL_BOOL_VEC4_ARB                                              0x8B59
#define GL_BOUNDING_BOX_NV                                            0x908D
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV                          0x909C
#define GL_BUFFER                                                     0x82E0
#define GL_BUFFER_ACCESS                                              0x88BB
#define GL_BUFFER_ACCESS_ARB                                          0x88BB
#define GL_BUFFER_ACCESS_FLAGS                                        0x911F
#define GL_BUFFER_BINDING                                             0x9302
#define GL_BUFFER_DATA_SIZE                                           0x9303
#define GL_BUFFER_FLUSHING_UNMAP_APPLE                                0x8A13
#define GL_BUFFER_GPU_ADDRESS_NV                                      0x8F1D
#define GL_BUFFER_IMMUTABLE_STORAGE                                   0x821F
#define GL_BUFFER_KHR                                                 0x82E0
#define GL_BUFFER_MAPPED                                              0x88BC
#define GL_BUFFER_MAPPED_ARB                                          0x88BC
#define GL_BUFFER_MAP_LENGTH                                          0x9120
#define GL_BUFFER_MAP_OFFSET                                          0x9121
#define GL_BUFFER_MAP_POINTER                                         0x88BD
#define GL_BUFFER_MAP_POINTER_ARB                                     0x88BD
#define GL_BUFFER_OBJECT_APPLE                                        0x85B3
#define GL_BUFFER_OBJECT_EXT                                          0x9151
#define GL_BUFFER_SERIALIZED_MODIFY_APPLE                             0x8A12
#define GL_BUFFER_SIZE                                                0x8764
#define GL_BUFFER_SIZE_ARB                                            0x8764
#define GL_BUFFER_STORAGE_FLAGS                                       0x8220
#define GL_BUFFER_UPDATE_BARRIER_BIT                                  0x00000200
#define GL_BUFFER_UPDATE_BARRIER_BIT_EXT                              0x00000200
#define GL_BUFFER_USAGE                                               0x8765
#define GL_BUFFER_USAGE_ARB                                           0x8765
#define GL_BUFFER_VARIABLE                                            0x92E5
#define GL_BUMP_ENVMAP_ATI                                            0x877B
#define GL_BUMP_NUM_TEX_UNITS_ATI                                     0x8777
#define GL_BUMP_ROT_MATRIX_ATI                                        0x8775
#define GL_BUMP_ROT_MATRIX_SIZE_ATI                                   0x8776
#define GL_BUMP_TARGET_ATI                                            0x877C
#define GL_BUMP_TEX_UNITS_ATI                                         0x8778
#define GL_BYTE                                                       0x1400
#define GL_C3F_V3F                                                    0x2A24
#define GL_C4F_N3F_V3F                                                0x2A26
#define GL_C4UB_V2F                                                   0x2A22
#define GL_C4UB_V3F                                                   0x2A23
#define GL_CALLIGRAPHIC_FRAGMENT_SGIX                                 0x8183
#define GL_CAVEAT_SUPPORT                                             0x82B8
#define GL_CCW                                                        0x0901
#define GL_CIRCULAR_CCW_ARC_TO_NV                                     0xF8
#define GL_CIRCULAR_CW_ARC_TO_NV                                      0xFA
#define GL_CIRCULAR_TANGENT_ARC_TO_NV                                 0xFC
#define GL_CLAMP                                                      0x2900
#define GL_CLAMP_FRAGMENT_COLOR                                       0x891B
#define GL_CLAMP_FRAGMENT_COLOR_ARB                                   0x891B
#define GL_CLAMP_READ_COLOR                                           0x891C
#define GL_CLAMP_READ_COLOR_ARB                                       0x891C
#define GL_CLAMP_TO_BORDER                                            0x812D
#define GL_CLAMP_TO_BORDER_ARB                                        0x812D
#define GL_CLAMP_TO_BORDER_SGIS                                       0x812D
#define GL_CLAMP_TO_EDGE                                              0x812F
#define GL_CLAMP_TO_EDGE_SGIS                                         0x812F
#define GL_CLAMP_VERTEX_COLOR                                         0x891A
#define GL_CLAMP_VERTEX_COLOR_ARB                                     0x891A
#define GL_CLEAR                                                      0x1500
#define GL_CLEAR_BUFFER                                               0x82B4
#define GL_CLEAR_TEXTURE                                              0x9365
#define GL_CLIENT_ACTIVE_TEXTURE                                      0x84E1
#define GL_CLIENT_ACTIVE_TEXTURE_ARB                                  0x84E1
#define GL_CLIENT_ALL_ATTRIB_BITS                                     0xFFFFFFFF
#define GL_CLIENT_ATTRIB_STACK_DEPTH                                  0x0BB1
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT                           0x00004000
#define GL_CLIENT_PIXEL_STORE_BIT                                     0x00000001
#define GL_CLIENT_STORAGE_BIT                                         0x0200
#define GL_CLIENT_VERTEX_ARRAY_BIT                                    0x00000002
#define GL_CLIPPING_INPUT_PRIMITIVES                                  0x82F6
#define GL_CLIPPING_INPUT_PRIMITIVES_ARB                              0x82F6
#define GL_CLIPPING_OUTPUT_PRIMITIVES                                 0x82F7
#define GL_CLIPPING_OUTPUT_PRIMITIVES_ARB                             0x82F7
#define GL_CLIP_DEPTH_MODE                                            0x935D
#define GL_CLIP_DISTANCE0                                             0x3000
#define GL_CLIP_DISTANCE1                                             0x3001
#define GL_CLIP_DISTANCE2                                             0x3002
#define GL_CLIP_DISTANCE3                                             0x3003
#define GL_CLIP_DISTANCE4                                             0x3004
#define GL_CLIP_DISTANCE5                                             0x3005
#define GL_CLIP_DISTANCE6                                             0x3006
#define GL_CLIP_DISTANCE7                                             0x3007
#define GL_CLIP_DISTANCE_NV                                           0x8C7A
#define GL_CLIP_FAR_HINT_PGI                                          0x1A221
#define GL_CLIP_NEAR_HINT_PGI                                         0x1A220
#define GL_CLIP_ORIGIN                                                0x935C
#define GL_CLIP_PLANE0                                                0x3000
#define GL_CLIP_PLANE1                                                0x3001
#define GL_CLIP_PLANE2                                                0x3002
#define GL_CLIP_PLANE3                                                0x3003
#define GL_CLIP_PLANE4                                                0x3004
#define GL_CLIP_PLANE5                                                0x3005
#define GL_CLIP_VOLUME_CLIPPING_HINT_EXT                              0x80F0
#define GL_CLOSE_PATH_NV                                              0x00
#define GL_CMYKA_EXT                                                  0x800D
#define GL_CMYK_EXT                                                   0x800C
#define GL_CND0_ATI                                                   0x896B
#define GL_CND_ATI                                                    0x896A
#define GL_COEFF                                                      0x0A00
#define GL_COLOR                                                      0x1800
#define GL_COLOR3_BIT_PGI                                             0x00010000
#define GL_COLOR4_BIT_PGI                                             0x00020000
#define GL_COLORBURN_KHR                                              0x929A
#define GL_COLORBURN_NV                                               0x929A
#define GL_COLORDODGE_KHR                                             0x9299
#define GL_COLORDODGE_NV                                              0x9299
#define GL_COLOR_ALPHA_PAIRING_ATI                                    0x8975
#define GL_COLOR_ARRAY                                                0x8076
#define GL_COLOR_ARRAY_ADDRESS_NV                                     0x8F23
#define GL_COLOR_ARRAY_BUFFER_BINDING                                 0x8898
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB                             0x8898
#define GL_COLOR_ARRAY_COUNT_EXT                                      0x8084
#define GL_COLOR_ARRAY_EXT                                            0x8076
#define GL_COLOR_ARRAY_LENGTH_NV                                      0x8F2D
#define GL_COLOR_ARRAY_LIST_IBM                                       103072
#define GL_COLOR_ARRAY_LIST_STRIDE_IBM                                103082
#define GL_COLOR_ARRAY_PARALLEL_POINTERS_INTEL                        0x83F7
#define GL_COLOR_ARRAY_POINTER                                        0x8090
#define GL_COLOR_ARRAY_POINTER_EXT                                    0x8090
#define GL_COLOR_ARRAY_SIZE                                           0x8081
#define GL_COLOR_ARRAY_SIZE_EXT                                       0x8081
#define GL_COLOR_ARRAY_STRIDE                                         0x8083
#define GL_COLOR_ARRAY_STRIDE_EXT                                     0x8083
#define GL_COLOR_ARRAY_TYPE                                           0x8082
#define GL_COLOR_ARRAY_TYPE_EXT                                       0x8082
#define GL_COLOR_ATTACHMENT0                                          0x8CE0
#define GL_COLOR_ATTACHMENT0_EXT                                      0x8CE0
#define GL_COLOR_ATTACHMENT1                                          0x8CE1
#define GL_COLOR_ATTACHMENT10                                         0x8CEA
#define GL_COLOR_ATTACHMENT10_EXT                                     0x8CEA
#define GL_COLOR_ATTACHMENT11                                         0x8CEB
#define GL_COLOR_ATTACHMENT11_EXT                                     0x8CEB
#define GL_COLOR_ATTACHMENT12                                         0x8CEC
#define GL_COLOR_ATTACHMENT12_EXT                                     0x8CEC
#define GL_COLOR_ATTACHMENT13                                         0x8CED
#define GL_COLOR_ATTACHMENT13_EXT                                     0x8CED
#define GL_COLOR_ATTACHMENT14                                         0x8CEE
#define GL_COLOR_ATTACHMENT14_EXT                                     0x8CEE
#define GL_COLOR_ATTACHMENT15                                         0x8CEF
#define GL_COLOR_ATTACHMENT15_EXT                                     0x8CEF
#define GL_COLOR_ATTACHMENT16                                         0x8CF0
#define GL_COLOR_ATTACHMENT17                                         0x8CF1
#define GL_COLOR_ATTACHMENT18                                         0x8CF2
#define GL_COLOR_ATTACHMENT19                                         0x8CF3
#define GL_COLOR_ATTACHMENT1_EXT                                      0x8CE1
#define GL_COLOR_ATTACHMENT2                                          0x8CE2
#define GL_COLOR_ATTACHMENT20                                         0x8CF4
#define GL_COLOR_ATTACHMENT21                                         0x8CF5
#define GL_COLOR_ATTACHMENT22                                         0x8CF6
#define GL_COLOR_ATTACHMENT23                                         0x8CF7
#define GL_COLOR_ATTACHMENT24                                         0x8CF8
#define GL_COLOR_ATTACHMENT25                                         0x8CF9
#define GL_COLOR_ATTACHMENT26                                         0x8CFA
#define GL_COLOR_ATTACHMENT27                                         0x8CFB
#define GL_COLOR_ATTACHMENT28                                         0x8CFC
#define GL_COLOR_ATTACHMENT29                                         0x8CFD
#define GL_COLOR_ATTACHMENT2_EXT                                      0x8CE2
#define GL_COLOR_ATTACHMENT3                                          0x8CE3
#define GL_COLOR_ATTACHMENT30                                         0x8CFE
#define GL_COLOR_ATTACHMENT31                                         0x8CFF
#define GL_COLOR_ATTACHMENT3_EXT                                      0x8CE3
#define GL_COLOR_ATTACHMENT4                                          0x8CE4
#define GL_COLOR_ATTACHMENT4_EXT                                      0x8CE4
#define GL_COLOR_ATTACHMENT5                                          0x8CE5
#define GL_COLOR_ATTACHMENT5_EXT                                      0x8CE5
#define GL_COLOR_ATTACHMENT6                                          0x8CE6
#define GL_COLOR_ATTACHMENT6_EXT                                      0x8CE6
#define GL_COLOR_ATTACHMENT7                                          0x8CE7
#define GL_COLOR_ATTACHMENT7_EXT                                      0x8CE7
#define GL_COLOR_ATTACHMENT8                                          0x8CE8
#define GL_COLOR_ATTACHMENT8_EXT                                      0x8CE8
#define GL_COLOR_ATTACHMENT9                                          0x8CE9
#define GL_COLOR_ATTACHMENT9_EXT                                      0x8CE9
#define GL_COLOR_BUFFER_BIT                                           0x00004000
#define GL_COLOR_CLEAR_UNCLAMPED_VALUE_ATI                            0x8835
#define GL_COLOR_CLEAR_VALUE                                          0x0C22
#define GL_COLOR_COMPONENTS                                           0x8283
#define GL_COLOR_ENCODING                                             0x8296
#define GL_COLOR_FLOAT_APPLE                                          0x8A0F
#define GL_COLOR_INDEX                                                0x1900
#define GL_COLOR_INDEX12_EXT                                          0x80E6
#define GL_COLOR_INDEX16_EXT                                          0x80E7
#define GL_COLOR_INDEX1_EXT                                           0x80E2
#define GL_COLOR_INDEX2_EXT                                           0x80E3
#define GL_COLOR_INDEX4_EXT                                           0x80E4
#define GL_COLOR_INDEX8_EXT                                           0x80E5
#define GL_COLOR_INDEXES                                              0x1603
#define GL_COLOR_LOGIC_OP                                             0x0BF2
#define GL_COLOR_MATERIAL                                             0x0B57
#define GL_COLOR_MATERIAL_FACE                                        0x0B55
#define GL_COLOR_MATERIAL_PARAMETER                                   0x0B56
#define GL_COLOR_MATRIX                                               0x80B1
#define GL_COLOR_MATRIX_SGI                                           0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH                                   0x80B2
#define GL_COLOR_MATRIX_STACK_DEPTH_SGI                               0x80B2
#define GL_COLOR_RENDERABLE                                           0x8286
#define GL_COLOR_SAMPLES_NV                                           0x8E20
#define GL_COLOR_SUM                                                  0x8458
#define GL_COLOR_SUM_ARB                                              0x8458
#define GL_COLOR_SUM_CLAMP_NV                                         0x854F
#define GL_COLOR_SUM_EXT                                              0x8458
#define GL_COLOR_TABLE                                                0x80D0
#define GL_COLOR_TABLE_ALPHA_SIZE                                     0x80DD
#define GL_COLOR_TABLE_ALPHA_SIZE_SGI                                 0x80DD
#define GL_COLOR_TABLE_BIAS                                           0x80D7
#define GL_COLOR_TABLE_BIAS_SGI                                       0x80D7
#define GL_COLOR_TABLE_BLUE_SIZE                                      0x80DC
#define GL_COLOR_TABLE_BLUE_SIZE_SGI                                  0x80DC
#define GL_COLOR_TABLE_FORMAT                                         0x80D8
#define GL_COLOR_TABLE_FORMAT_SGI                                     0x80D8
#define GL_COLOR_TABLE_GREEN_SIZE                                     0x80DB
#define GL_COLOR_TABLE_GREEN_SIZE_SGI                                 0x80DB
#define GL_COLOR_TABLE_INTENSITY_SIZE                                 0x80DF
#define GL_COLOR_TABLE_INTENSITY_SIZE_SGI                             0x80DF
#define GL_COLOR_TABLE_LUMINANCE_SIZE                                 0x80DE
#define GL_COLOR_TABLE_LUMINANCE_SIZE_SGI                             0x80DE
#define GL_COLOR_TABLE_RED_SIZE                                       0x80DA
#define GL_COLOR_TABLE_RED_SIZE_SGI                                   0x80DA
#define GL_COLOR_TABLE_SCALE                                          0x80D6
#define GL_COLOR_TABLE_SCALE_SGI                                      0x80D6
#define GL_COLOR_TABLE_SGI                                            0x80D0
#define GL_COLOR_TABLE_WIDTH                                          0x80D9
#define GL_COLOR_TABLE_WIDTH_SGI                                      0x80D9
#define GL_COLOR_WRITEMASK                                            0x0C23
#define GL_COMBINE                                                    0x8570
#define GL_COMBINE4_NV                                                0x8503
#define GL_COMBINER0_NV                                               0x8550
#define GL_COMBINER1_NV                                               0x8551
#define GL_COMBINER2_NV                                               0x8552
#define GL_COMBINER3_NV                                               0x8553
#define GL_COMBINER4_NV                                               0x8554
#define GL_COMBINER5_NV                                               0x8555
#define GL_COMBINER6_NV                                               0x8556
#define GL_COMBINER7_NV                                               0x8557
#define GL_COMBINER_AB_DOT_PRODUCT_NV                                 0x8545
#define GL_COMBINER_AB_OUTPUT_NV                                      0x854A
#define GL_COMBINER_BIAS_NV                                           0x8549
#define GL_COMBINER_CD_DOT_PRODUCT_NV                                 0x8546
#define GL_COMBINER_CD_OUTPUT_NV                                      0x854B
#define GL_COMBINER_COMPONENT_USAGE_NV                                0x8544
#define GL_COMBINER_INPUT_NV                                          0x8542
#define GL_COMBINER_MAPPING_NV                                        0x8543
#define GL_COMBINER_MUX_SUM_NV                                        0x8547
#define GL_COMBINER_SCALE_NV                                          0x8548
#define GL_COMBINER_SUM_OUTPUT_NV                                     0x854C
#define GL_COMBINE_ALPHA                                              0x8572
#define GL_COMBINE_ALPHA_ARB                                          0x8572
#define GL_COMBINE_ALPHA_EXT                                          0x8572
#define GL_COMBINE_ARB                                                0x8570
#define GL_COMBINE_EXT                                                0x8570
#define GL_COMBINE_RGB                                                0x8571
#define GL_COMBINE_RGB_ARB                                            0x8571
#define GL_COMBINE_RGB_EXT                                            0x8571
#define GL_COMMAND_BARRIER_BIT                                        0x00000040
#define GL_COMMAND_BARRIER_BIT_EXT                                    0x00000040
#define GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT                           0x884E
#define GL_COMPARE_REF_TO_TEXTURE                                     0x884E
#define GL_COMPARE_R_TO_TEXTURE                                       0x884E
#define GL_COMPARE_R_TO_TEXTURE_ARB                                   0x884E
#define GL_COMPATIBLE_SUBROUTINES                                     0x8E4B
#define GL_COMPILE                                                    0x1300
#define GL_COMPILE_AND_EXECUTE                                        0x1301
#define GL_COMPILE_STATUS                                             0x8B81
#define GL_COMPLETION_STATUS_ARB                                      0x91B1
#define GL_COMPLETION_STATUS_KHR                                      0x91B1
#define GL_COMPRESSED_ALPHA                                           0x84E9
#define GL_COMPRESSED_ALPHA_ARB                                       0x84E9
#define GL_COMPRESSED_INTENSITY                                       0x84EC
#define GL_COMPRESSED_INTENSITY_ARB                                   0x84EC
#define GL_COMPRESSED_LUMINANCE                                       0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA                                 0x84EB
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB                             0x84EB
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT                       0x8C72
#define GL_COMPRESSED_LUMINANCE_ARB                                   0x84EA
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT                             0x8C70
#define GL_COMPRESSED_R11_EAC                                         0x9270
#define GL_COMPRESSED_RED                                             0x8225
#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT                             0x8DBD
#define GL_COMPRESSED_RED_RGTC1                                       0x8DBB
#define GL_COMPRESSED_RED_RGTC1_EXT                                   0x8DBB
#define GL_COMPRESSED_RG                                              0x8226
#define GL_COMPRESSED_RG11_EAC                                        0x9272
#define GL_COMPRESSED_RGB                                             0x84ED
#define GL_COMPRESSED_RGB8_ETC2                                       0x9274
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2                   0x9276
#define GL_COMPRESSED_RGBA                                            0x84EE
#define GL_COMPRESSED_RGBA8_ETC2_EAC                                  0x9278
#define GL_COMPRESSED_RGBA_ARB                                        0x84EE
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR                             0x93BB
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR                              0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR                              0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR                              0x93BA
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR                             0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR                             0x93BD
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR                               0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR                               0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR                               0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR                               0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR                               0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR                               0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR                               0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR                               0x93B7
#define GL_COMPRESSED_RGBA_BPTC_UNORM                                 0x8E8C
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB                             0x8E8C
#define GL_COMPRESSED_RGBA_FXT1_3DFX                                  0x86B1
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                              0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                              0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                              0x83F3
#define GL_COMPRESSED_RGB_ARB                                         0x84ED
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT                           0x8E8E
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB                       0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT                         0x8E8F
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB                     0x8E8F
#define GL_COMPRESSED_RGB_FXT1_3DFX                                   0x86B0
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                               0x83F0
#define GL_COMPRESSED_RG_RGTC2                                        0x8DBD
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT                0x8C73
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT                      0x8C71
#define GL_COMPRESSED_SIGNED_R11_EAC                                  0x9271
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT                      0x8DBE
#define GL_COMPRESSED_SIGNED_RED_RGTC1                                0x8DBC
#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT                            0x8DBC
#define GL_COMPRESSED_SIGNED_RG11_EAC                                 0x9273
#define GL_COMPRESSED_SIGNED_RG_RGTC2                                 0x8DBE
#define GL_COMPRESSED_SLUMINANCE                                      0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA                                0x8C4B
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT                            0x8C4B
#define GL_COMPRESSED_SLUMINANCE_EXT                                  0x8C4A
#define GL_COMPRESSED_SRGB                                            0x8C48
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR                     0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR                      0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR                      0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR                      0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR                     0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR                     0x93DD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR                       0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR                       0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR                       0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR                       0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR                       0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR                       0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR                       0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR                       0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC                           0x9279
#define GL_COMPRESSED_SRGB8_ETC2                                      0x9275
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2                  0x9277
#define GL_COMPRESSED_SRGB_ALPHA                                      0x8C49
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM                           0x8E8D
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB                       0x8E8D
#define GL_COMPRESSED_SRGB_ALPHA_EXT                                  0x8C49
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT                        0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT                        0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT                        0x8C4F
#define GL_COMPRESSED_SRGB_EXT                                        0x8C48
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT                              0x8C4C
#define GL_COMPRESSED_TEXTURE_FORMATS                                 0x86A3
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB                             0x86A3
#define GL_COMPUTE_PROGRAM_NV                                         0x90FB
#define GL_COMPUTE_PROGRAM_PARAMETER_BUFFER_NV                        0x90FC
#define GL_COMPUTE_SHADER                                             0x91B9
#define GL_COMPUTE_SHADER_BIT                                         0x00000020
#define GL_COMPUTE_SHADER_INVOCATIONS                                 0x82F5
#define GL_COMPUTE_SHADER_INVOCATIONS_ARB                             0x82F5
#define GL_COMPUTE_SUBROUTINE                                         0x92ED
#define GL_COMPUTE_SUBROUTINE_UNIFORM                                 0x92F3
#define GL_COMPUTE_TEXTURE                                            0x82A0
#define GL_COMPUTE_WORK_GROUP_SIZE                                    0x8267
#define GL_COMP_BIT_ATI                                               0x00000002
#define GL_CONDITION_SATISFIED                                        0x911C
#define GL_CONFORMANT_NV                                              0x9374
#define GL_CONIC_CURVE_TO_NV                                          0x1A
#define GL_CONJOINT_NV                                                0x9284
#define GL_CONSERVATIVE_RASTERIZATION_INTEL                           0x83FE
#define GL_CONSERVATIVE_RASTERIZATION_NV                              0x9346
#define GL_CONSERVATIVE_RASTER_DILATE_GRANULARITY_NV                  0x937B
#define GL_CONSERVATIVE_RASTER_DILATE_NV                              0x9379
#define GL_CONSERVATIVE_RASTER_DILATE_RANGE_NV                        0x937A
#define GL_CONSERVATIVE_RASTER_MODE_NV                                0x954D
#define GL_CONSERVATIVE_RASTER_MODE_POST_SNAP_NV                      0x954E
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_NV                       0x9550
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_TRIANGLES_NV             0x954F
#define GL_CONSERVE_MEMORY_HINT_PGI                                   0x1A1FD
#define GL_CONSTANT                                                   0x8576
#define GL_CONSTANT_ALPHA                                             0x8003
#define GL_CONSTANT_ALPHA_EXT                                         0x8003
#define GL_CONSTANT_ARB                                               0x8576
#define GL_CONSTANT_ATTENUATION                                       0x1207
#define GL_CONSTANT_BORDER                                            0x8151
#define GL_CONSTANT_BORDER_HP                                         0x8151
#define GL_CONSTANT_COLOR                                             0x8001
#define GL_CONSTANT_COLOR0_NV                                         0x852A
#define GL_CONSTANT_COLOR1_NV                                         0x852B
#define GL_CONSTANT_COLOR_EXT                                         0x8001
#define GL_CONSTANT_EXT                                               0x8576
#define GL_CONSTANT_NV                                                0x8576
#define GL_CONST_EYE_NV                                               0x86E5
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT                          0x00000002
#define GL_CONTEXT_CORE_PROFILE_BIT                                   0x00000001
#define GL_CONTEXT_FLAGS                                              0x821E
#define GL_CONTEXT_FLAG_DEBUG_BIT                                     0x00000002
#define GL_CONTEXT_FLAG_DEBUG_BIT_KHR                                 0x00000002
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT                        0x00000001
#define GL_CONTEXT_FLAG_NO_ERROR_BIT                                  0x00000008
#define GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR                              0x00000008
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT                             0x00000004
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB                         0x00000004
#define GL_CONTEXT_LOST                                               0x0507
#define GL_CONTEXT_LOST_KHR                                           0x0507
#define GL_CONTEXT_PROFILE_MASK                                       0x9126
#define GL_CONTEXT_RELEASE_BEHAVIOR                                   0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH                             0x82FC
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_KHR                         0x82FC
#define GL_CONTEXT_RELEASE_BEHAVIOR_KHR                               0x82FB
#define GL_CONTEXT_ROBUST_ACCESS                                      0x90F3
#define GL_CONTEXT_ROBUST_ACCESS_KHR                                  0x90F3
#define GL_CONTINUOUS_AMD                                             0x9007
#define GL_CONTRAST_NV                                                0x92A1
#define GL_CONVEX_HULL_NV                                             0x908B
#define GL_CONVOLUTION_1D                                             0x8010
#define GL_CONVOLUTION_1D_EXT                                         0x8010
#define GL_CONVOLUTION_2D                                             0x8011
#define GL_CONVOLUTION_2D_EXT                                         0x8011
#define GL_CONVOLUTION_BORDER_COLOR                                   0x8154
#define GL_CONVOLUTION_BORDER_COLOR_HP                                0x8154
#define GL_CONVOLUTION_BORDER_MODE                                    0x8013
#define GL_CONVOLUTION_BORDER_MODE_EXT                                0x8013
#define GL_CONVOLUTION_FILTER_BIAS                                    0x8015
#define GL_CONVOLUTION_FILTER_BIAS_EXT                                0x8015
#define GL_CONVOLUTION_FILTER_SCALE                                   0x8014
#define GL_CONVOLUTION_FILTER_SCALE_EXT                               0x8014
#define GL_CONVOLUTION_FORMAT                                         0x8017
#define GL_CONVOLUTION_FORMAT_EXT                                     0x8017
#define GL_CONVOLUTION_HEIGHT                                         0x8019
#define GL_CONVOLUTION_HEIGHT_EXT                                     0x8019
#define GL_CONVOLUTION_HINT_SGIX                                      0x8316
#define GL_CONVOLUTION_WIDTH                                          0x8018
#define GL_CONVOLUTION_WIDTH_EXT                                      0x8018
#define GL_CON_0_ATI                                                  0x8941
#define GL_CON_10_ATI                                                 0x894B
#define GL_CON_11_ATI                                                 0x894C
#define GL_CON_12_ATI                                                 0x894D
#define GL_CON_13_ATI                                                 0x894E
#define GL_CON_14_ATI                                                 0x894F
#define GL_CON_15_ATI                                                 0x8950
#define GL_CON_16_ATI                                                 0x8951
#define GL_CON_17_ATI                                                 0x8952
#define GL_CON_18_ATI                                                 0x8953
#define GL_CON_19_ATI                                                 0x8954
#define GL_CON_1_ATI                                                  0x8942
#define GL_CON_20_ATI                                                 0x8955
#define GL_CON_21_ATI                                                 0x8956
#define GL_CON_22_ATI                                                 0x8957
#define GL_CON_23_ATI                                                 0x8958
#define GL_CON_24_ATI                                                 0x8959
#define GL_CON_25_ATI                                                 0x895A
#define GL_CON_26_ATI                                                 0x895B
#define GL_CON_27_ATI                                                 0x895C
#define GL_CON_28_ATI                                                 0x895D
#define GL_CON_29_ATI                                                 0x895E
#define GL_CON_2_ATI                                                  0x8943
#define GL_CON_30_ATI                                                 0x895F
#define GL_CON_31_ATI                                                 0x8960
#define GL_CON_3_ATI                                                  0x8944
#define GL_CON_4_ATI                                                  0x8945
#define GL_CON_5_ATI                                                  0x8946
#define GL_CON_6_ATI                                                  0x8947
#define GL_CON_7_ATI                                                  0x8948
#define GL_CON_8_ATI                                                  0x8949
#define GL_CON_9_ATI                                                  0x894A
#define GL_COORD_REPLACE                                              0x8862
#define GL_COORD_REPLACE_ARB                                          0x8862
#define GL_COORD_REPLACE_NV                                           0x8862
#define GL_COPY                                                       0x1503
#define GL_COPY_INVERTED                                              0x150C
#define GL_COPY_PIXEL_TOKEN                                           0x0706
#define GL_COPY_READ_BUFFER                                           0x8F36
#define GL_COPY_READ_BUFFER_BINDING                                   0x8F36
#define GL_COPY_WRITE_BUFFER                                          0x8F37
#define GL_COPY_WRITE_BUFFER_BINDING                                  0x8F37
#define GL_COUNTER_RANGE_AMD                                          0x8BC1
#define GL_COUNTER_TYPE_AMD                                           0x8BC0
#define GL_COUNT_DOWN_NV                                              0x9089
#define GL_COUNT_UP_NV                                                0x9088
#define GL_COVERAGE_MODULATION_NV                                     0x9332
#define GL_COVERAGE_MODULATION_TABLE_NV                               0x9331
#define GL_COVERAGE_MODULATION_TABLE_SIZE_NV                          0x9333
#define GL_CUBIC_CURVE_TO_NV                                          0x0C
#define GL_CUBIC_EXT                                                  0x8334
#define GL_CUBIC_HP                                                   0x815F
#define GL_CULL_FACE                                                  0x0B44
#define GL_CULL_FACE_MODE                                             0x0B45
#define GL_CULL_FRAGMENT_NV                                           0x86E7
#define GL_CULL_MODES_NV                                              0x86E0
#define GL_CULL_VERTEX_EXT                                            0x81AA
#define GL_CULL_VERTEX_EYE_POSITION_EXT                               0x81AB
#define GL_CULL_VERTEX_IBM                                            103050
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT                            0x81AC
#define GL_CURRENT_ATTRIB_NV                                          0x8626
#define GL_CURRENT_BINORMAL_EXT                                       0x843C
#define GL_CURRENT_BIT                                                0x00000001
#define GL_CURRENT_COLOR                                              0x0B00
#define GL_CURRENT_FOG_COORD                                          0x8453
#define GL_CURRENT_FOG_COORDINATE                                     0x8453
#define GL_CURRENT_FOG_COORDINATE_EXT                                 0x8453
#define GL_CURRENT_INDEX                                              0x0B01
#define GL_CURRENT_MATRIX_ARB                                         0x8641
#define GL_CURRENT_MATRIX_INDEX_ARB                                   0x8845
#define GL_CURRENT_MATRIX_NV                                          0x8641
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB                             0x8640
#define GL_CURRENT_MATRIX_STACK_DEPTH_NV                              0x8640
#define GL_CURRENT_NORMAL                                             0x0B02
#define GL_CURRENT_OCCLUSION_QUERY_ID_NV                              0x8865
#define GL_CURRENT_PALETTE_MATRIX_ARB                                 0x8843
#define GL_CURRENT_PROGRAM                                            0x8B8D
#define GL_CURRENT_QUERY                                              0x8865
#define GL_CURRENT_QUERY_ARB                                          0x8865
#define GL_CURRENT_RASTER_COLOR                                       0x0B04
#define GL_CURRENT_RASTER_DISTANCE                                    0x0B09
#define GL_CURRENT_RASTER_INDEX                                       0x0B05
#define GL_CURRENT_RASTER_NORMAL_SGIX                                 0x8406
#define GL_CURRENT_RASTER_POSITION                                    0x0B07
#define GL_CURRENT_RASTER_POSITION_VALID                              0x0B08
#define GL_CURRENT_RASTER_SECONDARY_COLOR                             0x845F
#define GL_CURRENT_RASTER_TEXTURE_COORDS                              0x0B06
#define GL_CURRENT_SECONDARY_COLOR                                    0x8459
#define GL_CURRENT_SECONDARY_COLOR_EXT                                0x8459
#define GL_CURRENT_TANGENT_EXT                                        0x843B
#define GL_CURRENT_TEXTURE_COORDS                                     0x0B03
#define GL_CURRENT_TIME_NV                                            0x8E28
#define GL_CURRENT_VERTEX_ATTRIB                                      0x8626
#define GL_CURRENT_VERTEX_ATTRIB_ARB                                  0x8626
#define GL_CURRENT_VERTEX_EXT                                         0x87E2
#define GL_CURRENT_VERTEX_WEIGHT_EXT                                  0x850B
#define GL_CURRENT_WEIGHT_ARB                                         0x86A8
#define GL_CW                                                         0x0900
#define GL_D3D12_FENCE_VALUE_EXT                                      0x9595
#define GL_DARKEN_KHR                                                 0x9297
#define GL_DARKEN_NV                                                  0x9297
#define GL_DATA_BUFFER_AMD                                            0x9151
#define GL_DEBUG_CALLBACK_FUNCTION                                    0x8244
#define GL_DEBUG_CALLBACK_FUNCTION_ARB                                0x8244
#define GL_DEBUG_CALLBACK_FUNCTION_KHR                                0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM                                  0x8245
#define GL_DEBUG_CALLBACK_USER_PARAM_ARB                              0x8245
#define GL_DEBUG_CALLBACK_USER_PARAM_KHR                              0x8245
#define GL_DEBUG_CATEGORY_API_ERROR_AMD                               0x9149
#define GL_DEBUG_CATEGORY_APPLICATION_AMD                             0x914F
#define GL_DEBUG_CATEGORY_DEPRECATION_AMD                             0x914B
#define GL_DEBUG_CATEGORY_OTHER_AMD                                   0x9150
#define GL_DEBUG_CATEGORY_PERFORMANCE_AMD                             0x914D
#define GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD                         0x914E
#define GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD                      0x914C
#define GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD                           0x914A
#define GL_DEBUG_GROUP_STACK_DEPTH                                    0x826D
#define GL_DEBUG_GROUP_STACK_DEPTH_KHR                                0x826D
#define GL_DEBUG_LOGGED_MESSAGES                                      0x9145
#define GL_DEBUG_LOGGED_MESSAGES_AMD                                  0x9145
#define GL_DEBUG_LOGGED_MESSAGES_ARB                                  0x9145
#define GL_DEBUG_LOGGED_MESSAGES_KHR                                  0x9145
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH                           0x8243
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB                       0x8243
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_KHR                       0x8243
#define GL_DEBUG_OUTPUT                                               0x92E0
#define GL_DEBUG_OUTPUT_KHR                                           0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS                                   0x8242
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB                               0x8242
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR                               0x8242
#define GL_DEBUG_SEVERITY_HIGH                                        0x9146
#define GL_DEBUG_SEVERITY_HIGH_AMD                                    0x9146
#define GL_DEBUG_SEVERITY_HIGH_ARB                                    0x9146
#define GL_DEBUG_SEVERITY_HIGH_KHR                                    0x9146
#define GL_DEBUG_SEVERITY_LOW                                         0x9148
#define GL_DEBUG_SEVERITY_LOW_AMD                                     0x9148
#define GL_DEBUG_SEVERITY_LOW_ARB                                     0x9148
#define GL_DEBUG_SEVERITY_LOW_KHR                                     0x9148
#define GL_DEBUG_SEVERITY_MEDIUM                                      0x9147
#define GL_DEBUG_SEVERITY_MEDIUM_AMD                                  0x9147
#define GL_DEBUG_SEVERITY_MEDIUM_ARB                                  0x9147
#define GL_DEBUG_SEVERITY_MEDIUM_KHR                                  0x9147
#define GL_DEBUG_SEVERITY_NOTIFICATION                                0x826B
#define GL_DEBUG_SEVERITY_NOTIFICATION_KHR                            0x826B
#define GL_DEBUG_SOURCE_API                                           0x8246
#define GL_DEBUG_SOURCE_API_ARB                                       0x8246
#define GL_DEBUG_SOURCE_API_KHR                                       0x8246
#define GL_DEBUG_SOURCE_APPLICATION                                   0x824A
#define GL_DEBUG_SOURCE_APPLICATION_ARB                               0x824A
#define GL_DEBUG_SOURCE_APPLICATION_KHR                               0x824A
#define GL_DEBUG_SOURCE_OTHER                                         0x824B
#define GL_DEBUG_SOURCE_OTHER_ARB                                     0x824B
#define GL_DEBUG_SOURCE_OTHER_KHR                                     0x824B
#define GL_DEBUG_SOURCE_SHADER_COMPILER                               0x8248
#define GL_DEBUG_SOURCE_SHADER_COMPILER_ARB                           0x8248
#define GL_DEBUG_SOURCE_SHADER_COMPILER_KHR                           0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY                                   0x8249
#define GL_DEBUG_SOURCE_THIRD_PARTY_ARB                               0x8249
#define GL_DEBUG_SOURCE_THIRD_PARTY_KHR                               0x8249
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM                                 0x8247
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB                             0x8247
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR                             0x8247
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR                             0x824D
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB                         0x824D
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR                         0x824D
#define GL_DEBUG_TYPE_ERROR                                           0x824C
#define GL_DEBUG_TYPE_ERROR_ARB                                       0x824C
#define GL_DEBUG_TYPE_ERROR_KHR                                       0x824C
#define GL_DEBUG_TYPE_MARKER                                          0x8268
#define GL_DEBUG_TYPE_MARKER_KHR                                      0x8268
#define GL_DEBUG_TYPE_OTHER                                           0x8251
#define GL_DEBUG_TYPE_OTHER_ARB                                       0x8251
#define GL_DEBUG_TYPE_OTHER_KHR                                       0x8251
#define GL_DEBUG_TYPE_PERFORMANCE                                     0x8250
#define GL_DEBUG_TYPE_PERFORMANCE_ARB                                 0x8250
#define GL_DEBUG_TYPE_PERFORMANCE_KHR                                 0x8250
#define GL_DEBUG_TYPE_POP_GROUP                                       0x826A
#define GL_DEBUG_TYPE_POP_GROUP_KHR                                   0x826A
#define GL_DEBUG_TYPE_PORTABILITY                                     0x824F
#define GL_DEBUG_TYPE_PORTABILITY_ARB                                 0x824F
#define GL_DEBUG_TYPE_PORTABILITY_KHR                                 0x824F
#define GL_DEBUG_TYPE_PUSH_GROUP                                      0x8269
#define GL_DEBUG_TYPE_PUSH_GROUP_KHR                                  0x8269
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR                              0x824E
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB                          0x824E
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR                          0x824E
#define GL_DECAL                                                      0x2101
#define GL_DECODE_EXT                                                 0x8A49
#define GL_DECR                                                       0x1E03
#define GL_DECR_WRAP                                                  0x8508
#define GL_DECR_WRAP_EXT                                              0x8508
#define GL_DEDICATED_MEMORY_OBJECT_EXT                                0x9581
#define GL_DEFORMATIONS_MASK_SGIX                                     0x8196
#define GL_DELETE_STATUS                                              0x8B80
#define GL_DEPENDENT_AR_TEXTURE_2D_NV                                 0x86E9
#define GL_DEPENDENT_GB_TEXTURE_2D_NV                                 0x86EA
#define GL_DEPENDENT_HILO_TEXTURE_2D_NV                               0x8858
#define GL_DEPENDENT_RGB_TEXTURE_3D_NV                                0x8859
#define GL_DEPENDENT_RGB_TEXTURE_CUBE_MAP_NV                          0x885A
#define GL_DEPTH                                                      0x1801
#define GL_DEPTH24_STENCIL8                                           0x88F0
#define GL_DEPTH24_STENCIL8_EXT                                       0x88F0
#define GL_DEPTH32F_STENCIL8                                          0x8CAD
#define GL_DEPTH32F_STENCIL8_NV                                       0x8DAC
#define GL_DEPTH_ATTACHMENT                                           0x8D00
#define GL_DEPTH_ATTACHMENT_EXT                                       0x8D00
#define GL_DEPTH_BIAS                                                 0x0D1F
#define GL_DEPTH_BITS                                                 0x0D56
#define GL_DEPTH_BOUNDS_EXT                                           0x8891
#define GL_DEPTH_BOUNDS_TEST_EXT                                      0x8890
#define GL_DEPTH_BUFFER_BIT                                           0x00000100
#define GL_DEPTH_BUFFER_FLOAT_MODE_NV                                 0x8DAF
#define GL_DEPTH_CLAMP                                                0x864F
#define GL_DEPTH_CLAMP_FAR_AMD                                        0x901F
#define GL_DEPTH_CLAMP_NEAR_AMD                                       0x901E
#define GL_DEPTH_CLAMP_NV                                             0x864F
#define GL_DEPTH_CLEAR_VALUE                                          0x0B73
#define GL_DEPTH_COMPONENT                                            0x1902
#define GL_DEPTH_COMPONENT16                                          0x81A5
#define GL_DEPTH_COMPONENT16_ARB                                      0x81A5
#define GL_DEPTH_COMPONENT16_SGIX                                     0x81A5
#define GL_DEPTH_COMPONENT24                                          0x81A6
#define GL_DEPTH_COMPONENT24_ARB                                      0x81A6
#define GL_DEPTH_COMPONENT24_SGIX                                     0x81A6
#define GL_DEPTH_COMPONENT32                                          0x81A7
#define GL_DEPTH_COMPONENT32F                                         0x8CAC
#define GL_DEPTH_COMPONENT32F_NV                                      0x8DAB
#define GL_DEPTH_COMPONENT32_ARB                                      0x81A7
#define GL_DEPTH_COMPONENT32_SGIX                                     0x81A7
#define GL_DEPTH_COMPONENTS                                           0x8284
#define GL_DEPTH_FUNC                                                 0x0B74
#define GL_DEPTH_RANGE                                                0x0B70
#define GL_DEPTH_RENDERABLE                                           0x8287
#define GL_DEPTH_SAMPLES_NV                                           0x932D
#define GL_DEPTH_SCALE                                                0x0D1E
#define GL_DEPTH_STENCIL                                              0x84F9
#define GL_DEPTH_STENCIL_ATTACHMENT                                   0x821A
#define GL_DEPTH_STENCIL_EXT                                          0x84F9
#define GL_DEPTH_STENCIL_NV                                           0x84F9
#define GL_DEPTH_STENCIL_TEXTURE_MODE                                 0x90EA
#define GL_DEPTH_STENCIL_TO_BGRA_NV                                   0x886F
#define GL_DEPTH_STENCIL_TO_RGBA_NV                                   0x886E
#define GL_DEPTH_TEST                                                 0x0B71
#define GL_DEPTH_TEXTURE_MODE                                         0x884B
#define GL_DEPTH_TEXTURE_MODE_ARB                                     0x884B
#define GL_DEPTH_WRITEMASK                                            0x0B72
#define GL_DETACHED_BUFFERS_NV                                        0x95AB
#define GL_DETACHED_MEMORY_INCARNATION_NV                             0x95A9
#define GL_DETACHED_TEXTURES_NV                                       0x95AA
#define GL_DETAIL_TEXTURE_2D_BINDING_SGIS                             0x8096
#define GL_DETAIL_TEXTURE_2D_SGIS                                     0x8095
#define GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS                            0x809C
#define GL_DETAIL_TEXTURE_LEVEL_SGIS                                  0x809A
#define GL_DETAIL_TEXTURE_MODE_SGIS                                   0x809B
#define GL_DEVICE_LUID_EXT                                            0x9599
#define GL_DEVICE_NODE_MASK_EXT                                       0x959A
#define GL_DEVICE_UUID_EXT                                            0x9597
#define GL_DIFFERENCE_KHR                                             0x929E
#define GL_DIFFERENCE_NV                                              0x929E
#define GL_DIFFUSE                                                    0x1201
#define GL_DISCARD_ATI                                                0x8763
#define GL_DISCARD_NV                                                 0x8530
#define GL_DISCRETE_AMD                                               0x9006
#define GL_DISJOINT_NV                                                0x9283
#define GL_DISPATCH_INDIRECT_BUFFER                                   0x90EE
#define GL_DISPATCH_INDIRECT_BUFFER_BINDING                           0x90EF
#define GL_DISPLAY_LIST                                               0x82E7
#define GL_DISTANCE_ATTENUATION_EXT                                   0x8129
#define GL_DISTANCE_ATTENUATION_SGIS                                  0x8129
#define GL_DITHER                                                     0x0BD0
#define GL_DOMAIN                                                     0x0A02
#define GL_DONT_CARE                                                  0x1100
#define GL_DOT2_ADD_ATI                                               0x896C
#define GL_DOT3_ATI                                                   0x8966
#define GL_DOT3_RGB                                                   0x86AE
#define GL_DOT3_RGBA                                                  0x86AF
#define GL_DOT3_RGBA_ARB                                              0x86AF
#define GL_DOT3_RGBA_EXT                                              0x8741
#define GL_DOT3_RGB_ARB                                               0x86AE
#define GL_DOT3_RGB_EXT                                               0x8740
#define GL_DOT4_ATI                                                   0x8967
#define GL_DOT_PRODUCT_AFFINE_DEPTH_REPLACE_NV                        0x885D
#define GL_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV                  0x86F3
#define GL_DOT_PRODUCT_DEPTH_REPLACE_NV                               0x86ED
#define GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV                            0x86F1
#define GL_DOT_PRODUCT_NV                                             0x86EC
#define GL_DOT_PRODUCT_PASS_THROUGH_NV                                0x885B
#define GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV                            0x86F2
#define GL_DOT_PRODUCT_TEXTURE_1D_NV                                  0x885C
#define GL_DOT_PRODUCT_TEXTURE_2D_NV                                  0x86EE
#define GL_DOT_PRODUCT_TEXTURE_3D_NV                                  0x86EF
#define GL_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV                            0x86F0
#define GL_DOT_PRODUCT_TEXTURE_RECTANGLE_NV                           0x864E
#define GL_DOUBLE                                                     0x140A
#define GL_DOUBLEBUFFER                                               0x0C32
#define GL_DOUBLE_MAT2                                                0x8F46
#define GL_DOUBLE_MAT2_EXT                                            0x8F46
#define GL_DOUBLE_MAT2x3                                              0x8F49
#define GL_DOUBLE_MAT2x3_EXT                                          0x8F49
#define GL_DOUBLE_MAT2x4                                              0x8F4A
#define GL_DOUBLE_MAT2x4_EXT                                          0x8F4A
#define GL_DOUBLE_MAT3                                                0x8F47
#define GL_DOUBLE_MAT3_EXT                                            0x8F47
#define GL_DOUBLE_MAT3x2                                              0x8F4B
#define GL_DOUBLE_MAT3x2_EXT                                          0x8F4B
#define GL_DOUBLE_MAT3x4                                              0x8F4C
#define GL_DOUBLE_MAT3x4_EXT                                          0x8F4C
#define GL_DOUBLE_MAT4                                                0x8F48
#define GL_DOUBLE_MAT4_EXT                                            0x8F48
#define GL_DOUBLE_MAT4x2                                              0x8F4D
#define GL_DOUBLE_MAT4x2_EXT                                          0x8F4D
#define GL_DOUBLE_MAT4x3                                              0x8F4E
#define GL_DOUBLE_MAT4x3_EXT                                          0x8F4E
#define GL_DOUBLE_VEC2                                                0x8FFC
#define GL_DOUBLE_VEC2_EXT                                            0x8FFC
#define GL_DOUBLE_VEC3                                                0x8FFD
#define GL_DOUBLE_VEC3_EXT                                            0x8FFD
#define GL_DOUBLE_VEC4                                                0x8FFE
#define GL_DOUBLE_VEC4_EXT                                            0x8FFE
#define GL_DRAW_ARRAYS_COMMAND_NV                                     0x0003
#define GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV                           0x0007
#define GL_DRAW_ARRAYS_STRIP_COMMAND_NV                               0x0005
#define GL_DRAW_BUFFER                                                0x0C01
#define GL_DRAW_BUFFER0                                               0x8825
#define GL_DRAW_BUFFER0_ARB                                           0x8825
#define GL_DRAW_BUFFER0_ATI                                           0x8825
#define GL_DRAW_BUFFER1                                               0x8826
#define GL_DRAW_BUFFER10                                              0x882F
#define GL_DRAW_BUFFER10_ARB                                          0x882F
#define GL_DRAW_BUFFER10_ATI                                          0x882F
#define GL_DRAW_BUFFER11                                              0x8830
#define GL_DRAW_BUFFER11_ARB                                          0x8830
#define GL_DRAW_BUFFER11_ATI                                          0x8830
#define GL_DRAW_BUFFER12                                              0x8831
#define GL_DRAW_BUFFER12_ARB                                          0x8831
#define GL_DRAW_BUFFER12_ATI                                          0x8831
#define GL_DRAW_BUFFER13                                              0x8832
#define GL_DRAW_BUFFER13_ARB                                          0x8832
#define GL_DRAW_BUFFER13_ATI                                          0x8832
#define GL_DRAW_BUFFER14                                              0x8833
#define GL_DRAW_BUFFER14_ARB                                          0x8833
#define GL_DRAW_BUFFER14_ATI                                          0x8833
#define GL_DRAW_BUFFER15                                              0x8834
#define GL_DRAW_BUFFER15_ARB                                          0x8834
#define GL_DRAW_BUFFER15_ATI                                          0x8834
#define GL_DRAW_BUFFER1_ARB                                           0x8826
#define GL_DRAW_BUFFER1_ATI                                           0x8826
#define GL_DRAW_BUFFER2                                               0x8827
#define GL_DRAW_BUFFER2_ARB                                           0x8827
#define GL_DRAW_BUFFER2_ATI                                           0x8827
#define GL_DRAW_BUFFER3                                               0x8828
#define GL_DRAW_BUFFER3_ARB                                           0x8828
#define GL_DRAW_BUFFER3_ATI                                           0x8828
#define GL_DRAW_BUFFER4                                               0x8829
#define GL_DRAW_BUFFER4_ARB                                           0x8829
#define GL_DRAW_BUFFER4_ATI                                           0x8829
#define GL_DRAW_BUFFER5                                               0x882A
#define GL_DRAW_BUFFER5_ARB                                           0x882A
#define GL_DRAW_BUFFER5_ATI                                           0x882A
#define GL_DRAW_BUFFER6                                               0x882B
#define GL_DRAW_BUFFER6_ARB                                           0x882B
#define GL_DRAW_BUFFER6_ATI                                           0x882B
#define GL_DRAW_BUFFER7                                               0x882C
#define GL_DRAW_BUFFER7_ARB                                           0x882C
#define GL_DRAW_BUFFER7_ATI                                           0x882C
#define GL_DRAW_BUFFER8                                               0x882D
#define GL_DRAW_BUFFER8_ARB                                           0x882D
#define GL_DRAW_BUFFER8_ATI                                           0x882D
#define GL_DRAW_BUFFER9                                               0x882E
#define GL_DRAW_BUFFER9_ARB                                           0x882E
#define GL_DRAW_BUFFER9_ATI                                           0x882E
#define GL_DRAW_ELEMENTS_COMMAND_NV                                   0x0002
#define GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV                         0x0006
#define GL_DRAW_ELEMENTS_STRIP_COMMAND_NV                             0x0004
#define GL_DRAW_FRAMEBUFFER                                           0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING                                   0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT                               0x8CA6
#define GL_DRAW_FRAMEBUFFER_EXT                                       0x8CA9
#define GL_DRAW_INDIRECT_ADDRESS_NV                                   0x8F41
#define GL_DRAW_INDIRECT_BUFFER                                       0x8F3F
#define GL_DRAW_INDIRECT_BUFFER_BINDING                               0x8F43
#define GL_DRAW_INDIRECT_LENGTH_NV                                    0x8F42
#define GL_DRAW_INDIRECT_UNIFIED_NV                                   0x8F40
#define GL_DRAW_PIXELS_APPLE                                          0x8A0A
#define GL_DRAW_PIXEL_TOKEN                                           0x0705
#define GL_DRIVER_UUID_EXT                                            0x9598
#define GL_DSDT8_MAG8_INTENSITY8_NV                                   0x870B
#define GL_DSDT8_MAG8_NV                                              0x870A
#define GL_DSDT8_NV                                                   0x8709
#define GL_DSDT_MAG_INTENSITY_NV                                      0x86DC
#define GL_DSDT_MAG_NV                                                0x86F6
#define GL_DSDT_MAG_VIB_NV                                            0x86F7
#define GL_DSDT_NV                                                    0x86F5
#define GL_DST_ALPHA                                                  0x0304
#define GL_DST_ATOP_NV                                                0x928F
#define GL_DST_COLOR                                                  0x0306
#define GL_DST_IN_NV                                                  0x928B
#define GL_DST_NV                                                     0x9287
#define GL_DST_OUT_NV                                                 0x928D
#define GL_DST_OVER_NV                                                0x9289
#define GL_DS_BIAS_NV                                                 0x8716
#define GL_DS_SCALE_NV                                                0x8710
#define GL_DT_BIAS_NV                                                 0x8717
#define GL_DT_SCALE_NV                                                0x8711
#define GL_DU8DV8_ATI                                                 0x877A
#define GL_DUAL_ALPHA12_SGIS                                          0x8112
#define GL_DUAL_ALPHA16_SGIS                                          0x8113
#define GL_DUAL_ALPHA4_SGIS                                           0x8110
#define GL_DUAL_ALPHA8_SGIS                                           0x8111
#define GL_DUAL_INTENSITY12_SGIS                                      0x811A
#define GL_DUAL_INTENSITY16_SGIS                                      0x811B
#define GL_DUAL_INTENSITY4_SGIS                                       0x8118
#define GL_DUAL_INTENSITY8_SGIS                                       0x8119
#define GL_DUAL_LUMINANCE12_SGIS                                      0x8116
#define GL_DUAL_LUMINANCE16_SGIS                                      0x8117
#define GL_DUAL_LUMINANCE4_SGIS                                       0x8114
#define GL_DUAL_LUMINANCE8_SGIS                                       0x8115
#define GL_DUAL_LUMINANCE_ALPHA4_SGIS                                 0x811C
#define GL_DUAL_LUMINANCE_ALPHA8_SGIS                                 0x811D
#define GL_DUAL_TEXTURE_SELECT_SGIS                                   0x8124
#define GL_DUDV_ATI                                                   0x8779
#define GL_DUP_FIRST_CUBIC_CURVE_TO_NV                                0xF2
#define GL_DUP_LAST_CUBIC_CURVE_TO_NV                                 0xF4
#define GL_DYNAMIC_ATI                                                0x8761
#define GL_DYNAMIC_COPY                                               0x88EA
#define GL_DYNAMIC_COPY_ARB                                           0x88EA
#define GL_DYNAMIC_DRAW                                               0x88E8
#define GL_DYNAMIC_DRAW_ARB                                           0x88E8
#define GL_DYNAMIC_READ                                               0x88E9
#define GL_DYNAMIC_READ_ARB                                           0x88E9
#define GL_DYNAMIC_STORAGE_BIT                                        0x0100
#define GL_EDGEFLAG_BIT_PGI                                           0x00040000
#define GL_EDGE_FLAG                                                  0x0B43
#define GL_EDGE_FLAG_ARRAY                                            0x8079
#define GL_EDGE_FLAG_ARRAY_ADDRESS_NV                                 0x8F26
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING                             0x889B
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB                         0x889B
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT                                  0x808D
#define GL_EDGE_FLAG_ARRAY_EXT                                        0x8079
#define GL_EDGE_FLAG_ARRAY_LENGTH_NV                                  0x8F30
#define GL_EDGE_FLAG_ARRAY_LIST_IBM                                   103075
#define GL_EDGE_FLAG_ARRAY_LIST_STRIDE_IBM                            103085
#define GL_EDGE_FLAG_ARRAY_POINTER                                    0x8093
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT                                0x8093
#define GL_EDGE_FLAG_ARRAY_STRIDE                                     0x808C
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT                                 0x808C
#define GL_EFFECTIVE_RASTER_SAMPLES_EXT                               0x932C
#define GL_EIGHTH_BIT_ATI                                             0x00000020
#define GL_ELEMENT_ADDRESS_COMMAND_NV                                 0x0008
#define GL_ELEMENT_ARRAY_ADDRESS_NV                                   0x8F29
#define GL_ELEMENT_ARRAY_APPLE                                        0x8A0C
#define GL_ELEMENT_ARRAY_ATI                                          0x8768
#define GL_ELEMENT_ARRAY_BARRIER_BIT                                  0x00000002
#define GL_ELEMENT_ARRAY_BARRIER_BIT_EXT                              0x00000002
#define GL_ELEMENT_ARRAY_BUFFER                                       0x8893
#define GL_ELEMENT_ARRAY_BUFFER_ARB                                   0x8893
#define GL_ELEMENT_ARRAY_BUFFER_BINDING                               0x8895
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB                           0x8895
#define GL_ELEMENT_ARRAY_LENGTH_NV                                    0x8F33
#define GL_ELEMENT_ARRAY_POINTER_APPLE                                0x8A0E
#define GL_ELEMENT_ARRAY_POINTER_ATI                                  0x876A
#define GL_ELEMENT_ARRAY_TYPE_APPLE                                   0x8A0D
#define GL_ELEMENT_ARRAY_TYPE_ATI                                     0x8769
#define GL_ELEMENT_ARRAY_UNIFIED_NV                                   0x8F1F
#define GL_EMBOSS_CONSTANT_NV                                         0x855E
#define GL_EMBOSS_LIGHT_NV                                            0x855D
#define GL_EMBOSS_MAP_NV                                              0x855F
#define GL_EMISSION                                                   0x1600
#define GL_ENABLE_BIT                                                 0x00002000
#define GL_EQUAL                                                      0x0202
#define GL_EQUIV                                                      0x1509
#define GL_EVAL_2D_NV                                                 0x86C0
#define GL_EVAL_BIT                                                   0x00010000
#define GL_EVAL_FRACTIONAL_TESSELLATION_NV                            0x86C5
#define GL_EVAL_TRIANGULAR_2D_NV                                      0x86C1
#define GL_EVAL_VERTEX_ATTRIB0_NV                                     0x86C6
#define GL_EVAL_VERTEX_ATTRIB10_NV                                    0x86D0
#define GL_EVAL_VERTEX_ATTRIB11_NV                                    0x86D1
#define GL_EVAL_VERTEX_ATTRIB12_NV                                    0x86D2
#define GL_EVAL_VERTEX_ATTRIB13_NV                                    0x86D3
#define GL_EVAL_VERTEX_ATTRIB14_NV                                    0x86D4
#define GL_EVAL_VERTEX_ATTRIB15_NV                                    0x86D5
#define GL_EVAL_VERTEX_ATTRIB1_NV                                     0x86C7
#define GL_EVAL_VERTEX_ATTRIB2_NV                                     0x86C8
#define GL_EVAL_VERTEX_ATTRIB3_NV                                     0x86C9
#define GL_EVAL_VERTEX_ATTRIB4_NV                                     0x86CA
#define GL_EVAL_VERTEX_ATTRIB5_NV                                     0x86CB
#define GL_EVAL_VERTEX_ATTRIB6_NV                                     0x86CC
#define GL_EVAL_VERTEX_ATTRIB7_NV                                     0x86CD
#define GL_EVAL_VERTEX_ATTRIB8_NV                                     0x86CE
#define GL_EVAL_VERTEX_ATTRIB9_NV                                     0x86CF
#define GL_EXCLUSION_KHR                                              0x92A0
#define GL_EXCLUSION_NV                                               0x92A0
#define GL_EXCLUSIVE_EXT                                              0x8F11
#define GL_EXP                                                        0x0800
#define GL_EXP2                                                       0x0801
#define GL_EXPAND_NEGATE_NV                                           0x8539
#define GL_EXPAND_NORMAL_NV                                           0x8538
#define GL_EXTENSIONS                                                 0x1F03
#define GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD                         0x9160
#define GL_EYE_DISTANCE_TO_LINE_SGIS                                  0x81F2
#define GL_EYE_DISTANCE_TO_POINT_SGIS                                 0x81F0
#define GL_EYE_LINEAR                                                 0x2400
#define GL_EYE_LINEAR_NV                                              0x2400
#define GL_EYE_LINE_SGIS                                              0x81F6
#define GL_EYE_PLANE                                                  0x2502
#define GL_EYE_PLANE_ABSOLUTE_NV                                      0x855C
#define GL_EYE_POINT_SGIS                                             0x81F4
#define GL_EYE_RADIAL_NV                                              0x855B
#define GL_E_TIMES_F_NV                                               0x8531
#define GL_FACTOR_MAX_AMD                                             0x901D
#define GL_FACTOR_MIN_AMD                                             0x901C
#define GL_FAILURE_NV                                                 0x9030
#define GL_FALSE                                                      0
#define GL_FASTEST                                                    0x1101
#define GL_FEEDBACK                                                   0x1C01
#define GL_FEEDBACK_BUFFER_POINTER                                    0x0DF0
#define GL_FEEDBACK_BUFFER_SIZE                                       0x0DF1
#define GL_FEEDBACK_BUFFER_TYPE                                       0x0DF2
#define GL_FENCE_APPLE                                                0x8A0B
#define GL_FENCE_CONDITION_NV                                         0x84F4
#define GL_FENCE_STATUS_NV                                            0x84F3
#define GL_FIELDS_NV                                                  0x8E27
#define GL_FIELD_LOWER_NV                                             0x9023
#define GL_FIELD_UPPER_NV                                             0x9022
#define GL_FILE_NAME_NV                                               0x9074
#define GL_FILL                                                       0x1B02
#define GL_FILL_RECTANGLE_NV                                          0x933C
#define GL_FILTER                                                     0x829A
#define GL_FILTER4_SGIS                                               0x8146
#define GL_FIRST_TO_REST_NV                                           0x90AF
#define GL_FIRST_VERTEX_CONVENTION                                    0x8E4D
#define GL_FIRST_VERTEX_CONVENTION_EXT                                0x8E4D
#define GL_FIXED                                                      0x140C
#define GL_FIXED_OES                                                  0x140C
#define GL_FIXED_ONLY                                                 0x891D
#define GL_FIXED_ONLY_ARB                                             0x891D
#define GL_FLAT                                                       0x1D00
#define GL_FLOAT                                                      0x1406
#define GL_FLOAT16_MAT2_AMD                                           0x91C5
#define GL_FLOAT16_MAT2x3_AMD                                         0x91C8
#define GL_FLOAT16_MAT2x4_AMD                                         0x91C9
#define GL_FLOAT16_MAT3_AMD                                           0x91C6
#define GL_FLOAT16_MAT3x2_AMD                                         0x91CA
#define GL_FLOAT16_MAT3x4_AMD                                         0x91CB
#define GL_FLOAT16_MAT4_AMD                                           0x91C7
#define GL_FLOAT16_MAT4x2_AMD                                         0x91CC
#define GL_FLOAT16_MAT4x3_AMD                                         0x91CD
#define GL_FLOAT16_NV                                                 0x8FF8
#define GL_FLOAT16_VEC2_NV                                            0x8FF9
#define GL_FLOAT16_VEC3_NV                                            0x8FFA
#define GL_FLOAT16_VEC4_NV                                            0x8FFB
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV                             0x8DAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV_NV                          0x8DAD
#define GL_FLOAT_CLEAR_COLOR_VALUE_NV                                 0x888D
#define GL_FLOAT_MAT2                                                 0x8B5A
#define GL_FLOAT_MAT2_ARB                                             0x8B5A
#define GL_FLOAT_MAT2x3                                               0x8B65
#define GL_FLOAT_MAT2x4                                               0x8B66
#define GL_FLOAT_MAT3                                                 0x8B5B
#define GL_FLOAT_MAT3_ARB                                             0x8B5B
#define GL_FLOAT_MAT3x2                                               0x8B67
#define GL_FLOAT_MAT3x4                                               0x8B68
#define GL_FLOAT_MAT4                                                 0x8B5C
#define GL_FLOAT_MAT4_ARB                                             0x8B5C
#define GL_FLOAT_MAT4x2                                               0x8B69
#define GL_FLOAT_MAT4x3                                               0x8B6A
#define GL_FLOAT_R16_NV                                               0x8884
#define GL_FLOAT_R32_NV                                               0x8885
#define GL_FLOAT_RG16_NV                                              0x8886
#define GL_FLOAT_RG32_NV                                              0x8887
#define GL_FLOAT_RGB16_NV                                             0x8888
#define GL_FLOAT_RGB32_NV                                             0x8889
#define GL_FLOAT_RGBA16_NV                                            0x888A
#define GL_FLOAT_RGBA32_NV                                            0x888B
#define GL_FLOAT_RGBA_MODE_NV                                         0x888E
#define GL_FLOAT_RGBA_NV                                              0x8883
#define GL_FLOAT_RGB_NV                                               0x8882
#define GL_FLOAT_RG_NV                                                0x8881
#define GL_FLOAT_R_NV                                                 0x8880
#define GL_FLOAT_VEC2                                                 0x8B50
#define GL_FLOAT_VEC2_ARB                                             0x8B50
#define GL_FLOAT_VEC3                                                 0x8B51
#define GL_FLOAT_VEC3_ARB                                             0x8B51
#define GL_FLOAT_VEC4                                                 0x8B52
#define GL_FLOAT_VEC4_ARB                                             0x8B52
#define GL_FOG                                                        0x0B60
#define GL_FOG_BIT                                                    0x00000080
#define GL_FOG_COLOR                                                  0x0B66
#define GL_FOG_COORD                                                  0x8451
#define GL_FOG_COORDINATE                                             0x8451
#define GL_FOG_COORDINATE_ARRAY                                       0x8457
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING                        0x889D
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB                    0x889D
#define GL_FOG_COORDINATE_ARRAY_EXT                                   0x8457
#define GL_FOG_COORDINATE_ARRAY_LIST_IBM                              103076
#define GL_FOG_COORDINATE_ARRAY_LIST_STRIDE_IBM                       103086
#define GL_FOG_COORDINATE_ARRAY_POINTER                               0x8456
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT                           0x8456
#define GL_FOG_COORDINATE_ARRAY_STRIDE                                0x8455
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT                            0x8455
#define GL_FOG_COORDINATE_ARRAY_TYPE                                  0x8454
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT                              0x8454
#define GL_FOG_COORDINATE_EXT                                         0x8451
#define GL_FOG_COORDINATE_SOURCE                                      0x8450
#define GL_FOG_COORDINATE_SOURCE_EXT                                  0x8450
#define GL_FOG_COORD_ARRAY                                            0x8457
#define GL_FOG_COORD_ARRAY_ADDRESS_NV                                 0x8F28
#define GL_FOG_COORD_ARRAY_BUFFER_BINDING                             0x889D
#define GL_FOG_COORD_ARRAY_LENGTH_NV                                  0x8F32
#define GL_FOG_COORD_ARRAY_POINTER                                    0x8456
#define GL_FOG_COORD_ARRAY_STRIDE                                     0x8455
#define GL_FOG_COORD_ARRAY_TYPE                                       0x8454
#define GL_FOG_COORD_SRC                                              0x8450
#define GL_FOG_DENSITY                                                0x0B62
#define GL_FOG_DISTANCE_MODE_NV                                       0x855A
#define GL_FOG_END                                                    0x0B64
#define GL_FOG_FUNC_POINTS_SGIS                                       0x812B
#define GL_FOG_FUNC_SGIS                                              0x812A
#define GL_FOG_HINT                                                   0x0C54
#define GL_FOG_INDEX                                                  0x0B61
#define GL_FOG_MODE                                                   0x0B65
#define GL_FOG_OFFSET_SGIX                                            0x8198
#define GL_FOG_OFFSET_VALUE_SGIX                                      0x8199
#define GL_FOG_SPECULAR_TEXTURE_WIN                                   0x80EC
#define GL_FOG_START                                                  0x0B63
#define GL_FONT_ASCENDER_BIT_NV                                       0x00200000
#define GL_FONT_DESCENDER_BIT_NV                                      0x00400000
#define GL_FONT_GLYPHS_AVAILABLE_NV                                   0x9368
#define GL_FONT_HAS_KERNING_BIT_NV                                    0x10000000
#define GL_FONT_HEIGHT_BIT_NV                                         0x00800000
#define GL_FONT_MAX_ADVANCE_HEIGHT_BIT_NV                             0x02000000
#define GL_FONT_MAX_ADVANCE_WIDTH_BIT_NV                              0x01000000
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV                              0x20000000
#define GL_FONT_TARGET_UNAVAILABLE_NV                                 0x9369
#define GL_FONT_UNAVAILABLE_NV                                        0x936A
#define GL_FONT_UNDERLINE_POSITION_BIT_NV                             0x04000000
#define GL_FONT_UNDERLINE_THICKNESS_BIT_NV                            0x08000000
#define GL_FONT_UNINTELLIGIBLE_NV                                     0x936B
#define GL_FONT_UNITS_PER_EM_BIT_NV                                   0x00100000
#define GL_FONT_X_MAX_BOUNDS_BIT_NV                                   0x00040000
#define GL_FONT_X_MIN_BOUNDS_BIT_NV                                   0x00010000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV                                   0x00080000
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV                                   0x00020000
#define GL_FORCE_BLUE_TO_ONE_NV                                       0x8860
#define GL_FORMAT_SUBSAMPLE_244_244_OML                               0x8983
#define GL_FORMAT_SUBSAMPLE_24_24_OML                                 0x8982
#define GL_FRACTIONAL_EVEN                                            0x8E7C
#define GL_FRACTIONAL_ODD                                             0x8E7B
#define GL_FRAGMENT_COLOR_EXT                                         0x834C
#define GL_FRAGMENT_COLOR_MATERIAL_FACE_SGIX                          0x8402
#define GL_FRAGMENT_COLOR_MATERIAL_PARAMETER_SGIX                     0x8403
#define GL_FRAGMENT_COLOR_MATERIAL_SGIX                               0x8401
#define GL_FRAGMENT_COVERAGE_COLOR_NV                                 0x92DE
#define GL_FRAGMENT_COVERAGE_TO_COLOR_NV                              0x92DD
#define GL_FRAGMENT_DEPTH                                             0x8452
#define GL_FRAGMENT_DEPTH_EXT                                         0x8452
#define GL_FRAGMENT_INPUT_NV                                          0x936D
#define GL_FRAGMENT_INTERPOLATION_OFFSET_BITS                         0x8E5D
#define GL_FRAGMENT_LIGHT0_SGIX                                       0x840C
#define GL_FRAGMENT_LIGHT1_SGIX                                       0x840D
#define GL_FRAGMENT_LIGHT2_SGIX                                       0x840E
#define GL_FRAGMENT_LIGHT3_SGIX                                       0x840F
#define GL_FRAGMENT_LIGHT4_SGIX                                       0x8410
#define GL_FRAGMENT_LIGHT5_SGIX                                       0x8411
#define GL_FRAGMENT_LIGHT6_SGIX                                       0x8412
#define GL_FRAGMENT_LIGHT7_SGIX                                       0x8413
#define GL_FRAGMENT_LIGHTING_SGIX                                     0x8400
#define GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX                          0x840A
#define GL_FRAGMENT_LIGHT_MODEL_LOCAL_VIEWER_SGIX                     0x8408
#define GL_FRAGMENT_LIGHT_MODEL_NORMAL_INTERPOLATION_SGIX             0x840B
#define GL_FRAGMENT_LIGHT_MODEL_TWO_SIDE_SGIX                         0x8409
#define GL_FRAGMENT_MATERIAL_EXT                                      0x8349
#define GL_FRAGMENT_NORMAL_EXT                                        0x834A
#define GL_FRAGMENT_PROGRAM_ARB                                       0x8804
#define GL_FRAGMENT_PROGRAM_BINDING_NV                                0x8873
#define GL_FRAGMENT_PROGRAM_INTERPOLATION_OFFSET_BITS_NV              0x8E5D
#define GL_FRAGMENT_PROGRAM_NV                                        0x8870
#define GL_FRAGMENT_PROGRAM_PARAMETER_BUFFER_NV                       0x8DA4
#define GL_FRAGMENT_SHADER                                            0x8B30
#define GL_FRAGMENT_SHADER_ARB                                        0x8B30
#define GL_FRAGMENT_SHADER_ATI                                        0x8920
#define GL_FRAGMENT_SHADER_BIT                                        0x00000002
#define GL_FRAGMENT_SHADER_BIT_EXT                                    0x00000002
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT                            0x8B8B
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB                        0x8B8B
#define GL_FRAGMENT_SHADER_DISCARDS_SAMPLES_EXT                       0x8A52
#define GL_FRAGMENT_SHADER_INVOCATIONS                                0x82F4
#define GL_FRAGMENT_SHADER_INVOCATIONS_ARB                            0x82F4
#define GL_FRAGMENT_SUBROUTINE                                        0x92EC
#define GL_FRAGMENT_SUBROUTINE_UNIFORM                                0x92F2
#define GL_FRAGMENT_TEXTURE                                           0x829F
#define GL_FRAMEBUFFER                                                0x8D40
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE                          0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE                           0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING                      0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE                      0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE                          0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE                          0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED                             0x8DA7
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_ARB                         0x8DA7
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT                         0x8DA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME                         0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT                     0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE                         0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT                     0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE                            0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE                        0x8217
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT              0x8CD4
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR         0x9632
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE               0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT           0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER                       0x8CD4
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT                   0x8CD4
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL                       0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT                   0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR               0x9630
#define GL_FRAMEBUFFER_BARRIER_BIT                                    0x00000400
#define GL_FRAMEBUFFER_BARRIER_BIT_EXT                                0x00000400
#define GL_FRAMEBUFFER_BINDING                                        0x8CA6
#define GL_FRAMEBUFFER_BINDING_EXT                                    0x8CA6
#define GL_FRAMEBUFFER_BLEND                                          0x828B
#define GL_FRAMEBUFFER_COMPLETE                                       0x8CD5
#define GL_FRAMEBUFFER_COMPLETE_EXT                                   0x8CD5
#define GL_FRAMEBUFFER_DEFAULT                                        0x8218
#define GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS                 0x9314
#define GL_FRAMEBUFFER_DEFAULT_HEIGHT                                 0x9311
#define GL_FRAMEBUFFER_DEFAULT_LAYERS                                 0x9312
#define GL_FRAMEBUFFER_DEFAULT_SAMPLES                                0x9313
#define GL_FRAMEBUFFER_DEFAULT_WIDTH                                  0x9310
#define GL_FRAMEBUFFER_EXT                                            0x8D40
#define GL_FRAMEBUFFER_FLIP_Y_MESA                                    0x8BBB
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT                          0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT                      0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT                      0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER                         0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT                     0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                         0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_ARB                     0x8DA9
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT                     0x8DA9
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS                       0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_ARB                   0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT                   0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT                  0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT              0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE                         0x8D56
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT                     0x8D56
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER                         0x8CDC
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT                     0x8CDC
#define GL_FRAMEBUFFER_INCOMPLETE_VIEW_TARGETS_OVR                    0x9633
#define GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_ARB              0x9342
#define GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_NV               0x9342
#define GL_FRAMEBUFFER_RENDERABLE                                     0x8289
#define GL_FRAMEBUFFER_RENDERABLE_LAYERED                             0x828A
#define GL_FRAMEBUFFER_SAMPLE_LOCATION_PIXEL_GRID_ARB                 0x9343
#define GL_FRAMEBUFFER_SAMPLE_LOCATION_PIXEL_GRID_NV                  0x9343
#define GL_FRAMEBUFFER_SRGB                                           0x8DB9
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT                               0x8DBA
#define GL_FRAMEBUFFER_SRGB_EXT                                       0x8DB9
#define GL_FRAMEBUFFER_UNDEFINED                                      0x8219
#define GL_FRAMEBUFFER_UNSUPPORTED                                    0x8CDD
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                                0x8CDD
#define GL_FRAMEZOOM_FACTOR_SGIX                                      0x818C
#define GL_FRAMEZOOM_SGIX                                             0x818B
#define GL_FRAME_NV                                                   0x8E26
#define GL_FRONT                                                      0x0404
#define GL_FRONT_AND_BACK                                             0x0408
#define GL_FRONT_FACE                                                 0x0B46
#define GL_FRONT_FACE_COMMAND_NV                                      0x0012
#define GL_FRONT_LEFT                                                 0x0400
#define GL_FRONT_RIGHT                                                0x0401
#define GL_FULL_RANGE_EXT                                             0x87E1
#define GL_FULL_STIPPLE_HINT_PGI                                      0x1A219
#define GL_FULL_SUPPORT                                               0x82B7
#define GL_FUNC_ADD                                                   0x8006
#define GL_FUNC_ADD_EXT                                               0x8006
#define GL_FUNC_REVERSE_SUBTRACT                                      0x800B
#define GL_FUNC_REVERSE_SUBTRACT_EXT                                  0x800B
#define GL_FUNC_SUBTRACT                                              0x800A
#define GL_FUNC_SUBTRACT_EXT                                          0x800A
#define GL_GENERATE_MIPMAP                                            0x8191
#define GL_GENERATE_MIPMAP_HINT                                       0x8192
#define GL_GENERATE_MIPMAP_HINT_SGIS                                  0x8192
#define GL_GENERATE_MIPMAP_SGIS                                       0x8191
#define GL_GENERIC_ATTRIB_NV                                          0x8C7D
#define GL_GEOMETRY_DEFORMATION_BIT_SGIX                              0x00000002
#define GL_GEOMETRY_DEFORMATION_SGIX                                  0x8194
#define GL_GEOMETRY_INPUT_TYPE                                        0x8917
#define GL_GEOMETRY_INPUT_TYPE_ARB                                    0x8DDB
#define GL_GEOMETRY_INPUT_TYPE_EXT                                    0x8DDB
#define GL_GEOMETRY_OUTPUT_TYPE                                       0x8918
#define GL_GEOMETRY_OUTPUT_TYPE_ARB                                   0x8DDC
#define GL_GEOMETRY_OUTPUT_TYPE_EXT                                   0x8DDC
#define GL_GEOMETRY_PROGRAM_NV                                        0x8C26
#define GL_GEOMETRY_PROGRAM_PARAMETER_BUFFER_NV                       0x8DA3
#define GL_GEOMETRY_SHADER                                            0x8DD9
#define GL_GEOMETRY_SHADER_ARB                                        0x8DD9
#define GL_GEOMETRY_SHADER_BIT                                        0x00000004
#define GL_GEOMETRY_SHADER_EXT                                        0x8DD9
#define GL_GEOMETRY_SHADER_INVOCATIONS                                0x887F
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED                         0x82F3
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB                     0x82F3
#define GL_GEOMETRY_SUBROUTINE                                        0x92EB
#define GL_GEOMETRY_SUBROUTINE_UNIFORM                                0x92F1
#define GL_GEOMETRY_TEXTURE                                           0x829E
#define GL_GEOMETRY_VERTICES_OUT                                      0x8916
#define GL_GEOMETRY_VERTICES_OUT_ARB                                  0x8DDA
#define GL_GEOMETRY_VERTICES_OUT_EXT                                  0x8DDA
#define GL_GEQUAL                                                     0x0206
#define GL_GET_TEXTURE_IMAGE_FORMAT                                   0x8291
#define GL_GET_TEXTURE_IMAGE_TYPE                                     0x8292
#define GL_GLOBAL_ALPHA_FACTOR_SUN                                    0x81DA
#define GL_GLOBAL_ALPHA_SUN                                           0x81D9
#define GL_GLYPH_HAS_KERNING_BIT_NV                                   0x100
#define GL_GLYPH_HEIGHT_BIT_NV                                        0x02
#define GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV                    0x10
#define GL_GLYPH_HORIZONTAL_BEARING_X_BIT_NV                          0x04
#define GL_GLYPH_HORIZONTAL_BEARING_Y_BIT_NV                          0x08
#define GL_GLYPH_VERTICAL_BEARING_ADVANCE_BIT_NV                      0x80
#define GL_GLYPH_VERTICAL_BEARING_X_BIT_NV                            0x20
#define GL_GLYPH_VERTICAL_BEARING_Y_BIT_NV                            0x40
#define GL_GLYPH_WIDTH_BIT_NV                                         0x01
#define GL_GPU_ADDRESS_NV                                             0x8F34
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX               0x9049
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX                       0x9047
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX                         0x904B
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX                         0x904A
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX                 0x9048
#define GL_GREATER                                                    0x0204
#define GL_GREEN                                                      0x1904
#define GL_GREEN_BIAS                                                 0x0D19
#define GL_GREEN_BITS                                                 0x0D53
#define GL_GREEN_BIT_ATI                                              0x00000002
#define GL_GREEN_INTEGER                                              0x8D95
#define GL_GREEN_INTEGER_EXT                                          0x8D95
#define GL_GREEN_MAX_CLAMP_INGR                                       0x8565
#define GL_GREEN_MIN_CLAMP_INGR                                       0x8561
#define GL_GREEN_NV                                                   0x1904
#define GL_GREEN_SCALE                                                0x0D18
#define GL_GUILTY_CONTEXT_RESET                                       0x8253
#define GL_GUILTY_CONTEXT_RESET_ARB                                   0x8253
#define GL_GUILTY_CONTEXT_RESET_KHR                                   0x8253
#define GL_HALF_APPLE                                                 0x140B
#define GL_HALF_BIAS_NEGATE_NV                                        0x853B
#define GL_HALF_BIAS_NORMAL_NV                                        0x853A
#define GL_HALF_BIT_ATI                                               0x00000008
#define GL_HALF_FLOAT                                                 0x140B
#define GL_HALF_FLOAT_ARB                                             0x140B
#define GL_HALF_FLOAT_NV                                              0x140B
#define GL_HANDLE_TYPE_D3D11_IMAGE_EXT                                0x958B
#define GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT                            0x958C
#define GL_HANDLE_TYPE_D3D12_FENCE_EXT                                0x9594
#define GL_HANDLE_TYPE_D3D12_RESOURCE_EXT                             0x958A
#define GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT                             0x9589
#define GL_HANDLE_TYPE_OPAQUE_FD_EXT                                  0x9586
#define GL_HANDLE_TYPE_OPAQUE_WIN32_EXT                               0x9587
#define GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT                           0x9588
#define GL_HARDLIGHT_KHR                                              0x929B
#define GL_HARDLIGHT_NV                                               0x929B
#define GL_HARDMIX_NV                                                 0x92A9
#define GL_HIGH_FLOAT                                                 0x8DF2
#define GL_HIGH_INT                                                   0x8DF5
#define GL_HILO16_NV                                                  0x86F8
#define GL_HILO8_NV                                                   0x885E
#define GL_HILO_NV                                                    0x86F4
#define GL_HINT_BIT                                                   0x00008000
#define GL_HISTOGRAM                                                  0x8024
#define GL_HISTOGRAM_ALPHA_SIZE                                       0x802B
#define GL_HISTOGRAM_ALPHA_SIZE_EXT                                   0x802B
#define GL_HISTOGRAM_BLUE_SIZE                                        0x802A
#define GL_HISTOGRAM_BLUE_SIZE_EXT                                    0x802A
#define GL_HISTOGRAM_EXT                                              0x8024
#define GL_HISTOGRAM_FORMAT                                           0x8027
#define GL_HISTOGRAM_FORMAT_EXT                                       0x8027
#define GL_HISTOGRAM_GREEN_SIZE                                       0x8029
#define GL_HISTOGRAM_GREEN_SIZE_EXT                                   0x8029
#define GL_HISTOGRAM_LUMINANCE_SIZE                                   0x802C
#define GL_HISTOGRAM_LUMINANCE_SIZE_EXT                               0x802C
#define GL_HISTOGRAM_RED_SIZE                                         0x8028
#define GL_HISTOGRAM_RED_SIZE_EXT                                     0x8028
#define GL_HISTOGRAM_SINK                                             0x802D
#define GL_HISTOGRAM_SINK_EXT                                         0x802D
#define GL_HISTOGRAM_WIDTH                                            0x8026
#define GL_HISTOGRAM_WIDTH_EXT                                        0x8026
#define GL_HI_BIAS_NV                                                 0x8714
#define GL_HI_SCALE_NV                                                0x870E
#define GL_HORIZONTAL_LINE_TO_NV                                      0x06
#define GL_HSL_COLOR_KHR                                              0x92AF
#define GL_HSL_COLOR_NV                                               0x92AF
#define GL_HSL_HUE_KHR                                                0x92AD
#define GL_HSL_HUE_NV                                                 0x92AD
#define GL_HSL_LUMINOSITY_KHR                                         0x92B0
#define GL_HSL_LUMINOSITY_NV                                          0x92B0
#define GL_HSL_SATURATION_KHR                                         0x92AE
#define GL_HSL_SATURATION_NV                                          0x92AE
#define GL_IDENTITY_NV                                                0x862A
#define GL_IGNORE_BORDER_HP                                           0x8150
#define GL_IMAGE_1D                                                   0x904C
#define GL_IMAGE_1D_ARRAY                                             0x9052
#define GL_IMAGE_1D_ARRAY_EXT                                         0x9052
#define GL_IMAGE_1D_EXT                                               0x904C
#define GL_IMAGE_2D                                                   0x904D
#define GL_IMAGE_2D_ARRAY                                             0x9053
#define GL_IMAGE_2D_ARRAY_EXT                                         0x9053
#define GL_IMAGE_2D_EXT                                               0x904D
#define GL_IMAGE_2D_MULTISAMPLE                                       0x9055
#define GL_IMAGE_2D_MULTISAMPLE_ARRAY                                 0x9056
#define GL_IMAGE_2D_MULTISAMPLE_ARRAY_EXT                             0x9056
#define GL_IMAGE_2D_MULTISAMPLE_EXT                                   0x9055
#define GL_IMAGE_2D_RECT                                              0x904F
#define GL_IMAGE_2D_RECT_EXT                                          0x904F
#define GL_IMAGE_3D                                                   0x904E
#define GL_IMAGE_3D_EXT                                               0x904E
#define GL_IMAGE_BINDING_ACCESS                                       0x8F3E
#define GL_IMAGE_BINDING_ACCESS_EXT                                   0x8F3E
#define GL_IMAGE_BINDING_FORMAT                                       0x906E
#define GL_IMAGE_BINDING_FORMAT_EXT                                   0x906E
#define GL_IMAGE_BINDING_LAYER                                        0x8F3D
#define GL_IMAGE_BINDING_LAYERED                                      0x8F3C
#define GL_IMAGE_BINDING_LAYERED_EXT                                  0x8F3C
#define GL_IMAGE_BINDING_LAYER_EXT                                    0x8F3D
#define GL_IMAGE_BINDING_LEVEL                                        0x8F3B
#define GL_IMAGE_BINDING_LEVEL_EXT                                    0x8F3B
#define GL_IMAGE_BINDING_NAME                                         0x8F3A
#define GL_IMAGE_BINDING_NAME_EXT                                     0x8F3A
#define GL_IMAGE_BUFFER                                               0x9051
#define GL_IMAGE_BUFFER_EXT                                           0x9051
#define GL_IMAGE_CLASS_10_10_10_2                                     0x82C3
#define GL_IMAGE_CLASS_11_11_10                                       0x82C2
#define GL_IMAGE_CLASS_1_X_16                                         0x82BE
#define GL_IMAGE_CLASS_1_X_32                                         0x82BB
#define GL_IMAGE_CLASS_1_X_8                                          0x82C1
#define GL_IMAGE_CLASS_2_X_16                                         0x82BD
#define GL_IMAGE_CLASS_2_X_32                                         0x82BA
#define GL_IMAGE_CLASS_2_X_8                                          0x82C0
#define GL_IMAGE_CLASS_4_X_16                                         0x82BC
#define GL_IMAGE_CLASS_4_X_32                                         0x82B9
#define GL_IMAGE_CLASS_4_X_8                                          0x82BF
#define GL_IMAGE_COMPATIBILITY_CLASS                                  0x82A8
#define GL_IMAGE_CUBE                                                 0x9050
#define GL_IMAGE_CUBE_EXT                                             0x9050
#define GL_IMAGE_CUBE_MAP_ARRAY                                       0x9054
#define GL_IMAGE_CUBE_MAP_ARRAY_EXT                                   0x9054
#define GL_IMAGE_CUBIC_WEIGHT_HP                                      0x815E
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS                        0x90C9
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE                         0x90C8
#define GL_IMAGE_FORMAT_COMPATIBILITY_TYPE                            0x90C7
#define GL_IMAGE_MAG_FILTER_HP                                        0x815C
#define GL_IMAGE_MIN_FILTER_HP                                        0x815D
#define GL_IMAGE_PIXEL_FORMAT                                         0x82A9
#define GL_IMAGE_PIXEL_TYPE                                           0x82AA
#define GL_IMAGE_ROTATE_ANGLE_HP                                      0x8159
#define GL_IMAGE_ROTATE_ORIGIN_X_HP                                   0x815A
#define GL_IMAGE_ROTATE_ORIGIN_Y_HP                                   0x815B
#define GL_IMAGE_SCALE_X_HP                                           0x8155
#define GL_IMAGE_SCALE_Y_HP                                           0x8156
#define GL_IMAGE_TEXEL_SIZE                                           0x82A7
#define GL_IMAGE_TRANSFORM_2D_HP                                      0x8161
#define GL_IMAGE_TRANSLATE_X_HP                                       0x8157
#define GL_IMAGE_TRANSLATE_Y_HP                                       0x8158
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT                           0x8B9B
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES                       0x8B9B
#define GL_IMPLEMENTATION_COLOR_READ_TYPE                             0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_TYPE_OES                         0x8B9A
#define GL_INCLUSIVE_EXT                                              0x8F10
#define GL_INCR                                                       0x1E02
#define GL_INCR_WRAP                                                  0x8507
#define GL_INCR_WRAP_EXT                                              0x8507
#define GL_INDEX                                                      0x8222
#define GL_INDEX_ARRAY                                                0x8077
#define GL_INDEX_ARRAY_ADDRESS_NV                                     0x8F24
#define GL_INDEX_ARRAY_BUFFER_BINDING                                 0x8899
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB                             0x8899
#define GL_INDEX_ARRAY_COUNT_EXT                                      0x8087
#define GL_INDEX_ARRAY_EXT                                            0x8077
#define GL_INDEX_ARRAY_LENGTH_NV                                      0x8F2E
#define GL_INDEX_ARRAY_LIST_IBM                                       103073
#define GL_INDEX_ARRAY_LIST_STRIDE_IBM                                103083
#define GL_INDEX_ARRAY_POINTER                                        0x8091
#define GL_INDEX_ARRAY_POINTER_EXT                                    0x8091
#define GL_INDEX_ARRAY_STRIDE                                         0x8086
#define GL_INDEX_ARRAY_STRIDE_EXT                                     0x8086
#define GL_INDEX_ARRAY_TYPE                                           0x8085
#define GL_INDEX_ARRAY_TYPE_EXT                                       0x8085
#define GL_INDEX_BITS                                                 0x0D51
#define GL_INDEX_BIT_PGI                                              0x00080000
#define GL_INDEX_CLEAR_VALUE                                          0x0C20
#define GL_INDEX_LOGIC_OP                                             0x0BF1
#define GL_INDEX_MATERIAL_EXT                                         0x81B8
#define GL_INDEX_MATERIAL_FACE_EXT                                    0x81BA
#define GL_INDEX_MATERIAL_PARAMETER_EXT                               0x81B9
#define GL_INDEX_MODE                                                 0x0C30
#define GL_INDEX_OFFSET                                               0x0D13
#define GL_INDEX_SHIFT                                                0x0D12
#define GL_INDEX_TEST_EXT                                             0x81B5
#define GL_INDEX_TEST_FUNC_EXT                                        0x81B6
#define GL_INDEX_TEST_REF_EXT                                         0x81B7
#define GL_INDEX_WRITEMASK                                            0x0C21
#define GL_INFO_LOG_LENGTH                                            0x8B84
#define GL_INNOCENT_CONTEXT_RESET                                     0x8254
#define GL_INNOCENT_CONTEXT_RESET_ARB                                 0x8254
#define GL_INNOCENT_CONTEXT_RESET_KHR                                 0x8254
#define GL_INSTRUMENT_BUFFER_POINTER_SGIX                             0x8180
#define GL_INSTRUMENT_MEASUREMENTS_SGIX                               0x8181
#define GL_INT                                                        0x1404
#define GL_INT16_NV                                                   0x8FE4
#define GL_INT16_VEC2_NV                                              0x8FE5
#define GL_INT16_VEC3_NV                                              0x8FE6
#define GL_INT16_VEC4_NV                                              0x8FE7
#define GL_INT64_ARB                                                  0x140E
#define GL_INT64_NV                                                   0x140E
#define GL_INT64_VEC2_ARB                                             0x8FE9
#define GL_INT64_VEC2_NV                                              0x8FE9
#define GL_INT64_VEC3_ARB                                             0x8FEA
#define GL_INT64_VEC3_NV                                              0x8FEA
#define GL_INT64_VEC4_ARB                                             0x8FEB
#define GL_INT64_VEC4_NV                                              0x8FEB
#define GL_INT8_NV                                                    0x8FE0
#define GL_INT8_VEC2_NV                                               0x8FE1
#define GL_INT8_VEC3_NV                                               0x8FE2
#define GL_INT8_VEC4_NV                                               0x8FE3
#define GL_INTENSITY                                                  0x8049
#define GL_INTENSITY12                                                0x804C
#define GL_INTENSITY12_EXT                                            0x804C
#define GL_INTENSITY16                                                0x804D
#define GL_INTENSITY16F_ARB                                           0x881D
#define GL_INTENSITY16I_EXT                                           0x8D8B
#define GL_INTENSITY16UI_EXT                                          0x8D79
#define GL_INTENSITY16_EXT                                            0x804D
#define GL_INTENSITY16_SNORM                                          0x901B
#define GL_INTENSITY32F_ARB                                           0x8817
#define GL_INTENSITY32I_EXT                                           0x8D85
#define GL_INTENSITY32UI_EXT                                          0x8D73
#define GL_INTENSITY4                                                 0x804A
#define GL_INTENSITY4_EXT                                             0x804A
#define GL_INTENSITY8                                                 0x804B
#define GL_INTENSITY8I_EXT                                            0x8D91
#define GL_INTENSITY8UI_EXT                                           0x8D7F
#define GL_INTENSITY8_EXT                                             0x804B
#define GL_INTENSITY8_SNORM                                           0x9017
#define GL_INTENSITY_EXT                                              0x8049
#define GL_INTENSITY_FLOAT16_APPLE                                    0x881D
#define GL_INTENSITY_FLOAT16_ATI                                      0x881D
#define GL_INTENSITY_FLOAT32_APPLE                                    0x8817
#define GL_INTENSITY_FLOAT32_ATI                                      0x8817
#define GL_INTENSITY_SNORM                                            0x9013
#define GL_INTERLACE_OML                                              0x8980
#define GL_INTERLACE_READ_INGR                                        0x8568
#define GL_INTERLACE_READ_OML                                         0x8981
#define GL_INTERLACE_SGIX                                             0x8094
#define GL_INTERLEAVED_ATTRIBS                                        0x8C8C
#define GL_INTERLEAVED_ATTRIBS_EXT                                    0x8C8C
#define GL_INTERLEAVED_ATTRIBS_NV                                     0x8C8C
#define GL_INTERNALFORMAT_ALPHA_SIZE                                  0x8274
#define GL_INTERNALFORMAT_ALPHA_TYPE                                  0x827B
#define GL_INTERNALFORMAT_BLUE_SIZE                                   0x8273
#define GL_INTERNALFORMAT_BLUE_TYPE                                   0x827A
#define GL_INTERNALFORMAT_DEPTH_SIZE                                  0x8275
#define GL_INTERNALFORMAT_DEPTH_TYPE                                  0x827C
#define GL_INTERNALFORMAT_GREEN_SIZE                                  0x8272
#define GL_INTERNALFORMAT_GREEN_TYPE                                  0x8279
#define GL_INTERNALFORMAT_PREFERRED                                   0x8270
#define GL_INTERNALFORMAT_RED_SIZE                                    0x8271
#define GL_INTERNALFORMAT_RED_TYPE                                    0x8278
#define GL_INTERNALFORMAT_SHARED_SIZE                                 0x8277
#define GL_INTERNALFORMAT_STENCIL_SIZE                                0x8276
#define GL_INTERNALFORMAT_STENCIL_TYPE                                0x827D
#define GL_INTERNALFORMAT_SUPPORTED                                   0x826F
#define GL_INTERPOLATE                                                0x8575
#define GL_INTERPOLATE_ARB                                            0x8575
#define GL_INTERPOLATE_EXT                                            0x8575
#define GL_INT_2_10_10_10_REV                                         0x8D9F
#define GL_INT_IMAGE_1D                                               0x9057
#define GL_INT_IMAGE_1D_ARRAY                                         0x905D
#define GL_INT_IMAGE_1D_ARRAY_EXT                                     0x905D
#define GL_INT_IMAGE_1D_EXT                                           0x9057
#define GL_INT_IMAGE_2D                                               0x9058
#define GL_INT_IMAGE_2D_ARRAY                                         0x905E
#define GL_INT_IMAGE_2D_ARRAY_EXT                                     0x905E
#define GL_INT_IMAGE_2D_EXT                                           0x9058
#define GL_INT_IMAGE_2D_MULTISAMPLE                                   0x9060
#define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY                             0x9061
#define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT                         0x9061
#define GL_INT_IMAGE_2D_MULTISAMPLE_EXT                               0x9060
#define GL_INT_IMAGE_2D_RECT                                          0x905A
#define GL_INT_IMAGE_2D_RECT_EXT                                      0x905A
#define GL_INT_IMAGE_3D                                               0x9059
#define GL_INT_IMAGE_3D_EXT                                           0x9059
#define GL_INT_IMAGE_BUFFER                                           0x905C
#define GL_INT_IMAGE_BUFFER_EXT                                       0x905C
#define GL_INT_IMAGE_CUBE                                             0x905B
#define GL_INT_IMAGE_CUBE_EXT                                         0x905B
#define GL_INT_IMAGE_CUBE_MAP_ARRAY                                   0x905F
#define GL_INT_IMAGE_CUBE_MAP_ARRAY_EXT                               0x905F
#define GL_INT_SAMPLER_1D                                             0x8DC9
#define GL_INT_SAMPLER_1D_ARRAY                                       0x8DCE
#define GL_INT_SAMPLER_1D_ARRAY_EXT                                   0x8DCE
#define GL_INT_SAMPLER_1D_EXT                                         0x8DC9
#define GL_INT_SAMPLER_2D                                             0x8DCA
#define GL_INT_SAMPLER_2D_ARRAY                                       0x8DCF
#define GL_INT_SAMPLER_2D_ARRAY_EXT                                   0x8DCF
#define GL_INT_SAMPLER_2D_EXT                                         0x8DCA
#define GL_INT_SAMPLER_2D_MULTISAMPLE                                 0x9109
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY                           0x910C
#define GL_INT_SAMPLER_2D_RECT                                        0x8DCD
#define GL_INT_SAMPLER_2D_RECT_EXT                                    0x8DCD
#define GL_INT_SAMPLER_3D                                             0x8DCB
#define GL_INT_SAMPLER_3D_EXT                                         0x8DCB
#define GL_INT_SAMPLER_BUFFER                                         0x8DD0
#define GL_INT_SAMPLER_BUFFER_AMD                                     0x9002
#define GL_INT_SAMPLER_BUFFER_EXT                                     0x8DD0
#define GL_INT_SAMPLER_CUBE                                           0x8DCC
#define GL_INT_SAMPLER_CUBE_EXT                                       0x8DCC
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY                                 0x900E
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_ARB                             0x900E
#define GL_INT_SAMPLER_RENDERBUFFER_NV                                0x8E57
#define GL_INT_VEC2                                                   0x8B53
#define GL_INT_VEC2_ARB                                               0x8B53
#define GL_INT_VEC3                                                   0x8B54
#define GL_INT_VEC3_ARB                                               0x8B54
#define GL_INT_VEC4                                                   0x8B55
#define GL_INT_VEC4_ARB                                               0x8B55
#define GL_INVALID_ENUM                                               0x0500
#define GL_INVALID_FRAMEBUFFER_OPERATION                              0x0506
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT                          0x0506
#define GL_INVALID_INDEX                                              0xFFFFFFFF
#define GL_INVALID_OPERATION                                          0x0502
#define GL_INVALID_VALUE                                              0x0501
#define GL_INVARIANT_DATATYPE_EXT                                     0x87EB
#define GL_INVARIANT_EXT                                              0x87C2
#define GL_INVARIANT_VALUE_EXT                                        0x87EA
#define GL_INVERSE_NV                                                 0x862B
#define GL_INVERSE_TRANSPOSE_NV                                       0x862D
#define GL_INVERT                                                     0x150A
#define GL_INVERTED_SCREEN_W_REND                                     0x8491
#define GL_INVERT_OVG_NV                                              0x92B4
#define GL_INVERT_RGB_NV                                              0x92A3
#define GL_IR_INSTRUMENT1_SGIX                                        0x817F
#define GL_ISOLINES                                                   0x8E7A
#define GL_IS_PER_PATCH                                               0x92E7
#define GL_IS_ROW_MAJOR                                               0x9300
#define GL_ITALIC_BIT_NV                                              0x02
#define GL_IUI_N3F_V2F_EXT                                            0x81AF
#define GL_IUI_N3F_V3F_EXT                                            0x81B0
#define GL_IUI_V2F_EXT                                                0x81AD
#define GL_IUI_V3F_EXT                                                0x81AE
#define GL_KEEP                                                       0x1E00
#define GL_LARGE_CCW_ARC_TO_NV                                        0x16
#define GL_LARGE_CW_ARC_TO_NV                                         0x18
#define GL_LAST_VERTEX_CONVENTION                                     0x8E4E
#define GL_LAST_VERTEX_CONVENTION_EXT                                 0x8E4E
#define GL_LAST_VIDEO_CAPTURE_STATUS_NV                               0x9027
#define GL_LAYER_NV                                                   0x8DAA
#define GL_LAYER_PROVOKING_VERTEX                                     0x825E
#define GL_LAYOUT_COLOR_ATTACHMENT_EXT                                0x958E
#define GL_LAYOUT_DEFAULT_INTEL                                       0
#define GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT              0x9531
#define GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT              0x9530
#define GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT                        0x958F
#define GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT                         0x9590
#define GL_LAYOUT_GENERAL_EXT                                         0x958D
#define GL_LAYOUT_LINEAR_CPU_CACHED_INTEL                             2
#define GL_LAYOUT_LINEAR_INTEL                                        1
#define GL_LAYOUT_SHADER_READ_ONLY_EXT                                0x9591
#define GL_LAYOUT_TRANSFER_DST_EXT                                    0x9593
#define GL_LAYOUT_TRANSFER_SRC_EXT                                    0x9592
#define GL_LEFT                                                       0x0406
#define GL_LEQUAL                                                     0x0203
#define GL_LERP_ATI                                                   0x8969
#define GL_LESS                                                       0x0201
#define GL_LGPU_SEPARATE_STORAGE_BIT_NVX                              0x0800
#define GL_LIGHT0                                                     0x4000
#define GL_LIGHT1                                                     0x4001
#define GL_LIGHT2                                                     0x4002
#define GL_LIGHT3                                                     0x4003
#define GL_LIGHT4                                                     0x4004
#define GL_LIGHT5                                                     0x4005
#define GL_LIGHT6                                                     0x4006
#define GL_LIGHT7                                                     0x4007
#define GL_LIGHTEN_KHR                                                0x9298
#define GL_LIGHTEN_NV                                                 0x9298
#define GL_LIGHTING                                                   0x0B50
#define GL_LIGHTING_BIT                                               0x00000040
#define GL_LIGHT_ENV_MODE_SGIX                                        0x8407
#define GL_LIGHT_MODEL_AMBIENT                                        0x0B53
#define GL_LIGHT_MODEL_COLOR_CONTROL                                  0x81F8
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT                              0x81F8
#define GL_LIGHT_MODEL_LOCAL_VIEWER                                   0x0B51
#define GL_LIGHT_MODEL_SPECULAR_VECTOR_APPLE                          0x85B0
#define GL_LIGHT_MODEL_TWO_SIDE                                       0x0B52
#define GL_LINE                                                       0x1B01
#define GL_LINEAR                                                     0x2601
#define GL_LINEARBURN_NV                                              0x92A5
#define GL_LINEARDODGE_NV                                             0x92A4
#define GL_LINEARLIGHT_NV                                             0x92A7
#define GL_LINEAR_ATTENUATION                                         0x1208
#define GL_LINEAR_CLIPMAP_LINEAR_SGIX                                 0x8170
#define GL_LINEAR_CLIPMAP_NEAREST_SGIX                                0x844F
#define GL_LINEAR_DETAIL_ALPHA_SGIS                                   0x8098
#define GL_LINEAR_DETAIL_COLOR_SGIS                                   0x8099
#define GL_LINEAR_DETAIL_SGIS                                         0x8097
#define GL_LINEAR_MIPMAP_LINEAR                                       0x2703
#define GL_LINEAR_MIPMAP_NEAREST                                      0x2701
#define GL_LINEAR_SHARPEN_ALPHA_SGIS                                  0x80AE
#define GL_LINEAR_SHARPEN_COLOR_SGIS                                  0x80AF
#define GL_LINEAR_SHARPEN_SGIS                                        0x80AD
#define GL_LINEAR_TILING_EXT                                          0x9585
#define GL_LINES                                                      0x0001
#define GL_LINES_ADJACENCY                                            0x000A
#define GL_LINES_ADJACENCY_ARB                                        0x000A
#define GL_LINES_ADJACENCY_EXT                                        0x000A
#define GL_LINE_BIT                                                   0x00000004
#define GL_LINE_LOOP                                                  0x0002
#define GL_LINE_RESET_TOKEN                                           0x0707
#define GL_LINE_SMOOTH                                                0x0B20
#define GL_LINE_SMOOTH_HINT                                           0x0C52
#define GL_LINE_STIPPLE                                               0x0B24
#define GL_LINE_STIPPLE_PATTERN                                       0x0B25
#define GL_LINE_STIPPLE_REPEAT                                        0x0B26
#define GL_LINE_STRIP                                                 0x0003
#define GL_LINE_STRIP_ADJACENCY                                       0x000B
#define GL_LINE_STRIP_ADJACENCY_ARB                                   0x000B
#define GL_LINE_STRIP_ADJACENCY_EXT                                   0x000B
#define GL_LINE_TOKEN                                                 0x0702
#define GL_LINE_TO_NV                                                 0x04
#define GL_LINE_WIDTH                                                 0x0B21
#define GL_LINE_WIDTH_COMMAND_NV                                      0x000D
#define GL_LINE_WIDTH_GRANULARITY                                     0x0B23
#define GL_LINE_WIDTH_RANGE                                           0x0B22
#define GL_LINK_STATUS                                                0x8B82
#define GL_LIST_BASE                                                  0x0B32
#define GL_LIST_BIT                                                   0x00020000
#define GL_LIST_INDEX                                                 0x0B33
#define GL_LIST_MODE                                                  0x0B30
#define GL_LIST_PRIORITY_SGIX                                         0x8182
#define GL_LOAD                                                       0x0101
#define GL_LOCAL_CONSTANT_DATATYPE_EXT                                0x87ED
#define GL_LOCAL_CONSTANT_EXT                                         0x87C3
#define GL_LOCAL_CONSTANT_VALUE_EXT                                   0x87EC
#define GL_LOCAL_EXT                                                  0x87C4
#define GL_LOCATION                                                   0x930E
#define GL_LOCATION_COMPONENT                                         0x934A
#define GL_LOCATION_INDEX                                             0x930F
#define GL_LOGIC_OP                                                   0x0BF1
#define GL_LOGIC_OP_MODE                                              0x0BF0
#define GL_LOSE_CONTEXT_ON_RESET                                      0x8252
#define GL_LOSE_CONTEXT_ON_RESET_ARB                                  0x8252
#define GL_LOSE_CONTEXT_ON_RESET_KHR                                  0x8252
#define GL_LOWER_LEFT                                                 0x8CA1
#define GL_LOW_FLOAT                                                  0x8DF0
#define GL_LOW_INT                                                    0x8DF3
#define GL_LO_BIAS_NV                                                 0x8715
#define GL_LO_SCALE_NV                                                0x870F
#define GL_LUID_SIZE_EXT                                              8
#define GL_LUMINANCE                                                  0x1909
#define GL_LUMINANCE12                                                0x8041
#define GL_LUMINANCE12_ALPHA12                                        0x8047
#define GL_LUMINANCE12_ALPHA12_EXT                                    0x8047
#define GL_LUMINANCE12_ALPHA4                                         0x8046
#define GL_LUMINANCE12_ALPHA4_EXT                                     0x8046
#define GL_LUMINANCE12_EXT                                            0x8041
#define GL_LUMINANCE16                                                0x8042
#define GL_LUMINANCE16F_ARB                                           0x881E
#define GL_LUMINANCE16I_EXT                                           0x8D8C
#define GL_LUMINANCE16UI_EXT                                          0x8D7A
#define GL_LUMINANCE16_ALPHA16                                        0x8048
#define GL_LUMINANCE16_ALPHA16_EXT                                    0x8048
#define GL_LUMINANCE16_ALPHA16_SNORM                                  0x901A
#define GL_LUMINANCE16_EXT                                            0x8042
#define GL_LUMINANCE16_SNORM                                          0x9019
#define GL_LUMINANCE32F_ARB                                           0x8818
#define GL_LUMINANCE32I_EXT                                           0x8D86
#define GL_LUMINANCE32UI_EXT                                          0x8D74
#define GL_LUMINANCE4                                                 0x803F
#define GL_LUMINANCE4_ALPHA4                                          0x8043
#define GL_LUMINANCE4_ALPHA4_EXT                                      0x8043
#define GL_LUMINANCE4_EXT                                             0x803F
#define GL_LUMINANCE6_ALPHA2                                          0x8044
#define GL_LUMINANCE6_ALPHA2_EXT                                      0x8044
#define GL_LUMINANCE8                                                 0x8040
#define GL_LUMINANCE8I_EXT                                            0x8D92
#define GL_LUMINANCE8UI_EXT                                           0x8D80
#define GL_LUMINANCE8_ALPHA8                                          0x8045
#define GL_LUMINANCE8_ALPHA8_EXT                                      0x8045
#define GL_LUMINANCE8_ALPHA8_SNORM                                    0x9016
#define GL_LUMINANCE8_EXT                                             0x8040
#define GL_LUMINANCE8_SNORM                                           0x9015
#define GL_LUMINANCE_ALPHA                                            0x190A
#define GL_LUMINANCE_ALPHA16F_ARB                                     0x881F
#define GL_LUMINANCE_ALPHA16I_EXT                                     0x8D8D
#define GL_LUMINANCE_ALPHA16UI_EXT                                    0x8D7B
#define GL_LUMINANCE_ALPHA32F_ARB                                     0x8819
#define GL_LUMINANCE_ALPHA32I_EXT                                     0x8D87
#define GL_LUMINANCE_ALPHA32UI_EXT                                    0x8D75
#define GL_LUMINANCE_ALPHA8I_EXT                                      0x8D93
#define GL_LUMINANCE_ALPHA8UI_EXT                                     0x8D81
#define GL_LUMINANCE_ALPHA_FLOAT16_APPLE                              0x881F
#define GL_LUMINANCE_ALPHA_FLOAT16_ATI                                0x881F
#define GL_LUMINANCE_ALPHA_FLOAT32_APPLE                              0x8819
#define GL_LUMINANCE_ALPHA_FLOAT32_ATI                                0x8819
#define GL_LUMINANCE_ALPHA_INTEGER_EXT                                0x8D9D
#define GL_LUMINANCE_ALPHA_SNORM                                      0x9012
#define GL_LUMINANCE_FLOAT16_APPLE                                    0x881E
#define GL_LUMINANCE_FLOAT16_ATI                                      0x881E
#define GL_LUMINANCE_FLOAT32_APPLE                                    0x8818
#define GL_LUMINANCE_FLOAT32_ATI                                      0x8818
#define GL_LUMINANCE_INTEGER_EXT                                      0x8D9C
#define GL_LUMINANCE_SNORM                                            0x9011
#define GL_MAD_ATI                                                    0x8968
#define GL_MAGNITUDE_BIAS_NV                                          0x8718
#define GL_MAGNITUDE_SCALE_NV                                         0x8712
#define GL_MAJOR_VERSION                                              0x821B
#define GL_MANUAL_GENERATE_MIPMAP                                     0x8294
#define GL_MAP1_BINORMAL_EXT                                          0x8446
#define GL_MAP1_COLOR_4                                               0x0D90
#define GL_MAP1_GRID_DOMAIN                                           0x0DD0
#define GL_MAP1_GRID_SEGMENTS                                         0x0DD1
#define GL_MAP1_INDEX                                                 0x0D91
#define GL_MAP1_NORMAL                                                0x0D92
#define GL_MAP1_TANGENT_EXT                                           0x8444
#define GL_MAP1_TEXTURE_COORD_1                                       0x0D93
#define GL_MAP1_TEXTURE_COORD_2                                       0x0D94
#define GL_MAP1_TEXTURE_COORD_3                                       0x0D95
#define GL_MAP1_TEXTURE_COORD_4                                       0x0D96
#define GL_MAP1_VERTEX_3                                              0x0D97
#define GL_MAP1_VERTEX_4                                              0x0D98
#define GL_MAP1_VERTEX_ATTRIB0_4_NV                                   0x8660
#define GL_MAP1_VERTEX_ATTRIB10_4_NV                                  0x866A
#define GL_MAP1_VERTEX_ATTRIB11_4_NV                                  0x866B
#define GL_MAP1_VERTEX_ATTRIB12_4_NV                                  0x866C
#define GL_MAP1_VERTEX_ATTRIB13_4_NV                                  0x866D
#define GL_MAP1_VERTEX_ATTRIB14_4_NV                                  0x866E
#define GL_MAP1_VERTEX_ATTRIB15_4_NV                                  0x866F
#define GL_MAP1_VERTEX_ATTRIB1_4_NV                                   0x8661
#define GL_MAP1_VERTEX_ATTRIB2_4_NV                                   0x8662
#define GL_MAP1_VERTEX_ATTRIB3_4_NV                                   0x8663
#define GL_MAP1_VERTEX_ATTRIB4_4_NV                                   0x8664
#define GL_MAP1_VERTEX_ATTRIB5_4_NV                                   0x8665
#define GL_MAP1_VERTEX_ATTRIB6_4_NV                                   0x8666
#define GL_MAP1_VERTEX_ATTRIB7_4_NV                                   0x8667
#define GL_MAP1_VERTEX_ATTRIB8_4_NV                                   0x8668
#define GL_MAP1_VERTEX_ATTRIB9_4_NV                                   0x8669
#define GL_MAP2_BINORMAL_EXT                                          0x8447
#define GL_MAP2_COLOR_4                                               0x0DB0
#define GL_MAP2_GRID_DOMAIN                                           0x0DD2
#define GL_MAP2_GRID_SEGMENTS                                         0x0DD3
#define GL_MAP2_INDEX                                                 0x0DB1
#define GL_MAP2_NORMAL                                                0x0DB2
#define GL_MAP2_TANGENT_EXT                                           0x8445
#define GL_MAP2_TEXTURE_COORD_1                                       0x0DB3
#define GL_MAP2_TEXTURE_COORD_2                                       0x0DB4
#define GL_MAP2_TEXTURE_COORD_3                                       0x0DB5
#define GL_MAP2_TEXTURE_COORD_4                                       0x0DB6
#define GL_MAP2_VERTEX_3                                              0x0DB7
#define GL_MAP2_VERTEX_4                                              0x0DB8
#define GL_MAP2_VERTEX_ATTRIB0_4_NV                                   0x8670
#define GL_MAP2_VERTEX_ATTRIB10_4_NV                                  0x867A
#define GL_MAP2_VERTEX_ATTRIB11_4_NV                                  0x867B
#define GL_MAP2_VERTEX_ATTRIB12_4_NV                                  0x867C
#define GL_MAP2_VERTEX_ATTRIB13_4_NV                                  0x867D
#define GL_MAP2_VERTEX_ATTRIB14_4_NV                                  0x867E
#define GL_MAP2_VERTEX_ATTRIB15_4_NV                                  0x867F
#define GL_MAP2_VERTEX_ATTRIB1_4_NV                                   0x8671
#define GL_MAP2_VERTEX_ATTRIB2_4_NV                                   0x8672
#define GL_MAP2_VERTEX_ATTRIB3_4_NV                                   0x8673
#define GL_MAP2_VERTEX_ATTRIB4_4_NV                                   0x8674
#define GL_MAP2_VERTEX_ATTRIB5_4_NV                                   0x8675
#define GL_MAP2_VERTEX_ATTRIB6_4_NV                                   0x8676
#define GL_MAP2_VERTEX_ATTRIB7_4_NV                                   0x8677
#define GL_MAP2_VERTEX_ATTRIB8_4_NV                                   0x8678
#define GL_MAP2_VERTEX_ATTRIB9_4_NV                                   0x8679
#define GL_MAP_ATTRIB_U_ORDER_NV                                      0x86C3
#define GL_MAP_ATTRIB_V_ORDER_NV                                      0x86C4
#define GL_MAP_COHERENT_BIT                                           0x0080
#define GL_MAP_COLOR                                                  0x0D10
#define GL_MAP_FLUSH_EXPLICIT_BIT                                     0x0010
#define GL_MAP_INVALIDATE_BUFFER_BIT                                  0x0008
#define GL_MAP_INVALIDATE_RANGE_BIT                                   0x0004
#define GL_MAP_PERSISTENT_BIT                                         0x0040
#define GL_MAP_READ_BIT                                               0x0001
#define GL_MAP_STENCIL                                                0x0D11
#define GL_MAP_TESSELLATION_NV                                        0x86C2
#define GL_MAP_UNSYNCHRONIZED_BIT                                     0x0020
#define GL_MAP_WRITE_BIT                                              0x0002
#define GL_MATERIAL_SIDE_HINT_PGI                                     0x1A22C
#define GL_MATRIX0_ARB                                                0x88C0
#define GL_MATRIX0_NV                                                 0x8630
#define GL_MATRIX10_ARB                                               0x88CA
#define GL_MATRIX11_ARB                                               0x88CB
#define GL_MATRIX12_ARB                                               0x88CC
#define GL_MATRIX13_ARB                                               0x88CD
#define GL_MATRIX14_ARB                                               0x88CE
#define GL_MATRIX15_ARB                                               0x88CF
#define GL_MATRIX16_ARB                                               0x88D0
#define GL_MATRIX17_ARB                                               0x88D1
#define GL_MATRIX18_ARB                                               0x88D2
#define GL_MATRIX19_ARB                                               0x88D3
#define GL_MATRIX1_ARB                                                0x88C1
#define GL_MATRIX1_NV                                                 0x8631
#define GL_MATRIX20_ARB                                               0x88D4
#define GL_MATRIX21_ARB                                               0x88D5
#define GL_MATRIX22_ARB                                               0x88D6
#define GL_MATRIX23_ARB                                               0x88D7
#define GL_MATRIX24_ARB                                               0x88D8
#define GL_MATRIX25_ARB                                               0x88D9
#define GL_MATRIX26_ARB                                               0x88DA
#define GL_MATRIX27_ARB                                               0x88DB
#define GL_MATRIX28_ARB                                               0x88DC
#define GL_MATRIX29_ARB                                               0x88DD
#define GL_MATRIX2_ARB                                                0x88C2
#define GL_MATRIX2_NV                                                 0x8632
#define GL_MATRIX30_ARB                                               0x88DE
#define GL_MATRIX31_ARB                                               0x88DF
#define GL_MATRIX3_ARB                                                0x88C3
#define GL_MATRIX3_NV                                                 0x8633
#define GL_MATRIX4_ARB                                                0x88C4
#define GL_MATRIX4_NV                                                 0x8634
#define GL_MATRIX5_ARB                                                0x88C5
#define GL_MATRIX5_NV                                                 0x8635
#define GL_MATRIX6_ARB                                                0x88C6
#define GL_MATRIX6_NV                                                 0x8636
#define GL_MATRIX7_ARB                                                0x88C7
#define GL_MATRIX7_NV                                                 0x8637
#define GL_MATRIX8_ARB                                                0x88C8
#define GL_MATRIX9_ARB                                                0x88C9
#define GL_MATRIX_EXT                                                 0x87C0
#define GL_MATRIX_INDEX_ARRAY_ARB                                     0x8844
#define GL_MATRIX_INDEX_ARRAY_POINTER_ARB                             0x8849
#define GL_MATRIX_INDEX_ARRAY_SIZE_ARB                                0x8846
#define GL_MATRIX_INDEX_ARRAY_STRIDE_ARB                              0x8848
#define GL_MATRIX_INDEX_ARRAY_TYPE_ARB                                0x8847
#define GL_MATRIX_MODE                                                0x0BA0
#define GL_MATRIX_PALETTE_ARB                                         0x8840
#define GL_MATRIX_STRIDE                                              0x92FF
#define GL_MAT_AMBIENT_AND_DIFFUSE_BIT_PGI                            0x00200000
#define GL_MAT_AMBIENT_BIT_PGI                                        0x00100000
#define GL_MAT_COLOR_INDEXES_BIT_PGI                                  0x01000000
#define GL_MAT_DIFFUSE_BIT_PGI                                        0x00400000
#define GL_MAT_EMISSION_BIT_PGI                                       0x00800000
#define GL_MAT_SHININESS_BIT_PGI                                      0x02000000
#define GL_MAT_SPECULAR_BIT_PGI                                       0x04000000
#define GL_MAX                                                        0x8008
#define GL_MAX_3D_TEXTURE_SIZE                                        0x8073
#define GL_MAX_3D_TEXTURE_SIZE_EXT                                    0x8073
#define GL_MAX_4D_TEXTURE_SIZE_SGIS                                   0x8138
#define GL_MAX_ACTIVE_LIGHTS_SGIX                                     0x8405
#define GL_MAX_ARRAY_TEXTURE_LAYERS                                   0x88FF
#define GL_MAX_ARRAY_TEXTURE_LAYERS_EXT                               0x88FF
#define GL_MAX_ASYNC_DRAW_PIXELS_SGIX                                 0x8360
#define GL_MAX_ASYNC_HISTOGRAM_SGIX                                   0x832D
#define GL_MAX_ASYNC_READ_PIXELS_SGIX                                 0x8361
#define GL_MAX_ASYNC_TEX_IMAGE_SGIX                                   0x835F
#define GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS                         0x92DC
#define GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE                             0x92D8
#define GL_MAX_ATTRIB_STACK_DEPTH                                     0x0D35
#define GL_MAX_BINDABLE_UNIFORM_SIZE_EXT                              0x8DED
#define GL_MAX_CLIENT_ATTRIB_STACK_DEPTH                              0x0D3B
#define GL_MAX_CLIPMAP_DEPTH_SGIX                                     0x8177
#define GL_MAX_CLIPMAP_VIRTUAL_DEPTH_SGIX                             0x8178
#define GL_MAX_CLIP_DISTANCES                                         0x0D32
#define GL_MAX_CLIP_PLANES                                            0x0D32
#define GL_MAX_COARSE_FRAGMENT_SAMPLES_NV                             0x955F
#define GL_MAX_COLOR_ATTACHMENTS                                      0x8CDF
#define GL_MAX_COLOR_ATTACHMENTS_EXT                                  0x8CDF
#define GL_MAX_COLOR_FRAMEBUFFER_SAMPLES_AMD                          0x91B3
#define GL_MAX_COLOR_FRAMEBUFFER_STORAGE_SAMPLES_AMD                  0x91B4
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH                               0x80B3
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI                           0x80B3
#define GL_MAX_COLOR_TEXTURE_SAMPLES                                  0x910E
#define GL_MAX_COMBINED_ATOMIC_COUNTERS                               0x92D7
#define GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS                        0x92D1
#define GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES                       0x82FA
#define GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS                    0x8266
#define GL_MAX_COMBINED_DIMENSIONS                                    0x8282
#define GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS                   0x8A33
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS                   0x8A32
#define GL_MAX_COMBINED_IMAGE_UNIFORMS                                0x90CF
#define GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS              0x8F39
#define GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS_EXT          0x8F39
#define GL_MAX_COMBINED_MESH_UNIFORM_COMPONENTS_NV                    0x8E67
#define GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES                       0x8F39
#define GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS                         0x90DC
#define GL_MAX_COMBINED_TASK_UNIFORM_COMPONENTS_NV                    0x8E6F
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS               0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS            0x8E1F
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS                           0x8B4D
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB                       0x8B4D
#define GL_MAX_COMBINED_UNIFORM_BLOCKS                                0x8A2E
#define GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS                     0x8A31
#define GL_MAX_COMPUTE_ATOMIC_COUNTERS                                0x8265
#define GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS                         0x8264
#define GL_MAX_COMPUTE_FIXED_GROUP_INVOCATIONS_ARB                    0x90EB
#define GL_MAX_COMPUTE_FIXED_GROUP_SIZE_ARB                           0x91BF
#define GL_MAX_COMPUTE_IMAGE_UNIFORMS                                 0x91BD
#define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS                          0x90DB
#define GL_MAX_COMPUTE_SHARED_MEMORY_SIZE                             0x8262
#define GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS                            0x91BC
#define GL_MAX_COMPUTE_UNIFORM_BLOCKS                                 0x91BB
#define GL_MAX_COMPUTE_UNIFORM_COMPONENTS                             0x8263
#define GL_MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB                 0x9344
#define GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB                        0x9345
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT                               0x91BE
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS                         0x90EB
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE                                0x91BF
#define GL_MAX_CONVOLUTION_HEIGHT                                     0x801B
#define GL_MAX_CONVOLUTION_HEIGHT_EXT                                 0x801B
#define GL_MAX_CONVOLUTION_WIDTH                                      0x801A
#define GL_MAX_CONVOLUTION_WIDTH_EXT                                  0x801A
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE                                  0x851C
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB                              0x851C
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT                              0x851C
#define GL_MAX_CULL_DISTANCES                                         0x82F9
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH                                0x826C
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH_KHR                            0x826C
#define GL_MAX_DEBUG_LOGGED_MESSAGES                                  0x9144
#define GL_MAX_DEBUG_LOGGED_MESSAGES_AMD                              0x9144
#define GL_MAX_DEBUG_LOGGED_MESSAGES_ARB                              0x9144
#define GL_MAX_DEBUG_LOGGED_MESSAGES_KHR                              0x9144
#define GL_MAX_DEBUG_MESSAGE_LENGTH                                   0x9143
#define GL_MAX_DEBUG_MESSAGE_LENGTH_AMD                               0x9143
#define GL_MAX_DEBUG_MESSAGE_LENGTH_ARB                               0x9143
#define GL_MAX_DEBUG_MESSAGE_LENGTH_KHR                               0x9143
#define GL_MAX_DEEP_3D_TEXTURE_DEPTH_NV                               0x90D1
#define GL_MAX_DEEP_3D_TEXTURE_WIDTH_HEIGHT_NV                        0x90D0
#define GL_MAX_DEFORMATION_ORDER_SGIX                                 0x8197
#define GL_MAX_DEPTH                                                  0x8280
#define GL_MAX_DEPTH_STENCIL_FRAMEBUFFER_SAMPLES_AMD                  0x91B5
#define GL_MAX_DEPTH_TEXTURE_SAMPLES                                  0x910F
#define GL_MAX_DETACHED_BUFFERS_NV                                    0x95AD
#define GL_MAX_DETACHED_TEXTURES_NV                                   0x95AC
#define GL_MAX_DRAW_BUFFERS                                           0x8824
#define GL_MAX_DRAW_BUFFERS_ARB                                       0x8824
#define GL_MAX_DRAW_BUFFERS_ATI                                       0x8824
#define GL_MAX_DRAW_MESH_TASKS_COUNT_NV                               0x953D
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS                               0x88FC
#define GL_MAX_ELEMENTS_INDICES                                       0x80E9
#define GL_MAX_ELEMENTS_INDICES_EXT                                   0x80E9
#define GL_MAX_ELEMENTS_VERTICES                                      0x80E8
#define GL_MAX_ELEMENTS_VERTICES_EXT                                  0x80E8
#define GL_MAX_ELEMENT_INDEX                                          0x8D6B
#define GL_MAX_EVAL_ORDER                                             0x0D30
#define GL_MAX_EXT                                                    0x8008
#define GL_MAX_FOG_FUNC_POINTS_SGIS                                   0x812C
#define GL_MAX_FRAGMENT_ATOMIC_COUNTERS                               0x92D6
#define GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS                        0x92D0
#define GL_MAX_FRAGMENT_BINDABLE_UNIFORMS_EXT                         0x8DE3
#define GL_MAX_FRAGMENT_IMAGE_UNIFORMS                                0x90CE
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS                              0x9125
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET                          0x8E5C
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET_NV                       0x8E5C
#define GL_MAX_FRAGMENT_LIGHTS_SGIX                                   0x8404
#define GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV                   0x8868
#define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS                         0x90DA
#define GL_MAX_FRAGMENT_UNIFORM_BLOCKS                                0x8A2D
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS                            0x8B49
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB                        0x8B49
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS                               0x8DFD
#define GL_MAX_FRAMEBUFFER_HEIGHT                                     0x9316
#define GL_MAX_FRAMEBUFFER_LAYERS                                     0x9317
#define GL_MAX_FRAMEBUFFER_SAMPLES                                    0x9318
#define GL_MAX_FRAMEBUFFER_WIDTH                                      0x9315
#define GL_MAX_FRAMEZOOM_FACTOR_SGIX                                  0x818D
#define GL_MAX_GENERAL_COMBINERS_NV                                   0x854D
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS                               0x92D5
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS                        0x92CF
#define GL_MAX_GEOMETRY_BINDABLE_UNIFORMS_EXT                         0x8DE4
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS                                0x90CD
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS                              0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS                             0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES                               0x8DE0
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB                           0x8DE0
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT                           0x8DE0
#define GL_MAX_GEOMETRY_PROGRAM_INVOCATIONS_NV                        0x8E5A
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS                            0x8E5A
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS                         0x90D7
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS                           0x8C29
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_ARB                       0x8C29
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT                       0x8C29
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS                       0x8DE1
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB                   0x8DE1
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT                   0x8DE1
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS                                0x8A2C
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS                            0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_ARB                        0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT                        0x8DDF
#define GL_MAX_GEOMETRY_VARYING_COMPONENTS_ARB                        0x8DDD
#define GL_MAX_GEOMETRY_VARYING_COMPONENTS_EXT                        0x8DDD
#define GL_MAX_HEIGHT                                                 0x827F
#define GL_MAX_IMAGE_SAMPLES                                          0x906D
#define GL_MAX_IMAGE_SAMPLES_EXT                                      0x906D
#define GL_MAX_IMAGE_UNITS                                            0x8F38
#define GL_MAX_IMAGE_UNITS_EXT                                        0x8F38
#define GL_MAX_INTEGER_SAMPLES                                        0x9110
#define GL_MAX_LABEL_LENGTH                                           0x82E8
#define GL_MAX_LABEL_LENGTH_KHR                                       0x82E8
#define GL_MAX_LAYERS                                                 0x8281
#define GL_MAX_LGPU_GPUS_NVX                                          0x92BA
#define GL_MAX_LIGHTS                                                 0x0D31
#define GL_MAX_LIST_NESTING                                           0x0B31
#define GL_MAX_MAP_TESSELLATION_NV                                    0x86D6
#define GL_MAX_MATRIX_PALETTE_STACK_DEPTH_ARB                         0x8841
#define GL_MAX_MESH_ATOMIC_COUNTERS_NV                                0x8E65
#define GL_MAX_MESH_ATOMIC_COUNTER_BUFFERS_NV                         0x8E64
#define GL_MAX_MESH_IMAGE_UNIFORMS_NV                                 0x8E62
#define GL_MAX_MESH_OUTPUT_PRIMITIVES_NV                              0x9539
#define GL_MAX_MESH_OUTPUT_VERTICES_NV                                0x9538
#define GL_MAX_MESH_SHADER_STORAGE_BLOCKS_NV                          0x8E66
#define GL_MAX_MESH_TEXTURE_IMAGE_UNITS_NV                            0x8E61
#define GL_MAX_MESH_TOTAL_MEMORY_SIZE_NV                              0x9536
#define GL_MAX_MESH_UNIFORM_BLOCKS_NV                                 0x8E60
#define GL_MAX_MESH_UNIFORM_COMPONENTS_NV                             0x8E63
#define GL_MAX_MESH_VIEWS_NV                                          0x9557
#define GL_MAX_MESH_WORK_GROUP_INVOCATIONS_NV                         0x95A2
#define GL_MAX_MESH_WORK_GROUP_SIZE_NV                                0x953B
#define GL_MAX_MODELVIEW_STACK_DEPTH                                  0x0D36
#define GL_MAX_MULTISAMPLE_COVERAGE_MODES_NV                          0x8E11
#define GL_MAX_NAME_LENGTH                                            0x92F6
#define GL_MAX_NAME_STACK_DEPTH                                       0x0D37
#define GL_MAX_NUM_ACTIVE_VARIABLES                                   0x92F7
#define GL_MAX_NUM_COMPATIBLE_SUBROUTINES                             0x92F8
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_INSTRUCTIONS_EXT               0x87CA
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_INVARIANTS_EXT                 0x87CD
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCALS_EXT                     0x87CE
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCAL_CONSTANTS_EXT            0x87CC
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_VARIANTS_EXT                   0x87CB
#define GL_MAX_PALETTE_MATRICES_ARB                                   0x8842
#define GL_MAX_PATCH_VERTICES                                         0x8E7D
#define GL_MAX_PIXEL_MAP_TABLE                                        0x0D34
#define GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT                     0x8337
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI                     0x87F1
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB                          0x88B1
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB                           0x880B
#define GL_MAX_PROGRAM_ATTRIBS_ARB                                    0x88AD
#define GL_MAX_PROGRAM_ATTRIB_COMPONENTS_NV                           0x8908
#define GL_MAX_PROGRAM_CALL_DEPTH_NV                                  0x88F5
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB                             0x88B5
#define GL_MAX_PROGRAM_EXEC_INSTRUCTIONS_NV                           0x88F4
#define GL_MAX_PROGRAM_GENERIC_ATTRIBS_NV                             0x8DA5
#define GL_MAX_PROGRAM_GENERIC_RESULTS_NV                             0x8DA6
#define GL_MAX_PROGRAM_IF_DEPTH_NV                                    0x88F6
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB                               0x88A1
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB                           0x88B4
#define GL_MAX_PROGRAM_LOOP_COUNT_NV                                  0x88F8
#define GL_MAX_PROGRAM_LOOP_DEPTH_NV                                  0x88F7
#define GL_MAX_PROGRAM_MATRICES_ARB                                   0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB                         0x862E
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB                   0x88B3
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB                    0x880E
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB                             0x88AF
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB                        0x88A3
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB                          0x88AB
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB                         0x88A7
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB                    0x8810
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB                    0x880F
#define GL_MAX_PROGRAM_OUTPUT_VERTICES_NV                             0x8C27
#define GL_MAX_PROGRAM_PARAMETERS_ARB                                 0x88A9
#define GL_MAX_PROGRAM_PARAMETER_BUFFER_BINDINGS_NV                   0x8DA0
#define GL_MAX_PROGRAM_PARAMETER_BUFFER_SIZE_NV                       0x8DA1
#define GL_MAX_PROGRAM_PATCH_ATTRIBS_NV                               0x86D8
#define GL_MAX_PROGRAM_RESULT_COMPONENTS_NV                           0x8909
#define GL_MAX_PROGRAM_SUBROUTINE_NUM_NV                              0x8F45
#define GL_MAX_PROGRAM_SUBROUTINE_PARAMETERS_NV                       0x8F44
#define GL_MAX_PROGRAM_TEMPORARIES_ARB                                0x88A5
#define GL_MAX_PROGRAM_TEXEL_OFFSET                                   0x8905
#define GL_MAX_PROGRAM_TEXEL_OFFSET_EXT                               0x8905
#define GL_MAX_PROGRAM_TEXEL_OFFSET_NV                                0x8905
#define GL_MAX_PROGRAM_TEXTURE_GATHER_COMPONENTS_ARB                  0x8F9F
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET                          0x8E5F
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET_ARB                      0x8E5F
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET_NV                       0x8E5F
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB                           0x880D
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB                           0x880C
#define GL_MAX_PROGRAM_TOTAL_OUTPUT_COMPONENTS_NV                     0x8C28
#define GL_MAX_PROJECTION_STACK_DEPTH                                 0x0D38
#define GL_MAX_RASTER_SAMPLES_EXT                                     0x9329
#define GL_MAX_RATIONAL_EVAL_ORDER_NV                                 0x86D7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE                                 0x84F8
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB                             0x84F8
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV                              0x84F8
#define GL_MAX_RENDERBUFFER_SIZE                                      0x84E8
#define GL_MAX_RENDERBUFFER_SIZE_EXT                                  0x84E8
#define GL_MAX_SAMPLES                                                0x8D57
#define GL_MAX_SAMPLES_EXT                                            0x8D57
#define GL_MAX_SAMPLE_MASK_WORDS                                      0x8E59
#define GL_MAX_SAMPLE_MASK_WORDS_NV                                   0x8E59
#define GL_MAX_SERVER_WAIT_TIMEOUT                                    0x9111
#define GL_MAX_SHADER_BUFFER_ADDRESS_NV                               0x8F35
#define GL_MAX_SHADER_COMPILER_THREADS_ARB                            0x91B0
#define GL_MAX_SHADER_COMPILER_THREADS_KHR                            0x91B0
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE                              0x90DE
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS                         0x90DD
#define GL_MAX_SHININESS_NV                                           0x8504
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_AMD                             0x9199
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB                             0x9199
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS                            0x919A
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB                        0x919A
#define GL_MAX_SPARSE_TEXTURE_SIZE_AMD                                0x9198
#define GL_MAX_SPARSE_TEXTURE_SIZE_ARB                                0x9198
#define GL_MAX_SPOT_EXPONENT_NV                                       0x8505
#define GL_MAX_SUBPIXEL_PRECISION_BIAS_BITS_NV                        0x9349
#define GL_MAX_SUBROUTINES                                            0x8DE7
#define GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS                           0x8DE8
#define GL_MAX_TASK_ATOMIC_COUNTERS_NV                                0x8E6D
#define GL_MAX_TASK_ATOMIC_COUNTER_BUFFERS_NV                         0x8E6C
#define GL_MAX_TASK_IMAGE_UNIFORMS_NV                                 0x8E6A
#define GL_MAX_TASK_OUTPUT_COUNT_NV                                   0x953A
#define GL_MAX_TASK_SHADER_STORAGE_BLOCKS_NV                          0x8E6E
#define GL_MAX_TASK_TEXTURE_IMAGE_UNITS_NV                            0x8E69
#define GL_MAX_TASK_TOTAL_MEMORY_SIZE_NV                              0x9537
#define GL_MAX_TASK_UNIFORM_BLOCKS_NV                                 0x8E68
#define GL_MAX_TASK_UNIFORM_COMPONENTS_NV                             0x8E6B
#define GL_MAX_TASK_WORK_GROUP_INVOCATIONS_NV                         0x95A3
#define GL_MAX_TASK_WORK_GROUP_SIZE_NV                                0x953C
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS                           0x92D3
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS                    0x92CD
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS                            0x90CB
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS                          0x886C
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS                         0x8E83
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS                     0x90D8
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS                       0x8E81
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS                   0x8E85
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS                            0x8E89
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS                        0x8E7F
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS                        0x92D4
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS                 0x92CE
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS                         0x90CC
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS                       0x886D
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS                      0x8E86
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS                  0x90D9
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS                    0x8E82
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS                         0x8E8A
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS                     0x8E80
#define GL_MAX_TESS_GEN_LEVEL                                         0x8E7E
#define GL_MAX_TESS_PATCH_COMPONENTS                                  0x8E84
#define GL_MAX_TEXTURE_BUFFER_SIZE                                    0x8C2B
#define GL_MAX_TEXTURE_BUFFER_SIZE_ARB                                0x8C2B
#define GL_MAX_TEXTURE_BUFFER_SIZE_EXT                                0x8C2B
#define GL_MAX_TEXTURE_COORDS                                         0x8871
#define GL_MAX_TEXTURE_COORDS_ARB                                     0x8871
#define GL_MAX_TEXTURE_COORDS_NV                                      0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS                                    0x8872
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB                                0x8872
#define GL_MAX_TEXTURE_IMAGE_UNITS_NV                                 0x8872
#define GL_MAX_TEXTURE_LOD_BIAS                                       0x84FD
#define GL_MAX_TEXTURE_LOD_BIAS_EXT                                   0x84FD
#define GL_MAX_TEXTURE_MAX_ANISOTROPY                                 0x84FF
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT                             0x84FF
#define GL_MAX_TEXTURE_SIZE                                           0x0D33
#define GL_MAX_TEXTURE_STACK_DEPTH                                    0x0D39
#define GL_MAX_TEXTURE_UNITS                                          0x84E2
#define GL_MAX_TEXTURE_UNITS_ARB                                      0x84E2
#define GL_MAX_TRACK_MATRICES_NV                                      0x862F
#define GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV                            0x862E
#define GL_MAX_TRANSFORM_FEEDBACK_BUFFERS                             0x8E70
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS              0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT          0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_NV           0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS                    0x8C8B
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT                0x8C8B
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_NV                 0x8C8B
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS                 0x8C80
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT             0x8C80
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_NV              0x8C80
#define GL_MAX_UNIFORM_BLOCK_SIZE                                     0x8A30
#define GL_MAX_UNIFORM_BUFFER_BINDINGS                                0x8A2F
#define GL_MAX_UNIFORM_LOCATIONS                                      0x826E
#define GL_MAX_VARYING_COMPONENTS                                     0x8B4B
#define GL_MAX_VARYING_COMPONENTS_EXT                                 0x8B4B
#define GL_MAX_VARYING_FLOATS                                         0x8B4B
#define GL_MAX_VARYING_FLOATS_ARB                                     0x8B4B
#define GL_MAX_VARYING_VECTORS                                        0x8DFC
#define GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV                          0x8520
#define GL_MAX_VERTEX_ATOMIC_COUNTERS                                 0x92D2
#define GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS                          0x92CC
#define GL_MAX_VERTEX_ATTRIBS                                         0x8869
#define GL_MAX_VERTEX_ATTRIBS_ARB                                     0x8869
#define GL_MAX_VERTEX_ATTRIB_BINDINGS                                 0x82DA
#define GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET                          0x82D9
#define GL_MAX_VERTEX_ATTRIB_STRIDE                                   0x82E5
#define GL_MAX_VERTEX_BINDABLE_UNIFORMS_EXT                           0x8DE2
#define GL_MAX_VERTEX_HINT_PGI                                        0x1A22D
#define GL_MAX_VERTEX_IMAGE_UNIFORMS                                  0x90CA
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS                               0x9122
#define GL_MAX_VERTEX_SHADER_INSTRUCTIONS_EXT                         0x87C5
#define GL_MAX_VERTEX_SHADER_INVARIANTS_EXT                           0x87C7
#define GL_MAX_VERTEX_SHADER_LOCALS_EXT                               0x87C9
#define GL_MAX_VERTEX_SHADER_LOCAL_CONSTANTS_EXT                      0x87C8
#define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS                           0x90D6
#define GL_MAX_VERTEX_SHADER_VARIANTS_EXT                             0x87C6
#define GL_MAX_VERTEX_STREAMS                                         0x8E71
#define GL_MAX_VERTEX_STREAMS_ATI                                     0x876B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS                             0x8B4C
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB                         0x8B4C
#define GL_MAX_VERTEX_UNIFORM_BLOCKS                                  0x8A2B
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS                              0x8B4A
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB                          0x8B4A
#define GL_MAX_VERTEX_UNIFORM_VECTORS                                 0x8DFB
#define GL_MAX_VERTEX_UNITS_ARB                                       0x86A4
#define GL_MAX_VERTEX_VARYING_COMPONENTS_ARB                          0x8DDE
#define GL_MAX_VERTEX_VARYING_COMPONENTS_EXT                          0x8DDE
#define GL_MAX_VIEWPORTS                                              0x825B
#define GL_MAX_VIEWPORT_DIMS                                          0x0D3A
#define GL_MAX_VIEWS_OVR                                              0x9631
#define GL_MAX_WIDTH                                                  0x827E
#define GL_MAX_WINDOW_RECTANGLES_EXT                                  0x8F14
#define GL_MEDIUM_FLOAT                                               0x8DF1
#define GL_MEDIUM_INT                                                 0x8DF4
#define GL_MEMORY_ATTACHABLE_ALIGNMENT_NV                             0x95A6
#define GL_MEMORY_ATTACHABLE_NV                                       0x95A8
#define GL_MEMORY_ATTACHABLE_SIZE_NV                                  0x95A7
#define GL_MESH_OUTPUT_PER_PRIMITIVE_GRANULARITY_NV                   0x9543
#define GL_MESH_OUTPUT_PER_VERTEX_GRANULARITY_NV                      0x92DF
#define GL_MESH_OUTPUT_TYPE_NV                                        0x957B
#define GL_MESH_PRIMITIVES_OUT_NV                                     0x957A
#define GL_MESH_SHADER_BIT_NV                                         0x00000040
#define GL_MESH_SHADER_NV                                             0x9559
#define GL_MESH_SUBROUTINE_NV                                         0x957C
#define GL_MESH_SUBROUTINE_UNIFORM_NV                                 0x957E
#define GL_MESH_VERTICES_OUT_NV                                       0x9579
#define GL_MESH_WORK_GROUP_SIZE_NV                                    0x953E
#define GL_MIN                                                        0x8007
#define GL_MINMAX                                                     0x802E
#define GL_MINMAX_EXT                                                 0x802E
#define GL_MINMAX_FORMAT                                              0x802F
#define GL_MINMAX_FORMAT_EXT                                          0x802F
#define GL_MINMAX_SINK                                                0x8030
#define GL_MINMAX_SINK_EXT                                            0x8030
#define GL_MINOR_VERSION                                              0x821C
#define GL_MINUS_CLAMPED_NV                                           0x92B3
#define GL_MINUS_NV                                                   0x929F
#define GL_MIN_EXT                                                    0x8007
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET                          0x8E5B
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET_NV                       0x8E5B
#define GL_MIN_LOD_WARNING_AMD                                        0x919C
#define GL_MIN_MAP_BUFFER_ALIGNMENT                                   0x90BC
#define GL_MIN_PROGRAM_TEXEL_OFFSET                                   0x8904
#define GL_MIN_PROGRAM_TEXEL_OFFSET_EXT                               0x8904
#define GL_MIN_PROGRAM_TEXEL_OFFSET_NV                                0x8904
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET                          0x8E5E
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET_ARB                      0x8E5E
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET_NV                       0x8E5E
#define GL_MIN_SAMPLE_SHADING_VALUE                                   0x8C37
#define GL_MIN_SAMPLE_SHADING_VALUE_ARB                               0x8C37
#define GL_MIN_SPARSE_LEVEL_AMD                                       0x919B
#define GL_MIPMAP                                                     0x8293
#define GL_MIRRORED_REPEAT                                            0x8370
#define GL_MIRRORED_REPEAT_ARB                                        0x8370
#define GL_MIRRORED_REPEAT_IBM                                        0x8370
#define GL_MIRROR_CLAMP_ATI                                           0x8742
#define GL_MIRROR_CLAMP_EXT                                           0x8742
#define GL_MIRROR_CLAMP_TO_BORDER_EXT                                 0x8912
#define GL_MIRROR_CLAMP_TO_EDGE                                       0x8743
#define GL_MIRROR_CLAMP_TO_EDGE_ATI                                   0x8743
#define GL_MIRROR_CLAMP_TO_EDGE_EXT                                   0x8743
#define GL_MITER_REVERT_NV                                            0x90A7
#define GL_MITER_TRUNCATE_NV                                          0x90A8
#define GL_MIXED_DEPTH_SAMPLES_SUPPORTED_NV                           0x932F
#define GL_MIXED_STENCIL_SAMPLES_SUPPORTED_NV                         0x9330
#define GL_MODELVIEW                                                  0x1700
#define GL_MODELVIEW0_ARB                                             0x1700
#define GL_MODELVIEW0_EXT                                             0x1700
#define GL_MODELVIEW0_MATRIX_EXT                                      0x0BA6
#define GL_MODELVIEW0_STACK_DEPTH_EXT                                 0x0BA3
#define GL_MODELVIEW10_ARB                                            0x872A
#define GL_MODELVIEW11_ARB                                            0x872B
#define GL_MODELVIEW12_ARB                                            0x872C
#define GL_MODELVIEW13_ARB                                            0x872D
#define GL_MODELVIEW14_ARB                                            0x872E
#define GL_MODELVIEW15_ARB                                            0x872F
#define GL_MODELVIEW16_ARB                                            0x8730
#define GL_MODELVIEW17_ARB                                            0x8731
#define GL_MODELVIEW18_ARB                                            0x8732
#define GL_MODELVIEW19_ARB                                            0x8733
#define GL_MODELVIEW1_ARB                                             0x850A
#define GL_MODELVIEW1_EXT                                             0x850A
#define GL_MODELVIEW1_MATRIX_EXT                                      0x8506
#define GL_MODELVIEW1_STACK_DEPTH_EXT                                 0x8502
#define GL_MODELVIEW20_ARB                                            0x8734
#define GL_MODELVIEW21_ARB                                            0x8735
#define GL_MODELVIEW22_ARB                                            0x8736
#define GL_MODELVIEW23_ARB                                            0x8737
#define GL_MODELVIEW24_ARB                                            0x8738
#define GL_MODELVIEW25_ARB                                            0x8739
#define GL_MODELVIEW26_ARB                                            0x873A
#define GL_MODELVIEW27_ARB                                            0x873B
#define GL_MODELVIEW28_ARB                                            0x873C
#define GL_MODELVIEW29_ARB                                            0x873D
#define GL_MODELVIEW2_ARB                                             0x8722
#define GL_MODELVIEW30_ARB                                            0x873E
#define GL_MODELVIEW31_ARB                                            0x873F
#define GL_MODELVIEW3_ARB                                             0x8723
#define GL_MODELVIEW4_ARB                                             0x8724
#define GL_MODELVIEW5_ARB                                             0x8725
#define GL_MODELVIEW6_ARB                                             0x8726
#define GL_MODELVIEW7_ARB                                             0x8727
#define GL_MODELVIEW8_ARB                                             0x8728
#define GL_MODELVIEW9_ARB                                             0x8729
#define GL_MODELVIEW_MATRIX                                           0x0BA6
#define GL_MODELVIEW_PROJECTION_NV                                    0x8629
#define GL_MODELVIEW_STACK_DEPTH                                      0x0BA3
#define GL_MODULATE                                                   0x2100
#define GL_MODULATE_ADD_ATI                                           0x8744
#define GL_MODULATE_SIGNED_ADD_ATI                                    0x8745
#define GL_MODULATE_SUBTRACT_ATI                                      0x8746
#define GL_MOVE_TO_CONTINUES_NV                                       0x90B6
#define GL_MOVE_TO_NV                                                 0x02
#define GL_MOVE_TO_RESETS_NV                                          0x90B5
#define GL_MOV_ATI                                                    0x8961
#define GL_MULT                                                       0x0103
#define GL_MULTICAST_GPUS_NV                                          0x92BA
#define GL_MULTICAST_PROGRAMMABLE_SAMPLE_LOCATION_NV                  0x9549
#define GL_MULTIPLY_KHR                                               0x9294
#define GL_MULTIPLY_NV                                                0x9294
#define GL_MULTISAMPLE                                                0x809D
#define GL_MULTISAMPLES_NV                                            0x9371
#define GL_MULTISAMPLE_3DFX                                           0x86B2
#define GL_MULTISAMPLE_ARB                                            0x809D
#define GL_MULTISAMPLE_BIT                                            0x20000000
#define GL_MULTISAMPLE_BIT_3DFX                                       0x20000000
#define GL_MULTISAMPLE_BIT_ARB                                        0x20000000
#define GL_MULTISAMPLE_BIT_EXT                                        0x20000000
#define GL_MULTISAMPLE_COVERAGE_MODES_NV                              0x8E12
#define GL_MULTISAMPLE_EXT                                            0x809D
#define GL_MULTISAMPLE_FILTER_HINT_NV                                 0x8534
#define GL_MULTISAMPLE_LINE_WIDTH_GRANULARITY_ARB                     0x9382
#define GL_MULTISAMPLE_LINE_WIDTH_RANGE_ARB                           0x9381
#define GL_MULTISAMPLE_RASTERIZATION_ALLOWED_EXT                      0x932B
#define GL_MULTISAMPLE_SGIS                                           0x809D
#define GL_MUL_ATI                                                    0x8964
#define GL_MVP_MATRIX_EXT                                             0x87E3
#define GL_N3F_V3F                                                    0x2A25
#define GL_NAMED_STRING_LENGTH_ARB                                    0x8DE9
#define GL_NAMED_STRING_TYPE_ARB                                      0x8DEA
#define GL_NAME_LENGTH                                                0x92F9
#define GL_NAME_STACK_DEPTH                                           0x0D70
#define GL_NAND                                                       0x150E
#define GL_NATIVE_GRAPHICS_BEGIN_HINT_PGI                             0x1A203
#define GL_NATIVE_GRAPHICS_END_HINT_PGI                               0x1A204
#define GL_NATIVE_GRAPHICS_HANDLE_PGI                                 0x1A202
#define GL_NEAREST                                                    0x2600
#define GL_NEAREST_CLIPMAP_LINEAR_SGIX                                0x844E
#define GL_NEAREST_CLIPMAP_NEAREST_SGIX                               0x844D
#define GL_NEAREST_MIPMAP_LINEAR                                      0x2702
#define GL_NEAREST_MIPMAP_NEAREST                                     0x2700
#define GL_NEGATE_BIT_ATI                                             0x00000004
#define GL_NEGATIVE_ONE_EXT                                           0x87DF
#define GL_NEGATIVE_ONE_TO_ONE                                        0x935E
#define GL_NEGATIVE_W_EXT                                             0x87DC
#define GL_NEGATIVE_X_EXT                                             0x87D9
#define GL_NEGATIVE_Y_EXT                                             0x87DA
#define GL_NEGATIVE_Z_EXT                                             0x87DB
#define GL_NEVER                                                      0x0200
#define GL_NEXT_BUFFER_NV                                             -2
#define GL_NEXT_VIDEO_CAPTURE_BUFFER_STATUS_NV                        0x9025
#define GL_NICEST                                                     0x1102
#define GL_NONE                                                       0
#define GL_NOOP                                                       0x1505
#define GL_NOP_COMMAND_NV                                             0x0001
#define GL_NOR                                                        0x1508
#define GL_NORMALIZE                                                  0x0BA1
#define GL_NORMALIZED_RANGE_EXT                                       0x87E0
#define GL_NORMAL_ARRAY                                               0x8075
#define GL_NORMAL_ARRAY_ADDRESS_NV                                    0x8F22
#define GL_NORMAL_ARRAY_BUFFER_BINDING                                0x8897
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB                            0x8897
#define GL_NORMAL_ARRAY_COUNT_EXT                                     0x8080
#define GL_NORMAL_ARRAY_EXT                                           0x8075
#define GL_NORMAL_ARRAY_LENGTH_NV                                     0x8F2C
#define GL_NORMAL_ARRAY_LIST_IBM                                      103071
#define GL_NORMAL_ARRAY_LIST_STRIDE_IBM                               103081
#define GL_NORMAL_ARRAY_PARALLEL_POINTERS_INTEL                       0x83F6
#define GL_NORMAL_ARRAY_POINTER                                       0x808F
#define GL_NORMAL_ARRAY_POINTER_EXT                                   0x808F
#define GL_NORMAL_ARRAY_STRIDE                                        0x807F
#define GL_NORMAL_ARRAY_STRIDE_EXT                                    0x807F
#define GL_NORMAL_ARRAY_TYPE                                          0x807E
#define GL_NORMAL_ARRAY_TYPE_EXT                                      0x807E
#define GL_NORMAL_BIT_PGI                                             0x08000000
#define GL_NORMAL_MAP                                                 0x8511
#define GL_NORMAL_MAP_ARB                                             0x8511
#define GL_NORMAL_MAP_EXT                                             0x8511
#define GL_NORMAL_MAP_NV                                              0x8511
#define GL_NOTEQUAL                                                   0x0205
#define GL_NO_ERROR                                                   0
#define GL_NO_RESET_NOTIFICATION                                      0x8261
#define GL_NO_RESET_NOTIFICATION_ARB                                  0x8261
#define GL_NO_RESET_NOTIFICATION_KHR                                  0x8261
#define GL_NUM_ACTIVE_VARIABLES                                       0x9304
#define GL_NUM_COMPATIBLE_SUBROUTINES                                 0x8E4A
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS                             0x86A2
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB                         0x86A2
#define GL_NUM_DEVICE_UUIDS_EXT                                       0x9596
#define GL_NUM_EXTENSIONS                                             0x821D
#define GL_NUM_FILL_STREAMS_NV                                        0x8E29
#define GL_NUM_FRAGMENT_CONSTANTS_ATI                                 0x896F
#define GL_NUM_FRAGMENT_REGISTERS_ATI                                 0x896E
#define GL_NUM_GENERAL_COMBINERS_NV                                   0x854E
#define GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI                      0x8973
#define GL_NUM_INSTRUCTIONS_PER_PASS_ATI                              0x8971
#define GL_NUM_INSTRUCTIONS_TOTAL_ATI                                 0x8972
#define GL_NUM_LOOPBACK_COMPONENTS_ATI                                0x8974
#define GL_NUM_PASSES_ATI                                             0x8970
#define GL_NUM_PROGRAM_BINARY_FORMATS                                 0x87FE
#define GL_NUM_SAMPLE_COUNTS                                          0x9380
#define GL_NUM_SHADER_BINARY_FORMATS                                  0x8DF9
#define GL_NUM_SHADING_LANGUAGE_VERSIONS                              0x82E9
#define GL_NUM_SPARSE_LEVELS_ARB                                      0x91AA
#define GL_NUM_SPIR_V_EXTENSIONS                                      0x9554
#define GL_NUM_SUPPORTED_MULTISAMPLE_MODES_AMD                        0x91B6
#define GL_NUM_TILING_TYPES_EXT                                       0x9582
#define GL_NUM_VIDEO_CAPTURE_STREAMS_NV                               0x9024
#define GL_NUM_VIRTUAL_PAGE_SIZES_ARB                                 0x91A8
#define GL_NUM_WINDOW_RECTANGLES_EXT                                  0x8F15
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB                               0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB                     0x8B8A
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB                                 0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB                       0x8B87
#define GL_OBJECT_ATTACHED_OBJECTS_ARB                                0x8B85
#define GL_OBJECT_BUFFER_SIZE_ATI                                     0x8764
#define GL_OBJECT_BUFFER_USAGE_ATI                                    0x8765
#define GL_OBJECT_COMPILE_STATUS_ARB                                  0x8B81
#define GL_OBJECT_DELETE_STATUS_ARB                                   0x8B80
#define GL_OBJECT_DISTANCE_TO_LINE_SGIS                               0x81F3
#define GL_OBJECT_DISTANCE_TO_POINT_SGIS                              0x81F1
#define GL_OBJECT_INFO_LOG_LENGTH_ARB                                 0x8B84
#define GL_OBJECT_LINEAR                                              0x2401
#define GL_OBJECT_LINEAR_NV                                           0x2401
#define GL_OBJECT_LINE_SGIS                                           0x81F7
#define GL_OBJECT_LINK_STATUS_ARB                                     0x8B82
#define GL_OBJECT_PLANE                                               0x2501
#define GL_OBJECT_POINT_SGIS                                          0x81F5
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB                            0x8B88
#define GL_OBJECT_SUBTYPE_ARB                                         0x8B4F
#define GL_OBJECT_TYPE                                                0x9112
#define GL_OBJECT_TYPE_ARB                                            0x8B4E
#define GL_OBJECT_VALIDATE_STATUS_ARB                                 0x8B83
#define GL_OCCLUSION_QUERY_EVENT_MASK_AMD                             0x874F
#define GL_OCCLUSION_TEST_HP                                          0x8165
#define GL_OCCLUSION_TEST_RESULT_HP                                   0x8166
#define GL_OFFSET                                                     0x92FC
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_2D_NV                       0x8856
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_RECTANGLE_NV                0x8857
#define GL_OFFSET_HILO_TEXTURE_2D_NV                                  0x8854
#define GL_OFFSET_HILO_TEXTURE_RECTANGLE_NV                           0x8855
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_NV                            0x8850
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_SCALE_NV                      0x8851
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_NV                     0x8852
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_SCALE_NV               0x8853
#define GL_OFFSET_TEXTURE_2D_BIAS_NV                                  0x86E3
#define GL_OFFSET_TEXTURE_2D_MATRIX_NV                                0x86E1
#define GL_OFFSET_TEXTURE_2D_NV                                       0x86E8
#define GL_OFFSET_TEXTURE_2D_SCALE_NV                                 0x86E2
#define GL_OFFSET_TEXTURE_BIAS_NV                                     0x86E3
#define GL_OFFSET_TEXTURE_MATRIX_NV                                   0x86E1
#define GL_OFFSET_TEXTURE_RECTANGLE_NV                                0x864C
#define GL_OFFSET_TEXTURE_RECTANGLE_SCALE_NV                          0x864D
#define GL_OFFSET_TEXTURE_SCALE_NV                                    0x86E2
#define GL_ONE                                                        1
#define GL_ONE_EXT                                                    0x87DE
#define GL_ONE_MINUS_CONSTANT_ALPHA                                   0x8004
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT                               0x8004
#define GL_ONE_MINUS_CONSTANT_COLOR                                   0x8002
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT                               0x8002
#define GL_ONE_MINUS_DST_ALPHA                                        0x0305
#define GL_ONE_MINUS_DST_COLOR                                        0x0307
#define GL_ONE_MINUS_SRC1_ALPHA                                       0x88FB
#define GL_ONE_MINUS_SRC1_COLOR                                       0x88FA
#define GL_ONE_MINUS_SRC_ALPHA                                        0x0303
#define GL_ONE_MINUS_SRC_COLOR                                        0x0301
#define GL_OPERAND0_ALPHA                                             0x8598
#define GL_OPERAND0_ALPHA_ARB                                         0x8598
#define GL_OPERAND0_ALPHA_EXT                                         0x8598
#define GL_OPERAND0_RGB                                               0x8590
#define GL_OPERAND0_RGB_ARB                                           0x8590
#define GL_OPERAND0_RGB_EXT                                           0x8590
#define GL_OPERAND1_ALPHA                                             0x8599
#define GL_OPERAND1_ALPHA_ARB                                         0x8599
#define GL_OPERAND1_ALPHA_EXT                                         0x8599
#define GL_OPERAND1_RGB                                               0x8591
#define GL_OPERAND1_RGB_ARB                                           0x8591
#define GL_OPERAND1_RGB_EXT                                           0x8591
#define GL_OPERAND2_ALPHA                                             0x859A
#define GL_OPERAND2_ALPHA_ARB                                         0x859A
#define GL_OPERAND2_ALPHA_EXT                                         0x859A
#define GL_OPERAND2_RGB                                               0x8592
#define GL_OPERAND2_RGB_ARB                                           0x8592
#define GL_OPERAND2_RGB_EXT                                           0x8592
#define GL_OPERAND3_ALPHA_NV                                          0x859B
#define GL_OPERAND3_RGB_NV                                            0x8593
#define GL_OPTIMAL_TILING_EXT                                         0x9584
#define GL_OP_ADD_EXT                                                 0x8787
#define GL_OP_CLAMP_EXT                                               0x878E
#define GL_OP_CROSS_PRODUCT_EXT                                       0x8797
#define GL_OP_DOT3_EXT                                                0x8784
#define GL_OP_DOT4_EXT                                                0x8785
#define GL_OP_EXP_BASE_2_EXT                                          0x8791
#define GL_OP_FLOOR_EXT                                               0x878F
#define GL_OP_FRAC_EXT                                                0x8789
#define GL_OP_INDEX_EXT                                               0x8782
#define GL_OP_LOG_BASE_2_EXT                                          0x8792
#define GL_OP_MADD_EXT                                                0x8788
#define GL_OP_MAX_EXT                                                 0x878A
#define GL_OP_MIN_EXT                                                 0x878B
#define GL_OP_MOV_EXT                                                 0x8799
#define GL_OP_MULTIPLY_MATRIX_EXT                                     0x8798
#define GL_OP_MUL_EXT                                                 0x8786
#define GL_OP_NEGATE_EXT                                              0x8783
#define GL_OP_POWER_EXT                                               0x8793
#define GL_OP_RECIP_EXT                                               0x8794
#define GL_OP_RECIP_SQRT_EXT                                          0x8795
#define GL_OP_ROUND_EXT                                               0x8790
#define GL_OP_SET_GE_EXT                                              0x878C
#define GL_OP_SET_LT_EXT                                              0x878D
#define GL_OP_SUB_EXT                                                 0x8796
#define GL_OR                                                         0x1507
#define GL_ORDER                                                      0x0A01
#define GL_OR_INVERTED                                                0x150D
#define GL_OR_REVERSE                                                 0x150B
#define GL_OUTPUT_COLOR0_EXT                                          0x879B
#define GL_OUTPUT_COLOR1_EXT                                          0x879C
#define GL_OUTPUT_FOG_EXT                                             0x87BD
#define GL_OUTPUT_TEXTURE_COORD0_EXT                                  0x879D
#define GL_OUTPUT_TEXTURE_COORD10_EXT                                 0x87A7
#define GL_OUTPUT_TEXTURE_COORD11_EXT                                 0x87A8
#define GL_OUTPUT_TEXTURE_COORD12_EXT                                 0x87A9
#define GL_OUTPUT_TEXTURE_COORD13_EXT                                 0x87AA
#define GL_OUTPUT_TEXTURE_COORD14_EXT                                 0x87AB
#define GL_OUTPUT_TEXTURE_COORD15_EXT                                 0x87AC
#define GL_OUTPUT_TEXTURE_COORD16_EXT                                 0x87AD
#define GL_OUTPUT_TEXTURE_COORD17_EXT                                 0x87AE
#define GL_OUTPUT_TEXTURE_COORD18_EXT                                 0x87AF
#define GL_OUTPUT_TEXTURE_COORD19_EXT                                 0x87B0
#define GL_OUTPUT_TEXTURE_COORD1_EXT                                  0x879E
#define GL_OUTPUT_TEXTURE_COORD20_EXT                                 0x87B1
#define GL_OUTPUT_TEXTURE_COORD21_EXT                                 0x87B2
#define GL_OUTPUT_TEXTURE_COORD22_EXT                                 0x87B3
#define GL_OUTPUT_TEXTURE_COORD23_EXT                                 0x87B4
#define GL_OUTPUT_TEXTURE_COORD24_EXT                                 0x87B5
#define GL_OUTPUT_TEXTURE_COORD25_EXT                                 0x87B6
#define GL_OUTPUT_TEXTURE_COORD26_EXT                                 0x87B7
#define GL_OUTPUT_TEXTURE_COORD27_EXT                                 0x87B8
#define GL_OUTPUT_TEXTURE_COORD28_EXT                                 0x87B9
#define GL_OUTPUT_TEXTURE_COORD29_EXT                                 0x87BA
#define GL_OUTPUT_TEXTURE_COORD2_EXT                                  0x879F
#define GL_OUTPUT_TEXTURE_COORD30_EXT                                 0x87BB
#define GL_OUTPUT_TEXTURE_COORD31_EXT                                 0x87BC
#define GL_OUTPUT_TEXTURE_COORD3_EXT                                  0x87A0
#define GL_OUTPUT_TEXTURE_COORD4_EXT                                  0x87A1
#define GL_OUTPUT_TEXTURE_COORD5_EXT                                  0x87A2
#define GL_OUTPUT_TEXTURE_COORD6_EXT                                  0x87A3
#define GL_OUTPUT_TEXTURE_COORD7_EXT                                  0x87A4
#define GL_OUTPUT_TEXTURE_COORD8_EXT                                  0x87A5
#define GL_OUTPUT_TEXTURE_COORD9_EXT                                  0x87A6
#define GL_OUTPUT_VERTEX_EXT                                          0x879A
#define GL_OUT_OF_MEMORY                                              0x0505
#define GL_OVERLAY_KHR                                                0x9296
#define GL_OVERLAY_NV                                                 0x9296
#define GL_PACK_ALIGNMENT                                             0x0D05
#define GL_PACK_CMYK_HINT_EXT                                         0x800E
#define GL_PACK_COMPRESSED_BLOCK_DEPTH                                0x912D
#define GL_PACK_COMPRESSED_BLOCK_HEIGHT                               0x912C
#define GL_PACK_COMPRESSED_BLOCK_SIZE                                 0x912E
#define GL_PACK_COMPRESSED_BLOCK_WIDTH                                0x912B
#define GL_PACK_IMAGE_DEPTH_SGIS                                      0x8131
#define GL_PACK_IMAGE_HEIGHT                                          0x806C
#define GL_PACK_IMAGE_HEIGHT_EXT                                      0x806C
#define GL_PACK_INVERT_MESA                                           0x8758
#define GL_PACK_LSB_FIRST                                             0x0D01
#define GL_PACK_RESAMPLE_OML                                          0x8984
#define GL_PACK_RESAMPLE_SGIX                                         0x842E
#define GL_PACK_ROW_BYTES_APPLE                                       0x8A15
#define GL_PACK_ROW_LENGTH                                            0x0D02
#define GL_PACK_SKIP_IMAGES                                           0x806B
#define GL_PACK_SKIP_IMAGES_EXT                                       0x806B
#define GL_PACK_SKIP_PIXELS                                           0x0D04
#define GL_PACK_SKIP_ROWS                                             0x0D03
#define GL_PACK_SKIP_VOLUMES_SGIS                                     0x8130
#define GL_PACK_SUBSAMPLE_RATE_SGIX                                   0x85A0
#define GL_PACK_SWAP_BYTES                                            0x0D00
#define GL_PALETTE4_R5_G6_B5_OES                                      0x8B92
#define GL_PALETTE4_RGB5_A1_OES                                       0x8B94
#define GL_PALETTE4_RGB8_OES                                          0x8B90
#define GL_PALETTE4_RGBA4_OES                                         0x8B93
#define GL_PALETTE4_RGBA8_OES                                         0x8B91
#define GL_PALETTE8_R5_G6_B5_OES                                      0x8B97
#define GL_PALETTE8_RGB5_A1_OES                                       0x8B99
#define GL_PALETTE8_RGB8_OES                                          0x8B95
#define GL_PALETTE8_RGBA4_OES                                         0x8B98
#define GL_PALETTE8_RGBA8_OES                                         0x8B96
#define GL_PARALLEL_ARRAYS_INTEL                                      0x83F4
#define GL_PARAMETER_BUFFER                                           0x80EE
#define GL_PARAMETER_BUFFER_ARB                                       0x80EE
#define GL_PARAMETER_BUFFER_BINDING                                   0x80EF
#define GL_PARAMETER_BUFFER_BINDING_ARB                               0x80EF
#define GL_PARTIAL_SUCCESS_NV                                         0x902E
#define GL_PASS_THROUGH_NV                                            0x86E6
#define GL_PASS_THROUGH_TOKEN                                         0x0700
#define GL_PATCHES                                                    0x000E
#define GL_PATCH_DEFAULT_INNER_LEVEL                                  0x8E73
#define GL_PATCH_DEFAULT_OUTER_LEVEL                                  0x8E74
#define GL_PATCH_VERTICES                                             0x8E72
#define GL_PATH_CLIENT_LENGTH_NV                                      0x907F
#define GL_PATH_COMMAND_COUNT_NV                                      0x909D
#define GL_PATH_COMPUTED_LENGTH_NV                                    0x90A0
#define GL_PATH_COORD_COUNT_NV                                        0x909E
#define GL_PATH_COVER_DEPTH_FUNC_NV                                   0x90BF
#define GL_PATH_DASH_ARRAY_COUNT_NV                                   0x909F
#define GL_PATH_DASH_CAPS_NV                                          0x907B
#define GL_PATH_DASH_OFFSET_NV                                        0x907E
#define GL_PATH_DASH_OFFSET_RESET_NV                                  0x90B4
#define GL_PATH_END_CAPS_NV                                           0x9076
#define GL_PATH_ERROR_POSITION_NV                                     0x90AB
#define GL_PATH_FILL_BOUNDING_BOX_NV                                  0x90A1
#define GL_PATH_FILL_COVER_MODE_NV                                    0x9082
#define GL_PATH_FILL_MASK_NV                                          0x9081
#define GL_PATH_FILL_MODE_NV                                          0x9080
#define GL_PATH_FOG_GEN_MODE_NV                                       0x90AC
#define GL_PATH_FORMAT_PS_NV                                          0x9071
#define GL_PATH_FORMAT_SVG_NV                                         0x9070
#define GL_PATH_GEN_COEFF_NV                                          0x90B1
#define GL_PATH_GEN_COLOR_FORMAT_NV                                   0x90B2
#define GL_PATH_GEN_COMPONENTS_NV                                     0x90B3
#define GL_PATH_GEN_MODE_NV                                           0x90B0
#define GL_PATH_INITIAL_DASH_CAP_NV                                   0x907C
#define GL_PATH_INITIAL_END_CAP_NV                                    0x9077
#define GL_PATH_JOIN_STYLE_NV                                         0x9079
#define GL_PATH_MAX_MODELVIEW_STACK_DEPTH_NV                          0x0D36
#define GL_PATH_MAX_PROJECTION_STACK_DEPTH_NV                         0x0D38
#define GL_PATH_MITER_LIMIT_NV                                        0x907A
#define GL_PATH_MODELVIEW_MATRIX_NV                                   0x0BA6
#define GL_PATH_MODELVIEW_NV                                          0x1700
#define GL_PATH_MODELVIEW_STACK_DEPTH_NV                              0x0BA3
#define GL_PATH_OBJECT_BOUNDING_BOX_NV                                0x908A
#define GL_PATH_PROJECTION_MATRIX_NV                                  0x0BA7
#define GL_PATH_PROJECTION_NV                                         0x1701
#define GL_PATH_PROJECTION_STACK_DEPTH_NV                             0x0BA4
#define GL_PATH_STENCIL_DEPTH_OFFSET_FACTOR_NV                        0x90BD
#define GL_PATH_STENCIL_DEPTH_OFFSET_UNITS_NV                         0x90BE
#define GL_PATH_STENCIL_FUNC_NV                                       0x90B7
#define GL_PATH_STENCIL_REF_NV                                        0x90B8
#define GL_PATH_STENCIL_VALUE_MASK_NV                                 0x90B9
#define GL_PATH_STROKE_BOUNDING_BOX_NV                                0x90A2
#define GL_PATH_STROKE_COVER_MODE_NV                                  0x9083
#define GL_PATH_STROKE_MASK_NV                                        0x9084
#define GL_PATH_STROKE_WIDTH_NV                                       0x9075
#define GL_PATH_TERMINAL_DASH_CAP_NV                                  0x907D
#define GL_PATH_TERMINAL_END_CAP_NV                                   0x9078
#define GL_PATH_TRANSPOSE_MODELVIEW_MATRIX_NV                         0x84E3
#define GL_PATH_TRANSPOSE_PROJECTION_MATRIX_NV                        0x84E4
#define GL_PERCENTAGE_AMD                                             0x8BC3
#define GL_PERFMON_RESULT_AMD                                         0x8BC6
#define GL_PERFMON_RESULT_AVAILABLE_AMD                               0x8BC4
#define GL_PERFMON_RESULT_SIZE_AMD                                    0x8BC5
#define GL_PERFORMANCE_MONITOR_AMD                                    0x9152
#define GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL                        0x94FC
#define GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL                        0x94FB
#define GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL                         0x94FA
#define GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL                        0x94F8
#define GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL                        0x94F9
#define GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL                    0x94FF
#define GL_PERFQUERY_COUNTER_DURATION_NORM_INTEL                      0x94F1
#define GL_PERFQUERY_COUNTER_DURATION_RAW_INTEL                       0x94F2
#define GL_PERFQUERY_COUNTER_EVENT_INTEL                              0x94F0
#define GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL                    0x94FE
#define GL_PERFQUERY_COUNTER_RAW_INTEL                                0x94F4
#define GL_PERFQUERY_COUNTER_THROUGHPUT_INTEL                         0x94F3
#define GL_PERFQUERY_COUNTER_TIMESTAMP_INTEL                          0x94F5
#define GL_PERFQUERY_DONOT_FLUSH_INTEL                                0x83F9
#define GL_PERFQUERY_FLUSH_INTEL                                      0x83FA
#define GL_PERFQUERY_GLOBAL_CONTEXT_INTEL                             0x00000001
#define GL_PERFQUERY_GPA_EXTENDED_COUNTERS_INTEL                      0x9500
#define GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL                      0x94FD
#define GL_PERFQUERY_SINGLE_CONTEXT_INTEL                             0x00000000
#define GL_PERFQUERY_WAIT_INTEL                                       0x83FB
#define GL_PERSPECTIVE_CORRECTION_HINT                                0x0C50
#define GL_PERTURB_EXT                                                0x85AE
#define GL_PER_GPU_STORAGE_BIT_NV                                     0x0800
#define GL_PER_GPU_STORAGE_NV                                         0x9548
#define GL_PER_STAGE_CONSTANTS_NV                                     0x8535
#define GL_PHONG_HINT_WIN                                             0x80EB
#define GL_PHONG_WIN                                                  0x80EA
#define GL_PINLIGHT_NV                                                0x92A8
#define GL_PIXELS_PER_SAMPLE_PATTERN_X_AMD                            0x91AE
#define GL_PIXELS_PER_SAMPLE_PATTERN_Y_AMD                            0x91AF
#define GL_PIXEL_BUFFER_BARRIER_BIT                                   0x00000080
#define GL_PIXEL_BUFFER_BARRIER_BIT_EXT                               0x00000080
#define GL_PIXEL_COUNTER_BITS_NV                                      0x8864
#define GL_PIXEL_COUNT_AVAILABLE_NV                                   0x8867
#define GL_PIXEL_COUNT_NV                                             0x8866
#define GL_PIXEL_CUBIC_WEIGHT_EXT                                     0x8333
#define GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS                           0x8355
#define GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS                             0x8354
#define GL_PIXEL_GROUP_COLOR_SGIS                                     0x8356
#define GL_PIXEL_MAG_FILTER_EXT                                       0x8331
#define GL_PIXEL_MAP_A_TO_A                                           0x0C79
#define GL_PIXEL_MAP_A_TO_A_SIZE                                      0x0CB9
#define GL_PIXEL_MAP_B_TO_B                                           0x0C78
#define GL_PIXEL_MAP_B_TO_B_SIZE                                      0x0CB8
#define GL_PIXEL_MAP_G_TO_G                                           0x0C77
#define GL_PIXEL_MAP_G_TO_G_SIZE                                      0x0CB7
#define GL_PIXEL_MAP_I_TO_A                                           0x0C75
#define GL_PIXEL_MAP_I_TO_A_SIZE                                      0x0CB5
#define GL_PIXEL_MAP_I_TO_B                                           0x0C74
#define GL_PIXEL_MAP_I_TO_B_SIZE                                      0x0CB4
#define GL_PIXEL_MAP_I_TO_G                                           0x0C73
#define GL_PIXEL_MAP_I_TO_G_SIZE                                      0x0CB3
#define GL_PIXEL_MAP_I_TO_I                                           0x0C70
#define GL_PIXEL_MAP_I_TO_I_SIZE                                      0x0CB0
#define GL_PIXEL_MAP_I_TO_R                                           0x0C72
#define GL_PIXEL_MAP_I_TO_R_SIZE                                      0x0CB2
#define GL_PIXEL_MAP_R_TO_R                                           0x0C76
#define GL_PIXEL_MAP_R_TO_R_SIZE                                      0x0CB6
#define GL_PIXEL_MAP_S_TO_S                                           0x0C71
#define GL_PIXEL_MAP_S_TO_S_SIZE                                      0x0CB1
#define GL_PIXEL_MIN_FILTER_EXT                                       0x8332
#define GL_PIXEL_MODE_BIT                                             0x00000020
#define GL_PIXEL_PACK_BUFFER                                          0x88EB
#define GL_PIXEL_PACK_BUFFER_ARB                                      0x88EB
#define GL_PIXEL_PACK_BUFFER_BINDING                                  0x88ED
#define GL_PIXEL_PACK_BUFFER_BINDING_ARB                              0x88ED
#define GL_PIXEL_PACK_BUFFER_BINDING_EXT                              0x88ED
#define GL_PIXEL_PACK_BUFFER_EXT                                      0x88EB
#define GL_PIXEL_SUBSAMPLE_2424_SGIX                                  0x85A3
#define GL_PIXEL_SUBSAMPLE_4242_SGIX                                  0x85A4
#define GL_PIXEL_SUBSAMPLE_4444_SGIX                                  0x85A2
#define GL_PIXEL_TEXTURE_SGIS                                         0x8353
#define GL_PIXEL_TEX_GEN_MODE_SGIX                                    0x832B
#define GL_PIXEL_TEX_GEN_SGIX                                         0x8139
#define GL_PIXEL_TILE_BEST_ALIGNMENT_SGIX                             0x813E
#define GL_PIXEL_TILE_CACHE_INCREMENT_SGIX                            0x813F
#define GL_PIXEL_TILE_CACHE_SIZE_SGIX                                 0x8145
#define GL_PIXEL_TILE_GRID_DEPTH_SGIX                                 0x8144
#define GL_PIXEL_TILE_GRID_HEIGHT_SGIX                                0x8143
#define GL_PIXEL_TILE_GRID_WIDTH_SGIX                                 0x8142
#define GL_PIXEL_TILE_HEIGHT_SGIX                                     0x8141
#define GL_PIXEL_TILE_WIDTH_SGIX                                      0x8140
#define GL_PIXEL_TRANSFORM_2D_EXT                                     0x8330
#define GL_PIXEL_TRANSFORM_2D_MATRIX_EXT                              0x8338
#define GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT                         0x8336
#define GL_PIXEL_UNPACK_BUFFER                                        0x88EC
#define GL_PIXEL_UNPACK_BUFFER_ARB                                    0x88EC
#define GL_PIXEL_UNPACK_BUFFER_BINDING                                0x88EF
#define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB                            0x88EF
#define GL_PIXEL_UNPACK_BUFFER_BINDING_EXT                            0x88EF
#define GL_PIXEL_UNPACK_BUFFER_EXT                                    0x88EC
#define GL_PLUS_CLAMPED_ALPHA_NV                                      0x92B2
#define GL_PLUS_CLAMPED_NV                                            0x92B1
#define GL_PLUS_DARKER_NV                                             0x9292
#define GL_PLUS_NV                                                    0x9291
#define GL_PN_TRIANGLES_ATI                                           0x87F0
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI                               0x87F3
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI                        0x87F7
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI                     0x87F8
#define GL_PN_TRIANGLES_POINT_MODE_ATI                                0x87F2
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI                          0x87F6
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI                         0x87F5
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI                         0x87F4
#define GL_POINT                                                      0x1B00
#define GL_POINTS                                                     0x0000
#define GL_POINT_BIT                                                  0x00000002
#define GL_POINT_DISTANCE_ATTENUATION                                 0x8129
#define GL_POINT_DISTANCE_ATTENUATION_ARB                             0x8129
#define GL_POINT_FADE_THRESHOLD_SIZE                                  0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_ARB                              0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT                              0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_SGIS                             0x8128
#define GL_POINT_SIZE                                                 0x0B11
#define GL_POINT_SIZE_GRANULARITY                                     0x0B13
#define GL_POINT_SIZE_MAX                                             0x8127
#define GL_POINT_SIZE_MAX_ARB                                         0x8127
#define GL_POINT_SIZE_MAX_EXT                                         0x8127
#define GL_POINT_SIZE_MAX_SGIS                                        0x8127
#define GL_POINT_SIZE_MIN                                             0x8126
#define GL_POINT_SIZE_MIN_ARB                                         0x8126
#define GL_POINT_SIZE_MIN_EXT                                         0x8126
#define GL_POINT_SIZE_MIN_SGIS                                        0x8126
#define GL_POINT_SIZE_RANGE                                           0x0B12
#define GL_POINT_SMOOTH                                               0x0B10
#define GL_POINT_SMOOTH_HINT                                          0x0C51
#define GL_POINT_SPRITE                                               0x8861
#define GL_POINT_SPRITE_ARB                                           0x8861
#define GL_POINT_SPRITE_COORD_ORIGIN                                  0x8CA0
#define GL_POINT_SPRITE_NV                                            0x8861
#define GL_POINT_SPRITE_R_MODE_NV                                     0x8863
#define GL_POINT_TOKEN                                                0x0701
#define GL_POLYGON                                                    0x0009
#define GL_POLYGON_BIT                                                0x00000008
#define GL_POLYGON_MODE                                               0x0B40
#define GL_POLYGON_OFFSET_BIAS_EXT                                    0x8039
#define GL_POLYGON_OFFSET_CLAMP                                       0x8E1B
#define GL_POLYGON_OFFSET_CLAMP_EXT                                   0x8E1B
#define GL_POLYGON_OFFSET_COMMAND_NV                                  0x000E
#define GL_POLYGON_OFFSET_EXT                                         0x8037
#define GL_POLYGON_OFFSET_FACTOR                                      0x8038
#define GL_POLYGON_OFFSET_FACTOR_EXT                                  0x8038
#define GL_POLYGON_OFFSET_FILL                                        0x8037
#define GL_POLYGON_OFFSET_LINE                                        0x2A02
#define GL_POLYGON_OFFSET_POINT                                       0x2A01
#define GL_POLYGON_OFFSET_UNITS                                       0x2A00
#define GL_POLYGON_SMOOTH                                             0x0B41
#define GL_POLYGON_SMOOTH_HINT                                        0x0C53
#define GL_POLYGON_STIPPLE                                            0x0B42
#define GL_POLYGON_STIPPLE_BIT                                        0x00000010
#define GL_POLYGON_TOKEN                                              0x0703
#define GL_POSITION                                                   0x1203
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS                               0x80BB
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI                           0x80BB
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE                              0x80B7
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI                          0x80B7
#define GL_POST_COLOR_MATRIX_BLUE_BIAS                                0x80BA
#define GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI                            0x80BA
#define GL_POST_COLOR_MATRIX_BLUE_SCALE                               0x80B6
#define GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI                           0x80B6
#define GL_POST_COLOR_MATRIX_COLOR_TABLE                              0x80D2
#define GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI                          0x80D2
#define GL_POST_COLOR_MATRIX_GREEN_BIAS                               0x80B9
#define GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI                           0x80B9
#define GL_POST_COLOR_MATRIX_GREEN_SCALE                              0x80B5
#define GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI                          0x80B5
#define GL_POST_COLOR_MATRIX_RED_BIAS                                 0x80B8
#define GL_POST_COLOR_MATRIX_RED_BIAS_SGI                             0x80B8
#define GL_POST_COLOR_MATRIX_RED_SCALE                                0x80B4
#define GL_POST_COLOR_MATRIX_RED_SCALE_SGI                            0x80B4
#define GL_POST_CONVOLUTION_ALPHA_BIAS                                0x8023
#define GL_POST_CONVOLUTION_ALPHA_BIAS_EXT                            0x8023
#define GL_POST_CONVOLUTION_ALPHA_SCALE                               0x801F
#define GL_POST_CONVOLUTION_ALPHA_SCALE_EXT                           0x801F
#define GL_POST_CONVOLUTION_BLUE_BIAS                                 0x8022
#define GL_POST_CONVOLUTION_BLUE_BIAS_EXT                             0x8022
#define GL_POST_CONVOLUTION_BLUE_SCALE                                0x801E
#define GL_POST_CONVOLUTION_BLUE_SCALE_EXT                            0x801E
#define GL_POST_CONVOLUTION_COLOR_TABLE                               0x80D1
#define GL_POST_CONVOLUTION_COLOR_TABLE_SGI                           0x80D1
#define GL_POST_CONVOLUTION_GREEN_BIAS                                0x8021
#define GL_POST_CONVOLUTION_GREEN_BIAS_EXT                            0x8021
#define GL_POST_CONVOLUTION_GREEN_SCALE                               0x801D
#define GL_POST_CONVOLUTION_GREEN_SCALE_EXT                           0x801D
#define GL_POST_CONVOLUTION_RED_BIAS                                  0x8020
#define GL_POST_CONVOLUTION_RED_BIAS_EXT                              0x8020
#define GL_POST_CONVOLUTION_RED_SCALE                                 0x801C
#define GL_POST_CONVOLUTION_RED_SCALE_EXT                             0x801C
#define GL_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP                        0x8162
#define GL_POST_TEXTURE_FILTER_BIAS_RANGE_SGIX                        0x817B
#define GL_POST_TEXTURE_FILTER_BIAS_SGIX                              0x8179
#define GL_POST_TEXTURE_FILTER_SCALE_RANGE_SGIX                       0x817C
#define GL_POST_TEXTURE_FILTER_SCALE_SGIX                             0x817A
#define GL_PREFER_DOUBLEBUFFER_HINT_PGI                               0x1A1F8
#define GL_PRESENT_DURATION_NV                                        0x8E2B
#define GL_PRESENT_TIME_NV                                            0x8E2A
#define GL_PRESERVE_ATI                                               0x8762
#define GL_PREVIOUS                                                   0x8578
#define GL_PREVIOUS_ARB                                               0x8578
#define GL_PREVIOUS_EXT                                               0x8578
#define GL_PREVIOUS_TEXTURE_INPUT_NV                                  0x86E4
#define GL_PRIMARY_COLOR                                              0x8577
#define GL_PRIMARY_COLOR_ARB                                          0x8577
#define GL_PRIMARY_COLOR_EXT                                          0x8577
#define GL_PRIMARY_COLOR_NV                                           0x852C
#define GL_PRIMITIVES_GENERATED                                       0x8C87
#define GL_PRIMITIVES_GENERATED_EXT                                   0x8C87
#define GL_PRIMITIVES_GENERATED_NV                                    0x8C87
#define GL_PRIMITIVES_SUBMITTED                                       0x82EF
#define GL_PRIMITIVES_SUBMITTED_ARB                                   0x82EF
#define GL_PRIMITIVE_BOUNDING_BOX_ARB                                 0x92BE
#define GL_PRIMITIVE_ID_NV                                            0x8C7C
#define GL_PRIMITIVE_RESTART                                          0x8F9D
#define GL_PRIMITIVE_RESTART_FIXED_INDEX                              0x8D69
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED                    0x8221
#define GL_PRIMITIVE_RESTART_INDEX                                    0x8F9E
#define GL_PRIMITIVE_RESTART_INDEX_NV                                 0x8559
#define GL_PRIMITIVE_RESTART_NV                                       0x8558
#define GL_PROGRAM                                                    0x82E2
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_ARB                           0x9341
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_NV                            0x9341
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_ARB                0x9340
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_NV                 0x9340
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB                              0x88B0
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB                               0x8805
#define GL_PROGRAM_ATTRIBS_ARB                                        0x88AC
#define GL_PROGRAM_ATTRIB_COMPONENTS_NV                               0x8906
#define GL_PROGRAM_BINARY_FORMATS                                     0x87FF
#define GL_PROGRAM_BINARY_FORMAT_MESA                                 0x875F
#define GL_PROGRAM_BINARY_LENGTH                                      0x8741
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT                            0x8257
#define GL_PROGRAM_BINDING_ARB                                        0x8677
#define GL_PROGRAM_ERROR_POSITION_ARB                                 0x864B
#define GL_PROGRAM_ERROR_POSITION_NV                                  0x864B
#define GL_PROGRAM_ERROR_STRING_ARB                                   0x8874
#define GL_PROGRAM_ERROR_STRING_NV                                    0x8874
#define GL_PROGRAM_FORMAT_ARB                                         0x8876
#define GL_PROGRAM_FORMAT_ASCII_ARB                                   0x8875
#define GL_PROGRAM_INPUT                                              0x92E3
#define GL_PROGRAM_INSTRUCTIONS_ARB                                   0x88A0
#define GL_PROGRAM_KHR                                                0x82E2
#define GL_PROGRAM_LENGTH_ARB                                         0x8627
#define GL_PROGRAM_LENGTH_NV                                          0x8627
#define GL_PROGRAM_MATRIX_EXT                                         0x8E2D
#define GL_PROGRAM_MATRIX_STACK_DEPTH_EXT                             0x8E2F
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB                       0x88B2
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB                        0x8808
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB                                 0x88AE
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB                            0x88A2
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB                              0x88AA
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB                             0x88A6
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB                        0x880A
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB                        0x8809
#define GL_PROGRAM_OBJECT_ARB                                         0x8B40
#define GL_PROGRAM_OBJECT_EXT                                         0x8B40
#define GL_PROGRAM_OUTPUT                                             0x92E4
#define GL_PROGRAM_PARAMETERS_ARB                                     0x88A8
#define GL_PROGRAM_PARAMETER_NV                                       0x8644
#define GL_PROGRAM_PIPELINE                                           0x82E4
#define GL_PROGRAM_PIPELINE_BINDING                                   0x825A
#define GL_PROGRAM_PIPELINE_BINDING_EXT                               0x825A
#define GL_PROGRAM_PIPELINE_KHR                                       0x82E4
#define GL_PROGRAM_PIPELINE_OBJECT_EXT                                0x8A4F
#define GL_PROGRAM_POINT_SIZE                                         0x8642
#define GL_PROGRAM_POINT_SIZE_ARB                                     0x8642
#define GL_PROGRAM_POINT_SIZE_EXT                                     0x8642
#define GL_PROGRAM_RESIDENT_NV                                        0x8647
#define GL_PROGRAM_RESULT_COMPONENTS_NV                               0x8907
#define GL_PROGRAM_SEPARABLE                                          0x8258
#define GL_PROGRAM_SEPARABLE_EXT                                      0x8258
#define GL_PROGRAM_STRING_ARB                                         0x8628
#define GL_PROGRAM_STRING_NV                                          0x8628
#define GL_PROGRAM_TARGET_NV                                          0x8646
#define GL_PROGRAM_TEMPORARIES_ARB                                    0x88A4
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB                               0x8807
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB                               0x8806
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB                            0x88B6
#define GL_PROJECTION                                                 0x1701
#define GL_PROJECTION_MATRIX                                          0x0BA7
#define GL_PROJECTION_STACK_DEPTH                                     0x0BA4
#define GL_PROTECTED_MEMORY_OBJECT_EXT                                0x959B
#define GL_PROVOKING_VERTEX                                           0x8E4F
#define GL_PROVOKING_VERTEX_EXT                                       0x8E4F
#define GL_PROXY_COLOR_TABLE                                          0x80D3
#define GL_PROXY_COLOR_TABLE_SGI                                      0x80D3
#define GL_PROXY_HISTOGRAM                                            0x8025
#define GL_PROXY_HISTOGRAM_EXT                                        0x8025
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE                        0x80D5
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI                    0x80D5
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE                         0x80D4
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI                     0x80D4
#define GL_PROXY_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP                  0x8163
#define GL_PROXY_TEXTURE_1D                                           0x8063
#define GL_PROXY_TEXTURE_1D_ARRAY                                     0x8C19
#define GL_PROXY_TEXTURE_1D_ARRAY_EXT                                 0x8C19
#define GL_PROXY_TEXTURE_1D_EXT                                       0x8063
#define GL_PROXY_TEXTURE_1D_STACK_MESAX                               0x875B
#define GL_PROXY_TEXTURE_2D                                           0x8064
#define GL_PROXY_TEXTURE_2D_ARRAY                                     0x8C1B
#define GL_PROXY_TEXTURE_2D_ARRAY_EXT                                 0x8C1B
#define GL_PROXY_TEXTURE_2D_EXT                                       0x8064
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE                               0x9101
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY                         0x9103
#define GL_PROXY_TEXTURE_2D_STACK_MESAX                               0x875C
#define GL_PROXY_TEXTURE_3D                                           0x8070
#define GL_PROXY_TEXTURE_3D_EXT                                       0x8070
#define GL_PROXY_TEXTURE_4D_SGIS                                      0x8135
#define GL_PROXY_TEXTURE_COLOR_TABLE_SGI                              0x80BD
#define GL_PROXY_TEXTURE_CUBE_MAP                                     0x851B
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB                                 0x851B
#define GL_PROXY_TEXTURE_CUBE_MAP_ARRAY                               0x900B
#define GL_PROXY_TEXTURE_CUBE_MAP_ARRAY_ARB                           0x900B
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT                                 0x851B
#define GL_PROXY_TEXTURE_RECTANGLE                                    0x84F7
#define GL_PROXY_TEXTURE_RECTANGLE_ARB                                0x84F7
#define GL_PROXY_TEXTURE_RECTANGLE_NV                                 0x84F7
#define GL_PURGEABLE_APPLE                                            0x8A1D
#define GL_PURGED_CONTEXT_RESET_NV                                    0x92BB
#define GL_Q                                                          0x2003
#define GL_QUADRATIC_ATTENUATION                                      0x1209
#define GL_QUADRATIC_CURVE_TO_NV                                      0x0A
#define GL_QUADS                                                      0x0007
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION                   0x8E4C
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION_EXT               0x8E4C
#define GL_QUAD_ALPHA4_SGIS                                           0x811E
#define GL_QUAD_ALPHA8_SGIS                                           0x811F
#define GL_QUAD_INTENSITY4_SGIS                                       0x8122
#define GL_QUAD_INTENSITY8_SGIS                                       0x8123
#define GL_QUAD_LUMINANCE4_SGIS                                       0x8120
#define GL_QUAD_LUMINANCE8_SGIS                                       0x8121
#define GL_QUAD_MESH_SUN                                              0x8614
#define GL_QUAD_STRIP                                                 0x0008
#define GL_QUAD_TEXTURE_SELECT_SGIS                                   0x8125
#define GL_QUARTER_BIT_ATI                                            0x00000010
#define GL_QUERY                                                      0x82E3
#define GL_QUERY_ALL_EVENT_BITS_AMD                                   0xFFFFFFFF
#define GL_QUERY_BUFFER                                               0x9192
#define GL_QUERY_BUFFER_AMD                                           0x9192
#define GL_QUERY_BUFFER_BARRIER_BIT                                   0x00008000
#define GL_QUERY_BUFFER_BINDING                                       0x9193
#define GL_QUERY_BUFFER_BINDING_AMD                                   0x9193
#define GL_QUERY_BY_REGION_NO_WAIT                                    0x8E16
#define GL_QUERY_BY_REGION_NO_WAIT_INVERTED                           0x8E1A
#define GL_QUERY_BY_REGION_NO_WAIT_NV                                 0x8E16
#define GL_QUERY_BY_REGION_WAIT                                       0x8E15
#define GL_QUERY_BY_REGION_WAIT_INVERTED                              0x8E19
#define GL_QUERY_BY_REGION_WAIT_NV                                    0x8E15
#define GL_QUERY_COUNTER_BITS                                         0x8864
#define GL_QUERY_COUNTER_BITS_ARB                                     0x8864
#define GL_QUERY_DEPTH_BOUNDS_FAIL_EVENT_BIT_AMD                      0x00000008
#define GL_QUERY_DEPTH_FAIL_EVENT_BIT_AMD                             0x00000002
#define GL_QUERY_DEPTH_PASS_EVENT_BIT_AMD                             0x00000001
#define GL_QUERY_KHR                                                  0x82E3
#define GL_QUERY_NO_WAIT                                              0x8E14
#define GL_QUERY_NO_WAIT_INVERTED                                     0x8E18
#define GL_QUERY_NO_WAIT_NV                                           0x8E14
#define GL_QUERY_OBJECT_AMD                                           0x9153
#define GL_QUERY_OBJECT_EXT                                           0x9153
#define GL_QUERY_RESOURCE_BUFFEROBJECT_NV                             0x9547
#define GL_QUERY_RESOURCE_MEMTYPE_VIDMEM_NV                           0x9542
#define GL_QUERY_RESOURCE_RENDERBUFFER_NV                             0x9546
#define GL_QUERY_RESOURCE_SYS_RESERVED_NV                             0x9544
#define GL_QUERY_RESOURCE_TEXTURE_NV                                  0x9545
#define GL_QUERY_RESOURCE_TYPE_VIDMEM_ALLOC_NV                        0x9540
#define GL_QUERY_RESULT                                               0x8866
#define GL_QUERY_RESULT_ARB                                           0x8866
#define GL_QUERY_RESULT_AVAILABLE                                     0x8867
#define GL_QUERY_RESULT_AVAILABLE_ARB                                 0x8867
#define GL_QUERY_RESULT_NO_WAIT                                       0x9194
#define GL_QUERY_RESULT_NO_WAIT_AMD                                   0x9194
#define GL_QUERY_STENCIL_FAIL_EVENT_BIT_AMD                           0x00000004
#define GL_QUERY_TARGET                                               0x82EA
#define GL_QUERY_WAIT                                                 0x8E13
#define GL_QUERY_WAIT_INVERTED                                        0x8E17
#define GL_QUERY_WAIT_NV                                              0x8E13
#define GL_R                                                          0x2002
#define GL_R11F_G11F_B10F                                             0x8C3A
#define GL_R11F_G11F_B10F_EXT                                         0x8C3A
#define GL_R16                                                        0x822A
#define GL_R16F                                                       0x822D
#define GL_R16I                                                       0x8233
#define GL_R16UI                                                      0x8234
#define GL_R16_SNORM                                                  0x8F98
#define GL_R1UI_C3F_V3F_SUN                                           0x85C6
#define GL_R1UI_C4F_N3F_V3F_SUN                                       0x85C8
#define GL_R1UI_C4UB_V3F_SUN                                          0x85C5
#define GL_R1UI_N3F_V3F_SUN                                           0x85C7
#define GL_R1UI_T2F_C4F_N3F_V3F_SUN                                   0x85CB
#define GL_R1UI_T2F_N3F_V3F_SUN                                       0x85CA
#define GL_R1UI_T2F_V3F_SUN                                           0x85C9
#define GL_R1UI_V3F_SUN                                               0x85C4
#define GL_R32F                                                       0x822E
#define GL_R32I                                                       0x8235
#define GL_R32UI                                                      0x8236
#define GL_R3_G3_B2                                                   0x2A10
#define GL_R8                                                         0x8229
#define GL_R8I                                                        0x8231
#define GL_R8UI                                                       0x8232
#define GL_R8_SNORM                                                   0x8F94
#define GL_RASTERIZER_DISCARD                                         0x8C89
#define GL_RASTERIZER_DISCARD_EXT                                     0x8C89
#define GL_RASTERIZER_DISCARD_NV                                      0x8C89
#define GL_RASTER_FIXED_SAMPLE_LOCATIONS_EXT                          0x932A
#define GL_RASTER_MULTISAMPLE_EXT                                     0x9327
#define GL_RASTER_POSITION_UNCLIPPED_IBM                              0x19262
#define GL_RASTER_SAMPLES_EXT                                         0x9328
#define GL_READ_BUFFER                                                0x0C02
#define GL_READ_FRAMEBUFFER                                           0x8CA8
#define GL_READ_FRAMEBUFFER_BINDING                                   0x8CAA
#define GL_READ_FRAMEBUFFER_BINDING_EXT                               0x8CAA
#define GL_READ_FRAMEBUFFER_EXT                                       0x8CA8
#define GL_READ_ONLY                                                  0x88B8
#define GL_READ_ONLY_ARB                                              0x88B8
#define GL_READ_PIXELS                                                0x828C
#define GL_READ_PIXELS_FORMAT                                         0x828D
#define GL_READ_PIXELS_TYPE                                           0x828E
#define GL_READ_PIXEL_DATA_RANGE_LENGTH_NV                            0x887B
#define GL_READ_PIXEL_DATA_RANGE_NV                                   0x8879
#define GL_READ_PIXEL_DATA_RANGE_POINTER_NV                           0x887D
#define GL_READ_WRITE                                                 0x88BA
#define GL_READ_WRITE_ARB                                             0x88BA
#define GL_RECLAIM_MEMORY_HINT_PGI                                    0x1A1FE
#define GL_RECT_NV                                                    0xF6
#define GL_RED                                                        0x1903
#define GL_REDUCE                                                     0x8016
#define GL_REDUCE_EXT                                                 0x8016
#define GL_RED_BIAS                                                   0x0D15
#define GL_RED_BITS                                                   0x0D52
#define GL_RED_BIT_ATI                                                0x00000001
#define GL_RED_INTEGER                                                0x8D94
#define GL_RED_INTEGER_EXT                                            0x8D94
#define GL_RED_MAX_CLAMP_INGR                                         0x8564
#define GL_RED_MIN_CLAMP_INGR                                         0x8560
#define GL_RED_NV                                                     0x1903
#define GL_RED_SCALE                                                  0x0D14
#define GL_RED_SNORM                                                  0x8F90
#define GL_REFERENCED_BY_COMPUTE_SHADER                               0x930B
#define GL_REFERENCED_BY_FRAGMENT_SHADER                              0x930A
#define GL_REFERENCED_BY_GEOMETRY_SHADER                              0x9309
#define GL_REFERENCED_BY_MESH_SHADER_NV                               0x95A0
#define GL_REFERENCED_BY_TASK_SHADER_NV                               0x95A1
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER                          0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER                       0x9308
#define GL_REFERENCED_BY_VERTEX_SHADER                                0x9306
#define GL_REFERENCE_PLANE_EQUATION_SGIX                              0x817E
#define GL_REFERENCE_PLANE_SGIX                                       0x817D
#define GL_REFLECTION_MAP                                             0x8512
#define GL_REFLECTION_MAP_ARB                                         0x8512
#define GL_REFLECTION_MAP_EXT                                         0x8512
#define GL_REFLECTION_MAP_NV                                          0x8512
#define GL_REGISTER_COMBINERS_NV                                      0x8522
#define GL_REG_0_ATI                                                  0x8921
#define GL_REG_10_ATI                                                 0x892B
#define GL_REG_11_ATI                                                 0x892C
#define GL_REG_12_ATI                                                 0x892D
#define GL_REG_13_ATI                                                 0x892E
#define GL_REG_14_ATI                                                 0x892F
#define GL_REG_15_ATI                                                 0x8930
#define GL_REG_16_ATI                                                 0x8931
#define GL_REG_17_ATI                                                 0x8932
#define GL_REG_18_ATI                                                 0x8933
#define GL_REG_19_ATI                                                 0x8934
#define GL_REG_1_ATI                                                  0x8922
#define GL_REG_20_ATI                                                 0x8935
#define GL_REG_21_ATI                                                 0x8936
#define GL_REG_22_ATI                                                 0x8937
#define GL_REG_23_ATI                                                 0x8938
#define GL_REG_24_ATI                                                 0x8939
#define GL_REG_25_ATI                                                 0x893A
#define GL_REG_26_ATI                                                 0x893B
#define GL_REG_27_ATI                                                 0x893C
#define GL_REG_28_ATI                                                 0x893D
#define GL_REG_29_ATI                                                 0x893E
#define GL_REG_2_ATI                                                  0x8923
#define GL_REG_30_ATI                                                 0x893F
#define GL_REG_31_ATI                                                 0x8940
#define GL_REG_3_ATI                                                  0x8924
#define GL_REG_4_ATI                                                  0x8925
#define GL_REG_5_ATI                                                  0x8926
#define GL_REG_6_ATI                                                  0x8927
#define GL_REG_7_ATI                                                  0x8928
#define GL_REG_8_ATI                                                  0x8929
#define GL_REG_9_ATI                                                  0x892A
#define GL_RELATIVE_ARC_TO_NV                                         0xFF
#define GL_RELATIVE_CONIC_CURVE_TO_NV                                 0x1B
#define GL_RELATIVE_CUBIC_CURVE_TO_NV                                 0x0D
#define GL_RELATIVE_HORIZONTAL_LINE_TO_NV                             0x07
#define GL_RELATIVE_LARGE_CCW_ARC_TO_NV                               0x17
#define GL_RELATIVE_LARGE_CW_ARC_TO_NV                                0x19
#define GL_RELATIVE_LINE_TO_NV                                        0x05
#define GL_RELATIVE_MOVE_TO_NV                                        0x03
#define GL_RELATIVE_QUADRATIC_CURVE_TO_NV                             0x0B
#define GL_RELATIVE_RECT_NV                                           0xF7
#define GL_RELATIVE_ROUNDED_RECT2_NV                                  0xEB
#define GL_RELATIVE_ROUNDED_RECT4_NV                                  0xED
#define GL_RELATIVE_ROUNDED_RECT8_NV                                  0xEF
#define GL_RELATIVE_ROUNDED_RECT_NV                                   0xE9
#define GL_RELATIVE_SMALL_CCW_ARC_TO_NV                               0x13
#define GL_RELATIVE_SMALL_CW_ARC_TO_NV                                0x15
#define GL_RELATIVE_SMOOTH_CUBIC_CURVE_TO_NV                          0x11
#define GL_RELATIVE_SMOOTH_QUADRATIC_CURVE_TO_NV                      0x0F
#define GL_RELATIVE_VERTICAL_LINE_TO_NV                               0x09
#define GL_RELEASED_APPLE                                             0x8A19
#define GL_RENDER                                                     0x1C00
#define GL_RENDERBUFFER                                               0x8D41
#define GL_RENDERBUFFER_ALPHA_SIZE                                    0x8D53
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT                                0x8D53
#define GL_RENDERBUFFER_BINDING                                       0x8CA7
#define GL_RENDERBUFFER_BINDING_EXT                                   0x8CA7
#define GL_RENDERBUFFER_BLUE_SIZE                                     0x8D52
#define GL_RENDERBUFFER_BLUE_SIZE_EXT                                 0x8D52
#define GL_RENDERBUFFER_COLOR_SAMPLES_NV                              0x8E10
#define GL_RENDERBUFFER_COVERAGE_SAMPLES_NV                           0x8CAB
#define GL_RENDERBUFFER_DEPTH_SIZE                                    0x8D54
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT                                0x8D54
#define GL_RENDERBUFFER_EXT                                           0x8D41
#define GL_RENDERBUFFER_FREE_MEMORY_ATI                               0x87FD
#define GL_RENDERBUFFER_GREEN_SIZE                                    0x8D51
#define GL_RENDERBUFFER_GREEN_SIZE_EXT                                0x8D51
#define GL_RENDERBUFFER_HEIGHT                                        0x8D43
#define GL_RENDERBUFFER_HEIGHT_EXT                                    0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT                               0x8D44
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT                           0x8D44
#define GL_RENDERBUFFER_RED_SIZE                                      0x8D50
#define GL_RENDERBUFFER_RED_SIZE_EXT                                  0x8D50
#define GL_RENDERBUFFER_SAMPLES                                       0x8CAB
#define GL_RENDERBUFFER_SAMPLES_EXT                                   0x8CAB
#define GL_RENDERBUFFER_STENCIL_SIZE                                  0x8D55
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT                              0x8D55
#define GL_RENDERBUFFER_STORAGE_SAMPLES_AMD                           0x91B2
#define GL_RENDERBUFFER_WIDTH                                         0x8D42
#define GL_RENDERBUFFER_WIDTH_EXT                                     0x8D42
#define GL_RENDERER                                                   0x1F01
#define GL_RENDER_GPU_MASK_NV                                         0x9558
#define GL_RENDER_MODE                                                0x0C40
#define GL_REPEAT                                                     0x2901
#define GL_REPLACE                                                    0x1E01
#define GL_REPLACEMENT_CODE_ARRAY_POINTER_SUN                         0x85C3
#define GL_REPLACEMENT_CODE_ARRAY_STRIDE_SUN                          0x85C2
#define GL_REPLACEMENT_CODE_ARRAY_SUN                                 0x85C0
#define GL_REPLACEMENT_CODE_ARRAY_TYPE_SUN                            0x85C1
#define GL_REPLACEMENT_CODE_SUN                                       0x81D8
#define GL_REPLACE_EXT                                                0x8062
#define GL_REPLACE_MIDDLE_SUN                                         0x0002
#define GL_REPLACE_OLDEST_SUN                                         0x0003
#define GL_REPLACE_VALUE_AMD                                          0x874B
#define GL_REPLICATE_BORDER                                           0x8153
#define GL_REPLICATE_BORDER_HP                                        0x8153
#define GL_REPRESENTATIVE_FRAGMENT_TEST_NV                            0x937F
#define GL_RESAMPLE_AVERAGE_OML                                       0x8988
#define GL_RESAMPLE_DECIMATE_OML                                      0x8989
#define GL_RESAMPLE_DECIMATE_SGIX                                     0x8430
#define GL_RESAMPLE_REPLICATE_OML                                     0x8986
#define GL_RESAMPLE_REPLICATE_SGIX                                    0x8433
#define GL_RESAMPLE_ZERO_FILL_OML                                     0x8987
#define GL_RESAMPLE_ZERO_FILL_SGIX                                    0x8434
#define GL_RESCALE_NORMAL                                             0x803A
#define GL_RESCALE_NORMAL_EXT                                         0x803A
#define GL_RESET_NOTIFICATION_STRATEGY                                0x8256
#define GL_RESET_NOTIFICATION_STRATEGY_ARB                            0x8256
#define GL_RESET_NOTIFICATION_STRATEGY_KHR                            0x8256
#define GL_RESTART_PATH_NV                                            0xF0
#define GL_RESTART_SUN                                                0x0001
#define GL_RETAINED_APPLE                                             0x8A1B
#define GL_RETURN                                                     0x0102
#define GL_RG                                                         0x8227
#define GL_RG16                                                       0x822C
#define GL_RG16F                                                      0x822F
#define GL_RG16I                                                      0x8239
#define GL_RG16UI                                                     0x823A
#define GL_RG16_SNORM                                                 0x8F99
#define GL_RG32F                                                      0x8230
#define GL_RG32I                                                      0x823B
#define GL_RG32UI                                                     0x823C
#define GL_RG8                                                        0x822B
#define GL_RG8I                                                       0x8237
#define GL_RG8UI                                                      0x8238
#define GL_RG8_SNORM                                                  0x8F95
#define GL_RGB                                                        0x1907
#define GL_RGB10                                                      0x8052
#define GL_RGB10_A2                                                   0x8059
#define GL_RGB10_A2UI                                                 0x906F
#define GL_RGB10_A2_EXT                                               0x8059
#define GL_RGB10_EXT                                                  0x8052
#define GL_RGB12                                                      0x8053
#define GL_RGB12_EXT                                                  0x8053
#define GL_RGB16                                                      0x8054
#define GL_RGB16F                                                     0x881B
#define GL_RGB16F_ARB                                                 0x881B
#define GL_RGB16I                                                     0x8D89
#define GL_RGB16I_EXT                                                 0x8D89
#define GL_RGB16UI                                                    0x8D77
#define GL_RGB16UI_EXT                                                0x8D77
#define GL_RGB16_EXT                                                  0x8054
#define GL_RGB16_SNORM                                                0x8F9A
#define GL_RGB2_EXT                                                   0x804E
#define GL_RGB32F                                                     0x8815
#define GL_RGB32F_ARB                                                 0x8815
#define GL_RGB32I                                                     0x8D83
#define GL_RGB32I_EXT                                                 0x8D83
#define GL_RGB32UI                                                    0x8D71
#define GL_RGB32UI_EXT                                                0x8D71
#define GL_RGB4                                                       0x804F
#define GL_RGB4_EXT                                                   0x804F
#define GL_RGB4_S3TC                                                  0x83A1
#define GL_RGB5                                                       0x8050
#define GL_RGB565                                                     0x8D62
#define GL_RGB5_A1                                                    0x8057
#define GL_RGB5_A1_EXT                                                0x8057
#define GL_RGB5_EXT                                                   0x8050
#define GL_RGB8                                                       0x8051
#define GL_RGB8I                                                      0x8D8F
#define GL_RGB8I_EXT                                                  0x8D8F
#define GL_RGB8UI                                                     0x8D7D
#define GL_RGB8UI_EXT                                                 0x8D7D
#define GL_RGB8_EXT                                                   0x8051
#define GL_RGB8_SNORM                                                 0x8F96
#define GL_RGB9_E5                                                    0x8C3D
#define GL_RGB9_E5_EXT                                                0x8C3D
#define GL_RGBA                                                       0x1908
#define GL_RGBA12                                                     0x805A
#define GL_RGBA12_EXT                                                 0x805A
#define GL_RGBA16                                                     0x805B
#define GL_RGBA16F                                                    0x881A
#define GL_RGBA16F_ARB                                                0x881A
#define GL_RGBA16I                                                    0x8D88
#define GL_RGBA16I_EXT                                                0x8D88
#define GL_RGBA16UI                                                   0x8D76
#define GL_RGBA16UI_EXT                                               0x8D76
#define GL_RGBA16_EXT                                                 0x805B
#define GL_RGBA16_SNORM                                               0x8F9B
#define GL_RGBA2                                                      0x8055
#define GL_RGBA2_EXT                                                  0x8055
#define GL_RGBA32F                                                    0x8814
#define GL_RGBA32F_ARB                                                0x8814
#define GL_RGBA32I                                                    0x8D82
#define GL_RGBA32I_EXT                                                0x8D82
#define GL_RGBA32UI                                                   0x8D70
#define GL_RGBA32UI_EXT                                               0x8D70
#define GL_RGBA4                                                      0x8056
#define GL_RGBA4_DXT5_S3TC                                            0x83A5
#define GL_RGBA4_EXT                                                  0x8056
#define GL_RGBA4_S3TC                                                 0x83A3
#define GL_RGBA8                                                      0x8058
#define GL_RGBA8I                                                     0x8D8E
#define GL_RGBA8I_EXT                                                 0x8D8E
#define GL_RGBA8UI                                                    0x8D7C
#define GL_RGBA8UI_EXT                                                0x8D7C
#define GL_RGBA8_EXT                                                  0x8058
#define GL_RGBA8_SNORM                                                0x8F97
#define GL_RGBA_DXT5_S3TC                                             0x83A4
#define GL_RGBA_FLOAT16_APPLE                                         0x881A
#define GL_RGBA_FLOAT16_ATI                                           0x881A
#define GL_RGBA_FLOAT32_APPLE                                         0x8814
#define GL_RGBA_FLOAT32_ATI                                           0x8814
#define GL_RGBA_FLOAT_MODE_ARB                                        0x8820
#define GL_RGBA_FLOAT_MODE_ATI                                        0x8820
#define GL_RGBA_INTEGER                                               0x8D99
#define GL_RGBA_INTEGER_EXT                                           0x8D99
#define GL_RGBA_INTEGER_MODE_EXT                                      0x8D9E
#define GL_RGBA_MODE                                                  0x0C31
#define GL_RGBA_S3TC                                                  0x83A2
#define GL_RGBA_SIGNED_COMPONENTS_EXT                                 0x8C3C
#define GL_RGBA_SNORM                                                 0x8F93
#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV                       0x86D9
#define GL_RGB_422_APPLE                                              0x8A1F
#define GL_RGB_FLOAT16_APPLE                                          0x881B
#define GL_RGB_FLOAT16_ATI                                            0x881B
#define GL_RGB_FLOAT32_APPLE                                          0x8815
#define GL_RGB_FLOAT32_ATI                                            0x8815
#define GL_RGB_INTEGER                                                0x8D98
#define GL_RGB_INTEGER_EXT                                            0x8D98
#define GL_RGB_RAW_422_APPLE                                          0x8A51
#define GL_RGB_S3TC                                                   0x83A0
#define GL_RGB_SCALE                                                  0x8573
#define GL_RGB_SCALE_ARB                                              0x8573
#define GL_RGB_SCALE_EXT                                              0x8573
#define GL_RGB_SNORM                                                  0x8F92
#define GL_RG_INTEGER                                                 0x8228
#define GL_RG_SNORM                                                   0x8F91
#define GL_RIGHT                                                      0x0407
#define GL_ROUNDED_RECT2_NV                                           0xEA
#define GL_ROUNDED_RECT4_NV                                           0xEC
#define GL_ROUNDED_RECT8_NV                                           0xEE
#define GL_ROUNDED_RECT_NV                                            0xE8
#define GL_ROUND_NV                                                   0x90A4
#define GL_S                                                          0x2000
#define GL_SAMPLER                                                    0x82E6
#define GL_SAMPLER_1D                                                 0x8B5D
#define GL_SAMPLER_1D_ARB                                             0x8B5D
#define GL_SAMPLER_1D_ARRAY                                           0x8DC0
#define GL_SAMPLER_1D_ARRAY_EXT                                       0x8DC0
#define GL_SAMPLER_1D_ARRAY_SHADOW                                    0x8DC3
#define GL_SAMPLER_1D_ARRAY_SHADOW_EXT                                0x8DC3
#define GL_SAMPLER_1D_SHADOW                                          0x8B61
#define GL_SAMPLER_1D_SHADOW_ARB                                      0x8B61
#define GL_SAMPLER_2D                                                 0x8B5E
#define GL_SAMPLER_2D_ARB                                             0x8B5E
#define GL_SAMPLER_2D_ARRAY                                           0x8DC1
#define GL_SAMPLER_2D_ARRAY_EXT                                       0x8DC1
#define GL_SAMPLER_2D_ARRAY_SHADOW                                    0x8DC4
#define GL_SAMPLER_2D_ARRAY_SHADOW_EXT                                0x8DC4
#define GL_SAMPLER_2D_MULTISAMPLE                                     0x9108
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY                               0x910B
#define GL_SAMPLER_2D_RECT                                            0x8B63
#define GL_SAMPLER_2D_RECT_ARB                                        0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW                                     0x8B64
#define GL_SAMPLER_2D_RECT_SHADOW_ARB                                 0x8B64
#define GL_SAMPLER_2D_SHADOW                                          0x8B62
#define GL_SAMPLER_2D_SHADOW_ARB                                      0x8B62
#define GL_SAMPLER_3D                                                 0x8B5F
#define GL_SAMPLER_3D_ARB                                             0x8B5F
#define GL_SAMPLER_BINDING                                            0x8919
#define GL_SAMPLER_BUFFER                                             0x8DC2
#define GL_SAMPLER_BUFFER_AMD                                         0x9001
#define GL_SAMPLER_BUFFER_EXT                                         0x8DC2
#define GL_SAMPLER_CUBE                                               0x8B60
#define GL_SAMPLER_CUBE_ARB                                           0x8B60
#define GL_SAMPLER_CUBE_MAP_ARRAY                                     0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_ARB                                 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW                              0x900D
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_ARB                          0x900D
#define GL_SAMPLER_CUBE_SHADOW                                        0x8DC5
#define GL_SAMPLER_CUBE_SHADOW_EXT                                    0x8DC5
#define GL_SAMPLER_KHR                                                0x82E6
#define GL_SAMPLER_OBJECT_AMD                                         0x9155
#define GL_SAMPLER_RENDERBUFFER_NV                                    0x8E56
#define GL_SAMPLES                                                    0x80A9
#define GL_SAMPLES_3DFX                                               0x86B4
#define GL_SAMPLES_ARB                                                0x80A9
#define GL_SAMPLES_EXT                                                0x80A9
#define GL_SAMPLES_PASSED                                             0x8914
#define GL_SAMPLES_PASSED_ARB                                         0x8914
#define GL_SAMPLES_SGIS                                               0x80A9
#define GL_SAMPLE_ALPHA_TO_COVERAGE                                   0x809E
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB                               0x809E
#define GL_SAMPLE_ALPHA_TO_MASK_EXT                                   0x809E
#define GL_SAMPLE_ALPHA_TO_MASK_SGIS                                  0x809E
#define GL_SAMPLE_ALPHA_TO_ONE                                        0x809F
#define GL_SAMPLE_ALPHA_TO_ONE_ARB                                    0x809F
#define GL_SAMPLE_ALPHA_TO_ONE_EXT                                    0x809F
#define GL_SAMPLE_ALPHA_TO_ONE_SGIS                                   0x809F
#define GL_SAMPLE_BUFFERS                                             0x80A8
#define GL_SAMPLE_BUFFERS_3DFX                                        0x86B3
#define GL_SAMPLE_BUFFERS_ARB                                         0x80A8
#define GL_SAMPLE_BUFFERS_EXT                                         0x80A8
#define GL_SAMPLE_BUFFERS_SGIS                                        0x80A8
#define GL_SAMPLE_COVERAGE                                            0x80A0
#define GL_SAMPLE_COVERAGE_ARB                                        0x80A0
#define GL_SAMPLE_COVERAGE_INVERT                                     0x80AB
#define GL_SAMPLE_COVERAGE_INVERT_ARB                                 0x80AB
#define GL_SAMPLE_COVERAGE_VALUE                                      0x80AA
#define GL_SAMPLE_COVERAGE_VALUE_ARB                                  0x80AA
#define GL_SAMPLE_LOCATION_ARB                                        0x8E50
#define GL_SAMPLE_LOCATION_NV                                         0x8E50
#define GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_ARB                      0x933F
#define GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_NV                       0x933F
#define GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_ARB                       0x933E
#define GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_NV                        0x933E
#define GL_SAMPLE_LOCATION_SUBPIXEL_BITS_ARB                          0x933D
#define GL_SAMPLE_LOCATION_SUBPIXEL_BITS_NV                           0x933D
#define GL_SAMPLE_MASK                                                0x8E51
#define GL_SAMPLE_MASK_EXT                                            0x80A0
#define GL_SAMPLE_MASK_INVERT_EXT                                     0x80AB
#define GL_SAMPLE_MASK_INVERT_SGIS                                    0x80AB
#define GL_SAMPLE_MASK_NV                                             0x8E51
#define GL_SAMPLE_MASK_SGIS                                           0x80A0
#define GL_SAMPLE_MASK_VALUE                                          0x8E52
#define GL_SAMPLE_MASK_VALUE_EXT                                      0x80AA
#define GL_SAMPLE_MASK_VALUE_NV                                       0x8E52
#define GL_SAMPLE_MASK_VALUE_SGIS                                     0x80AA
#define GL_SAMPLE_PATTERN_EXT                                         0x80AC
#define GL_SAMPLE_PATTERN_SGIS                                        0x80AC
#define GL_SAMPLE_POSITION                                            0x8E50
#define GL_SAMPLE_POSITION_NV                                         0x8E50
#define GL_SAMPLE_SHADING                                             0x8C36
#define GL_SAMPLE_SHADING_ARB                                         0x8C36
#define GL_SATURATE_BIT_ATI                                           0x00000040
#define GL_SCALAR_EXT                                                 0x87BE
#define GL_SCALEBIAS_HINT_SGIX                                        0x8322
#define GL_SCALED_RESOLVE_FASTEST_EXT                                 0x90BA
#define GL_SCALED_RESOLVE_NICEST_EXT                                  0x90BB
#define GL_SCALE_BY_FOUR_NV                                           0x853F
#define GL_SCALE_BY_ONE_HALF_NV                                       0x8540
#define GL_SCALE_BY_TWO_NV                                            0x853E
#define GL_SCISSOR_BIT                                                0x00080000
#define GL_SCISSOR_BOX                                                0x0C10
#define GL_SCISSOR_BOX_EXCLUSIVE_NV                                   0x9556
#define GL_SCISSOR_COMMAND_NV                                         0x0011
#define GL_SCISSOR_TEST                                               0x0C11
#define GL_SCISSOR_TEST_EXCLUSIVE_NV                                  0x9555
#define GL_SCREEN_COORDINATES_REND                                    0x8490
#define GL_SCREEN_KHR                                                 0x9295
#define GL_SCREEN_NV                                                  0x9295
#define GL_SECONDARY_COLOR_ARRAY                                      0x845E
#define GL_SECONDARY_COLOR_ARRAY_ADDRESS_NV                           0x8F27
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING                       0x889C
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB                   0x889C
#define GL_SECONDARY_COLOR_ARRAY_EXT                                  0x845E
#define GL_SECONDARY_COLOR_ARRAY_LENGTH_NV                            0x8F31
#define GL_SECONDARY_COLOR_ARRAY_LIST_IBM                             103077
#define GL_SECONDARY_COLOR_ARRAY_LIST_STRIDE_IBM                      103087
#define GL_SECONDARY_COLOR_ARRAY_POINTER                              0x845D
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT                          0x845D
#define GL_SECONDARY_COLOR_ARRAY_SIZE                                 0x845A
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT                             0x845A
#define GL_SECONDARY_COLOR_ARRAY_STRIDE                               0x845C
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT                           0x845C
#define GL_SECONDARY_COLOR_ARRAY_TYPE                                 0x845B
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT                             0x845B
#define GL_SECONDARY_COLOR_NV                                         0x852D
#define GL_SECONDARY_INTERPOLATOR_ATI                                 0x896D
#define GL_SELECT                                                     0x1C02
#define GL_SELECTION_BUFFER_POINTER                                   0x0DF3
#define GL_SELECTION_BUFFER_SIZE                                      0x0DF4
#define GL_SEPARABLE_2D                                               0x8012
#define GL_SEPARABLE_2D_EXT                                           0x8012
#define GL_SEPARATE_ATTRIBS                                           0x8C8D
#define GL_SEPARATE_ATTRIBS_EXT                                       0x8C8D
#define GL_SEPARATE_ATTRIBS_NV                                        0x8C8D
#define GL_SEPARATE_SPECULAR_COLOR                                    0x81FA
#define GL_SEPARATE_SPECULAR_COLOR_EXT                                0x81FA
#define GL_SET                                                        0x150F
#define GL_SET_AMD                                                    0x874A
#define GL_SHADER                                                     0x82E1
#define GL_SHADER_BINARY_FORMATS                                      0x8DF8
#define GL_SHADER_BINARY_FORMAT_SPIR_V                                0x9551
#define GL_SHADER_BINARY_FORMAT_SPIR_V_ARB                            0x9551
#define GL_SHADER_COMPILER                                            0x8DFA
#define GL_SHADER_CONSISTENT_NV                                       0x86DD
#define GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV                        0x00000010
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT                            0x00000020
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT_EXT                        0x00000020
#define GL_SHADER_IMAGE_ATOMIC                                        0x82A6
#define GL_SHADER_IMAGE_LOAD                                          0x82A4
#define GL_SHADER_IMAGE_STORE                                         0x82A5
#define GL_SHADER_INCLUDE_ARB                                         0x8DAE
#define GL_SHADER_KHR                                                 0x82E1
#define GL_SHADER_OBJECT_ARB                                          0x8B48
#define GL_SHADER_OBJECT_EXT                                          0x8B48
#define GL_SHADER_OPERATION_NV                                        0x86DF
#define GL_SHADER_SOURCE_LENGTH                                       0x8B88
#define GL_SHADER_STORAGE_BARRIER_BIT                                 0x00002000
#define GL_SHADER_STORAGE_BLOCK                                       0x92E6
#define GL_SHADER_STORAGE_BUFFER                                      0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING                              0x90D3
#define GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT                     0x90DF
#define GL_SHADER_STORAGE_BUFFER_SIZE                                 0x90D5
#define GL_SHADER_STORAGE_BUFFER_START                                0x90D4
#define GL_SHADER_TYPE                                                0x8B4F
#define GL_SHADE_MODEL                                                0x0B54
#define GL_SHADING_LANGUAGE_VERSION                                   0x8B8C
#define GL_SHADING_LANGUAGE_VERSION_ARB                               0x8B8C
#define GL_SHADING_RATE_16_INVOCATIONS_PER_PIXEL_NV                   0x956F
#define GL_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV                0x9566
#define GL_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV                0x9567
#define GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV                0x9568
#define GL_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV                0x9569
#define GL_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV                0x956A
#define GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV                0x956B
#define GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV                     0x9565
#define GL_SHADING_RATE_2_INVOCATIONS_PER_PIXEL_NV                    0x956C
#define GL_SHADING_RATE_4_INVOCATIONS_PER_PIXEL_NV                    0x956D
#define GL_SHADING_RATE_8_INVOCATIONS_PER_PIXEL_NV                    0x956E
#define GL_SHADING_RATE_IMAGE_BINDING_NV                              0x955B
#define GL_SHADING_RATE_IMAGE_NV                                      0x9563
#define GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV                         0x955E
#define GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV                         0x955D
#define GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV                          0x955C
#define GL_SHADING_RATE_NO_INVOCATIONS_NV                             0x9564
#define GL_SHADING_RATE_SAMPLE_ORDER_DEFAULT_NV                       0x95AE
#define GL_SHADING_RATE_SAMPLE_ORDER_PIXEL_MAJOR_NV                   0x95AF
#define GL_SHADING_RATE_SAMPLE_ORDER_SAMPLE_MAJOR_NV                  0x95B0
#define GL_SHADOW_AMBIENT_SGIX                                        0x80BF
#define GL_SHADOW_ATTENUATION_EXT                                     0x834E
#define GL_SHARED_EDGE_NV                                             0xC0
#define GL_SHARED_TEXTURE_PALETTE_EXT                                 0x81FB
#define GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS                           0x80B0
#define GL_SHININESS                                                  0x1601
#define GL_SHORT                                                      0x1402
#define GL_SIGNALED                                                   0x9119
#define GL_SIGNED_ALPHA8_NV                                           0x8706
#define GL_SIGNED_ALPHA_NV                                            0x8705
#define GL_SIGNED_HILO16_NV                                           0x86FA
#define GL_SIGNED_HILO8_NV                                            0x885F
#define GL_SIGNED_HILO_NV                                             0x86F9
#define GL_SIGNED_IDENTITY_NV                                         0x853C
#define GL_SIGNED_INTENSITY8_NV                                       0x8708
#define GL_SIGNED_INTENSITY_NV                                        0x8707
#define GL_SIGNED_LUMINANCE8_ALPHA8_NV                                0x8704
#define GL_SIGNED_LUMINANCE8_NV                                       0x8702
#define GL_SIGNED_LUMINANCE_ALPHA_NV                                  0x8703
#define GL_SIGNED_LUMINANCE_NV                                        0x8701
#define GL_SIGNED_NEGATE_NV                                           0x853D
#define GL_SIGNED_NORMALIZED                                          0x8F9C
#define GL_SIGNED_RGB8_NV                                             0x86FF
#define GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV                             0x870D
#define GL_SIGNED_RGBA8_NV                                            0x86FC
#define GL_SIGNED_RGBA_NV                                             0x86FB
#define GL_SIGNED_RGB_NV                                              0x86FE
#define GL_SIGNED_RGB_UNSIGNED_ALPHA_NV                               0x870C
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST                        0x82AC
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE                       0x82AE
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST                      0x82AD
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE                     0x82AF
#define GL_SINGLE_COLOR                                               0x81F9
#define GL_SINGLE_COLOR_EXT                                           0x81F9
#define GL_SKIP_COMPONENTS1_NV                                        -6
#define GL_SKIP_COMPONENTS2_NV                                        -5
#define GL_SKIP_COMPONENTS3_NV                                        -4
#define GL_SKIP_COMPONENTS4_NV                                        -3
#define GL_SKIP_DECODE_EXT                                            0x8A4A
#define GL_SKIP_MISSING_GLYPH_NV                                      0x90A9
#define GL_SLICE_ACCUM_SUN                                            0x85CC
#define GL_SLUMINANCE                                                 0x8C46
#define GL_SLUMINANCE8                                                0x8C47
#define GL_SLUMINANCE8_ALPHA8                                         0x8C45
#define GL_SLUMINANCE8_ALPHA8_EXT                                     0x8C45
#define GL_SLUMINANCE8_EXT                                            0x8C47
#define GL_SLUMINANCE_ALPHA                                           0x8C44
#define GL_SLUMINANCE_ALPHA_EXT                                       0x8C44
#define GL_SLUMINANCE_EXT                                             0x8C46
#define GL_SMALL_CCW_ARC_TO_NV                                        0x12
#define GL_SMALL_CW_ARC_TO_NV                                         0x14
#define GL_SMOOTH                                                     0x1D01
#define GL_SMOOTH_CUBIC_CURVE_TO_NV                                   0x10
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY                              0x0B23
#define GL_SMOOTH_LINE_WIDTH_RANGE                                    0x0B22
#define GL_SMOOTH_POINT_SIZE_GRANULARITY                              0x0B13
#define GL_SMOOTH_POINT_SIZE_RANGE                                    0x0B12
#define GL_SMOOTH_QUADRATIC_CURVE_TO_NV                               0x0E
#define GL_SM_COUNT_NV                                                0x933B
#define GL_SOFTLIGHT_KHR                                              0x929C
#define GL_SOFTLIGHT_NV                                               0x929C
#define GL_SOURCE0_ALPHA                                              0x8588
#define GL_SOURCE0_ALPHA_ARB                                          0x8588
#define GL_SOURCE0_ALPHA_EXT                                          0x8588
#define GL_SOURCE0_RGB                                                0x8580
#define GL_SOURCE0_RGB_ARB                                            0x8580
#define GL_SOURCE0_RGB_EXT                                            0x8580
#define GL_SOURCE1_ALPHA                                              0x8589
#define GL_SOURCE1_ALPHA_ARB                                          0x8589
#define GL_SOURCE1_ALPHA_EXT                                          0x8589
#define GL_SOURCE1_RGB                                                0x8581
#define GL_SOURCE1_RGB_ARB                                            0x8581
#define GL_SOURCE1_RGB_EXT                                            0x8581
#define GL_SOURCE2_ALPHA                                              0x858A
#define GL_SOURCE2_ALPHA_ARB                                          0x858A
#define GL_SOURCE2_ALPHA_EXT                                          0x858A
#define GL_SOURCE2_RGB                                                0x8582
#define GL_SOURCE2_RGB_ARB                                            0x8582
#define GL_SOURCE2_RGB_EXT                                            0x8582
#define GL_SOURCE3_ALPHA_NV                                           0x858B
#define GL_SOURCE3_RGB_NV                                             0x8583
#define GL_SPARE0_NV                                                  0x852E
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV                             0x8532
#define GL_SPARE1_NV                                                  0x852F
#define GL_SPARSE_BUFFER_PAGE_SIZE_ARB                                0x82F8
#define GL_SPARSE_STORAGE_BIT_ARB                                     0x0400
#define GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB                 0x91A9
#define GL_SPECULAR                                                   0x1202
#define GL_SPHERE_MAP                                                 0x2402
#define GL_SPIR_V_BINARY                                              0x9552
#define GL_SPIR_V_BINARY_ARB                                          0x9552
#define GL_SPIR_V_EXTENSIONS                                          0x9553
#define GL_SPOT_CUTOFF                                                0x1206
#define GL_SPOT_DIRECTION                                             0x1204
#define GL_SPOT_EXPONENT                                              0x1205
#define GL_SPRITE_AXIAL_SGIX                                          0x814C
#define GL_SPRITE_AXIS_SGIX                                           0x814A
#define GL_SPRITE_EYE_ALIGNED_SGIX                                    0x814E
#define GL_SPRITE_MODE_SGIX                                           0x8149
#define GL_SPRITE_OBJECT_ALIGNED_SGIX                                 0x814D
#define GL_SPRITE_SGIX                                                0x8148
#define GL_SPRITE_TRANSLATION_SGIX                                    0x814B
#define GL_SQUARE_NV                                                  0x90A3
#define GL_SR8_EXT                                                    0x8FBD
#define GL_SRC0_ALPHA                                                 0x8588
#define GL_SRC0_RGB                                                   0x8580
#define GL_SRC1_ALPHA                                                 0x8589
#define GL_SRC1_COLOR                                                 0x88F9
#define GL_SRC1_RGB                                                   0x8581
#define GL_SRC2_ALPHA                                                 0x858A
#define GL_SRC2_RGB                                                   0x8582
#define GL_SRC_ALPHA                                                  0x0302
#define GL_SRC_ALPHA_SATURATE                                         0x0308
#define GL_SRC_ATOP_NV                                                0x928E
#define GL_SRC_COLOR                                                  0x0300
#define GL_SRC_IN_NV                                                  0x928A
#define GL_SRC_NV                                                     0x9286
#define GL_SRC_OUT_NV                                                 0x928C
#define GL_SRC_OVER_NV                                                0x9288
#define GL_SRGB                                                       0x8C40
#define GL_SRGB8                                                      0x8C41
#define GL_SRGB8_ALPHA8                                               0x8C43
#define GL_SRGB8_ALPHA8_EXT                                           0x8C43
#define GL_SRGB8_EXT                                                  0x8C41
#define GL_SRGB_ALPHA                                                 0x8C42
#define GL_SRGB_ALPHA_EXT                                             0x8C42
#define GL_SRGB_DECODE_ARB                                            0x8299
#define GL_SRGB_EXT                                                   0x8C40
#define GL_SRGB_READ                                                  0x8297
#define GL_SRGB_WRITE                                                 0x8298
#define GL_STACK_OVERFLOW                                             0x0503
#define GL_STACK_OVERFLOW_KHR                                         0x0503
#define GL_STACK_UNDERFLOW                                            0x0504
#define GL_STACK_UNDERFLOW_KHR                                        0x0504
#define GL_STANDARD_FONT_FORMAT_NV                                    0x936C
#define GL_STANDARD_FONT_NAME_NV                                      0x9072
#define GL_STATIC_ATI                                                 0x8760
#define GL_STATIC_COPY                                                0x88E6
#define GL_STATIC_COPY_ARB                                            0x88E6
#define GL_STATIC_DRAW                                                0x88E4
#define GL_STATIC_DRAW_ARB                                            0x88E4
#define GL_STATIC_READ                                                0x88E5
#define GL_STATIC_READ_ARB                                            0x88E5
#define GL_STATIC_VERTEX_ARRAY_IBM                                    103061
#define GL_STENCIL                                                    0x1802
#define GL_STENCIL_ATTACHMENT                                         0x8D20
#define GL_STENCIL_ATTACHMENT_EXT                                     0x8D20
#define GL_STENCIL_BACK_FAIL                                          0x8801
#define GL_STENCIL_BACK_FAIL_ATI                                      0x8801
#define GL_STENCIL_BACK_FUNC                                          0x8800
#define GL_STENCIL_BACK_FUNC_ATI                                      0x8800
#define GL_STENCIL_BACK_OP_VALUE_AMD                                  0x874D
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL                               0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL_ATI                           0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS                               0x8803
#define GL_STENCIL_BACK_PASS_DEPTH_PASS_ATI                           0x8803
#define GL_STENCIL_BACK_REF                                           0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK                                    0x8CA4
#define GL_STENCIL_BACK_WRITEMASK                                     0x8CA5
#define GL_STENCIL_BITS                                               0x0D57
#define GL_STENCIL_BUFFER_BIT                                         0x00000400
#define GL_STENCIL_CLEAR_TAG_VALUE_EXT                                0x88F3
#define GL_STENCIL_CLEAR_VALUE                                        0x0B91
#define GL_STENCIL_COMPONENTS                                         0x8285
#define GL_STENCIL_FAIL                                               0x0B94
#define GL_STENCIL_FUNC                                               0x0B92
#define GL_STENCIL_INDEX                                              0x1901
#define GL_STENCIL_INDEX1                                             0x8D46
#define GL_STENCIL_INDEX16                                            0x8D49
#define GL_STENCIL_INDEX16_EXT                                        0x8D49
#define GL_STENCIL_INDEX1_EXT                                         0x8D46
#define GL_STENCIL_INDEX4                                             0x8D47
#define GL_STENCIL_INDEX4_EXT                                         0x8D47
#define GL_STENCIL_INDEX8                                             0x8D48
#define GL_STENCIL_INDEX8_EXT                                         0x8D48
#define GL_STENCIL_OP_VALUE_AMD                                       0x874C
#define GL_STENCIL_PASS_DEPTH_FAIL                                    0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS                                    0x0B96
#define GL_STENCIL_REF                                                0x0B97
#define GL_STENCIL_REF_COMMAND_NV                                     0x000C
#define GL_STENCIL_RENDERABLE                                         0x8288
#define GL_STENCIL_SAMPLES_NV                                         0x932E
#define GL_STENCIL_TAG_BITS_EXT                                       0x88F2
#define GL_STENCIL_TEST                                               0x0B90
#define GL_STENCIL_TEST_TWO_SIDE_EXT                                  0x8910
#define GL_STENCIL_VALUE_MASK                                         0x0B93
#define GL_STENCIL_WRITEMASK                                          0x0B98
#define GL_STEREO                                                     0x0C33
#define GL_STORAGE_CACHED_APPLE                                       0x85BE
#define GL_STORAGE_CLIENT_APPLE                                       0x85B4
#define GL_STORAGE_PRIVATE_APPLE                                      0x85BD
#define GL_STORAGE_SHARED_APPLE                                       0x85BF
#define GL_STREAM_COPY                                                0x88E2
#define GL_STREAM_COPY_ARB                                            0x88E2
#define GL_STREAM_DRAW                                                0x88E0
#define GL_STREAM_DRAW_ARB                                            0x88E0
#define GL_STREAM_RASTERIZATION_AMD                                   0x91A0
#define GL_STREAM_READ                                                0x88E1
#define GL_STREAM_READ_ARB                                            0x88E1
#define GL_STRICT_DEPTHFUNC_HINT_PGI                                  0x1A216
#define GL_STRICT_LIGHTING_HINT_PGI                                   0x1A217
#define GL_STRICT_SCISSOR_HINT_PGI                                    0x1A218
#define GL_SUBGROUP_FEATURE_ARITHMETIC_BIT_KHR                        0x00000004
#define GL_SUBGROUP_FEATURE_BALLOT_BIT_KHR                            0x00000008
#define GL_SUBGROUP_FEATURE_BASIC_BIT_KHR                             0x00000001
#define GL_SUBGROUP_FEATURE_CLUSTERED_BIT_KHR                         0x00000040
#define GL_SUBGROUP_FEATURE_PARTITIONED_BIT_NV                        0x00000100
#define GL_SUBGROUP_FEATURE_QUAD_BIT_KHR                              0x00000080
#define GL_SUBGROUP_FEATURE_SHUFFLE_BIT_KHR                           0x00000010
#define GL_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT_KHR                  0x00000020
#define GL_SUBGROUP_FEATURE_VOTE_BIT_KHR                              0x00000002
#define GL_SUBGROUP_QUAD_ALL_STAGES_KHR                               0x9535
#define GL_SUBGROUP_SIZE_KHR                                          0x9532
#define GL_SUBGROUP_SUPPORTED_FEATURES_KHR                            0x9534
#define GL_SUBGROUP_SUPPORTED_STAGES_KHR                              0x9533
#define GL_SUBPIXEL_BITS                                              0x0D50
#define GL_SUBPIXEL_PRECISION_BIAS_X_BITS_NV                          0x9347
#define GL_SUBPIXEL_PRECISION_BIAS_Y_BITS_NV                          0x9348
#define GL_SUBSAMPLE_DISTANCE_AMD                                     0x883F
#define GL_SUBTRACT                                                   0x84E7
#define GL_SUBTRACT_ARB                                               0x84E7
#define GL_SUB_ATI                                                    0x8965
#define GL_SUCCESS_NV                                                 0x902F
#define GL_SUPERSAMPLE_SCALE_X_NV                                     0x9372
#define GL_SUPERSAMPLE_SCALE_Y_NV                                     0x9373
#define GL_SUPPORTED_MULTISAMPLE_MODES_AMD                            0x91B7
#define GL_SURFACE_MAPPED_NV                                          0x8700
#define GL_SURFACE_REGISTERED_NV                                      0x86FD
#define GL_SURFACE_STATE_NV                                           0x86EB
#define GL_SWIZZLE_STQ_ATI                                            0x8977
#define GL_SWIZZLE_STQ_DQ_ATI                                         0x8979
#define GL_SWIZZLE_STRQ_ATI                                           0x897A
#define GL_SWIZZLE_STRQ_DQ_ATI                                        0x897B
#define GL_SWIZZLE_STR_ATI                                            0x8976
#define GL_SWIZZLE_STR_DR_ATI                                         0x8978
#define GL_SYNC_CL_EVENT_ARB                                          0x8240
#define GL_SYNC_CL_EVENT_COMPLETE_ARB                                 0x8241
#define GL_SYNC_CONDITION                                             0x9113
#define GL_SYNC_FENCE                                                 0x9116
#define GL_SYNC_FLAGS                                                 0x9115
#define GL_SYNC_FLUSH_COMMANDS_BIT                                    0x00000001
#define GL_SYNC_GPU_COMMANDS_COMPLETE                                 0x9117
#define GL_SYNC_STATUS                                                0x9114
#define GL_SYNC_X11_FENCE_EXT                                         0x90E1
#define GL_SYSTEM_FONT_NAME_NV                                        0x9073
#define GL_T                                                          0x2001
#define GL_T2F_C3F_V3F                                                0x2A2A
#define GL_T2F_C4F_N3F_V3F                                            0x2A2C
#define GL_T2F_C4UB_V3F                                               0x2A29
#define GL_T2F_IUI_N3F_V2F_EXT                                        0x81B3
#define GL_T2F_IUI_N3F_V3F_EXT                                        0x81B4
#define GL_T2F_IUI_V2F_EXT                                            0x81B1
#define GL_T2F_IUI_V3F_EXT                                            0x81B2
#define GL_T2F_N3F_V3F                                                0x2A2B
#define GL_T2F_V3F                                                    0x2A27
#define GL_T4F_C4F_N3F_V4F                                            0x2A2D
#define GL_T4F_V4F                                                    0x2A28
#define GL_TABLE_TOO_LARGE                                            0x8031
#define GL_TABLE_TOO_LARGE_EXT                                        0x8031
#define GL_TANGENT_ARRAY_EXT                                          0x8439
#define GL_TANGENT_ARRAY_POINTER_EXT                                  0x8442
#define GL_TANGENT_ARRAY_STRIDE_EXT                                   0x843F
#define GL_TANGENT_ARRAY_TYPE_EXT                                     0x843E
#define GL_TASK_SHADER_BIT_NV                                         0x00000080
#define GL_TASK_SHADER_NV                                             0x955A
#define GL_TASK_SUBROUTINE_NV                                         0x957D
#define GL_TASK_SUBROUTINE_UNIFORM_NV                                 0x957F
#define GL_TASK_WORK_GROUP_SIZE_NV                                    0x953F
#define GL_TERMINATE_SEQUENCE_COMMAND_NV                              0x0000
#define GL_TESSELLATION_FACTOR_AMD                                    0x9005
#define GL_TESSELLATION_MODE_AMD                                      0x9004
#define GL_TESS_CONTROL_OUTPUT_VERTICES                               0x8E75
#define GL_TESS_CONTROL_PROGRAM_NV                                    0x891E
#define GL_TESS_CONTROL_PROGRAM_PARAMETER_BUFFER_NV                   0x8C74
#define GL_TESS_CONTROL_SHADER                                        0x8E88
#define GL_TESS_CONTROL_SHADER_BIT                                    0x00000008
#define GL_TESS_CONTROL_SHADER_PATCHES                                0x82F1
#define GL_TESS_CONTROL_SHADER_PATCHES_ARB                            0x82F1
#define GL_TESS_CONTROL_SUBROUTINE                                    0x92E9
#define GL_TESS_CONTROL_SUBROUTINE_UNIFORM                            0x92EF
#define GL_TESS_CONTROL_TEXTURE                                       0x829C
#define GL_TESS_EVALUATION_PROGRAM_NV                                 0x891F
#define GL_TESS_EVALUATION_PROGRAM_PARAMETER_BUFFER_NV                0x8C75
#define GL_TESS_EVALUATION_SHADER                                     0x8E87
#define GL_TESS_EVALUATION_SHADER_BIT                                 0x00000010
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS                         0x82F2
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB                     0x82F2
#define GL_TESS_EVALUATION_SUBROUTINE                                 0x92EA
#define GL_TESS_EVALUATION_SUBROUTINE_UNIFORM                         0x92F0
#define GL_TESS_EVALUATION_TEXTURE                                    0x829D
#define GL_TESS_GEN_MODE                                              0x8E76
#define GL_TESS_GEN_POINT_MODE                                        0x8E79
#define GL_TESS_GEN_SPACING                                           0x8E77
#define GL_TESS_GEN_VERTEX_ORDER                                      0x8E78
#define GL_TEXCOORD1_BIT_PGI                                          0x10000000
#define GL_TEXCOORD2_BIT_PGI                                          0x20000000
#define GL_TEXCOORD3_BIT_PGI                                          0x40000000
#define GL_TEXCOORD4_BIT_PGI                                          0x80000000
#define GL_TEXTURE                                                    0x1702
#define GL_TEXTURE0                                                   0x84C0
#define GL_TEXTURE0_ARB                                               0x84C0
#define GL_TEXTURE1                                                   0x84C1
#define GL_TEXTURE10                                                  0x84CA
#define GL_TEXTURE10_ARB                                              0x84CA
#define GL_TEXTURE11                                                  0x84CB
#define GL_TEXTURE11_ARB                                              0x84CB
#define GL_TEXTURE12                                                  0x84CC
#define GL_TEXTURE12_ARB                                              0x84CC
#define GL_TEXTURE13                                                  0x84CD
#define GL_TEXTURE13_ARB                                              0x84CD
#define GL_TEXTURE14                                                  0x84CE
#define GL_TEXTURE14_ARB                                              0x84CE
#define GL_TEXTURE15                                                  0x84CF
#define GL_TEXTURE15_ARB                                              0x84CF
#define GL_TEXTURE16                                                  0x84D0
#define GL_TEXTURE16_ARB                                              0x84D0
#define GL_TEXTURE17                                                  0x84D1
#define GL_TEXTURE17_ARB                                              0x84D1
#define GL_TEXTURE18                                                  0x84D2
#define GL_TEXTURE18_ARB                                              0x84D2
#define GL_TEXTURE19                                                  0x84D3
#define GL_TEXTURE19_ARB                                              0x84D3
#define GL_TEXTURE1_ARB                                               0x84C1
#define GL_TEXTURE2                                                   0x84C2
#define GL_TEXTURE20                                                  0x84D4
#define GL_TEXTURE20_ARB                                              0x84D4
#define GL_TEXTURE21                                                  0x84D5
#define GL_TEXTURE21_ARB                                              0x84D5
#define GL_TEXTURE22                                                  0x84D6
#define GL_TEXTURE22_ARB                                              0x84D6
#define GL_TEXTURE23                                                  0x84D7
#define GL_TEXTURE23_ARB                                              0x84D7
#define GL_TEXTURE24                                                  0x84D8
#define GL_TEXTURE24_ARB                                              0x84D8
#define GL_TEXTURE25                                                  0x84D9
#define GL_TEXTURE25_ARB                                              0x84D9
#define GL_TEXTURE26                                                  0x84DA
#define GL_TEXTURE26_ARB                                              0x84DA
#define GL_TEXTURE27                                                  0x84DB
#define GL_TEXTURE27_ARB                                              0x84DB
#define GL_TEXTURE28                                                  0x84DC
#define GL_TEXTURE28_ARB                                              0x84DC
#define GL_TEXTURE29                                                  0x84DD
#define GL_TEXTURE29_ARB                                              0x84DD
#define GL_TEXTURE2_ARB                                               0x84C2
#define GL_TEXTURE3                                                   0x84C3
#define GL_TEXTURE30                                                  0x84DE
#define GL_TEXTURE30_ARB                                              0x84DE
#define GL_TEXTURE31                                                  0x84DF
#define GL_TEXTURE31_ARB                                              0x84DF
#define GL_TEXTURE3_ARB                                               0x84C3
#define GL_TEXTURE4                                                   0x84C4
#define GL_TEXTURE4_ARB                                               0x84C4
#define GL_TEXTURE5                                                   0x84C5
#define GL_TEXTURE5_ARB                                               0x84C5
#define GL_TEXTURE6                                                   0x84C6
#define GL_TEXTURE6_ARB                                               0x84C6
#define GL_TEXTURE7                                                   0x84C7
#define GL_TEXTURE7_ARB                                               0x84C7
#define GL_TEXTURE8                                                   0x84C8
#define GL_TEXTURE8_ARB                                               0x84C8
#define GL_TEXTURE9                                                   0x84C9
#define GL_TEXTURE9_ARB                                               0x84C9
#define GL_TEXTURE_1D                                                 0x0DE0
#define GL_TEXTURE_1D_ARRAY                                           0x8C18
#define GL_TEXTURE_1D_ARRAY_EXT                                       0x8C18
#define GL_TEXTURE_1D_BINDING_EXT                                     0x8068
#define GL_TEXTURE_1D_STACK_BINDING_MESAX                             0x875D
#define GL_TEXTURE_1D_STACK_MESAX                                     0x8759
#define GL_TEXTURE_2D                                                 0x0DE1
#define GL_TEXTURE_2D_ARRAY                                           0x8C1A
#define GL_TEXTURE_2D_ARRAY_EXT                                       0x8C1A
#define GL_TEXTURE_2D_BINDING_EXT                                     0x8069
#define GL_TEXTURE_2D_MULTISAMPLE                                     0x9100
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY                               0x9102
#define GL_TEXTURE_2D_STACK_BINDING_MESAX                             0x875E
#define GL_TEXTURE_2D_STACK_MESAX                                     0x875A
#define GL_TEXTURE_3D                                                 0x806F
#define GL_TEXTURE_3D_BINDING_EXT                                     0x806A
#define GL_TEXTURE_3D_EXT                                             0x806F
#define GL_TEXTURE_4DSIZE_SGIS                                        0x8136
#define GL_TEXTURE_4D_BINDING_SGIS                                    0x814F
#define GL_TEXTURE_4D_SGIS                                            0x8134
#define GL_TEXTURE_ALPHA_SIZE                                         0x805F
#define GL_TEXTURE_ALPHA_SIZE_EXT                                     0x805F
#define GL_TEXTURE_ALPHA_TYPE                                         0x8C13
#define GL_TEXTURE_ALPHA_TYPE_ARB                                     0x8C13
#define GL_TEXTURE_APPLICATION_MODE_EXT                               0x834F
#define GL_TEXTURE_BASE_LEVEL                                         0x813C
#define GL_TEXTURE_BASE_LEVEL_SGIS                                    0x813C
#define GL_TEXTURE_BINDING_1D                                         0x8068
#define GL_TEXTURE_BINDING_1D_ARRAY                                   0x8C1C
#define GL_TEXTURE_BINDING_1D_ARRAY_EXT                               0x8C1C
#define GL_TEXTURE_BINDING_2D                                         0x8069
#define GL_TEXTURE_BINDING_2D_ARRAY                                   0x8C1D
#define GL_TEXTURE_BINDING_2D_ARRAY_EXT                               0x8C1D
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE                             0x9104
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY                       0x9105
#define GL_TEXTURE_BINDING_3D                                         0x806A
#define GL_TEXTURE_BINDING_BUFFER                                     0x8C2C
#define GL_TEXTURE_BINDING_BUFFER_ARB                                 0x8C2C
#define GL_TEXTURE_BINDING_BUFFER_EXT                                 0x8C2C
#define GL_TEXTURE_BINDING_CUBE_MAP                                   0x8514
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB                               0x8514
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY                             0x900A
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_ARB                         0x900A
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT                               0x8514
#define GL_TEXTURE_BINDING_RECTANGLE                                  0x84F6
#define GL_TEXTURE_BINDING_RECTANGLE_ARB                              0x84F6
#define GL_TEXTURE_BINDING_RECTANGLE_NV                               0x84F6
#define GL_TEXTURE_BINDING_RENDERBUFFER_NV                            0x8E53
#define GL_TEXTURE_BIT                                                0x00040000
#define GL_TEXTURE_BLUE_SIZE                                          0x805E
#define GL_TEXTURE_BLUE_SIZE_EXT                                      0x805E
#define GL_TEXTURE_BLUE_TYPE                                          0x8C12
#define GL_TEXTURE_BLUE_TYPE_ARB                                      0x8C12
#define GL_TEXTURE_BORDER                                             0x1005
#define GL_TEXTURE_BORDER_COLOR                                       0x1004
#define GL_TEXTURE_BORDER_VALUES_NV                                   0x871A
#define GL_TEXTURE_BUFFER                                             0x8C2A
#define GL_TEXTURE_BUFFER_ARB                                         0x8C2A
#define GL_TEXTURE_BUFFER_BINDING                                     0x8C2A
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING                          0x8C2D
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_ARB                      0x8C2D
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT                      0x8C2D
#define GL_TEXTURE_BUFFER_EXT                                         0x8C2A
#define GL_TEXTURE_BUFFER_FORMAT_ARB                                  0x8C2E
#define GL_TEXTURE_BUFFER_FORMAT_EXT                                  0x8C2E
#define GL_TEXTURE_BUFFER_OFFSET                                      0x919D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT                            0x919F
#define GL_TEXTURE_BUFFER_SIZE                                        0x919E
#define GL_TEXTURE_CLIPMAP_CENTER_SGIX                                0x8171
#define GL_TEXTURE_CLIPMAP_DEPTH_SGIX                                 0x8176
#define GL_TEXTURE_CLIPMAP_FRAME_SGIX                                 0x8172
#define GL_TEXTURE_CLIPMAP_LOD_OFFSET_SGIX                            0x8175
#define GL_TEXTURE_CLIPMAP_OFFSET_SGIX                                0x8173
#define GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX                         0x8174
#define GL_TEXTURE_COLOR_SAMPLES_NV                                   0x9046
#define GL_TEXTURE_COLOR_TABLE_SGI                                    0x80BC
#define GL_TEXTURE_COLOR_WRITEMASK_SGIS                               0x81EF
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB                             0x80BF
#define GL_TEXTURE_COMPARE_FUNC                                       0x884D
#define GL_TEXTURE_COMPARE_FUNC_ARB                                   0x884D
#define GL_TEXTURE_COMPARE_MODE                                       0x884C
#define GL_TEXTURE_COMPARE_MODE_ARB                                   0x884C
#define GL_TEXTURE_COMPARE_OPERATOR_SGIX                              0x819B
#define GL_TEXTURE_COMPARE_SGIX                                       0x819A
#define GL_TEXTURE_COMPONENTS                                         0x1003
#define GL_TEXTURE_COMPRESSED                                         0x86A1
#define GL_TEXTURE_COMPRESSED_ARB                                     0x86A1
#define GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT                            0x82B2
#define GL_TEXTURE_COMPRESSED_BLOCK_SIZE                              0x82B3
#define GL_TEXTURE_COMPRESSED_BLOCK_WIDTH                             0x82B1
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE                              0x86A0
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB                          0x86A0
#define GL_TEXTURE_COMPRESSION_HINT                                   0x84EF
#define GL_TEXTURE_COMPRESSION_HINT_ARB                               0x84EF
#define GL_TEXTURE_CONSTANT_DATA_SUNX                                 0x81D6
#define GL_TEXTURE_COORD_ARRAY                                        0x8078
#define GL_TEXTURE_COORD_ARRAY_ADDRESS_NV                             0x8F25
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING                         0x889A
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB                     0x889A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT                              0x808B
#define GL_TEXTURE_COORD_ARRAY_EXT                                    0x8078
#define GL_TEXTURE_COORD_ARRAY_LENGTH_NV                              0x8F2F
#define GL_TEXTURE_COORD_ARRAY_LIST_IBM                               103074
#define GL_TEXTURE_COORD_ARRAY_LIST_STRIDE_IBM                        103084
#define GL_TEXTURE_COORD_ARRAY_PARALLEL_POINTERS_INTEL                0x83F8
#define GL_TEXTURE_COORD_ARRAY_POINTER                                0x8092
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT                            0x8092
#define GL_TEXTURE_COORD_ARRAY_SIZE                                   0x8088
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT                               0x8088
#define GL_TEXTURE_COORD_ARRAY_STRIDE                                 0x808A
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT                             0x808A
#define GL_TEXTURE_COORD_ARRAY_TYPE                                   0x8089
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT                               0x8089
#define GL_TEXTURE_COORD_NV                                           0x8C79
#define GL_TEXTURE_COVERAGE_SAMPLES_NV                                0x9045
#define GL_TEXTURE_CUBE_MAP                                           0x8513
#define GL_TEXTURE_CUBE_MAP_ARB                                       0x8513
#define GL_TEXTURE_CUBE_MAP_ARRAY                                     0x9009
#define GL_TEXTURE_CUBE_MAP_ARRAY_ARB                                 0x9009
#define GL_TEXTURE_CUBE_MAP_EXT                                       0x8513
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X                                0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB                            0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT                            0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y                                0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB                            0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT                            0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z                                0x851A
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB                            0x851A
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT                            0x851A
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X                                0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB                            0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT                            0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y                                0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB                            0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT                            0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z                                0x8519
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB                            0x8519
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT                            0x8519
#define GL_TEXTURE_CUBE_MAP_SEAMLESS                                  0x884F
#define GL_TEXTURE_DEFORMATION_BIT_SGIX                               0x00000001
#define GL_TEXTURE_DEFORMATION_SGIX                                   0x8195
#define GL_TEXTURE_DEPTH                                              0x8071
#define GL_TEXTURE_DEPTH_EXT                                          0x8071
#define GL_TEXTURE_DEPTH_SIZE                                         0x884A
#define GL_TEXTURE_DEPTH_SIZE_ARB                                     0x884A
#define GL_TEXTURE_DEPTH_TYPE                                         0x8C16
#define GL_TEXTURE_DEPTH_TYPE_ARB                                     0x8C16
#define GL_TEXTURE_DS_SIZE_NV                                         0x871D
#define GL_TEXTURE_DT_SIZE_NV                                         0x871E
#define GL_TEXTURE_ENV                                                0x2300
#define GL_TEXTURE_ENV_BIAS_SGIX                                      0x80BE
#define GL_TEXTURE_ENV_COLOR                                          0x2201
#define GL_TEXTURE_ENV_MODE                                           0x2200
#define GL_TEXTURE_FETCH_BARRIER_BIT                                  0x00000008
#define GL_TEXTURE_FETCH_BARRIER_BIT_EXT                              0x00000008
#define GL_TEXTURE_FILTER4_SIZE_SGIS                                  0x8147
#define GL_TEXTURE_FILTER_CONTROL                                     0x8500
#define GL_TEXTURE_FILTER_CONTROL_EXT                                 0x8500
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS                             0x9107
#define GL_TEXTURE_FLOAT_COMPONENTS_NV                                0x888C
#define GL_TEXTURE_FREE_MEMORY_ATI                                    0x87FC
#define GL_TEXTURE_GATHER                                             0x82A2
#define GL_TEXTURE_GATHER_SHADOW                                      0x82A3
#define GL_TEXTURE_GEN_MODE                                           0x2500
#define GL_TEXTURE_GEN_Q                                              0x0C63
#define GL_TEXTURE_GEN_R                                              0x0C62
#define GL_TEXTURE_GEN_S                                              0x0C60
#define GL_TEXTURE_GEN_T                                              0x0C61
#define GL_TEXTURE_GEQUAL_R_SGIX                                      0x819D
#define GL_TEXTURE_GREEN_SIZE                                         0x805D
#define GL_TEXTURE_GREEN_SIZE_EXT                                     0x805D
#define GL_TEXTURE_GREEN_TYPE                                         0x8C11
#define GL_TEXTURE_GREEN_TYPE_ARB                                     0x8C11
#define GL_TEXTURE_HEIGHT                                             0x1001
#define GL_TEXTURE_HI_SIZE_NV                                         0x871B
#define GL_TEXTURE_IMAGE_FORMAT                                       0x828F
#define GL_TEXTURE_IMAGE_TYPE                                         0x8290
#define GL_TEXTURE_IMMUTABLE_FORMAT                                   0x912F
#define GL_TEXTURE_IMMUTABLE_LEVELS                                   0x82DF
#define GL_TEXTURE_INDEX_SIZE_EXT                                     0x80ED
#define GL_TEXTURE_INTENSITY_SIZE                                     0x8061
#define GL_TEXTURE_INTENSITY_SIZE_EXT                                 0x8061
#define GL_TEXTURE_INTENSITY_TYPE                                     0x8C15
#define GL_TEXTURE_INTENSITY_TYPE_ARB                                 0x8C15
#define GL_TEXTURE_INTERNAL_FORMAT                                    0x1003
#define GL_TEXTURE_LEQUAL_R_SGIX                                      0x819C
#define GL_TEXTURE_LIGHTING_MODE_HP                                   0x8167
#define GL_TEXTURE_LIGHT_EXT                                          0x8350
#define GL_TEXTURE_LOD_BIAS                                           0x8501
#define GL_TEXTURE_LOD_BIAS_EXT                                       0x8501
#define GL_TEXTURE_LOD_BIAS_R_SGIX                                    0x8190
#define GL_TEXTURE_LOD_BIAS_S_SGIX                                    0x818E
#define GL_TEXTURE_LOD_BIAS_T_SGIX                                    0x818F
#define GL_TEXTURE_LO_SIZE_NV                                         0x871C
#define GL_TEXTURE_LUMINANCE_SIZE                                     0x8060
#define GL_TEXTURE_LUMINANCE_SIZE_EXT                                 0x8060
#define GL_TEXTURE_LUMINANCE_TYPE                                     0x8C14
#define GL_TEXTURE_LUMINANCE_TYPE_ARB                                 0x8C14
#define GL_TEXTURE_MAG_FILTER                                         0x2800
#define GL_TEXTURE_MAG_SIZE_NV                                        0x871F
#define GL_TEXTURE_MATERIAL_FACE_EXT                                  0x8351
#define GL_TEXTURE_MATERIAL_PARAMETER_EXT                             0x8352
#define GL_TEXTURE_MATRIX                                             0x0BA8
#define GL_TEXTURE_MAX_ANISOTROPY                                     0x84FE
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                                 0x84FE
#define GL_TEXTURE_MAX_CLAMP_R_SGIX                                   0x836B
#define GL_TEXTURE_MAX_CLAMP_S_SGIX                                   0x8369
#define GL_TEXTURE_MAX_CLAMP_T_SGIX                                   0x836A
#define GL_TEXTURE_MAX_LEVEL                                          0x813D
#define GL_TEXTURE_MAX_LEVEL_SGIS                                     0x813D
#define GL_TEXTURE_MAX_LOD                                            0x813B
#define GL_TEXTURE_MAX_LOD_SGIS                                       0x813B
#define GL_TEXTURE_MEMORY_LAYOUT_INTEL                                0x83FF
#define GL_TEXTURE_MIN_FILTER                                         0x2801
#define GL_TEXTURE_MIN_LOD                                            0x813A
#define GL_TEXTURE_MIN_LOD_SGIS                                       0x813A
#define GL_TEXTURE_MULTI_BUFFER_HINT_SGIX                             0x812E
#define GL_TEXTURE_NORMAL_EXT                                         0x85AF
#define GL_TEXTURE_POST_SPECULAR_HP                                   0x8168
#define GL_TEXTURE_PRE_SPECULAR_HP                                    0x8169
#define GL_TEXTURE_PRIORITY                                           0x8066
#define GL_TEXTURE_PRIORITY_EXT                                       0x8066
#define GL_TEXTURE_RANGE_LENGTH_APPLE                                 0x85B7
#define GL_TEXTURE_RANGE_POINTER_APPLE                                0x85B8
#define GL_TEXTURE_RECTANGLE                                          0x84F5
#define GL_TEXTURE_RECTANGLE_ARB                                      0x84F5
#define GL_TEXTURE_RECTANGLE_NV                                       0x84F5
#define GL_TEXTURE_REDUCTION_MODE_ARB                                 0x9366
#define GL_TEXTURE_REDUCTION_MODE_EXT                                 0x9366
#define GL_TEXTURE_RED_SIZE                                           0x805C
#define GL_TEXTURE_RED_SIZE_EXT                                       0x805C
#define GL_TEXTURE_RED_TYPE                                           0x8C10
#define GL_TEXTURE_RED_TYPE_ARB                                       0x8C10
#define GL_TEXTURE_RENDERBUFFER_DATA_STORE_BINDING_NV                 0x8E54
#define GL_TEXTURE_RENDERBUFFER_NV                                    0x8E55
#define GL_TEXTURE_RESIDENT                                           0x8067
#define GL_TEXTURE_RESIDENT_EXT                                       0x8067
#define GL_TEXTURE_SAMPLES                                            0x9106
#define GL_TEXTURE_SHADER_NV                                          0x86DE
#define GL_TEXTURE_SHADOW                                             0x82A1
#define GL_TEXTURE_SHARED_SIZE                                        0x8C3F
#define GL_TEXTURE_SHARED_SIZE_EXT                                    0x8C3F
#define GL_TEXTURE_SPARSE_ARB                                         0x91A6
#define GL_TEXTURE_SRGB_DECODE_EXT                                    0x8A48
#define GL_TEXTURE_STACK_DEPTH                                        0x0BA5
#define GL_TEXTURE_STENCIL_SIZE                                       0x88F1
#define GL_TEXTURE_STENCIL_SIZE_EXT                                   0x88F1
#define GL_TEXTURE_STORAGE_HINT_APPLE                                 0x85BC
#define GL_TEXTURE_STORAGE_SPARSE_BIT_AMD                             0x00000001
#define GL_TEXTURE_SWIZZLE_A                                          0x8E45
#define GL_TEXTURE_SWIZZLE_A_EXT                                      0x8E45
#define GL_TEXTURE_SWIZZLE_B                                          0x8E44
#define GL_TEXTURE_SWIZZLE_B_EXT                                      0x8E44
#define GL_TEXTURE_SWIZZLE_G                                          0x8E43
#define GL_TEXTURE_SWIZZLE_G_EXT                                      0x8E43
#define GL_TEXTURE_SWIZZLE_R                                          0x8E42
#define GL_TEXTURE_SWIZZLE_RGBA                                       0x8E46
#define GL_TEXTURE_SWIZZLE_RGBA_EXT                                   0x8E46
#define GL_TEXTURE_SWIZZLE_R_EXT                                      0x8E42
#define GL_TEXTURE_TARGET                                             0x1006
#define GL_TEXTURE_TILING_EXT                                         0x9580
#define GL_TEXTURE_TOO_LARGE_EXT                                      0x8065
#define GL_TEXTURE_UNSIGNED_REMAP_MODE_NV                             0x888F
#define GL_TEXTURE_UPDATE_BARRIER_BIT                                 0x00000100
#define GL_TEXTURE_UPDATE_BARRIER_BIT_EXT                             0x00000100
#define GL_TEXTURE_VIEW                                               0x82B5
#define GL_TEXTURE_VIEW_MIN_LAYER                                     0x82DD
#define GL_TEXTURE_VIEW_MIN_LEVEL                                     0x82DB
#define GL_TEXTURE_VIEW_NUM_LAYERS                                    0x82DE
#define GL_TEXTURE_VIEW_NUM_LEVELS                                    0x82DC
#define GL_TEXTURE_WIDTH                                              0x1000
#define GL_TEXTURE_WRAP_Q_SGIS                                        0x8137
#define GL_TEXTURE_WRAP_R                                             0x8072
#define GL_TEXTURE_WRAP_R_EXT                                         0x8072
#define GL_TEXTURE_WRAP_S                                             0x2802
#define GL_TEXTURE_WRAP_T                                             0x2803
#define GL_TEXT_FRAGMENT_SHADER_ATI                                   0x8200
#define GL_TILE_RASTER_ORDER_FIXED_MESA                               0x8BB8
#define GL_TILE_RASTER_ORDER_INCREASING_X_MESA                        0x8BB9
#define GL_TILE_RASTER_ORDER_INCREASING_Y_MESA                        0x8BBA
#define GL_TILING_TYPES_EXT                                           0x9583
#define GL_TIMEOUT_EXPIRED                                            0x911B
#define GL_TIMEOUT_IGNORED                                            0xFFFFFFFFFFFFFFFF
#define GL_TIMESTAMP                                                  0x8E28
#define GL_TIME_ELAPSED                                               0x88BF
#define GL_TIME_ELAPSED_EXT                                           0x88BF
#define GL_TOP_LEVEL_ARRAY_SIZE                                       0x930C
#define GL_TOP_LEVEL_ARRAY_STRIDE                                     0x930D
#define GL_TRACK_MATRIX_NV                                            0x8648
#define GL_TRACK_MATRIX_TRANSFORM_NV                                  0x8649
#define GL_TRANSFORM_BIT                                              0x00001000
#define GL_TRANSFORM_FEEDBACK                                         0x8E22
#define GL_TRANSFORM_FEEDBACK_ACTIVE                                  0x8E24
#define GL_TRANSFORM_FEEDBACK_ATTRIBS_NV                              0x8C7E
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT                             0x00000800
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT_EXT                         0x00000800
#define GL_TRANSFORM_FEEDBACK_BINDING                                 0x8E25
#define GL_TRANSFORM_FEEDBACK_BINDING_NV                              0x8E25
#define GL_TRANSFORM_FEEDBACK_BUFFER                                  0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE                           0x8E24
#define GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE_NV                        0x8E24
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING                          0x8C8F
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_EXT                      0x8C8F
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_NV                       0x8C8F
#define GL_TRANSFORM_FEEDBACK_BUFFER_EXT                              0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_INDEX                            0x934B
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE                             0x8C7F
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE_EXT                         0x8C7F
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE_NV                          0x8C7F
#define GL_TRANSFORM_FEEDBACK_BUFFER_NV                               0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED                           0x8E23
#define GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED_NV                        0x8E23
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE                             0x8C85
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE_EXT                         0x8C85
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE_NV                          0x8C85
#define GL_TRANSFORM_FEEDBACK_BUFFER_START                            0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_START_EXT                        0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_START_NV                         0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE                           0x934C
#define GL_TRANSFORM_FEEDBACK_NV                                      0x8E22
#define GL_TRANSFORM_FEEDBACK_OVERFLOW                                0x82EC
#define GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB                            0x82EC
#define GL_TRANSFORM_FEEDBACK_PAUSED                                  0x8E23
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN                      0x8C88
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT                  0x8C88
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_NV                   0x8C88
#define GL_TRANSFORM_FEEDBACK_RECORD_NV                               0x8C86
#define GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW                         0x82ED
#define GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB                     0x82ED
#define GL_TRANSFORM_FEEDBACK_VARYING                                 0x92F4
#define GL_TRANSFORM_FEEDBACK_VARYINGS                                0x8C83
#define GL_TRANSFORM_FEEDBACK_VARYINGS_EXT                            0x8C83
#define GL_TRANSFORM_FEEDBACK_VARYINGS_NV                             0x8C83
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH                      0x8C76
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH_EXT                  0x8C76
#define GL_TRANSFORM_HINT_APPLE                                       0x85B1
#define GL_TRANSLATE_2D_NV                                            0x9090
#define GL_TRANSLATE_3D_NV                                            0x9091
#define GL_TRANSLATE_X_NV                                             0x908E
#define GL_TRANSLATE_Y_NV                                             0x908F
#define GL_TRANSPOSE_AFFINE_2D_NV                                     0x9096
#define GL_TRANSPOSE_AFFINE_3D_NV                                     0x9098
#define GL_TRANSPOSE_COLOR_MATRIX                                     0x84E6
#define GL_TRANSPOSE_COLOR_MATRIX_ARB                                 0x84E6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB                               0x88B7
#define GL_TRANSPOSE_MODELVIEW_MATRIX                                 0x84E3
#define GL_TRANSPOSE_MODELVIEW_MATRIX_ARB                             0x84E3
#define GL_TRANSPOSE_NV                                               0x862C
#define GL_TRANSPOSE_PROGRAM_MATRIX_EXT                               0x8E2E
#define GL_TRANSPOSE_PROJECTION_MATRIX                                0x84E4
#define GL_TRANSPOSE_PROJECTION_MATRIX_ARB                            0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX                                   0x84E5
#define GL_TRANSPOSE_TEXTURE_MATRIX_ARB                               0x84E5
#define GL_TRIANGLES                                                  0x0004
#define GL_TRIANGLES_ADJACENCY                                        0x000C
#define GL_TRIANGLES_ADJACENCY_ARB                                    0x000C
#define GL_TRIANGLES_ADJACENCY_EXT                                    0x000C
#define GL_TRIANGLE_FAN                                               0x0006
#define GL_TRIANGLE_LIST_SUN                                          0x81D7
#define GL_TRIANGLE_MESH_SUN                                          0x8615
#define GL_TRIANGLE_STRIP                                             0x0005
#define GL_TRIANGLE_STRIP_ADJACENCY                                   0x000D
#define GL_TRIANGLE_STRIP_ADJACENCY_ARB                               0x000D
#define GL_TRIANGLE_STRIP_ADJACENCY_EXT                               0x000D
#define GL_TRIANGULAR_NV                                              0x90A5
#define GL_TRUE                                                       1
#define GL_TYPE                                                       0x92FA
#define GL_UNCORRELATED_NV                                            0x9282
#define GL_UNDEFINED_APPLE                                            0x8A1C
#define GL_UNDEFINED_VERTEX                                           0x8260
#define GL_UNIFORM                                                    0x92E1
#define GL_UNIFORM_ADDRESS_COMMAND_NV                                 0x000A
#define GL_UNIFORM_ARRAY_STRIDE                                       0x8A3C
#define GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX                        0x92DA
#define GL_UNIFORM_BARRIER_BIT                                        0x00000004
#define GL_UNIFORM_BARRIER_BIT_EXT                                    0x00000004
#define GL_UNIFORM_BLOCK                                              0x92E2
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS                              0x8A42
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES                       0x8A43
#define GL_UNIFORM_BLOCK_BINDING                                      0x8A3F
#define GL_UNIFORM_BLOCK_DATA_SIZE                                    0x8A40
#define GL_UNIFORM_BLOCK_INDEX                                        0x8A3A
#define GL_UNIFORM_BLOCK_NAME_LENGTH                                  0x8A41
#define GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER                 0x90EC
#define GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER                0x8A46
#define GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER                0x8A45
#define GL_UNIFORM_BLOCK_REFERENCED_BY_MESH_SHADER_NV                 0x959C
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TASK_SHADER_NV                 0x959D
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER            0x84F0
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER         0x84F1
#define GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER                  0x8A44
#define GL_UNIFORM_BUFFER                                             0x8A11
#define GL_UNIFORM_BUFFER_ADDRESS_NV                                  0x936F
#define GL_UNIFORM_BUFFER_BINDING                                     0x8A28
#define GL_UNIFORM_BUFFER_BINDING_EXT                                 0x8DEF
#define GL_UNIFORM_BUFFER_EXT                                         0x8DEE
#define GL_UNIFORM_BUFFER_LENGTH_NV                                   0x9370
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT                            0x8A34
#define GL_UNIFORM_BUFFER_SIZE                                        0x8A2A
#define GL_UNIFORM_BUFFER_START                                       0x8A29
#define GL_UNIFORM_BUFFER_UNIFIED_NV                                  0x936E
#define GL_UNIFORM_IS_ROW_MAJOR                                       0x8A3E
#define GL_UNIFORM_MATRIX_STRIDE                                      0x8A3D
#define GL_UNIFORM_NAME_LENGTH                                        0x8A39
#define GL_UNIFORM_OFFSET                                             0x8A3B
#define GL_UNIFORM_SIZE                                               0x8A38
#define GL_UNIFORM_TYPE                                               0x8A37
#define GL_UNKNOWN_CONTEXT_RESET                                      0x8255
#define GL_UNKNOWN_CONTEXT_RESET_ARB                                  0x8255
#define GL_UNKNOWN_CONTEXT_RESET_KHR                                  0x8255
#define GL_UNPACK_ALIGNMENT                                           0x0CF5
#define GL_UNPACK_CLIENT_STORAGE_APPLE                                0x85B2
#define GL_UNPACK_CMYK_HINT_EXT                                       0x800F
#define GL_UNPACK_COMPRESSED_BLOCK_DEPTH                              0x9129
#define GL_UNPACK_COMPRESSED_BLOCK_HEIGHT                             0x9128
#define GL_UNPACK_COMPRESSED_BLOCK_SIZE                               0x912A
#define GL_UNPACK_COMPRESSED_BLOCK_WIDTH                              0x9127
#define GL_UNPACK_CONSTANT_DATA_SUNX                                  0x81D5
#define GL_UNPACK_IMAGE_DEPTH_SGIS                                    0x8133
#define GL_UNPACK_IMAGE_HEIGHT                                        0x806E
#define GL_UNPACK_IMAGE_HEIGHT_EXT                                    0x806E
#define GL_UNPACK_LSB_FIRST                                           0x0CF1
#define GL_UNPACK_RESAMPLE_OML                                        0x8985
#define GL_UNPACK_RESAMPLE_SGIX                                       0x842F
#define GL_UNPACK_ROW_BYTES_APPLE                                     0x8A16
#define GL_UNPACK_ROW_LENGTH                                          0x0CF2
#define GL_UNPACK_SKIP_IMAGES                                         0x806D
#define GL_UNPACK_SKIP_IMAGES_EXT                                     0x806D
#define GL_UNPACK_SKIP_PIXELS                                         0x0CF4
#define GL_UNPACK_SKIP_ROWS                                           0x0CF3
#define GL_UNPACK_SKIP_VOLUMES_SGIS                                   0x8132
#define GL_UNPACK_SUBSAMPLE_RATE_SGIX                                 0x85A1
#define GL_UNPACK_SWAP_BYTES                                          0x0CF0
#define GL_UNSIGNALED                                                 0x9118
#define GL_UNSIGNED_BYTE                                              0x1401
#define GL_UNSIGNED_BYTE_2_3_3_REV                                    0x8362
#define GL_UNSIGNED_BYTE_3_3_2                                        0x8032
#define GL_UNSIGNED_BYTE_3_3_2_EXT                                    0x8032
#define GL_UNSIGNED_IDENTITY_NV                                       0x8536
#define GL_UNSIGNED_INT                                               0x1405
#define GL_UNSIGNED_INT16_NV                                          0x8FF0
#define GL_UNSIGNED_INT16_VEC2_NV                                     0x8FF1
#define GL_UNSIGNED_INT16_VEC3_NV                                     0x8FF2
#define GL_UNSIGNED_INT16_VEC4_NV                                     0x8FF3
#define GL_UNSIGNED_INT64_AMD                                         0x8BC2
#define GL_UNSIGNED_INT64_ARB                                         0x140F
#define GL_UNSIGNED_INT64_NV                                          0x140F
#define GL_UNSIGNED_INT64_VEC2_ARB                                    0x8FF5
#define GL_UNSIGNED_INT64_VEC2_NV                                     0x8FF5
#define GL_UNSIGNED_INT64_VEC3_ARB                                    0x8FF6
#define GL_UNSIGNED_INT64_VEC3_NV                                     0x8FF6
#define GL_UNSIGNED_INT64_VEC4_ARB                                    0x8FF7
#define GL_UNSIGNED_INT64_VEC4_NV                                     0x8FF7
#define GL_UNSIGNED_INT8_NV                                           0x8FEC
#define GL_UNSIGNED_INT8_VEC2_NV                                      0x8FED
#define GL_UNSIGNED_INT8_VEC3_NV                                      0x8FEE
#define GL_UNSIGNED_INT8_VEC4_NV                                      0x8FEF
#define GL_UNSIGNED_INT_10F_11F_11F_REV                               0x8C3B
#define GL_UNSIGNED_INT_10F_11F_11F_REV_EXT                           0x8C3B
#define GL_UNSIGNED_INT_10_10_10_2                                    0x8036
#define GL_UNSIGNED_INT_10_10_10_2_EXT                                0x8036
#define GL_UNSIGNED_INT_24_8                                          0x84FA
#define GL_UNSIGNED_INT_24_8_EXT                                      0x84FA
#define GL_UNSIGNED_INT_24_8_NV                                       0x84FA
#define GL_UNSIGNED_INT_2_10_10_10_REV                                0x8368
#define GL_UNSIGNED_INT_5_9_9_9_REV                                   0x8C3E
#define GL_UNSIGNED_INT_5_9_9_9_REV_EXT                               0x8C3E
#define GL_UNSIGNED_INT_8_8_8_8                                       0x8035
#define GL_UNSIGNED_INT_8_8_8_8_EXT                                   0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV                                   0x8367
#define GL_UNSIGNED_INT_8_8_S8_S8_REV_NV                              0x86DB
#define GL_UNSIGNED_INT_ATOMIC_COUNTER                                0x92DB
#define GL_UNSIGNED_INT_IMAGE_1D                                      0x9062
#define GL_UNSIGNED_INT_IMAGE_1D_ARRAY                                0x9068
#define GL_UNSIGNED_INT_IMAGE_1D_ARRAY_EXT                            0x9068
#define GL_UNSIGNED_INT_IMAGE_1D_EXT                                  0x9062
#define GL_UNSIGNED_INT_IMAGE_2D                                      0x9063
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY                                0x9069
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY_EXT                            0x9069
#define GL_UNSIGNED_INT_IMAGE_2D_EXT                                  0x9063
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE                          0x906B
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY                    0x906C
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT                0x906C
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_EXT                      0x906B
#define GL_UNSIGNED_INT_IMAGE_2D_RECT                                 0x9065
#define GL_UNSIGNED_INT_IMAGE_2D_RECT_EXT                             0x9065
#define GL_UNSIGNED_INT_IMAGE_3D                                      0x9064
#define GL_UNSIGNED_INT_IMAGE_3D_EXT                                  0x9064
#define GL_UNSIGNED_INT_IMAGE_BUFFER                                  0x9067
#define GL_UNSIGNED_INT_IMAGE_BUFFER_EXT                              0x9067
#define GL_UNSIGNED_INT_IMAGE_CUBE                                    0x9066
#define GL_UNSIGNED_INT_IMAGE_CUBE_EXT                                0x9066
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY                          0x906A
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_EXT                      0x906A
#define GL_UNSIGNED_INT_S8_S8_8_8_NV                                  0x86DA
#define GL_UNSIGNED_INT_SAMPLER_1D                                    0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY                              0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT                          0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_1D_EXT                                0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_2D                                    0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY                              0x8DD7
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT                          0x8DD7
#define GL_UNSIGNED_INT_SAMPLER_2D_EXT                                0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE                        0x910A
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY                  0x910D
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT                               0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT                           0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_3D                                    0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_3D_EXT                                0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_BUFFER                                0x8DD8
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_AMD                            0x9003
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT                            0x8DD8
#define GL_UNSIGNED_INT_SAMPLER_CUBE                                  0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_CUBE_EXT                              0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY                        0x900F
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_ARB                    0x900F
#define GL_UNSIGNED_INT_SAMPLER_RENDERBUFFER_NV                       0x8E58
#define GL_UNSIGNED_INT_VEC2                                          0x8DC6
#define GL_UNSIGNED_INT_VEC2_EXT                                      0x8DC6
#define GL_UNSIGNED_INT_VEC3                                          0x8DC7
#define GL_UNSIGNED_INT_VEC3_EXT                                      0x8DC7
#define GL_UNSIGNED_INT_VEC4                                          0x8DC8
#define GL_UNSIGNED_INT_VEC4_EXT                                      0x8DC8
#define GL_UNSIGNED_INVERT_NV                                         0x8537
#define GL_UNSIGNED_NORMALIZED                                        0x8C17
#define GL_UNSIGNED_NORMALIZED_ARB                                    0x8C17
#define GL_UNSIGNED_SHORT                                             0x1403
#define GL_UNSIGNED_SHORT_1_5_5_5_REV                                 0x8366
#define GL_UNSIGNED_SHORT_4_4_4_4                                     0x8033
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT                                 0x8033
#define GL_UNSIGNED_SHORT_4_4_4_4_REV                                 0x8365
#define GL_UNSIGNED_SHORT_5_5_5_1                                     0x8034
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT                                 0x8034
#define GL_UNSIGNED_SHORT_5_6_5                                       0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV                                   0x8364
#define GL_UNSIGNED_SHORT_8_8_APPLE                                   0x85BA
#define GL_UNSIGNED_SHORT_8_8_MESA                                    0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_APPLE                               0x85BB
#define GL_UNSIGNED_SHORT_8_8_REV_MESA                                0x85BB
#define GL_UPLOAD_GPU_MASK_NVX                                        0x954A
#define GL_UPPER_LEFT                                                 0x8CA2
#define GL_USE_MISSING_GLYPH_NV                                       0x90AA
#define GL_UTF16_NV                                                   0x909B
#define GL_UTF8_NV                                                    0x909A
#define GL_UUID_SIZE_EXT                                              16
#define GL_V2F                                                        0x2A20
#define GL_V3F                                                        0x2A21
#define GL_VALIDATE_STATUS                                            0x8B83
#define GL_VARIABLE_A_NV                                              0x8523
#define GL_VARIABLE_B_NV                                              0x8524
#define GL_VARIABLE_C_NV                                              0x8525
#define GL_VARIABLE_D_NV                                              0x8526
#define GL_VARIABLE_E_NV                                              0x8527
#define GL_VARIABLE_F_NV                                              0x8528
#define GL_VARIABLE_G_NV                                              0x8529
#define GL_VARIANT_ARRAY_EXT                                          0x87E8
#define GL_VARIANT_ARRAY_POINTER_EXT                                  0x87E9
#define GL_VARIANT_ARRAY_STRIDE_EXT                                   0x87E6
#define GL_VARIANT_ARRAY_TYPE_EXT                                     0x87E7
#define GL_VARIANT_DATATYPE_EXT                                       0x87E5
#define GL_VARIANT_EXT                                                0x87C1
#define GL_VARIANT_VALUE_EXT                                          0x87E4
#define GL_VBO_FREE_MEMORY_ATI                                        0x87FB
#define GL_VECTOR_EXT                                                 0x87BF
#define GL_VENDOR                                                     0x1F00
#define GL_VERSION                                                    0x1F02
#define GL_VERTEX23_BIT_PGI                                           0x00000004
#define GL_VERTEX4_BIT_PGI                                            0x00000008
#define GL_VERTEX_ARRAY                                               0x8074
#define GL_VERTEX_ARRAY_ADDRESS_NV                                    0x8F21
#define GL_VERTEX_ARRAY_BINDING                                       0x85B5
#define GL_VERTEX_ARRAY_BINDING_APPLE                                 0x85B5
#define GL_VERTEX_ARRAY_BUFFER_BINDING                                0x8896
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB                            0x8896
#define GL_VERTEX_ARRAY_COUNT_EXT                                     0x807D
#define GL_VERTEX_ARRAY_EXT                                           0x8074
#define GL_VERTEX_ARRAY_KHR                                           0x8074
#define GL_VERTEX_ARRAY_LENGTH_NV                                     0x8F2B
#define GL_VERTEX_ARRAY_LIST_IBM                                      103070
#define GL_VERTEX_ARRAY_LIST_STRIDE_IBM                               103080
#define GL_VERTEX_ARRAY_OBJECT_AMD                                    0x9154
#define GL_VERTEX_ARRAY_OBJECT_EXT                                    0x9154
#define GL_VERTEX_ARRAY_PARALLEL_POINTERS_INTEL                       0x83F5
#define GL_VERTEX_ARRAY_POINTER                                       0x808E
#define GL_VERTEX_ARRAY_POINTER_EXT                                   0x808E
#define GL_VERTEX_ARRAY_RANGE_APPLE                                   0x851D
#define GL_VERTEX_ARRAY_RANGE_LENGTH_APPLE                            0x851E
#define GL_VERTEX_ARRAY_RANGE_LENGTH_NV                               0x851E
#define GL_VERTEX_ARRAY_RANGE_NV                                      0x851D
#define GL_VERTEX_ARRAY_RANGE_POINTER_APPLE                           0x8521
#define GL_VERTEX_ARRAY_RANGE_POINTER_NV                              0x8521
#define GL_VERTEX_ARRAY_RANGE_VALID_NV                                0x851F
#define GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV                        0x8533
#define GL_VERTEX_ARRAY_SIZE                                          0x807A
#define GL_VERTEX_ARRAY_SIZE_EXT                                      0x807A
#define GL_VERTEX_ARRAY_STORAGE_HINT_APPLE                            0x851F
#define GL_VERTEX_ARRAY_STRIDE                                        0x807C
#define GL_VERTEX_ARRAY_STRIDE_EXT                                    0x807C
#define GL_VERTEX_ARRAY_TYPE                                          0x807B
#define GL_VERTEX_ARRAY_TYPE_EXT                                      0x807B
#define GL_VERTEX_ATTRIB_ARRAY0_NV                                    0x8650
#define GL_VERTEX_ATTRIB_ARRAY10_NV                                   0x865A
#define GL_VERTEX_ATTRIB_ARRAY11_NV                                   0x865B
#define GL_VERTEX_ATTRIB_ARRAY12_NV                                   0x865C
#define GL_VERTEX_ATTRIB_ARRAY13_NV                                   0x865D
#define GL_VERTEX_ATTRIB_ARRAY14_NV                                   0x865E
#define GL_VERTEX_ATTRIB_ARRAY15_NV                                   0x865F
#define GL_VERTEX_ATTRIB_ARRAY1_NV                                    0x8651
#define GL_VERTEX_ATTRIB_ARRAY2_NV                                    0x8652
#define GL_VERTEX_ATTRIB_ARRAY3_NV                                    0x8653
#define GL_VERTEX_ATTRIB_ARRAY4_NV                                    0x8654
#define GL_VERTEX_ATTRIB_ARRAY5_NV                                    0x8655
#define GL_VERTEX_ATTRIB_ARRAY6_NV                                    0x8656
#define GL_VERTEX_ATTRIB_ARRAY7_NV                                    0x8657
#define GL_VERTEX_ATTRIB_ARRAY8_NV                                    0x8658
#define GL_VERTEX_ATTRIB_ARRAY9_NV                                    0x8659
#define GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV                             0x8F20
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT                            0x00000001
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT_EXT                        0x00000001
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING                         0x889F
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB                     0x889F
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR                                0x88FE
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ARB                            0x88FE
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED                                0x8622
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB                            0x8622
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER                                0x88FD
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER_EXT                            0x88FD
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER_NV                             0x88FD
#define GL_VERTEX_ATTRIB_ARRAY_LENGTH_NV                              0x8F2A
#define GL_VERTEX_ATTRIB_ARRAY_LONG                                   0x874E
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED                             0x886A
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB                         0x886A
#define GL_VERTEX_ATTRIB_ARRAY_POINTER                                0x8645
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB                            0x8645
#define GL_VERTEX_ATTRIB_ARRAY_SIZE                                   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB                               0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE                                 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB                             0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE                                   0x8625
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB                               0x8625
#define GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV                             0x8F1E
#define GL_VERTEX_ATTRIB_BINDING                                      0x82D4
#define GL_VERTEX_ATTRIB_MAP1_APPLE                                   0x8A00
#define GL_VERTEX_ATTRIB_MAP1_COEFF_APPLE                             0x8A03
#define GL_VERTEX_ATTRIB_MAP1_DOMAIN_APPLE                            0x8A05
#define GL_VERTEX_ATTRIB_MAP1_ORDER_APPLE                             0x8A04
#define GL_VERTEX_ATTRIB_MAP1_SIZE_APPLE                              0x8A02
#define GL_VERTEX_ATTRIB_MAP2_APPLE                                   0x8A01
#define GL_VERTEX_ATTRIB_MAP2_COEFF_APPLE                             0x8A07
#define GL_VERTEX_ATTRIB_MAP2_DOMAIN_APPLE                            0x8A09
#define GL_VERTEX_ATTRIB_MAP2_ORDER_APPLE                             0x8A08
#define GL_VERTEX_ATTRIB_MAP2_SIZE_APPLE                              0x8A06
#define GL_VERTEX_ATTRIB_RELATIVE_OFFSET                              0x82D5
#define GL_VERTEX_BINDING_BUFFER                                      0x8F4F
#define GL_VERTEX_BINDING_DIVISOR                                     0x82D6
#define GL_VERTEX_BINDING_OFFSET                                      0x82D7
#define GL_VERTEX_BINDING_STRIDE                                      0x82D8
#define GL_VERTEX_BLEND_ARB                                           0x86A7
#define GL_VERTEX_CONSISTENT_HINT_PGI                                 0x1A22B
#define GL_VERTEX_DATA_HINT_PGI                                       0x1A22A
#define GL_VERTEX_ELEMENT_SWIZZLE_AMD                                 0x91A4
#define GL_VERTEX_ID_NV                                               0x8C7B
#define GL_VERTEX_ID_SWIZZLE_AMD                                      0x91A5
#define GL_VERTEX_PRECLIP_HINT_SGIX                                   0x83EF
#define GL_VERTEX_PRECLIP_SGIX                                        0x83EE
#define GL_VERTEX_PROGRAM_ARB                                         0x8620
#define GL_VERTEX_PROGRAM_BINDING_NV                                  0x864A
#define GL_VERTEX_PROGRAM_NV                                          0x8620
#define GL_VERTEX_PROGRAM_PARAMETER_BUFFER_NV                         0x8DA2
#define GL_VERTEX_PROGRAM_POINT_SIZE                                  0x8642
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB                              0x8642
#define GL_VERTEX_PROGRAM_POINT_SIZE_NV                               0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE                                    0x8643
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB                                0x8643
#define GL_VERTEX_PROGRAM_TWO_SIDE_NV                                 0x8643
#define GL_VERTEX_SHADER                                              0x8B31
#define GL_VERTEX_SHADER_ARB                                          0x8B31
#define GL_VERTEX_SHADER_BINDING_EXT                                  0x8781
#define GL_VERTEX_SHADER_BIT                                          0x00000001
#define GL_VERTEX_SHADER_BIT_EXT                                      0x00000001
#define GL_VERTEX_SHADER_EXT                                          0x8780
#define GL_VERTEX_SHADER_INSTRUCTIONS_EXT                             0x87CF
#define GL_VERTEX_SHADER_INVARIANTS_EXT                               0x87D1
#define GL_VERTEX_SHADER_INVOCATIONS                                  0x82F0
#define GL_VERTEX_SHADER_INVOCATIONS_ARB                              0x82F0
#define GL_VERTEX_SHADER_LOCALS_EXT                                   0x87D3
#define GL_VERTEX_SHADER_LOCAL_CONSTANTS_EXT                          0x87D2
#define GL_VERTEX_SHADER_OPTIMIZED_EXT                                0x87D4
#define GL_VERTEX_SHADER_VARIANTS_EXT                                 0x87D0
#define GL_VERTEX_SOURCE_ATI                                          0x8774
#define GL_VERTEX_STATE_PROGRAM_NV                                    0x8621
#define GL_VERTEX_STREAM0_ATI                                         0x876C
#define GL_VERTEX_STREAM1_ATI                                         0x876D
#define GL_VERTEX_STREAM2_ATI                                         0x876E
#define GL_VERTEX_STREAM3_ATI                                         0x876F
#define GL_VERTEX_STREAM4_ATI                                         0x8770
#define GL_VERTEX_STREAM5_ATI                                         0x8771
#define GL_VERTEX_STREAM6_ATI                                         0x8772
#define GL_VERTEX_STREAM7_ATI                                         0x8773
#define GL_VERTEX_SUBROUTINE                                          0x92E8
#define GL_VERTEX_SUBROUTINE_UNIFORM                                  0x92EE
#define GL_VERTEX_TEXTURE                                             0x829B
#define GL_VERTEX_WEIGHTING_EXT                                       0x8509
#define GL_VERTEX_WEIGHT_ARRAY_EXT                                    0x850C
#define GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT                            0x8510
#define GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT                               0x850D
#define GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT                             0x850F
#define GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT                               0x850E
#define GL_VERTICAL_LINE_TO_NV                                        0x08
#define GL_VERTICES_SUBMITTED                                         0x82EE
#define GL_VERTICES_SUBMITTED_ARB                                     0x82EE
#define GL_VIBRANCE_BIAS_NV                                           0x8719
#define GL_VIBRANCE_SCALE_NV                                          0x8713
#define GL_VIDEO_BUFFER_BINDING_NV                                    0x9021
#define GL_VIDEO_BUFFER_INTERNAL_FORMAT_NV                            0x902D
#define GL_VIDEO_BUFFER_NV                                            0x9020
#define GL_VIDEO_BUFFER_PITCH_NV                                      0x9028
#define GL_VIDEO_CAPTURE_FIELD_LOWER_HEIGHT_NV                        0x903B
#define GL_VIDEO_CAPTURE_FIELD_UPPER_HEIGHT_NV                        0x903A
#define GL_VIDEO_CAPTURE_FRAME_HEIGHT_NV                              0x9039
#define GL_VIDEO_CAPTURE_FRAME_WIDTH_NV                               0x9038
#define GL_VIDEO_CAPTURE_SURFACE_ORIGIN_NV                            0x903C
#define GL_VIDEO_CAPTURE_TO_422_SUPPORTED_NV                          0x9026
#define GL_VIDEO_COLOR_CONVERSION_MATRIX_NV                           0x9029
#define GL_VIDEO_COLOR_CONVERSION_MAX_NV                              0x902A
#define GL_VIDEO_COLOR_CONVERSION_MIN_NV                              0x902B
#define GL_VIDEO_COLOR_CONVERSION_OFFSET_NV                           0x902C
#define GL_VIEWPORT                                                   0x0BA2
#define GL_VIEWPORT_BIT                                               0x00000800
#define GL_VIEWPORT_BOUNDS_RANGE                                      0x825D
#define GL_VIEWPORT_COMMAND_NV                                        0x0010
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX                            0x825F
#define GL_VIEWPORT_POSITION_W_SCALE_NV                               0x937C
#define GL_VIEWPORT_POSITION_W_SCALE_X_COEFF_NV                       0x937D
#define GL_VIEWPORT_POSITION_W_SCALE_Y_COEFF_NV                       0x937E
#define GL_VIEWPORT_SUBPIXEL_BITS                                     0x825C
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_W_NV                             0x9357
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV                             0x9351
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV                             0x9353
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV                             0x9355
#define GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV                             0x9356
#define GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV                             0x9350
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV                             0x9352
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV                             0x9354
#define GL_VIEWPORT_SWIZZLE_W_NV                                      0x935B
#define GL_VIEWPORT_SWIZZLE_X_NV                                      0x9358
#define GL_VIEWPORT_SWIZZLE_Y_NV                                      0x9359
#define GL_VIEWPORT_SWIZZLE_Z_NV                                      0x935A
#define GL_VIEW_CLASS_128_BITS                                        0x82C4
#define GL_VIEW_CLASS_16_BITS                                         0x82CA
#define GL_VIEW_CLASS_24_BITS                                         0x82C9
#define GL_VIEW_CLASS_32_BITS                                         0x82C8
#define GL_VIEW_CLASS_48_BITS                                         0x82C7
#define GL_VIEW_CLASS_64_BITS                                         0x82C6
#define GL_VIEW_CLASS_8_BITS                                          0x82CB
#define GL_VIEW_CLASS_96_BITS                                         0x82C5
#define GL_VIEW_CLASS_ASTC_10x10_RGBA                                 0x9393
#define GL_VIEW_CLASS_ASTC_10x5_RGBA                                  0x9390
#define GL_VIEW_CLASS_ASTC_10x6_RGBA                                  0x9391
#define GL_VIEW_CLASS_ASTC_10x8_RGBA                                  0x9392
#define GL_VIEW_CLASS_ASTC_12x10_RGBA                                 0x9394
#define GL_VIEW_CLASS_ASTC_12x12_RGBA                                 0x9395
#define GL_VIEW_CLASS_ASTC_4x4_RGBA                                   0x9388
#define GL_VIEW_CLASS_ASTC_5x4_RGBA                                   0x9389
#define GL_VIEW_CLASS_ASTC_5x5_RGBA                                   0x938A
#define GL_VIEW_CLASS_ASTC_6x5_RGBA                                   0x938B
#define GL_VIEW_CLASS_ASTC_6x6_RGBA                                   0x938C
#define GL_VIEW_CLASS_ASTC_8x5_RGBA                                   0x938D
#define GL_VIEW_CLASS_ASTC_8x6_RGBA                                   0x938E
#define GL_VIEW_CLASS_ASTC_8x8_RGBA                                   0x938F
#define GL_VIEW_CLASS_BPTC_FLOAT                                      0x82D3
#define GL_VIEW_CLASS_BPTC_UNORM                                      0x82D2
#define GL_VIEW_CLASS_EAC_R11                                         0x9383
#define GL_VIEW_CLASS_EAC_RG11                                        0x9384
#define GL_VIEW_CLASS_ETC2_EAC_RGBA                                   0x9387
#define GL_VIEW_CLASS_ETC2_RGB                                        0x9385
#define GL_VIEW_CLASS_ETC2_RGBA                                       0x9386
#define GL_VIEW_CLASS_RGTC1_RED                                       0x82D0
#define GL_VIEW_CLASS_RGTC2_RG                                        0x82D1
#define GL_VIEW_CLASS_S3TC_DXT1_RGB                                   0x82CC
#define GL_VIEW_CLASS_S3TC_DXT1_RGBA                                  0x82CD
#define GL_VIEW_CLASS_S3TC_DXT3_RGBA                                  0x82CE
#define GL_VIEW_CLASS_S3TC_DXT5_RGBA                                  0x82CF
#define GL_VIEW_COMPATIBILITY_CLASS                                   0x82B6
#define GL_VIRTUAL_PAGE_SIZE_INDEX_ARB                                0x91A7
#define GL_VIRTUAL_PAGE_SIZE_X_AMD                                    0x9195
#define GL_VIRTUAL_PAGE_SIZE_X_ARB                                    0x9195
#define GL_VIRTUAL_PAGE_SIZE_Y_AMD                                    0x9196
#define GL_VIRTUAL_PAGE_SIZE_Y_ARB                                    0x9196
#define GL_VIRTUAL_PAGE_SIZE_Z_AMD                                    0x9197
#define GL_VIRTUAL_PAGE_SIZE_Z_ARB                                    0x9197
#define GL_VIVIDLIGHT_NV                                              0x92A6
#define GL_VOLATILE_APPLE                                             0x8A1A
#define GL_WAIT_FAILED                                                0x911D
#define GL_WARPS_PER_SM_NV                                            0x933A
#define GL_WARP_SIZE_NV                                               0x9339
#define GL_WEIGHTED_AVERAGE_ARB                                       0x9367
#define GL_WEIGHTED_AVERAGE_EXT                                       0x9367
#define GL_WEIGHT_ARRAY_ARB                                           0x86AD
#define GL_WEIGHT_ARRAY_BUFFER_BINDING                                0x889E
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB                            0x889E
#define GL_WEIGHT_ARRAY_POINTER_ARB                                   0x86AC
#define GL_WEIGHT_ARRAY_SIZE_ARB                                      0x86AB
#define GL_WEIGHT_ARRAY_STRIDE_ARB                                    0x86AA
#define GL_WEIGHT_ARRAY_TYPE_ARB                                      0x86A9
#define GL_WEIGHT_SUM_UNITY_ARB                                       0x86A6
#define GL_WIDE_LINE_HINT_PGI                                         0x1A222
#define GL_WINDOW_RECTANGLE_EXT                                       0x8F12
#define GL_WINDOW_RECTANGLE_MODE_EXT                                  0x8F13
#define GL_WRAP_BORDER_SUN                                            0x81D4
#define GL_WRITE_DISCARD_NV                                           0x88BE
#define GL_WRITE_ONLY                                                 0x88B9
#define GL_WRITE_ONLY_ARB                                             0x88B9
#define GL_WRITE_PIXEL_DATA_RANGE_LENGTH_NV                           0x887A
#define GL_WRITE_PIXEL_DATA_RANGE_NV                                  0x8878
#define GL_WRITE_PIXEL_DATA_RANGE_POINTER_NV                          0x887C
#define GL_W_EXT                                                      0x87D8
#define GL_XOR                                                        0x1506
#define GL_XOR_NV                                                     0x1506
#define GL_X_EXT                                                      0x87D5
#define GL_YCBAYCR8A_4224_NV                                          0x9032
#define GL_YCBCR_422_APPLE                                            0x85B9
#define GL_YCBCR_MESA                                                 0x8757
#define GL_YCBYCR8_422_NV                                             0x9031
#define GL_YCRCBA_SGIX                                                0x8319
#define GL_YCRCB_422_SGIX                                             0x81BB
#define GL_YCRCB_444_SGIX                                             0x81BC
#define GL_YCRCB_SGIX                                                 0x8318
#define GL_Y_EXT                                                      0x87D6
#define GL_Z4Y12Z4CB12Z4A12Z4Y12Z4CR12Z4A12_4224_NV                   0x9036
#define GL_Z4Y12Z4CB12Z4CR12_444_NV                                   0x9037
#define GL_Z4Y12Z4CB12Z4Y12Z4CR12_422_NV                              0x9035
#define GL_Z6Y10Z6CB10Z6A10Z6Y10Z6CR10Z6A10_4224_NV                   0x9034
#define GL_Z6Y10Z6CB10Z6Y10Z6CR10_422_NV                              0x9033
#define GL_ZERO                                                       0
#define GL_ZERO_EXT                                                   0x87DD
#define GL_ZERO_TO_ONE                                                0x935F
#define GL_ZOOM_X                                                     0x0D16
#define GL_ZOOM_Y                                                     0x0D17
#define GL_Z_EXT                                                      0x87D7
#define WGL_ACCELERATION_ARB                                          0x2003
#define WGL_ACCUM_ALPHA_BITS_ARB                                      0x2021
#define WGL_ACCUM_BITS_ARB                                            0x201D
#define WGL_ACCUM_BLUE_BITS_ARB                                       0x2020
#define WGL_ACCUM_GREEN_BITS_ARB                                      0x201F
#define WGL_ACCUM_RED_BITS_ARB                                        0x201E
#define WGL_ALPHA_BITS_ARB                                            0x201B
#define WGL_ALPHA_SHIFT_ARB                                           0x201C
#define WGL_AUX0_ARB                                                  0x2087
#define WGL_AUX1_ARB                                                  0x2088
#define WGL_AUX2_ARB                                                  0x2089
#define WGL_AUX3_ARB                                                  0x208A
#define WGL_AUX4_ARB                                                  0x208B
#define WGL_AUX5_ARB                                                  0x208C
#define WGL_AUX6_ARB                                                  0x208D
#define WGL_AUX7_ARB                                                  0x208E
#define WGL_AUX8_ARB                                                  0x208F
#define WGL_AUX9_ARB                                                  0x2090
#define WGL_AUX_BUFFERS_ARB                                           0x2024
#define WGL_BACK_LEFT_ARB                                             0x2085
#define WGL_BACK_RIGHT_ARB                                            0x2086
#define WGL_BIND_TO_TEXTURE_DEPTH_NV                                  0x20A3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_DEPTH_NV                        0x20A4
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV                   0x20B4
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV                    0x20B3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV                     0x20B2
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV                      0x20B1
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV                         0x20A1
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV                          0x20A0
#define WGL_BIND_TO_TEXTURE_RGBA_ARB                                  0x2071
#define WGL_BIND_TO_TEXTURE_RGB_ARB                                   0x2070
#define WGL_BLUE_BITS_ARB                                             0x2019
#define WGL_BLUE_SHIFT_ARB                                            0x201A
#define WGL_COLOR_BITS_ARB                                            0x2014
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB                     0x00000002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB                              0x00000001
#define WGL_CONTEXT_DEBUG_BIT_ARB                                     0x00000001
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT                               0x00000004
#define WGL_CONTEXT_FLAGS_ARB                                         0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB                        0x00000002
#define WGL_CONTEXT_LAYER_PLANE_ARB                                   0x2093
#define WGL_CONTEXT_MAJOR_VERSION_ARB                                 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB                                 0x2092
#define WGL_CONTEXT_OPENGL_NO_ERROR_ARB                               0x31B3
#define WGL_CONTEXT_PROFILE_MASK_ARB                                  0x9126
#define WGL_CUBE_MAP_FACE_ARB                                         0x207C
#define WGL_DEPTH_BITS_ARB                                            0x2022
#define WGL_DEPTH_COMPONENT_NV                                        0x20A7
#define WGL_DEPTH_TEXTURE_FORMAT_NV                                   0x20A5
#define WGL_DOUBLE_BUFFER_ARB                                         0x2011
#define WGL_DRAW_TO_BITMAP_ARB                                        0x2002
#define WGL_DRAW_TO_PBUFFER_ARB                                       0x202D
#define WGL_DRAW_TO_WINDOW_ARB                                        0x2001
#define WGL_FLOAT_COMPONENTS_NV                                       0x20B0
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB                              0x20A9
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT                              0x20A9
#define WGL_FRONT_LEFT_ARB                                            0x2083
#define WGL_FRONT_RIGHT_ARB                                           0x2084
#define WGL_FULL_ACCELERATION_ARB                                     0x2027
#define WGL_GENERIC_ACCELERATION_ARB                                  0x2026
#define WGL_GREEN_BITS_ARB                                            0x2017
#define WGL_GREEN_SHIFT_ARB                                           0x2018
#define WGL_MAX_PBUFFER_HEIGHT_ARB                                    0x2030
#define WGL_MAX_PBUFFER_PIXELS_ARB                                    0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB                                     0x202F
#define WGL_MIPMAP_LEVEL_ARB                                          0x207B
#define WGL_MIPMAP_TEXTURE_ARB                                        0x2074
#define WGL_NEED_PALETTE_ARB                                          0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB                                   0x2005
#define WGL_NO_ACCELERATION_ARB                                       0x2025
#define WGL_NO_TEXTURE_ARB                                            0x2077
#define WGL_NUMBER_OVERLAYS_ARB                                       0x2008
#define WGL_NUMBER_PIXEL_FORMATS_ARB                                  0x2000
#define WGL_NUMBER_UNDERLAYS_ARB                                      0x2009
#define WGL_PBUFFER_HEIGHT_ARB                                        0x2035
#define WGL_PBUFFER_LARGEST_ARB                                       0x2033
#define WGL_PBUFFER_LOST_ARB                                          0x2036
#define WGL_PBUFFER_WIDTH_ARB                                         0x2034
#define WGL_PIXEL_TYPE_ARB                                            0x2013
#define WGL_RED_BITS_ARB                                              0x2015
#define WGL_RED_SHIFT_ARB                                             0x2016
#define WGL_RENDERER_ACCELERATED_WINE                                 0x8186
#define WGL_RENDERER_DEVICE_ID_WINE                                   0x8184
#define WGL_RENDERER_ID_WINE                                          0x818E
#define WGL_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_WINE        0x818B
#define WGL_RENDERER_OPENGL_CORE_PROFILE_VERSION_WINE                 0x818A
#define WGL_RENDERER_OPENGL_ES2_PROFILE_VERSION_WINE                  0x818D
#define WGL_RENDERER_OPENGL_ES_PROFILE_VERSION_WINE                   0x818C
#define WGL_RENDERER_PREFERRED_PROFILE_WINE                           0x8189
#define WGL_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_WINE                 0x8188
#define WGL_RENDERER_VENDOR_ID_WINE                                   0x8183
#define WGL_RENDERER_VERSION_WINE                                     0x8185
#define WGL_RENDERER_VIDEO_MEMORY_WINE                                0x8187
#define WGL_SAMPLES_ARB                                               0x2042
#define WGL_SAMPLE_BUFFERS_ARB                                        0x2041
#define WGL_SHARE_ACCUM_ARB                                           0x200E
#define WGL_SHARE_DEPTH_ARB                                           0x200C
#define WGL_SHARE_STENCIL_ARB                                         0x200D
#define WGL_STENCIL_BITS_ARB                                          0x2023
#define WGL_STEREO_ARB                                                0x2012
#define WGL_SUPPORT_GDI_ARB                                           0x200F
#define WGL_SUPPORT_OPENGL_ARB                                        0x2010
#define WGL_SWAP_COPY_ARB                                             0x2029
#define WGL_SWAP_EXCHANGE_ARB                                         0x2028
#define WGL_SWAP_LAYER_BUFFERS_ARB                                    0x2006
#define WGL_SWAP_METHOD_ARB                                           0x2007
#define WGL_SWAP_UNDEFINED_ARB                                        0x202A
#define WGL_TEXTURE_1D_ARB                                            0x2079
#define WGL_TEXTURE_2D_ARB                                            0x207A
#define WGL_TEXTURE_CUBE_MAP_ARB                                      0x2078
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB                           0x207E
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB                           0x2080
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB                           0x2082
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB                           0x207D
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB                           0x207F
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB                           0x2081
#define WGL_TEXTURE_DEPTH_COMPONENT_NV                                0x20A6
#define WGL_TEXTURE_FLOAT_RGBA_NV                                     0x20B8
#define WGL_TEXTURE_FLOAT_RGB_NV                                      0x20B7
#define WGL_TEXTURE_FLOAT_RG_NV                                       0x20B6
#define WGL_TEXTURE_FLOAT_R_NV                                        0x20B5
#define WGL_TEXTURE_FORMAT_ARB                                        0x2072
#define WGL_TEXTURE_RECTANGLE_NV                                      0x20A2
#define WGL_TEXTURE_RGBA_ARB                                          0x2076
#define WGL_TEXTURE_RGB_ARB                                           0x2075
#define WGL_TEXTURE_TARGET_ARB                                        0x2073
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB                               0x203A
#define WGL_TRANSPARENT_ARB                                           0x200A
#define WGL_TRANSPARENT_BLUE_VALUE_ARB                                0x2039
#define WGL_TRANSPARENT_GREEN_VALUE_ARB                               0x2038
#define WGL_TRANSPARENT_INDEX_VALUE_ARB                               0x203B
#define WGL_TRANSPARENT_RED_VALUE_ARB                                 0x2037
#define WGL_TYPE_COLORINDEX_ARB                                       0x202C
#define WGL_TYPE_RGBA_ARB                                             0x202B
#define WGL_TYPE_RGBA_FLOAT_ARB                                       0x21A0
#define WGL_TYPE_RGBA_FLOAT_ATI                                       0x21A0
#define WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT                              0x20A8

#ifndef GL_NO_PROTOTYPES
void       GLAPIENTRY glAccum( GLenum op, GLfloat value );
void       GLAPIENTRY glAlphaFunc( GLenum func, GLfloat ref );
GLboolean  GLAPIENTRY glAreTexturesResident( GLsizei n, const GLuint *textures, GLboolean *residences );
void       GLAPIENTRY glArrayElement( GLint i );
void       GLAPIENTRY glBegin( GLenum mode );
void       GLAPIENTRY glBindTexture( GLenum target, GLuint texture );
void       GLAPIENTRY glBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
void       GLAPIENTRY glBlendFunc( GLenum sfactor, GLenum dfactor );
void       GLAPIENTRY glCallList( GLuint list );
void       GLAPIENTRY glCallLists( GLsizei n, GLenum type, const void *lists );
void       GLAPIENTRY glClear( GLbitfield mask );
void       GLAPIENTRY glClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
void       GLAPIENTRY glClearColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
void       GLAPIENTRY glClearDepth( GLdouble depth );
void       GLAPIENTRY glClearIndex( GLfloat c );
void       GLAPIENTRY glClearStencil( GLint s );
void       GLAPIENTRY glClipPlane( GLenum plane, const GLdouble *equation );
void       GLAPIENTRY glColor3b( GLbyte red, GLbyte green, GLbyte blue );
void       GLAPIENTRY glColor3bv( const GLbyte *v );
void       GLAPIENTRY glColor3d( GLdouble red, GLdouble green, GLdouble blue );
void       GLAPIENTRY glColor3dv( const GLdouble *v );
void       GLAPIENTRY glColor3f( GLfloat red, GLfloat green, GLfloat blue );
void       GLAPIENTRY glColor3fv( const GLfloat *v );
void       GLAPIENTRY glColor3i( GLint red, GLint green, GLint blue );
void       GLAPIENTRY glColor3iv( const GLint *v );
void       GLAPIENTRY glColor3s( GLshort red, GLshort green, GLshort blue );
void       GLAPIENTRY glColor3sv( const GLshort *v );
void       GLAPIENTRY glColor3ub( GLubyte red, GLubyte green, GLubyte blue );
void       GLAPIENTRY glColor3ubv( const GLubyte *v );
void       GLAPIENTRY glColor3ui( GLuint red, GLuint green, GLuint blue );
void       GLAPIENTRY glColor3uiv( const GLuint *v );
void       GLAPIENTRY glColor3us( GLushort red, GLushort green, GLushort blue );
void       GLAPIENTRY glColor3usv( const GLushort *v );
void       GLAPIENTRY glColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );
void       GLAPIENTRY glColor4bv( const GLbyte *v );
void       GLAPIENTRY glColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );
void       GLAPIENTRY glColor4dv( const GLdouble *v );
void       GLAPIENTRY glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
void       GLAPIENTRY glColor4fv( const GLfloat *v );
void       GLAPIENTRY glColor4i( GLint red, GLint green, GLint blue, GLint alpha );
void       GLAPIENTRY glColor4iv( const GLint *v );
void       GLAPIENTRY glColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha );
void       GLAPIENTRY glColor4sv( const GLshort *v );
void       GLAPIENTRY glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
void       GLAPIENTRY glColor4ubv( const GLubyte *v );
void       GLAPIENTRY glColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha );
void       GLAPIENTRY glColor4uiv( const GLuint *v );
void       GLAPIENTRY glColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha );
void       GLAPIENTRY glColor4usv( const GLushort *v );
void       GLAPIENTRY glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
void       GLAPIENTRY glColorMaterial( GLenum face, GLenum mode );
void       GLAPIENTRY glColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer );
void       GLAPIENTRY glCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type );
void       GLAPIENTRY glCopyTexImage1D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
void       GLAPIENTRY glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
void       GLAPIENTRY glCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
void       GLAPIENTRY glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
void       GLAPIENTRY glCullFace( GLenum mode );
GLint      GLAPIENTRY glDebugEntry( GLint unknown1, GLint unknown2 );
void       GLAPIENTRY glDeleteLists( GLuint list, GLsizei range );
void       GLAPIENTRY glDeleteTextures( GLsizei n, const GLuint *textures );
void       GLAPIENTRY glDepthFunc( GLenum func );
void       GLAPIENTRY glDepthMask( GLboolean flag );
void       GLAPIENTRY glDepthRange( GLdouble n, GLdouble f );
void       GLAPIENTRY glDisable( GLenum cap );
void       GLAPIENTRY glDisableClientState( GLenum array );
void       GLAPIENTRY glDrawArrays( GLenum mode, GLint first, GLsizei count );
void       GLAPIENTRY glDrawBuffer( GLenum buf );
void       GLAPIENTRY glDrawElements( GLenum mode, GLsizei count, GLenum type, const void *indices );
void       GLAPIENTRY glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
void       GLAPIENTRY glEdgeFlag( GLboolean flag );
void       GLAPIENTRY glEdgeFlagPointer( GLsizei stride, const void *pointer );
void       GLAPIENTRY glEdgeFlagv( const GLboolean *flag );
void       GLAPIENTRY glEnable( GLenum cap );
void       GLAPIENTRY glEnableClientState( GLenum array );
void       GLAPIENTRY glEnd(void);
void       GLAPIENTRY glEndList(void);
void       GLAPIENTRY glEvalCoord1d( GLdouble u );
void       GLAPIENTRY glEvalCoord1dv( const GLdouble *u );
void       GLAPIENTRY glEvalCoord1f( GLfloat u );
void       GLAPIENTRY glEvalCoord1fv( const GLfloat *u );
void       GLAPIENTRY glEvalCoord2d( GLdouble u, GLdouble v );
void       GLAPIENTRY glEvalCoord2dv( const GLdouble *u );
void       GLAPIENTRY glEvalCoord2f( GLfloat u, GLfloat v );
void       GLAPIENTRY glEvalCoord2fv( const GLfloat *u );
void       GLAPIENTRY glEvalMesh1( GLenum mode, GLint i1, GLint i2 );
void       GLAPIENTRY glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
void       GLAPIENTRY glEvalPoint1( GLint i );
void       GLAPIENTRY glEvalPoint2( GLint i, GLint j );
void       GLAPIENTRY glFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer );
void       GLAPIENTRY glFinish(void);
void       GLAPIENTRY glFlush(void);
void       GLAPIENTRY glFogf( GLenum pname, GLfloat param );
void       GLAPIENTRY glFogfv( GLenum pname, const GLfloat *params );
void       GLAPIENTRY glFogi( GLenum pname, GLint param );
void       GLAPIENTRY glFogiv( GLenum pname, const GLint *params );
void       GLAPIENTRY glFrontFace( GLenum mode );
void       GLAPIENTRY glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
GLuint     GLAPIENTRY glGenLists( GLsizei range );
void       GLAPIENTRY glGenTextures( GLsizei n, GLuint *textures );
void       GLAPIENTRY glGetBooleanv( GLenum pname, GLboolean *data );
void       GLAPIENTRY glGetClipPlane( GLenum plane, GLdouble *equation );
void       GLAPIENTRY glGetDoublev( GLenum pname, GLdouble *data );
GLenum     GLAPIENTRY glGetError(void);
void       GLAPIENTRY glGetFloatv( GLenum pname, GLfloat *data );
void       GLAPIENTRY glGetIntegerv( GLenum pname, GLint *data );
void       GLAPIENTRY glGetLightfv( GLenum light, GLenum pname, GLfloat *params );
void       GLAPIENTRY glGetLightiv( GLenum light, GLenum pname, GLint *params );
void       GLAPIENTRY glGetMapdv( GLenum target, GLenum query, GLdouble *v );
void       GLAPIENTRY glGetMapfv( GLenum target, GLenum query, GLfloat *v );
void       GLAPIENTRY glGetMapiv( GLenum target, GLenum query, GLint *v );
void       GLAPIENTRY glGetMaterialfv( GLenum face, GLenum pname, GLfloat *params );
void       GLAPIENTRY glGetMaterialiv( GLenum face, GLenum pname, GLint *params );
void       GLAPIENTRY glGetPixelMapfv( GLenum map, GLfloat *values );
void       GLAPIENTRY glGetPixelMapuiv( GLenum map, GLuint *values );
void       GLAPIENTRY glGetPixelMapusv( GLenum map, GLushort *values );
void       GLAPIENTRY glGetPointerv( GLenum pname, void **params );
void       GLAPIENTRY glGetPolygonStipple( GLubyte *mask );
const GLubyte * GLAPIENTRY glGetString( GLenum name );
void       GLAPIENTRY glGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params );
void       GLAPIENTRY glGetTexEnviv( GLenum target, GLenum pname, GLint *params );
void       GLAPIENTRY glGetTexGendv( GLenum coord, GLenum pname, GLdouble *params );
void       GLAPIENTRY glGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params );
void       GLAPIENTRY glGetTexGeniv( GLenum coord, GLenum pname, GLint *params );
void       GLAPIENTRY glGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
void       GLAPIENTRY glGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat *params );
void       GLAPIENTRY glGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint *params );
void       GLAPIENTRY glGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params );
void       GLAPIENTRY glGetTexParameteriv( GLenum target, GLenum pname, GLint *params );
void       GLAPIENTRY glHint( GLenum target, GLenum mode );
void       GLAPIENTRY glIndexMask( GLuint mask );
void       GLAPIENTRY glIndexPointer( GLenum type, GLsizei stride, const void *pointer );
void       GLAPIENTRY glIndexd( GLdouble c );
void       GLAPIENTRY glIndexdv( const GLdouble *c );
void       GLAPIENTRY glIndexf( GLfloat c );
void       GLAPIENTRY glIndexfv( const GLfloat *c );
void       GLAPIENTRY glIndexi( GLint c );
void       GLAPIENTRY glIndexiv( const GLint *c );
void       GLAPIENTRY glIndexs( GLshort c );
void       GLAPIENTRY glIndexsv( const GLshort *c );
void       GLAPIENTRY glIndexub( GLubyte c );
void       GLAPIENTRY glIndexubv( const GLubyte *c );
void       GLAPIENTRY glInitNames(void);
void       GLAPIENTRY glInterleavedArrays( GLenum format, GLsizei stride, const void *pointer );
GLboolean  GLAPIENTRY glIsEnabled( GLenum cap );
GLboolean  GLAPIENTRY glIsList( GLuint list );
GLboolean  GLAPIENTRY glIsTexture( GLuint texture );
void       GLAPIENTRY glLightModelf( GLenum pname, GLfloat param );
void       GLAPIENTRY glLightModelfv( GLenum pname, const GLfloat *params );
void       GLAPIENTRY glLightModeli( GLenum pname, GLint param );
void       GLAPIENTRY glLightModeliv( GLenum pname, const GLint *params );
void       GLAPIENTRY glLightf( GLenum light, GLenum pname, GLfloat param );
void       GLAPIENTRY glLightfv( GLenum light, GLenum pname, const GLfloat *params );
void       GLAPIENTRY glLighti( GLenum light, GLenum pname, GLint param );
void       GLAPIENTRY glLightiv( GLenum light, GLenum pname, const GLint *params );
void       GLAPIENTRY glLineStipple( GLint factor, GLushort pattern );
void       GLAPIENTRY glLineWidth( GLfloat width );
void       GLAPIENTRY glListBase( GLuint base );
void       GLAPIENTRY glLoadIdentity(void);
void       GLAPIENTRY glLoadMatrixd( const GLdouble *m );
void       GLAPIENTRY glLoadMatrixf( const GLfloat *m );
void       GLAPIENTRY glLoadName( GLuint name );
void       GLAPIENTRY glLogicOp( GLenum opcode );
void       GLAPIENTRY glMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
void       GLAPIENTRY glMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
void       GLAPIENTRY glMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
void       GLAPIENTRY glMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
void       GLAPIENTRY glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 );
void       GLAPIENTRY glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 );
void       GLAPIENTRY glMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 );
void       GLAPIENTRY glMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 );
void       GLAPIENTRY glMaterialf( GLenum face, GLenum pname, GLfloat param );
void       GLAPIENTRY glMaterialfv( GLenum face, GLenum pname, const GLfloat *params );
void       GLAPIENTRY glMateriali( GLenum face, GLenum pname, GLint param );
void       GLAPIENTRY glMaterialiv( GLenum face, GLenum pname, const GLint *params );
void       GLAPIENTRY glMatrixMode( GLenum mode );
void       GLAPIENTRY glMultMatrixd( const GLdouble *m );
void       GLAPIENTRY glMultMatrixf( const GLfloat *m );
void       GLAPIENTRY glNewList( GLuint list, GLenum mode );
void       GLAPIENTRY glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz );
void       GLAPIENTRY glNormal3bv( const GLbyte *v );
void       GLAPIENTRY glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz );
void       GLAPIENTRY glNormal3dv( const GLdouble *v );
void       GLAPIENTRY glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz );
void       GLAPIENTRY glNormal3fv( const GLfloat *v );
void       GLAPIENTRY glNormal3i( GLint nx, GLint ny, GLint nz );
void       GLAPIENTRY glNormal3iv( const GLint *v );
void       GLAPIENTRY glNormal3s( GLshort nx, GLshort ny, GLshort nz );
void       GLAPIENTRY glNormal3sv( const GLshort *v );
void       GLAPIENTRY glNormalPointer( GLenum type, GLsizei stride, const void *pointer );
void       GLAPIENTRY glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
void       GLAPIENTRY glPassThrough( GLfloat token );
void       GLAPIENTRY glPixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values );
void       GLAPIENTRY glPixelMapuiv( GLenum map, GLsizei mapsize, const GLuint *values );
void       GLAPIENTRY glPixelMapusv( GLenum map, GLsizei mapsize, const GLushort *values );
void       GLAPIENTRY glPixelStoref( GLenum pname, GLfloat param );
void       GLAPIENTRY glPixelStorei( GLenum pname, GLint param );
void       GLAPIENTRY glPixelTransferf( GLenum pname, GLfloat param );
void       GLAPIENTRY glPixelTransferi( GLenum pname, GLint param );
void       GLAPIENTRY glPixelZoom( GLfloat xfactor, GLfloat yfactor );
void       GLAPIENTRY glPointSize( GLfloat size );
void       GLAPIENTRY glPolygonMode( GLenum face, GLenum mode );
void       GLAPIENTRY glPolygonOffset( GLfloat factor, GLfloat units );
void       GLAPIENTRY glPolygonStipple( const GLubyte *mask );
void       GLAPIENTRY glPopAttrib(void);
void       GLAPIENTRY glPopClientAttrib(void);
void       GLAPIENTRY glPopMatrix(void);
void       GLAPIENTRY glPopName(void);
void       GLAPIENTRY glPrioritizeTextures( GLsizei n, const GLuint *textures, const GLfloat *priorities );
void       GLAPIENTRY glPushAttrib( GLbitfield mask );
void       GLAPIENTRY glPushClientAttrib( GLbitfield mask );
void       GLAPIENTRY glPushMatrix(void);
void       GLAPIENTRY glPushName( GLuint name );
void       GLAPIENTRY glRasterPos2d( GLdouble x, GLdouble y );
void       GLAPIENTRY glRasterPos2dv( const GLdouble *v );
void       GLAPIENTRY glRasterPos2f( GLfloat x, GLfloat y );
void       GLAPIENTRY glRasterPos2fv( const GLfloat *v );
void       GLAPIENTRY glRasterPos2i( GLint x, GLint y );
void       GLAPIENTRY glRasterPos2iv( const GLint *v );
void       GLAPIENTRY glRasterPos2s( GLshort x, GLshort y );
void       GLAPIENTRY glRasterPos2sv( const GLshort *v );
void       GLAPIENTRY glRasterPos3d( GLdouble x, GLdouble y, GLdouble z );
void       GLAPIENTRY glRasterPos3dv( const GLdouble *v );
void       GLAPIENTRY glRasterPos3f( GLfloat x, GLfloat y, GLfloat z );
void       GLAPIENTRY glRasterPos3fv( const GLfloat *v );
void       GLAPIENTRY glRasterPos3i( GLint x, GLint y, GLint z );
void       GLAPIENTRY glRasterPos3iv( const GLint *v );
void       GLAPIENTRY glRasterPos3s( GLshort x, GLshort y, GLshort z );
void       GLAPIENTRY glRasterPos3sv( const GLshort *v );
void       GLAPIENTRY glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
void       GLAPIENTRY glRasterPos4dv( const GLdouble *v );
void       GLAPIENTRY glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
void       GLAPIENTRY glRasterPos4fv( const GLfloat *v );
void       GLAPIENTRY glRasterPos4i( GLint x, GLint y, GLint z, GLint w );
void       GLAPIENTRY glRasterPos4iv( const GLint *v );
void       GLAPIENTRY glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w );
void       GLAPIENTRY glRasterPos4sv( const GLshort *v );
void       GLAPIENTRY glReadBuffer( GLenum src );
void       GLAPIENTRY glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels );
void       GLAPIENTRY glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );
void       GLAPIENTRY glRectdv( const GLdouble *v1, const GLdouble *v2 );
void       GLAPIENTRY glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );
void       GLAPIENTRY glRectfv( const GLfloat *v1, const GLfloat *v2 );
void       GLAPIENTRY glRecti( GLint x1, GLint y1, GLint x2, GLint y2 );
void       GLAPIENTRY glRectiv( const GLint *v1, const GLint *v2 );
void       GLAPIENTRY glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );
void       GLAPIENTRY glRectsv( const GLshort *v1, const GLshort *v2 );
GLint      GLAPIENTRY glRenderMode( GLenum mode );
void       GLAPIENTRY glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
void       GLAPIENTRY glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
void       GLAPIENTRY glScaled( GLdouble x, GLdouble y, GLdouble z );
void       GLAPIENTRY glScalef( GLfloat x, GLfloat y, GLfloat z );
void       GLAPIENTRY glScissor( GLint x, GLint y, GLsizei width, GLsizei height );
void       GLAPIENTRY glSelectBuffer( GLsizei size, GLuint *buffer );
void       GLAPIENTRY glShadeModel( GLenum mode );
void       GLAPIENTRY glStencilFunc( GLenum func, GLint ref, GLuint mask );
void       GLAPIENTRY glStencilMask( GLuint mask );
void       GLAPIENTRY glStencilOp( GLenum fail, GLenum zfail, GLenum zpass );
void       GLAPIENTRY glTexCoord1d( GLdouble s );
void       GLAPIENTRY glTexCoord1dv( const GLdouble *v );
void       GLAPIENTRY glTexCoord1f( GLfloat s );
void       GLAPIENTRY glTexCoord1fv( const GLfloat *v );
void       GLAPIENTRY glTexCoord1i( GLint s );
void       GLAPIENTRY glTexCoord1iv( const GLint *v );
void       GLAPIENTRY glTexCoord1s( GLshort s );
void       GLAPIENTRY glTexCoord1sv( const GLshort *v );
void       GLAPIENTRY glTexCoord2d( GLdouble s, GLdouble t );
void       GLAPIENTRY glTexCoord2dv( const GLdouble *v );
void       GLAPIENTRY glTexCoord2f( GLfloat s, GLfloat t );
void       GLAPIENTRY glTexCoord2fv( const GLfloat *v );
void       GLAPIENTRY glTexCoord2i( GLint s, GLint t );
void       GLAPIENTRY glTexCoord2iv( const GLint *v );
void       GLAPIENTRY glTexCoord2s( GLshort s, GLshort t );
void       GLAPIENTRY glTexCoord2sv( const GLshort *v );
void       GLAPIENTRY glTexCoord3d( GLdouble s, GLdouble t, GLdouble r );
void       GLAPIENTRY glTexCoord3dv( const GLdouble *v );
void       GLAPIENTRY glTexCoord3f( GLfloat s, GLfloat t, GLfloat r );
void       GLAPIENTRY glTexCoord3fv( const GLfloat *v );
void       GLAPIENTRY glTexCoord3i( GLint s, GLint t, GLint r );
void       GLAPIENTRY glTexCoord3iv( const GLint *v );
void       GLAPIENTRY glTexCoord3s( GLshort s, GLshort t, GLshort r );
void       GLAPIENTRY glTexCoord3sv( const GLshort *v );
void       GLAPIENTRY glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
void       GLAPIENTRY glTexCoord4dv( const GLdouble *v );
void       GLAPIENTRY glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
void       GLAPIENTRY glTexCoord4fv( const GLfloat *v );
void       GLAPIENTRY glTexCoord4i( GLint s, GLint t, GLint r, GLint q );
void       GLAPIENTRY glTexCoord4iv( const GLint *v );
void       GLAPIENTRY glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q );
void       GLAPIENTRY glTexCoord4sv( const GLshort *v );
void       GLAPIENTRY glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const void *pointer );
void       GLAPIENTRY glTexEnvf( GLenum target, GLenum pname, GLfloat param );
void       GLAPIENTRY glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params );
void       GLAPIENTRY glTexEnvi( GLenum target, GLenum pname, GLint param );
void       GLAPIENTRY glTexEnviv( GLenum target, GLenum pname, const GLint *params );
void       GLAPIENTRY glTexGend( GLenum coord, GLenum pname, GLdouble param );
void       GLAPIENTRY glTexGendv( GLenum coord, GLenum pname, const GLdouble *params );
void       GLAPIENTRY glTexGenf( GLenum coord, GLenum pname, GLfloat param );
void       GLAPIENTRY glTexGenfv( GLenum coord, GLenum pname, const GLfloat *params );
void       GLAPIENTRY glTexGeni( GLenum coord, GLenum pname, GLint param );
void       GLAPIENTRY glTexGeniv( GLenum coord, GLenum pname, const GLint *params );
void       GLAPIENTRY glTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
void       GLAPIENTRY glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
void       GLAPIENTRY glTexParameterf( GLenum target, GLenum pname, GLfloat param );
void       GLAPIENTRY glTexParameterfv( GLenum target, GLenum pname, const GLfloat *params );
void       GLAPIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param );
void       GLAPIENTRY glTexParameteriv( GLenum target, GLenum pname, const GLint *params );
void       GLAPIENTRY glTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
void       GLAPIENTRY glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
void       GLAPIENTRY glTranslated( GLdouble x, GLdouble y, GLdouble z );
void       GLAPIENTRY glTranslatef( GLfloat x, GLfloat y, GLfloat z );
void       GLAPIENTRY glVertex2d( GLdouble x, GLdouble y );
void       GLAPIENTRY glVertex2dv( const GLdouble *v );
void       GLAPIENTRY glVertex2f( GLfloat x, GLfloat y );
void       GLAPIENTRY glVertex2fv( const GLfloat *v );
void       GLAPIENTRY glVertex2i( GLint x, GLint y );
void       GLAPIENTRY glVertex2iv( const GLint *v );
void       GLAPIENTRY glVertex2s( GLshort x, GLshort y );
void       GLAPIENTRY glVertex2sv( const GLshort *v );
void       GLAPIENTRY glVertex3d( GLdouble x, GLdouble y, GLdouble z );
void       GLAPIENTRY glVertex3dv( const GLdouble *v );
void       GLAPIENTRY glVertex3f( GLfloat x, GLfloat y, GLfloat z );
void       GLAPIENTRY glVertex3fv( const GLfloat *v );
void       GLAPIENTRY glVertex3i( GLint x, GLint y, GLint z );
void       GLAPIENTRY glVertex3iv( const GLint *v );
void       GLAPIENTRY glVertex3s( GLshort x, GLshort y, GLshort z );
void       GLAPIENTRY glVertex3sv( const GLshort *v );
void       GLAPIENTRY glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
void       GLAPIENTRY glVertex4dv( const GLdouble *v );
void       GLAPIENTRY glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
void       GLAPIENTRY glVertex4fv( const GLfloat *v );
void       GLAPIENTRY glVertex4i( GLint x, GLint y, GLint z, GLint w );
void       GLAPIENTRY glVertex4iv( const GLint *v );
void       GLAPIENTRY glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w );
void       GLAPIENTRY glVertex4sv( const GLshort *v );
void       GLAPIENTRY glVertexPointer( GLint size, GLenum type, GLsizei stride, const void *pointer );
void       GLAPIENTRY glViewport( GLint x, GLint y, GLsizei width, GLsizei height );
#endif

typedef int        (GLAPIENTRY *PFN_wglChoosePixelFormat)( HDC hDc, const PIXELFORMATDESCRIPTOR *pPfd );
typedef BOOL       (GLAPIENTRY *PFN_wglCopyContext)( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask );
typedef HGLRC      (GLAPIENTRY *PFN_wglCreateContext)( HDC hDc );
typedef HGLRC      (GLAPIENTRY *PFN_wglCreateLayerContext)( HDC hDc, int level );
typedef BOOL       (GLAPIENTRY *PFN_wglDeleteContext)( HGLRC oldContext );
typedef BOOL       (GLAPIENTRY *PFN_wglDescribeLayerPlane)( HDC hDc, int pixelFormat, int layerPlane, UINT nBytes, const LAYERPLANEDESCRIPTOR *plpd );
typedef int        (GLAPIENTRY *PFN_wglDescribePixelFormat)( HDC hdc, int ipfd, UINT cjpfd, PIXELFORMATDESCRIPTOR *ppfd );
typedef HGLRC      (GLAPIENTRY *PFN_wglGetCurrentContext)(void);
typedef HDC        (GLAPIENTRY *PFN_wglGetCurrentDC)(void);
typedef PROC       (GLAPIENTRY *PFN_wglGetDefaultProcAddress)( LPCSTR lpszProc );
typedef int        (GLAPIENTRY *PFN_wglGetLayerPaletteEntries)( HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr );
typedef int        (GLAPIENTRY *PFN_wglGetPixelFormat)( HDC hdc );
typedef PROC       (GLAPIENTRY *PFN_wglGetProcAddress)( LPCSTR lpszProc );
typedef BOOL       (GLAPIENTRY *PFN_wglMakeCurrent)( HDC hDc, HGLRC newContext );
typedef BOOL       (GLAPIENTRY *PFN_wglRealizeLayerPalette)( HDC hdc, int iLayerPlane, BOOL bRealize );
typedef int        (GLAPIENTRY *PFN_wglSetLayerPaletteEntries)( HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr );
typedef BOOL       (GLAPIENTRY *PFN_wglSetPixelFormat)( HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd );
typedef BOOL       (GLAPIENTRY *PFN_wglShareLists)( HGLRC hrcSrvShare, HGLRC hrcSrvSource );
typedef BOOL       (GLAPIENTRY *PFN_wglSwapBuffers)( HDC hdc );
typedef BOOL       (GLAPIENTRY *PFN_wglSwapLayerBuffers)( HDC hdc, UINT fuFlags );
typedef BOOL       (GLAPIENTRY *PFN_wglUseFontBitmapsA)( HDC hDC, DWORD first, DWORD count, DWORD listBase );
typedef BOOL       (GLAPIENTRY *PFN_wglUseFontBitmapsW)( HDC hDC, DWORD first, DWORD count, DWORD listBase );
typedef BOOL       (GLAPIENTRY *PFN_wglUseFontOutlinesA)( HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf );
typedef BOOL       (GLAPIENTRY *PFN_wglUseFontOutlinesW)( HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf );
typedef EGLBoolean (GLAPIENTRY *PFN_eglBindAPI)( EGLenum api );
typedef EGLBoolean (GLAPIENTRY *PFN_eglBindTexImage)( EGLDisplay dpy, EGLSurface surface, EGLint buffer );
typedef EGLBoolean (GLAPIENTRY *PFN_eglChooseConfig)( EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config );
typedef EGLint     (GLAPIENTRY *PFN_eglClientWaitSync)( EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout );
typedef EGLBoolean (GLAPIENTRY *PFN_eglCopyBuffers)( EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target );
typedef EGLContext (GLAPIENTRY *PFN_eglCreateContext)( EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list );
typedef EGLImage   (GLAPIENTRY *PFN_eglCreateImage)( EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list );
typedef EGLSurface (GLAPIENTRY *PFN_eglCreatePbufferFromClientBuffer)( EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list );
typedef EGLSurface (GLAPIENTRY *PFN_eglCreatePbufferSurface)( EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list );
typedef EGLSurface (GLAPIENTRY *PFN_eglCreatePixmapSurface)( EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list );
typedef EGLSurface (GLAPIENTRY *PFN_eglCreatePlatformPixmapSurface)( EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list );
typedef EGLSurface (GLAPIENTRY *PFN_eglCreatePlatformWindowSurface)( EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list );
typedef EGLSync    (GLAPIENTRY *PFN_eglCreateSync)( EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list );
typedef EGLSurface (GLAPIENTRY *PFN_eglCreateWindowSurface)( EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list );
typedef EGLBoolean (GLAPIENTRY *PFN_eglDestroyContext)( EGLDisplay dpy, EGLContext ctx );
typedef EGLBoolean (GLAPIENTRY *PFN_eglDestroyImage)( EGLDisplay dpy, EGLImage image );
typedef EGLBoolean (GLAPIENTRY *PFN_eglDestroySurface)( EGLDisplay dpy, EGLSurface surface );
typedef EGLBoolean (GLAPIENTRY *PFN_eglDestroySync)( EGLDisplay dpy, EGLSync sync );
typedef EGLBoolean (GLAPIENTRY *PFN_eglGetConfigAttrib)( EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value );
typedef EGLBoolean (GLAPIENTRY *PFN_eglGetConfigs)( EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config );
typedef EGLContext (GLAPIENTRY *PFN_eglGetCurrentContext)(void);
typedef EGLDisplay (GLAPIENTRY *PFN_eglGetCurrentDisplay)(void);
typedef EGLSurface (GLAPIENTRY *PFN_eglGetCurrentSurface)( EGLint readdraw );
typedef EGLDisplay (GLAPIENTRY *PFN_eglGetDisplay)( EGLNativeDisplayType display_id );
typedef EGLint     (GLAPIENTRY *PFN_eglGetError)(void);
typedef EGLDisplay (GLAPIENTRY *PFN_eglGetPlatformDisplay)( EGLenum platform, void *native_display, const EGLAttrib *attrib_list );
typedef __eglMustCastToProperFunctionPointerType (GLAPIENTRY *PFN_eglGetProcAddress)( const char *procname );
typedef EGLBoolean (GLAPIENTRY *PFN_eglGetSyncAttrib)( EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value );
typedef EGLBoolean (GLAPIENTRY *PFN_eglInitialize)( EGLDisplay dpy, EGLint *major, EGLint *minor );
typedef EGLBoolean (GLAPIENTRY *PFN_eglMakeCurrent)( EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx );
typedef EGLenum    (GLAPIENTRY *PFN_eglQueryAPI)(void);
typedef EGLBoolean (GLAPIENTRY *PFN_eglQueryContext)( EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value );
typedef const char * (GLAPIENTRY *PFN_eglQueryString)( EGLDisplay dpy, EGLint name );
typedef EGLBoolean (GLAPIENTRY *PFN_eglQuerySurface)( EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value );
typedef EGLBoolean (GLAPIENTRY *PFN_eglReleaseTexImage)( EGLDisplay dpy, EGLSurface surface, EGLint buffer );
typedef EGLBoolean (GLAPIENTRY *PFN_eglReleaseThread)(void);
typedef EGLBoolean (GLAPIENTRY *PFN_eglSurfaceAttrib)( EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value );
typedef EGLBoolean (GLAPIENTRY *PFN_eglSwapBuffers)( EGLDisplay dpy, EGLSurface surface );
typedef EGLBoolean (GLAPIENTRY *PFN_eglSwapInterval)( EGLDisplay dpy, EGLint interval );
typedef EGLBoolean (GLAPIENTRY *PFN_eglTerminate)( EGLDisplay dpy );
typedef EGLBoolean (GLAPIENTRY *PFN_eglWaitClient)(void);
typedef EGLBoolean (GLAPIENTRY *PFN_eglWaitGL)(void);
typedef EGLBoolean (GLAPIENTRY *PFN_eglWaitNative)( EGLint engine );
typedef EGLBoolean (GLAPIENTRY *PFN_eglWaitSync)( EGLDisplay dpy, EGLSync sync, EGLint flags );
typedef void       (GLAPIENTRY *PFN_glAccum)( GLenum op, GLfloat value );
typedef void       (GLAPIENTRY *PFN_glAlphaFunc)( GLenum func, GLfloat ref );
typedef GLboolean  (GLAPIENTRY *PFN_glAreTexturesResident)( GLsizei n, const GLuint *textures, GLboolean *residences );
typedef void       (GLAPIENTRY *PFN_glArrayElement)( GLint i );
typedef void       (GLAPIENTRY *PFN_glBegin)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBindTexture)( GLenum target, GLuint texture );
typedef void       (GLAPIENTRY *PFN_glBitmap)( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
typedef void       (GLAPIENTRY *PFN_glBlendFunc)( GLenum sfactor, GLenum dfactor );
typedef void       (GLAPIENTRY *PFN_glCallList)( GLuint list );
typedef void       (GLAPIENTRY *PFN_glCallLists)( GLsizei n, GLenum type, const void *lists );
typedef void       (GLAPIENTRY *PFN_glClear)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glClearAccum)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
typedef void       (GLAPIENTRY *PFN_glClearColor)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
typedef void       (GLAPIENTRY *PFN_glClearDepth)( GLdouble depth );
typedef void       (GLAPIENTRY *PFN_glClearIndex)( GLfloat c );
typedef void       (GLAPIENTRY *PFN_glClearStencil)( GLint s );
typedef void       (GLAPIENTRY *PFN_glClipPlane)( GLenum plane, const GLdouble *equation );
typedef void       (GLAPIENTRY *PFN_glColor3b)( GLbyte red, GLbyte green, GLbyte blue );
typedef void       (GLAPIENTRY *PFN_glColor3bv)( const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glColor3d)( GLdouble red, GLdouble green, GLdouble blue );
typedef void       (GLAPIENTRY *PFN_glColor3dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glColor3f)( GLfloat red, GLfloat green, GLfloat blue );
typedef void       (GLAPIENTRY *PFN_glColor3fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glColor3i)( GLint red, GLint green, GLint blue );
typedef void       (GLAPIENTRY *PFN_glColor3iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glColor3s)( GLshort red, GLshort green, GLshort blue );
typedef void       (GLAPIENTRY *PFN_glColor3sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glColor3ub)( GLubyte red, GLubyte green, GLubyte blue );
typedef void       (GLAPIENTRY *PFN_glColor3ubv)( const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glColor3ui)( GLuint red, GLuint green, GLuint blue );
typedef void       (GLAPIENTRY *PFN_glColor3uiv)( const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glColor3us)( GLushort red, GLushort green, GLushort blue );
typedef void       (GLAPIENTRY *PFN_glColor3usv)( const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glColor4b)( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );
typedef void       (GLAPIENTRY *PFN_glColor4bv)( const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glColor4d)( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );
typedef void       (GLAPIENTRY *PFN_glColor4dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glColor4f)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
typedef void       (GLAPIENTRY *PFN_glColor4fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glColor4i)( GLint red, GLint green, GLint blue, GLint alpha );
typedef void       (GLAPIENTRY *PFN_glColor4iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glColor4s)( GLshort red, GLshort green, GLshort blue, GLshort alpha );
typedef void       (GLAPIENTRY *PFN_glColor4sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glColor4ub)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
typedef void       (GLAPIENTRY *PFN_glColor4ubv)( const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glColor4ui)( GLuint red, GLuint green, GLuint blue, GLuint alpha );
typedef void       (GLAPIENTRY *PFN_glColor4uiv)( const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glColor4us)( GLushort red, GLushort green, GLushort blue, GLushort alpha );
typedef void       (GLAPIENTRY *PFN_glColor4usv)( const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glColorMask)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
typedef void       (GLAPIENTRY *PFN_glColorMaterial)( GLenum face, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glColorPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glCopyPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type );
typedef void       (GLAPIENTRY *PFN_glCopyTexImage1D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCullFace)( GLenum mode );
typedef GLint      (GLAPIENTRY *PFN_glDebugEntry)( GLint unknown1, GLint unknown2 );
typedef void       (GLAPIENTRY *PFN_glDeleteLists)( GLuint list, GLsizei range );
typedef void       (GLAPIENTRY *PFN_glDeleteTextures)( GLsizei n, const GLuint *textures );
typedef void       (GLAPIENTRY *PFN_glDepthFunc)( GLenum func );
typedef void       (GLAPIENTRY *PFN_glDepthMask)( GLboolean flag );
typedef void       (GLAPIENTRY *PFN_glDepthRange)( GLdouble n, GLdouble f );
typedef void       (GLAPIENTRY *PFN_glDisable)( GLenum cap );
typedef void       (GLAPIENTRY *PFN_glDisableClientState)( GLenum array );
typedef void       (GLAPIENTRY *PFN_glDrawArrays)( GLenum mode, GLint first, GLsizei count );
typedef void       (GLAPIENTRY *PFN_glDrawBuffer)( GLenum buf );
typedef void       (GLAPIENTRY *PFN_glDrawElements)( GLenum mode, GLsizei count, GLenum type, const void *indices );
typedef void       (GLAPIENTRY *PFN_glDrawPixels)( GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glEdgeFlag)( GLboolean flag );
typedef void       (GLAPIENTRY *PFN_glEdgeFlagPointer)( GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glEdgeFlagv)( const GLboolean *flag );
typedef void       (GLAPIENTRY *PFN_glEnable)( GLenum cap );
typedef void       (GLAPIENTRY *PFN_glEnableClientState)( GLenum array );
typedef void       (GLAPIENTRY *PFN_glEnd)(void);
typedef void       (GLAPIENTRY *PFN_glEndList)(void);
typedef void       (GLAPIENTRY *PFN_glEvalCoord1d)( GLdouble u );
typedef void       (GLAPIENTRY *PFN_glEvalCoord1dv)( const GLdouble *u );
typedef void       (GLAPIENTRY *PFN_glEvalCoord1f)( GLfloat u );
typedef void       (GLAPIENTRY *PFN_glEvalCoord1fv)( const GLfloat *u );
typedef void       (GLAPIENTRY *PFN_glEvalCoord2d)( GLdouble u, GLdouble v );
typedef void       (GLAPIENTRY *PFN_glEvalCoord2dv)( const GLdouble *u );
typedef void       (GLAPIENTRY *PFN_glEvalCoord2f)( GLfloat u, GLfloat v );
typedef void       (GLAPIENTRY *PFN_glEvalCoord2fv)( const GLfloat *u );
typedef void       (GLAPIENTRY *PFN_glEvalMesh1)( GLenum mode, GLint i1, GLint i2 );
typedef void       (GLAPIENTRY *PFN_glEvalMesh2)( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
typedef void       (GLAPIENTRY *PFN_glEvalPoint1)( GLint i );
typedef void       (GLAPIENTRY *PFN_glEvalPoint2)( GLint i, GLint j );
typedef void       (GLAPIENTRY *PFN_glFeedbackBuffer)( GLsizei size, GLenum type, GLfloat *buffer );
typedef void       (GLAPIENTRY *PFN_glFinish)(void);
typedef void       (GLAPIENTRY *PFN_glFlush)(void);
typedef void       (GLAPIENTRY *PFN_glFogf)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glFogfv)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glFogi)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glFogiv)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glFrontFace)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glFrustum)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
typedef GLuint     (GLAPIENTRY *PFN_glGenLists)( GLsizei range );
typedef void       (GLAPIENTRY *PFN_glGenTextures)( GLsizei n, GLuint *textures );
typedef void       (GLAPIENTRY *PFN_glGetBooleanv)( GLenum pname, GLboolean *data );
typedef void       (GLAPIENTRY *PFN_glGetClipPlane)( GLenum plane, GLdouble *equation );
typedef void       (GLAPIENTRY *PFN_glGetDoublev)( GLenum pname, GLdouble *data );
typedef GLenum     (GLAPIENTRY *PFN_glGetError)(void);
typedef void       (GLAPIENTRY *PFN_glGetFloatv)( GLenum pname, GLfloat *data );
typedef void       (GLAPIENTRY *PFN_glGetIntegerv)( GLenum pname, GLint *data );
typedef void       (GLAPIENTRY *PFN_glGetLightfv)( GLenum light, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetLightiv)( GLenum light, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMapdv)( GLenum target, GLenum query, GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glGetMapfv)( GLenum target, GLenum query, GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glGetMapiv)( GLenum target, GLenum query, GLint *v );
typedef void       (GLAPIENTRY *PFN_glGetMaterialfv)( GLenum face, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMaterialiv)( GLenum face, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetPixelMapfv)( GLenum map, GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glGetPixelMapuiv)( GLenum map, GLuint *values );
typedef void       (GLAPIENTRY *PFN_glGetPixelMapusv)( GLenum map, GLushort *values );
typedef void       (GLAPIENTRY *PFN_glGetPointerv)( GLenum pname, void **params );
typedef void       (GLAPIENTRY *PFN_glGetPolygonStipple)( GLubyte *mask );
typedef const GLubyte * (GLAPIENTRY *PFN_glGetString)( GLenum name );
typedef void       (GLAPIENTRY *PFN_glGetTexEnvfv)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTexEnviv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTexGendv)( GLenum coord, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetTexGenfv)( GLenum coord, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTexGeniv)( GLenum coord, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTexImage)( GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetTexLevelParameterfv)( GLenum target, GLint level, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTexLevelParameteriv)( GLenum target, GLint level, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameterfv)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glHint)( GLenum target, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glIndexMask)( GLuint mask );
typedef void       (GLAPIENTRY *PFN_glIndexPointer)( GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glIndexd)( GLdouble c );
typedef void       (GLAPIENTRY *PFN_glIndexdv)( const GLdouble *c );
typedef void       (GLAPIENTRY *PFN_glIndexf)( GLfloat c );
typedef void       (GLAPIENTRY *PFN_glIndexfv)( const GLfloat *c );
typedef void       (GLAPIENTRY *PFN_glIndexi)( GLint c );
typedef void       (GLAPIENTRY *PFN_glIndexiv)( const GLint *c );
typedef void       (GLAPIENTRY *PFN_glIndexs)( GLshort c );
typedef void       (GLAPIENTRY *PFN_glIndexsv)( const GLshort *c );
typedef void       (GLAPIENTRY *PFN_glIndexub)( GLubyte c );
typedef void       (GLAPIENTRY *PFN_glIndexubv)( const GLubyte *c );
typedef void       (GLAPIENTRY *PFN_glInitNames)(void);
typedef void       (GLAPIENTRY *PFN_glInterleavedArrays)( GLenum format, GLsizei stride, const void *pointer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsEnabled)( GLenum cap );
typedef GLboolean  (GLAPIENTRY *PFN_glIsList)( GLuint list );
typedef GLboolean  (GLAPIENTRY *PFN_glIsTexture)( GLuint texture );
typedef void       (GLAPIENTRY *PFN_glLightModelf)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glLightModelfv)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glLightModeli)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glLightModeliv)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glLightf)( GLenum light, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glLightfv)( GLenum light, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glLighti)( GLenum light, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glLightiv)( GLenum light, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glLineStipple)( GLint factor, GLushort pattern );
typedef void       (GLAPIENTRY *PFN_glLineWidth)( GLfloat width );
typedef void       (GLAPIENTRY *PFN_glListBase)( GLuint base );
typedef void       (GLAPIENTRY *PFN_glLoadIdentity)(void);
typedef void       (GLAPIENTRY *PFN_glLoadMatrixd)( const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glLoadMatrixf)( const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glLoadName)( GLuint name );
typedef void       (GLAPIENTRY *PFN_glLogicOp)( GLenum opcode );
typedef void       (GLAPIENTRY *PFN_glMap1d)( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
typedef void       (GLAPIENTRY *PFN_glMap1f)( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glMap2d)( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
typedef void       (GLAPIENTRY *PFN_glMap2f)( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glMapGrid1d)( GLint un, GLdouble u1, GLdouble u2 );
typedef void       (GLAPIENTRY *PFN_glMapGrid1f)( GLint un, GLfloat u1, GLfloat u2 );
typedef void       (GLAPIENTRY *PFN_glMapGrid2d)( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 );
typedef void       (GLAPIENTRY *PFN_glMapGrid2f)( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 );
typedef void       (GLAPIENTRY *PFN_glMaterialf)( GLenum face, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glMaterialfv)( GLenum face, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glMateriali)( GLenum face, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glMaterialiv)( GLenum face, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glMatrixMode)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glMultMatrixd)( const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glMultMatrixf)( const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glNewList)( GLuint list, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glNormal3b)( GLbyte nx, GLbyte ny, GLbyte nz );
typedef void       (GLAPIENTRY *PFN_glNormal3bv)( const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glNormal3d)( GLdouble nx, GLdouble ny, GLdouble nz );
typedef void       (GLAPIENTRY *PFN_glNormal3dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glNormal3f)( GLfloat nx, GLfloat ny, GLfloat nz );
typedef void       (GLAPIENTRY *PFN_glNormal3fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glNormal3i)( GLint nx, GLint ny, GLint nz );
typedef void       (GLAPIENTRY *PFN_glNormal3iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glNormal3s)( GLshort nx, GLshort ny, GLshort nz );
typedef void       (GLAPIENTRY *PFN_glNormal3sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glNormalPointer)( GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glOrtho)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
typedef void       (GLAPIENTRY *PFN_glPassThrough)( GLfloat token );
typedef void       (GLAPIENTRY *PFN_glPixelMapfv)( GLenum map, GLsizei mapsize, const GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glPixelMapuiv)( GLenum map, GLsizei mapsize, const GLuint *values );
typedef void       (GLAPIENTRY *PFN_glPixelMapusv)( GLenum map, GLsizei mapsize, const GLushort *values );
typedef void       (GLAPIENTRY *PFN_glPixelStoref)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPixelStorei)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glPixelTransferf)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPixelTransferi)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glPixelZoom)( GLfloat xfactor, GLfloat yfactor );
typedef void       (GLAPIENTRY *PFN_glPointSize)( GLfloat size );
typedef void       (GLAPIENTRY *PFN_glPolygonMode)( GLenum face, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glPolygonOffset)( GLfloat factor, GLfloat units );
typedef void       (GLAPIENTRY *PFN_glPolygonStipple)( const GLubyte *mask );
typedef void       (GLAPIENTRY *PFN_glPopAttrib)(void);
typedef void       (GLAPIENTRY *PFN_glPopClientAttrib)(void);
typedef void       (GLAPIENTRY *PFN_glPopMatrix)(void);
typedef void       (GLAPIENTRY *PFN_glPopName)(void);
typedef void       (GLAPIENTRY *PFN_glPrioritizeTextures)( GLsizei n, const GLuint *textures, const GLfloat *priorities );
typedef void       (GLAPIENTRY *PFN_glPushAttrib)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glPushClientAttrib)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glPushMatrix)(void);
typedef void       (GLAPIENTRY *PFN_glPushName)( GLuint name );
typedef void       (GLAPIENTRY *PFN_glRasterPos2d)( GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glRasterPos2dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos2f)( GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glRasterPos2fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos2i)( GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glRasterPos2iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos2s)( GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glRasterPos2sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos3d)( GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glRasterPos3dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos3f)( GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glRasterPos3fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos3i)( GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glRasterPos3iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos3s)( GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glRasterPos3sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos4d)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glRasterPos4dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glRasterPos4fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos4i)( GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glRasterPos4iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glRasterPos4s)( GLshort x, GLshort y, GLshort z, GLshort w );
typedef void       (GLAPIENTRY *PFN_glRasterPos4sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glReadBuffer)( GLenum src );
typedef void       (GLAPIENTRY *PFN_glReadPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels );
typedef void       (GLAPIENTRY *PFN_glRectd)( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );
typedef void       (GLAPIENTRY *PFN_glRectdv)( const GLdouble *v1, const GLdouble *v2 );
typedef void       (GLAPIENTRY *PFN_glRectf)( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );
typedef void       (GLAPIENTRY *PFN_glRectfv)( const GLfloat *v1, const GLfloat *v2 );
typedef void       (GLAPIENTRY *PFN_glRecti)( GLint x1, GLint y1, GLint x2, GLint y2 );
typedef void       (GLAPIENTRY *PFN_glRectiv)( const GLint *v1, const GLint *v2 );
typedef void       (GLAPIENTRY *PFN_glRects)( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );
typedef void       (GLAPIENTRY *PFN_glRectsv)( const GLshort *v1, const GLshort *v2 );
typedef GLint      (GLAPIENTRY *PFN_glRenderMode)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glRotated)( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glRotatef)( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glScaled)( GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glScalef)( GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glScissor)( GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glSelectBuffer)( GLsizei size, GLuint *buffer );
typedef void       (GLAPIENTRY *PFN_glShadeModel)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glStencilFunc)( GLenum func, GLint ref, GLuint mask );
typedef void       (GLAPIENTRY *PFN_glStencilMask)( GLuint mask );
typedef void       (GLAPIENTRY *PFN_glStencilOp)( GLenum fail, GLenum zfail, GLenum zpass );
typedef void       (GLAPIENTRY *PFN_glTexCoord1d)( GLdouble s );
typedef void       (GLAPIENTRY *PFN_glTexCoord1dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord1f)( GLfloat s );
typedef void       (GLAPIENTRY *PFN_glTexCoord1fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord1i)( GLint s );
typedef void       (GLAPIENTRY *PFN_glTexCoord1iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord1s)( GLshort s );
typedef void       (GLAPIENTRY *PFN_glTexCoord1sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2d)( GLdouble s, GLdouble t );
typedef void       (GLAPIENTRY *PFN_glTexCoord2dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2f)( GLfloat s, GLfloat t );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2i)( GLint s, GLint t );
typedef void       (GLAPIENTRY *PFN_glTexCoord2iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2s)( GLshort s, GLshort t );
typedef void       (GLAPIENTRY *PFN_glTexCoord2sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord3d)( GLdouble s, GLdouble t, GLdouble r );
typedef void       (GLAPIENTRY *PFN_glTexCoord3dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord3f)( GLfloat s, GLfloat t, GLfloat r );
typedef void       (GLAPIENTRY *PFN_glTexCoord3fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord3i)( GLint s, GLint t, GLint r );
typedef void       (GLAPIENTRY *PFN_glTexCoord3iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord3s)( GLshort s, GLshort t, GLshort r );
typedef void       (GLAPIENTRY *PFN_glTexCoord3sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord4d)( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
typedef void       (GLAPIENTRY *PFN_glTexCoord4dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord4f)( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
typedef void       (GLAPIENTRY *PFN_glTexCoord4fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord4i)( GLint s, GLint t, GLint r, GLint q );
typedef void       (GLAPIENTRY *PFN_glTexCoord4iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord4s)( GLshort s, GLshort t, GLshort r, GLshort q );
typedef void       (GLAPIENTRY *PFN_glTexCoord4sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glTexCoordPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glTexEnvf)( GLenum target, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glTexEnvfv)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glTexEnvi)( GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glTexEnviv)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTexGend)( GLenum coord, GLenum pname, GLdouble param );
typedef void       (GLAPIENTRY *PFN_glTexGendv)( GLenum coord, GLenum pname, const GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glTexGenf)( GLenum coord, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glTexGenfv)( GLenum coord, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glTexGeni)( GLenum coord, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glTexGeniv)( GLenum coord, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTexImage1D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexImage2D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexParameterf)( GLenum target, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glTexParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glTexParameteri)( GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glTexParameteriv)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTranslated)( GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glTranslatef)( GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glVertex2d)( GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glVertex2dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertex2f)( GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glVertex2fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertex2i)( GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glVertex2iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertex2s)( GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glVertex2sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertex3d)( GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glVertex3dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertex3f)( GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glVertex3fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertex3i)( GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glVertex3iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertex3s)( GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glVertex3sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertex4d)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glVertex4dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertex4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glVertex4fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertex4i)( GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glVertex4iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertex4s)( GLshort x, GLshort y, GLshort z, GLshort w );
typedef void       (GLAPIENTRY *PFN_glVertex4sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glViewport)( GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glAccumxOES)( GLenum op, GLfixed value );
typedef GLboolean  (GLAPIENTRY *PFN_glAcquireKeyedMutexWin32EXT)( GLuint memory, GLuint64 key, GLuint timeout );
typedef void       (GLAPIENTRY *PFN_glActiveProgramEXT)( GLuint program );
typedef void       (GLAPIENTRY *PFN_glActiveShaderProgram)( GLuint pipeline, GLuint program );
typedef void       (GLAPIENTRY *PFN_glActiveStencilFaceEXT)( GLenum face );
typedef void       (GLAPIENTRY *PFN_glActiveTexture)( GLenum texture );
typedef void       (GLAPIENTRY *PFN_glActiveTextureARB)( GLenum texture );
typedef void       (GLAPIENTRY *PFN_glActiveVaryingNV)( GLuint program, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glAlphaFragmentOp1ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
typedef void       (GLAPIENTRY *PFN_glAlphaFragmentOp2ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
typedef void       (GLAPIENTRY *PFN_glAlphaFragmentOp3ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
typedef void       (GLAPIENTRY *PFN_glAlphaFuncxOES)( GLenum func, GLfixed ref );
typedef void       (GLAPIENTRY *PFN_glAlphaToCoverageDitherControlNV)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glApplyFramebufferAttachmentCMAAINTEL)(void);
typedef void       (GLAPIENTRY *PFN_glApplyTextureEXT)( GLenum mode );
typedef GLboolean  (GLAPIENTRY *PFN_glAreProgramsResidentNV)( GLsizei n, const GLuint *programs, GLboolean *residences );
typedef GLboolean  (GLAPIENTRY *PFN_glAreTexturesResidentEXT)( GLsizei n, const GLuint *textures, GLboolean *residences );
typedef void       (GLAPIENTRY *PFN_glArrayElementEXT)( GLint i );
typedef void       (GLAPIENTRY *PFN_glArrayObjectATI)( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
typedef GLuint     (GLAPIENTRY *PFN_glAsyncCopyBufferSubDataNVX)( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *fenceValueArray, GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray );
typedef GLuint     (GLAPIENTRY *PFN_glAsyncCopyImageSubDataNVX)( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *waitValueArray, GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray );
typedef void       (GLAPIENTRY *PFN_glAsyncMarkerSGIX)( GLuint marker );
typedef void       (GLAPIENTRY *PFN_glAttachObjectARB)( GLhandleARB containerObj, GLhandleARB obj );
typedef void       (GLAPIENTRY *PFN_glAttachShader)( GLuint program, GLuint shader );
typedef void       (GLAPIENTRY *PFN_glBeginConditionalRender)( GLuint id, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBeginConditionalRenderNV)( GLuint id, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBeginConditionalRenderNVX)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glBeginFragmentShaderATI)(void);
typedef void       (GLAPIENTRY *PFN_glBeginOcclusionQueryNV)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glBeginPerfMonitorAMD)( GLuint monitor );
typedef void       (GLAPIENTRY *PFN_glBeginPerfQueryINTEL)( GLuint queryHandle );
typedef void       (GLAPIENTRY *PFN_glBeginQuery)( GLenum target, GLuint id );
typedef void       (GLAPIENTRY *PFN_glBeginQueryARB)( GLenum target, GLuint id );
typedef void       (GLAPIENTRY *PFN_glBeginQueryIndexed)( GLenum target, GLuint index, GLuint id );
typedef void       (GLAPIENTRY *PFN_glBeginTransformFeedback)( GLenum primitiveMode );
typedef void       (GLAPIENTRY *PFN_glBeginTransformFeedbackEXT)( GLenum primitiveMode );
typedef void       (GLAPIENTRY *PFN_glBeginTransformFeedbackNV)( GLenum primitiveMode );
typedef void       (GLAPIENTRY *PFN_glBeginVertexShaderEXT)(void);
typedef void       (GLAPIENTRY *PFN_glBeginVideoCaptureNV)( GLuint video_capture_slot );
typedef void       (GLAPIENTRY *PFN_glBindAttribLocation)( GLuint program, GLuint index, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glBindAttribLocationARB)( GLhandleARB programObj, GLuint index, const GLcharARB *name );
typedef void       (GLAPIENTRY *PFN_glBindBuffer)( GLenum target, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glBindBufferARB)( GLenum target, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glBindBufferBase)( GLenum target, GLuint index, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glBindBufferBaseEXT)( GLenum target, GLuint index, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glBindBufferBaseNV)( GLenum target, GLuint index, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glBindBufferOffsetEXT)( GLenum target, GLuint index, GLuint buffer, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glBindBufferOffsetNV)( GLenum target, GLuint index, GLuint buffer, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glBindBufferRange)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glBindBufferRangeEXT)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glBindBufferRangeNV)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glBindBuffersBase)( GLenum target, GLuint first, GLsizei count, const GLuint *buffers );
typedef void       (GLAPIENTRY *PFN_glBindBuffersRange)( GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes );
typedef void       (GLAPIENTRY *PFN_glBindFragDataLocation)( GLuint program, GLuint color, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glBindFragDataLocationEXT)( GLuint program, GLuint color, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glBindFragDataLocationIndexed)( GLuint program, GLuint colorNumber, GLuint index, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glBindFragmentShaderATI)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glBindFramebuffer)( GLenum target, GLuint framebuffer );
typedef void       (GLAPIENTRY *PFN_glBindFramebufferEXT)( GLenum target, GLuint framebuffer );
typedef void       (GLAPIENTRY *PFN_glBindImageTexture)( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format );
typedef void       (GLAPIENTRY *PFN_glBindImageTextureEXT)( GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format );
typedef void       (GLAPIENTRY *PFN_glBindImageTextures)( GLuint first, GLsizei count, const GLuint *textures );
typedef GLuint     (GLAPIENTRY *PFN_glBindLightParameterEXT)( GLenum light, GLenum value );
typedef GLuint     (GLAPIENTRY *PFN_glBindMaterialParameterEXT)( GLenum face, GLenum value );
typedef void       (GLAPIENTRY *PFN_glBindMultiTextureEXT)( GLenum texunit, GLenum target, GLuint texture );
typedef GLuint     (GLAPIENTRY *PFN_glBindParameterEXT)( GLenum value );
typedef void       (GLAPIENTRY *PFN_glBindProgramARB)( GLenum target, GLuint program );
typedef void       (GLAPIENTRY *PFN_glBindProgramNV)( GLenum target, GLuint id );
typedef void       (GLAPIENTRY *PFN_glBindProgramPipeline)( GLuint pipeline );
typedef void       (GLAPIENTRY *PFN_glBindRenderbuffer)( GLenum target, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glBindRenderbufferEXT)( GLenum target, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glBindSampler)( GLuint unit, GLuint sampler );
typedef void       (GLAPIENTRY *PFN_glBindSamplers)( GLuint first, GLsizei count, const GLuint *samplers );
typedef void       (GLAPIENTRY *PFN_glBindShadingRateImageNV)( GLuint texture );
typedef GLuint     (GLAPIENTRY *PFN_glBindTexGenParameterEXT)( GLenum unit, GLenum coord, GLenum value );
typedef void       (GLAPIENTRY *PFN_glBindTextureEXT)( GLenum target, GLuint texture );
typedef void       (GLAPIENTRY *PFN_glBindTextureUnit)( GLuint unit, GLuint texture );
typedef GLuint     (GLAPIENTRY *PFN_glBindTextureUnitParameterEXT)( GLenum unit, GLenum value );
typedef void       (GLAPIENTRY *PFN_glBindTextures)( GLuint first, GLsizei count, const GLuint *textures );
typedef void       (GLAPIENTRY *PFN_glBindTransformFeedback)( GLenum target, GLuint id );
typedef void       (GLAPIENTRY *PFN_glBindTransformFeedbackNV)( GLenum target, GLuint id );
typedef void       (GLAPIENTRY *PFN_glBindVertexArray)( GLuint array );
typedef void       (GLAPIENTRY *PFN_glBindVertexArrayAPPLE)( GLuint array );
typedef void       (GLAPIENTRY *PFN_glBindVertexBuffer)( GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glBindVertexBuffers)( GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides );
typedef void       (GLAPIENTRY *PFN_glBindVertexShaderEXT)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glBindVideoCaptureStreamBufferNV)( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLintptrARB offset );
typedef void       (GLAPIENTRY *PFN_glBindVideoCaptureStreamTextureNV)( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture );
typedef void       (GLAPIENTRY *PFN_glBinormal3bEXT)( GLbyte bx, GLbyte by, GLbyte bz );
typedef void       (GLAPIENTRY *PFN_glBinormal3bvEXT)( const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glBinormal3dEXT)( GLdouble bx, GLdouble by, GLdouble bz );
typedef void       (GLAPIENTRY *PFN_glBinormal3dvEXT)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glBinormal3fEXT)( GLfloat bx, GLfloat by, GLfloat bz );
typedef void       (GLAPIENTRY *PFN_glBinormal3fvEXT)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glBinormal3iEXT)( GLint bx, GLint by, GLint bz );
typedef void       (GLAPIENTRY *PFN_glBinormal3ivEXT)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glBinormal3sEXT)( GLshort bx, GLshort by, GLshort bz );
typedef void       (GLAPIENTRY *PFN_glBinormal3svEXT)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glBinormalPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glBitmapxOES)( GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig, GLfixed xmove, GLfixed ymove, const GLubyte *bitmap );
typedef void       (GLAPIENTRY *PFN_glBlendBarrierKHR)(void);
typedef void       (GLAPIENTRY *PFN_glBlendBarrierNV)(void);
typedef void       (GLAPIENTRY *PFN_glBlendColor)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
typedef void       (GLAPIENTRY *PFN_glBlendColorEXT)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
typedef void       (GLAPIENTRY *PFN_glBlendColorxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
typedef void       (GLAPIENTRY *PFN_glBlendEquation)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBlendEquationEXT)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBlendEquationIndexedAMD)( GLuint buf, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBlendEquationSeparate)( GLenum modeRGB, GLenum modeAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendEquationSeparateEXT)( GLenum modeRGB, GLenum modeAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendEquationSeparateIndexedAMD)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendEquationSeparatei)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendEquationSeparateiARB)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendEquationi)( GLuint buf, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBlendEquationiARB)( GLuint buf, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glBlendFuncIndexedAMD)( GLuint buf, GLenum src, GLenum dst );
typedef void       (GLAPIENTRY *PFN_glBlendFuncSeparate)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendFuncSeparateEXT)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendFuncSeparateINGR)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendFuncSeparateIndexedAMD)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendFuncSeparatei)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendFuncSeparateiARB)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
typedef void       (GLAPIENTRY *PFN_glBlendFunci)( GLuint buf, GLenum src, GLenum dst );
typedef void       (GLAPIENTRY *PFN_glBlendFunciARB)( GLuint buf, GLenum src, GLenum dst );
typedef void       (GLAPIENTRY *PFN_glBlendParameteriNV)( GLenum pname, GLint value );
typedef void       (GLAPIENTRY *PFN_glBlitFramebuffer)( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
typedef void       (GLAPIENTRY *PFN_glBlitFramebufferEXT)( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
typedef void       (GLAPIENTRY *PFN_glBlitNamedFramebuffer)( GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
typedef void       (GLAPIENTRY *PFN_glBufferAddressRangeNV)( GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length );
typedef void       (GLAPIENTRY *PFN_glBufferAttachMemoryNV)( GLenum target, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glBufferData)( GLenum target, GLsizeiptr size, const void *data, GLenum usage );
typedef void       (GLAPIENTRY *PFN_glBufferDataARB)( GLenum target, GLsizeiptrARB size, const void *data, GLenum usage );
typedef void       (GLAPIENTRY *PFN_glBufferPageCommitmentARB)( GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit );
typedef void       (GLAPIENTRY *PFN_glBufferParameteriAPPLE)( GLenum target, GLenum pname, GLint param );
typedef GLuint     (GLAPIENTRY *PFN_glBufferRegionEnabled)(void);
typedef void       (GLAPIENTRY *PFN_glBufferStorage)( GLenum target, GLsizeiptr size, const void *data, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glBufferStorageExternalEXT)( GLenum target, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glBufferStorageMemEXT)( GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glBufferSubData)( GLenum target, GLintptr offset, GLsizeiptr size, const void *data );
typedef void       (GLAPIENTRY *PFN_glBufferSubDataARB)( GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data );
typedef void       (GLAPIENTRY *PFN_glCallCommandListNV)( GLuint list );
typedef GLenum     (GLAPIENTRY *PFN_glCheckFramebufferStatus)( GLenum target );
typedef GLenum     (GLAPIENTRY *PFN_glCheckFramebufferStatusEXT)( GLenum target );
typedef GLenum     (GLAPIENTRY *PFN_glCheckNamedFramebufferStatus)( GLuint framebuffer, GLenum target );
typedef GLenum     (GLAPIENTRY *PFN_glCheckNamedFramebufferStatusEXT)( GLuint framebuffer, GLenum target );
typedef void       (GLAPIENTRY *PFN_glClampColor)( GLenum target, GLenum clamp );
typedef void       (GLAPIENTRY *PFN_glClampColorARB)( GLenum target, GLenum clamp );
typedef void       (GLAPIENTRY *PFN_glClearAccumxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
typedef void       (GLAPIENTRY *PFN_glClearBufferData)( GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClearBufferSubData)( GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClearBufferfi)( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil );
typedef void       (GLAPIENTRY *PFN_glClearBufferfv)( GLenum buffer, GLint drawbuffer, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glClearBufferiv)( GLenum buffer, GLint drawbuffer, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glClearBufferuiv)( GLenum buffer, GLint drawbuffer, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glClearColorIiEXT)( GLint red, GLint green, GLint blue, GLint alpha );
typedef void       (GLAPIENTRY *PFN_glClearColorIuiEXT)( GLuint red, GLuint green, GLuint blue, GLuint alpha );
typedef void       (GLAPIENTRY *PFN_glClearColorxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
typedef void       (GLAPIENTRY *PFN_glClearDepthdNV)( GLdouble depth );
typedef void       (GLAPIENTRY *PFN_glClearDepthf)( GLfloat d );
typedef void       (GLAPIENTRY *PFN_glClearDepthfOES)( GLclampf depth );
typedef void       (GLAPIENTRY *PFN_glClearDepthxOES)( GLfixed depth );
typedef void       (GLAPIENTRY *PFN_glClearNamedBufferData)( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClearNamedBufferDataEXT)( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClearNamedBufferSubData)( GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClearNamedBufferSubDataEXT)( GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClearNamedFramebufferfi)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil );
typedef void       (GLAPIENTRY *PFN_glClearNamedFramebufferfv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glClearNamedFramebufferiv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glClearNamedFramebufferuiv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glClearTexImage)( GLuint texture, GLint level, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClearTexSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glClientActiveTexture)( GLenum texture );
typedef void       (GLAPIENTRY *PFN_glClientActiveTextureARB)( GLenum texture );
typedef void       (GLAPIENTRY *PFN_glClientActiveVertexStreamATI)( GLenum stream );
typedef void       (GLAPIENTRY *PFN_glClientAttribDefaultEXT)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glClientWaitSemaphoreui64NVX)( GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
typedef GLenum     (GLAPIENTRY *PFN_glClientWaitSync)( GLsync sync, GLbitfield flags, GLuint64 timeout );
typedef void       (GLAPIENTRY *PFN_glClipControl)( GLenum origin, GLenum depth );
typedef void       (GLAPIENTRY *PFN_glClipPlanefOES)( GLenum plane, const GLfloat *equation );
typedef void       (GLAPIENTRY *PFN_glClipPlanexOES)( GLenum plane, const GLfixed *equation );
typedef void       (GLAPIENTRY *PFN_glColor3fVertex3fSUN)( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glColor3fVertex3fvSUN)( const GLfloat *c, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glColor3hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue );
typedef void       (GLAPIENTRY *PFN_glColor3hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glColor3xOES)( GLfixed red, GLfixed green, GLfixed blue );
typedef void       (GLAPIENTRY *PFN_glColor3xvOES)( const GLfixed *components );
typedef void       (GLAPIENTRY *PFN_glColor4fNormal3fVertex3fSUN)( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glColor4fNormal3fVertex3fvSUN)( const GLfloat *c, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glColor4hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha );
typedef void       (GLAPIENTRY *PFN_glColor4hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glColor4ubVertex2fSUN)( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glColor4ubVertex2fvSUN)( const GLubyte *c, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glColor4ubVertex3fSUN)( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glColor4ubVertex3fvSUN)( const GLubyte *c, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glColor4xOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
typedef void       (GLAPIENTRY *PFN_glColor4xvOES)( const GLfixed *components );
typedef void       (GLAPIENTRY *PFN_glColorFormatNV)( GLint size, GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glColorFragmentOp1ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
typedef void       (GLAPIENTRY *PFN_glColorFragmentOp2ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
typedef void       (GLAPIENTRY *PFN_glColorFragmentOp3ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
typedef void       (GLAPIENTRY *PFN_glColorMaskIndexedEXT)( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a );
typedef void       (GLAPIENTRY *PFN_glColorMaski)( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a );
typedef void       (GLAPIENTRY *PFN_glColorP3ui)( GLenum type, GLuint color );
typedef void       (GLAPIENTRY *PFN_glColorP3uiv)( GLenum type, const GLuint *color );
typedef void       (GLAPIENTRY *PFN_glColorP4ui)( GLenum type, GLuint color );
typedef void       (GLAPIENTRY *PFN_glColorP4uiv)( GLenum type, const GLuint *color );
typedef void       (GLAPIENTRY *PFN_glColorPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glColorPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glColorPointervINTEL)( GLint size, GLenum type, const void **pointer );
typedef void       (GLAPIENTRY *PFN_glColorSubTable)( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glColorSubTableEXT)( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data );
typedef void       (GLAPIENTRY *PFN_glColorTable)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table );
typedef void       (GLAPIENTRY *PFN_glColorTableEXT)( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void *table );
typedef void       (GLAPIENTRY *PFN_glColorTableParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glColorTableParameterfvSGI)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glColorTableParameteriv)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glColorTableParameterivSGI)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glColorTableSGI)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table );
typedef void       (GLAPIENTRY *PFN_glCombinerInputNV)( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
typedef void       (GLAPIENTRY *PFN_glCombinerOutputNV)( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum );
typedef void       (GLAPIENTRY *PFN_glCombinerParameterfNV)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glCombinerParameterfvNV)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glCombinerParameteriNV)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glCombinerParameterivNV)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glCombinerStageParameterfvNV)( GLenum stage, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glCommandListSegmentsNV)( GLuint list, GLuint segments );
typedef void       (GLAPIENTRY *PFN_glCompileCommandListNV)( GLuint list );
typedef void       (GLAPIENTRY *PFN_glCompileShader)( GLuint shader );
typedef void       (GLAPIENTRY *PFN_glCompileShaderARB)( GLhandleARB shaderObj );
typedef void       (GLAPIENTRY *PFN_glCompileShaderIncludeARB)( GLuint shader, GLsizei count, const GLchar *const*path, const GLint *length );
typedef void       (GLAPIENTRY *PFN_glCompressedMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedMultiTexImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedTexImage1D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexImage1DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexImage2DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexImage3D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexImage3DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexSubImage1DARB)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexSubImage2DARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTexSubImage3DARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureImage3DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
typedef void       (GLAPIENTRY *PFN_glCompressedTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits );
typedef void       (GLAPIENTRY *PFN_glConservativeRasterParameterfNV)( GLenum pname, GLfloat value );
typedef void       (GLAPIENTRY *PFN_glConservativeRasterParameteriNV)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glConvolutionFilter1D)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image );
typedef void       (GLAPIENTRY *PFN_glConvolutionFilter1DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image );
typedef void       (GLAPIENTRY *PFN_glConvolutionFilter2D)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image );
typedef void       (GLAPIENTRY *PFN_glConvolutionFilter2DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameterf)( GLenum target, GLenum pname, GLfloat params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameterfEXT)( GLenum target, GLenum pname, GLfloat params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameterfvEXT)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameteri)( GLenum target, GLenum pname, GLint params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameteriEXT)( GLenum target, GLenum pname, GLint params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameteriv)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameterivEXT)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameterxOES)( GLenum target, GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glConvolutionParameterxvOES)( GLenum target, GLenum pname, const GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glCopyBufferSubData)( GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glCopyColorSubTable)( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyColorSubTableEXT)( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyColorTable)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyColorTableSGI)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyConvolutionFilter1D)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyConvolutionFilter1DEXT)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyConvolutionFilter2D)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyConvolutionFilter2DEXT)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyImageSubData)( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth );
typedef void       (GLAPIENTRY *PFN_glCopyImageSubDataNV)( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth );
typedef void       (GLAPIENTRY *PFN_glCopyMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyNamedBufferSubData)( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glCopyPathNV)( GLuint resultPath, GLuint srcPath );
typedef void       (GLAPIENTRY *PFN_glCopyTexImage1DEXT)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyTexImage2DEXT)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyTexSubImage1DEXT)( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyTexSubImage2DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyTexSubImage3DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
typedef void       (GLAPIENTRY *PFN_glCopyTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glCopyTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCopyTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glCoverFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
typedef void       (GLAPIENTRY *PFN_glCoverFillPathNV)( GLuint path, GLenum coverMode );
typedef void       (GLAPIENTRY *PFN_glCoverStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
typedef void       (GLAPIENTRY *PFN_glCoverStrokePathNV)( GLuint path, GLenum coverMode );
typedef void       (GLAPIENTRY *PFN_glCoverageModulationNV)( GLenum components );
typedef void       (GLAPIENTRY *PFN_glCoverageModulationTableNV)( GLsizei n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glCreateBuffers)( GLsizei n, GLuint *buffers );
typedef void       (GLAPIENTRY *PFN_glCreateCommandListsNV)( GLsizei n, GLuint *lists );
typedef void       (GLAPIENTRY *PFN_glCreateFramebuffers)( GLsizei n, GLuint *framebuffers );
typedef void       (GLAPIENTRY *PFN_glCreateMemoryObjectsEXT)( GLsizei n, GLuint *memoryObjects );
typedef void       (GLAPIENTRY *PFN_glCreatePerfQueryINTEL)( GLuint queryId, GLuint *queryHandle );
typedef GLuint     (GLAPIENTRY *PFN_glCreateProgram)(void);
typedef GLhandleARB (GLAPIENTRY *PFN_glCreateProgramObjectARB)(void);
typedef void       (GLAPIENTRY *PFN_glCreateProgramPipelines)( GLsizei n, GLuint *pipelines );
typedef GLuint     (GLAPIENTRY *PFN_glCreateProgressFenceNVX)(void);
typedef void       (GLAPIENTRY *PFN_glCreateQueries)( GLenum target, GLsizei n, GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glCreateRenderbuffers)( GLsizei n, GLuint *renderbuffers );
typedef void       (GLAPIENTRY *PFN_glCreateSamplers)( GLsizei n, GLuint *samplers );
typedef GLuint     (GLAPIENTRY *PFN_glCreateShader)( GLenum type );
typedef GLhandleARB (GLAPIENTRY *PFN_glCreateShaderObjectARB)( GLenum shaderType );
typedef GLuint     (GLAPIENTRY *PFN_glCreateShaderProgramEXT)( GLenum type, const GLchar *string );
typedef GLuint     (GLAPIENTRY *PFN_glCreateShaderProgramv)( GLenum type, GLsizei count, const GLchar *const*strings );
typedef void       (GLAPIENTRY *PFN_glCreateStatesNV)( GLsizei n, GLuint *states );
typedef GLsync     (GLAPIENTRY *PFN_glCreateSyncFromCLeventARB)( struct _cl_context *context, struct _cl_event *event, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glCreateTextures)( GLenum target, GLsizei n, GLuint *textures );
typedef void       (GLAPIENTRY *PFN_glCreateTransformFeedbacks)( GLsizei n, GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glCreateVertexArrays)( GLsizei n, GLuint *arrays );
typedef void       (GLAPIENTRY *PFN_glCullParameterdvEXT)( GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glCullParameterfvEXT)( GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glCurrentPaletteMatrixARB)( GLint index );
typedef void       (GLAPIENTRY *PFN_glDebugMessageCallback)( GLDEBUGPROC callback, const void *userParam );
typedef void       (GLAPIENTRY *PFN_glDebugMessageCallbackAMD)( GLDEBUGPROCAMD callback, void *userParam );
typedef void       (GLAPIENTRY *PFN_glDebugMessageCallbackARB)( GLDEBUGPROCARB callback, const void *userParam );
typedef void       (GLAPIENTRY *PFN_glDebugMessageControl)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
typedef void       (GLAPIENTRY *PFN_glDebugMessageControlARB)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
typedef void       (GLAPIENTRY *PFN_glDebugMessageEnableAMD)( GLenum category, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
typedef void       (GLAPIENTRY *PFN_glDebugMessageInsert)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf );
typedef void       (GLAPIENTRY *PFN_glDebugMessageInsertAMD)( GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar *buf );
typedef void       (GLAPIENTRY *PFN_glDebugMessageInsertARB)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf );
typedef void       (GLAPIENTRY *PFN_glDeformSGIX)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glDeformationMap3dSGIX)( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points );
typedef void       (GLAPIENTRY *PFN_glDeformationMap3fSGIX)( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glDeleteAsyncMarkersSGIX)( GLuint marker, GLsizei range );
typedef void       (GLAPIENTRY *PFN_glDeleteBufferRegion)( GLenum region );
typedef void       (GLAPIENTRY *PFN_glDeleteBuffers)( GLsizei n, const GLuint *buffers );
typedef void       (GLAPIENTRY *PFN_glDeleteBuffersARB)( GLsizei n, const GLuint *buffers );
typedef void       (GLAPIENTRY *PFN_glDeleteCommandListsNV)( GLsizei n, const GLuint *lists );
typedef void       (GLAPIENTRY *PFN_glDeleteFencesAPPLE)( GLsizei n, const GLuint *fences );
typedef void       (GLAPIENTRY *PFN_glDeleteFencesNV)( GLsizei n, const GLuint *fences );
typedef void       (GLAPIENTRY *PFN_glDeleteFragmentShaderATI)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glDeleteFramebuffers)( GLsizei n, const GLuint *framebuffers );
typedef void       (GLAPIENTRY *PFN_glDeleteFramebuffersEXT)( GLsizei n, const GLuint *framebuffers );
typedef void       (GLAPIENTRY *PFN_glDeleteMemoryObjectsEXT)( GLsizei n, const GLuint *memoryObjects );
typedef void       (GLAPIENTRY *PFN_glDeleteNamedStringARB)( GLint namelen, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glDeleteNamesAMD)( GLenum identifier, GLuint num, const GLuint *names );
typedef void       (GLAPIENTRY *PFN_glDeleteObjectARB)( GLhandleARB obj );
typedef void       (GLAPIENTRY *PFN_glDeleteObjectBufferATI)( GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glDeleteOcclusionQueriesNV)( GLsizei n, const GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glDeletePathsNV)( GLuint path, GLsizei range );
typedef void       (GLAPIENTRY *PFN_glDeletePerfMonitorsAMD)( GLsizei n, GLuint *monitors );
typedef void       (GLAPIENTRY *PFN_glDeletePerfQueryINTEL)( GLuint queryHandle );
typedef void       (GLAPIENTRY *PFN_glDeleteProgram)( GLuint program );
typedef void       (GLAPIENTRY *PFN_glDeleteProgramPipelines)( GLsizei n, const GLuint *pipelines );
typedef void       (GLAPIENTRY *PFN_glDeleteProgramsARB)( GLsizei n, const GLuint *programs );
typedef void       (GLAPIENTRY *PFN_glDeleteProgramsNV)( GLsizei n, const GLuint *programs );
typedef void       (GLAPIENTRY *PFN_glDeleteQueries)( GLsizei n, const GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glDeleteQueriesARB)( GLsizei n, const GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glDeleteQueryResourceTagNV)( GLsizei n, const GLint *tagIds );
typedef void       (GLAPIENTRY *PFN_glDeleteRenderbuffers)( GLsizei n, const GLuint *renderbuffers );
typedef void       (GLAPIENTRY *PFN_glDeleteRenderbuffersEXT)( GLsizei n, const GLuint *renderbuffers );
typedef void       (GLAPIENTRY *PFN_glDeleteSamplers)( GLsizei count, const GLuint *samplers );
typedef void       (GLAPIENTRY *PFN_glDeleteSemaphoresEXT)( GLsizei n, const GLuint *semaphores );
typedef void       (GLAPIENTRY *PFN_glDeleteShader)( GLuint shader );
typedef void       (GLAPIENTRY *PFN_glDeleteStatesNV)( GLsizei n, const GLuint *states );
typedef void       (GLAPIENTRY *PFN_glDeleteSync)( GLsync sync );
typedef void       (GLAPIENTRY *PFN_glDeleteTexturesEXT)( GLsizei n, const GLuint *textures );
typedef void       (GLAPIENTRY *PFN_glDeleteTransformFeedbacks)( GLsizei n, const GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glDeleteTransformFeedbacksNV)( GLsizei n, const GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glDeleteVertexArrays)( GLsizei n, const GLuint *arrays );
typedef void       (GLAPIENTRY *PFN_glDeleteVertexArraysAPPLE)( GLsizei n, const GLuint *arrays );
typedef void       (GLAPIENTRY *PFN_glDeleteVertexShaderEXT)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glDepthBoundsEXT)( GLclampd zmin, GLclampd zmax );
typedef void       (GLAPIENTRY *PFN_glDepthBoundsdNV)( GLdouble zmin, GLdouble zmax );
typedef void       (GLAPIENTRY *PFN_glDepthRangeArraydvNV)( GLuint first, GLsizei count, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glDepthRangeArrayv)( GLuint first, GLsizei count, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glDepthRangeIndexed)( GLuint index, GLdouble n, GLdouble f );
typedef void       (GLAPIENTRY *PFN_glDepthRangeIndexeddNV)( GLuint index, GLdouble n, GLdouble f );
typedef void       (GLAPIENTRY *PFN_glDepthRangedNV)( GLdouble zNear, GLdouble zFar );
typedef void       (GLAPIENTRY *PFN_glDepthRangef)( GLfloat n, GLfloat f );
typedef void       (GLAPIENTRY *PFN_glDepthRangefOES)( GLclampf n, GLclampf f );
typedef void       (GLAPIENTRY *PFN_glDepthRangexOES)( GLfixed n, GLfixed f );
typedef void       (GLAPIENTRY *PFN_glDetachObjectARB)( GLhandleARB containerObj, GLhandleARB attachedObj );
typedef void       (GLAPIENTRY *PFN_glDetachShader)( GLuint program, GLuint shader );
typedef void       (GLAPIENTRY *PFN_glDetailTexFuncSGIS)( GLenum target, GLsizei n, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glDisableClientStateIndexedEXT)( GLenum array, GLuint index );
typedef void       (GLAPIENTRY *PFN_glDisableClientStateiEXT)( GLenum array, GLuint index );
typedef void       (GLAPIENTRY *PFN_glDisableIndexedEXT)( GLenum target, GLuint index );
typedef void       (GLAPIENTRY *PFN_glDisableVariantClientStateEXT)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glDisableVertexArrayAttrib)( GLuint vaobj, GLuint index );
typedef void       (GLAPIENTRY *PFN_glDisableVertexArrayAttribEXT)( GLuint vaobj, GLuint index );
typedef void       (GLAPIENTRY *PFN_glDisableVertexArrayEXT)( GLuint vaobj, GLenum array );
typedef void       (GLAPIENTRY *PFN_glDisableVertexAttribAPPLE)( GLuint index, GLenum pname );
typedef void       (GLAPIENTRY *PFN_glDisableVertexAttribArray)( GLuint index );
typedef void       (GLAPIENTRY *PFN_glDisableVertexAttribArrayARB)( GLuint index );
typedef void       (GLAPIENTRY *PFN_glDisablei)( GLenum target, GLuint index );
typedef void       (GLAPIENTRY *PFN_glDispatchCompute)( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z );
typedef void       (GLAPIENTRY *PFN_glDispatchComputeGroupSizeARB)( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z );
typedef void       (GLAPIENTRY *PFN_glDispatchComputeIndirect)( GLintptr indirect );
typedef void       (GLAPIENTRY *PFN_glDrawArraysEXT)( GLenum mode, GLint first, GLsizei count );
typedef void       (GLAPIENTRY *PFN_glDrawArraysIndirect)( GLenum mode, const void *indirect );
typedef void       (GLAPIENTRY *PFN_glDrawArraysInstanced)( GLenum mode, GLint first, GLsizei count, GLsizei instancecount );
typedef void       (GLAPIENTRY *PFN_glDrawArraysInstancedARB)( GLenum mode, GLint first, GLsizei count, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glDrawArraysInstancedBaseInstance)( GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance );
typedef void       (GLAPIENTRY *PFN_glDrawArraysInstancedEXT)( GLenum mode, GLint start, GLsizei count, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glDrawBufferRegion)( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest );
typedef void       (GLAPIENTRY *PFN_glDrawBuffers)( GLsizei n, const GLenum *bufs );
typedef void       (GLAPIENTRY *PFN_glDrawBuffersARB)( GLsizei n, const GLenum *bufs );
typedef void       (GLAPIENTRY *PFN_glDrawBuffersATI)( GLsizei n, const GLenum *bufs );
typedef void       (GLAPIENTRY *PFN_glDrawCommandsAddressNV)( GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count );
typedef void       (GLAPIENTRY *PFN_glDrawCommandsNV)( GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count );
typedef void       (GLAPIENTRY *PFN_glDrawCommandsStatesAddressNV)( const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
typedef void       (GLAPIENTRY *PFN_glDrawCommandsStatesNV)( GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
typedef void       (GLAPIENTRY *PFN_glDrawElementArrayAPPLE)( GLenum mode, GLint first, GLsizei count );
typedef void       (GLAPIENTRY *PFN_glDrawElementArrayATI)( GLenum mode, GLsizei count );
typedef void       (GLAPIENTRY *PFN_glDrawElementsBaseVertex)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex );
typedef void       (GLAPIENTRY *PFN_glDrawElementsIndirect)( GLenum mode, GLenum type, const void *indirect );
typedef void       (GLAPIENTRY *PFN_glDrawElementsInstanced)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount );
typedef void       (GLAPIENTRY *PFN_glDrawElementsInstancedARB)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glDrawElementsInstancedBaseInstance)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance );
typedef void       (GLAPIENTRY *PFN_glDrawElementsInstancedBaseVertex)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex );
typedef void       (GLAPIENTRY *PFN_glDrawElementsInstancedBaseVertexBaseInstance)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance );
typedef void       (GLAPIENTRY *PFN_glDrawElementsInstancedEXT)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glDrawMeshArraysSUN)( GLenum mode, GLint first, GLsizei count, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glDrawMeshTasksIndirectNV)( GLintptr indirect );
typedef void       (GLAPIENTRY *PFN_glDrawMeshTasksNV)( GLuint first, GLuint count );
typedef void       (GLAPIENTRY *PFN_glDrawRangeElementArrayAPPLE)( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count );
typedef void       (GLAPIENTRY *PFN_glDrawRangeElementArrayATI)( GLenum mode, GLuint start, GLuint end, GLsizei count );
typedef void       (GLAPIENTRY *PFN_glDrawRangeElements)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
typedef void       (GLAPIENTRY *PFN_glDrawRangeElementsBaseVertex)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex );
typedef void       (GLAPIENTRY *PFN_glDrawRangeElementsEXT)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
typedef void       (GLAPIENTRY *PFN_glDrawTextureNV)( GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 );
typedef void       (GLAPIENTRY *PFN_glDrawTransformFeedback)( GLenum mode, GLuint id );
typedef void       (GLAPIENTRY *PFN_glDrawTransformFeedbackInstanced)( GLenum mode, GLuint id, GLsizei instancecount );
typedef void       (GLAPIENTRY *PFN_glDrawTransformFeedbackNV)( GLenum mode, GLuint id );
typedef void       (GLAPIENTRY *PFN_glDrawTransformFeedbackStream)( GLenum mode, GLuint id, GLuint stream );
typedef void       (GLAPIENTRY *PFN_glDrawTransformFeedbackStreamInstanced)( GLenum mode, GLuint id, GLuint stream, GLsizei instancecount );
typedef void       (GLAPIENTRY *PFN_glDrawVkImageNV)( GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 );
typedef void       (GLAPIENTRY *PFN_glEGLImageTargetTexStorageEXT)( GLenum target, GLeglImageOES image, const GLint* attrib_list );
typedef void       (GLAPIENTRY *PFN_glEGLImageTargetTextureStorageEXT)( GLuint texture, GLeglImageOES image, const GLint* attrib_list );
typedef void       (GLAPIENTRY *PFN_glEdgeFlagFormatNV)( GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glEdgeFlagPointerEXT)( GLsizei stride, GLsizei count, const GLboolean *pointer );
typedef void       (GLAPIENTRY *PFN_glEdgeFlagPointerListIBM)( GLint stride, const GLboolean **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glElementPointerAPPLE)( GLenum type, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glElementPointerATI)( GLenum type, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glEnableClientStateIndexedEXT)( GLenum array, GLuint index );
typedef void       (GLAPIENTRY *PFN_glEnableClientStateiEXT)( GLenum array, GLuint index );
typedef void       (GLAPIENTRY *PFN_glEnableIndexedEXT)( GLenum target, GLuint index );
typedef void       (GLAPIENTRY *PFN_glEnableVariantClientStateEXT)( GLuint id );
typedef void       (GLAPIENTRY *PFN_glEnableVertexArrayAttrib)( GLuint vaobj, GLuint index );
typedef void       (GLAPIENTRY *PFN_glEnableVertexArrayAttribEXT)( GLuint vaobj, GLuint index );
typedef void       (GLAPIENTRY *PFN_glEnableVertexArrayEXT)( GLuint vaobj, GLenum array );
typedef void       (GLAPIENTRY *PFN_glEnableVertexAttribAPPLE)( GLuint index, GLenum pname );
typedef void       (GLAPIENTRY *PFN_glEnableVertexAttribArray)( GLuint index );
typedef void       (GLAPIENTRY *PFN_glEnableVertexAttribArrayARB)( GLuint index );
typedef void       (GLAPIENTRY *PFN_glEnablei)( GLenum target, GLuint index );
typedef void       (GLAPIENTRY *PFN_glEndConditionalRender)(void);
typedef void       (GLAPIENTRY *PFN_glEndConditionalRenderNV)(void);
typedef void       (GLAPIENTRY *PFN_glEndConditionalRenderNVX)(void);
typedef void       (GLAPIENTRY *PFN_glEndFragmentShaderATI)(void);
typedef void       (GLAPIENTRY *PFN_glEndOcclusionQueryNV)(void);
typedef void       (GLAPIENTRY *PFN_glEndPerfMonitorAMD)( GLuint monitor );
typedef void       (GLAPIENTRY *PFN_glEndPerfQueryINTEL)( GLuint queryHandle );
typedef void       (GLAPIENTRY *PFN_glEndQuery)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glEndQueryARB)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glEndQueryIndexed)( GLenum target, GLuint index );
typedef void       (GLAPIENTRY *PFN_glEndTransformFeedback)(void);
typedef void       (GLAPIENTRY *PFN_glEndTransformFeedbackEXT)(void);
typedef void       (GLAPIENTRY *PFN_glEndTransformFeedbackNV)(void);
typedef void       (GLAPIENTRY *PFN_glEndVertexShaderEXT)(void);
typedef void       (GLAPIENTRY *PFN_glEndVideoCaptureNV)( GLuint video_capture_slot );
typedef void       (GLAPIENTRY *PFN_glEvalCoord1xOES)( GLfixed u );
typedef void       (GLAPIENTRY *PFN_glEvalCoord1xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glEvalCoord2xOES)( GLfixed u, GLfixed v );
typedef void       (GLAPIENTRY *PFN_glEvalCoord2xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glEvalMapsNV)( GLenum target, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glEvaluateDepthValuesARB)(void);
typedef void       (GLAPIENTRY *PFN_glExecuteProgramNV)( GLenum target, GLuint id, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glExtractComponentEXT)( GLuint res, GLuint src, GLuint num );
typedef void       (GLAPIENTRY *PFN_glFeedbackBufferxOES)( GLsizei n, GLenum type, const GLfixed *buffer );
typedef GLsync     (GLAPIENTRY *PFN_glFenceSync)( GLenum condition, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glFinalCombinerInputNV)( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
typedef GLint      (GLAPIENTRY *PFN_glFinishAsyncSGIX)( GLuint *markerp );
typedef void       (GLAPIENTRY *PFN_glFinishFenceAPPLE)( GLuint fence );
typedef void       (GLAPIENTRY *PFN_glFinishFenceNV)( GLuint fence );
typedef void       (GLAPIENTRY *PFN_glFinishObjectAPPLE)( GLenum object, GLint name );
typedef void       (GLAPIENTRY *PFN_glFinishTextureSUNX)(void);
typedef void       (GLAPIENTRY *PFN_glFlushMappedBufferRange)( GLenum target, GLintptr offset, GLsizeiptr length );
typedef void       (GLAPIENTRY *PFN_glFlushMappedBufferRangeAPPLE)( GLenum target, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glFlushMappedNamedBufferRange)( GLuint buffer, GLintptr offset, GLsizeiptr length );
typedef void       (GLAPIENTRY *PFN_glFlushMappedNamedBufferRangeEXT)( GLuint buffer, GLintptr offset, GLsizeiptr length );
typedef void       (GLAPIENTRY *PFN_glFlushPixelDataRangeNV)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glFlushRasterSGIX)(void);
typedef void       (GLAPIENTRY *PFN_glFlushStaticDataIBM)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glFlushVertexArrayRangeAPPLE)( GLsizei length, void *pointer );
typedef void       (GLAPIENTRY *PFN_glFlushVertexArrayRangeNV)(void);
typedef void       (GLAPIENTRY *PFN_glFogCoordFormatNV)( GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glFogCoordPointer)( GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glFogCoordPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glFogCoordPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glFogCoordd)( GLdouble coord );
typedef void       (GLAPIENTRY *PFN_glFogCoorddEXT)( GLdouble coord );
typedef void       (GLAPIENTRY *PFN_glFogCoorddv)( const GLdouble *coord );
typedef void       (GLAPIENTRY *PFN_glFogCoorddvEXT)( const GLdouble *coord );
typedef void       (GLAPIENTRY *PFN_glFogCoordf)( GLfloat coord );
typedef void       (GLAPIENTRY *PFN_glFogCoordfEXT)( GLfloat coord );
typedef void       (GLAPIENTRY *PFN_glFogCoordfv)( const GLfloat *coord );
typedef void       (GLAPIENTRY *PFN_glFogCoordfvEXT)( const GLfloat *coord );
typedef void       (GLAPIENTRY *PFN_glFogCoordhNV)( GLhalfNV fog );
typedef void       (GLAPIENTRY *PFN_glFogCoordhvNV)( const GLhalfNV *fog );
typedef void       (GLAPIENTRY *PFN_glFogFuncSGIS)( GLsizei n, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glFogxOES)( GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glFogxvOES)( GLenum pname, const GLfixed *param );
typedef void       (GLAPIENTRY *PFN_glFragmentColorMaterialSGIX)( GLenum face, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glFragmentCoverageColorNV)( GLuint color );
typedef void       (GLAPIENTRY *PFN_glFragmentLightModelfSGIX)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glFragmentLightModelfvSGIX)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glFragmentLightModeliSGIX)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glFragmentLightModelivSGIX)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glFragmentLightfSGIX)( GLenum light, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glFragmentLightfvSGIX)( GLenum light, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glFragmentLightiSGIX)( GLenum light, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glFragmentLightivSGIX)( GLenum light, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glFragmentMaterialfSGIX)( GLenum face, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glFragmentMaterialfvSGIX)( GLenum face, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glFragmentMaterialiSGIX)( GLenum face, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glFragmentMaterialivSGIX)( GLenum face, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glFrameTerminatorGREMEDY)(void);
typedef void       (GLAPIENTRY *PFN_glFrameZoomSGIX)( GLint factor );
typedef void       (GLAPIENTRY *PFN_glFramebufferDrawBufferEXT)( GLuint framebuffer, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glFramebufferDrawBuffersEXT)( GLuint framebuffer, GLsizei n, const GLenum *bufs );
typedef void       (GLAPIENTRY *PFN_glFramebufferFetchBarrierEXT)(void);
typedef void       (GLAPIENTRY *PFN_glFramebufferParameteri)( GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glFramebufferParameteriMESA)( GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glFramebufferReadBufferEXT)( GLuint framebuffer, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glFramebufferRenderbuffer)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glFramebufferRenderbufferEXT)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glFramebufferSampleLocationsfvARB)( GLenum target, GLuint start, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glFramebufferSampleLocationsfvNV)( GLenum target, GLuint start, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glFramebufferSamplePositionsfvAMD)( GLenum target, GLuint numsamples, GLuint pixelindex, const GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glFramebufferTexture)( GLenum target, GLenum attachment, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glFramebufferTexture1D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glFramebufferTexture1DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glFramebufferTexture2D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glFramebufferTexture2DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glFramebufferTexture3D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
typedef void       (GLAPIENTRY *PFN_glFramebufferTexture3DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureARB)( GLenum target, GLenum attachment, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureFaceARB)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureFaceEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureLayer)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureLayerARB)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureLayerEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
typedef void       (GLAPIENTRY *PFN_glFramebufferTextureMultiviewOVR)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews );
typedef void       (GLAPIENTRY *PFN_glFreeObjectBufferATI)( GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glFrustumfOES)( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f );
typedef void       (GLAPIENTRY *PFN_glFrustumxOES)( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f );
typedef GLuint     (GLAPIENTRY *PFN_glGenAsyncMarkersSGIX)( GLsizei range );
typedef void       (GLAPIENTRY *PFN_glGenBuffers)( GLsizei n, GLuint *buffers );
typedef void       (GLAPIENTRY *PFN_glGenBuffersARB)( GLsizei n, GLuint *buffers );
typedef void       (GLAPIENTRY *PFN_glGenFencesAPPLE)( GLsizei n, GLuint *fences );
typedef void       (GLAPIENTRY *PFN_glGenFencesNV)( GLsizei n, GLuint *fences );
typedef GLuint     (GLAPIENTRY *PFN_glGenFragmentShadersATI)( GLuint range );
typedef void       (GLAPIENTRY *PFN_glGenFramebuffers)( GLsizei n, GLuint *framebuffers );
typedef void       (GLAPIENTRY *PFN_glGenFramebuffersEXT)( GLsizei n, GLuint *framebuffers );
typedef void       (GLAPIENTRY *PFN_glGenNamesAMD)( GLenum identifier, GLuint num, GLuint *names );
typedef void       (GLAPIENTRY *PFN_glGenOcclusionQueriesNV)( GLsizei n, GLuint *ids );
typedef GLuint     (GLAPIENTRY *PFN_glGenPathsNV)( GLsizei range );
typedef void       (GLAPIENTRY *PFN_glGenPerfMonitorsAMD)( GLsizei n, GLuint *monitors );
typedef void       (GLAPIENTRY *PFN_glGenProgramPipelines)( GLsizei n, GLuint *pipelines );
typedef void       (GLAPIENTRY *PFN_glGenProgramsARB)( GLsizei n, GLuint *programs );
typedef void       (GLAPIENTRY *PFN_glGenProgramsNV)( GLsizei n, GLuint *programs );
typedef void       (GLAPIENTRY *PFN_glGenQueries)( GLsizei n, GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glGenQueriesARB)( GLsizei n, GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glGenQueryResourceTagNV)( GLsizei n, GLint *tagIds );
typedef void       (GLAPIENTRY *PFN_glGenRenderbuffers)( GLsizei n, GLuint *renderbuffers );
typedef void       (GLAPIENTRY *PFN_glGenRenderbuffersEXT)( GLsizei n, GLuint *renderbuffers );
typedef void       (GLAPIENTRY *PFN_glGenSamplers)( GLsizei count, GLuint *samplers );
typedef void       (GLAPIENTRY *PFN_glGenSemaphoresEXT)( GLsizei n, GLuint *semaphores );
typedef GLuint     (GLAPIENTRY *PFN_glGenSymbolsEXT)( GLenum datatype, GLenum storagetype, GLenum range, GLuint components );
typedef void       (GLAPIENTRY *PFN_glGenTexturesEXT)( GLsizei n, GLuint *textures );
typedef void       (GLAPIENTRY *PFN_glGenTransformFeedbacks)( GLsizei n, GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glGenTransformFeedbacksNV)( GLsizei n, GLuint *ids );
typedef void       (GLAPIENTRY *PFN_glGenVertexArrays)( GLsizei n, GLuint *arrays );
typedef void       (GLAPIENTRY *PFN_glGenVertexArraysAPPLE)( GLsizei n, GLuint *arrays );
typedef GLuint     (GLAPIENTRY *PFN_glGenVertexShadersEXT)( GLuint range );
typedef void       (GLAPIENTRY *PFN_glGenerateMipmap)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glGenerateMipmapEXT)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glGenerateMultiTexMipmapEXT)( GLenum texunit, GLenum target );
typedef void       (GLAPIENTRY *PFN_glGenerateTextureMipmap)( GLuint texture );
typedef void       (GLAPIENTRY *PFN_glGenerateTextureMipmapEXT)( GLuint texture, GLenum target );
typedef void       (GLAPIENTRY *PFN_glGetActiveAtomicCounterBufferiv)( GLuint program, GLuint bufferIndex, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetActiveAttrib)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetActiveAttribARB)( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name );
typedef void       (GLAPIENTRY *PFN_glGetActiveSubroutineName)( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetActiveSubroutineUniformName)( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetActiveSubroutineUniformiv)( GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values );
typedef void       (GLAPIENTRY *PFN_glGetActiveUniform)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetActiveUniformARB)( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name );
typedef void       (GLAPIENTRY *PFN_glGetActiveUniformBlockName)( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName );
typedef void       (GLAPIENTRY *PFN_glGetActiveUniformBlockiv)( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetActiveUniformName)( GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName );
typedef void       (GLAPIENTRY *PFN_glGetActiveUniformsiv)( GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetActiveVaryingNV)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetArrayObjectfvATI)( GLenum array, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetArrayObjectivATI)( GLenum array, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetAttachedObjectsARB)( GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj );
typedef void       (GLAPIENTRY *PFN_glGetAttachedShaders)( GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders );
typedef GLint      (GLAPIENTRY *PFN_glGetAttribLocation)( GLuint program, const GLchar *name );
typedef GLint      (GLAPIENTRY *PFN_glGetAttribLocationARB)( GLhandleARB programObj, const GLcharARB *name );
typedef void       (GLAPIENTRY *PFN_glGetBooleanIndexedvEXT)( GLenum target, GLuint index, GLboolean *data );
typedef void       (GLAPIENTRY *PFN_glGetBooleani_v)( GLenum target, GLuint index, GLboolean *data );
typedef void       (GLAPIENTRY *PFN_glGetBufferParameteri64v)( GLenum target, GLenum pname, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetBufferParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetBufferParameterivARB)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetBufferParameterui64vNV)( GLenum target, GLenum pname, GLuint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetBufferPointerv)( GLenum target, GLenum pname, void **params );
typedef void       (GLAPIENTRY *PFN_glGetBufferPointervARB)( GLenum target, GLenum pname, void **params );
typedef void       (GLAPIENTRY *PFN_glGetBufferSubData)( GLenum target, GLintptr offset, GLsizeiptr size, void *data );
typedef void       (GLAPIENTRY *PFN_glGetBufferSubDataARB)( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void *data );
typedef void       (GLAPIENTRY *PFN_glGetClipPlanefOES)( GLenum plane, GLfloat *equation );
typedef void       (GLAPIENTRY *PFN_glGetClipPlanexOES)( GLenum plane, GLfixed *equation );
typedef void       (GLAPIENTRY *PFN_glGetColorTable)( GLenum target, GLenum format, GLenum type, void *table );
typedef void       (GLAPIENTRY *PFN_glGetColorTableEXT)( GLenum target, GLenum format, GLenum type, void *data );
typedef void       (GLAPIENTRY *PFN_glGetColorTableParameterfv)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetColorTableParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetColorTableParameterfvSGI)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetColorTableParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetColorTableParameterivEXT)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetColorTableParameterivSGI)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetColorTableSGI)( GLenum target, GLenum format, GLenum type, void *table );
typedef void       (GLAPIENTRY *PFN_glGetCombinerInputParameterfvNV)( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetCombinerInputParameterivNV)( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetCombinerOutputParameterfvNV)( GLenum stage, GLenum portion, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetCombinerOutputParameterivNV)( GLenum stage, GLenum portion, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetCombinerStageParameterfvNV)( GLenum stage, GLenum pname, GLfloat *params );
typedef GLuint     (GLAPIENTRY *PFN_glGetCommandHeaderNV)( GLenum tokenID, GLuint size );
typedef void       (GLAPIENTRY *PFN_glGetCompressedMultiTexImageEXT)( GLenum texunit, GLenum target, GLint lod, void *img );
typedef void       (GLAPIENTRY *PFN_glGetCompressedTexImage)( GLenum target, GLint level, void *img );
typedef void       (GLAPIENTRY *PFN_glGetCompressedTexImageARB)( GLenum target, GLint level, void *img );
typedef void       (GLAPIENTRY *PFN_glGetCompressedTextureImage)( GLuint texture, GLint level, GLsizei bufSize, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetCompressedTextureImageEXT)( GLuint texture, GLenum target, GLint lod, void *img );
typedef void       (GLAPIENTRY *PFN_glGetCompressedTextureSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetConvolutionFilter)( GLenum target, GLenum format, GLenum type, void *image );
typedef void       (GLAPIENTRY *PFN_glGetConvolutionFilterEXT)( GLenum target, GLenum format, GLenum type, void *image );
typedef void       (GLAPIENTRY *PFN_glGetConvolutionParameterfv)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetConvolutionParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetConvolutionParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetConvolutionParameterivEXT)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetConvolutionParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glGetCoverageModulationTableNV)( GLsizei bufSize, GLfloat *v );
typedef GLuint     (GLAPIENTRY *PFN_glGetDebugMessageLog)( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog );
typedef GLuint     (GLAPIENTRY *PFN_glGetDebugMessageLogAMD)( GLuint count, GLsizei bufSize, GLenum *categories, GLuint *severities, GLuint *ids, GLsizei *lengths, GLchar *message );
typedef GLuint     (GLAPIENTRY *PFN_glGetDebugMessageLogARB)( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog );
typedef void       (GLAPIENTRY *PFN_glGetDetailTexFuncSGIS)( GLenum target, GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glGetDoubleIndexedvEXT)( GLenum target, GLuint index, GLdouble *data );
typedef void       (GLAPIENTRY *PFN_glGetDoublei_v)( GLenum target, GLuint index, GLdouble *data );
typedef void       (GLAPIENTRY *PFN_glGetDoublei_vEXT)( GLenum pname, GLuint index, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetFenceivNV)( GLuint fence, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFinalCombinerInputParameterfvNV)( GLenum variable, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetFinalCombinerInputParameterivNV)( GLenum variable, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFirstPerfQueryIdINTEL)( GLuint *queryId );
typedef void       (GLAPIENTRY *PFN_glGetFixedvOES)( GLenum pname, GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glGetFloatIndexedvEXT)( GLenum target, GLuint index, GLfloat *data );
typedef void       (GLAPIENTRY *PFN_glGetFloati_v)( GLenum target, GLuint index, GLfloat *data );
typedef void       (GLAPIENTRY *PFN_glGetFloati_vEXT)( GLenum pname, GLuint index, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetFogFuncSGIS)( GLfloat *points );
typedef GLint      (GLAPIENTRY *PFN_glGetFragDataIndex)( GLuint program, const GLchar *name );
typedef GLint      (GLAPIENTRY *PFN_glGetFragDataLocation)( GLuint program, const GLchar *name );
typedef GLint      (GLAPIENTRY *PFN_glGetFragDataLocationEXT)( GLuint program, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetFragmentLightfvSGIX)( GLenum light, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetFragmentLightivSGIX)( GLenum light, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFragmentMaterialfvSGIX)( GLenum face, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetFragmentMaterialivSGIX)( GLenum face, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFramebufferAttachmentParameteriv)( GLenum target, GLenum attachment, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFramebufferAttachmentParameterivEXT)( GLenum target, GLenum attachment, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFramebufferParameterfvAMD)( GLenum target, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glGetFramebufferParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFramebufferParameterivEXT)( GLuint framebuffer, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetFramebufferParameterivMESA)( GLenum target, GLenum pname, GLint *params );
typedef GLenum     (GLAPIENTRY *PFN_glGetGraphicsResetStatus)(void);
typedef GLenum     (GLAPIENTRY *PFN_glGetGraphicsResetStatusARB)(void);
typedef GLhandleARB (GLAPIENTRY *PFN_glGetHandleARB)( GLenum pname );
typedef void       (GLAPIENTRY *PFN_glGetHistogram)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
typedef void       (GLAPIENTRY *PFN_glGetHistogramEXT)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
typedef void       (GLAPIENTRY *PFN_glGetHistogramParameterfv)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetHistogramParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetHistogramParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetHistogramParameterivEXT)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetHistogramParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
typedef GLuint64   (GLAPIENTRY *PFN_glGetImageHandleARB)( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format );
typedef GLuint64   (GLAPIENTRY *PFN_glGetImageHandleNV)( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format );
typedef void       (GLAPIENTRY *PFN_glGetImageTransformParameterfvHP)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetImageTransformParameterivHP)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetInfoLogARB)( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog );
typedef GLint      (GLAPIENTRY *PFN_glGetInstrumentsSGIX)(void);
typedef void       (GLAPIENTRY *PFN_glGetInteger64i_v)( GLenum target, GLuint index, GLint64 *data );
typedef void       (GLAPIENTRY *PFN_glGetInteger64v)( GLenum pname, GLint64 *data );
typedef void       (GLAPIENTRY *PFN_glGetIntegerIndexedvEXT)( GLenum target, GLuint index, GLint *data );
typedef void       (GLAPIENTRY *PFN_glGetIntegeri_v)( GLenum target, GLuint index, GLint *data );
typedef void       (GLAPIENTRY *PFN_glGetIntegerui64i_vNV)( GLenum value, GLuint index, GLuint64EXT *result );
typedef void       (GLAPIENTRY *PFN_glGetIntegerui64vNV)( GLenum value, GLuint64EXT *result );
typedef void       (GLAPIENTRY *PFN_glGetInternalformatSampleivNV)( GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei count, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetInternalformati64v)( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetInternalformativ)( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetInvariantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
typedef void       (GLAPIENTRY *PFN_glGetInvariantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
typedef void       (GLAPIENTRY *PFN_glGetInvariantIntegervEXT)( GLuint id, GLenum value, GLint *data );
typedef void       (GLAPIENTRY *PFN_glGetLightxOES)( GLenum light, GLenum pname, GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glGetListParameterfvSGIX)( GLuint list, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetListParameterivSGIX)( GLuint list, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetLocalConstantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
typedef void       (GLAPIENTRY *PFN_glGetLocalConstantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
typedef void       (GLAPIENTRY *PFN_glGetLocalConstantIntegervEXT)( GLuint id, GLenum value, GLint *data );
typedef void       (GLAPIENTRY *PFN_glGetMapAttribParameterfvNV)( GLenum target, GLuint index, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMapAttribParameterivNV)( GLenum target, GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMapControlPointsNV)( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, void *points );
typedef void       (GLAPIENTRY *PFN_glGetMapParameterfvNV)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMapParameterivNV)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMapxvOES)( GLenum target, GLenum query, GLfixed *v );
typedef void       (GLAPIENTRY *PFN_glGetMaterialxOES)( GLenum face, GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glGetMemoryObjectDetachedResourcesuivNV)( GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetMemoryObjectParameterivEXT)( GLuint memoryObject, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMinmax)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
typedef void       (GLAPIENTRY *PFN_glGetMinmaxEXT)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
typedef void       (GLAPIENTRY *PFN_glGetMinmaxParameterfv)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMinmaxParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMinmaxParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMinmaxParameterivEXT)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexEnvfvEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexEnvivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexGendvEXT)( GLenum texunit, GLenum coord, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexGenfvEXT)( GLenum texunit, GLenum coord, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexGenivEXT)( GLenum texunit, GLenum coord, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexImageEXT)( GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexLevelParameterfvEXT)( GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexLevelParameterivEXT)( GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexParameterIivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexParameterIuivEXT)( GLenum texunit, GLenum target, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexParameterfvEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetMultiTexParameterivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetMultisamplefv)( GLenum pname, GLuint index, GLfloat *val );
typedef void       (GLAPIENTRY *PFN_glGetMultisamplefvNV)( GLenum pname, GLuint index, GLfloat *val );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferParameteri64v)( GLuint buffer, GLenum pname, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferParameteriv)( GLuint buffer, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferParameterivEXT)( GLuint buffer, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferParameterui64vNV)( GLuint buffer, GLenum pname, GLuint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferPointerv)( GLuint buffer, GLenum pname, void **params );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferPointervEXT)( GLuint buffer, GLenum pname, void **params );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data );
typedef void       (GLAPIENTRY *PFN_glGetNamedBufferSubDataEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data );
typedef void       (GLAPIENTRY *PFN_glGetNamedFramebufferAttachmentParameteriv)( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedFramebufferAttachmentParameterivEXT)( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedFramebufferParameterfvAMD)( GLuint framebuffer, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glGetNamedFramebufferParameteriv)( GLuint framebuffer, GLenum pname, GLint *param );
typedef void       (GLAPIENTRY *PFN_glGetNamedFramebufferParameterivEXT)( GLuint framebuffer, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedProgramLocalParameterIivEXT)( GLuint program, GLenum target, GLuint index, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedProgramLocalParameterIuivEXT)( GLuint program, GLenum target, GLuint index, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedProgramLocalParameterdvEXT)( GLuint program, GLenum target, GLuint index, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedProgramLocalParameterfvEXT)( GLuint program, GLenum target, GLuint index, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedProgramStringEXT)( GLuint program, GLenum target, GLenum pname, void *string );
typedef void       (GLAPIENTRY *PFN_glGetNamedProgramivEXT)( GLuint program, GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedRenderbufferParameteriv)( GLuint renderbuffer, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedRenderbufferParameterivEXT)( GLuint renderbuffer, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNamedStringARB)( GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string );
typedef void       (GLAPIENTRY *PFN_glGetNamedStringivARB)( GLint namelen, const GLchar *name, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetNextPerfQueryIdINTEL)( GLuint queryId, GLuint *nextQueryId );
typedef void       (GLAPIENTRY *PFN_glGetObjectBufferfvATI)( GLuint buffer, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetObjectBufferivATI)( GLuint buffer, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetObjectLabel)( GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label );
typedef void       (GLAPIENTRY *PFN_glGetObjectLabelEXT)( GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label );
typedef void       (GLAPIENTRY *PFN_glGetObjectParameterfvARB)( GLhandleARB obj, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetObjectParameterivAPPLE)( GLenum objectType, GLuint name, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetObjectParameterivARB)( GLhandleARB obj, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetObjectPtrLabel)( const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label );
typedef void       (GLAPIENTRY *PFN_glGetOcclusionQueryivNV)( GLuint id, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetOcclusionQueryuivNV)( GLuint id, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetPathColorGenfvNV)( GLenum color, GLenum pname, GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glGetPathColorGenivNV)( GLenum color, GLenum pname, GLint *value );
typedef void       (GLAPIENTRY *PFN_glGetPathCommandsNV)( GLuint path, GLubyte *commands );
typedef void       (GLAPIENTRY *PFN_glGetPathCoordsNV)( GLuint path, GLfloat *coords );
typedef void       (GLAPIENTRY *PFN_glGetPathDashArrayNV)( GLuint path, GLfloat *dashArray );
typedef GLfloat    (GLAPIENTRY *PFN_glGetPathLengthNV)( GLuint path, GLsizei startSegment, GLsizei numSegments );
typedef void       (GLAPIENTRY *PFN_glGetPathMetricRangeNV)( GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics );
typedef void       (GLAPIENTRY *PFN_glGetPathMetricsNV)( GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics );
typedef void       (GLAPIENTRY *PFN_glGetPathParameterfvNV)( GLuint path, GLenum pname, GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glGetPathParameterivNV)( GLuint path, GLenum pname, GLint *value );
typedef void       (GLAPIENTRY *PFN_glGetPathSpacingNV)( GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing );
typedef void       (GLAPIENTRY *PFN_glGetPathTexGenfvNV)( GLenum texCoordSet, GLenum pname, GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glGetPathTexGenivNV)( GLenum texCoordSet, GLenum pname, GLint *value );
typedef void       (GLAPIENTRY *PFN_glGetPerfCounterInfoINTEL)( GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue );
typedef void       (GLAPIENTRY *PFN_glGetPerfMonitorCounterDataAMD)( GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten );
typedef void       (GLAPIENTRY *PFN_glGetPerfMonitorCounterInfoAMD)( GLuint group, GLuint counter, GLenum pname, void *data );
typedef void       (GLAPIENTRY *PFN_glGetPerfMonitorCounterStringAMD)( GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString );
typedef void       (GLAPIENTRY *PFN_glGetPerfMonitorCountersAMD)( GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters );
typedef void       (GLAPIENTRY *PFN_glGetPerfMonitorGroupStringAMD)( GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString );
typedef void       (GLAPIENTRY *PFN_glGetPerfMonitorGroupsAMD)( GLint *numGroups, GLsizei groupsSize, GLuint *groups );
typedef void       (GLAPIENTRY *PFN_glGetPerfQueryDataINTEL)( GLuint queryHandle, GLuint flags, GLsizei dataSize, void *data, GLuint *bytesWritten );
typedef void       (GLAPIENTRY *PFN_glGetPerfQueryIdByNameINTEL)( GLchar *queryName, GLuint *queryId );
typedef void       (GLAPIENTRY *PFN_glGetPerfQueryInfoINTEL)( GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask );
typedef void       (GLAPIENTRY *PFN_glGetPixelMapxv)( GLenum map, GLint size, GLfixed *values );
typedef void       (GLAPIENTRY *PFN_glGetPixelTexGenParameterfvSGIS)( GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetPixelTexGenParameterivSGIS)( GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetPixelTransformParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetPixelTransformParameterivEXT)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetPointerIndexedvEXT)( GLenum target, GLuint index, void **data );
typedef void       (GLAPIENTRY *PFN_glGetPointeri_vEXT)( GLenum pname, GLuint index, void **params );
typedef void       (GLAPIENTRY *PFN_glGetPointervEXT)( GLenum pname, void **params );
typedef void       (GLAPIENTRY *PFN_glGetProgramBinary)( GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary );
typedef void       (GLAPIENTRY *PFN_glGetProgramEnvParameterIivNV)( GLenum target, GLuint index, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramEnvParameterIuivNV)( GLenum target, GLuint index, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramEnvParameterdvARB)( GLenum target, GLuint index, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramEnvParameterfvARB)( GLenum target, GLuint index, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramInfoLog)( GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
typedef void       (GLAPIENTRY *PFN_glGetProgramInterfaceiv)( GLuint program, GLenum programInterface, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramLocalParameterIivNV)( GLenum target, GLuint index, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramLocalParameterIuivNV)( GLenum target, GLuint index, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramLocalParameterdvARB)( GLenum target, GLuint index, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramLocalParameterfvARB)( GLenum target, GLuint index, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramNamedParameterdvNV)( GLuint id, GLsizei len, const GLubyte *name, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramNamedParameterfvNV)( GLuint id, GLsizei len, const GLubyte *name, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramParameterdvNV)( GLenum target, GLuint index, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramParameterfvNV)( GLenum target, GLuint index, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramPipelineInfoLog)( GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
typedef void       (GLAPIENTRY *PFN_glGetProgramPipelineiv)( GLuint pipeline, GLenum pname, GLint *params );
typedef GLuint     (GLAPIENTRY *PFN_glGetProgramResourceIndex)( GLuint program, GLenum programInterface, const GLchar *name );
typedef GLint      (GLAPIENTRY *PFN_glGetProgramResourceLocation)( GLuint program, GLenum programInterface, const GLchar *name );
typedef GLint      (GLAPIENTRY *PFN_glGetProgramResourceLocationIndex)( GLuint program, GLenum programInterface, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetProgramResourceName)( GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetProgramResourcefvNV)( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramResourceiv)( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramStageiv)( GLuint program, GLenum shadertype, GLenum pname, GLint *values );
typedef void       (GLAPIENTRY *PFN_glGetProgramStringARB)( GLenum target, GLenum pname, void *string );
typedef void       (GLAPIENTRY *PFN_glGetProgramStringNV)( GLuint id, GLenum pname, GLubyte *program );
typedef void       (GLAPIENTRY *PFN_glGetProgramSubroutineParameteruivNV)( GLenum target, GLuint index, GLuint *param );
typedef void       (GLAPIENTRY *PFN_glGetProgramiv)( GLuint program, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramivARB)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetProgramivNV)( GLuint id, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryBufferObjecti64v)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glGetQueryBufferObjectiv)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glGetQueryBufferObjectui64v)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glGetQueryBufferObjectuiv)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glGetQueryIndexediv)( GLenum target, GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjecti64v)( GLuint id, GLenum pname, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjecti64vEXT)( GLuint id, GLenum pname, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjectiv)( GLuint id, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjectivARB)( GLuint id, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjectui64v)( GLuint id, GLenum pname, GLuint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjectui64vEXT)( GLuint id, GLenum pname, GLuint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjectuiv)( GLuint id, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryObjectuivARB)( GLuint id, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryiv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetQueryivARB)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetRenderbufferParameteriv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetRenderbufferParameterivEXT)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetSamplerParameterIiv)( GLuint sampler, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetSamplerParameterIuiv)( GLuint sampler, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetSamplerParameterfv)( GLuint sampler, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetSamplerParameteriv)( GLuint sampler, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetSemaphoreParameterui64vEXT)( GLuint semaphore, GLenum pname, GLuint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetSeparableFilter)( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span );
typedef void       (GLAPIENTRY *PFN_glGetSeparableFilterEXT)( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span );
typedef void       (GLAPIENTRY *PFN_glGetShaderInfoLog)( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
typedef void       (GLAPIENTRY *PFN_glGetShaderPrecisionFormat)( GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision );
typedef void       (GLAPIENTRY *PFN_glGetShaderSource)( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source );
typedef void       (GLAPIENTRY *PFN_glGetShaderSourceARB)( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source );
typedef void       (GLAPIENTRY *PFN_glGetShaderiv)( GLuint shader, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetShadingRateImagePaletteNV)( GLuint viewport, GLuint entry, GLenum *rate );
typedef void       (GLAPIENTRY *PFN_glGetShadingRateSampleLocationivNV)( GLenum rate, GLuint samples, GLuint index, GLint *location );
typedef void       (GLAPIENTRY *PFN_glGetSharpenTexFuncSGIS)( GLenum target, GLfloat *points );
typedef GLushort   (GLAPIENTRY *PFN_glGetStageIndexNV)( GLenum shadertype );
typedef const GLubyte * (GLAPIENTRY *PFN_glGetStringi)( GLenum name, GLuint index );
typedef GLuint     (GLAPIENTRY *PFN_glGetSubroutineIndex)( GLuint program, GLenum shadertype, const GLchar *name );
typedef GLint      (GLAPIENTRY *PFN_glGetSubroutineUniformLocation)( GLuint program, GLenum shadertype, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetSynciv)( GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values );
typedef void       (GLAPIENTRY *PFN_glGetTexBumpParameterfvATI)( GLenum pname, GLfloat *param );
typedef void       (GLAPIENTRY *PFN_glGetTexBumpParameterivATI)( GLenum pname, GLint *param );
typedef void       (GLAPIENTRY *PFN_glGetTexEnvxvOES)( GLenum target, GLenum pname, GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glGetTexFilterFuncSGIS)( GLenum target, GLenum filter, GLfloat *weights );
typedef void       (GLAPIENTRY *PFN_glGetTexGenxvOES)( GLenum coord, GLenum pname, GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glGetTexLevelParameterxvOES)( GLenum target, GLint level, GLenum pname, GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameterIiv)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameterIivEXT)( GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameterIuiv)( GLenum target, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameterIuivEXT)( GLenum target, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameterPointervAPPLE)( GLenum target, GLenum pname, void **params );
typedef void       (GLAPIENTRY *PFN_glGetTexParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
typedef GLuint64   (GLAPIENTRY *PFN_glGetTextureHandleARB)( GLuint texture );
typedef GLuint64   (GLAPIENTRY *PFN_glGetTextureHandleNV)( GLuint texture );
typedef void       (GLAPIENTRY *PFN_glGetTextureImage)( GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetTextureImageEXT)( GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetTextureLevelParameterfv)( GLuint texture, GLint level, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureLevelParameterfvEXT)( GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureLevelParameteriv)( GLuint texture, GLint level, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureLevelParameterivEXT)( GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameterIiv)( GLuint texture, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameterIivEXT)( GLuint texture, GLenum target, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameterIuiv)( GLuint texture, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameterIuivEXT)( GLuint texture, GLenum target, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameterfv)( GLuint texture, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameterfvEXT)( GLuint texture, GLenum target, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameteriv)( GLuint texture, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTextureParameterivEXT)( GLuint texture, GLenum target, GLenum pname, GLint *params );
typedef GLuint64   (GLAPIENTRY *PFN_glGetTextureSamplerHandleARB)( GLuint texture, GLuint sampler );
typedef GLuint64   (GLAPIENTRY *PFN_glGetTextureSamplerHandleNV)( GLuint texture, GLuint sampler );
typedef void       (GLAPIENTRY *PFN_glGetTextureSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetTrackMatrixivNV)( GLenum target, GLuint address, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetTransformFeedbackVarying)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetTransformFeedbackVaryingEXT)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetTransformFeedbackVaryingNV)( GLuint program, GLuint index, GLint *location );
typedef void       (GLAPIENTRY *PFN_glGetTransformFeedbacki64_v)( GLuint xfb, GLenum pname, GLuint index, GLint64 *param );
typedef void       (GLAPIENTRY *PFN_glGetTransformFeedbacki_v)( GLuint xfb, GLenum pname, GLuint index, GLint *param );
typedef void       (GLAPIENTRY *PFN_glGetTransformFeedbackiv)( GLuint xfb, GLenum pname, GLint *param );
typedef GLuint     (GLAPIENTRY *PFN_glGetUniformBlockIndex)( GLuint program, const GLchar *uniformBlockName );
typedef GLint      (GLAPIENTRY *PFN_glGetUniformBufferSizeEXT)( GLuint program, GLint location );
typedef void       (GLAPIENTRY *PFN_glGetUniformIndices)( GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices );
typedef GLint      (GLAPIENTRY *PFN_glGetUniformLocation)( GLuint program, const GLchar *name );
typedef GLint      (GLAPIENTRY *PFN_glGetUniformLocationARB)( GLhandleARB programObj, const GLcharARB *name );
typedef GLintptr   (GLAPIENTRY *PFN_glGetUniformOffsetEXT)( GLuint program, GLint location );
typedef void       (GLAPIENTRY *PFN_glGetUniformSubroutineuiv)( GLenum shadertype, GLint location, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformdv)( GLuint program, GLint location, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformfv)( GLuint program, GLint location, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformfvARB)( GLhandleARB programObj, GLint location, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformi64vARB)( GLuint program, GLint location, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformi64vNV)( GLuint program, GLint location, GLint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformiv)( GLuint program, GLint location, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformivARB)( GLhandleARB programObj, GLint location, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformui64vARB)( GLuint program, GLint location, GLuint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformui64vNV)( GLuint program, GLint location, GLuint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformuiv)( GLuint program, GLint location, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetUniformuivEXT)( GLuint program, GLint location, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetUnsignedBytei_vEXT)( GLenum target, GLuint index, GLubyte *data );
typedef void       (GLAPIENTRY *PFN_glGetUnsignedBytevEXT)( GLenum pname, GLubyte *data );
typedef void       (GLAPIENTRY *PFN_glGetVariantArrayObjectfvATI)( GLuint id, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetVariantArrayObjectivATI)( GLuint id, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVariantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
typedef void       (GLAPIENTRY *PFN_glGetVariantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
typedef void       (GLAPIENTRY *PFN_glGetVariantIntegervEXT)( GLuint id, GLenum value, GLint *data );
typedef void       (GLAPIENTRY *PFN_glGetVariantPointervEXT)( GLuint id, GLenum value, void **data );
typedef GLint      (GLAPIENTRY *PFN_glGetVaryingLocationNV)( GLuint program, const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetVertexArrayIndexed64iv)( GLuint vaobj, GLuint index, GLenum pname, GLint64 *param );
typedef void       (GLAPIENTRY *PFN_glGetVertexArrayIndexediv)( GLuint vaobj, GLuint index, GLenum pname, GLint *param );
typedef void       (GLAPIENTRY *PFN_glGetVertexArrayIntegeri_vEXT)( GLuint vaobj, GLuint index, GLenum pname, GLint *param );
typedef void       (GLAPIENTRY *PFN_glGetVertexArrayIntegervEXT)( GLuint vaobj, GLenum pname, GLint *param );
typedef void       (GLAPIENTRY *PFN_glGetVertexArrayPointeri_vEXT)( GLuint vaobj, GLuint index, GLenum pname, void **param );
typedef void       (GLAPIENTRY *PFN_glGetVertexArrayPointervEXT)( GLuint vaobj, GLenum pname, void **param );
typedef void       (GLAPIENTRY *PFN_glGetVertexArrayiv)( GLuint vaobj, GLenum pname, GLint *param );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribArrayObjectfvATI)( GLuint index, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribArrayObjectivATI)( GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribIiv)( GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribIivEXT)( GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribIuiv)( GLuint index, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribIuivEXT)( GLuint index, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribLdv)( GLuint index, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribLdvEXT)( GLuint index, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribLi64vNV)( GLuint index, GLenum pname, GLint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribLui64vARB)( GLuint index, GLenum pname, GLuint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribLui64vNV)( GLuint index, GLenum pname, GLuint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribPointerv)( GLuint index, GLenum pname, void **pointer );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribPointervARB)( GLuint index, GLenum pname, void **pointer );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribPointervNV)( GLuint index, GLenum pname, void **pointer );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribdv)( GLuint index, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribdvARB)( GLuint index, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribdvNV)( GLuint index, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribfv)( GLuint index, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribfvARB)( GLuint index, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribfvNV)( GLuint index, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribiv)( GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribivARB)( GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVertexAttribivNV)( GLuint index, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVideoCaptureStreamdvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetVideoCaptureStreamfvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetVideoCaptureStreamivNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVideoCaptureivNV)( GLuint video_capture_slot, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVideoi64vNV)( GLuint video_slot, GLenum pname, GLint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetVideoivNV)( GLuint video_slot, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetVideoui64vNV)( GLuint video_slot, GLenum pname, GLuint64EXT *params );
typedef void       (GLAPIENTRY *PFN_glGetVideouivNV)( GLuint video_slot, GLenum pname, GLuint *params );
typedef GLVULKANPROCNV (GLAPIENTRY *PFN_glGetVkProcAddrNV)( const GLchar *name );
typedef void       (GLAPIENTRY *PFN_glGetnColorTable)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table );
typedef void       (GLAPIENTRY *PFN_glGetnColorTableARB)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table );
typedef void       (GLAPIENTRY *PFN_glGetnCompressedTexImage)( GLenum target, GLint lod, GLsizei bufSize, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetnCompressedTexImageARB)( GLenum target, GLint lod, GLsizei bufSize, void *img );
typedef void       (GLAPIENTRY *PFN_glGetnConvolutionFilter)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image );
typedef void       (GLAPIENTRY *PFN_glGetnConvolutionFilterARB)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image );
typedef void       (GLAPIENTRY *PFN_glGetnHistogram)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
typedef void       (GLAPIENTRY *PFN_glGetnHistogramARB)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
typedef void       (GLAPIENTRY *PFN_glGetnMapdv)( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glGetnMapdvARB)( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glGetnMapfv)( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glGetnMapfvARB)( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glGetnMapiv)( GLenum target, GLenum query, GLsizei bufSize, GLint *v );
typedef void       (GLAPIENTRY *PFN_glGetnMapivARB)( GLenum target, GLenum query, GLsizei bufSize, GLint *v );
typedef void       (GLAPIENTRY *PFN_glGetnMinmax)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
typedef void       (GLAPIENTRY *PFN_glGetnMinmaxARB)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
typedef void       (GLAPIENTRY *PFN_glGetnPixelMapfv)( GLenum map, GLsizei bufSize, GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glGetnPixelMapfvARB)( GLenum map, GLsizei bufSize, GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glGetnPixelMapuiv)( GLenum map, GLsizei bufSize, GLuint *values );
typedef void       (GLAPIENTRY *PFN_glGetnPixelMapuivARB)( GLenum map, GLsizei bufSize, GLuint *values );
typedef void       (GLAPIENTRY *PFN_glGetnPixelMapusv)( GLenum map, GLsizei bufSize, GLushort *values );
typedef void       (GLAPIENTRY *PFN_glGetnPixelMapusvARB)( GLenum map, GLsizei bufSize, GLushort *values );
typedef void       (GLAPIENTRY *PFN_glGetnPolygonStipple)( GLsizei bufSize, GLubyte *pattern );
typedef void       (GLAPIENTRY *PFN_glGetnPolygonStippleARB)( GLsizei bufSize, GLubyte *pattern );
typedef void       (GLAPIENTRY *PFN_glGetnSeparableFilter)( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span );
typedef void       (GLAPIENTRY *PFN_glGetnSeparableFilterARB)( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span );
typedef void       (GLAPIENTRY *PFN_glGetnTexImage)( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
typedef void       (GLAPIENTRY *PFN_glGetnTexImageARB)( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img );
typedef void       (GLAPIENTRY *PFN_glGetnUniformdv)( GLuint program, GLint location, GLsizei bufSize, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformdvARB)( GLuint program, GLint location, GLsizei bufSize, GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformfv)( GLuint program, GLint location, GLsizei bufSize, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformfvARB)( GLuint program, GLint location, GLsizei bufSize, GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformi64vARB)( GLuint program, GLint location, GLsizei bufSize, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformiv)( GLuint program, GLint location, GLsizei bufSize, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformivARB)( GLuint program, GLint location, GLsizei bufSize, GLint *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformui64vARB)( GLuint program, GLint location, GLsizei bufSize, GLuint64 *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformuiv)( GLuint program, GLint location, GLsizei bufSize, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGetnUniformuivARB)( GLuint program, GLint location, GLsizei bufSize, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactorbSUN)( GLbyte factor );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactordSUN)( GLdouble factor );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactorfSUN)( GLfloat factor );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactoriSUN)( GLint factor );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactorsSUN)( GLshort factor );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactorubSUN)( GLubyte factor );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactoruiSUN)( GLuint factor );
typedef void       (GLAPIENTRY *PFN_glGlobalAlphaFactorusSUN)( GLushort factor );
typedef void       (GLAPIENTRY *PFN_glHintPGI)( GLenum target, GLint mode );
typedef void       (GLAPIENTRY *PFN_glHistogram)( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink );
typedef void       (GLAPIENTRY *PFN_glHistogramEXT)( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink );
typedef void       (GLAPIENTRY *PFN_glIglooInterfaceSGIX)( GLenum pname, const void *params );
typedef void       (GLAPIENTRY *PFN_glImageTransformParameterfHP)( GLenum target, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glImageTransformParameterfvHP)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glImageTransformParameteriHP)( GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glImageTransformParameterivHP)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glImportMemoryFdEXT)( GLuint memory, GLuint64 size, GLenum handleType, GLint fd );
typedef void       (GLAPIENTRY *PFN_glImportMemoryWin32HandleEXT)( GLuint memory, GLuint64 size, GLenum handleType, void *handle );
typedef void       (GLAPIENTRY *PFN_glImportMemoryWin32NameEXT)( GLuint memory, GLuint64 size, GLenum handleType, const void *name );
typedef void       (GLAPIENTRY *PFN_glImportSemaphoreFdEXT)( GLuint semaphore, GLenum handleType, GLint fd );
typedef void       (GLAPIENTRY *PFN_glImportSemaphoreWin32HandleEXT)( GLuint semaphore, GLenum handleType, void *handle );
typedef void       (GLAPIENTRY *PFN_glImportSemaphoreWin32NameEXT)( GLuint semaphore, GLenum handleType, const void *name );
typedef GLsync     (GLAPIENTRY *PFN_glImportSyncEXT)( GLenum external_sync_type, GLintptr external_sync, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glIndexFormatNV)( GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glIndexFuncEXT)( GLenum func, GLclampf ref );
typedef void       (GLAPIENTRY *PFN_glIndexMaterialEXT)( GLenum face, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glIndexPointerEXT)( GLenum type, GLsizei stride, GLsizei count, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glIndexPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glIndexxOES)( GLfixed component );
typedef void       (GLAPIENTRY *PFN_glIndexxvOES)( const GLfixed *component );
typedef void       (GLAPIENTRY *PFN_glInsertComponentEXT)( GLuint res, GLuint src, GLuint num );
typedef void       (GLAPIENTRY *PFN_glInsertEventMarkerEXT)( GLsizei length, const GLchar *marker );
typedef void       (GLAPIENTRY *PFN_glInstrumentsBufferSGIX)( GLsizei size, GLint *buffer );
typedef void       (GLAPIENTRY *PFN_glInterpolatePathsNV)( GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight );
typedef void       (GLAPIENTRY *PFN_glInvalidateBufferData)( GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glInvalidateBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr length );
typedef void       (GLAPIENTRY *PFN_glInvalidateFramebuffer)( GLenum target, GLsizei numAttachments, const GLenum *attachments );
typedef void       (GLAPIENTRY *PFN_glInvalidateNamedFramebufferData)( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments );
typedef void       (GLAPIENTRY *PFN_glInvalidateNamedFramebufferSubData)( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glInvalidateSubFramebuffer)( GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glInvalidateTexImage)( GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glInvalidateTexSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth );
typedef GLboolean  (GLAPIENTRY *PFN_glIsAsyncMarkerSGIX)( GLuint marker );
typedef GLboolean  (GLAPIENTRY *PFN_glIsBuffer)( GLuint buffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsBufferARB)( GLuint buffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsBufferResidentNV)( GLenum target );
typedef GLboolean  (GLAPIENTRY *PFN_glIsCommandListNV)( GLuint list );
typedef GLboolean  (GLAPIENTRY *PFN_glIsEnabledIndexedEXT)( GLenum target, GLuint index );
typedef GLboolean  (GLAPIENTRY *PFN_glIsEnabledi)( GLenum target, GLuint index );
typedef GLboolean  (GLAPIENTRY *PFN_glIsFenceAPPLE)( GLuint fence );
typedef GLboolean  (GLAPIENTRY *PFN_glIsFenceNV)( GLuint fence );
typedef GLboolean  (GLAPIENTRY *PFN_glIsFramebuffer)( GLuint framebuffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsFramebufferEXT)( GLuint framebuffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsImageHandleResidentARB)( GLuint64 handle );
typedef GLboolean  (GLAPIENTRY *PFN_glIsImageHandleResidentNV)( GLuint64 handle );
typedef GLboolean  (GLAPIENTRY *PFN_glIsMemoryObjectEXT)( GLuint memoryObject );
typedef GLboolean  (GLAPIENTRY *PFN_glIsNameAMD)( GLenum identifier, GLuint name );
typedef GLboolean  (GLAPIENTRY *PFN_glIsNamedBufferResidentNV)( GLuint buffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsNamedStringARB)( GLint namelen, const GLchar *name );
typedef GLboolean  (GLAPIENTRY *PFN_glIsObjectBufferATI)( GLuint buffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsOcclusionQueryNV)( GLuint id );
typedef GLboolean  (GLAPIENTRY *PFN_glIsPathNV)( GLuint path );
typedef GLboolean  (GLAPIENTRY *PFN_glIsPointInFillPathNV)( GLuint path, GLuint mask, GLfloat x, GLfloat y );
typedef GLboolean  (GLAPIENTRY *PFN_glIsPointInStrokePathNV)( GLuint path, GLfloat x, GLfloat y );
typedef GLboolean  (GLAPIENTRY *PFN_glIsProgram)( GLuint program );
typedef GLboolean  (GLAPIENTRY *PFN_glIsProgramARB)( GLuint program );
typedef GLboolean  (GLAPIENTRY *PFN_glIsProgramNV)( GLuint id );
typedef GLboolean  (GLAPIENTRY *PFN_glIsProgramPipeline)( GLuint pipeline );
typedef GLboolean  (GLAPIENTRY *PFN_glIsQuery)( GLuint id );
typedef GLboolean  (GLAPIENTRY *PFN_glIsQueryARB)( GLuint id );
typedef GLboolean  (GLAPIENTRY *PFN_glIsRenderbuffer)( GLuint renderbuffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsRenderbufferEXT)( GLuint renderbuffer );
typedef GLboolean  (GLAPIENTRY *PFN_glIsSampler)( GLuint sampler );
typedef GLboolean  (GLAPIENTRY *PFN_glIsSemaphoreEXT)( GLuint semaphore );
typedef GLboolean  (GLAPIENTRY *PFN_glIsShader)( GLuint shader );
typedef GLboolean  (GLAPIENTRY *PFN_glIsStateNV)( GLuint state );
typedef GLboolean  (GLAPIENTRY *PFN_glIsSync)( GLsync sync );
typedef GLboolean  (GLAPIENTRY *PFN_glIsTextureEXT)( GLuint texture );
typedef GLboolean  (GLAPIENTRY *PFN_glIsTextureHandleResidentARB)( GLuint64 handle );
typedef GLboolean  (GLAPIENTRY *PFN_glIsTextureHandleResidentNV)( GLuint64 handle );
typedef GLboolean  (GLAPIENTRY *PFN_glIsTransformFeedback)( GLuint id );
typedef GLboolean  (GLAPIENTRY *PFN_glIsTransformFeedbackNV)( GLuint id );
typedef GLboolean  (GLAPIENTRY *PFN_glIsVariantEnabledEXT)( GLuint id, GLenum cap );
typedef GLboolean  (GLAPIENTRY *PFN_glIsVertexArray)( GLuint array );
typedef GLboolean  (GLAPIENTRY *PFN_glIsVertexArrayAPPLE)( GLuint array );
typedef GLboolean  (GLAPIENTRY *PFN_glIsVertexAttribEnabledAPPLE)( GLuint index, GLenum pname );
typedef void       (GLAPIENTRY *PFN_glLGPUCopyImageSubDataNVX)( GLuint sourceGpu, GLbitfield destinationGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srxY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth );
typedef void       (GLAPIENTRY *PFN_glLGPUInterlockNVX)(void);
typedef void       (GLAPIENTRY *PFN_glLGPUNamedBufferSubDataNVX)( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
typedef void       (GLAPIENTRY *PFN_glLabelObjectEXT)( GLenum type, GLuint object, GLsizei length, const GLchar *label );
typedef void       (GLAPIENTRY *PFN_glLightEnviSGIX)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glLightModelxOES)( GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glLightModelxvOES)( GLenum pname, const GLfixed *param );
typedef void       (GLAPIENTRY *PFN_glLightxOES)( GLenum light, GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glLightxvOES)( GLenum light, GLenum pname, const GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glLineWidthxOES)( GLfixed width );
typedef void       (GLAPIENTRY *PFN_glLinkProgram)( GLuint program );
typedef void       (GLAPIENTRY *PFN_glLinkProgramARB)( GLhandleARB programObj );
typedef void       (GLAPIENTRY *PFN_glListDrawCommandsStatesClientNV)( GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
typedef void       (GLAPIENTRY *PFN_glListParameterfSGIX)( GLuint list, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glListParameterfvSGIX)( GLuint list, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glListParameteriSGIX)( GLuint list, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glListParameterivSGIX)( GLuint list, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glLoadIdentityDeformationMapSGIX)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glLoadMatrixxOES)( const GLfixed *m );
typedef void       (GLAPIENTRY *PFN_glLoadProgramNV)( GLenum target, GLuint id, GLsizei len, const GLubyte *program );
typedef void       (GLAPIENTRY *PFN_glLoadTransposeMatrixd)( const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glLoadTransposeMatrixdARB)( const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glLoadTransposeMatrixf)( const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glLoadTransposeMatrixfARB)( const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glLoadTransposeMatrixxOES)( const GLfixed *m );
typedef void       (GLAPIENTRY *PFN_glLockArraysEXT)( GLint first, GLsizei count );
typedef void       (GLAPIENTRY *PFN_glMTexCoord2fSGIS)( GLenum target, GLfloat s, GLfloat t );
typedef void       (GLAPIENTRY *PFN_glMTexCoord2fvSGIS)( GLenum target, GLfloat * v );
typedef void       (GLAPIENTRY *PFN_glMakeBufferNonResidentNV)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glMakeBufferResidentNV)( GLenum target, GLenum access );
typedef void       (GLAPIENTRY *PFN_glMakeImageHandleNonResidentARB)( GLuint64 handle );
typedef void       (GLAPIENTRY *PFN_glMakeImageHandleNonResidentNV)( GLuint64 handle );
typedef void       (GLAPIENTRY *PFN_glMakeImageHandleResidentARB)( GLuint64 handle, GLenum access );
typedef void       (GLAPIENTRY *PFN_glMakeImageHandleResidentNV)( GLuint64 handle, GLenum access );
typedef void       (GLAPIENTRY *PFN_glMakeNamedBufferNonResidentNV)( GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glMakeNamedBufferResidentNV)( GLuint buffer, GLenum access );
typedef void       (GLAPIENTRY *PFN_glMakeTextureHandleNonResidentARB)( GLuint64 handle );
typedef void       (GLAPIENTRY *PFN_glMakeTextureHandleNonResidentNV)( GLuint64 handle );
typedef void       (GLAPIENTRY *PFN_glMakeTextureHandleResidentARB)( GLuint64 handle );
typedef void       (GLAPIENTRY *PFN_glMakeTextureHandleResidentNV)( GLuint64 handle );
typedef void       (GLAPIENTRY *PFN_glMap1xOES)( GLenum target, GLfixed u1, GLfixed u2, GLint stride, GLint order, GLfixed points );
typedef void       (GLAPIENTRY *PFN_glMap2xOES)( GLenum target, GLfixed u1, GLfixed u2, GLint ustride, GLint uorder, GLfixed v1, GLfixed v2, GLint vstride, GLint vorder, GLfixed points );
typedef void *     (GLAPIENTRY *PFN_glMapBuffer)( GLenum target, GLenum access );
typedef void *     (GLAPIENTRY *PFN_glMapBufferARB)( GLenum target, GLenum access );
typedef void *     (GLAPIENTRY *PFN_glMapBufferRange)( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access );
typedef void       (GLAPIENTRY *PFN_glMapControlPointsNV)( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const void *points );
typedef void       (GLAPIENTRY *PFN_glMapGrid1xOES)( GLint n, GLfixed u1, GLfixed u2 );
typedef void       (GLAPIENTRY *PFN_glMapGrid2xOES)( GLint n, GLfixed u1, GLfixed u2, GLfixed v1, GLfixed v2 );
typedef void *     (GLAPIENTRY *PFN_glMapNamedBuffer)( GLuint buffer, GLenum access );
typedef void *     (GLAPIENTRY *PFN_glMapNamedBufferEXT)( GLuint buffer, GLenum access );
typedef void *     (GLAPIENTRY *PFN_glMapNamedBufferRange)( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access );
typedef void *     (GLAPIENTRY *PFN_glMapNamedBufferRangeEXT)( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access );
typedef void *     (GLAPIENTRY *PFN_glMapObjectBufferATI)( GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glMapParameterfvNV)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glMapParameterivNV)( GLenum target, GLenum pname, const GLint *params );
typedef void *     (GLAPIENTRY *PFN_glMapTexture2DINTEL)( GLuint texture, GLint level, GLbitfield access, GLint *stride, GLenum *layout );
typedef void       (GLAPIENTRY *PFN_glMapVertexAttrib1dAPPLE)( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
typedef void       (GLAPIENTRY *PFN_glMapVertexAttrib1fAPPLE)( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glMapVertexAttrib2dAPPLE)( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
typedef void       (GLAPIENTRY *PFN_glMapVertexAttrib2fAPPLE)( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glMaterialxOES)( GLenum face, GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glMaterialxvOES)( GLenum face, GLenum pname, const GLfixed *param );
typedef void       (GLAPIENTRY *PFN_glMatrixFrustumEXT)( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
typedef void       (GLAPIENTRY *PFN_glMatrixIndexPointerARB)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glMatrixIndexubvARB)( GLint size, const GLubyte *indices );
typedef void       (GLAPIENTRY *PFN_glMatrixIndexuivARB)( GLint size, const GLuint *indices );
typedef void       (GLAPIENTRY *PFN_glMatrixIndexusvARB)( GLint size, const GLushort *indices );
typedef void       (GLAPIENTRY *PFN_glMatrixLoad3x2fNV)( GLenum matrixMode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixLoad3x3fNV)( GLenum matrixMode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixLoadIdentityEXT)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glMatrixLoadTranspose3x3fNV)( GLenum matrixMode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixLoadTransposedEXT)( GLenum mode, const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glMatrixLoadTransposefEXT)( GLenum mode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixLoaddEXT)( GLenum mode, const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glMatrixLoadfEXT)( GLenum mode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixMult3x2fNV)( GLenum matrixMode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixMult3x3fNV)( GLenum matrixMode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixMultTranspose3x3fNV)( GLenum matrixMode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixMultTransposedEXT)( GLenum mode, const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glMatrixMultTransposefEXT)( GLenum mode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixMultdEXT)( GLenum mode, const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glMatrixMultfEXT)( GLenum mode, const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMatrixOrthoEXT)( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
typedef void       (GLAPIENTRY *PFN_glMatrixPopEXT)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glMatrixPushEXT)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glMatrixRotatedEXT)( GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glMatrixRotatefEXT)( GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glMatrixScaledEXT)( GLenum mode, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glMatrixScalefEXT)( GLenum mode, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glMatrixTranslatedEXT)( GLenum mode, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glMatrixTranslatefEXT)( GLenum mode, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glMaxShaderCompilerThreadsARB)( GLuint count );
typedef void       (GLAPIENTRY *PFN_glMaxShaderCompilerThreadsKHR)( GLuint count );
typedef void       (GLAPIENTRY *PFN_glMemoryBarrier)( GLbitfield barriers );
typedef void       (GLAPIENTRY *PFN_glMemoryBarrierByRegion)( GLbitfield barriers );
typedef void       (GLAPIENTRY *PFN_glMemoryBarrierEXT)( GLbitfield barriers );
typedef void       (GLAPIENTRY *PFN_glMemoryObjectParameterivEXT)( GLuint memoryObject, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glMinSampleShading)( GLfloat value );
typedef void       (GLAPIENTRY *PFN_glMinSampleShadingARB)( GLfloat value );
typedef void       (GLAPIENTRY *PFN_glMinmax)( GLenum target, GLenum internalformat, GLboolean sink );
typedef void       (GLAPIENTRY *PFN_glMinmaxEXT)( GLenum target, GLenum internalformat, GLboolean sink );
typedef void       (GLAPIENTRY *PFN_glMultMatrixxOES)( const GLfixed *m );
typedef void       (GLAPIENTRY *PFN_glMultTransposeMatrixd)( const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glMultTransposeMatrixdARB)( const GLdouble *m );
typedef void       (GLAPIENTRY *PFN_glMultTransposeMatrixf)( const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMultTransposeMatrixfARB)( const GLfloat *m );
typedef void       (GLAPIENTRY *PFN_glMultTransposeMatrixxOES)( const GLfixed *m );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArrays)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArraysEXT)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArraysIndirect)( GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArraysIndirectAMD)( GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArraysIndirectBindlessCountNV)( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArraysIndirectBindlessNV)( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArraysIndirectCount)( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawArraysIndirectCountARB)( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementArrayAPPLE)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElements)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsBaseVertex)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsEXT)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsIndirect)( GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsIndirectAMD)( GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsIndirectBindlessCountNV)( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsIndirectBindlessNV)( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsIndirectCount)( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawElementsIndirectCountARB)( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawMeshTasksIndirectCountNV)( GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawMeshTasksIndirectNV)( GLintptr indirect, GLsizei drawcount, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glMultiDrawRangeElementArrayAPPLE)( GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount );
typedef void       (GLAPIENTRY *PFN_glMultiModeDrawArraysIBM)( const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride );
typedef void       (GLAPIENTRY *PFN_glMultiModeDrawElementsIBM)( const GLenum *mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, GLint modestride );
typedef void       (GLAPIENTRY *PFN_glMultiTexBufferEXT)( GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1bOES)( GLenum texture, GLbyte s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1bvOES)( GLenum texture, const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1d)( GLenum target, GLdouble s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1dARB)( GLenum target, GLdouble s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1dSGIS)( GLenum target, GLdouble s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1dv)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1dvARB)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1dvSGIS)( GLenum target, GLdouble * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1f)( GLenum target, GLfloat s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1fARB)( GLenum target, GLfloat s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1fSGIS)( GLenum target, GLfloat s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1fv)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1fvARB)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1fvSGIS)( GLenum target, const GLfloat * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1hNV)( GLenum target, GLhalfNV s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1hvNV)( GLenum target, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1i)( GLenum target, GLint s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1iARB)( GLenum target, GLint s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1iSGIS)( GLenum target, GLint s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1iv)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1ivARB)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1ivSGIS)( GLenum target, GLint * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1s)( GLenum target, GLshort s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1sARB)( GLenum target, GLshort s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1sSGIS)( GLenum target, GLshort s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1sv)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1svARB)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1svSGIS)( GLenum target, GLshort * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1xOES)( GLenum texture, GLfixed s );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord1xvOES)( GLenum texture, const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2bOES)( GLenum texture, GLbyte s, GLbyte t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2bvOES)( GLenum texture, const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2d)( GLenum target, GLdouble s, GLdouble t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2dARB)( GLenum target, GLdouble s, GLdouble t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2dSGIS)( GLenum target, GLdouble s, GLdouble t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2dv)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2dvARB)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2dvSGIS)( GLenum target, GLdouble * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2f)( GLenum target, GLfloat s, GLfloat t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2fARB)( GLenum target, GLfloat s, GLfloat t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2fSGIS)( GLenum target, GLfloat s, GLfloat t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2fv)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2fvARB)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2fvSGIS)( GLenum target, GLfloat * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2hNV)( GLenum target, GLhalfNV s, GLhalfNV t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2hvNV)( GLenum target, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2i)( GLenum target, GLint s, GLint t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2iARB)( GLenum target, GLint s, GLint t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2iSGIS)( GLenum target, GLint s, GLint t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2iv)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2ivARB)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2ivSGIS)( GLenum target, GLint * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2s)( GLenum target, GLshort s, GLshort t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2sARB)( GLenum target, GLshort s, GLshort t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2sSGIS)( GLenum target, GLshort s, GLshort t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2sv)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2svARB)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2svSGIS)( GLenum target, GLshort * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2xOES)( GLenum texture, GLfixed s, GLfixed t );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord2xvOES)( GLenum texture, const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3bOES)( GLenum texture, GLbyte s, GLbyte t, GLbyte r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3bvOES)( GLenum texture, const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3d)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3dARB)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3dSGIS)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3dv)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3dvARB)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3dvSGIS)( GLenum target, GLdouble * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3f)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3fARB)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3fSGIS)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3fv)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3fvARB)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3fvSGIS)( GLenum target, GLfloat * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3hNV)( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3hvNV)( GLenum target, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3i)( GLenum target, GLint s, GLint t, GLint r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3iARB)( GLenum target, GLint s, GLint t, GLint r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3iSGIS)( GLenum target, GLint s, GLint t, GLint r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3iv)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3ivARB)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3ivSGIS)( GLenum target, GLint * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3s)( GLenum target, GLshort s, GLshort t, GLshort r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3sARB)( GLenum target, GLshort s, GLshort t, GLshort r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3sSGIS)( GLenum target, GLshort s, GLshort t, GLshort r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3sv)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3svARB)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3svSGIS)( GLenum target, GLshort * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3xOES)( GLenum texture, GLfixed s, GLfixed t, GLfixed r );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord3xvOES)( GLenum texture, const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4bOES)( GLenum texture, GLbyte s, GLbyte t, GLbyte r, GLbyte q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4bvOES)( GLenum texture, const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4d)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4dARB)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4dSGIS)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4dv)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4dvARB)( GLenum target, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4dvSGIS)( GLenum target, GLdouble * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4f)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4fARB)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4fSGIS)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4fv)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4fvARB)( GLenum target, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4fvSGIS)( GLenum target, GLfloat * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4hNV)( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4hvNV)( GLenum target, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4i)( GLenum target, GLint s, GLint t, GLint r, GLint q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4iARB)( GLenum target, GLint s, GLint t, GLint r, GLint q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4iSGIS)( GLenum target, GLint s, GLint t, GLint r, GLint q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4iv)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4ivARB)( GLenum target, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4ivSGIS)( GLenum target, GLint * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4s)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4sARB)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4sSGIS)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4sv)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4svARB)( GLenum target, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4svSGIS)( GLenum target, GLshort * v );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4xOES)( GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoord4xvOES)( GLenum texture, const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP1ui)( GLenum texture, GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP1uiv)( GLenum texture, GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP2ui)( GLenum texture, GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP2uiv)( GLenum texture, GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP3ui)( GLenum texture, GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP3uiv)( GLenum texture, GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP4ui)( GLenum texture, GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordP4uiv)( GLenum texture, GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordPointerEXT)( GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glMultiTexCoordPointerSGIS)( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer );
typedef void       (GLAPIENTRY *PFN_glMultiTexEnvfEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glMultiTexEnvfvEXT)( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexEnviEXT)( GLenum texunit, GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glMultiTexEnvivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexGendEXT)( GLenum texunit, GLenum coord, GLenum pname, GLdouble param );
typedef void       (GLAPIENTRY *PFN_glMultiTexGendvEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexGenfEXT)( GLenum texunit, GLenum coord, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glMultiTexGenfvEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexGeniEXT)( GLenum texunit, GLenum coord, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glMultiTexGenivEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glMultiTexImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glMultiTexParameterIivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexParameterIuivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexParameterfEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glMultiTexParameterfvEXT)( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexParameteriEXT)( GLenum texunit, GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glMultiTexParameterivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glMultiTexRenderbufferEXT)( GLenum texunit, GLenum target, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glMulticastBarrierNV)(void);
typedef void       (GLAPIENTRY *PFN_glMulticastBlitFramebufferNV)( GLuint srcGpu, GLuint dstGpu, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
typedef void       (GLAPIENTRY *PFN_glMulticastBufferSubDataNV)( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
typedef void       (GLAPIENTRY *PFN_glMulticastCopyBufferSubDataNV)( GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glMulticastCopyImageSubDataNV)( GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth );
typedef void       (GLAPIENTRY *PFN_glMulticastFramebufferSampleLocationsfvNV)( GLuint gpu, GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMulticastGetQueryObjecti64vNV)( GLuint gpu, GLuint id, GLenum pname, GLint64 *params );
typedef void       (GLAPIENTRY *PFN_glMulticastGetQueryObjectivNV)( GLuint gpu, GLuint id, GLenum pname, GLint *params );
typedef void       (GLAPIENTRY *PFN_glMulticastGetQueryObjectui64vNV)( GLuint gpu, GLuint id, GLenum pname, GLuint64 *params );
typedef void       (GLAPIENTRY *PFN_glMulticastGetQueryObjectuivNV)( GLuint gpu, GLuint id, GLenum pname, GLuint *params );
typedef void       (GLAPIENTRY *PFN_glMulticastScissorArrayvNVX)( GLuint gpu, GLuint first, GLsizei count, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glMulticastViewportArrayvNVX)( GLuint gpu, GLuint first, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glMulticastViewportPositionWScaleNVX)( GLuint gpu, GLuint index, GLfloat xcoeff, GLfloat ycoeff );
typedef void       (GLAPIENTRY *PFN_glMulticastWaitSyncNV)( GLuint signalGpu, GLbitfield waitGpuMask );
typedef void       (GLAPIENTRY *PFN_glNamedBufferAttachMemoryNV)( GLuint buffer, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glNamedBufferData)( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage );
typedef void       (GLAPIENTRY *PFN_glNamedBufferDataEXT)( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage );
typedef void       (GLAPIENTRY *PFN_glNamedBufferPageCommitmentARB)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit );
typedef void       (GLAPIENTRY *PFN_glNamedBufferPageCommitmentEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit );
typedef void       (GLAPIENTRY *PFN_glNamedBufferStorage)( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glNamedBufferStorageEXT)( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glNamedBufferStorageExternalEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glNamedBufferStorageMemEXT)( GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glNamedBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
typedef void       (GLAPIENTRY *PFN_glNamedBufferSubDataEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
typedef void       (GLAPIENTRY *PFN_glNamedCopyBufferSubDataEXT)( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferDrawBuffer)( GLuint framebuffer, GLenum buf );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferDrawBuffers)( GLuint framebuffer, GLsizei n, const GLenum *bufs );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferParameteri)( GLuint framebuffer, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferParameteriEXT)( GLuint framebuffer, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferReadBuffer)( GLuint framebuffer, GLenum src );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferRenderbuffer)( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferRenderbufferEXT)( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferSampleLocationsfvARB)( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferSampleLocationsfvNV)( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferSamplePositionsfvAMD)( GLuint framebuffer, GLuint numsamples, GLuint pixelindex, const GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTexture)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTexture1DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTexture2DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTexture3DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTextureEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTextureFaceEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTextureLayer)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer );
typedef void       (GLAPIENTRY *PFN_glNamedFramebufferTextureLayerEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameter4dEXT)( GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameter4dvEXT)( GLuint program, GLenum target, GLuint index, const GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameter4fEXT)( GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameter4fvEXT)( GLuint program, GLenum target, GLuint index, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameterI4iEXT)( GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameterI4ivEXT)( GLuint program, GLenum target, GLuint index, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameterI4uiEXT)( GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameterI4uivEXT)( GLuint program, GLenum target, GLuint index, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParameters4fvEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParametersI4ivEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glNamedProgramLocalParametersI4uivEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glNamedProgramStringEXT)( GLuint program, GLenum target, GLenum format, GLsizei len, const void *string );
typedef void       (GLAPIENTRY *PFN_glNamedRenderbufferStorage)( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glNamedRenderbufferStorageEXT)( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glNamedRenderbufferStorageMultisample)( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glNamedRenderbufferStorageMultisampleAdvancedAMD)( GLuint renderbuffer, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glNamedRenderbufferStorageMultisampleCoverageEXT)( GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glNamedRenderbufferStorageMultisampleEXT)( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glNamedStringARB)( GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string );
typedef GLuint     (GLAPIENTRY *PFN_glNewBufferRegion)( GLenum type );
typedef GLuint     (GLAPIENTRY *PFN_glNewObjectBufferATI)( GLsizei size, const void *pointer, GLenum usage );
typedef void       (GLAPIENTRY *PFN_glNormal3fVertex3fSUN)( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glNormal3fVertex3fvSUN)( const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glNormal3hNV)( GLhalfNV nx, GLhalfNV ny, GLhalfNV nz );
typedef void       (GLAPIENTRY *PFN_glNormal3hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glNormal3xOES)( GLfixed nx, GLfixed ny, GLfixed nz );
typedef void       (GLAPIENTRY *PFN_glNormal3xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glNormalFormatNV)( GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glNormalP3ui)( GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glNormalP3uiv)( GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glNormalPointerEXT)( GLenum type, GLsizei stride, GLsizei count, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glNormalPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glNormalPointervINTEL)( GLenum type, const void **pointer );
typedef void       (GLAPIENTRY *PFN_glNormalStream3bATI)( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz );
typedef void       (GLAPIENTRY *PFN_glNormalStream3bvATI)( GLenum stream, const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glNormalStream3dATI)( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz );
typedef void       (GLAPIENTRY *PFN_glNormalStream3dvATI)( GLenum stream, const GLdouble *coords );
typedef void       (GLAPIENTRY *PFN_glNormalStream3fATI)( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz );
typedef void       (GLAPIENTRY *PFN_glNormalStream3fvATI)( GLenum stream, const GLfloat *coords );
typedef void       (GLAPIENTRY *PFN_glNormalStream3iATI)( GLenum stream, GLint nx, GLint ny, GLint nz );
typedef void       (GLAPIENTRY *PFN_glNormalStream3ivATI)( GLenum stream, const GLint *coords );
typedef void       (GLAPIENTRY *PFN_glNormalStream3sATI)( GLenum stream, GLshort nx, GLshort ny, GLshort nz );
typedef void       (GLAPIENTRY *PFN_glNormalStream3svATI)( GLenum stream, const GLshort *coords );
typedef void       (GLAPIENTRY *PFN_glObjectLabel)( GLenum identifier, GLuint name, GLsizei length, const GLchar *label );
typedef void       (GLAPIENTRY *PFN_glObjectPtrLabel)( const void *ptr, GLsizei length, const GLchar *label );
typedef GLenum     (GLAPIENTRY *PFN_glObjectPurgeableAPPLE)( GLenum objectType, GLuint name, GLenum option );
typedef GLenum     (GLAPIENTRY *PFN_glObjectUnpurgeableAPPLE)( GLenum objectType, GLuint name, GLenum option );
typedef void       (GLAPIENTRY *PFN_glOrthofOES)( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f );
typedef void       (GLAPIENTRY *PFN_glOrthoxOES)( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f );
typedef void       (GLAPIENTRY *PFN_glPNTrianglesfATI)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPNTrianglesiATI)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glPassTexCoordATI)( GLuint dst, GLuint coord, GLenum swizzle );
typedef void       (GLAPIENTRY *PFN_glPassThroughxOES)( GLfixed token );
typedef void       (GLAPIENTRY *PFN_glPatchParameterfv)( GLenum pname, const GLfloat *values );
typedef void       (GLAPIENTRY *PFN_glPatchParameteri)( GLenum pname, GLint value );
typedef void       (GLAPIENTRY *PFN_glPathColorGenNV)( GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs );
typedef void       (GLAPIENTRY *PFN_glPathCommandsNV)( GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords );
typedef void       (GLAPIENTRY *PFN_glPathCoordsNV)( GLuint path, GLsizei numCoords, GLenum coordType, const void *coords );
typedef void       (GLAPIENTRY *PFN_glPathCoverDepthFuncNV)( GLenum func );
typedef void       (GLAPIENTRY *PFN_glPathDashArrayNV)( GLuint path, GLsizei dashCount, const GLfloat *dashArray );
typedef void       (GLAPIENTRY *PFN_glPathFogGenNV)( GLenum genMode );
typedef GLenum     (GLAPIENTRY *PFN_glPathGlyphIndexArrayNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
typedef GLenum     (GLAPIENTRY *PFN_glPathGlyphIndexRangeNV)( GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2] );
typedef void       (GLAPIENTRY *PFN_glPathGlyphRangeNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
typedef void       (GLAPIENTRY *PFN_glPathGlyphsNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
typedef GLenum     (GLAPIENTRY *PFN_glPathMemoryGlyphIndexArrayNV)( GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
typedef void       (GLAPIENTRY *PFN_glPathParameterfNV)( GLuint path, GLenum pname, GLfloat value );
typedef void       (GLAPIENTRY *PFN_glPathParameterfvNV)( GLuint path, GLenum pname, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glPathParameteriNV)( GLuint path, GLenum pname, GLint value );
typedef void       (GLAPIENTRY *PFN_glPathParameterivNV)( GLuint path, GLenum pname, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glPathStencilDepthOffsetNV)( GLfloat factor, GLfloat units );
typedef void       (GLAPIENTRY *PFN_glPathStencilFuncNV)( GLenum func, GLint ref, GLuint mask );
typedef void       (GLAPIENTRY *PFN_glPathStringNV)( GLuint path, GLenum format, GLsizei length, const void *pathString );
typedef void       (GLAPIENTRY *PFN_glPathSubCommandsNV)( GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords );
typedef void       (GLAPIENTRY *PFN_glPathSubCoordsNV)( GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords );
typedef void       (GLAPIENTRY *PFN_glPathTexGenNV)( GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs );
typedef void       (GLAPIENTRY *PFN_glPauseTransformFeedback)(void);
typedef void       (GLAPIENTRY *PFN_glPauseTransformFeedbackNV)(void);
typedef void       (GLAPIENTRY *PFN_glPixelDataRangeNV)( GLenum target, GLsizei length, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glPixelMapx)( GLenum map, GLint size, const GLfixed *values );
typedef void       (GLAPIENTRY *PFN_glPixelStorex)( GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glPixelTexGenParameterfSGIS)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPixelTexGenParameterfvSGIS)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glPixelTexGenParameteriSGIS)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glPixelTexGenParameterivSGIS)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glPixelTexGenSGIX)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glPixelTransferxOES)( GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glPixelTransformParameterfEXT)( GLenum target, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPixelTransformParameterfvEXT)( GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glPixelTransformParameteriEXT)( GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glPixelTransformParameterivEXT)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glPixelZoomxOES)( GLfixed xfactor, GLfixed yfactor );
typedef GLboolean  (GLAPIENTRY *PFN_glPointAlongPathNV)( GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY );
typedef void       (GLAPIENTRY *PFN_glPointParameterf)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPointParameterfARB)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPointParameterfEXT)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPointParameterfSGIS)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glPointParameterfv)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glPointParameterfvARB)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glPointParameterfvEXT)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glPointParameterfvSGIS)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glPointParameteri)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glPointParameteriNV)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glPointParameteriv)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glPointParameterivNV)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glPointParameterxvOES)( GLenum pname, const GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glPointSizexOES)( GLfixed size );
typedef GLint      (GLAPIENTRY *PFN_glPollAsyncSGIX)( GLuint *markerp );
typedef GLint      (GLAPIENTRY *PFN_glPollInstrumentsSGIX)( GLint *marker_p );
typedef void       (GLAPIENTRY *PFN_glPolygonOffsetClamp)( GLfloat factor, GLfloat units, GLfloat clamp );
typedef void       (GLAPIENTRY *PFN_glPolygonOffsetClampEXT)( GLfloat factor, GLfloat units, GLfloat clamp );
typedef void       (GLAPIENTRY *PFN_glPolygonOffsetEXT)( GLfloat factor, GLfloat bias );
typedef void       (GLAPIENTRY *PFN_glPolygonOffsetxOES)( GLfixed factor, GLfixed units );
typedef void       (GLAPIENTRY *PFN_glPopDebugGroup)(void);
typedef void       (GLAPIENTRY *PFN_glPopGroupMarkerEXT)(void);
typedef void       (GLAPIENTRY *PFN_glPresentFrameDualFillNV)( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3 );
typedef void       (GLAPIENTRY *PFN_glPresentFrameKeyedNV)( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1 );
typedef void       (GLAPIENTRY *PFN_glPrimitiveBoundingBoxARB)( GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW );
typedef void       (GLAPIENTRY *PFN_glPrimitiveRestartIndex)( GLuint index );
typedef void       (GLAPIENTRY *PFN_glPrimitiveRestartIndexNV)( GLuint index );
typedef void       (GLAPIENTRY *PFN_glPrimitiveRestartNV)(void);
typedef void       (GLAPIENTRY *PFN_glPrioritizeTexturesEXT)( GLsizei n, const GLuint *textures, const GLclampf *priorities );
typedef void       (GLAPIENTRY *PFN_glPrioritizeTexturesxOES)( GLsizei n, const GLuint *textures, const GLfixed *priorities );
typedef void       (GLAPIENTRY *PFN_glProgramBinary)( GLuint program, GLenum binaryFormat, const void *binary, GLsizei length );
typedef void       (GLAPIENTRY *PFN_glProgramBufferParametersIivNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glProgramBufferParametersIuivNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glProgramBufferParametersfvNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameter4dARB)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameter4dvARB)( GLenum target, GLuint index, const GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameter4fARB)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameter4fvARB)( GLenum target, GLuint index, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameterI4iNV)( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameterI4ivNV)( GLenum target, GLuint index, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameterI4uiNV)( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameterI4uivNV)( GLenum target, GLuint index, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParameters4fvEXT)( GLenum target, GLuint index, GLsizei count, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParametersI4ivNV)( GLenum target, GLuint index, GLsizei count, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glProgramEnvParametersI4uivNV)( GLenum target, GLuint index, GLsizei count, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameter4dARB)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameter4dvARB)( GLenum target, GLuint index, const GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameter4fARB)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameter4fvARB)( GLenum target, GLuint index, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameterI4iNV)( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameterI4ivNV)( GLenum target, GLuint index, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameterI4uiNV)( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameterI4uivNV)( GLenum target, GLuint index, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParameters4fvEXT)( GLenum target, GLuint index, GLsizei count, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParametersI4ivNV)( GLenum target, GLuint index, GLsizei count, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glProgramLocalParametersI4uivNV)( GLenum target, GLuint index, GLsizei count, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glProgramNamedParameter4dNV)( GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glProgramNamedParameter4dvNV)( GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glProgramNamedParameter4fNV)( GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glProgramNamedParameter4fvNV)( GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glProgramParameter4dNV)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glProgramParameter4dvNV)( GLenum target, GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glProgramParameter4fNV)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glProgramParameter4fvNV)( GLenum target, GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glProgramParameteri)( GLuint program, GLenum pname, GLint value );
typedef void       (GLAPIENTRY *PFN_glProgramParameteriARB)( GLuint program, GLenum pname, GLint value );
typedef void       (GLAPIENTRY *PFN_glProgramParameteriEXT)( GLuint program, GLenum pname, GLint value );
typedef void       (GLAPIENTRY *PFN_glProgramParameters4dvNV)( GLenum target, GLuint index, GLsizei count, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glProgramParameters4fvNV)( GLenum target, GLuint index, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glProgramPathFragmentInputGenNV)( GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs );
typedef void       (GLAPIENTRY *PFN_glProgramStringARB)( GLenum target, GLenum format, GLsizei len, const void *string );
typedef void       (GLAPIENTRY *PFN_glProgramSubroutineParametersuivNV)( GLenum target, GLsizei count, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1d)( GLuint program, GLint location, GLdouble v0 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1dEXT)( GLuint program, GLint location, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1f)( GLuint program, GLint location, GLfloat v0 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1fEXT)( GLuint program, GLint location, GLfloat v0 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1i)( GLuint program, GLint location, GLint v0 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1i64ARB)( GLuint program, GLint location, GLint64 x );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1i64NV)( GLuint program, GLint location, GLint64EXT x );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1iEXT)( GLuint program, GLint location, GLint v0 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1ui)( GLuint program, GLint location, GLuint v0 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1ui64ARB)( GLuint program, GLint location, GLuint64 x );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1ui64NV)( GLuint program, GLint location, GLuint64EXT x );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1uiEXT)( GLuint program, GLint location, GLuint v0 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform1uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2d)( GLuint program, GLint location, GLdouble v0, GLdouble v1 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2f)( GLuint program, GLint location, GLfloat v0, GLfloat v1 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2i)( GLuint program, GLint location, GLint v0, GLint v1 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2iEXT)( GLuint program, GLint location, GLint v0, GLint v1 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2ui)( GLuint program, GLint location, GLuint v0, GLuint v1 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform2uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3d)( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3f)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3i)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3iEXT)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3ui)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform3uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4d)( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4f)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4i)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4iEXT)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4ui)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniform4uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformHandleui64ARB)( GLuint program, GLint location, GLuint64 value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformHandleui64NV)( GLuint program, GLint location, GLuint64 value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformHandleui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *values );
typedef void       (GLAPIENTRY *PFN_glProgramUniformHandleui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64 *values );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix2x4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix3x4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformMatrix4x3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformui64NV)( GLuint program, GLint location, GLuint64EXT value );
typedef void       (GLAPIENTRY *PFN_glProgramUniformui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glProgramVertexLimitNV)( GLenum target, GLint limit );
typedef void       (GLAPIENTRY *PFN_glProvokingVertex)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glProvokingVertexEXT)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glPushClientAttribDefaultEXT)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glPushDebugGroup)( GLenum source, GLuint id, GLsizei length, const GLchar *message );
typedef void       (GLAPIENTRY *PFN_glPushGroupMarkerEXT)( GLsizei length, const GLchar *marker );
typedef void       (GLAPIENTRY *PFN_glQueryCounter)( GLuint id, GLenum target );
typedef GLbitfield (GLAPIENTRY *PFN_glQueryMatrixxOES)( GLfixed *mantissa, GLint *exponent );
typedef void       (GLAPIENTRY *PFN_glQueryObjectParameteruiAMD)( GLenum target, GLuint id, GLenum pname, GLuint param );
typedef GLint      (GLAPIENTRY *PFN_glQueryResourceNV)( GLenum queryType, GLint tagId, GLuint count, GLint *buffer );
typedef void       (GLAPIENTRY *PFN_glQueryResourceTagNV)( GLint tagId, const GLchar *tagString );
typedef void       (GLAPIENTRY *PFN_glRasterPos2xOES)( GLfixed x, GLfixed y );
typedef void       (GLAPIENTRY *PFN_glRasterPos2xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glRasterPos3xOES)( GLfixed x, GLfixed y, GLfixed z );
typedef void       (GLAPIENTRY *PFN_glRasterPos3xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glRasterPos4xOES)( GLfixed x, GLfixed y, GLfixed z, GLfixed w );
typedef void       (GLAPIENTRY *PFN_glRasterPos4xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glRasterSamplesEXT)( GLuint samples, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glReadBufferRegion)( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glReadInstrumentsSGIX)( GLint marker );
typedef void       (GLAPIENTRY *PFN_glReadnPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data );
typedef void       (GLAPIENTRY *PFN_glReadnPixelsARB)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data );
typedef void       (GLAPIENTRY *PFN_glRectxOES)( GLfixed x1, GLfixed y1, GLfixed x2, GLfixed y2 );
typedef void       (GLAPIENTRY *PFN_glRectxvOES)( const GLfixed *v1, const GLfixed *v2 );
typedef void       (GLAPIENTRY *PFN_glReferencePlaneSGIX)( const GLdouble *equation );
typedef GLboolean  (GLAPIENTRY *PFN_glReleaseKeyedMutexWin32EXT)( GLuint memory, GLuint64 key );
typedef void       (GLAPIENTRY *PFN_glReleaseShaderCompiler)(void);
typedef void       (GLAPIENTRY *PFN_glRenderGpuMaskNV)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glRenderbufferStorage)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glRenderbufferStorageEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glRenderbufferStorageMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glRenderbufferStorageMultisampleAdvancedAMD)( GLenum target, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glRenderbufferStorageMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glRenderbufferStorageMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glReplacementCodePointerSUN)( GLenum type, GLsizei stride, const void **pointer );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeubSUN)( GLubyte code );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeubvSUN)( const GLubyte *code );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiColor3fVertex3fSUN)( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiColor3fVertex3fvSUN)( const GLuint *rc, const GLfloat *c, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiColor4fNormal3fVertex3fSUN)( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiColor4fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiColor4ubVertex3fSUN)( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiColor4ubVertex3fvSUN)( const GLuint *rc, const GLubyte *c, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiNormal3fVertex3fSUN)( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiSUN)( GLuint code );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiTexCoord2fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiTexCoord2fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiVertex3fSUN)( GLuint rc, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuiVertex3fvSUN)( const GLuint *rc, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeuivSUN)( const GLuint *code );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeusSUN)( GLushort code );
typedef void       (GLAPIENTRY *PFN_glReplacementCodeusvSUN)( const GLushort *code );
typedef void       (GLAPIENTRY *PFN_glRequestResidentProgramsNV)( GLsizei n, const GLuint *programs );
typedef void       (GLAPIENTRY *PFN_glResetHistogram)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glResetHistogramEXT)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glResetMemoryObjectParameterNV)( GLuint memory, GLenum pname );
typedef void       (GLAPIENTRY *PFN_glResetMinmax)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glResetMinmaxEXT)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glResizeBuffersMESA)(void);
typedef void       (GLAPIENTRY *PFN_glResolveDepthValuesNV)(void);
typedef void       (GLAPIENTRY *PFN_glResumeTransformFeedback)(void);
typedef void       (GLAPIENTRY *PFN_glResumeTransformFeedbackNV)(void);
typedef void       (GLAPIENTRY *PFN_glRotatexOES)( GLfixed angle, GLfixed x, GLfixed y, GLfixed z );
typedef void       (GLAPIENTRY *PFN_glSampleCoverage)( GLfloat value, GLboolean invert );
typedef void       (GLAPIENTRY *PFN_glSampleCoverageARB)( GLfloat value, GLboolean invert );
typedef void       (GLAPIENTRY *PFN_glSampleMapATI)( GLuint dst, GLuint interp, GLenum swizzle );
typedef void       (GLAPIENTRY *PFN_glSampleMaskEXT)( GLclampf value, GLboolean invert );
typedef void       (GLAPIENTRY *PFN_glSampleMaskIndexedNV)( GLuint index, GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glSampleMaskSGIS)( GLclampf value, GLboolean invert );
typedef void       (GLAPIENTRY *PFN_glSampleMaski)( GLuint maskNumber, GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glSamplePatternEXT)( GLenum pattern );
typedef void       (GLAPIENTRY *PFN_glSamplePatternSGIS)( GLenum pattern );
typedef void       (GLAPIENTRY *PFN_glSamplerParameterIiv)( GLuint sampler, GLenum pname, const GLint *param );
typedef void       (GLAPIENTRY *PFN_glSamplerParameterIuiv)( GLuint sampler, GLenum pname, const GLuint *param );
typedef void       (GLAPIENTRY *PFN_glSamplerParameterf)( GLuint sampler, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glSamplerParameterfv)( GLuint sampler, GLenum pname, const GLfloat *param );
typedef void       (GLAPIENTRY *PFN_glSamplerParameteri)( GLuint sampler, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glSamplerParameteriv)( GLuint sampler, GLenum pname, const GLint *param );
typedef void       (GLAPIENTRY *PFN_glScalexOES)( GLfixed x, GLfixed y, GLfixed z );
typedef void       (GLAPIENTRY *PFN_glScissorArrayv)( GLuint first, GLsizei count, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glScissorExclusiveArrayvNV)( GLuint first, GLsizei count, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glScissorExclusiveNV)( GLint x, GLint y, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glScissorIndexed)( GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glScissorIndexedv)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3b)( GLbyte red, GLbyte green, GLbyte blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3bEXT)( GLbyte red, GLbyte green, GLbyte blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3bv)( const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3bvEXT)( const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3d)( GLdouble red, GLdouble green, GLdouble blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3dEXT)( GLdouble red, GLdouble green, GLdouble blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3dvEXT)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3f)( GLfloat red, GLfloat green, GLfloat blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3fEXT)( GLfloat red, GLfloat green, GLfloat blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3fvEXT)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3i)( GLint red, GLint green, GLint blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3iEXT)( GLint red, GLint green, GLint blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3ivEXT)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3s)( GLshort red, GLshort green, GLshort blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3sEXT)( GLshort red, GLshort green, GLshort blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3svEXT)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3ub)( GLubyte red, GLubyte green, GLubyte blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3ubEXT)( GLubyte red, GLubyte green, GLubyte blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3ubv)( const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3ubvEXT)( const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3ui)( GLuint red, GLuint green, GLuint blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3uiEXT)( GLuint red, GLuint green, GLuint blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3uiv)( const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3uivEXT)( const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3us)( GLushort red, GLushort green, GLushort blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3usEXT)( GLushort red, GLushort green, GLushort blue );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3usv)( const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColor3usvEXT)( const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glSecondaryColorFormatNV)( GLint size, GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glSecondaryColorP3ui)( GLenum type, GLuint color );
typedef void       (GLAPIENTRY *PFN_glSecondaryColorP3uiv)( GLenum type, const GLuint *color );
typedef void       (GLAPIENTRY *PFN_glSecondaryColorPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glSecondaryColorPointerEXT)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glSecondaryColorPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glSelectPerfMonitorCountersAMD)( GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList );
typedef void       (GLAPIENTRY *PFN_glSelectTextureCoordSetSGIS)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glSelectTextureSGIS)( GLenum target );
typedef void       (GLAPIENTRY *PFN_glSemaphoreParameterui64vEXT)( GLuint semaphore, GLenum pname, const GLuint64 *params );
typedef void       (GLAPIENTRY *PFN_glSeparableFilter2D)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column );
typedef void       (GLAPIENTRY *PFN_glSeparableFilter2DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column );
typedef void       (GLAPIENTRY *PFN_glSetFenceAPPLE)( GLuint fence );
typedef void       (GLAPIENTRY *PFN_glSetFenceNV)( GLuint fence, GLenum condition );
typedef void       (GLAPIENTRY *PFN_glSetFragmentShaderConstantATI)( GLuint dst, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glSetInvariantEXT)( GLuint id, GLenum type, const void *addr );
typedef void       (GLAPIENTRY *PFN_glSetLocalConstantEXT)( GLuint id, GLenum type, const void *addr );
typedef void       (GLAPIENTRY *PFN_glSetMultisamplefvAMD)( GLenum pname, GLuint index, const GLfloat *val );
typedef void       (GLAPIENTRY *PFN_glShaderBinary)( GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length );
typedef void       (GLAPIENTRY *PFN_glShaderOp1EXT)( GLenum op, GLuint res, GLuint arg1 );
typedef void       (GLAPIENTRY *PFN_glShaderOp2EXT)( GLenum op, GLuint res, GLuint arg1, GLuint arg2 );
typedef void       (GLAPIENTRY *PFN_glShaderOp3EXT)( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 );
typedef void       (GLAPIENTRY *PFN_glShaderSource)( GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length );
typedef void       (GLAPIENTRY *PFN_glShaderSourceARB)( GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length );
typedef void       (GLAPIENTRY *PFN_glShaderStorageBlockBinding)( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding );
typedef void       (GLAPIENTRY *PFN_glShadingRateImageBarrierNV)( GLboolean synchronize );
typedef void       (GLAPIENTRY *PFN_glShadingRateImagePaletteNV)( GLuint viewport, GLuint first, GLsizei count, const GLenum *rates );
typedef void       (GLAPIENTRY *PFN_glShadingRateSampleOrderCustomNV)( GLenum rate, GLuint samples, const GLint *locations );
typedef void       (GLAPIENTRY *PFN_glShadingRateSampleOrderNV)( GLenum order );
typedef void       (GLAPIENTRY *PFN_glSharpenTexFuncSGIS)( GLenum target, GLsizei n, const GLfloat *points );
typedef void       (GLAPIENTRY *PFN_glSignalSemaphoreEXT)( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *dstLayouts );
typedef void       (GLAPIENTRY *PFN_glSignalSemaphoreui64NVX)( GLuint signalGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
typedef void       (GLAPIENTRY *PFN_glSignalVkFenceNV)( GLuint64 vkFence );
typedef void       (GLAPIENTRY *PFN_glSignalVkSemaphoreNV)( GLuint64 vkSemaphore );
typedef void       (GLAPIENTRY *PFN_glSpecializeShader)( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue );
typedef void       (GLAPIENTRY *PFN_glSpecializeShaderARB)( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue );
typedef void       (GLAPIENTRY *PFN_glSpriteParameterfSGIX)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glSpriteParameterfvSGIX)( GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glSpriteParameteriSGIX)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glSpriteParameterivSGIX)( GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glStartInstrumentsSGIX)(void);
typedef void       (GLAPIENTRY *PFN_glStateCaptureNV)( GLuint state, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glStencilClearTagEXT)( GLsizei stencilTagBits, GLuint stencilClearTag );
typedef void       (GLAPIENTRY *PFN_glStencilFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues );
typedef void       (GLAPIENTRY *PFN_glStencilFillPathNV)( GLuint path, GLenum fillMode, GLuint mask );
typedef void       (GLAPIENTRY *PFN_glStencilFuncSeparate)( GLenum face, GLenum func, GLint ref, GLuint mask );
typedef void       (GLAPIENTRY *PFN_glStencilFuncSeparateATI)( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask );
typedef void       (GLAPIENTRY *PFN_glStencilMaskSeparate)( GLenum face, GLuint mask );
typedef void       (GLAPIENTRY *PFN_glStencilOpSeparate)( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass );
typedef void       (GLAPIENTRY *PFN_glStencilOpSeparateATI)( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass );
typedef void       (GLAPIENTRY *PFN_glStencilOpValueAMD)( GLenum face, GLuint value );
typedef void       (GLAPIENTRY *PFN_glStencilStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues );
typedef void       (GLAPIENTRY *PFN_glStencilStrokePathNV)( GLuint path, GLint reference, GLuint mask );
typedef void       (GLAPIENTRY *PFN_glStencilThenCoverFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
typedef void       (GLAPIENTRY *PFN_glStencilThenCoverFillPathNV)( GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode );
typedef void       (GLAPIENTRY *PFN_glStencilThenCoverStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
typedef void       (GLAPIENTRY *PFN_glStencilThenCoverStrokePathNV)( GLuint path, GLint reference, GLuint mask, GLenum coverMode );
typedef void       (GLAPIENTRY *PFN_glStopInstrumentsSGIX)( GLint marker );
typedef void       (GLAPIENTRY *PFN_glStringMarkerGREMEDY)( GLsizei len, const void *string );
typedef void       (GLAPIENTRY *PFN_glSubpixelPrecisionBiasNV)( GLuint xbits, GLuint ybits );
typedef void       (GLAPIENTRY *PFN_glSwizzleEXT)( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW );
typedef void       (GLAPIENTRY *PFN_glSyncTextureINTEL)( GLuint texture );
typedef void       (GLAPIENTRY *PFN_glTagSampleBufferSGIX)(void);
typedef void       (GLAPIENTRY *PFN_glTangent3bEXT)( GLbyte tx, GLbyte ty, GLbyte tz );
typedef void       (GLAPIENTRY *PFN_glTangent3bvEXT)( const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glTangent3dEXT)( GLdouble tx, GLdouble ty, GLdouble tz );
typedef void       (GLAPIENTRY *PFN_glTangent3dvEXT)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glTangent3fEXT)( GLfloat tx, GLfloat ty, GLfloat tz );
typedef void       (GLAPIENTRY *PFN_glTangent3fvEXT)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTangent3iEXT)( GLint tx, GLint ty, GLint tz );
typedef void       (GLAPIENTRY *PFN_glTangent3ivEXT)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glTangent3sEXT)( GLshort tx, GLshort ty, GLshort tz );
typedef void       (GLAPIENTRY *PFN_glTangent3svEXT)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glTangentPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glTbufferMask3DFX)( GLuint mask );
typedef void       (GLAPIENTRY *PFN_glTessellationFactorAMD)( GLfloat factor );
typedef void       (GLAPIENTRY *PFN_glTessellationModeAMD)( GLenum mode );
typedef GLboolean  (GLAPIENTRY *PFN_glTestFenceAPPLE)( GLuint fence );
typedef GLboolean  (GLAPIENTRY *PFN_glTestFenceNV)( GLuint fence );
typedef GLboolean  (GLAPIENTRY *PFN_glTestObjectAPPLE)( GLenum object, GLuint name );
typedef void       (GLAPIENTRY *PFN_glTexAttachMemoryNV)( GLenum target, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTexBuffer)( GLenum target, GLenum internalformat, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glTexBufferARB)( GLenum target, GLenum internalformat, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glTexBufferEXT)( GLenum target, GLenum internalformat, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glTexBufferRange)( GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glTexBumpParameterfvATI)( GLenum pname, const GLfloat *param );
typedef void       (GLAPIENTRY *PFN_glTexBumpParameterivATI)( GLenum pname, const GLint *param );
typedef void       (GLAPIENTRY *PFN_glTexCoord1bOES)( GLbyte s );
typedef void       (GLAPIENTRY *PFN_glTexCoord1bvOES)( const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoord1hNV)( GLhalfNV s );
typedef void       (GLAPIENTRY *PFN_glTexCoord1hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord1xOES)( GLfixed s );
typedef void       (GLAPIENTRY *PFN_glTexCoord1xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoord2bOES)( GLbyte s, GLbyte t );
typedef void       (GLAPIENTRY *PFN_glTexCoord2bvOES)( const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fColor3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fColor3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fColor4fNormal3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fColor4fNormal3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fColor4ubVertex3fSUN)( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fColor4ubVertex3fvSUN)( const GLfloat *tc, const GLubyte *c, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fNormal3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fNormal3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glTexCoord2fVertex3fvSUN)( const GLfloat *tc, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2hNV)( GLhalfNV s, GLhalfNV t );
typedef void       (GLAPIENTRY *PFN_glTexCoord2hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord2xOES)( GLfixed s, GLfixed t );
typedef void       (GLAPIENTRY *PFN_glTexCoord2xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoord3bOES)( GLbyte s, GLbyte t, GLbyte r );
typedef void       (GLAPIENTRY *PFN_glTexCoord3bvOES)( const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoord3hNV)( GLhalfNV s, GLhalfNV t, GLhalfNV r );
typedef void       (GLAPIENTRY *PFN_glTexCoord3hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord3xOES)( GLfixed s, GLfixed t, GLfixed r );
typedef void       (GLAPIENTRY *PFN_glTexCoord3xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoord4bOES)( GLbyte s, GLbyte t, GLbyte r, GLbyte q );
typedef void       (GLAPIENTRY *PFN_glTexCoord4bvOES)( const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoord4fColor4fNormal3fVertex4fSUN)( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glTexCoord4fColor4fNormal3fVertex4fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord4fVertex4fSUN)( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glTexCoord4fVertex4fvSUN)( const GLfloat *tc, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord4hNV)( GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q );
typedef void       (GLAPIENTRY *PFN_glTexCoord4hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glTexCoord4xOES)( GLfixed s, GLfixed t, GLfixed r, GLfixed q );
typedef void       (GLAPIENTRY *PFN_glTexCoord4xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordFormatNV)( GLint size, GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glTexCoordP1ui)( GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordP1uiv)( GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordP2ui)( GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordP2uiv)( GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordP3ui)( GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordP3uiv)( GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordP4ui)( GLenum type, GLuint coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordP4uiv)( GLenum type, const GLuint *coords );
typedef void       (GLAPIENTRY *PFN_glTexCoordPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glTexCoordPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glTexCoordPointervINTEL)( GLint size, GLenum type, const void **pointer );
typedef void       (GLAPIENTRY *PFN_glTexEnvxOES)( GLenum target, GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glTexEnvxvOES)( GLenum target, GLenum pname, const GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glTexFilterFuncSGIS)( GLenum target, GLenum filter, GLsizei n, const GLfloat *weights );
typedef void       (GLAPIENTRY *PFN_glTexGenxOES)( GLenum coord, GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glTexGenxvOES)( GLenum coord, GLenum pname, const GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glTexImage2DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTexImage2DMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
typedef void       (GLAPIENTRY *PFN_glTexImage3D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexImage3DEXT)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexImage3DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTexImage3DMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
typedef void       (GLAPIENTRY *PFN_glTexImage4DSGIS)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexPageCommitmentARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit );
typedef void       (GLAPIENTRY *PFN_glTexParameterIiv)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTexParameterIivEXT)( GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTexParameterIuiv)( GLenum target, GLenum pname, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glTexParameterIuivEXT)( GLenum target, GLenum pname, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glTexParameterxOES)( GLenum target, GLenum pname, GLfixed param );
typedef void       (GLAPIENTRY *PFN_glTexParameterxvOES)( GLenum target, GLenum pname, const GLfixed *params );
typedef void       (GLAPIENTRY *PFN_glTexRenderbufferNV)( GLenum target, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glTexStorage1D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glTexStorage2D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glTexStorage2DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTexStorage3D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
typedef void       (GLAPIENTRY *PFN_glTexStorage3DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTexStorageMem1DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTexStorageMem2DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTexStorageMem2DMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTexStorageMem3DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTexStorageMem3DMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTexStorageSparseAMD)( GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glTexSubImage1DEXT)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexSubImage2DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexSubImage3DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTexSubImage4DSGIS)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureAttachMemoryNV)( GLuint texture, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTextureBarrier)(void);
typedef void       (GLAPIENTRY *PFN_glTextureBarrierNV)(void);
typedef void       (GLAPIENTRY *PFN_glTextureBuffer)( GLuint texture, GLenum internalformat, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glTextureBufferEXT)( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glTextureBufferRange)( GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glTextureBufferRangeEXT)( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glTextureColorMaskSGIS)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
typedef void       (GLAPIENTRY *PFN_glTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureImage2DMultisampleCoverageNV)( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
typedef void       (GLAPIENTRY *PFN_glTextureImage2DMultisampleNV)( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
typedef void       (GLAPIENTRY *PFN_glTextureImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureImage3DMultisampleCoverageNV)( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
typedef void       (GLAPIENTRY *PFN_glTextureImage3DMultisampleNV)( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
typedef void       (GLAPIENTRY *PFN_glTextureLightEXT)( GLenum pname );
typedef void       (GLAPIENTRY *PFN_glTextureMaterialEXT)( GLenum face, GLenum mode );
typedef void       (GLAPIENTRY *PFN_glTextureNormalEXT)( GLenum mode );
typedef void       (GLAPIENTRY *PFN_glTexturePageCommitmentEXT)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit );
typedef void       (GLAPIENTRY *PFN_glTextureParameterIiv)( GLuint texture, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTextureParameterIivEXT)( GLuint texture, GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTextureParameterIuiv)( GLuint texture, GLenum pname, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glTextureParameterIuivEXT)( GLuint texture, GLenum target, GLenum pname, const GLuint *params );
typedef void       (GLAPIENTRY *PFN_glTextureParameterf)( GLuint texture, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glTextureParameterfEXT)( GLuint texture, GLenum target, GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glTextureParameterfv)( GLuint texture, GLenum pname, const GLfloat *param );
typedef void       (GLAPIENTRY *PFN_glTextureParameterfvEXT)( GLuint texture, GLenum target, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glTextureParameteri)( GLuint texture, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glTextureParameteriEXT)( GLuint texture, GLenum target, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glTextureParameteriv)( GLuint texture, GLenum pname, const GLint *param );
typedef void       (GLAPIENTRY *PFN_glTextureParameterivEXT)( GLuint texture, GLenum target, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glTextureRangeAPPLE)( GLenum target, GLsizei length, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glTextureRenderbufferEXT)( GLuint texture, GLenum target, GLuint renderbuffer );
typedef void       (GLAPIENTRY *PFN_glTextureStorage1D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glTextureStorage1DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width );
typedef void       (GLAPIENTRY *PFN_glTextureStorage2D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glTextureStorage2DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
typedef void       (GLAPIENTRY *PFN_glTextureStorage2DMultisample)( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTextureStorage2DMultisampleEXT)( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTextureStorage3D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
typedef void       (GLAPIENTRY *PFN_glTextureStorage3DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
typedef void       (GLAPIENTRY *PFN_glTextureStorage3DMultisample)( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTextureStorage3DMultisampleEXT)( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
typedef void       (GLAPIENTRY *PFN_glTextureStorageMem1DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTextureStorageMem2DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTextureStorageMem2DMultisampleEXT)( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTextureStorageMem3DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTextureStorageMem3DMultisampleEXT)( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
typedef void       (GLAPIENTRY *PFN_glTextureStorageSparseAMD)( GLuint texture, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags );
typedef void       (GLAPIENTRY *PFN_glTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
typedef void       (GLAPIENTRY *PFN_glTextureView)( GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers );
typedef void       (GLAPIENTRY *PFN_glTrackMatrixNV)( GLenum target, GLuint address, GLenum matrix, GLenum transform );
typedef void       (GLAPIENTRY *PFN_glTransformFeedbackAttribsNV)( GLsizei count, const GLint *attribs, GLenum bufferMode );
typedef void       (GLAPIENTRY *PFN_glTransformFeedbackBufferBase)( GLuint xfb, GLuint index, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glTransformFeedbackBufferRange)( GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
typedef void       (GLAPIENTRY *PFN_glTransformFeedbackStreamAttribsNV)( GLsizei count, const GLint *attribs, GLsizei nbuffers, const GLint *bufstreams, GLenum bufferMode );
typedef void       (GLAPIENTRY *PFN_glTransformFeedbackVaryings)( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode );
typedef void       (GLAPIENTRY *PFN_glTransformFeedbackVaryingsEXT)( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode );
typedef void       (GLAPIENTRY *PFN_glTransformFeedbackVaryingsNV)( GLuint program, GLsizei count, const GLint *locations, GLenum bufferMode );
typedef void       (GLAPIENTRY *PFN_glTransformPathNV)( GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues );
typedef void       (GLAPIENTRY *PFN_glTranslatexOES)( GLfixed x, GLfixed y, GLfixed z );
typedef void       (GLAPIENTRY *PFN_glUniform1d)( GLint location, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glUniform1dv)( GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniform1f)( GLint location, GLfloat v0 );
typedef void       (GLAPIENTRY *PFN_glUniform1fARB)( GLint location, GLfloat v0 );
typedef void       (GLAPIENTRY *PFN_glUniform1fv)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform1fvARB)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform1i)( GLint location, GLint v0 );
typedef void       (GLAPIENTRY *PFN_glUniform1i64ARB)( GLint location, GLint64 x );
typedef void       (GLAPIENTRY *PFN_glUniform1i64NV)( GLint location, GLint64EXT x );
typedef void       (GLAPIENTRY *PFN_glUniform1i64vARB)( GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform1i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform1iARB)( GLint location, GLint v0 );
typedef void       (GLAPIENTRY *PFN_glUniform1iv)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform1ivARB)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform1ui)( GLint location, GLuint v0 );
typedef void       (GLAPIENTRY *PFN_glUniform1ui64ARB)( GLint location, GLuint64 x );
typedef void       (GLAPIENTRY *PFN_glUniform1ui64NV)( GLint location, GLuint64EXT x );
typedef void       (GLAPIENTRY *PFN_glUniform1ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform1ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform1uiEXT)( GLint location, GLuint v0 );
typedef void       (GLAPIENTRY *PFN_glUniform1uiv)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniform1uivEXT)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniform2d)( GLint location, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glUniform2dv)( GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniform2f)( GLint location, GLfloat v0, GLfloat v1 );
typedef void       (GLAPIENTRY *PFN_glUniform2fARB)( GLint location, GLfloat v0, GLfloat v1 );
typedef void       (GLAPIENTRY *PFN_glUniform2fv)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform2fvARB)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform2i)( GLint location, GLint v0, GLint v1 );
typedef void       (GLAPIENTRY *PFN_glUniform2i64ARB)( GLint location, GLint64 x, GLint64 y );
typedef void       (GLAPIENTRY *PFN_glUniform2i64NV)( GLint location, GLint64EXT x, GLint64EXT y );
typedef void       (GLAPIENTRY *PFN_glUniform2i64vARB)( GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform2i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform2iARB)( GLint location, GLint v0, GLint v1 );
typedef void       (GLAPIENTRY *PFN_glUniform2iv)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform2ivARB)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform2ui)( GLint location, GLuint v0, GLuint v1 );
typedef void       (GLAPIENTRY *PFN_glUniform2ui64ARB)( GLint location, GLuint64 x, GLuint64 y );
typedef void       (GLAPIENTRY *PFN_glUniform2ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y );
typedef void       (GLAPIENTRY *PFN_glUniform2ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform2ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform2uiEXT)( GLint location, GLuint v0, GLuint v1 );
typedef void       (GLAPIENTRY *PFN_glUniform2uiv)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniform2uivEXT)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniform3d)( GLint location, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glUniform3dv)( GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniform3f)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
typedef void       (GLAPIENTRY *PFN_glUniform3fARB)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
typedef void       (GLAPIENTRY *PFN_glUniform3fv)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform3fvARB)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform3i)( GLint location, GLint v0, GLint v1, GLint v2 );
typedef void       (GLAPIENTRY *PFN_glUniform3i64ARB)( GLint location, GLint64 x, GLint64 y, GLint64 z );
typedef void       (GLAPIENTRY *PFN_glUniform3i64NV)( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z );
typedef void       (GLAPIENTRY *PFN_glUniform3i64vARB)( GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform3i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform3iARB)( GLint location, GLint v0, GLint v1, GLint v2 );
typedef void       (GLAPIENTRY *PFN_glUniform3iv)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform3ivARB)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform3ui)( GLint location, GLuint v0, GLuint v1, GLuint v2 );
typedef void       (GLAPIENTRY *PFN_glUniform3ui64ARB)( GLint location, GLuint64 x, GLuint64 y, GLuint64 z );
typedef void       (GLAPIENTRY *PFN_glUniform3ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
typedef void       (GLAPIENTRY *PFN_glUniform3ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform3ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform3uiEXT)( GLint location, GLuint v0, GLuint v1, GLuint v2 );
typedef void       (GLAPIENTRY *PFN_glUniform3uiv)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniform3uivEXT)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniform4d)( GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glUniform4dv)( GLint location, GLsizei count, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniform4f)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
typedef void       (GLAPIENTRY *PFN_glUniform4fARB)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
typedef void       (GLAPIENTRY *PFN_glUniform4fv)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform4fvARB)( GLint location, GLsizei count, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniform4i)( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
typedef void       (GLAPIENTRY *PFN_glUniform4i64ARB)( GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w );
typedef void       (GLAPIENTRY *PFN_glUniform4i64NV)( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
typedef void       (GLAPIENTRY *PFN_glUniform4i64vARB)( GLint location, GLsizei count, const GLint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform4i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform4iARB)( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
typedef void       (GLAPIENTRY *PFN_glUniform4iv)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform4ivARB)( GLint location, GLsizei count, const GLint *value );
typedef void       (GLAPIENTRY *PFN_glUniform4ui)( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
typedef void       (GLAPIENTRY *PFN_glUniform4ui64ARB)( GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w );
typedef void       (GLAPIENTRY *PFN_glUniform4ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
typedef void       (GLAPIENTRY *PFN_glUniform4ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniform4ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUniform4uiEXT)( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
typedef void       (GLAPIENTRY *PFN_glUniform4uiv)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniform4uivEXT)( GLint location, GLsizei count, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glUniformBlockBinding)( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding );
typedef void       (GLAPIENTRY *PFN_glUniformBufferEXT)( GLuint program, GLint location, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glUniformHandleui64ARB)( GLint location, GLuint64 value );
typedef void       (GLAPIENTRY *PFN_glUniformHandleui64NV)( GLint location, GLuint64 value );
typedef void       (GLAPIENTRY *PFN_glUniformHandleui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniformHandleui64vNV)( GLint location, GLsizei count, const GLuint64 *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix2fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix2x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix2x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix2x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix2x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix3fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix3x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix3x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix3x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix3x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix4fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix4x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix4x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix4x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
typedef void       (GLAPIENTRY *PFN_glUniformMatrix4x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
typedef void       (GLAPIENTRY *PFN_glUniformSubroutinesuiv)( GLenum shadertype, GLsizei count, const GLuint *indices );
typedef void       (GLAPIENTRY *PFN_glUniformui64NV)( GLint location, GLuint64EXT value );
typedef void       (GLAPIENTRY *PFN_glUniformui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
typedef void       (GLAPIENTRY *PFN_glUnlockArraysEXT)(void);
typedef GLboolean  (GLAPIENTRY *PFN_glUnmapBuffer)( GLenum target );
typedef GLboolean  (GLAPIENTRY *PFN_glUnmapBufferARB)( GLenum target );
typedef GLboolean  (GLAPIENTRY *PFN_glUnmapNamedBuffer)( GLuint buffer );
typedef GLboolean  (GLAPIENTRY *PFN_glUnmapNamedBufferEXT)( GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glUnmapObjectBufferATI)( GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glUnmapTexture2DINTEL)( GLuint texture, GLint level );
typedef void       (GLAPIENTRY *PFN_glUpdateObjectBufferATI)( GLuint buffer, GLuint offset, GLsizei size, const void *pointer, GLenum preserve );
typedef void       (GLAPIENTRY *PFN_glUploadGpuMaskNVX)( GLbitfield mask );
typedef void       (GLAPIENTRY *PFN_glUseProgram)( GLuint program );
typedef void       (GLAPIENTRY *PFN_glUseProgramObjectARB)( GLhandleARB programObj );
typedef void       (GLAPIENTRY *PFN_glUseProgramStages)( GLuint pipeline, GLbitfield stages, GLuint program );
typedef void       (GLAPIENTRY *PFN_glUseShaderProgramEXT)( GLenum type, GLuint program );
typedef void       (GLAPIENTRY *PFN_glVDPAUFiniNV)(void);
typedef void       (GLAPIENTRY *PFN_glVDPAUGetSurfaceivNV)( GLvdpauSurfaceNV surface, GLenum pname, GLsizei count, GLsizei *length, GLint *values );
typedef void       (GLAPIENTRY *PFN_glVDPAUInitNV)( const void *vdpDevice, const void *getProcAddress );
typedef GLboolean  (GLAPIENTRY *PFN_glVDPAUIsSurfaceNV)( GLvdpauSurfaceNV surface );
typedef void       (GLAPIENTRY *PFN_glVDPAUMapSurfacesNV)( GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces );
typedef GLvdpauSurfaceNV (GLAPIENTRY *PFN_glVDPAURegisterOutputSurfaceNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames );
typedef GLvdpauSurfaceNV (GLAPIENTRY *PFN_glVDPAURegisterVideoSurfaceNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames );
typedef GLvdpauSurfaceNV (GLAPIENTRY *PFN_glVDPAURegisterVideoSurfaceWithPictureStructureNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames, GLboolean isFrameStructure );
typedef void       (GLAPIENTRY *PFN_glVDPAUSurfaceAccessNV)( GLvdpauSurfaceNV surface, GLenum access );
typedef void       (GLAPIENTRY *PFN_glVDPAUUnmapSurfacesNV)( GLsizei numSurface, const GLvdpauSurfaceNV *surfaces );
typedef void       (GLAPIENTRY *PFN_glVDPAUUnregisterSurfaceNV)( GLvdpauSurfaceNV surface );
typedef void       (GLAPIENTRY *PFN_glValidateProgram)( GLuint program );
typedef void       (GLAPIENTRY *PFN_glValidateProgramARB)( GLhandleARB programObj );
typedef void       (GLAPIENTRY *PFN_glValidateProgramPipeline)( GLuint pipeline );
typedef void       (GLAPIENTRY *PFN_glVariantArrayObjectATI)( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
typedef void       (GLAPIENTRY *PFN_glVariantPointerEXT)( GLuint id, GLenum type, GLuint stride, const void *addr );
typedef void       (GLAPIENTRY *PFN_glVariantbvEXT)( GLuint id, const GLbyte *addr );
typedef void       (GLAPIENTRY *PFN_glVariantdvEXT)( GLuint id, const GLdouble *addr );
typedef void       (GLAPIENTRY *PFN_glVariantfvEXT)( GLuint id, const GLfloat *addr );
typedef void       (GLAPIENTRY *PFN_glVariantivEXT)( GLuint id, const GLint *addr );
typedef void       (GLAPIENTRY *PFN_glVariantsvEXT)( GLuint id, const GLshort *addr );
typedef void       (GLAPIENTRY *PFN_glVariantubvEXT)( GLuint id, const GLubyte *addr );
typedef void       (GLAPIENTRY *PFN_glVariantuivEXT)( GLuint id, const GLuint *addr );
typedef void       (GLAPIENTRY *PFN_glVariantusvEXT)( GLuint id, const GLushort *addr );
typedef void       (GLAPIENTRY *PFN_glVertex2bOES)( GLbyte x, GLbyte y );
typedef void       (GLAPIENTRY *PFN_glVertex2bvOES)( const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glVertex2hNV)( GLhalfNV x, GLhalfNV y );
typedef void       (GLAPIENTRY *PFN_glVertex2hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertex2xOES)( GLfixed x );
typedef void       (GLAPIENTRY *PFN_glVertex2xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glVertex3bOES)( GLbyte x, GLbyte y, GLbyte z );
typedef void       (GLAPIENTRY *PFN_glVertex3bvOES)( const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glVertex3hNV)( GLhalfNV x, GLhalfNV y, GLhalfNV z );
typedef void       (GLAPIENTRY *PFN_glVertex3hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertex3xOES)( GLfixed x, GLfixed y );
typedef void       (GLAPIENTRY *PFN_glVertex3xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glVertex4bOES)( GLbyte x, GLbyte y, GLbyte z, GLbyte w );
typedef void       (GLAPIENTRY *PFN_glVertex4bvOES)( const GLbyte *coords );
typedef void       (GLAPIENTRY *PFN_glVertex4hNV)( GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w );
typedef void       (GLAPIENTRY *PFN_glVertex4hvNV)( const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertex4xOES)( GLfixed x, GLfixed y, GLfixed z );
typedef void       (GLAPIENTRY *PFN_glVertex4xvOES)( const GLfixed *coords );
typedef void       (GLAPIENTRY *PFN_glVertexArrayAttribBinding)( GLuint vaobj, GLuint attribindex, GLuint bindingindex );
typedef void       (GLAPIENTRY *PFN_glVertexArrayAttribFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayAttribIFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayAttribLFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayBindVertexBufferEXT)( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glVertexArrayBindingDivisor)( GLuint vaobj, GLuint bindingindex, GLuint divisor );
typedef void       (GLAPIENTRY *PFN_glVertexArrayColorOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayEdgeFlagOffsetEXT)( GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayElementBuffer)( GLuint vaobj, GLuint buffer );
typedef void       (GLAPIENTRY *PFN_glVertexArrayFogCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayIndexOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayMultiTexCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayNormalOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayParameteriAPPLE)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glVertexArrayRangeAPPLE)( GLsizei length, void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexArrayRangeNV)( GLsizei length, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexArraySecondaryColorOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayTexCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribBindingEXT)( GLuint vaobj, GLuint attribindex, GLuint bindingindex );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribDivisorEXT)( GLuint vaobj, GLuint index, GLuint divisor );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribIFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribIOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribLFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribLOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexAttribOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexBindingDivisorEXT)( GLuint vaobj, GLuint bindingindex, GLuint divisor );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexBuffer)( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexBuffers)( GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides );
typedef void       (GLAPIENTRY *PFN_glVertexArrayVertexOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1d)( GLuint index, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1dARB)( GLuint index, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1dNV)( GLuint index, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1dvARB)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1dvNV)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1f)( GLuint index, GLfloat x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1fARB)( GLuint index, GLfloat x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1fNV)( GLuint index, GLfloat x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1fv)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1fvARB)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1fvNV)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1hNV)( GLuint index, GLhalfNV x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1hvNV)( GLuint index, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1s)( GLuint index, GLshort x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1sARB)( GLuint index, GLshort x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1sNV)( GLuint index, GLshort x );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1sv)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1svARB)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib1svNV)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2d)( GLuint index, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2dARB)( GLuint index, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2dNV)( GLuint index, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2dvARB)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2dvNV)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2f)( GLuint index, GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2fARB)( GLuint index, GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2fNV)( GLuint index, GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2fv)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2fvARB)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2fvNV)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2hNV)( GLuint index, GLhalfNV x, GLhalfNV y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2hvNV)( GLuint index, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2s)( GLuint index, GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2sARB)( GLuint index, GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2sNV)( GLuint index, GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2sv)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2svARB)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib2svNV)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3d)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3dARB)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3dNV)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3dvARB)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3dvNV)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3f)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3fv)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3fvARB)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3fvNV)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3hNV)( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3hvNV)( GLuint index, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3s)( GLuint index, GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3sARB)( GLuint index, GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3sNV)( GLuint index, GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3sv)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3svARB)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib3svNV)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4Nbv)( GLuint index, const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4NbvARB)( GLuint index, const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4Niv)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4NivARB)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4Nsv)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4NsvARB)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4Nub)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4NubARB)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4Nubv)( GLuint index, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4NubvARB)( GLuint index, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4Nuiv)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4NuivARB)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4Nusv)( GLuint index, const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4NusvARB)( GLuint index, const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4bv)( GLuint index, const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4bvARB)( GLuint index, const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4d)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4dARB)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4dNV)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4dvARB)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4dvNV)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4f)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4fv)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4fvARB)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4fvNV)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4hNV)( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4hvNV)( GLuint index, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4iv)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4ivARB)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4s)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4sARB)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4sNV)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4sv)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4svARB)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4svNV)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4ubNV)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4ubv)( GLuint index, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4ubvARB)( GLuint index, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4ubvNV)( GLuint index, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4uiv)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4uivARB)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4usv)( GLuint index, const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttrib4usvARB)( GLuint index, const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribArrayObjectATI)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset );
typedef void       (GLAPIENTRY *PFN_glVertexAttribBinding)( GLuint attribindex, GLuint bindingindex );
typedef void       (GLAPIENTRY *PFN_glVertexAttribDivisor)( GLuint index, GLuint divisor );
typedef void       (GLAPIENTRY *PFN_glVertexAttribDivisorARB)( GLuint index, GLuint divisor );
typedef void       (GLAPIENTRY *PFN_glVertexAttribFormat)( GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexAttribFormatNV)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1i)( GLuint index, GLint x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1iEXT)( GLuint index, GLint x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1iv)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1ivEXT)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1ui)( GLuint index, GLuint x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1uiEXT)( GLuint index, GLuint x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1uiv)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI1uivEXT)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2i)( GLuint index, GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2iEXT)( GLuint index, GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2iv)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2ivEXT)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2ui)( GLuint index, GLuint x, GLuint y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2uiEXT)( GLuint index, GLuint x, GLuint y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2uiv)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI2uivEXT)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3i)( GLuint index, GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3iEXT)( GLuint index, GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3iv)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3ivEXT)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3ui)( GLuint index, GLuint x, GLuint y, GLuint z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3uiEXT)( GLuint index, GLuint x, GLuint y, GLuint z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3uiv)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI3uivEXT)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4bv)( GLuint index, const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4bvEXT)( GLuint index, const GLbyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4i)( GLuint index, GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4iEXT)( GLuint index, GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4iv)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4ivEXT)( GLuint index, const GLint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4sv)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4svEXT)( GLuint index, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4ubv)( GLuint index, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4ubvEXT)( GLuint index, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4ui)( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4uiEXT)( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4uiv)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4uivEXT)( GLuint index, const GLuint *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4usv)( GLuint index, const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribI4usvEXT)( GLuint index, const GLushort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribIFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexAttribIFormatNV)( GLuint index, GLint size, GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glVertexAttribIPointer)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexAttribIPointerEXT)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1d)( GLuint index, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1dEXT)( GLuint index, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1dvEXT)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1i64NV)( GLuint index, GLint64EXT x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1i64vNV)( GLuint index, const GLint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1ui64ARB)( GLuint index, GLuint64EXT x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1ui64NV)( GLuint index, GLuint64EXT x );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1ui64vARB)( GLuint index, const GLuint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL1ui64vNV)( GLuint index, const GLuint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2d)( GLuint index, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2dEXT)( GLuint index, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2dvEXT)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2i64NV)( GLuint index, GLint64EXT x, GLint64EXT y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2i64vNV)( GLuint index, const GLint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL2ui64vNV)( GLuint index, const GLuint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3d)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3dEXT)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3dvEXT)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3i64NV)( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3i64vNV)( GLuint index, const GLint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL3ui64vNV)( GLuint index, const GLuint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4d)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4dEXT)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4dv)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4dvEXT)( GLuint index, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4i64NV)( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4i64vNV)( GLuint index, const GLint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
typedef void       (GLAPIENTRY *PFN_glVertexAttribL4ui64vNV)( GLuint index, const GLuint64EXT *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribLFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
typedef void       (GLAPIENTRY *PFN_glVertexAttribLFormatNV)( GLuint index, GLint size, GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glVertexAttribLPointer)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexAttribLPointerEXT)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP1ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP1uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP2ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP2uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP3ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP3uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP4ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribP4uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glVertexAttribParameteriAMD)( GLuint index, GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glVertexAttribPointer)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexAttribPointerARB)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexAttribPointerNV)( GLuint index, GLint fsize, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs1dvNV)( GLuint index, GLsizei count, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs1fvNV)( GLuint index, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs1hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs1svNV)( GLuint index, GLsizei count, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs2dvNV)( GLuint index, GLsizei count, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs2fvNV)( GLuint index, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs2hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs2svNV)( GLuint index, GLsizei count, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs3dvNV)( GLuint index, GLsizei count, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs3fvNV)( GLuint index, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs3hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs3svNV)( GLuint index, GLsizei count, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs4dvNV)( GLuint index, GLsizei count, const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs4fvNV)( GLuint index, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs4hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs4svNV)( GLuint index, GLsizei count, const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glVertexAttribs4ubvNV)( GLuint index, GLsizei count, const GLubyte *v );
typedef void       (GLAPIENTRY *PFN_glVertexBindingDivisor)( GLuint bindingindex, GLuint divisor );
typedef void       (GLAPIENTRY *PFN_glVertexBlendARB)( GLint count );
typedef void       (GLAPIENTRY *PFN_glVertexBlendEnvfATI)( GLenum pname, GLfloat param );
typedef void       (GLAPIENTRY *PFN_glVertexBlendEnviATI)( GLenum pname, GLint param );
typedef void       (GLAPIENTRY *PFN_glVertexFormatNV)( GLint size, GLenum type, GLsizei stride );
typedef void       (GLAPIENTRY *PFN_glVertexP2ui)( GLenum type, GLuint value );
typedef void       (GLAPIENTRY *PFN_glVertexP2uiv)( GLenum type, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glVertexP3ui)( GLenum type, GLuint value );
typedef void       (GLAPIENTRY *PFN_glVertexP3uiv)( GLenum type, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glVertexP4ui)( GLenum type, GLuint value );
typedef void       (GLAPIENTRY *PFN_glVertexP4uiv)( GLenum type, const GLuint *value );
typedef void       (GLAPIENTRY *PFN_glVertexPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
typedef void       (GLAPIENTRY *PFN_glVertexPointervINTEL)( GLint size, GLenum type, const void **pointer );
typedef void       (GLAPIENTRY *PFN_glVertexStream1dATI)( GLenum stream, GLdouble x );
typedef void       (GLAPIENTRY *PFN_glVertexStream1dvATI)( GLenum stream, const GLdouble *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream1fATI)( GLenum stream, GLfloat x );
typedef void       (GLAPIENTRY *PFN_glVertexStream1fvATI)( GLenum stream, const GLfloat *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream1iATI)( GLenum stream, GLint x );
typedef void       (GLAPIENTRY *PFN_glVertexStream1ivATI)( GLenum stream, const GLint *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream1sATI)( GLenum stream, GLshort x );
typedef void       (GLAPIENTRY *PFN_glVertexStream1svATI)( GLenum stream, const GLshort *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream2dATI)( GLenum stream, GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glVertexStream2dvATI)( GLenum stream, const GLdouble *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream2fATI)( GLenum stream, GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glVertexStream2fvATI)( GLenum stream, const GLfloat *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream2iATI)( GLenum stream, GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glVertexStream2ivATI)( GLenum stream, const GLint *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream2sATI)( GLenum stream, GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glVertexStream2svATI)( GLenum stream, const GLshort *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream3dATI)( GLenum stream, GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glVertexStream3dvATI)( GLenum stream, const GLdouble *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream3fATI)( GLenum stream, GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glVertexStream3fvATI)( GLenum stream, const GLfloat *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream3iATI)( GLenum stream, GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glVertexStream3ivATI)( GLenum stream, const GLint *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream3sATI)( GLenum stream, GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glVertexStream3svATI)( GLenum stream, const GLshort *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream4dATI)( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glVertexStream4dvATI)( GLenum stream, const GLdouble *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream4fATI)( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glVertexStream4fvATI)( GLenum stream, const GLfloat *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream4iATI)( GLenum stream, GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glVertexStream4ivATI)( GLenum stream, const GLint *coords );
typedef void       (GLAPIENTRY *PFN_glVertexStream4sATI)( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w );
typedef void       (GLAPIENTRY *PFN_glVertexStream4svATI)( GLenum stream, const GLshort *coords );
typedef void       (GLAPIENTRY *PFN_glVertexWeightPointerEXT)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glVertexWeightfEXT)( GLfloat weight );
typedef void       (GLAPIENTRY *PFN_glVertexWeightfvEXT)( const GLfloat *weight );
typedef void       (GLAPIENTRY *PFN_glVertexWeighthNV)( GLhalfNV weight );
typedef void       (GLAPIENTRY *PFN_glVertexWeighthvNV)( const GLhalfNV *weight );
typedef GLenum     (GLAPIENTRY *PFN_glVideoCaptureNV)( GLuint video_capture_slot, GLuint *sequence_num, GLuint64EXT *capture_time );
typedef void       (GLAPIENTRY *PFN_glVideoCaptureStreamParameterdvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble *params );
typedef void       (GLAPIENTRY *PFN_glVideoCaptureStreamParameterfvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat *params );
typedef void       (GLAPIENTRY *PFN_glVideoCaptureStreamParameterivNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint *params );
typedef void       (GLAPIENTRY *PFN_glViewportArrayv)( GLuint first, GLsizei count, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glViewportIndexedf)( GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h );
typedef void       (GLAPIENTRY *PFN_glViewportIndexedfv)( GLuint index, const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glViewportPositionWScaleNV)( GLuint index, GLfloat xcoeff, GLfloat ycoeff );
typedef void       (GLAPIENTRY *PFN_glViewportSwizzleNV)( GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew );
typedef void       (GLAPIENTRY *PFN_glWaitSemaphoreEXT)( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *srcLayouts );
typedef void       (GLAPIENTRY *PFN_glWaitSemaphoreui64NVX)( GLuint waitGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
typedef void       (GLAPIENTRY *PFN_glWaitSync)( GLsync sync, GLbitfield flags, GLuint64 timeout );
typedef void       (GLAPIENTRY *PFN_glWaitVkSemaphoreNV)( GLuint64 vkSemaphore );
typedef void       (GLAPIENTRY *PFN_glWeightPathsNV)( GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights );
typedef void       (GLAPIENTRY *PFN_glWeightPointerARB)( GLint size, GLenum type, GLsizei stride, const void *pointer );
typedef void       (GLAPIENTRY *PFN_glWeightbvARB)( GLint size, const GLbyte *weights );
typedef void       (GLAPIENTRY *PFN_glWeightdvARB)( GLint size, const GLdouble *weights );
typedef void       (GLAPIENTRY *PFN_glWeightfvARB)( GLint size, const GLfloat *weights );
typedef void       (GLAPIENTRY *PFN_glWeightivARB)( GLint size, const GLint *weights );
typedef void       (GLAPIENTRY *PFN_glWeightsvARB)( GLint size, const GLshort *weights );
typedef void       (GLAPIENTRY *PFN_glWeightubvARB)( GLint size, const GLubyte *weights );
typedef void       (GLAPIENTRY *PFN_glWeightuivARB)( GLint size, const GLuint *weights );
typedef void       (GLAPIENTRY *PFN_glWeightusvARB)( GLint size, const GLushort *weights );
typedef void       (GLAPIENTRY *PFN_glWindowPos2d)( GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2dARB)( GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2dMESA)( GLdouble x, GLdouble y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2dvARB)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2dvMESA)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2f)( GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2fARB)( GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2fMESA)( GLfloat x, GLfloat y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2fvARB)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2fvMESA)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2i)( GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2iARB)( GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2iMESA)( GLint x, GLint y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2ivARB)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2ivMESA)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2s)( GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2sARB)( GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2sMESA)( GLshort x, GLshort y );
typedef void       (GLAPIENTRY *PFN_glWindowPos2sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2svARB)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos2svMESA)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3d)( GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3dARB)( GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3dMESA)( GLdouble x, GLdouble y, GLdouble z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3dv)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3dvARB)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3dvMESA)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3f)( GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3fARB)( GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3fMESA)( GLfloat x, GLfloat y, GLfloat z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3fv)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3fvARB)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3fvMESA)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3i)( GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3iARB)( GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3iMESA)( GLint x, GLint y, GLint z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3iv)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3ivARB)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3ivMESA)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3s)( GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3sARB)( GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3sMESA)( GLshort x, GLshort y, GLshort z );
typedef void       (GLAPIENTRY *PFN_glWindowPos3sv)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3svARB)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos3svMESA)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos4dMESA)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
typedef void       (GLAPIENTRY *PFN_glWindowPos4dvMESA)( const GLdouble *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos4fMESA)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
typedef void       (GLAPIENTRY *PFN_glWindowPos4fvMESA)( const GLfloat *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos4iMESA)( GLint x, GLint y, GLint z, GLint w );
typedef void       (GLAPIENTRY *PFN_glWindowPos4ivMESA)( const GLint *v );
typedef void       (GLAPIENTRY *PFN_glWindowPos4sMESA)( GLshort x, GLshort y, GLshort z, GLshort w );
typedef void       (GLAPIENTRY *PFN_glWindowPos4svMESA)( const GLshort *v );
typedef void       (GLAPIENTRY *PFN_glWindowRectanglesEXT)( GLenum mode, GLsizei count, const GLint *box );
typedef void       (GLAPIENTRY *PFN_glWriteMaskEXT)( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW );
typedef void *     (GLAPIENTRY *PFN_wglAllocateMemoryNV)( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority );
typedef BOOL       (GLAPIENTRY *PFN_wglBindTexImageARB)( HPBUFFERARB hPbuffer, int iBuffer );
typedef BOOL       (GLAPIENTRY *PFN_wglChoosePixelFormatARB)( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats );
typedef HGLRC      (GLAPIENTRY *PFN_wglCreateContextAttribsARB)( HDC hDC, HGLRC hShareContext, const int *attribList );
typedef HPBUFFERARB (GLAPIENTRY *PFN_wglCreatePbufferARB)( HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList );
typedef BOOL       (GLAPIENTRY *PFN_wglDestroyPbufferARB)( HPBUFFERARB hPbuffer );
typedef void       (GLAPIENTRY *PFN_wglFreeMemoryNV)( void *pointer );
typedef HDC        (GLAPIENTRY *PFN_wglGetCurrentReadDCARB)(void);
typedef const char * (GLAPIENTRY *PFN_wglGetExtensionsStringARB)( HDC hdc );
typedef const char * (GLAPIENTRY *PFN_wglGetExtensionsStringEXT)(void);
typedef HDC        (GLAPIENTRY *PFN_wglGetPbufferDCARB)( HPBUFFERARB hPbuffer );
typedef BOOL       (GLAPIENTRY *PFN_wglGetPixelFormatAttribfvARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues );
typedef BOOL       (GLAPIENTRY *PFN_wglGetPixelFormatAttribivARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues );
typedef int        (GLAPIENTRY *PFN_wglGetSwapIntervalEXT)(void);
typedef BOOL       (GLAPIENTRY *PFN_wglMakeContextCurrentARB)( HDC hDrawDC, HDC hReadDC, HGLRC hglrc );
typedef BOOL       (GLAPIENTRY *PFN_wglQueryCurrentRendererIntegerWINE)( GLenum attribute, GLuint *value );
typedef const GLchar * (GLAPIENTRY *PFN_wglQueryCurrentRendererStringWINE)( GLenum attribute );
typedef BOOL       (GLAPIENTRY *PFN_wglQueryPbufferARB)( HPBUFFERARB hPbuffer, int iAttribute, int *piValue );
typedef BOOL       (GLAPIENTRY *PFN_wglQueryRendererIntegerWINE)( HDC dc, GLint renderer, GLenum attribute, GLuint *value );
typedef const GLchar * (GLAPIENTRY *PFN_wglQueryRendererStringWINE)( HDC dc, GLint renderer, GLenum attribute );
typedef int        (GLAPIENTRY *PFN_wglReleasePbufferDCARB)( HPBUFFERARB hPbuffer, HDC hDC );
typedef BOOL       (GLAPIENTRY *PFN_wglReleaseTexImageARB)( HPBUFFERARB hPbuffer, int iBuffer );
typedef BOOL       (GLAPIENTRY *PFN_wglSetPbufferAttribARB)( HPBUFFERARB hPbuffer, const int *piAttribList );
typedef BOOL       (GLAPIENTRY *PFN_wglSetPixelFormatWINE)( HDC hdc, int format );
typedef BOOL       (GLAPIENTRY *PFN_wglSwapIntervalEXT)( int interval );

#define ALL_WGL_FUNCS \
    USE_GL_FUNC(wglChoosePixelFormat) \
    USE_GL_FUNC(wglCopyContext) \
    USE_GL_FUNC(wglCreateContext) \
    USE_GL_FUNC(wglCreateLayerContext) \
    USE_GL_FUNC(wglDeleteContext) \
    USE_GL_FUNC(wglDescribeLayerPlane) \
    USE_GL_FUNC(wglDescribePixelFormat) \
    USE_GL_FUNC(wglGetCurrentContext) \
    USE_GL_FUNC(wglGetCurrentDC) \
    USE_GL_FUNC(wglGetDefaultProcAddress) \
    USE_GL_FUNC(wglGetLayerPaletteEntries) \
    USE_GL_FUNC(wglGetPixelFormat) \
    USE_GL_FUNC(wglGetProcAddress) \
    USE_GL_FUNC(wglMakeCurrent) \
    USE_GL_FUNC(wglRealizeLayerPalette) \
    USE_GL_FUNC(wglSetLayerPaletteEntries) \
    USE_GL_FUNC(wglSetPixelFormat) \
    USE_GL_FUNC(wglShareLists) \
    USE_GL_FUNC(wglSwapBuffers) \
    USE_GL_FUNC(wglSwapLayerBuffers) \
    USE_GL_FUNC(wglUseFontBitmapsA) \
    USE_GL_FUNC(wglUseFontBitmapsW) \
    USE_GL_FUNC(wglUseFontOutlinesA) \
    USE_GL_FUNC(wglUseFontOutlinesW)

#define ALL_WGL_EXT_FUNCS \
    USE_GL_FUNC(wglAllocateMemoryNV) \
    USE_GL_FUNC(wglBindTexImageARB) \
    USE_GL_FUNC(wglChoosePixelFormatARB) \
    USE_GL_FUNC(wglCreateContextAttribsARB) \
    USE_GL_FUNC(wglCreatePbufferARB) \
    USE_GL_FUNC(wglDestroyPbufferARB) \
    USE_GL_FUNC(wglFreeMemoryNV) \
    USE_GL_FUNC(wglGetCurrentReadDCARB) \
    USE_GL_FUNC(wglGetExtensionsStringARB) \
    USE_GL_FUNC(wglGetExtensionsStringEXT) \
    USE_GL_FUNC(wglGetPbufferDCARB) \
    USE_GL_FUNC(wglGetPixelFormatAttribfvARB) \
    USE_GL_FUNC(wglGetPixelFormatAttribivARB) \
    USE_GL_FUNC(wglGetSwapIntervalEXT) \
    USE_GL_FUNC(wglMakeContextCurrentARB) \
    USE_GL_FUNC(wglQueryCurrentRendererIntegerWINE) \
    USE_GL_FUNC(wglQueryCurrentRendererStringWINE) \
    USE_GL_FUNC(wglQueryPbufferARB) \
    USE_GL_FUNC(wglQueryRendererIntegerWINE) \
    USE_GL_FUNC(wglQueryRendererStringWINE) \
    USE_GL_FUNC(wglReleasePbufferDCARB) \
    USE_GL_FUNC(wglReleaseTexImageARB) \
    USE_GL_FUNC(wglSetPbufferAttribARB) \
    USE_GL_FUNC(wglSetPixelFormatWINE) \
    USE_GL_FUNC(wglSwapIntervalEXT)

#define ALL_EGL_FUNCS \
    USE_GL_FUNC(eglBindAPI) \
    USE_GL_FUNC(eglBindTexImage) \
    USE_GL_FUNC(eglChooseConfig) \
    USE_GL_FUNC(eglClientWaitSync) \
    USE_GL_FUNC(eglCopyBuffers) \
    USE_GL_FUNC(eglCreateContext) \
    USE_GL_FUNC(eglCreateImage) \
    USE_GL_FUNC(eglCreatePbufferFromClientBuffer) \
    USE_GL_FUNC(eglCreatePbufferSurface) \
    USE_GL_FUNC(eglCreatePixmapSurface) \
    USE_GL_FUNC(eglCreatePlatformPixmapSurface) \
    USE_GL_FUNC(eglCreatePlatformWindowSurface) \
    USE_GL_FUNC(eglCreateSync) \
    USE_GL_FUNC(eglCreateWindowSurface) \
    USE_GL_FUNC(eglDestroyContext) \
    USE_GL_FUNC(eglDestroyImage) \
    USE_GL_FUNC(eglDestroySurface) \
    USE_GL_FUNC(eglDestroySync) \
    USE_GL_FUNC(eglGetConfigAttrib) \
    USE_GL_FUNC(eglGetConfigs) \
    USE_GL_FUNC(eglGetCurrentContext) \
    USE_GL_FUNC(eglGetCurrentDisplay) \
    USE_GL_FUNC(eglGetCurrentSurface) \
    USE_GL_FUNC(eglGetDisplay) \
    USE_GL_FUNC(eglGetError) \
    USE_GL_FUNC(eglGetPlatformDisplay) \
    USE_GL_FUNC(eglGetProcAddress) \
    USE_GL_FUNC(eglGetSyncAttrib) \
    USE_GL_FUNC(eglInitialize) \
    USE_GL_FUNC(eglMakeCurrent) \
    USE_GL_FUNC(eglQueryAPI) \
    USE_GL_FUNC(eglQueryContext) \
    USE_GL_FUNC(eglQueryString) \
    USE_GL_FUNC(eglQuerySurface) \
    USE_GL_FUNC(eglReleaseTexImage) \
    USE_GL_FUNC(eglReleaseThread) \
    USE_GL_FUNC(eglSurfaceAttrib) \
    USE_GL_FUNC(eglSwapBuffers) \
    USE_GL_FUNC(eglSwapInterval) \
    USE_GL_FUNC(eglTerminate) \
    USE_GL_FUNC(eglWaitClient) \
    USE_GL_FUNC(eglWaitGL) \
    USE_GL_FUNC(eglWaitNative) \
    USE_GL_FUNC(eglWaitSync)

#define ALL_EGL_EXT_FUNCS

#define ALL_GL_FUNCS \
    USE_GL_FUNC(glAccum) \
    USE_GL_FUNC(glAlphaFunc) \
    USE_GL_FUNC(glAreTexturesResident) \
    USE_GL_FUNC(glArrayElement) \
    USE_GL_FUNC(glBegin) \
    USE_GL_FUNC(glBindTexture) \
    USE_GL_FUNC(glBitmap) \
    USE_GL_FUNC(glBlendFunc) \
    USE_GL_FUNC(glCallList) \
    USE_GL_FUNC(glCallLists) \
    USE_GL_FUNC(glClear) \
    USE_GL_FUNC(glClearAccum) \
    USE_GL_FUNC(glClearColor) \
    USE_GL_FUNC(glClearDepth) \
    USE_GL_FUNC(glClearIndex) \
    USE_GL_FUNC(glClearStencil) \
    USE_GL_FUNC(glClipPlane) \
    USE_GL_FUNC(glColor3b) \
    USE_GL_FUNC(glColor3bv) \
    USE_GL_FUNC(glColor3d) \
    USE_GL_FUNC(glColor3dv) \
    USE_GL_FUNC(glColor3f) \
    USE_GL_FUNC(glColor3fv) \
    USE_GL_FUNC(glColor3i) \
    USE_GL_FUNC(glColor3iv) \
    USE_GL_FUNC(glColor3s) \
    USE_GL_FUNC(glColor3sv) \
    USE_GL_FUNC(glColor3ub) \
    USE_GL_FUNC(glColor3ubv) \
    USE_GL_FUNC(glColor3ui) \
    USE_GL_FUNC(glColor3uiv) \
    USE_GL_FUNC(glColor3us) \
    USE_GL_FUNC(glColor3usv) \
    USE_GL_FUNC(glColor4b) \
    USE_GL_FUNC(glColor4bv) \
    USE_GL_FUNC(glColor4d) \
    USE_GL_FUNC(glColor4dv) \
    USE_GL_FUNC(glColor4f) \
    USE_GL_FUNC(glColor4fv) \
    USE_GL_FUNC(glColor4i) \
    USE_GL_FUNC(glColor4iv) \
    USE_GL_FUNC(glColor4s) \
    USE_GL_FUNC(glColor4sv) \
    USE_GL_FUNC(glColor4ub) \
    USE_GL_FUNC(glColor4ubv) \
    USE_GL_FUNC(glColor4ui) \
    USE_GL_FUNC(glColor4uiv) \
    USE_GL_FUNC(glColor4us) \
    USE_GL_FUNC(glColor4usv) \
    USE_GL_FUNC(glColorMask) \
    USE_GL_FUNC(glColorMaterial) \
    USE_GL_FUNC(glColorPointer) \
    USE_GL_FUNC(glCopyPixels) \
    USE_GL_FUNC(glCopyTexImage1D) \
    USE_GL_FUNC(glCopyTexImage2D) \
    USE_GL_FUNC(glCopyTexSubImage1D) \
    USE_GL_FUNC(glCopyTexSubImage2D) \
    USE_GL_FUNC(glCullFace) \
    USE_GL_FUNC(glDeleteLists) \
    USE_GL_FUNC(glDeleteTextures) \
    USE_GL_FUNC(glDepthFunc) \
    USE_GL_FUNC(glDepthMask) \
    USE_GL_FUNC(glDepthRange) \
    USE_GL_FUNC(glDisable) \
    USE_GL_FUNC(glDisableClientState) \
    USE_GL_FUNC(glDrawArrays) \
    USE_GL_FUNC(glDrawBuffer) \
    USE_GL_FUNC(glDrawElements) \
    USE_GL_FUNC(glDrawPixels) \
    USE_GL_FUNC(glEdgeFlag) \
    USE_GL_FUNC(glEdgeFlagPointer) \
    USE_GL_FUNC(glEdgeFlagv) \
    USE_GL_FUNC(glEnable) \
    USE_GL_FUNC(glEnableClientState) \
    USE_GL_FUNC(glEnd) \
    USE_GL_FUNC(glEndList) \
    USE_GL_FUNC(glEvalCoord1d) \
    USE_GL_FUNC(glEvalCoord1dv) \
    USE_GL_FUNC(glEvalCoord1f) \
    USE_GL_FUNC(glEvalCoord1fv) \
    USE_GL_FUNC(glEvalCoord2d) \
    USE_GL_FUNC(glEvalCoord2dv) \
    USE_GL_FUNC(glEvalCoord2f) \
    USE_GL_FUNC(glEvalCoord2fv) \
    USE_GL_FUNC(glEvalMesh1) \
    USE_GL_FUNC(glEvalMesh2) \
    USE_GL_FUNC(glEvalPoint1) \
    USE_GL_FUNC(glEvalPoint2) \
    USE_GL_FUNC(glFeedbackBuffer) \
    USE_GL_FUNC(glFinish) \
    USE_GL_FUNC(glFlush) \
    USE_GL_FUNC(glFogf) \
    USE_GL_FUNC(glFogfv) \
    USE_GL_FUNC(glFogi) \
    USE_GL_FUNC(glFogiv) \
    USE_GL_FUNC(glFrontFace) \
    USE_GL_FUNC(glFrustum) \
    USE_GL_FUNC(glGenLists) \
    USE_GL_FUNC(glGenTextures) \
    USE_GL_FUNC(glGetBooleanv) \
    USE_GL_FUNC(glGetClipPlane) \
    USE_GL_FUNC(glGetDoublev) \
    USE_GL_FUNC(glGetError) \
    USE_GL_FUNC(glGetFloatv) \
    USE_GL_FUNC(glGetIntegerv) \
    USE_GL_FUNC(glGetLightfv) \
    USE_GL_FUNC(glGetLightiv) \
    USE_GL_FUNC(glGetMapdv) \
    USE_GL_FUNC(glGetMapfv) \
    USE_GL_FUNC(glGetMapiv) \
    USE_GL_FUNC(glGetMaterialfv) \
    USE_GL_FUNC(glGetMaterialiv) \
    USE_GL_FUNC(glGetPixelMapfv) \
    USE_GL_FUNC(glGetPixelMapuiv) \
    USE_GL_FUNC(glGetPixelMapusv) \
    USE_GL_FUNC(glGetPointerv) \
    USE_GL_FUNC(glGetPolygonStipple) \
    USE_GL_FUNC(glGetString) \
    USE_GL_FUNC(glGetTexEnvfv) \
    USE_GL_FUNC(glGetTexEnviv) \
    USE_GL_FUNC(glGetTexGendv) \
    USE_GL_FUNC(glGetTexGenfv) \
    USE_GL_FUNC(glGetTexGeniv) \
    USE_GL_FUNC(glGetTexImage) \
    USE_GL_FUNC(glGetTexLevelParameterfv) \
    USE_GL_FUNC(glGetTexLevelParameteriv) \
    USE_GL_FUNC(glGetTexParameterfv) \
    USE_GL_FUNC(glGetTexParameteriv) \
    USE_GL_FUNC(glHint) \
    USE_GL_FUNC(glIndexMask) \
    USE_GL_FUNC(glIndexPointer) \
    USE_GL_FUNC(glIndexd) \
    USE_GL_FUNC(glIndexdv) \
    USE_GL_FUNC(glIndexf) \
    USE_GL_FUNC(glIndexfv) \
    USE_GL_FUNC(glIndexi) \
    USE_GL_FUNC(glIndexiv) \
    USE_GL_FUNC(glIndexs) \
    USE_GL_FUNC(glIndexsv) \
    USE_GL_FUNC(glIndexub) \
    USE_GL_FUNC(glIndexubv) \
    USE_GL_FUNC(glInitNames) \
    USE_GL_FUNC(glInterleavedArrays) \
    USE_GL_FUNC(glIsEnabled) \
    USE_GL_FUNC(glIsList) \
    USE_GL_FUNC(glIsTexture) \
    USE_GL_FUNC(glLightModelf) \
    USE_GL_FUNC(glLightModelfv) \
    USE_GL_FUNC(glLightModeli) \
    USE_GL_FUNC(glLightModeliv) \
    USE_GL_FUNC(glLightf) \
    USE_GL_FUNC(glLightfv) \
    USE_GL_FUNC(glLighti) \
    USE_GL_FUNC(glLightiv) \
    USE_GL_FUNC(glLineStipple) \
    USE_GL_FUNC(glLineWidth) \
    USE_GL_FUNC(glListBase) \
    USE_GL_FUNC(glLoadIdentity) \
    USE_GL_FUNC(glLoadMatrixd) \
    USE_GL_FUNC(glLoadMatrixf) \
    USE_GL_FUNC(glLoadName) \
    USE_GL_FUNC(glLogicOp) \
    USE_GL_FUNC(glMap1d) \
    USE_GL_FUNC(glMap1f) \
    USE_GL_FUNC(glMap2d) \
    USE_GL_FUNC(glMap2f) \
    USE_GL_FUNC(glMapGrid1d) \
    USE_GL_FUNC(glMapGrid1f) \
    USE_GL_FUNC(glMapGrid2d) \
    USE_GL_FUNC(glMapGrid2f) \
    USE_GL_FUNC(glMaterialf) \
    USE_GL_FUNC(glMaterialfv) \
    USE_GL_FUNC(glMateriali) \
    USE_GL_FUNC(glMaterialiv) \
    USE_GL_FUNC(glMatrixMode) \
    USE_GL_FUNC(glMultMatrixd) \
    USE_GL_FUNC(glMultMatrixf) \
    USE_GL_FUNC(glNewList) \
    USE_GL_FUNC(glNormal3b) \
    USE_GL_FUNC(glNormal3bv) \
    USE_GL_FUNC(glNormal3d) \
    USE_GL_FUNC(glNormal3dv) \
    USE_GL_FUNC(glNormal3f) \
    USE_GL_FUNC(glNormal3fv) \
    USE_GL_FUNC(glNormal3i) \
    USE_GL_FUNC(glNormal3iv) \
    USE_GL_FUNC(glNormal3s) \
    USE_GL_FUNC(glNormal3sv) \
    USE_GL_FUNC(glNormalPointer) \
    USE_GL_FUNC(glOrtho) \
    USE_GL_FUNC(glPassThrough) \
    USE_GL_FUNC(glPixelMapfv) \
    USE_GL_FUNC(glPixelMapuiv) \
    USE_GL_FUNC(glPixelMapusv) \
    USE_GL_FUNC(glPixelStoref) \
    USE_GL_FUNC(glPixelStorei) \
    USE_GL_FUNC(glPixelTransferf) \
    USE_GL_FUNC(glPixelTransferi) \
    USE_GL_FUNC(glPixelZoom) \
    USE_GL_FUNC(glPointSize) \
    USE_GL_FUNC(glPolygonMode) \
    USE_GL_FUNC(glPolygonOffset) \
    USE_GL_FUNC(glPolygonStipple) \
    USE_GL_FUNC(glPopAttrib) \
    USE_GL_FUNC(glPopClientAttrib) \
    USE_GL_FUNC(glPopMatrix) \
    USE_GL_FUNC(glPopName) \
    USE_GL_FUNC(glPrioritizeTextures) \
    USE_GL_FUNC(glPushAttrib) \
    USE_GL_FUNC(glPushClientAttrib) \
    USE_GL_FUNC(glPushMatrix) \
    USE_GL_FUNC(glPushName) \
    USE_GL_FUNC(glRasterPos2d) \
    USE_GL_FUNC(glRasterPos2dv) \
    USE_GL_FUNC(glRasterPos2f) \
    USE_GL_FUNC(glRasterPos2fv) \
    USE_GL_FUNC(glRasterPos2i) \
    USE_GL_FUNC(glRasterPos2iv) \
    USE_GL_FUNC(glRasterPos2s) \
    USE_GL_FUNC(glRasterPos2sv) \
    USE_GL_FUNC(glRasterPos3d) \
    USE_GL_FUNC(glRasterPos3dv) \
    USE_GL_FUNC(glRasterPos3f) \
    USE_GL_FUNC(glRasterPos3fv) \
    USE_GL_FUNC(glRasterPos3i) \
    USE_GL_FUNC(glRasterPos3iv) \
    USE_GL_FUNC(glRasterPos3s) \
    USE_GL_FUNC(glRasterPos3sv) \
    USE_GL_FUNC(glRasterPos4d) \
    USE_GL_FUNC(glRasterPos4dv) \
    USE_GL_FUNC(glRasterPos4f) \
    USE_GL_FUNC(glRasterPos4fv) \
    USE_GL_FUNC(glRasterPos4i) \
    USE_GL_FUNC(glRasterPos4iv) \
    USE_GL_FUNC(glRasterPos4s) \
    USE_GL_FUNC(glRasterPos4sv) \
    USE_GL_FUNC(glReadBuffer) \
    USE_GL_FUNC(glReadPixels) \
    USE_GL_FUNC(glRectd) \
    USE_GL_FUNC(glRectdv) \
    USE_GL_FUNC(glRectf) \
    USE_GL_FUNC(glRectfv) \
    USE_GL_FUNC(glRecti) \
    USE_GL_FUNC(glRectiv) \
    USE_GL_FUNC(glRects) \
    USE_GL_FUNC(glRectsv) \
    USE_GL_FUNC(glRenderMode) \
    USE_GL_FUNC(glRotated) \
    USE_GL_FUNC(glRotatef) \
    USE_GL_FUNC(glScaled) \
    USE_GL_FUNC(glScalef) \
    USE_GL_FUNC(glScissor) \
    USE_GL_FUNC(glSelectBuffer) \
    USE_GL_FUNC(glShadeModel) \
    USE_GL_FUNC(glStencilFunc) \
    USE_GL_FUNC(glStencilMask) \
    USE_GL_FUNC(glStencilOp) \
    USE_GL_FUNC(glTexCoord1d) \
    USE_GL_FUNC(glTexCoord1dv) \
    USE_GL_FUNC(glTexCoord1f) \
    USE_GL_FUNC(glTexCoord1fv) \
    USE_GL_FUNC(glTexCoord1i) \
    USE_GL_FUNC(glTexCoord1iv) \
    USE_GL_FUNC(glTexCoord1s) \
    USE_GL_FUNC(glTexCoord1sv) \
    USE_GL_FUNC(glTexCoord2d) \
    USE_GL_FUNC(glTexCoord2dv) \
    USE_GL_FUNC(glTexCoord2f) \
    USE_GL_FUNC(glTexCoord2fv) \
    USE_GL_FUNC(glTexCoord2i) \
    USE_GL_FUNC(glTexCoord2iv) \
    USE_GL_FUNC(glTexCoord2s) \
    USE_GL_FUNC(glTexCoord2sv) \
    USE_GL_FUNC(glTexCoord3d) \
    USE_GL_FUNC(glTexCoord3dv) \
    USE_GL_FUNC(glTexCoord3f) \
    USE_GL_FUNC(glTexCoord3fv) \
    USE_GL_FUNC(glTexCoord3i) \
    USE_GL_FUNC(glTexCoord3iv) \
    USE_GL_FUNC(glTexCoord3s) \
    USE_GL_FUNC(glTexCoord3sv) \
    USE_GL_FUNC(glTexCoord4d) \
    USE_GL_FUNC(glTexCoord4dv) \
    USE_GL_FUNC(glTexCoord4f) \
    USE_GL_FUNC(glTexCoord4fv) \
    USE_GL_FUNC(glTexCoord4i) \
    USE_GL_FUNC(glTexCoord4iv) \
    USE_GL_FUNC(glTexCoord4s) \
    USE_GL_FUNC(glTexCoord4sv) \
    USE_GL_FUNC(glTexCoordPointer) \
    USE_GL_FUNC(glTexEnvf) \
    USE_GL_FUNC(glTexEnvfv) \
    USE_GL_FUNC(glTexEnvi) \
    USE_GL_FUNC(glTexEnviv) \
    USE_GL_FUNC(glTexGend) \
    USE_GL_FUNC(glTexGendv) \
    USE_GL_FUNC(glTexGenf) \
    USE_GL_FUNC(glTexGenfv) \
    USE_GL_FUNC(glTexGeni) \
    USE_GL_FUNC(glTexGeniv) \
    USE_GL_FUNC(glTexImage1D) \
    USE_GL_FUNC(glTexImage2D) \
    USE_GL_FUNC(glTexParameterf) \
    USE_GL_FUNC(glTexParameterfv) \
    USE_GL_FUNC(glTexParameteri) \
    USE_GL_FUNC(glTexParameteriv) \
    USE_GL_FUNC(glTexSubImage1D) \
    USE_GL_FUNC(glTexSubImage2D) \
    USE_GL_FUNC(glTranslated) \
    USE_GL_FUNC(glTranslatef) \
    USE_GL_FUNC(glVertex2d) \
    USE_GL_FUNC(glVertex2dv) \
    USE_GL_FUNC(glVertex2f) \
    USE_GL_FUNC(glVertex2fv) \
    USE_GL_FUNC(glVertex2i) \
    USE_GL_FUNC(glVertex2iv) \
    USE_GL_FUNC(glVertex2s) \
    USE_GL_FUNC(glVertex2sv) \
    USE_GL_FUNC(glVertex3d) \
    USE_GL_FUNC(glVertex3dv) \
    USE_GL_FUNC(glVertex3f) \
    USE_GL_FUNC(glVertex3fv) \
    USE_GL_FUNC(glVertex3i) \
    USE_GL_FUNC(glVertex3iv) \
    USE_GL_FUNC(glVertex3s) \
    USE_GL_FUNC(glVertex3sv) \
    USE_GL_FUNC(glVertex4d) \
    USE_GL_FUNC(glVertex4dv) \
    USE_GL_FUNC(glVertex4f) \
    USE_GL_FUNC(glVertex4fv) \
    USE_GL_FUNC(glVertex4i) \
    USE_GL_FUNC(glVertex4iv) \
    USE_GL_FUNC(glVertex4s) \
    USE_GL_FUNC(glVertex4sv) \
    USE_GL_FUNC(glVertexPointer) \
    USE_GL_FUNC(glViewport)

#define ALL_GL_EXT_FUNCS \
    USE_GL_FUNC(glAccumxOES) \
    USE_GL_FUNC(glAcquireKeyedMutexWin32EXT) \
    USE_GL_FUNC(glActiveProgramEXT) \
    USE_GL_FUNC(glActiveShaderProgram) \
    USE_GL_FUNC(glActiveStencilFaceEXT) \
    USE_GL_FUNC(glActiveTexture) \
    USE_GL_FUNC(glActiveTextureARB) \
    USE_GL_FUNC(glActiveVaryingNV) \
    USE_GL_FUNC(glAlphaFragmentOp1ATI) \
    USE_GL_FUNC(glAlphaFragmentOp2ATI) \
    USE_GL_FUNC(glAlphaFragmentOp3ATI) \
    USE_GL_FUNC(glAlphaFuncxOES) \
    USE_GL_FUNC(glAlphaToCoverageDitherControlNV) \
    USE_GL_FUNC(glApplyFramebufferAttachmentCMAAINTEL) \
    USE_GL_FUNC(glApplyTextureEXT) \
    USE_GL_FUNC(glAreProgramsResidentNV) \
    USE_GL_FUNC(glAreTexturesResidentEXT) \
    USE_GL_FUNC(glArrayElementEXT) \
    USE_GL_FUNC(glArrayObjectATI) \
    USE_GL_FUNC(glAsyncCopyBufferSubDataNVX) \
    USE_GL_FUNC(glAsyncCopyImageSubDataNVX) \
    USE_GL_FUNC(glAsyncMarkerSGIX) \
    USE_GL_FUNC(glAttachObjectARB) \
    USE_GL_FUNC(glAttachShader) \
    USE_GL_FUNC(glBeginConditionalRender) \
    USE_GL_FUNC(glBeginConditionalRenderNV) \
    USE_GL_FUNC(glBeginConditionalRenderNVX) \
    USE_GL_FUNC(glBeginFragmentShaderATI) \
    USE_GL_FUNC(glBeginOcclusionQueryNV) \
    USE_GL_FUNC(glBeginPerfMonitorAMD) \
    USE_GL_FUNC(glBeginPerfQueryINTEL) \
    USE_GL_FUNC(glBeginQuery) \
    USE_GL_FUNC(glBeginQueryARB) \
    USE_GL_FUNC(glBeginQueryIndexed) \
    USE_GL_FUNC(glBeginTransformFeedback) \
    USE_GL_FUNC(glBeginTransformFeedbackEXT) \
    USE_GL_FUNC(glBeginTransformFeedbackNV) \
    USE_GL_FUNC(glBeginVertexShaderEXT) \
    USE_GL_FUNC(glBeginVideoCaptureNV) \
    USE_GL_FUNC(glBindAttribLocation) \
    USE_GL_FUNC(glBindAttribLocationARB) \
    USE_GL_FUNC(glBindBuffer) \
    USE_GL_FUNC(glBindBufferARB) \
    USE_GL_FUNC(glBindBufferBase) \
    USE_GL_FUNC(glBindBufferBaseEXT) \
    USE_GL_FUNC(glBindBufferBaseNV) \
    USE_GL_FUNC(glBindBufferOffsetEXT) \
    USE_GL_FUNC(glBindBufferOffsetNV) \
    USE_GL_FUNC(glBindBufferRange) \
    USE_GL_FUNC(glBindBufferRangeEXT) \
    USE_GL_FUNC(glBindBufferRangeNV) \
    USE_GL_FUNC(glBindBuffersBase) \
    USE_GL_FUNC(glBindBuffersRange) \
    USE_GL_FUNC(glBindFragDataLocation) \
    USE_GL_FUNC(glBindFragDataLocationEXT) \
    USE_GL_FUNC(glBindFragDataLocationIndexed) \
    USE_GL_FUNC(glBindFragmentShaderATI) \
    USE_GL_FUNC(glBindFramebuffer) \
    USE_GL_FUNC(glBindFramebufferEXT) \
    USE_GL_FUNC(glBindImageTexture) \
    USE_GL_FUNC(glBindImageTextureEXT) \
    USE_GL_FUNC(glBindImageTextures) \
    USE_GL_FUNC(glBindLightParameterEXT) \
    USE_GL_FUNC(glBindMaterialParameterEXT) \
    USE_GL_FUNC(glBindMultiTextureEXT) \
    USE_GL_FUNC(glBindParameterEXT) \
    USE_GL_FUNC(glBindProgramARB) \
    USE_GL_FUNC(glBindProgramNV) \
    USE_GL_FUNC(glBindProgramPipeline) \
    USE_GL_FUNC(glBindRenderbuffer) \
    USE_GL_FUNC(glBindRenderbufferEXT) \
    USE_GL_FUNC(glBindSampler) \
    USE_GL_FUNC(glBindSamplers) \
    USE_GL_FUNC(glBindShadingRateImageNV) \
    USE_GL_FUNC(glBindTexGenParameterEXT) \
    USE_GL_FUNC(glBindTextureEXT) \
    USE_GL_FUNC(glBindTextureUnit) \
    USE_GL_FUNC(glBindTextureUnitParameterEXT) \
    USE_GL_FUNC(glBindTextures) \
    USE_GL_FUNC(glBindTransformFeedback) \
    USE_GL_FUNC(glBindTransformFeedbackNV) \
    USE_GL_FUNC(glBindVertexArray) \
    USE_GL_FUNC(glBindVertexArrayAPPLE) \
    USE_GL_FUNC(glBindVertexBuffer) \
    USE_GL_FUNC(glBindVertexBuffers) \
    USE_GL_FUNC(glBindVertexShaderEXT) \
    USE_GL_FUNC(glBindVideoCaptureStreamBufferNV) \
    USE_GL_FUNC(glBindVideoCaptureStreamTextureNV) \
    USE_GL_FUNC(glBinormal3bEXT) \
    USE_GL_FUNC(glBinormal3bvEXT) \
    USE_GL_FUNC(glBinormal3dEXT) \
    USE_GL_FUNC(glBinormal3dvEXT) \
    USE_GL_FUNC(glBinormal3fEXT) \
    USE_GL_FUNC(glBinormal3fvEXT) \
    USE_GL_FUNC(glBinormal3iEXT) \
    USE_GL_FUNC(glBinormal3ivEXT) \
    USE_GL_FUNC(glBinormal3sEXT) \
    USE_GL_FUNC(glBinormal3svEXT) \
    USE_GL_FUNC(glBinormalPointerEXT) \
    USE_GL_FUNC(glBitmapxOES) \
    USE_GL_FUNC(glBlendBarrierKHR) \
    USE_GL_FUNC(glBlendBarrierNV) \
    USE_GL_FUNC(glBlendColor) \
    USE_GL_FUNC(glBlendColorEXT) \
    USE_GL_FUNC(glBlendColorxOES) \
    USE_GL_FUNC(glBlendEquation) \
    USE_GL_FUNC(glBlendEquationEXT) \
    USE_GL_FUNC(glBlendEquationIndexedAMD) \
    USE_GL_FUNC(glBlendEquationSeparate) \
    USE_GL_FUNC(glBlendEquationSeparateEXT) \
    USE_GL_FUNC(glBlendEquationSeparateIndexedAMD) \
    USE_GL_FUNC(glBlendEquationSeparatei) \
    USE_GL_FUNC(glBlendEquationSeparateiARB) \
    USE_GL_FUNC(glBlendEquationi) \
    USE_GL_FUNC(glBlendEquationiARB) \
    USE_GL_FUNC(glBlendFuncIndexedAMD) \
    USE_GL_FUNC(glBlendFuncSeparate) \
    USE_GL_FUNC(glBlendFuncSeparateEXT) \
    USE_GL_FUNC(glBlendFuncSeparateINGR) \
    USE_GL_FUNC(glBlendFuncSeparateIndexedAMD) \
    USE_GL_FUNC(glBlendFuncSeparatei) \
    USE_GL_FUNC(glBlendFuncSeparateiARB) \
    USE_GL_FUNC(glBlendFunci) \
    USE_GL_FUNC(glBlendFunciARB) \
    USE_GL_FUNC(glBlendParameteriNV) \
    USE_GL_FUNC(glBlitFramebuffer) \
    USE_GL_FUNC(glBlitFramebufferEXT) \
    USE_GL_FUNC(glBlitNamedFramebuffer) \
    USE_GL_FUNC(glBufferAddressRangeNV) \
    USE_GL_FUNC(glBufferAttachMemoryNV) \
    USE_GL_FUNC(glBufferData) \
    USE_GL_FUNC(glBufferDataARB) \
    USE_GL_FUNC(glBufferPageCommitmentARB) \
    USE_GL_FUNC(glBufferParameteriAPPLE) \
    USE_GL_FUNC(glBufferRegionEnabled) \
    USE_GL_FUNC(glBufferStorage) \
    USE_GL_FUNC(glBufferStorageExternalEXT) \
    USE_GL_FUNC(glBufferStorageMemEXT) \
    USE_GL_FUNC(glBufferSubData) \
    USE_GL_FUNC(glBufferSubDataARB) \
    USE_GL_FUNC(glCallCommandListNV) \
    USE_GL_FUNC(glCheckFramebufferStatus) \
    USE_GL_FUNC(glCheckFramebufferStatusEXT) \
    USE_GL_FUNC(glCheckNamedFramebufferStatus) \
    USE_GL_FUNC(glCheckNamedFramebufferStatusEXT) \
    USE_GL_FUNC(glClampColor) \
    USE_GL_FUNC(glClampColorARB) \
    USE_GL_FUNC(glClearAccumxOES) \
    USE_GL_FUNC(glClearBufferData) \
    USE_GL_FUNC(glClearBufferSubData) \
    USE_GL_FUNC(glClearBufferfi) \
    USE_GL_FUNC(glClearBufferfv) \
    USE_GL_FUNC(glClearBufferiv) \
    USE_GL_FUNC(glClearBufferuiv) \
    USE_GL_FUNC(glClearColorIiEXT) \
    USE_GL_FUNC(glClearColorIuiEXT) \
    USE_GL_FUNC(glClearColorxOES) \
    USE_GL_FUNC(glClearDepthdNV) \
    USE_GL_FUNC(glClearDepthf) \
    USE_GL_FUNC(glClearDepthfOES) \
    USE_GL_FUNC(glClearDepthxOES) \
    USE_GL_FUNC(glClearNamedBufferData) \
    USE_GL_FUNC(glClearNamedBufferDataEXT) \
    USE_GL_FUNC(glClearNamedBufferSubData) \
    USE_GL_FUNC(glClearNamedBufferSubDataEXT) \
    USE_GL_FUNC(glClearNamedFramebufferfi) \
    USE_GL_FUNC(glClearNamedFramebufferfv) \
    USE_GL_FUNC(glClearNamedFramebufferiv) \
    USE_GL_FUNC(glClearNamedFramebufferuiv) \
    USE_GL_FUNC(glClearTexImage) \
    USE_GL_FUNC(glClearTexSubImage) \
    USE_GL_FUNC(glClientActiveTexture) \
    USE_GL_FUNC(glClientActiveTextureARB) \
    USE_GL_FUNC(glClientActiveVertexStreamATI) \
    USE_GL_FUNC(glClientAttribDefaultEXT) \
    USE_GL_FUNC(glClientWaitSemaphoreui64NVX) \
    USE_GL_FUNC(glClientWaitSync) \
    USE_GL_FUNC(glClipControl) \
    USE_GL_FUNC(glClipPlanefOES) \
    USE_GL_FUNC(glClipPlanexOES) \
    USE_GL_FUNC(glColor3fVertex3fSUN) \
    USE_GL_FUNC(glColor3fVertex3fvSUN) \
    USE_GL_FUNC(glColor3hNV) \
    USE_GL_FUNC(glColor3hvNV) \
    USE_GL_FUNC(glColor3xOES) \
    USE_GL_FUNC(glColor3xvOES) \
    USE_GL_FUNC(glColor4fNormal3fVertex3fSUN) \
    USE_GL_FUNC(glColor4fNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glColor4hNV) \
    USE_GL_FUNC(glColor4hvNV) \
    USE_GL_FUNC(glColor4ubVertex2fSUN) \
    USE_GL_FUNC(glColor4ubVertex2fvSUN) \
    USE_GL_FUNC(glColor4ubVertex3fSUN) \
    USE_GL_FUNC(glColor4ubVertex3fvSUN) \
    USE_GL_FUNC(glColor4xOES) \
    USE_GL_FUNC(glColor4xvOES) \
    USE_GL_FUNC(glColorFormatNV) \
    USE_GL_FUNC(glColorFragmentOp1ATI) \
    USE_GL_FUNC(glColorFragmentOp2ATI) \
    USE_GL_FUNC(glColorFragmentOp3ATI) \
    USE_GL_FUNC(glColorMaskIndexedEXT) \
    USE_GL_FUNC(glColorMaski) \
    USE_GL_FUNC(glColorP3ui) \
    USE_GL_FUNC(glColorP3uiv) \
    USE_GL_FUNC(glColorP4ui) \
    USE_GL_FUNC(glColorP4uiv) \
    USE_GL_FUNC(glColorPointerEXT) \
    USE_GL_FUNC(glColorPointerListIBM) \
    USE_GL_FUNC(glColorPointervINTEL) \
    USE_GL_FUNC(glColorSubTable) \
    USE_GL_FUNC(glColorSubTableEXT) \
    USE_GL_FUNC(glColorTable) \
    USE_GL_FUNC(glColorTableEXT) \
    USE_GL_FUNC(glColorTableParameterfv) \
    USE_GL_FUNC(glColorTableParameterfvSGI) \
    USE_GL_FUNC(glColorTableParameteriv) \
    USE_GL_FUNC(glColorTableParameterivSGI) \
    USE_GL_FUNC(glColorTableSGI) \
    USE_GL_FUNC(glCombinerInputNV) \
    USE_GL_FUNC(glCombinerOutputNV) \
    USE_GL_FUNC(glCombinerParameterfNV) \
    USE_GL_FUNC(glCombinerParameterfvNV) \
    USE_GL_FUNC(glCombinerParameteriNV) \
    USE_GL_FUNC(glCombinerParameterivNV) \
    USE_GL_FUNC(glCombinerStageParameterfvNV) \
    USE_GL_FUNC(glCommandListSegmentsNV) \
    USE_GL_FUNC(glCompileCommandListNV) \
    USE_GL_FUNC(glCompileShader) \
    USE_GL_FUNC(glCompileShaderARB) \
    USE_GL_FUNC(glCompileShaderIncludeARB) \
    USE_GL_FUNC(glCompressedMultiTexImage1DEXT) \
    USE_GL_FUNC(glCompressedMultiTexImage2DEXT) \
    USE_GL_FUNC(glCompressedMultiTexImage3DEXT) \
    USE_GL_FUNC(glCompressedMultiTexSubImage1DEXT) \
    USE_GL_FUNC(glCompressedMultiTexSubImage2DEXT) \
    USE_GL_FUNC(glCompressedMultiTexSubImage3DEXT) \
    USE_GL_FUNC(glCompressedTexImage1D) \
    USE_GL_FUNC(glCompressedTexImage1DARB) \
    USE_GL_FUNC(glCompressedTexImage2D) \
    USE_GL_FUNC(glCompressedTexImage2DARB) \
    USE_GL_FUNC(glCompressedTexImage3D) \
    USE_GL_FUNC(glCompressedTexImage3DARB) \
    USE_GL_FUNC(glCompressedTexSubImage1D) \
    USE_GL_FUNC(glCompressedTexSubImage1DARB) \
    USE_GL_FUNC(glCompressedTexSubImage2D) \
    USE_GL_FUNC(glCompressedTexSubImage2DARB) \
    USE_GL_FUNC(glCompressedTexSubImage3D) \
    USE_GL_FUNC(glCompressedTexSubImage3DARB) \
    USE_GL_FUNC(glCompressedTextureImage1DEXT) \
    USE_GL_FUNC(glCompressedTextureImage2DEXT) \
    USE_GL_FUNC(glCompressedTextureImage3DEXT) \
    USE_GL_FUNC(glCompressedTextureSubImage1D) \
    USE_GL_FUNC(glCompressedTextureSubImage1DEXT) \
    USE_GL_FUNC(glCompressedTextureSubImage2D) \
    USE_GL_FUNC(glCompressedTextureSubImage2DEXT) \
    USE_GL_FUNC(glCompressedTextureSubImage3D) \
    USE_GL_FUNC(glCompressedTextureSubImage3DEXT) \
    USE_GL_FUNC(glConservativeRasterParameterfNV) \
    USE_GL_FUNC(glConservativeRasterParameteriNV) \
    USE_GL_FUNC(glConvolutionFilter1D) \
    USE_GL_FUNC(glConvolutionFilter1DEXT) \
    USE_GL_FUNC(glConvolutionFilter2D) \
    USE_GL_FUNC(glConvolutionFilter2DEXT) \
    USE_GL_FUNC(glConvolutionParameterf) \
    USE_GL_FUNC(glConvolutionParameterfEXT) \
    USE_GL_FUNC(glConvolutionParameterfv) \
    USE_GL_FUNC(glConvolutionParameterfvEXT) \
    USE_GL_FUNC(glConvolutionParameteri) \
    USE_GL_FUNC(glConvolutionParameteriEXT) \
    USE_GL_FUNC(glConvolutionParameteriv) \
    USE_GL_FUNC(glConvolutionParameterivEXT) \
    USE_GL_FUNC(glConvolutionParameterxOES) \
    USE_GL_FUNC(glConvolutionParameterxvOES) \
    USE_GL_FUNC(glCopyBufferSubData) \
    USE_GL_FUNC(glCopyColorSubTable) \
    USE_GL_FUNC(glCopyColorSubTableEXT) \
    USE_GL_FUNC(glCopyColorTable) \
    USE_GL_FUNC(glCopyColorTableSGI) \
    USE_GL_FUNC(glCopyConvolutionFilter1D) \
    USE_GL_FUNC(glCopyConvolutionFilter1DEXT) \
    USE_GL_FUNC(glCopyConvolutionFilter2D) \
    USE_GL_FUNC(glCopyConvolutionFilter2DEXT) \
    USE_GL_FUNC(glCopyImageSubData) \
    USE_GL_FUNC(glCopyImageSubDataNV) \
    USE_GL_FUNC(glCopyMultiTexImage1DEXT) \
    USE_GL_FUNC(glCopyMultiTexImage2DEXT) \
    USE_GL_FUNC(glCopyMultiTexSubImage1DEXT) \
    USE_GL_FUNC(glCopyMultiTexSubImage2DEXT) \
    USE_GL_FUNC(glCopyMultiTexSubImage3DEXT) \
    USE_GL_FUNC(glCopyNamedBufferSubData) \
    USE_GL_FUNC(glCopyPathNV) \
    USE_GL_FUNC(glCopyTexImage1DEXT) \
    USE_GL_FUNC(glCopyTexImage2DEXT) \
    USE_GL_FUNC(glCopyTexSubImage1DEXT) \
    USE_GL_FUNC(glCopyTexSubImage2DEXT) \
    USE_GL_FUNC(glCopyTexSubImage3D) \
    USE_GL_FUNC(glCopyTexSubImage3DEXT) \
    USE_GL_FUNC(glCopyTextureImage1DEXT) \
    USE_GL_FUNC(glCopyTextureImage2DEXT) \
    USE_GL_FUNC(glCopyTextureSubImage1D) \
    USE_GL_FUNC(glCopyTextureSubImage1DEXT) \
    USE_GL_FUNC(glCopyTextureSubImage2D) \
    USE_GL_FUNC(glCopyTextureSubImage2DEXT) \
    USE_GL_FUNC(glCopyTextureSubImage3D) \
    USE_GL_FUNC(glCopyTextureSubImage3DEXT) \
    USE_GL_FUNC(glCoverFillPathInstancedNV) \
    USE_GL_FUNC(glCoverFillPathNV) \
    USE_GL_FUNC(glCoverStrokePathInstancedNV) \
    USE_GL_FUNC(glCoverStrokePathNV) \
    USE_GL_FUNC(glCoverageModulationNV) \
    USE_GL_FUNC(glCoverageModulationTableNV) \
    USE_GL_FUNC(glCreateBuffers) \
    USE_GL_FUNC(glCreateCommandListsNV) \
    USE_GL_FUNC(glCreateFramebuffers) \
    USE_GL_FUNC(glCreateMemoryObjectsEXT) \
    USE_GL_FUNC(glCreatePerfQueryINTEL) \
    USE_GL_FUNC(glCreateProgram) \
    USE_GL_FUNC(glCreateProgramObjectARB) \
    USE_GL_FUNC(glCreateProgramPipelines) \
    USE_GL_FUNC(glCreateProgressFenceNVX) \
    USE_GL_FUNC(glCreateQueries) \
    USE_GL_FUNC(glCreateRenderbuffers) \
    USE_GL_FUNC(glCreateSamplers) \
    USE_GL_FUNC(glCreateShader) \
    USE_GL_FUNC(glCreateShaderObjectARB) \
    USE_GL_FUNC(glCreateShaderProgramEXT) \
    USE_GL_FUNC(glCreateShaderProgramv) \
    USE_GL_FUNC(glCreateStatesNV) \
    USE_GL_FUNC(glCreateSyncFromCLeventARB) \
    USE_GL_FUNC(glCreateTextures) \
    USE_GL_FUNC(glCreateTransformFeedbacks) \
    USE_GL_FUNC(glCreateVertexArrays) \
    USE_GL_FUNC(glCullParameterdvEXT) \
    USE_GL_FUNC(glCullParameterfvEXT) \
    USE_GL_FUNC(glCurrentPaletteMatrixARB) \
    USE_GL_FUNC(glDebugMessageCallback) \
    USE_GL_FUNC(glDebugMessageCallbackAMD) \
    USE_GL_FUNC(glDebugMessageCallbackARB) \
    USE_GL_FUNC(glDebugMessageControl) \
    USE_GL_FUNC(glDebugMessageControlARB) \
    USE_GL_FUNC(glDebugMessageEnableAMD) \
    USE_GL_FUNC(glDebugMessageInsert) \
    USE_GL_FUNC(glDebugMessageInsertAMD) \
    USE_GL_FUNC(glDebugMessageInsertARB) \
    USE_GL_FUNC(glDeformSGIX) \
    USE_GL_FUNC(glDeformationMap3dSGIX) \
    USE_GL_FUNC(glDeformationMap3fSGIX) \
    USE_GL_FUNC(glDeleteAsyncMarkersSGIX) \
    USE_GL_FUNC(glDeleteBufferRegion) \
    USE_GL_FUNC(glDeleteBuffers) \
    USE_GL_FUNC(glDeleteBuffersARB) \
    USE_GL_FUNC(glDeleteCommandListsNV) \
    USE_GL_FUNC(glDeleteFencesAPPLE) \
    USE_GL_FUNC(glDeleteFencesNV) \
    USE_GL_FUNC(glDeleteFragmentShaderATI) \
    USE_GL_FUNC(glDeleteFramebuffers) \
    USE_GL_FUNC(glDeleteFramebuffersEXT) \
    USE_GL_FUNC(glDeleteMemoryObjectsEXT) \
    USE_GL_FUNC(glDeleteNamedStringARB) \
    USE_GL_FUNC(glDeleteNamesAMD) \
    USE_GL_FUNC(glDeleteObjectARB) \
    USE_GL_FUNC(glDeleteObjectBufferATI) \
    USE_GL_FUNC(glDeleteOcclusionQueriesNV) \
    USE_GL_FUNC(glDeletePathsNV) \
    USE_GL_FUNC(glDeletePerfMonitorsAMD) \
    USE_GL_FUNC(glDeletePerfQueryINTEL) \
    USE_GL_FUNC(glDeleteProgram) \
    USE_GL_FUNC(glDeleteProgramPipelines) \
    USE_GL_FUNC(glDeleteProgramsARB) \
    USE_GL_FUNC(glDeleteProgramsNV) \
    USE_GL_FUNC(glDeleteQueries) \
    USE_GL_FUNC(glDeleteQueriesARB) \
    USE_GL_FUNC(glDeleteQueryResourceTagNV) \
    USE_GL_FUNC(glDeleteRenderbuffers) \
    USE_GL_FUNC(glDeleteRenderbuffersEXT) \
    USE_GL_FUNC(glDeleteSamplers) \
    USE_GL_FUNC(glDeleteSemaphoresEXT) \
    USE_GL_FUNC(glDeleteShader) \
    USE_GL_FUNC(glDeleteStatesNV) \
    USE_GL_FUNC(glDeleteSync) \
    USE_GL_FUNC(glDeleteTexturesEXT) \
    USE_GL_FUNC(glDeleteTransformFeedbacks) \
    USE_GL_FUNC(glDeleteTransformFeedbacksNV) \
    USE_GL_FUNC(glDeleteVertexArrays) \
    USE_GL_FUNC(glDeleteVertexArraysAPPLE) \
    USE_GL_FUNC(glDeleteVertexShaderEXT) \
    USE_GL_FUNC(glDepthBoundsEXT) \
    USE_GL_FUNC(glDepthBoundsdNV) \
    USE_GL_FUNC(glDepthRangeArraydvNV) \
    USE_GL_FUNC(glDepthRangeArrayv) \
    USE_GL_FUNC(glDepthRangeIndexed) \
    USE_GL_FUNC(glDepthRangeIndexeddNV) \
    USE_GL_FUNC(glDepthRangedNV) \
    USE_GL_FUNC(glDepthRangef) \
    USE_GL_FUNC(glDepthRangefOES) \
    USE_GL_FUNC(glDepthRangexOES) \
    USE_GL_FUNC(glDetachObjectARB) \
    USE_GL_FUNC(glDetachShader) \
    USE_GL_FUNC(glDetailTexFuncSGIS) \
    USE_GL_FUNC(glDisableClientStateIndexedEXT) \
    USE_GL_FUNC(glDisableClientStateiEXT) \
    USE_GL_FUNC(glDisableIndexedEXT) \
    USE_GL_FUNC(glDisableVariantClientStateEXT) \
    USE_GL_FUNC(glDisableVertexArrayAttrib) \
    USE_GL_FUNC(glDisableVertexArrayAttribEXT) \
    USE_GL_FUNC(glDisableVertexArrayEXT) \
    USE_GL_FUNC(glDisableVertexAttribAPPLE) \
    USE_GL_FUNC(glDisableVertexAttribArray) \
    USE_GL_FUNC(glDisableVertexAttribArrayARB) \
    USE_GL_FUNC(glDisablei) \
    USE_GL_FUNC(glDispatchCompute) \
    USE_GL_FUNC(glDispatchComputeGroupSizeARB) \
    USE_GL_FUNC(glDispatchComputeIndirect) \
    USE_GL_FUNC(glDrawArraysEXT) \
    USE_GL_FUNC(glDrawArraysIndirect) \
    USE_GL_FUNC(glDrawArraysInstanced) \
    USE_GL_FUNC(glDrawArraysInstancedARB) \
    USE_GL_FUNC(glDrawArraysInstancedBaseInstance) \
    USE_GL_FUNC(glDrawArraysInstancedEXT) \
    USE_GL_FUNC(glDrawBufferRegion) \
    USE_GL_FUNC(glDrawBuffers) \
    USE_GL_FUNC(glDrawBuffersARB) \
    USE_GL_FUNC(glDrawBuffersATI) \
    USE_GL_FUNC(glDrawCommandsAddressNV) \
    USE_GL_FUNC(glDrawCommandsNV) \
    USE_GL_FUNC(glDrawCommandsStatesAddressNV) \
    USE_GL_FUNC(glDrawCommandsStatesNV) \
    USE_GL_FUNC(glDrawElementArrayAPPLE) \
    USE_GL_FUNC(glDrawElementArrayATI) \
    USE_GL_FUNC(glDrawElementsBaseVertex) \
    USE_GL_FUNC(glDrawElementsIndirect) \
    USE_GL_FUNC(glDrawElementsInstanced) \
    USE_GL_FUNC(glDrawElementsInstancedARB) \
    USE_GL_FUNC(glDrawElementsInstancedBaseInstance) \
    USE_GL_FUNC(glDrawElementsInstancedBaseVertex) \
    USE_GL_FUNC(glDrawElementsInstancedBaseVertexBaseInstance) \
    USE_GL_FUNC(glDrawElementsInstancedEXT) \
    USE_GL_FUNC(glDrawMeshArraysSUN) \
    USE_GL_FUNC(glDrawMeshTasksIndirectNV) \
    USE_GL_FUNC(glDrawMeshTasksNV) \
    USE_GL_FUNC(glDrawRangeElementArrayAPPLE) \
    USE_GL_FUNC(glDrawRangeElementArrayATI) \
    USE_GL_FUNC(glDrawRangeElements) \
    USE_GL_FUNC(glDrawRangeElementsBaseVertex) \
    USE_GL_FUNC(glDrawRangeElementsEXT) \
    USE_GL_FUNC(glDrawTextureNV) \
    USE_GL_FUNC(glDrawTransformFeedback) \
    USE_GL_FUNC(glDrawTransformFeedbackInstanced) \
    USE_GL_FUNC(glDrawTransformFeedbackNV) \
    USE_GL_FUNC(glDrawTransformFeedbackStream) \
    USE_GL_FUNC(glDrawTransformFeedbackStreamInstanced) \
    USE_GL_FUNC(glDrawVkImageNV) \
    USE_GL_FUNC(glEGLImageTargetTexStorageEXT) \
    USE_GL_FUNC(glEGLImageTargetTextureStorageEXT) \
    USE_GL_FUNC(glEdgeFlagFormatNV) \
    USE_GL_FUNC(glEdgeFlagPointerEXT) \
    USE_GL_FUNC(glEdgeFlagPointerListIBM) \
    USE_GL_FUNC(glElementPointerAPPLE) \
    USE_GL_FUNC(glElementPointerATI) \
    USE_GL_FUNC(glEnableClientStateIndexedEXT) \
    USE_GL_FUNC(glEnableClientStateiEXT) \
    USE_GL_FUNC(glEnableIndexedEXT) \
    USE_GL_FUNC(glEnableVariantClientStateEXT) \
    USE_GL_FUNC(glEnableVertexArrayAttrib) \
    USE_GL_FUNC(glEnableVertexArrayAttribEXT) \
    USE_GL_FUNC(glEnableVertexArrayEXT) \
    USE_GL_FUNC(glEnableVertexAttribAPPLE) \
    USE_GL_FUNC(glEnableVertexAttribArray) \
    USE_GL_FUNC(glEnableVertexAttribArrayARB) \
    USE_GL_FUNC(glEnablei) \
    USE_GL_FUNC(glEndConditionalRender) \
    USE_GL_FUNC(glEndConditionalRenderNV) \
    USE_GL_FUNC(glEndConditionalRenderNVX) \
    USE_GL_FUNC(glEndFragmentShaderATI) \
    USE_GL_FUNC(glEndOcclusionQueryNV) \
    USE_GL_FUNC(glEndPerfMonitorAMD) \
    USE_GL_FUNC(glEndPerfQueryINTEL) \
    USE_GL_FUNC(glEndQuery) \
    USE_GL_FUNC(glEndQueryARB) \
    USE_GL_FUNC(glEndQueryIndexed) \
    USE_GL_FUNC(glEndTransformFeedback) \
    USE_GL_FUNC(glEndTransformFeedbackEXT) \
    USE_GL_FUNC(glEndTransformFeedbackNV) \
    USE_GL_FUNC(glEndVertexShaderEXT) \
    USE_GL_FUNC(glEndVideoCaptureNV) \
    USE_GL_FUNC(glEvalCoord1xOES) \
    USE_GL_FUNC(glEvalCoord1xvOES) \
    USE_GL_FUNC(glEvalCoord2xOES) \
    USE_GL_FUNC(glEvalCoord2xvOES) \
    USE_GL_FUNC(glEvalMapsNV) \
    USE_GL_FUNC(glEvaluateDepthValuesARB) \
    USE_GL_FUNC(glExecuteProgramNV) \
    USE_GL_FUNC(glExtractComponentEXT) \
    USE_GL_FUNC(glFeedbackBufferxOES) \
    USE_GL_FUNC(glFenceSync) \
    USE_GL_FUNC(glFinalCombinerInputNV) \
    USE_GL_FUNC(glFinishAsyncSGIX) \
    USE_GL_FUNC(glFinishFenceAPPLE) \
    USE_GL_FUNC(glFinishFenceNV) \
    USE_GL_FUNC(glFinishObjectAPPLE) \
    USE_GL_FUNC(glFinishTextureSUNX) \
    USE_GL_FUNC(glFlushMappedBufferRange) \
    USE_GL_FUNC(glFlushMappedBufferRangeAPPLE) \
    USE_GL_FUNC(glFlushMappedNamedBufferRange) \
    USE_GL_FUNC(glFlushMappedNamedBufferRangeEXT) \
    USE_GL_FUNC(glFlushPixelDataRangeNV) \
    USE_GL_FUNC(glFlushRasterSGIX) \
    USE_GL_FUNC(glFlushStaticDataIBM) \
    USE_GL_FUNC(glFlushVertexArrayRangeAPPLE) \
    USE_GL_FUNC(glFlushVertexArrayRangeNV) \
    USE_GL_FUNC(glFogCoordFormatNV) \
    USE_GL_FUNC(glFogCoordPointer) \
    USE_GL_FUNC(glFogCoordPointerEXT) \
    USE_GL_FUNC(glFogCoordPointerListIBM) \
    USE_GL_FUNC(glFogCoordd) \
    USE_GL_FUNC(glFogCoorddEXT) \
    USE_GL_FUNC(glFogCoorddv) \
    USE_GL_FUNC(glFogCoorddvEXT) \
    USE_GL_FUNC(glFogCoordf) \
    USE_GL_FUNC(glFogCoordfEXT) \
    USE_GL_FUNC(glFogCoordfv) \
    USE_GL_FUNC(glFogCoordfvEXT) \
    USE_GL_FUNC(glFogCoordhNV) \
    USE_GL_FUNC(glFogCoordhvNV) \
    USE_GL_FUNC(glFogFuncSGIS) \
    USE_GL_FUNC(glFogxOES) \
    USE_GL_FUNC(glFogxvOES) \
    USE_GL_FUNC(glFragmentColorMaterialSGIX) \
    USE_GL_FUNC(glFragmentCoverageColorNV) \
    USE_GL_FUNC(glFragmentLightModelfSGIX) \
    USE_GL_FUNC(glFragmentLightModelfvSGIX) \
    USE_GL_FUNC(glFragmentLightModeliSGIX) \
    USE_GL_FUNC(glFragmentLightModelivSGIX) \
    USE_GL_FUNC(glFragmentLightfSGIX) \
    USE_GL_FUNC(glFragmentLightfvSGIX) \
    USE_GL_FUNC(glFragmentLightiSGIX) \
    USE_GL_FUNC(glFragmentLightivSGIX) \
    USE_GL_FUNC(glFragmentMaterialfSGIX) \
    USE_GL_FUNC(glFragmentMaterialfvSGIX) \
    USE_GL_FUNC(glFragmentMaterialiSGIX) \
    USE_GL_FUNC(glFragmentMaterialivSGIX) \
    USE_GL_FUNC(glFrameTerminatorGREMEDY) \
    USE_GL_FUNC(glFrameZoomSGIX) \
    USE_GL_FUNC(glFramebufferDrawBufferEXT) \
    USE_GL_FUNC(glFramebufferDrawBuffersEXT) \
    USE_GL_FUNC(glFramebufferFetchBarrierEXT) \
    USE_GL_FUNC(glFramebufferParameteri) \
    USE_GL_FUNC(glFramebufferParameteriMESA) \
    USE_GL_FUNC(glFramebufferReadBufferEXT) \
    USE_GL_FUNC(glFramebufferRenderbuffer) \
    USE_GL_FUNC(glFramebufferRenderbufferEXT) \
    USE_GL_FUNC(glFramebufferSampleLocationsfvARB) \
    USE_GL_FUNC(glFramebufferSampleLocationsfvNV) \
    USE_GL_FUNC(glFramebufferSamplePositionsfvAMD) \
    USE_GL_FUNC(glFramebufferTexture) \
    USE_GL_FUNC(glFramebufferTexture1D) \
    USE_GL_FUNC(glFramebufferTexture1DEXT) \
    USE_GL_FUNC(glFramebufferTexture2D) \
    USE_GL_FUNC(glFramebufferTexture2DEXT) \
    USE_GL_FUNC(glFramebufferTexture3D) \
    USE_GL_FUNC(glFramebufferTexture3DEXT) \
    USE_GL_FUNC(glFramebufferTextureARB) \
    USE_GL_FUNC(glFramebufferTextureEXT) \
    USE_GL_FUNC(glFramebufferTextureFaceARB) \
    USE_GL_FUNC(glFramebufferTextureFaceEXT) \
    USE_GL_FUNC(glFramebufferTextureLayer) \
    USE_GL_FUNC(glFramebufferTextureLayerARB) \
    USE_GL_FUNC(glFramebufferTextureLayerEXT) \
    USE_GL_FUNC(glFramebufferTextureMultiviewOVR) \
    USE_GL_FUNC(glFreeObjectBufferATI) \
    USE_GL_FUNC(glFrustumfOES) \
    USE_GL_FUNC(glFrustumxOES) \
    USE_GL_FUNC(glGenAsyncMarkersSGIX) \
    USE_GL_FUNC(glGenBuffers) \
    USE_GL_FUNC(glGenBuffersARB) \
    USE_GL_FUNC(glGenFencesAPPLE) \
    USE_GL_FUNC(glGenFencesNV) \
    USE_GL_FUNC(glGenFragmentShadersATI) \
    USE_GL_FUNC(glGenFramebuffers) \
    USE_GL_FUNC(glGenFramebuffersEXT) \
    USE_GL_FUNC(glGenNamesAMD) \
    USE_GL_FUNC(glGenOcclusionQueriesNV) \
    USE_GL_FUNC(glGenPathsNV) \
    USE_GL_FUNC(glGenPerfMonitorsAMD) \
    USE_GL_FUNC(glGenProgramPipelines) \
    USE_GL_FUNC(glGenProgramsARB) \
    USE_GL_FUNC(glGenProgramsNV) \
    USE_GL_FUNC(glGenQueries) \
    USE_GL_FUNC(glGenQueriesARB) \
    USE_GL_FUNC(glGenQueryResourceTagNV) \
    USE_GL_FUNC(glGenRenderbuffers) \
    USE_GL_FUNC(glGenRenderbuffersEXT) \
    USE_GL_FUNC(glGenSamplers) \
    USE_GL_FUNC(glGenSemaphoresEXT) \
    USE_GL_FUNC(glGenSymbolsEXT) \
    USE_GL_FUNC(glGenTexturesEXT) \
    USE_GL_FUNC(glGenTransformFeedbacks) \
    USE_GL_FUNC(glGenTransformFeedbacksNV) \
    USE_GL_FUNC(glGenVertexArrays) \
    USE_GL_FUNC(glGenVertexArraysAPPLE) \
    USE_GL_FUNC(glGenVertexShadersEXT) \
    USE_GL_FUNC(glGenerateMipmap) \
    USE_GL_FUNC(glGenerateMipmapEXT) \
    USE_GL_FUNC(glGenerateMultiTexMipmapEXT) \
    USE_GL_FUNC(glGenerateTextureMipmap) \
    USE_GL_FUNC(glGenerateTextureMipmapEXT) \
    USE_GL_FUNC(glGetActiveAtomicCounterBufferiv) \
    USE_GL_FUNC(glGetActiveAttrib) \
    USE_GL_FUNC(glGetActiveAttribARB) \
    USE_GL_FUNC(glGetActiveSubroutineName) \
    USE_GL_FUNC(glGetActiveSubroutineUniformName) \
    USE_GL_FUNC(glGetActiveSubroutineUniformiv) \
    USE_GL_FUNC(glGetActiveUniform) \
    USE_GL_FUNC(glGetActiveUniformARB) \
    USE_GL_FUNC(glGetActiveUniformBlockName) \
    USE_GL_FUNC(glGetActiveUniformBlockiv) \
    USE_GL_FUNC(glGetActiveUniformName) \
    USE_GL_FUNC(glGetActiveUniformsiv) \
    USE_GL_FUNC(glGetActiveVaryingNV) \
    USE_GL_FUNC(glGetArrayObjectfvATI) \
    USE_GL_FUNC(glGetArrayObjectivATI) \
    USE_GL_FUNC(glGetAttachedObjectsARB) \
    USE_GL_FUNC(glGetAttachedShaders) \
    USE_GL_FUNC(glGetAttribLocation) \
    USE_GL_FUNC(glGetAttribLocationARB) \
    USE_GL_FUNC(glGetBooleanIndexedvEXT) \
    USE_GL_FUNC(glGetBooleani_v) \
    USE_GL_FUNC(glGetBufferParameteri64v) \
    USE_GL_FUNC(glGetBufferParameteriv) \
    USE_GL_FUNC(glGetBufferParameterivARB) \
    USE_GL_FUNC(glGetBufferParameterui64vNV) \
    USE_GL_FUNC(glGetBufferPointerv) \
    USE_GL_FUNC(glGetBufferPointervARB) \
    USE_GL_FUNC(glGetBufferSubData) \
    USE_GL_FUNC(glGetBufferSubDataARB) \
    USE_GL_FUNC(glGetClipPlanefOES) \
    USE_GL_FUNC(glGetClipPlanexOES) \
    USE_GL_FUNC(glGetColorTable) \
    USE_GL_FUNC(glGetColorTableEXT) \
    USE_GL_FUNC(glGetColorTableParameterfv) \
    USE_GL_FUNC(glGetColorTableParameterfvEXT) \
    USE_GL_FUNC(glGetColorTableParameterfvSGI) \
    USE_GL_FUNC(glGetColorTableParameteriv) \
    USE_GL_FUNC(glGetColorTableParameterivEXT) \
    USE_GL_FUNC(glGetColorTableParameterivSGI) \
    USE_GL_FUNC(glGetColorTableSGI) \
    USE_GL_FUNC(glGetCombinerInputParameterfvNV) \
    USE_GL_FUNC(glGetCombinerInputParameterivNV) \
    USE_GL_FUNC(glGetCombinerOutputParameterfvNV) \
    USE_GL_FUNC(glGetCombinerOutputParameterivNV) \
    USE_GL_FUNC(glGetCombinerStageParameterfvNV) \
    USE_GL_FUNC(glGetCommandHeaderNV) \
    USE_GL_FUNC(glGetCompressedMultiTexImageEXT) \
    USE_GL_FUNC(glGetCompressedTexImage) \
    USE_GL_FUNC(glGetCompressedTexImageARB) \
    USE_GL_FUNC(glGetCompressedTextureImage) \
    USE_GL_FUNC(glGetCompressedTextureImageEXT) \
    USE_GL_FUNC(glGetCompressedTextureSubImage) \
    USE_GL_FUNC(glGetConvolutionFilter) \
    USE_GL_FUNC(glGetConvolutionFilterEXT) \
    USE_GL_FUNC(glGetConvolutionParameterfv) \
    USE_GL_FUNC(glGetConvolutionParameterfvEXT) \
    USE_GL_FUNC(glGetConvolutionParameteriv) \
    USE_GL_FUNC(glGetConvolutionParameterivEXT) \
    USE_GL_FUNC(glGetConvolutionParameterxvOES) \
    USE_GL_FUNC(glGetCoverageModulationTableNV) \
    USE_GL_FUNC(glGetDebugMessageLog) \
    USE_GL_FUNC(glGetDebugMessageLogAMD) \
    USE_GL_FUNC(glGetDebugMessageLogARB) \
    USE_GL_FUNC(glGetDetailTexFuncSGIS) \
    USE_GL_FUNC(glGetDoubleIndexedvEXT) \
    USE_GL_FUNC(glGetDoublei_v) \
    USE_GL_FUNC(glGetDoublei_vEXT) \
    USE_GL_FUNC(glGetFenceivNV) \
    USE_GL_FUNC(glGetFinalCombinerInputParameterfvNV) \
    USE_GL_FUNC(glGetFinalCombinerInputParameterivNV) \
    USE_GL_FUNC(glGetFirstPerfQueryIdINTEL) \
    USE_GL_FUNC(glGetFixedvOES) \
    USE_GL_FUNC(glGetFloatIndexedvEXT) \
    USE_GL_FUNC(glGetFloati_v) \
    USE_GL_FUNC(glGetFloati_vEXT) \
    USE_GL_FUNC(glGetFogFuncSGIS) \
    USE_GL_FUNC(glGetFragDataIndex) \
    USE_GL_FUNC(glGetFragDataLocation) \
    USE_GL_FUNC(glGetFragDataLocationEXT) \
    USE_GL_FUNC(glGetFragmentLightfvSGIX) \
    USE_GL_FUNC(glGetFragmentLightivSGIX) \
    USE_GL_FUNC(glGetFragmentMaterialfvSGIX) \
    USE_GL_FUNC(glGetFragmentMaterialivSGIX) \
    USE_GL_FUNC(glGetFramebufferAttachmentParameteriv) \
    USE_GL_FUNC(glGetFramebufferAttachmentParameterivEXT) \
    USE_GL_FUNC(glGetFramebufferParameterfvAMD) \
    USE_GL_FUNC(glGetFramebufferParameteriv) \
    USE_GL_FUNC(glGetFramebufferParameterivEXT) \
    USE_GL_FUNC(glGetFramebufferParameterivMESA) \
    USE_GL_FUNC(glGetGraphicsResetStatus) \
    USE_GL_FUNC(glGetGraphicsResetStatusARB) \
    USE_GL_FUNC(glGetHandleARB) \
    USE_GL_FUNC(glGetHistogram) \
    USE_GL_FUNC(glGetHistogramEXT) \
    USE_GL_FUNC(glGetHistogramParameterfv) \
    USE_GL_FUNC(glGetHistogramParameterfvEXT) \
    USE_GL_FUNC(glGetHistogramParameteriv) \
    USE_GL_FUNC(glGetHistogramParameterivEXT) \
    USE_GL_FUNC(glGetHistogramParameterxvOES) \
    USE_GL_FUNC(glGetImageHandleARB) \
    USE_GL_FUNC(glGetImageHandleNV) \
    USE_GL_FUNC(glGetImageTransformParameterfvHP) \
    USE_GL_FUNC(glGetImageTransformParameterivHP) \
    USE_GL_FUNC(glGetInfoLogARB) \
    USE_GL_FUNC(glGetInstrumentsSGIX) \
    USE_GL_FUNC(glGetInteger64i_v) \
    USE_GL_FUNC(glGetInteger64v) \
    USE_GL_FUNC(glGetIntegerIndexedvEXT) \
    USE_GL_FUNC(glGetIntegeri_v) \
    USE_GL_FUNC(glGetIntegerui64i_vNV) \
    USE_GL_FUNC(glGetIntegerui64vNV) \
    USE_GL_FUNC(glGetInternalformatSampleivNV) \
    USE_GL_FUNC(glGetInternalformati64v) \
    USE_GL_FUNC(glGetInternalformativ) \
    USE_GL_FUNC(glGetInvariantBooleanvEXT) \
    USE_GL_FUNC(glGetInvariantFloatvEXT) \
    USE_GL_FUNC(glGetInvariantIntegervEXT) \
    USE_GL_FUNC(glGetLightxOES) \
    USE_GL_FUNC(glGetListParameterfvSGIX) \
    USE_GL_FUNC(glGetListParameterivSGIX) \
    USE_GL_FUNC(glGetLocalConstantBooleanvEXT) \
    USE_GL_FUNC(glGetLocalConstantFloatvEXT) \
    USE_GL_FUNC(glGetLocalConstantIntegervEXT) \
    USE_GL_FUNC(glGetMapAttribParameterfvNV) \
    USE_GL_FUNC(glGetMapAttribParameterivNV) \
    USE_GL_FUNC(glGetMapControlPointsNV) \
    USE_GL_FUNC(glGetMapParameterfvNV) \
    USE_GL_FUNC(glGetMapParameterivNV) \
    USE_GL_FUNC(glGetMapxvOES) \
    USE_GL_FUNC(glGetMaterialxOES) \
    USE_GL_FUNC(glGetMemoryObjectDetachedResourcesuivNV) \
    USE_GL_FUNC(glGetMemoryObjectParameterivEXT) \
    USE_GL_FUNC(glGetMinmax) \
    USE_GL_FUNC(glGetMinmaxEXT) \
    USE_GL_FUNC(glGetMinmaxParameterfv) \
    USE_GL_FUNC(glGetMinmaxParameterfvEXT) \
    USE_GL_FUNC(glGetMinmaxParameteriv) \
    USE_GL_FUNC(glGetMinmaxParameterivEXT) \
    USE_GL_FUNC(glGetMultiTexEnvfvEXT) \
    USE_GL_FUNC(glGetMultiTexEnvivEXT) \
    USE_GL_FUNC(glGetMultiTexGendvEXT) \
    USE_GL_FUNC(glGetMultiTexGenfvEXT) \
    USE_GL_FUNC(glGetMultiTexGenivEXT) \
    USE_GL_FUNC(glGetMultiTexImageEXT) \
    USE_GL_FUNC(glGetMultiTexLevelParameterfvEXT) \
    USE_GL_FUNC(glGetMultiTexLevelParameterivEXT) \
    USE_GL_FUNC(glGetMultiTexParameterIivEXT) \
    USE_GL_FUNC(glGetMultiTexParameterIuivEXT) \
    USE_GL_FUNC(glGetMultiTexParameterfvEXT) \
    USE_GL_FUNC(glGetMultiTexParameterivEXT) \
    USE_GL_FUNC(glGetMultisamplefv) \
    USE_GL_FUNC(glGetMultisamplefvNV) \
    USE_GL_FUNC(glGetNamedBufferParameteri64v) \
    USE_GL_FUNC(glGetNamedBufferParameteriv) \
    USE_GL_FUNC(glGetNamedBufferParameterivEXT) \
    USE_GL_FUNC(glGetNamedBufferParameterui64vNV) \
    USE_GL_FUNC(glGetNamedBufferPointerv) \
    USE_GL_FUNC(glGetNamedBufferPointervEXT) \
    USE_GL_FUNC(glGetNamedBufferSubData) \
    USE_GL_FUNC(glGetNamedBufferSubDataEXT) \
    USE_GL_FUNC(glGetNamedFramebufferAttachmentParameteriv) \
    USE_GL_FUNC(glGetNamedFramebufferAttachmentParameterivEXT) \
    USE_GL_FUNC(glGetNamedFramebufferParameterfvAMD) \
    USE_GL_FUNC(glGetNamedFramebufferParameteriv) \
    USE_GL_FUNC(glGetNamedFramebufferParameterivEXT) \
    USE_GL_FUNC(glGetNamedProgramLocalParameterIivEXT) \
    USE_GL_FUNC(glGetNamedProgramLocalParameterIuivEXT) \
    USE_GL_FUNC(glGetNamedProgramLocalParameterdvEXT) \
    USE_GL_FUNC(glGetNamedProgramLocalParameterfvEXT) \
    USE_GL_FUNC(glGetNamedProgramStringEXT) \
    USE_GL_FUNC(glGetNamedProgramivEXT) \
    USE_GL_FUNC(glGetNamedRenderbufferParameteriv) \
    USE_GL_FUNC(glGetNamedRenderbufferParameterivEXT) \
    USE_GL_FUNC(glGetNamedStringARB) \
    USE_GL_FUNC(glGetNamedStringivARB) \
    USE_GL_FUNC(glGetNextPerfQueryIdINTEL) \
    USE_GL_FUNC(glGetObjectBufferfvATI) \
    USE_GL_FUNC(glGetObjectBufferivATI) \
    USE_GL_FUNC(glGetObjectLabel) \
    USE_GL_FUNC(glGetObjectLabelEXT) \
    USE_GL_FUNC(glGetObjectParameterfvARB) \
    USE_GL_FUNC(glGetObjectParameterivAPPLE) \
    USE_GL_FUNC(glGetObjectParameterivARB) \
    USE_GL_FUNC(glGetObjectPtrLabel) \
    USE_GL_FUNC(glGetOcclusionQueryivNV) \
    USE_GL_FUNC(glGetOcclusionQueryuivNV) \
    USE_GL_FUNC(glGetPathColorGenfvNV) \
    USE_GL_FUNC(glGetPathColorGenivNV) \
    USE_GL_FUNC(glGetPathCommandsNV) \
    USE_GL_FUNC(glGetPathCoordsNV) \
    USE_GL_FUNC(glGetPathDashArrayNV) \
    USE_GL_FUNC(glGetPathLengthNV) \
    USE_GL_FUNC(glGetPathMetricRangeNV) \
    USE_GL_FUNC(glGetPathMetricsNV) \
    USE_GL_FUNC(glGetPathParameterfvNV) \
    USE_GL_FUNC(glGetPathParameterivNV) \
    USE_GL_FUNC(glGetPathSpacingNV) \
    USE_GL_FUNC(glGetPathTexGenfvNV) \
    USE_GL_FUNC(glGetPathTexGenivNV) \
    USE_GL_FUNC(glGetPerfCounterInfoINTEL) \
    USE_GL_FUNC(glGetPerfMonitorCounterDataAMD) \
    USE_GL_FUNC(glGetPerfMonitorCounterInfoAMD) \
    USE_GL_FUNC(glGetPerfMonitorCounterStringAMD) \
    USE_GL_FUNC(glGetPerfMonitorCountersAMD) \
    USE_GL_FUNC(glGetPerfMonitorGroupStringAMD) \
    USE_GL_FUNC(glGetPerfMonitorGroupsAMD) \
    USE_GL_FUNC(glGetPerfQueryDataINTEL) \
    USE_GL_FUNC(glGetPerfQueryIdByNameINTEL) \
    USE_GL_FUNC(glGetPerfQueryInfoINTEL) \
    USE_GL_FUNC(glGetPixelMapxv) \
    USE_GL_FUNC(glGetPixelTexGenParameterfvSGIS) \
    USE_GL_FUNC(glGetPixelTexGenParameterivSGIS) \
    USE_GL_FUNC(glGetPixelTransformParameterfvEXT) \
    USE_GL_FUNC(glGetPixelTransformParameterivEXT) \
    USE_GL_FUNC(glGetPointerIndexedvEXT) \
    USE_GL_FUNC(glGetPointeri_vEXT) \
    USE_GL_FUNC(glGetPointervEXT) \
    USE_GL_FUNC(glGetProgramBinary) \
    USE_GL_FUNC(glGetProgramEnvParameterIivNV) \
    USE_GL_FUNC(glGetProgramEnvParameterIuivNV) \
    USE_GL_FUNC(glGetProgramEnvParameterdvARB) \
    USE_GL_FUNC(glGetProgramEnvParameterfvARB) \
    USE_GL_FUNC(glGetProgramInfoLog) \
    USE_GL_FUNC(glGetProgramInterfaceiv) \
    USE_GL_FUNC(glGetProgramLocalParameterIivNV) \
    USE_GL_FUNC(glGetProgramLocalParameterIuivNV) \
    USE_GL_FUNC(glGetProgramLocalParameterdvARB) \
    USE_GL_FUNC(glGetProgramLocalParameterfvARB) \
    USE_GL_FUNC(glGetProgramNamedParameterdvNV) \
    USE_GL_FUNC(glGetProgramNamedParameterfvNV) \
    USE_GL_FUNC(glGetProgramParameterdvNV) \
    USE_GL_FUNC(glGetProgramParameterfvNV) \
    USE_GL_FUNC(glGetProgramPipelineInfoLog) \
    USE_GL_FUNC(glGetProgramPipelineiv) \
    USE_GL_FUNC(glGetProgramResourceIndex) \
    USE_GL_FUNC(glGetProgramResourceLocation) \
    USE_GL_FUNC(glGetProgramResourceLocationIndex) \
    USE_GL_FUNC(glGetProgramResourceName) \
    USE_GL_FUNC(glGetProgramResourcefvNV) \
    USE_GL_FUNC(glGetProgramResourceiv) \
    USE_GL_FUNC(glGetProgramStageiv) \
    USE_GL_FUNC(glGetProgramStringARB) \
    USE_GL_FUNC(glGetProgramStringNV) \
    USE_GL_FUNC(glGetProgramSubroutineParameteruivNV) \
    USE_GL_FUNC(glGetProgramiv) \
    USE_GL_FUNC(glGetProgramivARB) \
    USE_GL_FUNC(glGetProgramivNV) \
    USE_GL_FUNC(glGetQueryBufferObjecti64v) \
    USE_GL_FUNC(glGetQueryBufferObjectiv) \
    USE_GL_FUNC(glGetQueryBufferObjectui64v) \
    USE_GL_FUNC(glGetQueryBufferObjectuiv) \
    USE_GL_FUNC(glGetQueryIndexediv) \
    USE_GL_FUNC(glGetQueryObjecti64v) \
    USE_GL_FUNC(glGetQueryObjecti64vEXT) \
    USE_GL_FUNC(glGetQueryObjectiv) \
    USE_GL_FUNC(glGetQueryObjectivARB) \
    USE_GL_FUNC(glGetQueryObjectui64v) \
    USE_GL_FUNC(glGetQueryObjectui64vEXT) \
    USE_GL_FUNC(glGetQueryObjectuiv) \
    USE_GL_FUNC(glGetQueryObjectuivARB) \
    USE_GL_FUNC(glGetQueryiv) \
    USE_GL_FUNC(glGetQueryivARB) \
    USE_GL_FUNC(glGetRenderbufferParameteriv) \
    USE_GL_FUNC(glGetRenderbufferParameterivEXT) \
    USE_GL_FUNC(glGetSamplerParameterIiv) \
    USE_GL_FUNC(glGetSamplerParameterIuiv) \
    USE_GL_FUNC(glGetSamplerParameterfv) \
    USE_GL_FUNC(glGetSamplerParameteriv) \
    USE_GL_FUNC(glGetSemaphoreParameterui64vEXT) \
    USE_GL_FUNC(glGetSeparableFilter) \
    USE_GL_FUNC(glGetSeparableFilterEXT) \
    USE_GL_FUNC(glGetShaderInfoLog) \
    USE_GL_FUNC(glGetShaderPrecisionFormat) \
    USE_GL_FUNC(glGetShaderSource) \
    USE_GL_FUNC(glGetShaderSourceARB) \
    USE_GL_FUNC(glGetShaderiv) \
    USE_GL_FUNC(glGetShadingRateImagePaletteNV) \
    USE_GL_FUNC(glGetShadingRateSampleLocationivNV) \
    USE_GL_FUNC(glGetSharpenTexFuncSGIS) \
    USE_GL_FUNC(glGetStageIndexNV) \
    USE_GL_FUNC(glGetStringi) \
    USE_GL_FUNC(glGetSubroutineIndex) \
    USE_GL_FUNC(glGetSubroutineUniformLocation) \
    USE_GL_FUNC(glGetSynciv) \
    USE_GL_FUNC(glGetTexBumpParameterfvATI) \
    USE_GL_FUNC(glGetTexBumpParameterivATI) \
    USE_GL_FUNC(glGetTexEnvxvOES) \
    USE_GL_FUNC(glGetTexFilterFuncSGIS) \
    USE_GL_FUNC(glGetTexGenxvOES) \
    USE_GL_FUNC(glGetTexLevelParameterxvOES) \
    USE_GL_FUNC(glGetTexParameterIiv) \
    USE_GL_FUNC(glGetTexParameterIivEXT) \
    USE_GL_FUNC(glGetTexParameterIuiv) \
    USE_GL_FUNC(glGetTexParameterIuivEXT) \
    USE_GL_FUNC(glGetTexParameterPointervAPPLE) \
    USE_GL_FUNC(glGetTexParameterxvOES) \
    USE_GL_FUNC(glGetTextureHandleARB) \
    USE_GL_FUNC(glGetTextureHandleNV) \
    USE_GL_FUNC(glGetTextureImage) \
    USE_GL_FUNC(glGetTextureImageEXT) \
    USE_GL_FUNC(glGetTextureLevelParameterfv) \
    USE_GL_FUNC(glGetTextureLevelParameterfvEXT) \
    USE_GL_FUNC(glGetTextureLevelParameteriv) \
    USE_GL_FUNC(glGetTextureLevelParameterivEXT) \
    USE_GL_FUNC(glGetTextureParameterIiv) \
    USE_GL_FUNC(glGetTextureParameterIivEXT) \
    USE_GL_FUNC(glGetTextureParameterIuiv) \
    USE_GL_FUNC(glGetTextureParameterIuivEXT) \
    USE_GL_FUNC(glGetTextureParameterfv) \
    USE_GL_FUNC(glGetTextureParameterfvEXT) \
    USE_GL_FUNC(glGetTextureParameteriv) \
    USE_GL_FUNC(glGetTextureParameterivEXT) \
    USE_GL_FUNC(glGetTextureSamplerHandleARB) \
    USE_GL_FUNC(glGetTextureSamplerHandleNV) \
    USE_GL_FUNC(glGetTextureSubImage) \
    USE_GL_FUNC(glGetTrackMatrixivNV) \
    USE_GL_FUNC(glGetTransformFeedbackVarying) \
    USE_GL_FUNC(glGetTransformFeedbackVaryingEXT) \
    USE_GL_FUNC(glGetTransformFeedbackVaryingNV) \
    USE_GL_FUNC(glGetTransformFeedbacki64_v) \
    USE_GL_FUNC(glGetTransformFeedbacki_v) \
    USE_GL_FUNC(glGetTransformFeedbackiv) \
    USE_GL_FUNC(glGetUniformBlockIndex) \
    USE_GL_FUNC(glGetUniformBufferSizeEXT) \
    USE_GL_FUNC(glGetUniformIndices) \
    USE_GL_FUNC(glGetUniformLocation) \
    USE_GL_FUNC(glGetUniformLocationARB) \
    USE_GL_FUNC(glGetUniformOffsetEXT) \
    USE_GL_FUNC(glGetUniformSubroutineuiv) \
    USE_GL_FUNC(glGetUniformdv) \
    USE_GL_FUNC(glGetUniformfv) \
    USE_GL_FUNC(glGetUniformfvARB) \
    USE_GL_FUNC(glGetUniformi64vARB) \
    USE_GL_FUNC(glGetUniformi64vNV) \
    USE_GL_FUNC(glGetUniformiv) \
    USE_GL_FUNC(glGetUniformivARB) \
    USE_GL_FUNC(glGetUniformui64vARB) \
    USE_GL_FUNC(glGetUniformui64vNV) \
    USE_GL_FUNC(glGetUniformuiv) \
    USE_GL_FUNC(glGetUniformuivEXT) \
    USE_GL_FUNC(glGetUnsignedBytei_vEXT) \
    USE_GL_FUNC(glGetUnsignedBytevEXT) \
    USE_GL_FUNC(glGetVariantArrayObjectfvATI) \
    USE_GL_FUNC(glGetVariantArrayObjectivATI) \
    USE_GL_FUNC(glGetVariantBooleanvEXT) \
    USE_GL_FUNC(glGetVariantFloatvEXT) \
    USE_GL_FUNC(glGetVariantIntegervEXT) \
    USE_GL_FUNC(glGetVariantPointervEXT) \
    USE_GL_FUNC(glGetVaryingLocationNV) \
    USE_GL_FUNC(glGetVertexArrayIndexed64iv) \
    USE_GL_FUNC(glGetVertexArrayIndexediv) \
    USE_GL_FUNC(glGetVertexArrayIntegeri_vEXT) \
    USE_GL_FUNC(glGetVertexArrayIntegervEXT) \
    USE_GL_FUNC(glGetVertexArrayPointeri_vEXT) \
    USE_GL_FUNC(glGetVertexArrayPointervEXT) \
    USE_GL_FUNC(glGetVertexArrayiv) \
    USE_GL_FUNC(glGetVertexAttribArrayObjectfvATI) \
    USE_GL_FUNC(glGetVertexAttribArrayObjectivATI) \
    USE_GL_FUNC(glGetVertexAttribIiv) \
    USE_GL_FUNC(glGetVertexAttribIivEXT) \
    USE_GL_FUNC(glGetVertexAttribIuiv) \
    USE_GL_FUNC(glGetVertexAttribIuivEXT) \
    USE_GL_FUNC(glGetVertexAttribLdv) \
    USE_GL_FUNC(glGetVertexAttribLdvEXT) \
    USE_GL_FUNC(glGetVertexAttribLi64vNV) \
    USE_GL_FUNC(glGetVertexAttribLui64vARB) \
    USE_GL_FUNC(glGetVertexAttribLui64vNV) \
    USE_GL_FUNC(glGetVertexAttribPointerv) \
    USE_GL_FUNC(glGetVertexAttribPointervARB) \
    USE_GL_FUNC(glGetVertexAttribPointervNV) \
    USE_GL_FUNC(glGetVertexAttribdv) \
    USE_GL_FUNC(glGetVertexAttribdvARB) \
    USE_GL_FUNC(glGetVertexAttribdvNV) \
    USE_GL_FUNC(glGetVertexAttribfv) \
    USE_GL_FUNC(glGetVertexAttribfvARB) \
    USE_GL_FUNC(glGetVertexAttribfvNV) \
    USE_GL_FUNC(glGetVertexAttribiv) \
    USE_GL_FUNC(glGetVertexAttribivARB) \
    USE_GL_FUNC(glGetVertexAttribivNV) \
    USE_GL_FUNC(glGetVideoCaptureStreamdvNV) \
    USE_GL_FUNC(glGetVideoCaptureStreamfvNV) \
    USE_GL_FUNC(glGetVideoCaptureStreamivNV) \
    USE_GL_FUNC(glGetVideoCaptureivNV) \
    USE_GL_FUNC(glGetVideoi64vNV) \
    USE_GL_FUNC(glGetVideoivNV) \
    USE_GL_FUNC(glGetVideoui64vNV) \
    USE_GL_FUNC(glGetVideouivNV) \
    USE_GL_FUNC(glGetVkProcAddrNV) \
    USE_GL_FUNC(glGetnColorTable) \
    USE_GL_FUNC(glGetnColorTableARB) \
    USE_GL_FUNC(glGetnCompressedTexImage) \
    USE_GL_FUNC(glGetnCompressedTexImageARB) \
    USE_GL_FUNC(glGetnConvolutionFilter) \
    USE_GL_FUNC(glGetnConvolutionFilterARB) \
    USE_GL_FUNC(glGetnHistogram) \
    USE_GL_FUNC(glGetnHistogramARB) \
    USE_GL_FUNC(glGetnMapdv) \
    USE_GL_FUNC(glGetnMapdvARB) \
    USE_GL_FUNC(glGetnMapfv) \
    USE_GL_FUNC(glGetnMapfvARB) \
    USE_GL_FUNC(glGetnMapiv) \
    USE_GL_FUNC(glGetnMapivARB) \
    USE_GL_FUNC(glGetnMinmax) \
    USE_GL_FUNC(glGetnMinmaxARB) \
    USE_GL_FUNC(glGetnPixelMapfv) \
    USE_GL_FUNC(glGetnPixelMapfvARB) \
    USE_GL_FUNC(glGetnPixelMapuiv) \
    USE_GL_FUNC(glGetnPixelMapuivARB) \
    USE_GL_FUNC(glGetnPixelMapusv) \
    USE_GL_FUNC(glGetnPixelMapusvARB) \
    USE_GL_FUNC(glGetnPolygonStipple) \
    USE_GL_FUNC(glGetnPolygonStippleARB) \
    USE_GL_FUNC(glGetnSeparableFilter) \
    USE_GL_FUNC(glGetnSeparableFilterARB) \
    USE_GL_FUNC(glGetnTexImage) \
    USE_GL_FUNC(glGetnTexImageARB) \
    USE_GL_FUNC(glGetnUniformdv) \
    USE_GL_FUNC(glGetnUniformdvARB) \
    USE_GL_FUNC(glGetnUniformfv) \
    USE_GL_FUNC(glGetnUniformfvARB) \
    USE_GL_FUNC(glGetnUniformi64vARB) \
    USE_GL_FUNC(glGetnUniformiv) \
    USE_GL_FUNC(glGetnUniformivARB) \
    USE_GL_FUNC(glGetnUniformui64vARB) \
    USE_GL_FUNC(glGetnUniformuiv) \
    USE_GL_FUNC(glGetnUniformuivARB) \
    USE_GL_FUNC(glGlobalAlphaFactorbSUN) \
    USE_GL_FUNC(glGlobalAlphaFactordSUN) \
    USE_GL_FUNC(glGlobalAlphaFactorfSUN) \
    USE_GL_FUNC(glGlobalAlphaFactoriSUN) \
    USE_GL_FUNC(glGlobalAlphaFactorsSUN) \
    USE_GL_FUNC(glGlobalAlphaFactorubSUN) \
    USE_GL_FUNC(glGlobalAlphaFactoruiSUN) \
    USE_GL_FUNC(glGlobalAlphaFactorusSUN) \
    USE_GL_FUNC(glHintPGI) \
    USE_GL_FUNC(glHistogram) \
    USE_GL_FUNC(glHistogramEXT) \
    USE_GL_FUNC(glIglooInterfaceSGIX) \
    USE_GL_FUNC(glImageTransformParameterfHP) \
    USE_GL_FUNC(glImageTransformParameterfvHP) \
    USE_GL_FUNC(glImageTransformParameteriHP) \
    USE_GL_FUNC(glImageTransformParameterivHP) \
    USE_GL_FUNC(glImportMemoryFdEXT) \
    USE_GL_FUNC(glImportMemoryWin32HandleEXT) \
    USE_GL_FUNC(glImportMemoryWin32NameEXT) \
    USE_GL_FUNC(glImportSemaphoreFdEXT) \
    USE_GL_FUNC(glImportSemaphoreWin32HandleEXT) \
    USE_GL_FUNC(glImportSemaphoreWin32NameEXT) \
    USE_GL_FUNC(glImportSyncEXT) \
    USE_GL_FUNC(glIndexFormatNV) \
    USE_GL_FUNC(glIndexFuncEXT) \
    USE_GL_FUNC(glIndexMaterialEXT) \
    USE_GL_FUNC(glIndexPointerEXT) \
    USE_GL_FUNC(glIndexPointerListIBM) \
    USE_GL_FUNC(glIndexxOES) \
    USE_GL_FUNC(glIndexxvOES) \
    USE_GL_FUNC(glInsertComponentEXT) \
    USE_GL_FUNC(glInsertEventMarkerEXT) \
    USE_GL_FUNC(glInstrumentsBufferSGIX) \
    USE_GL_FUNC(glInterpolatePathsNV) \
    USE_GL_FUNC(glInvalidateBufferData) \
    USE_GL_FUNC(glInvalidateBufferSubData) \
    USE_GL_FUNC(glInvalidateFramebuffer) \
    USE_GL_FUNC(glInvalidateNamedFramebufferData) \
    USE_GL_FUNC(glInvalidateNamedFramebufferSubData) \
    USE_GL_FUNC(glInvalidateSubFramebuffer) \
    USE_GL_FUNC(glInvalidateTexImage) \
    USE_GL_FUNC(glInvalidateTexSubImage) \
    USE_GL_FUNC(glIsAsyncMarkerSGIX) \
    USE_GL_FUNC(glIsBuffer) \
    USE_GL_FUNC(glIsBufferARB) \
    USE_GL_FUNC(glIsBufferResidentNV) \
    USE_GL_FUNC(glIsCommandListNV) \
    USE_GL_FUNC(glIsEnabledIndexedEXT) \
    USE_GL_FUNC(glIsEnabledi) \
    USE_GL_FUNC(glIsFenceAPPLE) \
    USE_GL_FUNC(glIsFenceNV) \
    USE_GL_FUNC(glIsFramebuffer) \
    USE_GL_FUNC(glIsFramebufferEXT) \
    USE_GL_FUNC(glIsImageHandleResidentARB) \
    USE_GL_FUNC(glIsImageHandleResidentNV) \
    USE_GL_FUNC(glIsMemoryObjectEXT) \
    USE_GL_FUNC(glIsNameAMD) \
    USE_GL_FUNC(glIsNamedBufferResidentNV) \
    USE_GL_FUNC(glIsNamedStringARB) \
    USE_GL_FUNC(glIsObjectBufferATI) \
    USE_GL_FUNC(glIsOcclusionQueryNV) \
    USE_GL_FUNC(glIsPathNV) \
    USE_GL_FUNC(glIsPointInFillPathNV) \
    USE_GL_FUNC(glIsPointInStrokePathNV) \
    USE_GL_FUNC(glIsProgram) \
    USE_GL_FUNC(glIsProgramARB) \
    USE_GL_FUNC(glIsProgramNV) \
    USE_GL_FUNC(glIsProgramPipeline) \
    USE_GL_FUNC(glIsQuery) \
    USE_GL_FUNC(glIsQueryARB) \
    USE_GL_FUNC(glIsRenderbuffer) \
    USE_GL_FUNC(glIsRenderbufferEXT) \
    USE_GL_FUNC(glIsSampler) \
    USE_GL_FUNC(glIsSemaphoreEXT) \
    USE_GL_FUNC(glIsShader) \
    USE_GL_FUNC(glIsStateNV) \
    USE_GL_FUNC(glIsSync) \
    USE_GL_FUNC(glIsTextureEXT) \
    USE_GL_FUNC(glIsTextureHandleResidentARB) \
    USE_GL_FUNC(glIsTextureHandleResidentNV) \
    USE_GL_FUNC(glIsTransformFeedback) \
    USE_GL_FUNC(glIsTransformFeedbackNV) \
    USE_GL_FUNC(glIsVariantEnabledEXT) \
    USE_GL_FUNC(glIsVertexArray) \
    USE_GL_FUNC(glIsVertexArrayAPPLE) \
    USE_GL_FUNC(glIsVertexAttribEnabledAPPLE) \
    USE_GL_FUNC(glLGPUCopyImageSubDataNVX) \
    USE_GL_FUNC(glLGPUInterlockNVX) \
    USE_GL_FUNC(glLGPUNamedBufferSubDataNVX) \
    USE_GL_FUNC(glLabelObjectEXT) \
    USE_GL_FUNC(glLightEnviSGIX) \
    USE_GL_FUNC(glLightModelxOES) \
    USE_GL_FUNC(glLightModelxvOES) \
    USE_GL_FUNC(glLightxOES) \
    USE_GL_FUNC(glLightxvOES) \
    USE_GL_FUNC(glLineWidthxOES) \
    USE_GL_FUNC(glLinkProgram) \
    USE_GL_FUNC(glLinkProgramARB) \
    USE_GL_FUNC(glListDrawCommandsStatesClientNV) \
    USE_GL_FUNC(glListParameterfSGIX) \
    USE_GL_FUNC(glListParameterfvSGIX) \
    USE_GL_FUNC(glListParameteriSGIX) \
    USE_GL_FUNC(glListParameterivSGIX) \
    USE_GL_FUNC(glLoadIdentityDeformationMapSGIX) \
    USE_GL_FUNC(glLoadMatrixxOES) \
    USE_GL_FUNC(glLoadProgramNV) \
    USE_GL_FUNC(glLoadTransposeMatrixd) \
    USE_GL_FUNC(glLoadTransposeMatrixdARB) \
    USE_GL_FUNC(glLoadTransposeMatrixf) \
    USE_GL_FUNC(glLoadTransposeMatrixfARB) \
    USE_GL_FUNC(glLoadTransposeMatrixxOES) \
    USE_GL_FUNC(glLockArraysEXT) \
    USE_GL_FUNC(glMTexCoord2fSGIS) \
    USE_GL_FUNC(glMTexCoord2fvSGIS) \
    USE_GL_FUNC(glMakeBufferNonResidentNV) \
    USE_GL_FUNC(glMakeBufferResidentNV) \
    USE_GL_FUNC(glMakeImageHandleNonResidentARB) \
    USE_GL_FUNC(glMakeImageHandleNonResidentNV) \
    USE_GL_FUNC(glMakeImageHandleResidentARB) \
    USE_GL_FUNC(glMakeImageHandleResidentNV) \
    USE_GL_FUNC(glMakeNamedBufferNonResidentNV) \
    USE_GL_FUNC(glMakeNamedBufferResidentNV) \
    USE_GL_FUNC(glMakeTextureHandleNonResidentARB) \
    USE_GL_FUNC(glMakeTextureHandleNonResidentNV) \
    USE_GL_FUNC(glMakeTextureHandleResidentARB) \
    USE_GL_FUNC(glMakeTextureHandleResidentNV) \
    USE_GL_FUNC(glMap1xOES) \
    USE_GL_FUNC(glMap2xOES) \
    USE_GL_FUNC(glMapBuffer) \
    USE_GL_FUNC(glMapBufferARB) \
    USE_GL_FUNC(glMapBufferRange) \
    USE_GL_FUNC(glMapControlPointsNV) \
    USE_GL_FUNC(glMapGrid1xOES) \
    USE_GL_FUNC(glMapGrid2xOES) \
    USE_GL_FUNC(glMapNamedBuffer) \
    USE_GL_FUNC(glMapNamedBufferEXT) \
    USE_GL_FUNC(glMapNamedBufferRange) \
    USE_GL_FUNC(glMapNamedBufferRangeEXT) \
    USE_GL_FUNC(glMapObjectBufferATI) \
    USE_GL_FUNC(glMapParameterfvNV) \
    USE_GL_FUNC(glMapParameterivNV) \
    USE_GL_FUNC(glMapTexture2DINTEL) \
    USE_GL_FUNC(glMapVertexAttrib1dAPPLE) \
    USE_GL_FUNC(glMapVertexAttrib1fAPPLE) \
    USE_GL_FUNC(glMapVertexAttrib2dAPPLE) \
    USE_GL_FUNC(glMapVertexAttrib2fAPPLE) \
    USE_GL_FUNC(glMaterialxOES) \
    USE_GL_FUNC(glMaterialxvOES) \
    USE_GL_FUNC(glMatrixFrustumEXT) \
    USE_GL_FUNC(glMatrixIndexPointerARB) \
    USE_GL_FUNC(glMatrixIndexubvARB) \
    USE_GL_FUNC(glMatrixIndexuivARB) \
    USE_GL_FUNC(glMatrixIndexusvARB) \
    USE_GL_FUNC(glMatrixLoad3x2fNV) \
    USE_GL_FUNC(glMatrixLoad3x3fNV) \
    USE_GL_FUNC(glMatrixLoadIdentityEXT) \
    USE_GL_FUNC(glMatrixLoadTranspose3x3fNV) \
    USE_GL_FUNC(glMatrixLoadTransposedEXT) \
    USE_GL_FUNC(glMatrixLoadTransposefEXT) \
    USE_GL_FUNC(glMatrixLoaddEXT) \
    USE_GL_FUNC(glMatrixLoadfEXT) \
    USE_GL_FUNC(glMatrixMult3x2fNV) \
    USE_GL_FUNC(glMatrixMult3x3fNV) \
    USE_GL_FUNC(glMatrixMultTranspose3x3fNV) \
    USE_GL_FUNC(glMatrixMultTransposedEXT) \
    USE_GL_FUNC(glMatrixMultTransposefEXT) \
    USE_GL_FUNC(glMatrixMultdEXT) \
    USE_GL_FUNC(glMatrixMultfEXT) \
    USE_GL_FUNC(glMatrixOrthoEXT) \
    USE_GL_FUNC(glMatrixPopEXT) \
    USE_GL_FUNC(glMatrixPushEXT) \
    USE_GL_FUNC(glMatrixRotatedEXT) \
    USE_GL_FUNC(glMatrixRotatefEXT) \
    USE_GL_FUNC(glMatrixScaledEXT) \
    USE_GL_FUNC(glMatrixScalefEXT) \
    USE_GL_FUNC(glMatrixTranslatedEXT) \
    USE_GL_FUNC(glMatrixTranslatefEXT) \
    USE_GL_FUNC(glMaxShaderCompilerThreadsARB) \
    USE_GL_FUNC(glMaxShaderCompilerThreadsKHR) \
    USE_GL_FUNC(glMemoryBarrier) \
    USE_GL_FUNC(glMemoryBarrierByRegion) \
    USE_GL_FUNC(glMemoryBarrierEXT) \
    USE_GL_FUNC(glMemoryObjectParameterivEXT) \
    USE_GL_FUNC(glMinSampleShading) \
    USE_GL_FUNC(glMinSampleShadingARB) \
    USE_GL_FUNC(glMinmax) \
    USE_GL_FUNC(glMinmaxEXT) \
    USE_GL_FUNC(glMultMatrixxOES) \
    USE_GL_FUNC(glMultTransposeMatrixd) \
    USE_GL_FUNC(glMultTransposeMatrixdARB) \
    USE_GL_FUNC(glMultTransposeMatrixf) \
    USE_GL_FUNC(glMultTransposeMatrixfARB) \
    USE_GL_FUNC(glMultTransposeMatrixxOES) \
    USE_GL_FUNC(glMultiDrawArrays) \
    USE_GL_FUNC(glMultiDrawArraysEXT) \
    USE_GL_FUNC(glMultiDrawArraysIndirect) \
    USE_GL_FUNC(glMultiDrawArraysIndirectAMD) \
    USE_GL_FUNC(glMultiDrawArraysIndirectBindlessCountNV) \
    USE_GL_FUNC(glMultiDrawArraysIndirectBindlessNV) \
    USE_GL_FUNC(glMultiDrawArraysIndirectCount) \
    USE_GL_FUNC(glMultiDrawArraysIndirectCountARB) \
    USE_GL_FUNC(glMultiDrawElementArrayAPPLE) \
    USE_GL_FUNC(glMultiDrawElements) \
    USE_GL_FUNC(glMultiDrawElementsBaseVertex) \
    USE_GL_FUNC(glMultiDrawElementsEXT) \
    USE_GL_FUNC(glMultiDrawElementsIndirect) \
    USE_GL_FUNC(glMultiDrawElementsIndirectAMD) \
    USE_GL_FUNC(glMultiDrawElementsIndirectBindlessCountNV) \
    USE_GL_FUNC(glMultiDrawElementsIndirectBindlessNV) \
    USE_GL_FUNC(glMultiDrawElementsIndirectCount) \
    USE_GL_FUNC(glMultiDrawElementsIndirectCountARB) \
    USE_GL_FUNC(glMultiDrawMeshTasksIndirectCountNV) \
    USE_GL_FUNC(glMultiDrawMeshTasksIndirectNV) \
    USE_GL_FUNC(glMultiDrawRangeElementArrayAPPLE) \
    USE_GL_FUNC(glMultiModeDrawArraysIBM) \
    USE_GL_FUNC(glMultiModeDrawElementsIBM) \
    USE_GL_FUNC(glMultiTexBufferEXT) \
    USE_GL_FUNC(glMultiTexCoord1bOES) \
    USE_GL_FUNC(glMultiTexCoord1bvOES) \
    USE_GL_FUNC(glMultiTexCoord1d) \
    USE_GL_FUNC(glMultiTexCoord1dARB) \
    USE_GL_FUNC(glMultiTexCoord1dSGIS) \
    USE_GL_FUNC(glMultiTexCoord1dv) \
    USE_GL_FUNC(glMultiTexCoord1dvARB) \
    USE_GL_FUNC(glMultiTexCoord1dvSGIS) \
    USE_GL_FUNC(glMultiTexCoord1f) \
    USE_GL_FUNC(glMultiTexCoord1fARB) \
    USE_GL_FUNC(glMultiTexCoord1fSGIS) \
    USE_GL_FUNC(glMultiTexCoord1fv) \
    USE_GL_FUNC(glMultiTexCoord1fvARB) \
    USE_GL_FUNC(glMultiTexCoord1fvSGIS) \
    USE_GL_FUNC(glMultiTexCoord1hNV) \
    USE_GL_FUNC(glMultiTexCoord1hvNV) \
    USE_GL_FUNC(glMultiTexCoord1i) \
    USE_GL_FUNC(glMultiTexCoord1iARB) \
    USE_GL_FUNC(glMultiTexCoord1iSGIS) \
    USE_GL_FUNC(glMultiTexCoord1iv) \
    USE_GL_FUNC(glMultiTexCoord1ivARB) \
    USE_GL_FUNC(glMultiTexCoord1ivSGIS) \
    USE_GL_FUNC(glMultiTexCoord1s) \
    USE_GL_FUNC(glMultiTexCoord1sARB) \
    USE_GL_FUNC(glMultiTexCoord1sSGIS) \
    USE_GL_FUNC(glMultiTexCoord1sv) \
    USE_GL_FUNC(glMultiTexCoord1svARB) \
    USE_GL_FUNC(glMultiTexCoord1svSGIS) \
    USE_GL_FUNC(glMultiTexCoord1xOES) \
    USE_GL_FUNC(glMultiTexCoord1xvOES) \
    USE_GL_FUNC(glMultiTexCoord2bOES) \
    USE_GL_FUNC(glMultiTexCoord2bvOES) \
    USE_GL_FUNC(glMultiTexCoord2d) \
    USE_GL_FUNC(glMultiTexCoord2dARB) \
    USE_GL_FUNC(glMultiTexCoord2dSGIS) \
    USE_GL_FUNC(glMultiTexCoord2dv) \
    USE_GL_FUNC(glMultiTexCoord2dvARB) \
    USE_GL_FUNC(glMultiTexCoord2dvSGIS) \
    USE_GL_FUNC(glMultiTexCoord2f) \
    USE_GL_FUNC(glMultiTexCoord2fARB) \
    USE_GL_FUNC(glMultiTexCoord2fSGIS) \
    USE_GL_FUNC(glMultiTexCoord2fv) \
    USE_GL_FUNC(glMultiTexCoord2fvARB) \
    USE_GL_FUNC(glMultiTexCoord2fvSGIS) \
    USE_GL_FUNC(glMultiTexCoord2hNV) \
    USE_GL_FUNC(glMultiTexCoord2hvNV) \
    USE_GL_FUNC(glMultiTexCoord2i) \
    USE_GL_FUNC(glMultiTexCoord2iARB) \
    USE_GL_FUNC(glMultiTexCoord2iSGIS) \
    USE_GL_FUNC(glMultiTexCoord2iv) \
    USE_GL_FUNC(glMultiTexCoord2ivARB) \
    USE_GL_FUNC(glMultiTexCoord2ivSGIS) \
    USE_GL_FUNC(glMultiTexCoord2s) \
    USE_GL_FUNC(glMultiTexCoord2sARB) \
    USE_GL_FUNC(glMultiTexCoord2sSGIS) \
    USE_GL_FUNC(glMultiTexCoord2sv) \
    USE_GL_FUNC(glMultiTexCoord2svARB) \
    USE_GL_FUNC(glMultiTexCoord2svSGIS) \
    USE_GL_FUNC(glMultiTexCoord2xOES) \
    USE_GL_FUNC(glMultiTexCoord2xvOES) \
    USE_GL_FUNC(glMultiTexCoord3bOES) \
    USE_GL_FUNC(glMultiTexCoord3bvOES) \
    USE_GL_FUNC(glMultiTexCoord3d) \
    USE_GL_FUNC(glMultiTexCoord3dARB) \
    USE_GL_FUNC(glMultiTexCoord3dSGIS) \
    USE_GL_FUNC(glMultiTexCoord3dv) \
    USE_GL_FUNC(glMultiTexCoord3dvARB) \
    USE_GL_FUNC(glMultiTexCoord3dvSGIS) \
    USE_GL_FUNC(glMultiTexCoord3f) \
    USE_GL_FUNC(glMultiTexCoord3fARB) \
    USE_GL_FUNC(glMultiTexCoord3fSGIS) \
    USE_GL_FUNC(glMultiTexCoord3fv) \
    USE_GL_FUNC(glMultiTexCoord3fvARB) \
    USE_GL_FUNC(glMultiTexCoord3fvSGIS) \
    USE_GL_FUNC(glMultiTexCoord3hNV) \
    USE_GL_FUNC(glMultiTexCoord3hvNV) \
    USE_GL_FUNC(glMultiTexCoord3i) \
    USE_GL_FUNC(glMultiTexCoord3iARB) \
    USE_GL_FUNC(glMultiTexCoord3iSGIS) \
    USE_GL_FUNC(glMultiTexCoord3iv) \
    USE_GL_FUNC(glMultiTexCoord3ivARB) \
    USE_GL_FUNC(glMultiTexCoord3ivSGIS) \
    USE_GL_FUNC(glMultiTexCoord3s) \
    USE_GL_FUNC(glMultiTexCoord3sARB) \
    USE_GL_FUNC(glMultiTexCoord3sSGIS) \
    USE_GL_FUNC(glMultiTexCoord3sv) \
    USE_GL_FUNC(glMultiTexCoord3svARB) \
    USE_GL_FUNC(glMultiTexCoord3svSGIS) \
    USE_GL_FUNC(glMultiTexCoord3xOES) \
    USE_GL_FUNC(glMultiTexCoord3xvOES) \
    USE_GL_FUNC(glMultiTexCoord4bOES) \
    USE_GL_FUNC(glMultiTexCoord4bvOES) \
    USE_GL_FUNC(glMultiTexCoord4d) \
    USE_GL_FUNC(glMultiTexCoord4dARB) \
    USE_GL_FUNC(glMultiTexCoord4dSGIS) \
    USE_GL_FUNC(glMultiTexCoord4dv) \
    USE_GL_FUNC(glMultiTexCoord4dvARB) \
    USE_GL_FUNC(glMultiTexCoord4dvSGIS) \
    USE_GL_FUNC(glMultiTexCoord4f) \
    USE_GL_FUNC(glMultiTexCoord4fARB) \
    USE_GL_FUNC(glMultiTexCoord4fSGIS) \
    USE_GL_FUNC(glMultiTexCoord4fv) \
    USE_GL_FUNC(glMultiTexCoord4fvARB) \
    USE_GL_FUNC(glMultiTexCoord4fvSGIS) \
    USE_GL_FUNC(glMultiTexCoord4hNV) \
    USE_GL_FUNC(glMultiTexCoord4hvNV) \
    USE_GL_FUNC(glMultiTexCoord4i) \
    USE_GL_FUNC(glMultiTexCoord4iARB) \
    USE_GL_FUNC(glMultiTexCoord4iSGIS) \
    USE_GL_FUNC(glMultiTexCoord4iv) \
    USE_GL_FUNC(glMultiTexCoord4ivARB) \
    USE_GL_FUNC(glMultiTexCoord4ivSGIS) \
    USE_GL_FUNC(glMultiTexCoord4s) \
    USE_GL_FUNC(glMultiTexCoord4sARB) \
    USE_GL_FUNC(glMultiTexCoord4sSGIS) \
    USE_GL_FUNC(glMultiTexCoord4sv) \
    USE_GL_FUNC(glMultiTexCoord4svARB) \
    USE_GL_FUNC(glMultiTexCoord4svSGIS) \
    USE_GL_FUNC(glMultiTexCoord4xOES) \
    USE_GL_FUNC(glMultiTexCoord4xvOES) \
    USE_GL_FUNC(glMultiTexCoordP1ui) \
    USE_GL_FUNC(glMultiTexCoordP1uiv) \
    USE_GL_FUNC(glMultiTexCoordP2ui) \
    USE_GL_FUNC(glMultiTexCoordP2uiv) \
    USE_GL_FUNC(glMultiTexCoordP3ui) \
    USE_GL_FUNC(glMultiTexCoordP3uiv) \
    USE_GL_FUNC(glMultiTexCoordP4ui) \
    USE_GL_FUNC(glMultiTexCoordP4uiv) \
    USE_GL_FUNC(glMultiTexCoordPointerEXT) \
    USE_GL_FUNC(glMultiTexCoordPointerSGIS) \
    USE_GL_FUNC(glMultiTexEnvfEXT) \
    USE_GL_FUNC(glMultiTexEnvfvEXT) \
    USE_GL_FUNC(glMultiTexEnviEXT) \
    USE_GL_FUNC(glMultiTexEnvivEXT) \
    USE_GL_FUNC(glMultiTexGendEXT) \
    USE_GL_FUNC(glMultiTexGendvEXT) \
    USE_GL_FUNC(glMultiTexGenfEXT) \
    USE_GL_FUNC(glMultiTexGenfvEXT) \
    USE_GL_FUNC(glMultiTexGeniEXT) \
    USE_GL_FUNC(glMultiTexGenivEXT) \
    USE_GL_FUNC(glMultiTexImage1DEXT) \
    USE_GL_FUNC(glMultiTexImage2DEXT) \
    USE_GL_FUNC(glMultiTexImage3DEXT) \
    USE_GL_FUNC(glMultiTexParameterIivEXT) \
    USE_GL_FUNC(glMultiTexParameterIuivEXT) \
    USE_GL_FUNC(glMultiTexParameterfEXT) \
    USE_GL_FUNC(glMultiTexParameterfvEXT) \
    USE_GL_FUNC(glMultiTexParameteriEXT) \
    USE_GL_FUNC(glMultiTexParameterivEXT) \
    USE_GL_FUNC(glMultiTexRenderbufferEXT) \
    USE_GL_FUNC(glMultiTexSubImage1DEXT) \
    USE_GL_FUNC(glMultiTexSubImage2DEXT) \
    USE_GL_FUNC(glMultiTexSubImage3DEXT) \
    USE_GL_FUNC(glMulticastBarrierNV) \
    USE_GL_FUNC(glMulticastBlitFramebufferNV) \
    USE_GL_FUNC(glMulticastBufferSubDataNV) \
    USE_GL_FUNC(glMulticastCopyBufferSubDataNV) \
    USE_GL_FUNC(glMulticastCopyImageSubDataNV) \
    USE_GL_FUNC(glMulticastFramebufferSampleLocationsfvNV) \
    USE_GL_FUNC(glMulticastGetQueryObjecti64vNV) \
    USE_GL_FUNC(glMulticastGetQueryObjectivNV) \
    USE_GL_FUNC(glMulticastGetQueryObjectui64vNV) \
    USE_GL_FUNC(glMulticastGetQueryObjectuivNV) \
    USE_GL_FUNC(glMulticastScissorArrayvNVX) \
    USE_GL_FUNC(glMulticastViewportArrayvNVX) \
    USE_GL_FUNC(glMulticastViewportPositionWScaleNVX) \
    USE_GL_FUNC(glMulticastWaitSyncNV) \
    USE_GL_FUNC(glNamedBufferAttachMemoryNV) \
    USE_GL_FUNC(glNamedBufferData) \
    USE_GL_FUNC(glNamedBufferDataEXT) \
    USE_GL_FUNC(glNamedBufferPageCommitmentARB) \
    USE_GL_FUNC(glNamedBufferPageCommitmentEXT) \
    USE_GL_FUNC(glNamedBufferStorage) \
    USE_GL_FUNC(glNamedBufferStorageEXT) \
    USE_GL_FUNC(glNamedBufferStorageExternalEXT) \
    USE_GL_FUNC(glNamedBufferStorageMemEXT) \
    USE_GL_FUNC(glNamedBufferSubData) \
    USE_GL_FUNC(glNamedBufferSubDataEXT) \
    USE_GL_FUNC(glNamedCopyBufferSubDataEXT) \
    USE_GL_FUNC(glNamedFramebufferDrawBuffer) \
    USE_GL_FUNC(glNamedFramebufferDrawBuffers) \
    USE_GL_FUNC(glNamedFramebufferParameteri) \
    USE_GL_FUNC(glNamedFramebufferParameteriEXT) \
    USE_GL_FUNC(glNamedFramebufferReadBuffer) \
    USE_GL_FUNC(glNamedFramebufferRenderbuffer) \
    USE_GL_FUNC(glNamedFramebufferRenderbufferEXT) \
    USE_GL_FUNC(glNamedFramebufferSampleLocationsfvARB) \
    USE_GL_FUNC(glNamedFramebufferSampleLocationsfvNV) \
    USE_GL_FUNC(glNamedFramebufferSamplePositionsfvAMD) \
    USE_GL_FUNC(glNamedFramebufferTexture) \
    USE_GL_FUNC(glNamedFramebufferTexture1DEXT) \
    USE_GL_FUNC(glNamedFramebufferTexture2DEXT) \
    USE_GL_FUNC(glNamedFramebufferTexture3DEXT) \
    USE_GL_FUNC(glNamedFramebufferTextureEXT) \
    USE_GL_FUNC(glNamedFramebufferTextureFaceEXT) \
    USE_GL_FUNC(glNamedFramebufferTextureLayer) \
    USE_GL_FUNC(glNamedFramebufferTextureLayerEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameter4dEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameter4dvEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameter4fEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameter4fvEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameterI4iEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameterI4ivEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameterI4uiEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameterI4uivEXT) \
    USE_GL_FUNC(glNamedProgramLocalParameters4fvEXT) \
    USE_GL_FUNC(glNamedProgramLocalParametersI4ivEXT) \
    USE_GL_FUNC(glNamedProgramLocalParametersI4uivEXT) \
    USE_GL_FUNC(glNamedProgramStringEXT) \
    USE_GL_FUNC(glNamedRenderbufferStorage) \
    USE_GL_FUNC(glNamedRenderbufferStorageEXT) \
    USE_GL_FUNC(glNamedRenderbufferStorageMultisample) \
    USE_GL_FUNC(glNamedRenderbufferStorageMultisampleAdvancedAMD) \
    USE_GL_FUNC(glNamedRenderbufferStorageMultisampleCoverageEXT) \
    USE_GL_FUNC(glNamedRenderbufferStorageMultisampleEXT) \
    USE_GL_FUNC(glNamedStringARB) \
    USE_GL_FUNC(glNewBufferRegion) \
    USE_GL_FUNC(glNewObjectBufferATI) \
    USE_GL_FUNC(glNormal3fVertex3fSUN) \
    USE_GL_FUNC(glNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glNormal3hNV) \
    USE_GL_FUNC(glNormal3hvNV) \
    USE_GL_FUNC(glNormal3xOES) \
    USE_GL_FUNC(glNormal3xvOES) \
    USE_GL_FUNC(glNormalFormatNV) \
    USE_GL_FUNC(glNormalP3ui) \
    USE_GL_FUNC(glNormalP3uiv) \
    USE_GL_FUNC(glNormalPointerEXT) \
    USE_GL_FUNC(glNormalPointerListIBM) \
    USE_GL_FUNC(glNormalPointervINTEL) \
    USE_GL_FUNC(glNormalStream3bATI) \
    USE_GL_FUNC(glNormalStream3bvATI) \
    USE_GL_FUNC(glNormalStream3dATI) \
    USE_GL_FUNC(glNormalStream3dvATI) \
    USE_GL_FUNC(glNormalStream3fATI) \
    USE_GL_FUNC(glNormalStream3fvATI) \
    USE_GL_FUNC(glNormalStream3iATI) \
    USE_GL_FUNC(glNormalStream3ivATI) \
    USE_GL_FUNC(glNormalStream3sATI) \
    USE_GL_FUNC(glNormalStream3svATI) \
    USE_GL_FUNC(glObjectLabel) \
    USE_GL_FUNC(glObjectPtrLabel) \
    USE_GL_FUNC(glObjectPurgeableAPPLE) \
    USE_GL_FUNC(glObjectUnpurgeableAPPLE) \
    USE_GL_FUNC(glOrthofOES) \
    USE_GL_FUNC(glOrthoxOES) \
    USE_GL_FUNC(glPNTrianglesfATI) \
    USE_GL_FUNC(glPNTrianglesiATI) \
    USE_GL_FUNC(glPassTexCoordATI) \
    USE_GL_FUNC(glPassThroughxOES) \
    USE_GL_FUNC(glPatchParameterfv) \
    USE_GL_FUNC(glPatchParameteri) \
    USE_GL_FUNC(glPathColorGenNV) \
    USE_GL_FUNC(glPathCommandsNV) \
    USE_GL_FUNC(glPathCoordsNV) \
    USE_GL_FUNC(glPathCoverDepthFuncNV) \
    USE_GL_FUNC(glPathDashArrayNV) \
    USE_GL_FUNC(glPathFogGenNV) \
    USE_GL_FUNC(glPathGlyphIndexArrayNV) \
    USE_GL_FUNC(glPathGlyphIndexRangeNV) \
    USE_GL_FUNC(glPathGlyphRangeNV) \
    USE_GL_FUNC(glPathGlyphsNV) \
    USE_GL_FUNC(glPathMemoryGlyphIndexArrayNV) \
    USE_GL_FUNC(glPathParameterfNV) \
    USE_GL_FUNC(glPathParameterfvNV) \
    USE_GL_FUNC(glPathParameteriNV) \
    USE_GL_FUNC(glPathParameterivNV) \
    USE_GL_FUNC(glPathStencilDepthOffsetNV) \
    USE_GL_FUNC(glPathStencilFuncNV) \
    USE_GL_FUNC(glPathStringNV) \
    USE_GL_FUNC(glPathSubCommandsNV) \
    USE_GL_FUNC(glPathSubCoordsNV) \
    USE_GL_FUNC(glPathTexGenNV) \
    USE_GL_FUNC(glPauseTransformFeedback) \
    USE_GL_FUNC(glPauseTransformFeedbackNV) \
    USE_GL_FUNC(glPixelDataRangeNV) \
    USE_GL_FUNC(glPixelMapx) \
    USE_GL_FUNC(glPixelStorex) \
    USE_GL_FUNC(glPixelTexGenParameterfSGIS) \
    USE_GL_FUNC(glPixelTexGenParameterfvSGIS) \
    USE_GL_FUNC(glPixelTexGenParameteriSGIS) \
    USE_GL_FUNC(glPixelTexGenParameterivSGIS) \
    USE_GL_FUNC(glPixelTexGenSGIX) \
    USE_GL_FUNC(glPixelTransferxOES) \
    USE_GL_FUNC(glPixelTransformParameterfEXT) \
    USE_GL_FUNC(glPixelTransformParameterfvEXT) \
    USE_GL_FUNC(glPixelTransformParameteriEXT) \
    USE_GL_FUNC(glPixelTransformParameterivEXT) \
    USE_GL_FUNC(glPixelZoomxOES) \
    USE_GL_FUNC(glPointAlongPathNV) \
    USE_GL_FUNC(glPointParameterf) \
    USE_GL_FUNC(glPointParameterfARB) \
    USE_GL_FUNC(glPointParameterfEXT) \
    USE_GL_FUNC(glPointParameterfSGIS) \
    USE_GL_FUNC(glPointParameterfv) \
    USE_GL_FUNC(glPointParameterfvARB) \
    USE_GL_FUNC(glPointParameterfvEXT) \
    USE_GL_FUNC(glPointParameterfvSGIS) \
    USE_GL_FUNC(glPointParameteri) \
    USE_GL_FUNC(glPointParameteriNV) \
    USE_GL_FUNC(glPointParameteriv) \
    USE_GL_FUNC(glPointParameterivNV) \
    USE_GL_FUNC(glPointParameterxvOES) \
    USE_GL_FUNC(glPointSizexOES) \
    USE_GL_FUNC(glPollAsyncSGIX) \
    USE_GL_FUNC(glPollInstrumentsSGIX) \
    USE_GL_FUNC(glPolygonOffsetClamp) \
    USE_GL_FUNC(glPolygonOffsetClampEXT) \
    USE_GL_FUNC(glPolygonOffsetEXT) \
    USE_GL_FUNC(glPolygonOffsetxOES) \
    USE_GL_FUNC(glPopDebugGroup) \
    USE_GL_FUNC(glPopGroupMarkerEXT) \
    USE_GL_FUNC(glPresentFrameDualFillNV) \
    USE_GL_FUNC(glPresentFrameKeyedNV) \
    USE_GL_FUNC(glPrimitiveBoundingBoxARB) \
    USE_GL_FUNC(glPrimitiveRestartIndex) \
    USE_GL_FUNC(glPrimitiveRestartIndexNV) \
    USE_GL_FUNC(glPrimitiveRestartNV) \
    USE_GL_FUNC(glPrioritizeTexturesEXT) \
    USE_GL_FUNC(glPrioritizeTexturesxOES) \
    USE_GL_FUNC(glProgramBinary) \
    USE_GL_FUNC(glProgramBufferParametersIivNV) \
    USE_GL_FUNC(glProgramBufferParametersIuivNV) \
    USE_GL_FUNC(glProgramBufferParametersfvNV) \
    USE_GL_FUNC(glProgramEnvParameter4dARB) \
    USE_GL_FUNC(glProgramEnvParameter4dvARB) \
    USE_GL_FUNC(glProgramEnvParameter4fARB) \
    USE_GL_FUNC(glProgramEnvParameter4fvARB) \
    USE_GL_FUNC(glProgramEnvParameterI4iNV) \
    USE_GL_FUNC(glProgramEnvParameterI4ivNV) \
    USE_GL_FUNC(glProgramEnvParameterI4uiNV) \
    USE_GL_FUNC(glProgramEnvParameterI4uivNV) \
    USE_GL_FUNC(glProgramEnvParameters4fvEXT) \
    USE_GL_FUNC(glProgramEnvParametersI4ivNV) \
    USE_GL_FUNC(glProgramEnvParametersI4uivNV) \
    USE_GL_FUNC(glProgramLocalParameter4dARB) \
    USE_GL_FUNC(glProgramLocalParameter4dvARB) \
    USE_GL_FUNC(glProgramLocalParameter4fARB) \
    USE_GL_FUNC(glProgramLocalParameter4fvARB) \
    USE_GL_FUNC(glProgramLocalParameterI4iNV) \
    USE_GL_FUNC(glProgramLocalParameterI4ivNV) \
    USE_GL_FUNC(glProgramLocalParameterI4uiNV) \
    USE_GL_FUNC(glProgramLocalParameterI4uivNV) \
    USE_GL_FUNC(glProgramLocalParameters4fvEXT) \
    USE_GL_FUNC(glProgramLocalParametersI4ivNV) \
    USE_GL_FUNC(glProgramLocalParametersI4uivNV) \
    USE_GL_FUNC(glProgramNamedParameter4dNV) \
    USE_GL_FUNC(glProgramNamedParameter4dvNV) \
    USE_GL_FUNC(glProgramNamedParameter4fNV) \
    USE_GL_FUNC(glProgramNamedParameter4fvNV) \
    USE_GL_FUNC(glProgramParameter4dNV) \
    USE_GL_FUNC(glProgramParameter4dvNV) \
    USE_GL_FUNC(glProgramParameter4fNV) \
    USE_GL_FUNC(glProgramParameter4fvNV) \
    USE_GL_FUNC(glProgramParameteri) \
    USE_GL_FUNC(glProgramParameteriARB) \
    USE_GL_FUNC(glProgramParameteriEXT) \
    USE_GL_FUNC(glProgramParameters4dvNV) \
    USE_GL_FUNC(glProgramParameters4fvNV) \
    USE_GL_FUNC(glProgramPathFragmentInputGenNV) \
    USE_GL_FUNC(glProgramStringARB) \
    USE_GL_FUNC(glProgramSubroutineParametersuivNV) \
    USE_GL_FUNC(glProgramUniform1d) \
    USE_GL_FUNC(glProgramUniform1dEXT) \
    USE_GL_FUNC(glProgramUniform1dv) \
    USE_GL_FUNC(glProgramUniform1dvEXT) \
    USE_GL_FUNC(glProgramUniform1f) \
    USE_GL_FUNC(glProgramUniform1fEXT) \
    USE_GL_FUNC(glProgramUniform1fv) \
    USE_GL_FUNC(glProgramUniform1fvEXT) \
    USE_GL_FUNC(glProgramUniform1i) \
    USE_GL_FUNC(glProgramUniform1i64ARB) \
    USE_GL_FUNC(glProgramUniform1i64NV) \
    USE_GL_FUNC(glProgramUniform1i64vARB) \
    USE_GL_FUNC(glProgramUniform1i64vNV) \
    USE_GL_FUNC(glProgramUniform1iEXT) \
    USE_GL_FUNC(glProgramUniform1iv) \
    USE_GL_FUNC(glProgramUniform1ivEXT) \
    USE_GL_FUNC(glProgramUniform1ui) \
    USE_GL_FUNC(glProgramUniform1ui64ARB) \
    USE_GL_FUNC(glProgramUniform1ui64NV) \
    USE_GL_FUNC(glProgramUniform1ui64vARB) \
    USE_GL_FUNC(glProgramUniform1ui64vNV) \
    USE_GL_FUNC(glProgramUniform1uiEXT) \
    USE_GL_FUNC(glProgramUniform1uiv) \
    USE_GL_FUNC(glProgramUniform1uivEXT) \
    USE_GL_FUNC(glProgramUniform2d) \
    USE_GL_FUNC(glProgramUniform2dEXT) \
    USE_GL_FUNC(glProgramUniform2dv) \
    USE_GL_FUNC(glProgramUniform2dvEXT) \
    USE_GL_FUNC(glProgramUniform2f) \
    USE_GL_FUNC(glProgramUniform2fEXT) \
    USE_GL_FUNC(glProgramUniform2fv) \
    USE_GL_FUNC(glProgramUniform2fvEXT) \
    USE_GL_FUNC(glProgramUniform2i) \
    USE_GL_FUNC(glProgramUniform2i64ARB) \
    USE_GL_FUNC(glProgramUniform2i64NV) \
    USE_GL_FUNC(glProgramUniform2i64vARB) \
    USE_GL_FUNC(glProgramUniform2i64vNV) \
    USE_GL_FUNC(glProgramUniform2iEXT) \
    USE_GL_FUNC(glProgramUniform2iv) \
    USE_GL_FUNC(glProgramUniform2ivEXT) \
    USE_GL_FUNC(glProgramUniform2ui) \
    USE_GL_FUNC(glProgramUniform2ui64ARB) \
    USE_GL_FUNC(glProgramUniform2ui64NV) \
    USE_GL_FUNC(glProgramUniform2ui64vARB) \
    USE_GL_FUNC(glProgramUniform2ui64vNV) \
    USE_GL_FUNC(glProgramUniform2uiEXT) \
    USE_GL_FUNC(glProgramUniform2uiv) \
    USE_GL_FUNC(glProgramUniform2uivEXT) \
    USE_GL_FUNC(glProgramUniform3d) \
    USE_GL_FUNC(glProgramUniform3dEXT) \
    USE_GL_FUNC(glProgramUniform3dv) \
    USE_GL_FUNC(glProgramUniform3dvEXT) \
    USE_GL_FUNC(glProgramUniform3f) \
    USE_GL_FUNC(glProgramUniform3fEXT) \
    USE_GL_FUNC(glProgramUniform3fv) \
    USE_GL_FUNC(glProgramUniform3fvEXT) \
    USE_GL_FUNC(glProgramUniform3i) \
    USE_GL_FUNC(glProgramUniform3i64ARB) \
    USE_GL_FUNC(glProgramUniform3i64NV) \
    USE_GL_FUNC(glProgramUniform3i64vARB) \
    USE_GL_FUNC(glProgramUniform3i64vNV) \
    USE_GL_FUNC(glProgramUniform3iEXT) \
    USE_GL_FUNC(glProgramUniform3iv) \
    USE_GL_FUNC(glProgramUniform3ivEXT) \
    USE_GL_FUNC(glProgramUniform3ui) \
    USE_GL_FUNC(glProgramUniform3ui64ARB) \
    USE_GL_FUNC(glProgramUniform3ui64NV) \
    USE_GL_FUNC(glProgramUniform3ui64vARB) \
    USE_GL_FUNC(glProgramUniform3ui64vNV) \
    USE_GL_FUNC(glProgramUniform3uiEXT) \
    USE_GL_FUNC(glProgramUniform3uiv) \
    USE_GL_FUNC(glProgramUniform3uivEXT) \
    USE_GL_FUNC(glProgramUniform4d) \
    USE_GL_FUNC(glProgramUniform4dEXT) \
    USE_GL_FUNC(glProgramUniform4dv) \
    USE_GL_FUNC(glProgramUniform4dvEXT) \
    USE_GL_FUNC(glProgramUniform4f) \
    USE_GL_FUNC(glProgramUniform4fEXT) \
    USE_GL_FUNC(glProgramUniform4fv) \
    USE_GL_FUNC(glProgramUniform4fvEXT) \
    USE_GL_FUNC(glProgramUniform4i) \
    USE_GL_FUNC(glProgramUniform4i64ARB) \
    USE_GL_FUNC(glProgramUniform4i64NV) \
    USE_GL_FUNC(glProgramUniform4i64vARB) \
    USE_GL_FUNC(glProgramUniform4i64vNV) \
    USE_GL_FUNC(glProgramUniform4iEXT) \
    USE_GL_FUNC(glProgramUniform4iv) \
    USE_GL_FUNC(glProgramUniform4ivEXT) \
    USE_GL_FUNC(glProgramUniform4ui) \
    USE_GL_FUNC(glProgramUniform4ui64ARB) \
    USE_GL_FUNC(glProgramUniform4ui64NV) \
    USE_GL_FUNC(glProgramUniform4ui64vARB) \
    USE_GL_FUNC(glProgramUniform4ui64vNV) \
    USE_GL_FUNC(glProgramUniform4uiEXT) \
    USE_GL_FUNC(glProgramUniform4uiv) \
    USE_GL_FUNC(glProgramUniform4uivEXT) \
    USE_GL_FUNC(glProgramUniformHandleui64ARB) \
    USE_GL_FUNC(glProgramUniformHandleui64NV) \
    USE_GL_FUNC(glProgramUniformHandleui64vARB) \
    USE_GL_FUNC(glProgramUniformHandleui64vNV) \
    USE_GL_FUNC(glProgramUniformMatrix2dv) \
    USE_GL_FUNC(glProgramUniformMatrix2dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix2fv) \
    USE_GL_FUNC(glProgramUniformMatrix2fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix2x3dv) \
    USE_GL_FUNC(glProgramUniformMatrix2x3dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix2x3fv) \
    USE_GL_FUNC(glProgramUniformMatrix2x3fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix2x4dv) \
    USE_GL_FUNC(glProgramUniformMatrix2x4dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix2x4fv) \
    USE_GL_FUNC(glProgramUniformMatrix2x4fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix3dv) \
    USE_GL_FUNC(glProgramUniformMatrix3dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix3fv) \
    USE_GL_FUNC(glProgramUniformMatrix3fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix3x2dv) \
    USE_GL_FUNC(glProgramUniformMatrix3x2dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix3x2fv) \
    USE_GL_FUNC(glProgramUniformMatrix3x2fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix3x4dv) \
    USE_GL_FUNC(glProgramUniformMatrix3x4dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix3x4fv) \
    USE_GL_FUNC(glProgramUniformMatrix3x4fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix4dv) \
    USE_GL_FUNC(glProgramUniformMatrix4dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix4fv) \
    USE_GL_FUNC(glProgramUniformMatrix4fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix4x2dv) \
    USE_GL_FUNC(glProgramUniformMatrix4x2dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix4x2fv) \
    USE_GL_FUNC(glProgramUniformMatrix4x2fvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix4x3dv) \
    USE_GL_FUNC(glProgramUniformMatrix4x3dvEXT) \
    USE_GL_FUNC(glProgramUniformMatrix4x3fv) \
    USE_GL_FUNC(glProgramUniformMatrix4x3fvEXT) \
    USE_GL_FUNC(glProgramUniformui64NV) \
    USE_GL_FUNC(glProgramUniformui64vNV) \
    USE_GL_FUNC(glProgramVertexLimitNV) \
    USE_GL_FUNC(glProvokingVertex) \
    USE_GL_FUNC(glProvokingVertexEXT) \
    USE_GL_FUNC(glPushClientAttribDefaultEXT) \
    USE_GL_FUNC(glPushDebugGroup) \
    USE_GL_FUNC(glPushGroupMarkerEXT) \
    USE_GL_FUNC(glQueryCounter) \
    USE_GL_FUNC(glQueryMatrixxOES) \
    USE_GL_FUNC(glQueryObjectParameteruiAMD) \
    USE_GL_FUNC(glQueryResourceNV) \
    USE_GL_FUNC(glQueryResourceTagNV) \
    USE_GL_FUNC(glRasterPos2xOES) \
    USE_GL_FUNC(glRasterPos2xvOES) \
    USE_GL_FUNC(glRasterPos3xOES) \
    USE_GL_FUNC(glRasterPos3xvOES) \
    USE_GL_FUNC(glRasterPos4xOES) \
    USE_GL_FUNC(glRasterPos4xvOES) \
    USE_GL_FUNC(glRasterSamplesEXT) \
    USE_GL_FUNC(glReadBufferRegion) \
    USE_GL_FUNC(glReadInstrumentsSGIX) \
    USE_GL_FUNC(glReadnPixels) \
    USE_GL_FUNC(glReadnPixelsARB) \
    USE_GL_FUNC(glRectxOES) \
    USE_GL_FUNC(glRectxvOES) \
    USE_GL_FUNC(glReferencePlaneSGIX) \
    USE_GL_FUNC(glReleaseKeyedMutexWin32EXT) \
    USE_GL_FUNC(glReleaseShaderCompiler) \
    USE_GL_FUNC(glRenderGpuMaskNV) \
    USE_GL_FUNC(glRenderbufferStorage) \
    USE_GL_FUNC(glRenderbufferStorageEXT) \
    USE_GL_FUNC(glRenderbufferStorageMultisample) \
    USE_GL_FUNC(glRenderbufferStorageMultisampleAdvancedAMD) \
    USE_GL_FUNC(glRenderbufferStorageMultisampleCoverageNV) \
    USE_GL_FUNC(glRenderbufferStorageMultisampleEXT) \
    USE_GL_FUNC(glReplacementCodePointerSUN) \
    USE_GL_FUNC(glReplacementCodeubSUN) \
    USE_GL_FUNC(glReplacementCodeubvSUN) \
    USE_GL_FUNC(glReplacementCodeuiColor3fVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiColor3fVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuiColor4fNormal3fVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiColor4fNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuiColor4ubVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiColor4ubVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuiNormal3fVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuiSUN) \
    USE_GL_FUNC(glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuiTexCoord2fVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiTexCoord2fVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuiVertex3fSUN) \
    USE_GL_FUNC(glReplacementCodeuiVertex3fvSUN) \
    USE_GL_FUNC(glReplacementCodeuivSUN) \
    USE_GL_FUNC(glReplacementCodeusSUN) \
    USE_GL_FUNC(glReplacementCodeusvSUN) \
    USE_GL_FUNC(glRequestResidentProgramsNV) \
    USE_GL_FUNC(glResetHistogram) \
    USE_GL_FUNC(glResetHistogramEXT) \
    USE_GL_FUNC(glResetMemoryObjectParameterNV) \
    USE_GL_FUNC(glResetMinmax) \
    USE_GL_FUNC(glResetMinmaxEXT) \
    USE_GL_FUNC(glResizeBuffersMESA) \
    USE_GL_FUNC(glResolveDepthValuesNV) \
    USE_GL_FUNC(glResumeTransformFeedback) \
    USE_GL_FUNC(glResumeTransformFeedbackNV) \
    USE_GL_FUNC(glRotatexOES) \
    USE_GL_FUNC(glSampleCoverage) \
    USE_GL_FUNC(glSampleCoverageARB) \
    USE_GL_FUNC(glSampleMapATI) \
    USE_GL_FUNC(glSampleMaskEXT) \
    USE_GL_FUNC(glSampleMaskIndexedNV) \
    USE_GL_FUNC(glSampleMaskSGIS) \
    USE_GL_FUNC(glSampleMaski) \
    USE_GL_FUNC(glSamplePatternEXT) \
    USE_GL_FUNC(glSamplePatternSGIS) \
    USE_GL_FUNC(glSamplerParameterIiv) \
    USE_GL_FUNC(glSamplerParameterIuiv) \
    USE_GL_FUNC(glSamplerParameterf) \
    USE_GL_FUNC(glSamplerParameterfv) \
    USE_GL_FUNC(glSamplerParameteri) \
    USE_GL_FUNC(glSamplerParameteriv) \
    USE_GL_FUNC(glScalexOES) \
    USE_GL_FUNC(glScissorArrayv) \
    USE_GL_FUNC(glScissorExclusiveArrayvNV) \
    USE_GL_FUNC(glScissorExclusiveNV) \
    USE_GL_FUNC(glScissorIndexed) \
    USE_GL_FUNC(glScissorIndexedv) \
    USE_GL_FUNC(glSecondaryColor3b) \
    USE_GL_FUNC(glSecondaryColor3bEXT) \
    USE_GL_FUNC(glSecondaryColor3bv) \
    USE_GL_FUNC(glSecondaryColor3bvEXT) \
    USE_GL_FUNC(glSecondaryColor3d) \
    USE_GL_FUNC(glSecondaryColor3dEXT) \
    USE_GL_FUNC(glSecondaryColor3dv) \
    USE_GL_FUNC(glSecondaryColor3dvEXT) \
    USE_GL_FUNC(glSecondaryColor3f) \
    USE_GL_FUNC(glSecondaryColor3fEXT) \
    USE_GL_FUNC(glSecondaryColor3fv) \
    USE_GL_FUNC(glSecondaryColor3fvEXT) \
    USE_GL_FUNC(glSecondaryColor3hNV) \
    USE_GL_FUNC(glSecondaryColor3hvNV) \
    USE_GL_FUNC(glSecondaryColor3i) \
    USE_GL_FUNC(glSecondaryColor3iEXT) \
    USE_GL_FUNC(glSecondaryColor3iv) \
    USE_GL_FUNC(glSecondaryColor3ivEXT) \
    USE_GL_FUNC(glSecondaryColor3s) \
    USE_GL_FUNC(glSecondaryColor3sEXT) \
    USE_GL_FUNC(glSecondaryColor3sv) \
    USE_GL_FUNC(glSecondaryColor3svEXT) \
    USE_GL_FUNC(glSecondaryColor3ub) \
    USE_GL_FUNC(glSecondaryColor3ubEXT) \
    USE_GL_FUNC(glSecondaryColor3ubv) \
    USE_GL_FUNC(glSecondaryColor3ubvEXT) \
    USE_GL_FUNC(glSecondaryColor3ui) \
    USE_GL_FUNC(glSecondaryColor3uiEXT) \
    USE_GL_FUNC(glSecondaryColor3uiv) \
    USE_GL_FUNC(glSecondaryColor3uivEXT) \
    USE_GL_FUNC(glSecondaryColor3us) \
    USE_GL_FUNC(glSecondaryColor3usEXT) \
    USE_GL_FUNC(glSecondaryColor3usv) \
    USE_GL_FUNC(glSecondaryColor3usvEXT) \
    USE_GL_FUNC(glSecondaryColorFormatNV) \
    USE_GL_FUNC(glSecondaryColorP3ui) \
    USE_GL_FUNC(glSecondaryColorP3uiv) \
    USE_GL_FUNC(glSecondaryColorPointer) \
    USE_GL_FUNC(glSecondaryColorPointerEXT) \
    USE_GL_FUNC(glSecondaryColorPointerListIBM) \
    USE_GL_FUNC(glSelectPerfMonitorCountersAMD) \
    USE_GL_FUNC(glSelectTextureCoordSetSGIS) \
    USE_GL_FUNC(glSelectTextureSGIS) \
    USE_GL_FUNC(glSemaphoreParameterui64vEXT) \
    USE_GL_FUNC(glSeparableFilter2D) \
    USE_GL_FUNC(glSeparableFilter2DEXT) \
    USE_GL_FUNC(glSetFenceAPPLE) \
    USE_GL_FUNC(glSetFenceNV) \
    USE_GL_FUNC(glSetFragmentShaderConstantATI) \
    USE_GL_FUNC(glSetInvariantEXT) \
    USE_GL_FUNC(glSetLocalConstantEXT) \
    USE_GL_FUNC(glSetMultisamplefvAMD) \
    USE_GL_FUNC(glShaderBinary) \
    USE_GL_FUNC(glShaderOp1EXT) \
    USE_GL_FUNC(glShaderOp2EXT) \
    USE_GL_FUNC(glShaderOp3EXT) \
    USE_GL_FUNC(glShaderSource) \
    USE_GL_FUNC(glShaderSourceARB) \
    USE_GL_FUNC(glShaderStorageBlockBinding) \
    USE_GL_FUNC(glShadingRateImageBarrierNV) \
    USE_GL_FUNC(glShadingRateImagePaletteNV) \
    USE_GL_FUNC(glShadingRateSampleOrderCustomNV) \
    USE_GL_FUNC(glShadingRateSampleOrderNV) \
    USE_GL_FUNC(glSharpenTexFuncSGIS) \
    USE_GL_FUNC(glSignalSemaphoreEXT) \
    USE_GL_FUNC(glSignalSemaphoreui64NVX) \
    USE_GL_FUNC(glSignalVkFenceNV) \
    USE_GL_FUNC(glSignalVkSemaphoreNV) \
    USE_GL_FUNC(glSpecializeShader) \
    USE_GL_FUNC(glSpecializeShaderARB) \
    USE_GL_FUNC(glSpriteParameterfSGIX) \
    USE_GL_FUNC(glSpriteParameterfvSGIX) \
    USE_GL_FUNC(glSpriteParameteriSGIX) \
    USE_GL_FUNC(glSpriteParameterivSGIX) \
    USE_GL_FUNC(glStartInstrumentsSGIX) \
    USE_GL_FUNC(glStateCaptureNV) \
    USE_GL_FUNC(glStencilClearTagEXT) \
    USE_GL_FUNC(glStencilFillPathInstancedNV) \
    USE_GL_FUNC(glStencilFillPathNV) \
    USE_GL_FUNC(glStencilFuncSeparate) \
    USE_GL_FUNC(glStencilFuncSeparateATI) \
    USE_GL_FUNC(glStencilMaskSeparate) \
    USE_GL_FUNC(glStencilOpSeparate) \
    USE_GL_FUNC(glStencilOpSeparateATI) \
    USE_GL_FUNC(glStencilOpValueAMD) \
    USE_GL_FUNC(glStencilStrokePathInstancedNV) \
    USE_GL_FUNC(glStencilStrokePathNV) \
    USE_GL_FUNC(glStencilThenCoverFillPathInstancedNV) \
    USE_GL_FUNC(glStencilThenCoverFillPathNV) \
    USE_GL_FUNC(glStencilThenCoverStrokePathInstancedNV) \
    USE_GL_FUNC(glStencilThenCoverStrokePathNV) \
    USE_GL_FUNC(glStopInstrumentsSGIX) \
    USE_GL_FUNC(glStringMarkerGREMEDY) \
    USE_GL_FUNC(glSubpixelPrecisionBiasNV) \
    USE_GL_FUNC(glSwizzleEXT) \
    USE_GL_FUNC(glSyncTextureINTEL) \
    USE_GL_FUNC(glTagSampleBufferSGIX) \
    USE_GL_FUNC(glTangent3bEXT) \
    USE_GL_FUNC(glTangent3bvEXT) \
    USE_GL_FUNC(glTangent3dEXT) \
    USE_GL_FUNC(glTangent3dvEXT) \
    USE_GL_FUNC(glTangent3fEXT) \
    USE_GL_FUNC(glTangent3fvEXT) \
    USE_GL_FUNC(glTangent3iEXT) \
    USE_GL_FUNC(glTangent3ivEXT) \
    USE_GL_FUNC(glTangent3sEXT) \
    USE_GL_FUNC(glTangent3svEXT) \
    USE_GL_FUNC(glTangentPointerEXT) \
    USE_GL_FUNC(glTbufferMask3DFX) \
    USE_GL_FUNC(glTessellationFactorAMD) \
    USE_GL_FUNC(glTessellationModeAMD) \
    USE_GL_FUNC(glTestFenceAPPLE) \
    USE_GL_FUNC(glTestFenceNV) \
    USE_GL_FUNC(glTestObjectAPPLE) \
    USE_GL_FUNC(glTexAttachMemoryNV) \
    USE_GL_FUNC(glTexBuffer) \
    USE_GL_FUNC(glTexBufferARB) \
    USE_GL_FUNC(glTexBufferEXT) \
    USE_GL_FUNC(glTexBufferRange) \
    USE_GL_FUNC(glTexBumpParameterfvATI) \
    USE_GL_FUNC(glTexBumpParameterivATI) \
    USE_GL_FUNC(glTexCoord1bOES) \
    USE_GL_FUNC(glTexCoord1bvOES) \
    USE_GL_FUNC(glTexCoord1hNV) \
    USE_GL_FUNC(glTexCoord1hvNV) \
    USE_GL_FUNC(glTexCoord1xOES) \
    USE_GL_FUNC(glTexCoord1xvOES) \
    USE_GL_FUNC(glTexCoord2bOES) \
    USE_GL_FUNC(glTexCoord2bvOES) \
    USE_GL_FUNC(glTexCoord2fColor3fVertex3fSUN) \
    USE_GL_FUNC(glTexCoord2fColor3fVertex3fvSUN) \
    USE_GL_FUNC(glTexCoord2fColor4fNormal3fVertex3fSUN) \
    USE_GL_FUNC(glTexCoord2fColor4fNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glTexCoord2fColor4ubVertex3fSUN) \
    USE_GL_FUNC(glTexCoord2fColor4ubVertex3fvSUN) \
    USE_GL_FUNC(glTexCoord2fNormal3fVertex3fSUN) \
    USE_GL_FUNC(glTexCoord2fNormal3fVertex3fvSUN) \
    USE_GL_FUNC(glTexCoord2fVertex3fSUN) \
    USE_GL_FUNC(glTexCoord2fVertex3fvSUN) \
    USE_GL_FUNC(glTexCoord2hNV) \
    USE_GL_FUNC(glTexCoord2hvNV) \
    USE_GL_FUNC(glTexCoord2xOES) \
    USE_GL_FUNC(glTexCoord2xvOES) \
    USE_GL_FUNC(glTexCoord3bOES) \
    USE_GL_FUNC(glTexCoord3bvOES) \
    USE_GL_FUNC(glTexCoord3hNV) \
    USE_GL_FUNC(glTexCoord3hvNV) \
    USE_GL_FUNC(glTexCoord3xOES) \
    USE_GL_FUNC(glTexCoord3xvOES) \
    USE_GL_FUNC(glTexCoord4bOES) \
    USE_GL_FUNC(glTexCoord4bvOES) \
    USE_GL_FUNC(glTexCoord4fColor4fNormal3fVertex4fSUN) \
    USE_GL_FUNC(glTexCoord4fColor4fNormal3fVertex4fvSUN) \
    USE_GL_FUNC(glTexCoord4fVertex4fSUN) \
    USE_GL_FUNC(glTexCoord4fVertex4fvSUN) \
    USE_GL_FUNC(glTexCoord4hNV) \
    USE_GL_FUNC(glTexCoord4hvNV) \
    USE_GL_FUNC(glTexCoord4xOES) \
    USE_GL_FUNC(glTexCoord4xvOES) \
    USE_GL_FUNC(glTexCoordFormatNV) \
    USE_GL_FUNC(glTexCoordP1ui) \
    USE_GL_FUNC(glTexCoordP1uiv) \
    USE_GL_FUNC(glTexCoordP2ui) \
    USE_GL_FUNC(glTexCoordP2uiv) \
    USE_GL_FUNC(glTexCoordP3ui) \
    USE_GL_FUNC(glTexCoordP3uiv) \
    USE_GL_FUNC(glTexCoordP4ui) \
    USE_GL_FUNC(glTexCoordP4uiv) \
    USE_GL_FUNC(glTexCoordPointerEXT) \
    USE_GL_FUNC(glTexCoordPointerListIBM) \
    USE_GL_FUNC(glTexCoordPointervINTEL) \
    USE_GL_FUNC(glTexEnvxOES) \
    USE_GL_FUNC(glTexEnvxvOES) \
    USE_GL_FUNC(glTexFilterFuncSGIS) \
    USE_GL_FUNC(glTexGenxOES) \
    USE_GL_FUNC(glTexGenxvOES) \
    USE_GL_FUNC(glTexImage2DMultisample) \
    USE_GL_FUNC(glTexImage2DMultisampleCoverageNV) \
    USE_GL_FUNC(glTexImage3D) \
    USE_GL_FUNC(glTexImage3DEXT) \
    USE_GL_FUNC(glTexImage3DMultisample) \
    USE_GL_FUNC(glTexImage3DMultisampleCoverageNV) \
    USE_GL_FUNC(glTexImage4DSGIS) \
    USE_GL_FUNC(glTexPageCommitmentARB) \
    USE_GL_FUNC(glTexParameterIiv) \
    USE_GL_FUNC(glTexParameterIivEXT) \
    USE_GL_FUNC(glTexParameterIuiv) \
    USE_GL_FUNC(glTexParameterIuivEXT) \
    USE_GL_FUNC(glTexParameterxOES) \
    USE_GL_FUNC(glTexParameterxvOES) \
    USE_GL_FUNC(glTexRenderbufferNV) \
    USE_GL_FUNC(glTexStorage1D) \
    USE_GL_FUNC(glTexStorage2D) \
    USE_GL_FUNC(glTexStorage2DMultisample) \
    USE_GL_FUNC(glTexStorage3D) \
    USE_GL_FUNC(glTexStorage3DMultisample) \
    USE_GL_FUNC(glTexStorageMem1DEXT) \
    USE_GL_FUNC(glTexStorageMem2DEXT) \
    USE_GL_FUNC(glTexStorageMem2DMultisampleEXT) \
    USE_GL_FUNC(glTexStorageMem3DEXT) \
    USE_GL_FUNC(glTexStorageMem3DMultisampleEXT) \
    USE_GL_FUNC(glTexStorageSparseAMD) \
    USE_GL_FUNC(glTexSubImage1DEXT) \
    USE_GL_FUNC(glTexSubImage2DEXT) \
    USE_GL_FUNC(glTexSubImage3D) \
    USE_GL_FUNC(glTexSubImage3DEXT) \
    USE_GL_FUNC(glTexSubImage4DSGIS) \
    USE_GL_FUNC(glTextureAttachMemoryNV) \
    USE_GL_FUNC(glTextureBarrier) \
    USE_GL_FUNC(glTextureBarrierNV) \
    USE_GL_FUNC(glTextureBuffer) \
    USE_GL_FUNC(glTextureBufferEXT) \
    USE_GL_FUNC(glTextureBufferRange) \
    USE_GL_FUNC(glTextureBufferRangeEXT) \
    USE_GL_FUNC(glTextureColorMaskSGIS) \
    USE_GL_FUNC(glTextureImage1DEXT) \
    USE_GL_FUNC(glTextureImage2DEXT) \
    USE_GL_FUNC(glTextureImage2DMultisampleCoverageNV) \
    USE_GL_FUNC(glTextureImage2DMultisampleNV) \
    USE_GL_FUNC(glTextureImage3DEXT) \
    USE_GL_FUNC(glTextureImage3DMultisampleCoverageNV) \
    USE_GL_FUNC(glTextureImage3DMultisampleNV) \
    USE_GL_FUNC(glTextureLightEXT) \
    USE_GL_FUNC(glTextureMaterialEXT) \
    USE_GL_FUNC(glTextureNormalEXT) \
    USE_GL_FUNC(glTexturePageCommitmentEXT) \
    USE_GL_FUNC(glTextureParameterIiv) \
    USE_GL_FUNC(glTextureParameterIivEXT) \
    USE_GL_FUNC(glTextureParameterIuiv) \
    USE_GL_FUNC(glTextureParameterIuivEXT) \
    USE_GL_FUNC(glTextureParameterf) \
    USE_GL_FUNC(glTextureParameterfEXT) \
    USE_GL_FUNC(glTextureParameterfv) \
    USE_GL_FUNC(glTextureParameterfvEXT) \
    USE_GL_FUNC(glTextureParameteri) \
    USE_GL_FUNC(glTextureParameteriEXT) \
    USE_GL_FUNC(glTextureParameteriv) \
    USE_GL_FUNC(glTextureParameterivEXT) \
    USE_GL_FUNC(glTextureRangeAPPLE) \
    USE_GL_FUNC(glTextureRenderbufferEXT) \
    USE_GL_FUNC(glTextureStorage1D) \
    USE_GL_FUNC(glTextureStorage1DEXT) \
    USE_GL_FUNC(glTextureStorage2D) \
    USE_GL_FUNC(glTextureStorage2DEXT) \
    USE_GL_FUNC(glTextureStorage2DMultisample) \
    USE_GL_FUNC(glTextureStorage2DMultisampleEXT) \
    USE_GL_FUNC(glTextureStorage3D) \
    USE_GL_FUNC(glTextureStorage3DEXT) \
    USE_GL_FUNC(glTextureStorage3DMultisample) \
    USE_GL_FUNC(glTextureStorage3DMultisampleEXT) \
    USE_GL_FUNC(glTextureStorageMem1DEXT) \
    USE_GL_FUNC(glTextureStorageMem2DEXT) \
    USE_GL_FUNC(glTextureStorageMem2DMultisampleEXT) \
    USE_GL_FUNC(glTextureStorageMem3DEXT) \
    USE_GL_FUNC(glTextureStorageMem3DMultisampleEXT) \
    USE_GL_FUNC(glTextureStorageSparseAMD) \
    USE_GL_FUNC(glTextureSubImage1D) \
    USE_GL_FUNC(glTextureSubImage1DEXT) \
    USE_GL_FUNC(glTextureSubImage2D) \
    USE_GL_FUNC(glTextureSubImage2DEXT) \
    USE_GL_FUNC(glTextureSubImage3D) \
    USE_GL_FUNC(glTextureSubImage3DEXT) \
    USE_GL_FUNC(glTextureView) \
    USE_GL_FUNC(glTrackMatrixNV) \
    USE_GL_FUNC(glTransformFeedbackAttribsNV) \
    USE_GL_FUNC(glTransformFeedbackBufferBase) \
    USE_GL_FUNC(glTransformFeedbackBufferRange) \
    USE_GL_FUNC(glTransformFeedbackStreamAttribsNV) \
    USE_GL_FUNC(glTransformFeedbackVaryings) \
    USE_GL_FUNC(glTransformFeedbackVaryingsEXT) \
    USE_GL_FUNC(glTransformFeedbackVaryingsNV) \
    USE_GL_FUNC(glTransformPathNV) \
    USE_GL_FUNC(glTranslatexOES) \
    USE_GL_FUNC(glUniform1d) \
    USE_GL_FUNC(glUniform1dv) \
    USE_GL_FUNC(glUniform1f) \
    USE_GL_FUNC(glUniform1fARB) \
    USE_GL_FUNC(glUniform1fv) \
    USE_GL_FUNC(glUniform1fvARB) \
    USE_GL_FUNC(glUniform1i) \
    USE_GL_FUNC(glUniform1i64ARB) \
    USE_GL_FUNC(glUniform1i64NV) \
    USE_GL_FUNC(glUniform1i64vARB) \
    USE_GL_FUNC(glUniform1i64vNV) \
    USE_GL_FUNC(glUniform1iARB) \
    USE_GL_FUNC(glUniform1iv) \
    USE_GL_FUNC(glUniform1ivARB) \
    USE_GL_FUNC(glUniform1ui) \
    USE_GL_FUNC(glUniform1ui64ARB) \
    USE_GL_FUNC(glUniform1ui64NV) \
    USE_GL_FUNC(glUniform1ui64vARB) \
    USE_GL_FUNC(glUniform1ui64vNV) \
    USE_GL_FUNC(glUniform1uiEXT) \
    USE_GL_FUNC(glUniform1uiv) \
    USE_GL_FUNC(glUniform1uivEXT) \
    USE_GL_FUNC(glUniform2d) \
    USE_GL_FUNC(glUniform2dv) \
    USE_GL_FUNC(glUniform2f) \
    USE_GL_FUNC(glUniform2fARB) \
    USE_GL_FUNC(glUniform2fv) \
    USE_GL_FUNC(glUniform2fvARB) \
    USE_GL_FUNC(glUniform2i) \
    USE_GL_FUNC(glUniform2i64ARB) \
    USE_GL_FUNC(glUniform2i64NV) \
    USE_GL_FUNC(glUniform2i64vARB) \
    USE_GL_FUNC(glUniform2i64vNV) \
    USE_GL_FUNC(glUniform2iARB) \
    USE_GL_FUNC(glUniform2iv) \
    USE_GL_FUNC(glUniform2ivARB) \
    USE_GL_FUNC(glUniform2ui) \
    USE_GL_FUNC(glUniform2ui64ARB) \
    USE_GL_FUNC(glUniform2ui64NV) \
    USE_GL_FUNC(glUniform2ui64vARB) \
    USE_GL_FUNC(glUniform2ui64vNV) \
    USE_GL_FUNC(glUniform2uiEXT) \
    USE_GL_FUNC(glUniform2uiv) \
    USE_GL_FUNC(glUniform2uivEXT) \
    USE_GL_FUNC(glUniform3d) \
    USE_GL_FUNC(glUniform3dv) \
    USE_GL_FUNC(glUniform3f) \
    USE_GL_FUNC(glUniform3fARB) \
    USE_GL_FUNC(glUniform3fv) \
    USE_GL_FUNC(glUniform3fvARB) \
    USE_GL_FUNC(glUniform3i) \
    USE_GL_FUNC(glUniform3i64ARB) \
    USE_GL_FUNC(glUniform3i64NV) \
    USE_GL_FUNC(glUniform3i64vARB) \
    USE_GL_FUNC(glUniform3i64vNV) \
    USE_GL_FUNC(glUniform3iARB) \
    USE_GL_FUNC(glUniform3iv) \
    USE_GL_FUNC(glUniform3ivARB) \
    USE_GL_FUNC(glUniform3ui) \
    USE_GL_FUNC(glUniform3ui64ARB) \
    USE_GL_FUNC(glUniform3ui64NV) \
    USE_GL_FUNC(glUniform3ui64vARB) \
    USE_GL_FUNC(glUniform3ui64vNV) \
    USE_GL_FUNC(glUniform3uiEXT) \
    USE_GL_FUNC(glUniform3uiv) \
    USE_GL_FUNC(glUniform3uivEXT) \
    USE_GL_FUNC(glUniform4d) \
    USE_GL_FUNC(glUniform4dv) \
    USE_GL_FUNC(glUniform4f) \
    USE_GL_FUNC(glUniform4fARB) \
    USE_GL_FUNC(glUniform4fv) \
    USE_GL_FUNC(glUniform4fvARB) \
    USE_GL_FUNC(glUniform4i) \
    USE_GL_FUNC(glUniform4i64ARB) \
    USE_GL_FUNC(glUniform4i64NV) \
    USE_GL_FUNC(glUniform4i64vARB) \
    USE_GL_FUNC(glUniform4i64vNV) \
    USE_GL_FUNC(glUniform4iARB) \
    USE_GL_FUNC(glUniform4iv) \
    USE_GL_FUNC(glUniform4ivARB) \
    USE_GL_FUNC(glUniform4ui) \
    USE_GL_FUNC(glUniform4ui64ARB) \
    USE_GL_FUNC(glUniform4ui64NV) \
    USE_GL_FUNC(glUniform4ui64vARB) \
    USE_GL_FUNC(glUniform4ui64vNV) \
    USE_GL_FUNC(glUniform4uiEXT) \
    USE_GL_FUNC(glUniform4uiv) \
    USE_GL_FUNC(glUniform4uivEXT) \
    USE_GL_FUNC(glUniformBlockBinding) \
    USE_GL_FUNC(glUniformBufferEXT) \
    USE_GL_FUNC(glUniformHandleui64ARB) \
    USE_GL_FUNC(glUniformHandleui64NV) \
    USE_GL_FUNC(glUniformHandleui64vARB) \
    USE_GL_FUNC(glUniformHandleui64vNV) \
    USE_GL_FUNC(glUniformMatrix2dv) \
    USE_GL_FUNC(glUniformMatrix2fv) \
    USE_GL_FUNC(glUniformMatrix2fvARB) \
    USE_GL_FUNC(glUniformMatrix2x3dv) \
    USE_GL_FUNC(glUniformMatrix2x3fv) \
    USE_GL_FUNC(glUniformMatrix2x4dv) \
    USE_GL_FUNC(glUniformMatrix2x4fv) \
    USE_GL_FUNC(glUniformMatrix3dv) \
    USE_GL_FUNC(glUniformMatrix3fv) \
    USE_GL_FUNC(glUniformMatrix3fvARB) \
    USE_GL_FUNC(glUniformMatrix3x2dv) \
    USE_GL_FUNC(glUniformMatrix3x2fv) \
    USE_GL_FUNC(glUniformMatrix3x4dv) \
    USE_GL_FUNC(glUniformMatrix3x4fv) \
    USE_GL_FUNC(glUniformMatrix4dv) \
    USE_GL_FUNC(glUniformMatrix4fv) \
    USE_GL_FUNC(glUniformMatrix4fvARB) \
    USE_GL_FUNC(glUniformMatrix4x2dv) \
    USE_GL_FUNC(glUniformMatrix4x2fv) \
    USE_GL_FUNC(glUniformMatrix4x3dv) \
    USE_GL_FUNC(glUniformMatrix4x3fv) \
    USE_GL_FUNC(glUniformSubroutinesuiv) \
    USE_GL_FUNC(glUniformui64NV) \
    USE_GL_FUNC(glUniformui64vNV) \
    USE_GL_FUNC(glUnlockArraysEXT) \
    USE_GL_FUNC(glUnmapBuffer) \
    USE_GL_FUNC(glUnmapBufferARB) \
    USE_GL_FUNC(glUnmapNamedBuffer) \
    USE_GL_FUNC(glUnmapNamedBufferEXT) \
    USE_GL_FUNC(glUnmapObjectBufferATI) \
    USE_GL_FUNC(glUnmapTexture2DINTEL) \
    USE_GL_FUNC(glUpdateObjectBufferATI) \
    USE_GL_FUNC(glUploadGpuMaskNVX) \
    USE_GL_FUNC(glUseProgram) \
    USE_GL_FUNC(glUseProgramObjectARB) \
    USE_GL_FUNC(glUseProgramStages) \
    USE_GL_FUNC(glUseShaderProgramEXT) \
    USE_GL_FUNC(glVDPAUFiniNV) \
    USE_GL_FUNC(glVDPAUGetSurfaceivNV) \
    USE_GL_FUNC(glVDPAUInitNV) \
    USE_GL_FUNC(glVDPAUIsSurfaceNV) \
    USE_GL_FUNC(glVDPAUMapSurfacesNV) \
    USE_GL_FUNC(glVDPAURegisterOutputSurfaceNV) \
    USE_GL_FUNC(glVDPAURegisterVideoSurfaceNV) \
    USE_GL_FUNC(glVDPAURegisterVideoSurfaceWithPictureStructureNV) \
    USE_GL_FUNC(glVDPAUSurfaceAccessNV) \
    USE_GL_FUNC(glVDPAUUnmapSurfacesNV) \
    USE_GL_FUNC(glVDPAUUnregisterSurfaceNV) \
    USE_GL_FUNC(glValidateProgram) \
    USE_GL_FUNC(glValidateProgramARB) \
    USE_GL_FUNC(glValidateProgramPipeline) \
    USE_GL_FUNC(glVariantArrayObjectATI) \
    USE_GL_FUNC(glVariantPointerEXT) \
    USE_GL_FUNC(glVariantbvEXT) \
    USE_GL_FUNC(glVariantdvEXT) \
    USE_GL_FUNC(glVariantfvEXT) \
    USE_GL_FUNC(glVariantivEXT) \
    USE_GL_FUNC(glVariantsvEXT) \
    USE_GL_FUNC(glVariantubvEXT) \
    USE_GL_FUNC(glVariantuivEXT) \
    USE_GL_FUNC(glVariantusvEXT) \
    USE_GL_FUNC(glVertex2bOES) \
    USE_GL_FUNC(glVertex2bvOES) \
    USE_GL_FUNC(glVertex2hNV) \
    USE_GL_FUNC(glVertex2hvNV) \
    USE_GL_FUNC(glVertex2xOES) \
    USE_GL_FUNC(glVertex2xvOES) \
    USE_GL_FUNC(glVertex3bOES) \
    USE_GL_FUNC(glVertex3bvOES) \
    USE_GL_FUNC(glVertex3hNV) \
    USE_GL_FUNC(glVertex3hvNV) \
    USE_GL_FUNC(glVertex3xOES) \
    USE_GL_FUNC(glVertex3xvOES) \
    USE_GL_FUNC(glVertex4bOES) \
    USE_GL_FUNC(glVertex4bvOES) \
    USE_GL_FUNC(glVertex4hNV) \
    USE_GL_FUNC(glVertex4hvNV) \
    USE_GL_FUNC(glVertex4xOES) \
    USE_GL_FUNC(glVertex4xvOES) \
    USE_GL_FUNC(glVertexArrayAttribBinding) \
    USE_GL_FUNC(glVertexArrayAttribFormat) \
    USE_GL_FUNC(glVertexArrayAttribIFormat) \
    USE_GL_FUNC(glVertexArrayAttribLFormat) \
    USE_GL_FUNC(glVertexArrayBindVertexBufferEXT) \
    USE_GL_FUNC(glVertexArrayBindingDivisor) \
    USE_GL_FUNC(glVertexArrayColorOffsetEXT) \
    USE_GL_FUNC(glVertexArrayEdgeFlagOffsetEXT) \
    USE_GL_FUNC(glVertexArrayElementBuffer) \
    USE_GL_FUNC(glVertexArrayFogCoordOffsetEXT) \
    USE_GL_FUNC(glVertexArrayIndexOffsetEXT) \
    USE_GL_FUNC(glVertexArrayMultiTexCoordOffsetEXT) \
    USE_GL_FUNC(glVertexArrayNormalOffsetEXT) \
    USE_GL_FUNC(glVertexArrayParameteriAPPLE) \
    USE_GL_FUNC(glVertexArrayRangeAPPLE) \
    USE_GL_FUNC(glVertexArrayRangeNV) \
    USE_GL_FUNC(glVertexArraySecondaryColorOffsetEXT) \
    USE_GL_FUNC(glVertexArrayTexCoordOffsetEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribBindingEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribDivisorEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribFormatEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribIFormatEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribIOffsetEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribLFormatEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribLOffsetEXT) \
    USE_GL_FUNC(glVertexArrayVertexAttribOffsetEXT) \
    USE_GL_FUNC(glVertexArrayVertexBindingDivisorEXT) \
    USE_GL_FUNC(glVertexArrayVertexBuffer) \
    USE_GL_FUNC(glVertexArrayVertexBuffers) \
    USE_GL_FUNC(glVertexArrayVertexOffsetEXT) \
    USE_GL_FUNC(glVertexAttrib1d) \
    USE_GL_FUNC(glVertexAttrib1dARB) \
    USE_GL_FUNC(glVertexAttrib1dNV) \
    USE_GL_FUNC(glVertexAttrib1dv) \
    USE_GL_FUNC(glVertexAttrib1dvARB) \
    USE_GL_FUNC(glVertexAttrib1dvNV) \
    USE_GL_FUNC(glVertexAttrib1f) \
    USE_GL_FUNC(glVertexAttrib1fARB) \
    USE_GL_FUNC(glVertexAttrib1fNV) \
    USE_GL_FUNC(glVertexAttrib1fv) \
    USE_GL_FUNC(glVertexAttrib1fvARB) \
    USE_GL_FUNC(glVertexAttrib1fvNV) \
    USE_GL_FUNC(glVertexAttrib1hNV) \
    USE_GL_FUNC(glVertexAttrib1hvNV) \
    USE_GL_FUNC(glVertexAttrib1s) \
    USE_GL_FUNC(glVertexAttrib1sARB) \
    USE_GL_FUNC(glVertexAttrib1sNV) \
    USE_GL_FUNC(glVertexAttrib1sv) \
    USE_GL_FUNC(glVertexAttrib1svARB) \
    USE_GL_FUNC(glVertexAttrib1svNV) \
    USE_GL_FUNC(glVertexAttrib2d) \
    USE_GL_FUNC(glVertexAttrib2dARB) \
    USE_GL_FUNC(glVertexAttrib2dNV) \
    USE_GL_FUNC(glVertexAttrib2dv) \
    USE_GL_FUNC(glVertexAttrib2dvARB) \
    USE_GL_FUNC(glVertexAttrib2dvNV) \
    USE_GL_FUNC(glVertexAttrib2f) \
    USE_GL_FUNC(glVertexAttrib2fARB) \
    USE_GL_FUNC(glVertexAttrib2fNV) \
    USE_GL_FUNC(glVertexAttrib2fv) \
    USE_GL_FUNC(glVertexAttrib2fvARB) \
    USE_GL_FUNC(glVertexAttrib2fvNV) \
    USE_GL_FUNC(glVertexAttrib2hNV) \
    USE_GL_FUNC(glVertexAttrib2hvNV) \
    USE_GL_FUNC(glVertexAttrib2s) \
    USE_GL_FUNC(glVertexAttrib2sARB) \
    USE_GL_FUNC(glVertexAttrib2sNV) \
    USE_GL_FUNC(glVertexAttrib2sv) \
    USE_GL_FUNC(glVertexAttrib2svARB) \
    USE_GL_FUNC(glVertexAttrib2svNV) \
    USE_GL_FUNC(glVertexAttrib3d) \
    USE_GL_FUNC(glVertexAttrib3dARB) \
    USE_GL_FUNC(glVertexAttrib3dNV) \
    USE_GL_FUNC(glVertexAttrib3dv) \
    USE_GL_FUNC(glVertexAttrib3dvARB) \
    USE_GL_FUNC(glVertexAttrib3dvNV) \
    USE_GL_FUNC(glVertexAttrib3f) \
    USE_GL_FUNC(glVertexAttrib3fARB) \
    USE_GL_FUNC(glVertexAttrib3fNV) \
    USE_GL_FUNC(glVertexAttrib3fv) \
    USE_GL_FUNC(glVertexAttrib3fvARB) \
    USE_GL_FUNC(glVertexAttrib3fvNV) \
    USE_GL_FUNC(glVertexAttrib3hNV) \
    USE_GL_FUNC(glVertexAttrib3hvNV) \
    USE_GL_FUNC(glVertexAttrib3s) \
    USE_GL_FUNC(glVertexAttrib3sARB) \
    USE_GL_FUNC(glVertexAttrib3sNV) \
    USE_GL_FUNC(glVertexAttrib3sv) \
    USE_GL_FUNC(glVertexAttrib3svARB) \
    USE_GL_FUNC(glVertexAttrib3svNV) \
    USE_GL_FUNC(glVertexAttrib4Nbv) \
    USE_GL_FUNC(glVertexAttrib4NbvARB) \
    USE_GL_FUNC(glVertexAttrib4Niv) \
    USE_GL_FUNC(glVertexAttrib4NivARB) \
    USE_GL_FUNC(glVertexAttrib4Nsv) \
    USE_GL_FUNC(glVertexAttrib4NsvARB) \
    USE_GL_FUNC(glVertexAttrib4Nub) \
    USE_GL_FUNC(glVertexAttrib4NubARB) \
    USE_GL_FUNC(glVertexAttrib4Nubv) \
    USE_GL_FUNC(glVertexAttrib4NubvARB) \
    USE_GL_FUNC(glVertexAttrib4Nuiv) \
    USE_GL_FUNC(glVertexAttrib4NuivARB) \
    USE_GL_FUNC(glVertexAttrib4Nusv) \
    USE_GL_FUNC(glVertexAttrib4NusvARB) \
    USE_GL_FUNC(glVertexAttrib4bv) \
    USE_GL_FUNC(glVertexAttrib4bvARB) \
    USE_GL_FUNC(glVertexAttrib4d) \
    USE_GL_FUNC(glVertexAttrib4dARB) \
    USE_GL_FUNC(glVertexAttrib4dNV) \
    USE_GL_FUNC(glVertexAttrib4dv) \
    USE_GL_FUNC(glVertexAttrib4dvARB) \
    USE_GL_FUNC(glVertexAttrib4dvNV) \
    USE_GL_FUNC(glVertexAttrib4f) \
    USE_GL_FUNC(glVertexAttrib4fARB) \
    USE_GL_FUNC(glVertexAttrib4fNV) \
    USE_GL_FUNC(glVertexAttrib4fv) \
    USE_GL_FUNC(glVertexAttrib4fvARB) \
    USE_GL_FUNC(glVertexAttrib4fvNV) \
    USE_GL_FUNC(glVertexAttrib4hNV) \
    USE_GL_FUNC(glVertexAttrib4hvNV) \
    USE_GL_FUNC(glVertexAttrib4iv) \
    USE_GL_FUNC(glVertexAttrib4ivARB) \
    USE_GL_FUNC(glVertexAttrib4s) \
    USE_GL_FUNC(glVertexAttrib4sARB) \
    USE_GL_FUNC(glVertexAttrib4sNV) \
    USE_GL_FUNC(glVertexAttrib4sv) \
    USE_GL_FUNC(glVertexAttrib4svARB) \
    USE_GL_FUNC(glVertexAttrib4svNV) \
    USE_GL_FUNC(glVertexAttrib4ubNV) \
    USE_GL_FUNC(glVertexAttrib4ubv) \
    USE_GL_FUNC(glVertexAttrib4ubvARB) \
    USE_GL_FUNC(glVertexAttrib4ubvNV) \
    USE_GL_FUNC(glVertexAttrib4uiv) \
    USE_GL_FUNC(glVertexAttrib4uivARB) \
    USE_GL_FUNC(glVertexAttrib4usv) \
    USE_GL_FUNC(glVertexAttrib4usvARB) \
    USE_GL_FUNC(glVertexAttribArrayObjectATI) \
    USE_GL_FUNC(glVertexAttribBinding) \
    USE_GL_FUNC(glVertexAttribDivisor) \
    USE_GL_FUNC(glVertexAttribDivisorARB) \
    USE_GL_FUNC(glVertexAttribFormat) \
    USE_GL_FUNC(glVertexAttribFormatNV) \
    USE_GL_FUNC(glVertexAttribI1i) \
    USE_GL_FUNC(glVertexAttribI1iEXT) \
    USE_GL_FUNC(glVertexAttribI1iv) \
    USE_GL_FUNC(glVertexAttribI1ivEXT) \
    USE_GL_FUNC(glVertexAttribI1ui) \
    USE_GL_FUNC(glVertexAttribI1uiEXT) \
    USE_GL_FUNC(glVertexAttribI1uiv) \
    USE_GL_FUNC(glVertexAttribI1uivEXT) \
    USE_GL_FUNC(glVertexAttribI2i) \
    USE_GL_FUNC(glVertexAttribI2iEXT) \
    USE_GL_FUNC(glVertexAttribI2iv) \
    USE_GL_FUNC(glVertexAttribI2ivEXT) \
    USE_GL_FUNC(glVertexAttribI2ui) \
    USE_GL_FUNC(glVertexAttribI2uiEXT) \
    USE_GL_FUNC(glVertexAttribI2uiv) \
    USE_GL_FUNC(glVertexAttribI2uivEXT) \
    USE_GL_FUNC(glVertexAttribI3i) \
    USE_GL_FUNC(glVertexAttribI3iEXT) \
    USE_GL_FUNC(glVertexAttribI3iv) \
    USE_GL_FUNC(glVertexAttribI3ivEXT) \
    USE_GL_FUNC(glVertexAttribI3ui) \
    USE_GL_FUNC(glVertexAttribI3uiEXT) \
    USE_GL_FUNC(glVertexAttribI3uiv) \
    USE_GL_FUNC(glVertexAttribI3uivEXT) \
    USE_GL_FUNC(glVertexAttribI4bv) \
    USE_GL_FUNC(glVertexAttribI4bvEXT) \
    USE_GL_FUNC(glVertexAttribI4i) \
    USE_GL_FUNC(glVertexAttribI4iEXT) \
    USE_GL_FUNC(glVertexAttribI4iv) \
    USE_GL_FUNC(glVertexAttribI4ivEXT) \
    USE_GL_FUNC(glVertexAttribI4sv) \
    USE_GL_FUNC(glVertexAttribI4svEXT) \
    USE_GL_FUNC(glVertexAttribI4ubv) \
    USE_GL_FUNC(glVertexAttribI4ubvEXT) \
    USE_GL_FUNC(glVertexAttribI4ui) \
    USE_GL_FUNC(glVertexAttribI4uiEXT) \
    USE_GL_FUNC(glVertexAttribI4uiv) \
    USE_GL_FUNC(glVertexAttribI4uivEXT) \
    USE_GL_FUNC(glVertexAttribI4usv) \
    USE_GL_FUNC(glVertexAttribI4usvEXT) \
    USE_GL_FUNC(glVertexAttribIFormat) \
    USE_GL_FUNC(glVertexAttribIFormatNV) \
    USE_GL_FUNC(glVertexAttribIPointer) \
    USE_GL_FUNC(glVertexAttribIPointerEXT) \
    USE_GL_FUNC(glVertexAttribL1d) \
    USE_GL_FUNC(glVertexAttribL1dEXT) \
    USE_GL_FUNC(glVertexAttribL1dv) \
    USE_GL_FUNC(glVertexAttribL1dvEXT) \
    USE_GL_FUNC(glVertexAttribL1i64NV) \
    USE_GL_FUNC(glVertexAttribL1i64vNV) \
    USE_GL_FUNC(glVertexAttribL1ui64ARB) \
    USE_GL_FUNC(glVertexAttribL1ui64NV) \
    USE_GL_FUNC(glVertexAttribL1ui64vARB) \
    USE_GL_FUNC(glVertexAttribL1ui64vNV) \
    USE_GL_FUNC(glVertexAttribL2d) \
    USE_GL_FUNC(glVertexAttribL2dEXT) \
    USE_GL_FUNC(glVertexAttribL2dv) \
    USE_GL_FUNC(glVertexAttribL2dvEXT) \
    USE_GL_FUNC(glVertexAttribL2i64NV) \
    USE_GL_FUNC(glVertexAttribL2i64vNV) \
    USE_GL_FUNC(glVertexAttribL2ui64NV) \
    USE_GL_FUNC(glVertexAttribL2ui64vNV) \
    USE_GL_FUNC(glVertexAttribL3d) \
    USE_GL_FUNC(glVertexAttribL3dEXT) \
    USE_GL_FUNC(glVertexAttribL3dv) \
    USE_GL_FUNC(glVertexAttribL3dvEXT) \
    USE_GL_FUNC(glVertexAttribL3i64NV) \
    USE_GL_FUNC(glVertexAttribL3i64vNV) \
    USE_GL_FUNC(glVertexAttribL3ui64NV) \
    USE_GL_FUNC(glVertexAttribL3ui64vNV) \
    USE_GL_FUNC(glVertexAttribL4d) \
    USE_GL_FUNC(glVertexAttribL4dEXT) \
    USE_GL_FUNC(glVertexAttribL4dv) \
    USE_GL_FUNC(glVertexAttribL4dvEXT) \
    USE_GL_FUNC(glVertexAttribL4i64NV) \
    USE_GL_FUNC(glVertexAttribL4i64vNV) \
    USE_GL_FUNC(glVertexAttribL4ui64NV) \
    USE_GL_FUNC(glVertexAttribL4ui64vNV) \
    USE_GL_FUNC(glVertexAttribLFormat) \
    USE_GL_FUNC(glVertexAttribLFormatNV) \
    USE_GL_FUNC(glVertexAttribLPointer) \
    USE_GL_FUNC(glVertexAttribLPointerEXT) \
    USE_GL_FUNC(glVertexAttribP1ui) \
    USE_GL_FUNC(glVertexAttribP1uiv) \
    USE_GL_FUNC(glVertexAttribP2ui) \
    USE_GL_FUNC(glVertexAttribP2uiv) \
    USE_GL_FUNC(glVertexAttribP3ui) \
    USE_GL_FUNC(glVertexAttribP3uiv) \
    USE_GL_FUNC(glVertexAttribP4ui) \
    USE_GL_FUNC(glVertexAttribP4uiv) \
    USE_GL_FUNC(glVertexAttribParameteriAMD) \
    USE_GL_FUNC(glVertexAttribPointer) \
    USE_GL_FUNC(glVertexAttribPointerARB) \
    USE_GL_FUNC(glVertexAttribPointerNV) \
    USE_GL_FUNC(glVertexAttribs1dvNV) \
    USE_GL_FUNC(glVertexAttribs1fvNV) \
    USE_GL_FUNC(glVertexAttribs1hvNV) \
    USE_GL_FUNC(glVertexAttribs1svNV) \
    USE_GL_FUNC(glVertexAttribs2dvNV) \
    USE_GL_FUNC(glVertexAttribs2fvNV) \
    USE_GL_FUNC(glVertexAttribs2hvNV) \
    USE_GL_FUNC(glVertexAttribs2svNV) \
    USE_GL_FUNC(glVertexAttribs3dvNV) \
    USE_GL_FUNC(glVertexAttribs3fvNV) \
    USE_GL_FUNC(glVertexAttribs3hvNV) \
    USE_GL_FUNC(glVertexAttribs3svNV) \
    USE_GL_FUNC(glVertexAttribs4dvNV) \
    USE_GL_FUNC(glVertexAttribs4fvNV) \
    USE_GL_FUNC(glVertexAttribs4hvNV) \
    USE_GL_FUNC(glVertexAttribs4svNV) \
    USE_GL_FUNC(glVertexAttribs4ubvNV) \
    USE_GL_FUNC(glVertexBindingDivisor) \
    USE_GL_FUNC(glVertexBlendARB) \
    USE_GL_FUNC(glVertexBlendEnvfATI) \
    USE_GL_FUNC(glVertexBlendEnviATI) \
    USE_GL_FUNC(glVertexFormatNV) \
    USE_GL_FUNC(glVertexP2ui) \
    USE_GL_FUNC(glVertexP2uiv) \
    USE_GL_FUNC(glVertexP3ui) \
    USE_GL_FUNC(glVertexP3uiv) \
    USE_GL_FUNC(glVertexP4ui) \
    USE_GL_FUNC(glVertexP4uiv) \
    USE_GL_FUNC(glVertexPointerEXT) \
    USE_GL_FUNC(glVertexPointerListIBM) \
    USE_GL_FUNC(glVertexPointervINTEL) \
    USE_GL_FUNC(glVertexStream1dATI) \
    USE_GL_FUNC(glVertexStream1dvATI) \
    USE_GL_FUNC(glVertexStream1fATI) \
    USE_GL_FUNC(glVertexStream1fvATI) \
    USE_GL_FUNC(glVertexStream1iATI) \
    USE_GL_FUNC(glVertexStream1ivATI) \
    USE_GL_FUNC(glVertexStream1sATI) \
    USE_GL_FUNC(glVertexStream1svATI) \
    USE_GL_FUNC(glVertexStream2dATI) \
    USE_GL_FUNC(glVertexStream2dvATI) \
    USE_GL_FUNC(glVertexStream2fATI) \
    USE_GL_FUNC(glVertexStream2fvATI) \
    USE_GL_FUNC(glVertexStream2iATI) \
    USE_GL_FUNC(glVertexStream2ivATI) \
    USE_GL_FUNC(glVertexStream2sATI) \
    USE_GL_FUNC(glVertexStream2svATI) \
    USE_GL_FUNC(glVertexStream3dATI) \
    USE_GL_FUNC(glVertexStream3dvATI) \
    USE_GL_FUNC(glVertexStream3fATI) \
    USE_GL_FUNC(glVertexStream3fvATI) \
    USE_GL_FUNC(glVertexStream3iATI) \
    USE_GL_FUNC(glVertexStream3ivATI) \
    USE_GL_FUNC(glVertexStream3sATI) \
    USE_GL_FUNC(glVertexStream3svATI) \
    USE_GL_FUNC(glVertexStream4dATI) \
    USE_GL_FUNC(glVertexStream4dvATI) \
    USE_GL_FUNC(glVertexStream4fATI) \
    USE_GL_FUNC(glVertexStream4fvATI) \
    USE_GL_FUNC(glVertexStream4iATI) \
    USE_GL_FUNC(glVertexStream4ivATI) \
    USE_GL_FUNC(glVertexStream4sATI) \
    USE_GL_FUNC(glVertexStream4svATI) \
    USE_GL_FUNC(glVertexWeightPointerEXT) \
    USE_GL_FUNC(glVertexWeightfEXT) \
    USE_GL_FUNC(glVertexWeightfvEXT) \
    USE_GL_FUNC(glVertexWeighthNV) \
    USE_GL_FUNC(glVertexWeighthvNV) \
    USE_GL_FUNC(glVideoCaptureNV) \
    USE_GL_FUNC(glVideoCaptureStreamParameterdvNV) \
    USE_GL_FUNC(glVideoCaptureStreamParameterfvNV) \
    USE_GL_FUNC(glVideoCaptureStreamParameterivNV) \
    USE_GL_FUNC(glViewportArrayv) \
    USE_GL_FUNC(glViewportIndexedf) \
    USE_GL_FUNC(glViewportIndexedfv) \
    USE_GL_FUNC(glViewportPositionWScaleNV) \
    USE_GL_FUNC(glViewportSwizzleNV) \
    USE_GL_FUNC(glWaitSemaphoreEXT) \
    USE_GL_FUNC(glWaitSemaphoreui64NVX) \
    USE_GL_FUNC(glWaitSync) \
    USE_GL_FUNC(glWaitVkSemaphoreNV) \
    USE_GL_FUNC(glWeightPathsNV) \
    USE_GL_FUNC(glWeightPointerARB) \
    USE_GL_FUNC(glWeightbvARB) \
    USE_GL_FUNC(glWeightdvARB) \
    USE_GL_FUNC(glWeightfvARB) \
    USE_GL_FUNC(glWeightivARB) \
    USE_GL_FUNC(glWeightsvARB) \
    USE_GL_FUNC(glWeightubvARB) \
    USE_GL_FUNC(glWeightuivARB) \
    USE_GL_FUNC(glWeightusvARB) \
    USE_GL_FUNC(glWindowPos2d) \
    USE_GL_FUNC(glWindowPos2dARB) \
    USE_GL_FUNC(glWindowPos2dMESA) \
    USE_GL_FUNC(glWindowPos2dv) \
    USE_GL_FUNC(glWindowPos2dvARB) \
    USE_GL_FUNC(glWindowPos2dvMESA) \
    USE_GL_FUNC(glWindowPos2f) \
    USE_GL_FUNC(glWindowPos2fARB) \
    USE_GL_FUNC(glWindowPos2fMESA) \
    USE_GL_FUNC(glWindowPos2fv) \
    USE_GL_FUNC(glWindowPos2fvARB) \
    USE_GL_FUNC(glWindowPos2fvMESA) \
    USE_GL_FUNC(glWindowPos2i) \
    USE_GL_FUNC(glWindowPos2iARB) \
    USE_GL_FUNC(glWindowPos2iMESA) \
    USE_GL_FUNC(glWindowPos2iv) \
    USE_GL_FUNC(glWindowPos2ivARB) \
    USE_GL_FUNC(glWindowPos2ivMESA) \
    USE_GL_FUNC(glWindowPos2s) \
    USE_GL_FUNC(glWindowPos2sARB) \
    USE_GL_FUNC(glWindowPos2sMESA) \
    USE_GL_FUNC(glWindowPos2sv) \
    USE_GL_FUNC(glWindowPos2svARB) \
    USE_GL_FUNC(glWindowPos2svMESA) \
    USE_GL_FUNC(glWindowPos3d) \
    USE_GL_FUNC(glWindowPos3dARB) \
    USE_GL_FUNC(glWindowPos3dMESA) \
    USE_GL_FUNC(glWindowPos3dv) \
    USE_GL_FUNC(glWindowPos3dvARB) \
    USE_GL_FUNC(glWindowPos3dvMESA) \
    USE_GL_FUNC(glWindowPos3f) \
    USE_GL_FUNC(glWindowPos3fARB) \
    USE_GL_FUNC(glWindowPos3fMESA) \
    USE_GL_FUNC(glWindowPos3fv) \
    USE_GL_FUNC(glWindowPos3fvARB) \
    USE_GL_FUNC(glWindowPos3fvMESA) \
    USE_GL_FUNC(glWindowPos3i) \
    USE_GL_FUNC(glWindowPos3iARB) \
    USE_GL_FUNC(glWindowPos3iMESA) \
    USE_GL_FUNC(glWindowPos3iv) \
    USE_GL_FUNC(glWindowPos3ivARB) \
    USE_GL_FUNC(glWindowPos3ivMESA) \
    USE_GL_FUNC(glWindowPos3s) \
    USE_GL_FUNC(glWindowPos3sARB) \
    USE_GL_FUNC(glWindowPos3sMESA) \
    USE_GL_FUNC(glWindowPos3sv) \
    USE_GL_FUNC(glWindowPos3svARB) \
    USE_GL_FUNC(glWindowPos3svMESA) \
    USE_GL_FUNC(glWindowPos4dMESA) \
    USE_GL_FUNC(glWindowPos4dvMESA) \
    USE_GL_FUNC(glWindowPos4fMESA) \
    USE_GL_FUNC(glWindowPos4fvMESA) \
    USE_GL_FUNC(glWindowPos4iMESA) \
    USE_GL_FUNC(glWindowPos4ivMESA) \
    USE_GL_FUNC(glWindowPos4sMESA) \
    USE_GL_FUNC(glWindowPos4svMESA) \
    USE_GL_FUNC(glWindowRectanglesEXT) \
    USE_GL_FUNC(glWriteMaskEXT)

#endif /* __WINE_WGL_H */
