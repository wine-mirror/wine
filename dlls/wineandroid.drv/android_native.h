/*
 * Android native system definitions
 *
 * Copyright 2013 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Copy of some Android native structures to avoid depending on the Android source */
/* Hopefully these won't change too frequently... */

#ifndef __WINE_ANDROID_NATIVE_H
#define __WINE_ANDROID_NATIVE_H

/* Native window definitions */

typedef struct native_handle
{
    int version;
    int numFds;
    int numInts;
    int data[0];
} native_handle_t;

typedef const native_handle_t *buffer_handle_t;

struct android_native_base_t
{
    int magic;
    int version;
    void *reserved[4];
    void (*incRef)(struct android_native_base_t *base);
    void (*decRef)(struct android_native_base_t *base);
};

typedef struct android_native_rect_t
{
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} android_native_rect_t;

struct ANativeWindowBuffer
{
    struct android_native_base_t common;
    int width;
    int height;
    int stride;
    int format;
    int usage;
    void *reserved[2];
    buffer_handle_t handle;
    void *reserved_proc[8];
};

struct ANativeWindow
{
    struct android_native_base_t common;
    uint32_t flags;
    int      minSwapInterval;
    int      maxSwapInterval;
    float    xdpi;
    float    ydpi;
    intptr_t oem[4];
    int (*setSwapInterval)(struct ANativeWindow *window, int interval);
    int (*dequeueBuffer_DEPRECATED)(struct ANativeWindow *window, struct ANativeWindowBuffer **buffer);
    int (*lockBuffer_DEPRECATED)(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer);
    int (*queueBuffer_DEPRECATED)(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer);
    int (*query)(const struct ANativeWindow *window, int what, int *value);
    int (*perform)(struct ANativeWindow *window, int operation, ... );
    int (*cancelBuffer_DEPRECATED)(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer);
    int (*dequeueBuffer)(struct ANativeWindow *window, struct ANativeWindowBuffer **buffer, int *fenceFd);
    int (*queueBuffer)(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fenceFd);
    int (*cancelBuffer)(struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fenceFd);
};

enum native_window_query
{
    NATIVE_WINDOW_WIDTH                     = 0,
    NATIVE_WINDOW_HEIGHT                    = 1,
    NATIVE_WINDOW_FORMAT                    = 2,
    NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS    = 3,
    NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER = 4,
    NATIVE_WINDOW_CONCRETE_TYPE             = 5,
    NATIVE_WINDOW_DEFAULT_WIDTH             = 6,
    NATIVE_WINDOW_DEFAULT_HEIGHT            = 7,
    NATIVE_WINDOW_TRANSFORM_HINT            = 8,
    NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND   = 9
};

enum native_window_perform
{
    NATIVE_WINDOW_SET_USAGE                   = 0,
    NATIVE_WINDOW_CONNECT                     = 1,
    NATIVE_WINDOW_DISCONNECT                  = 2,
    NATIVE_WINDOW_SET_CROP                    = 3,
    NATIVE_WINDOW_SET_BUFFER_COUNT            = 4,
    NATIVE_WINDOW_SET_BUFFERS_GEOMETRY        = 5,
    NATIVE_WINDOW_SET_BUFFERS_TRANSFORM       = 6,
    NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP       = 7,
    NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS      = 8,
    NATIVE_WINDOW_SET_BUFFERS_FORMAT          = 9,
    NATIVE_WINDOW_SET_SCALING_MODE            = 10,
    NATIVE_WINDOW_LOCK                        = 11,
    NATIVE_WINDOW_UNLOCK_AND_POST             = 12,
    NATIVE_WINDOW_API_CONNECT                 = 13,
    NATIVE_WINDOW_API_DISCONNECT              = 14,
    NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS = 15,
    NATIVE_WINDOW_SET_POST_TRANSFORM_CROP     = 16
};

enum native_window_api
{
    NATIVE_WINDOW_API_EGL = 1,
    NATIVE_WINDOW_API_CPU = 2,
    NATIVE_WINDOW_API_MEDIA = 3,
    NATIVE_WINDOW_API_CAMERA = 4
};

