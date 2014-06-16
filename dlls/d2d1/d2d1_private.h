/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_D2D1_PRIVATE_H
#define __WINE_D2D1_PRIVATE_H

#include "wine/debug.h"

#define COBJMACROS
#include "d2d1.h"

struct d2d_d3d_render_target
{
    ID2D1RenderTarget ID2D1RenderTarget_iface;
    LONG refcount;
};

void d2d_d3d_render_target_init(struct d2d_d3d_render_target *render_target, ID2D1Factory *factory,
        IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES *desc) DECLSPEC_HIDDEN;

struct d2d_brush
{
    ID2D1Brush ID2D1Brush_iface;
    LONG refcount;
};

void d2d_solid_color_brush_init(struct d2d_brush *brush, ID2D1RenderTarget *render_target,
        const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *desc) DECLSPEC_HIDDEN;

#endif /* __WINE_D2D1_PRIVATE_H */
