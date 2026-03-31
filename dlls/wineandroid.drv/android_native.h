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

struct ANativeWindowBuffer;

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
    NATIVE_WINDOW_API_CPU = 2
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

#define ANDROID_NATIVE_MAKE_CONSTANT(a,b,c,d) \
    (((unsigned)(a)<<24)|((unsigned)(b)<<16)|((unsigned)(c)<<8)|(unsigned)(d))

#define ANDROID_NATIVE_WINDOW_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','w','n','d')

#endif /* __WINE_ANDROID_NATIVE_H */