enum android_pixel_format
{
    PF_RGBA_8888 = 1,
    PF_RGBX_8888 = 2,
    PF_RGB_888   = 3,
    PF_RGB_565   = 4,
    PF_BGRA_8888 = 5,
    PF_RGBA_5551 = 6,
    PF_RGBA_4444 = 7
};


/* Hardware module definitions */

struct hw_module_methods_t;
struct hw_device_t;
struct android_ycbcr;

struct hw_module_t
{
    uint32_t tag;
    uint16_t module_api_version;
    uint16_t hal_api_version;
    const char *id;
    const char *name;
    const char *author;
    struct hw_module_methods_t *methods;
    void *dso;
    void *reserved[32-7];
};

struct hw_module_methods_t
{
    int (*open)(const struct hw_module_t *module, const char *id, struct hw_device_t **device);
};

struct hw_device_t
{
    uint32_t tag;
    uint32_t version;
    struct hw_module_t *module;
    void *reserved[12];
    int (*close)(struct hw_device_t *device);
};

struct gralloc_module_t
{
    struct hw_module_t common;
    int (*registerBuffer)(struct gralloc_module_t const *module, buffer_handle_t handle);
    int (*unregisterBuffer)(struct gralloc_module_t const *module, buffer_handle_t handle);
    int (*lock)(struct gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w, int h, void **vaddr);
    int (*unlock)(struct gralloc_module_t const *module, buffer_handle_t handle);
    int (*perform)(struct gralloc_module_t const *module, int operation, ... );
    int (*lock_ycbcr)(struct gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w, int h, struct android_ycbcr *ycbcr);
    void *reserved_proc[6];
};

#define ANDROID_NATIVE_MAKE_CONSTANT(a,b,c,d) \
    (((unsigned)(a)<<24)|((unsigned)(b)<<16)|((unsigned)(c)<<8)|(unsigned)(d))

#define ANDROID_NATIVE_WINDOW_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','w','n','d')

#define ANDROID_NATIVE_BUFFER_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','b','f','r')

enum gralloc_usage
{
    GRALLOC_USAGE_SW_READ_NEVER         = 0x00000000,
    GRALLOC_USAGE_SW_READ_RARELY        = 0x00000002,
    GRALLOC_USAGE_SW_READ_OFTEN         = 0x00000003,
    GRALLOC_USAGE_SW_READ_MASK          = 0x0000000F,
    GRALLOC_USAGE_SW_WRITE_NEVER        = 0x00000000,
    GRALLOC_USAGE_SW_WRITE_RARELY       = 0x00000020,
    GRALLOC_USAGE_SW_WRITE_OFTEN        = 0x00000030,
    GRALLOC_USAGE_SW_WRITE_MASK         = 0x000000F0,
    GRALLOC_USAGE_HW_TEXTURE            = 0x00000100,
    GRALLOC_USAGE_HW_RENDER             = 0x00000200,
    GRALLOC_USAGE_HW_2D                 = 0x00000400,
    GRALLOC_USAGE_HW_COMPOSER           = 0x00000800,
    GRALLOC_USAGE_HW_FB                 = 0x00001000,
    GRALLOC_USAGE_HW_VIDEO_ENCODER      = 0x00010000,
    GRALLOC_USAGE_HW_CAMERA_WRITE       = 0x00020000,
    GRALLOC_USAGE_HW_CAMERA_READ        = 0x00040000,
    GRALLOC_USAGE_HW_CAMERA_ZSL         = 0x00060000,
    GRALLOC_USAGE_HW_CAMERA_MASK        = 0x00060000,
    GRALLOC_USAGE_HW_MASK               = 0x00071F00,
    GRALLOC_USAGE_EXTERNAL_DISP         = 0x00002000,
    GRALLOC_USAGE_PROTECTED             = 0x00004000,
    GRALLOC_USAGE_PRIVATE_0             = 0x10000000,
    GRALLOC_USAGE_PRIVATE_1             = 0x20000000,
    GRALLOC_USAGE_PRIVATE_2             = 0x40000000,
    GRALLOC_USAGE_PRIVATE_3             = 0x80000000,
    GRALLOC_USAGE_PRIVATE_MASK          = 0xF0000000,
};

#define GRALLOC_HARDWARE_MODULE_ID "gralloc"

extern int hw_get_module(const char *id, const struct hw_module_t **module);

