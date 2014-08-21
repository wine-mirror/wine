/*
 * Copyright 2009, 2011 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_rendertarget_view_incref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

ULONG CDECL wined3d_rendertarget_view_decref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        wined3d_resource_decref(view->resource);
        HeapFree(GetProcessHeap(), 0, view);
    }

    return refcount;
}

void * CDECL wined3d_rendertarget_view_get_parent(const struct wined3d_rendertarget_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

struct wined3d_resource * CDECL wined3d_rendertarget_view_get_resource(const struct wined3d_rendertarget_view *view)
{
    TRACE("view %p.\n", view);

    return view->resource;
}

static void wined3d_rendertarget_view_init(struct wined3d_rendertarget_view *view,
        const struct wined3d_rendertarget_view_desc *desc, struct wined3d_resource *resource, void *parent)
{
    const struct wined3d_gl_info *gl_info = &resource->device->adapter->gl_info;

    view->refcount = 1;
    view->resource = resource;
    wined3d_resource_incref(resource);
    view->parent = parent;

    view->format = wined3d_get_format(gl_info, desc->format_id);
    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        view->sub_resource_idx = 0;
        view->buffer_offset = desc->u.buffer.start_idx;
        view->width = desc->u.buffer.count;
        view->height = 1;
        view->depth = 1;
    }
    else
    {
        struct wined3d_texture *texture = wined3d_texture_from_resource(resource);
        struct wined3d_resource *sub_resource;

        view->sub_resource_idx = desc->u.texture.layer_idx * texture->level_count + desc->u.texture.level_idx;
        sub_resource = wined3d_texture_get_sub_resource(texture, view->sub_resource_idx);

        view->buffer_offset = 0;
        view->width = sub_resource->width;
        view->height = sub_resource->height;
        view->depth = desc->u.texture.layer_count;
    }
}

HRESULT CDECL wined3d_rendertarget_view_create(const struct wined3d_rendertarget_view_desc *desc,
        struct wined3d_resource *resource, void *parent, struct wined3d_rendertarget_view **view)
{
    struct wined3d_rendertarget_view *object;

    TRACE("desc %p, resource %p, parent %p, view %p.\n",
            desc, resource, parent, view);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_rendertarget_view_init(object, desc, resource, parent);

    TRACE("Created render target view %p.\n", object);
    *view = object;

    return WINED3D_OK;
}
