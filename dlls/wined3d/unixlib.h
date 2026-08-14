/*
 * Copyright 2024 Elizabeth Figura for CodeWeavers
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

#ifndef __WINE_WINED3D_UNIXLIB_H
#define __WINE_WINED3D_UNIXLIB_H

/* We cannot put these definitions in wined3d_private.h, because
 * wined3d_private.h uses vkd3d-shader definitions, and we cannot access
 * vkd3d_shader.h from the Unix side. */

#define COBJMACROS
#include <stdint.h>
#include "objbase.h"
#include "wine/wined3d.h"
#include "wine/unixlib.h"

#define VA_DECODER_SURFACE_COUNT 17

struct va_get_profiles_vk_params
{
    UINT64 physical_device; /* VkPhysicalDevice */
    UINT64 count; /* unsigned int * */
    UINT64 profiles; /* GUID * */
};

struct va_decoder_create_vk_params
{
    struct wined3d_decoder_desc desc;
    UINT64 physical_device; /* VkPhysicalDevice */
    UINT64 device; /* VkDevice */
    UINT64 decoder;
    unsigned int width;
    unsigned int height;
    struct
    {
        UINT64 image; /* VkImage */
    } surfaces[VA_DECODER_SURFACE_COUNT];
};

struct va_decoder_destroy_vk_params
{
    UINT64 device; /* VkDevice */
    UINT64 decoder;
};

struct va_decoder_decode_params
{
    UINT64 decoder;
    UINT64 bitstream, parameters, matrix, slice_control;
    UINT32 bitstream_size, slice_control_size;
    UINT32 output_idx;
};

enum unix_funcs
{
    unix_va_get_profiles_vk,
    unix_va_decoder_create_vk,
    unix_va_decoder_destroy_vk,
    unix_va_decoder_decode,
};

#endif