typedef enum
{
    GRALLOC1_CAPABILITY_INVALID = 0,
    GRALLOC1_CAPABILITY_TEST_ALLOCATE = 1,
    GRALLOC1_CAPABILITY_LAYERED_BUFFERS = 2,
    GRALLOC1_CAPABILITY_RELEASE_IMPLY_DELETE = 3,
    GRALLOC1_LAST_CAPABILITY = 3,
} gralloc1_capability_t;

typedef enum
{
    GRALLOC1_FUNCTION_INVALID = 0,
    GRALLOC1_FUNCTION_DUMP = 1,
    GRALLOC1_FUNCTION_CREATE_DESCRIPTOR = 2,
    GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR = 3,
    GRALLOC1_FUNCTION_SET_CONSUMER_USAGE = 4,
    GRALLOC1_FUNCTION_SET_DIMENSIONS = 5,
    GRALLOC1_FUNCTION_SET_FORMAT = 6,
    GRALLOC1_FUNCTION_SET_PRODUCER_USAGE = 7,
    GRALLOC1_FUNCTION_GET_BACKING_STORE = 8,
    GRALLOC1_FUNCTION_GET_CONSUMER_USAGE = 9,
    GRALLOC1_FUNCTION_GET_DIMENSIONS = 10,
    GRALLOC1_FUNCTION_GET_FORMAT = 11,
    GRALLOC1_FUNCTION_GET_PRODUCER_USAGE = 12,
    GRALLOC1_FUNCTION_GET_STRIDE = 13,
    GRALLOC1_FUNCTION_ALLOCATE = 14,
    GRALLOC1_FUNCTION_RETAIN = 15,
    GRALLOC1_FUNCTION_RELEASE = 16,
    GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES = 17,
    GRALLOC1_FUNCTION_LOCK = 18,
    GRALLOC1_FUNCTION_LOCK_FLEX = 19,
    GRALLOC1_FUNCTION_UNLOCK = 20,
    GRALLOC1_FUNCTION_SET_LAYER_COUNT = 21,
    GRALLOC1_FUNCTION_GET_LAYER_COUNT = 22,
    GRALLOC1_FUNCTION_VALIDATE_BUFFER_SIZE = 23,
    GRALLOC1_FUNCTION_GET_TRANSPORT_SIZE = 24,
    GRALLOC1_FUNCTION_IMPORT_BUFFER = 25,
    GRALLOC1_LAST_FUNCTION = 25,
} gralloc1_function_descriptor_t;

typedef enum
{
    GRALLOC1_ERROR_NONE = 0,
    GRALLOC1_ERROR_BAD_DESCRIPTOR = 1,
    GRALLOC1_ERROR_BAD_HANDLE = 2,
    GRALLOC1_ERROR_BAD_VALUE = 3,
    GRALLOC1_ERROR_NOT_SHARED = 4,
    GRALLOC1_ERROR_NO_RESOURCES = 5,
    GRALLOC1_ERROR_UNDEFINED = 6,
    GRALLOC1_ERROR_UNSUPPORTED = 7,
} gralloc1_error_t;

typedef enum
{
    GRALLOC1_PRODUCER_USAGE_NONE = 0,
    GRALLOC1_PRODUCER_USAGE_CPU_READ = 1u << 1,
    GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN = 1u << 2 | GRALLOC1_PRODUCER_USAGE_CPU_READ,
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE = 1u << 5,
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN = 1u << 6 | GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
} gralloc1_producer_usage_t;

typedef enum
{
    GRALLOC1_CONSUMER_USAGE_NONE = 0,
    GRALLOC1_CONSUMER_USAGE_CPU_READ = 1u << 1,
    GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN = 1u << 2 | GRALLOC1_CONSUMER_USAGE_CPU_READ,
} gralloc1_consumer_usage_t;

typedef struct gralloc1_device
{
    struct hw_device_t common;
    void (*getCapabilities)(struct gralloc1_device *device, uint32_t *outCount, int32_t *outCapabilities);
    void* (*getFunction)(struct gralloc1_device *device, int32_t descriptor);
} gralloc1_device_t;

typedef struct gralloc1_rect
{
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
} gralloc1_rect_t;

#endif /* __WINE_ANDROID_NATIVE_H */
