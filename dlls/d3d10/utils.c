/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
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
 *
 */

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

#define WINE_D3D10_TO_STR(x) case x: return #x

const char *debug_d3d10_driver_type(D3D10_DRIVER_TYPE driver_type)
{
    switch(driver_type)
    {
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_HARDWARE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_REFERENCE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_NULL);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_SOFTWARE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_WARP);
        default:
            FIXME("Unrecognized D3D10_DRIVER_TYPE %#x\n", driver_type);
            return "unrecognized";
    }
}

const char *debug_d3d10_device_state_types(D3D10_DEVICE_STATE_TYPES t)
{
    switch (t)
    {
        WINE_D3D10_TO_STR(D3D10_DST_SO_BUFFERS);
        WINE_D3D10_TO_STR(D3D10_DST_OM_RENDER_TARGETS);
        WINE_D3D10_TO_STR(D3D10_DST_DEPTH_STENCIL_STATE);
        WINE_D3D10_TO_STR(D3D10_DST_BLEND_STATE);
        WINE_D3D10_TO_STR(D3D10_DST_VS);
        WINE_D3D10_TO_STR(D3D10_DST_VS_SAMPLERS);
        WINE_D3D10_TO_STR(D3D10_DST_VS_SHADER_RESOURCES);
        WINE_D3D10_TO_STR(D3D10_DST_VS_CONSTANT_BUFFERS);
        WINE_D3D10_TO_STR(D3D10_DST_GS);
        WINE_D3D10_TO_STR(D3D10_DST_GS_SAMPLERS);
        WINE_D3D10_TO_STR(D3D10_DST_GS_SHADER_RESOURCES);
        WINE_D3D10_TO_STR(D3D10_DST_GS_CONSTANT_BUFFERS);
        WINE_D3D10_TO_STR(D3D10_DST_PS);
        WINE_D3D10_TO_STR(D3D10_DST_PS_SAMPLERS);
        WINE_D3D10_TO_STR(D3D10_DST_PS_SHADER_RESOURCES);
        WINE_D3D10_TO_STR(D3D10_DST_PS_CONSTANT_BUFFERS);
        WINE_D3D10_TO_STR(D3D10_DST_IA_VERTEX_BUFFERS);
        WINE_D3D10_TO_STR(D3D10_DST_IA_INDEX_BUFFER);
        WINE_D3D10_TO_STR(D3D10_DST_IA_INPUT_LAYOUT);
        WINE_D3D10_TO_STR(D3D10_DST_IA_PRIMITIVE_TOPOLOGY);
        WINE_D3D10_TO_STR(D3D10_DST_RS_VIEWPORTS);
        WINE_D3D10_TO_STR(D3D10_DST_RS_SCISSOR_RECTS);
        WINE_D3D10_TO_STR(D3D10_DST_RS_RASTERIZER_STATE);
        WINE_D3D10_TO_STR(D3D10_DST_PREDICATION);
        default:
            FIXME("Unrecognized D3D10_DEVICE_STATE_TYPES %#x.\n", t);
            return "unrecognized";
    }
}

#undef WINE_D3D10_TO_STR
