/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2009, 2013 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define WINED3D_TEXTURE_DYNAMIC_MAP_THRESHOLD 50

struct wined3d_texture_idx
{
    struct wined3d_texture *texture;
    unsigned int sub_resource_idx;
};

bool wined3d_texture_validate_sub_resource_idx(const struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    if (sub_resource_idx < texture->level_count * texture->layer_count)
        return true;

    WARN("Invalid sub-resource index %u.\n", sub_resource_idx);
    return false;
}

BOOL wined3d_texture_can_use_pbo(const struct wined3d_texture *texture, const struct wined3d_d3d_info *d3d_info)
{
    if (!d3d_info->pbo || texture->resource.format->conv_byte_count || texture->resource.pin_sysmem)
        return FALSE;

    return TRUE;
}

static BOOL wined3d_texture_use_pbo(const struct wined3d_texture *texture, const struct wined3d_d3d_info *d3d_info)
{
    if (!wined3d_texture_can_use_pbo(texture, d3d_info))
        return FALSE;

    /* Use a PBO for dynamic textures and read-only staging textures. */
    return (!(texture->resource.access & WINED3D_RESOURCE_ACCESS_CPU)
                && texture->resource.usage & WINED3DUSAGE_DYNAMIC)
            || texture->resource.access == (WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R);
}

/* Front buffer coordinates are always full screen coordinates, but our GL
 * drawable is limited to the window's client area. The sysmem and texture
 * copies do have the full screen size. Note that GL has a bottom-left
 * origin, while D3D has a top-left origin. */
void wined3d_texture_translate_drawable_coords(const struct wined3d_texture *texture, HWND window, RECT *rect)
{
    unsigned int drawable_height;
    POINT offset = {0, 0};
    RECT windowsize;

    if (!texture->swapchain)
        return;

    if (texture == texture->swapchain->front_buffer)
    {
        ScreenToClient(window, &offset);
        OffsetRect(rect, offset.x, offset.y);
    }

    GetClientRect(window, &windowsize);
    drawable_height = windowsize.bottom - windowsize.top;

    rect->top = drawable_height - rect->top;
    rect->bottom = drawable_height - rect->bottom;
}

static uint32_t wined3d_resource_access_from_location(uint32_t location)
{
    switch (location)
    {
        case WINED3D_LOCATION_DISCARDED:
            return 0;

        case WINED3D_LOCATION_SYSMEM:
            return WINED3D_RESOURCE_ACCESS_CPU;

        case WINED3D_LOCATION_BUFFER:
        case WINED3D_LOCATION_DRAWABLE:
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
        case WINED3D_LOCATION_RB_RESOLVED:
            return WINED3D_RESOURCE_ACCESS_GPU;

        default:
            FIXME("Unhandled location %#x.\n", location);
            return 0;
    }
}

static void wined3d_texture_evict_sysmem(struct wined3d_texture *texture)
{
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int i, sub_count;

    if ((texture->flags & WINED3D_TEXTURE_CONVERTED)
            || texture->resource.pin_sysmem
            || texture->download_count > WINED3D_TEXTURE_DYNAMIC_MAP_THRESHOLD)
    {
        TRACE("Not evicting system memory for texture %p.\n", texture);
        return;
    }

    TRACE("Evicting system memory for texture %p.\n", texture);

    sub_count = texture->level_count * texture->layer_count;
    for (i = 0; i < sub_count; ++i)
    {
        sub_resource = &texture->sub_resources[i];
        if (sub_resource->locations == WINED3D_LOCATION_SYSMEM)
            ERR("WINED3D_LOCATION_SYSMEM is the only location for sub-resource %u of texture %p.\n",
                    i, texture);
        sub_resource->locations &= ~WINED3D_LOCATION_SYSMEM;
    }
    wined3d_resource_free_sysmem(&texture->resource);
}

void wined3d_texture_validate_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, uint32_t location)
{
    struct wined3d_texture_sub_resource *sub_resource;
    DWORD previous_locations;

    TRACE("texture %p, sub_resource_idx %u, location %s.\n",
            texture, sub_resource_idx, wined3d_debug_location(location));

    sub_resource = &texture->sub_resources[sub_resource_idx];
    previous_locations = sub_resource->locations;
    sub_resource->locations |= location;
    if (previous_locations == WINED3D_LOCATION_SYSMEM && location != WINED3D_LOCATION_SYSMEM
            && !--texture->sysmem_count)
        wined3d_texture_evict_sysmem(texture);

    TRACE("New locations flags are %s.\n", wined3d_debug_location(sub_resource->locations));
}

void wined3d_texture_invalidate_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, uint32_t location)
{
    struct wined3d_texture_sub_resource *sub_resource;
    DWORD previous_locations;

    TRACE("texture %p, sub_resource_idx %u, location %s.\n",
            texture, sub_resource_idx, wined3d_debug_location(location));

    if (location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
        wined3d_texture_set_dirty(texture);

    sub_resource = &texture->sub_resources[sub_resource_idx];
    previous_locations = sub_resource->locations;
    sub_resource->locations &= ~location;
    if (previous_locations != WINED3D_LOCATION_SYSMEM && sub_resource->locations == WINED3D_LOCATION_SYSMEM)
        ++texture->sysmem_count;

    TRACE("New locations flags are %s.\n", wined3d_debug_location(sub_resource->locations));

    if (!sub_resource->locations)
        ERR("Sub-resource %u of texture %p does not have any up to date location.\n",
                sub_resource_idx, texture);
}

void wined3d_texture_get_bo_address(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_bo_address *data, uint32_t location)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];

    if (location == WINED3D_LOCATION_BUFFER)
    {
        data->addr = NULL;
        data->buffer_object = sub_resource->bo;
    }
    else
    {
        assert(location == WINED3D_LOCATION_SYSMEM);
        if (sub_resource->user_memory)
        {
            data->addr = sub_resource->user_memory;
        }
        else
        {
            data->addr = texture->resource.heap_memory;
            data->addr += sub_resource->offset;
        }
        data->buffer_object = 0;
    }
}

void wined3d_texture_clear_dirty_regions(struct wined3d_texture *texture)
{
    unsigned int i;

    TRACE("texture %p\n", texture);

    if (!texture->dirty_regions)
        return;

    for (i = 0; i < texture->layer_count; ++i)
    {
        texture->dirty_regions[i].box_count = 0;
    }
}

/* Context activation is done by the caller. Context may be NULL in
 * WINED3D_NO3D mode. */
BOOL wined3d_texture_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, uint32_t location)
{
    DWORD current = texture->sub_resources[sub_resource_idx].locations;
    BOOL ret;

    TRACE("texture %p, sub_resource_idx %u, context %p, location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(location));

    TRACE("Current resource location %s.\n", wined3d_debug_location(current));

    if (current & location)
    {
        TRACE("Location %s is already up to date.\n", wined3d_debug_location(location));
        return TRUE;
    }

    if (WARN_ON(d3d))
    {
        uint32_t required_access = wined3d_resource_access_from_location(location);
        if ((texture->resource.access & required_access) != required_access)
            WARN("Operation requires %#x access, but texture only has %#x.\n",
                    required_access, texture->resource.access);
    }

    if (current & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("Sub-resource previously discarded, nothing to do.\n");
        if (!wined3d_texture_prepare_location(texture, sub_resource_idx, context, location))
            return FALSE;
        wined3d_texture_validate_location(texture, sub_resource_idx, location);
        wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_DISCARDED);
        return TRUE;
    }

    if (!current)
    {
        ERR("Sub-resource %u of texture %p does not have any up to date location.\n",
                sub_resource_idx, texture);
        wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_DISCARDED);
        return wined3d_texture_load_location(texture, sub_resource_idx, context, location);
    }

    if ((location & wined3d_texture_sysmem_locations)
            && (current & (wined3d_texture_sysmem_locations | WINED3D_LOCATION_CLEARED)))
    {
        struct wined3d_bo_address source, destination;
        struct wined3d_range range;

        if (!wined3d_texture_prepare_location(texture, sub_resource_idx, context, location))
            return FALSE;
        wined3d_texture_get_bo_address(texture, sub_resource_idx, &destination, location);
        range.offset = 0;
        range.size = texture->sub_resources[sub_resource_idx].size;
        if (current & WINED3D_LOCATION_CLEARED)
        {
            unsigned int level_idx = sub_resource_idx % texture->level_count;
            struct wined3d_map_desc map;
            struct wined3d_box box;
            struct wined3d_color c;

            if (texture->resource.format->caps[WINED3D_GL_RES_TYPE_TEX_2D]
                    & WINED3D_FORMAT_CAP_DEPTH_STENCIL)
            {
                c.r = texture->sub_resources[sub_resource_idx].clear_value.depth;
                c.g = texture->sub_resources[sub_resource_idx].clear_value.stencil;
                c.b = c.a = 0.0f;
            }
            else
            {
                c = texture->sub_resources[sub_resource_idx].clear_value.colour;
            }

            wined3d_texture_get_pitch(texture, level_idx, &map.row_pitch, &map.slice_pitch);
            if (destination.buffer_object)
                map.data = wined3d_context_map_bo_address(context, &destination, range.size,
                        WINED3D_MAP_WRITE | WINED3D_MAP_DISCARD);
            else
                map.data = destination.addr;

            wined3d_texture_get_level_box(texture, level_idx, &box);
            wined3d_resource_memory_colour_fill(&texture->resource, &map, &c, &box, true);

            if (destination.buffer_object)
                wined3d_context_unmap_bo_address(context, &destination, 1, &range);
        }
        else
        {
            wined3d_texture_get_bo_address(texture, sub_resource_idx,
                    &source, (current & wined3d_texture_sysmem_locations));
            wined3d_context_copy_bo_address(context, &destination, &source, 1, &range,
                    WINED3D_MAP_WRITE | WINED3D_MAP_DISCARD);
        }
        ret = TRUE;
    }
    else
    {
        ret = texture->texture_ops->texture_load_location(texture, sub_resource_idx, context, location);
    }

    if (ret)
        wined3d_texture_validate_location(texture, sub_resource_idx, location);

    return ret;
}

void wined3d_texture_get_memory(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, struct wined3d_bo_address *data)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    DWORD locations = sub_resource->locations;

    TRACE("texture %p, context %p, sub_resource_idx %u, data %p, locations %s.\n",
            texture, context, sub_resource_idx, data, wined3d_debug_location(locations));

    if (locations & (WINED3D_LOCATION_DISCARDED | WINED3D_LOCATION_CLEARED))
    {
        locations = (wined3d_texture_use_pbo(texture, context->d3d_info) ? WINED3D_LOCATION_BUFFER : WINED3D_LOCATION_SYSMEM);
        if (!wined3d_texture_load_location(texture, sub_resource_idx, context, locations))
        {
            data->buffer_object = 0;
            data->addr = NULL;
            return;
        }
    }
    if (locations & WINED3D_LOCATION_BUFFER)
    {
        wined3d_texture_get_bo_address(texture, sub_resource_idx, data, WINED3D_LOCATION_BUFFER);
        return;
    }

    if (locations & WINED3D_LOCATION_SYSMEM)
    {
        wined3d_texture_get_bo_address(texture, sub_resource_idx, data, WINED3D_LOCATION_SYSMEM);
        return;
    }

    ERR("Unexpected locations %s.\n", wined3d_debug_location(locations));
    data->addr = NULL;
    data->buffer_object = 0;
}

static void wined3d_texture_unload_location(struct wined3d_texture *texture,
        struct wined3d_context *context, unsigned int location)
{
    texture->texture_ops->texture_unload_location(texture, context, location);
}

static void wined3d_texture_update_map_binding(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    struct wined3d_device *device = texture->resource.device;
    DWORD map_binding = texture->update_map_binding;
    struct wined3d_context *context;
    unsigned int i;

    context = context_acquire(device, NULL, 0);

    for (i = 0; i < sub_count; ++i)
    {
        if (texture->sub_resources[i].locations == texture->resource.map_binding
                && !wined3d_texture_load_location(texture, i, context, map_binding))
            ERR("Failed to load location %s.\n", wined3d_debug_location(map_binding));
    }

    if (texture->resource.map_binding == WINED3D_LOCATION_BUFFER)
        wined3d_texture_unload_location(texture, context, WINED3D_LOCATION_BUFFER);

    context_release(context);

    texture->resource.map_binding = map_binding;
    texture->update_map_binding = 0;
}

static void wined3d_texture_dirty_region_add(struct wined3d_texture *texture,
        unsigned int layer, const struct wined3d_box *box)
{
    struct wined3d_dirty_regions *regions;
    unsigned int count;

    if (!texture->dirty_regions)
        return;

    regions = &texture->dirty_regions[layer];
    count = regions->box_count + 1;
    if (count >= WINED3D_MAX_DIRTY_REGION_COUNT || !box
            || (!box->left && !box->top && !box->front
            && box->right == texture->resource.width
            && box->bottom == texture->resource.height
            && box->back == texture->resource.depth))
    {
        regions->box_count = WINED3D_MAX_DIRTY_REGION_COUNT;
        return;
    }

    if (!wined3d_array_reserve((void **)&regions->boxes, &regions->boxes_size, count, sizeof(*regions->boxes)))
    {
        ERR("Failed to grow boxes array, marking entire texture dirty.\n");
        regions->box_count = WINED3D_MAX_DIRTY_REGION_COUNT;
        return;
    }

    regions->boxes[regions->box_count++] = *box;
}

void wined3d_texture_sub_resources_destroyed(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int i;

    for (i = 0; i < sub_count; ++i)
    {
        sub_resource = &texture->sub_resources[i];
        if (sub_resource->parent)
        {
            TRACE("sub-resource %u.\n", i);
            sub_resource->parent_ops->wined3d_object_destroyed(sub_resource->parent);
            sub_resource->parent = NULL;
        }
    }
}

static void wined3d_texture_create_dc(void *object)
{
    const struct wined3d_texture_idx *idx = object;
    struct wined3d_context *context = NULL;
    unsigned int sub_resource_idx, level;
    const struct wined3d_format *format;
    unsigned int row_pitch, slice_pitch;
    struct wined3d_texture *texture;
    struct wined3d_dc_info *dc_info;
    struct wined3d_bo_address data;
    D3DKMT_CREATEDCFROMMEMORY desc;
    struct wined3d_device *device;
    NTSTATUS status;

    TRACE("texture %p, sub_resource_idx %u.\n", idx->texture, idx->sub_resource_idx);

    texture = idx->texture;
    sub_resource_idx = idx->sub_resource_idx;
    level = sub_resource_idx % texture->level_count;
    device = texture->resource.device;

    format = texture->resource.format;
    if (!format->ddi_format)
    {
        WARN("Cannot create a DC for format %s.\n", debug_d3dformat(format->id));
        return;
    }

    if (!texture->dc_info)
    {
        unsigned int sub_count = texture->level_count * texture->layer_count;

        if (!(texture->dc_info = calloc(sub_count, sizeof(*texture->dc_info))))
        {
            ERR("Failed to allocate DC info.\n");
            return;
        }
    }

    if (!(texture->sub_resources[sub_resource_idx].locations & texture->resource.map_binding))
    {
        context = context_acquire(device, NULL, 0);
        wined3d_texture_load_location(texture, sub_resource_idx, context, texture->resource.map_binding);
    }

    if (texture->dirty_regions)
        wined3d_texture_dirty_region_add(texture, sub_resource_idx / texture->level_count, NULL);

    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~texture->resource.map_binding);
    wined3d_texture_get_pitch(texture, level, &row_pitch, &slice_pitch);
    wined3d_texture_get_bo_address(texture, sub_resource_idx, &data, texture->resource.map_binding);
    if (data.buffer_object)
    {
        if (!context)
            context = context_acquire(device, NULL, 0);
        desc.pMemory = wined3d_context_map_bo_address(context, &data,
                texture->sub_resources[sub_resource_idx].size, WINED3D_MAP_READ | WINED3D_MAP_WRITE);
    }
    else
    {
        desc.pMemory = data.addr;
    }

    if (context)
        context_release(context);

    desc.Format = format->ddi_format;
    desc.Width = wined3d_texture_get_level_width(texture, level);
    desc.Height = wined3d_texture_get_level_height(texture, level);
    desc.Pitch = row_pitch;
    desc.hDeviceDc = CreateCompatibleDC(NULL);
    desc.pColorTable = NULL;

    status = D3DKMTCreateDCFromMemory(&desc);
    DeleteDC(desc.hDeviceDc);
    if (status)
    {
        WARN("Failed to create DC, status %#lx.\n", status);
        return;
    }

    dc_info = &texture->dc_info[sub_resource_idx];
    dc_info->dc = desc.hDc;
    dc_info->bitmap = desc.hBitmap;

    TRACE("Created DC %p, bitmap %p for texture %p, %u.\n", dc_info->dc, dc_info->bitmap, texture, sub_resource_idx);
}

static void wined3d_texture_destroy_dc(void *object)
{
    const struct wined3d_texture_idx *idx = object;
    D3DKMT_DESTROYDCFROMMEMORY destroy_desc;
    struct wined3d_context *context;
    struct wined3d_texture *texture;
    struct wined3d_dc_info *dc_info;
    struct wined3d_bo_address data;
    unsigned int sub_resource_idx;
    struct wined3d_device *device;
    struct wined3d_range range;
    NTSTATUS status;

    TRACE("texture %p, sub_resource_idx %u.\n", idx->texture, idx->sub_resource_idx);

    texture = idx->texture;
    sub_resource_idx = idx->sub_resource_idx;
    device = texture->resource.device;
    dc_info = &texture->dc_info[sub_resource_idx];

    if (!dc_info->dc)
    {
        ERR("Sub-resource {%p, %u} has no DC.\n", texture, sub_resource_idx);
        return;
    }

    TRACE("dc %p, bitmap %p.\n", dc_info->dc, dc_info->bitmap);

    destroy_desc.hDc = dc_info->dc;
    destroy_desc.hBitmap = dc_info->bitmap;
    if ((status = D3DKMTDestroyDCFromMemory(&destroy_desc)))
        ERR("Failed to destroy dc, status %#lx.\n", status);
    dc_info->dc = NULL;
    dc_info->bitmap = NULL;

    wined3d_texture_get_bo_address(texture, sub_resource_idx, &data, texture->resource.map_binding);
    if (data.buffer_object)
    {
        context = context_acquire(device, NULL, 0);
        range.offset = 0;
        range.size = texture->sub_resources[sub_resource_idx].size;
        wined3d_context_unmap_bo_address(context, &data, 1, &range);
        context_release(context);
    }
}

void wined3d_texture_set_swapchain(struct wined3d_texture *texture, struct wined3d_swapchain *swapchain)
{
    texture->swapchain = swapchain;
    wined3d_resource_update_draw_binding(&texture->resource);
}

ULONG CDECL wined3d_texture_incref(struct wined3d_texture *texture)
{
    unsigned int refcount;

    TRACE("texture %p, swapchain %p.\n", texture, texture->swapchain);

    refcount = InterlockedIncrement(&texture->resource.ref);
    TRACE("%p increasing refcount to %u.\n", texture, refcount);

    return refcount;
}

static void wined3d_texture_destroy_object(void *object)
{
    struct wined3d_texture *texture = object;
    struct wined3d_resource *resource;
    struct wined3d_dc_info *dc_info;
    unsigned int sub_count;
    unsigned int i;

    TRACE("texture %p.\n", texture);

    resource = &texture->resource;
    sub_count = texture->level_count * texture->layer_count;

    if ((dc_info = texture->dc_info))
    {
        for (i = 0; i < sub_count; ++i)
        {
            if (dc_info[i].dc)
            {
                struct wined3d_texture_idx texture_idx = {texture, i};

                wined3d_texture_destroy_dc(&texture_idx);
            }
        }
        free(dc_info);
    }

    if (texture->overlay_info)
    {
        for (i = 0; i < sub_count; ++i)
        {
            struct wined3d_overlay_info *info = &texture->overlay_info[i];
            struct wined3d_overlay_info *overlay, *cur;

            list_remove(&info->entry);
            LIST_FOR_EACH_ENTRY_SAFE(overlay, cur, &info->overlays, struct wined3d_overlay_info, entry)
            {
                list_remove(&overlay->entry);
            }
        }
        free(texture->overlay_info);
    }

    if (texture->dirty_regions)
    {
        for (i = 0; i < texture->layer_count; ++i)
        {
            free(texture->dirty_regions[i].boxes);
        }
        free(texture->dirty_regions);
    }

    /* Discard the contents of resources with CPU access, to avoid downloading
     * them to SYSMEM on unload. */
    if (resource->access & WINED3D_RESOURCE_ACCESS_CPU)
    {
        for (i = 0; i < sub_count; ++i)
        {
            wined3d_texture_validate_location(texture, i, WINED3D_LOCATION_DISCARDED);
            wined3d_texture_invalidate_location(texture, i, ~WINED3D_LOCATION_DISCARDED);
        }
    }
    resource->resource_ops->resource_unload(resource);
}

void wined3d_texture_cleanup(struct wined3d_texture *texture)
{
    wined3d_cs_destroy_object(texture->resource.device->cs, wined3d_texture_destroy_object, texture);
    resource_cleanup(&texture->resource);
}

static void wined3d_texture_cleanup_sync(struct wined3d_texture *texture)
{
    wined3d_texture_sub_resources_destroyed(texture);
    wined3d_texture_cleanup(texture);
    wined3d_resource_wait_idle(&texture->resource);
}

ULONG CDECL wined3d_texture_decref(struct wined3d_texture *texture)
{
    unsigned int i, sub_resource_count, refcount;

    TRACE("texture %p, swapchain %p.\n", texture, texture->swapchain);

    refcount = InterlockedDecrement(&texture->resource.ref);
    TRACE("%p decreasing refcount to %u.\n", texture, refcount);

    if (!refcount)
    {
        bool in_cs_thread = GetCurrentThreadId() == texture->resource.device->cs->thread_id;

        if (texture->identity_srv)
        {
            assert(!in_cs_thread);
            wined3d_shader_resource_view_destroy(texture->identity_srv);
        }

        /* This is called from the CS thread to destroy temporary textures. */
        if (!in_cs_thread)
            wined3d_mutex_lock();
        /* Wait for the texture to become idle if it's using user memory,
         * since the application is allowed to free that memory once the
         * texture is destroyed. Note that this implies that
         * the destroy handler can't access that memory either. */
        sub_resource_count = texture->layer_count * texture->level_count;
        for (i = 0; i < sub_resource_count; ++i)
        {
            if (texture->sub_resources[i].user_memory)
            {
                wined3d_resource_wait_idle(&texture->resource);
                break;
            }
        }
        texture->resource.device->adapter->adapter_ops->adapter_destroy_texture(texture);
        if (!in_cs_thread)
            wined3d_mutex_unlock();
    }

    return refcount;
}

struct wined3d_resource * CDECL wined3d_texture_get_resource(struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return &texture->resource;
}

/* Context activation is done by the caller */
void wined3d_texture_load(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    DWORD flag;
    UINT i;

    TRACE("texture %p, context %p, srgb %#x.\n", texture, context, srgb);

    if (!needs_separate_srgb_gl_texture(context, texture))
        srgb = FALSE;

    if (srgb)
        flag = WINED3D_TEXTURE_SRGB_VALID;
    else
        flag = WINED3D_TEXTURE_RGB_VALID;

    if (texture->flags & flag)
    {
        TRACE("Texture %p not dirty, nothing to do.\n", texture);
        return;
    }

    /* Reload the surfaces if the texture is marked dirty. */
    for (i = 0; i < sub_count; ++i)
    {
        if (!wined3d_texture_load_location(texture, i, context,
                srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB))
            ERR("Failed to load location (srgb %#x).\n", srgb);
    }
    texture->flags |= flag;
}

void * CDECL wined3d_texture_get_parent(const struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return texture->resource.parent;
}

void CDECL wined3d_texture_get_pitch(const struct wined3d_texture *texture,
        unsigned int level, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    const struct wined3d_resource *resource = &texture->resource;
    unsigned int width = wined3d_texture_get_level_width(texture, level);
    unsigned int height = wined3d_texture_get_level_height(texture, level);

    if (texture->row_pitch)
    {
        *row_pitch = texture->row_pitch;
        *slice_pitch = texture->slice_pitch;
        return;
    }

    wined3d_format_calculate_pitch(resource->format, resource->device->surface_alignment,
            width, height, row_pitch, slice_pitch);
}

unsigned int CDECL wined3d_texture_get_lod(const struct wined3d_texture *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->lod);

    return texture->lod;
}

unsigned int CDECL wined3d_texture_set_lod(struct wined3d_texture *texture, unsigned int lod)
{
    struct wined3d_resource *resource;
    unsigned int old = texture->lod;

    TRACE("texture %p, returning %u.\n", texture, texture->lod);

    /* The d3d9:texture test shows that SetLOD is ignored on non-managed
     * textures. The call always returns 0, and GetLOD always returns 0. */
    resource = &texture->resource;
    if (!(resource->usage & WINED3DUSAGE_MANAGED))
    {
        TRACE("Ignoring LOD on texture with resource access %s.\n",
                wined3d_debug_resource_access(resource->access));
        return 0;
    }

    if (lod >= texture->level_count)
        lod = texture->level_count - 1;

    texture->lod = lod;
    return old;
}

UINT CDECL wined3d_texture_get_level_count(const struct wined3d_texture *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->level_count);

    return texture->level_count;
}

HRESULT CDECL wined3d_texture_set_color_key(struct wined3d_texture *texture,
        uint32_t flags, const struct wined3d_color_key *color_key)
{
    struct wined3d_device *device = texture->resource.device;
    static const DWORD all_flags = WINED3D_CKEY_DST_BLT | WINED3D_CKEY_DST_OVERLAY
            | WINED3D_CKEY_SRC_BLT | WINED3D_CKEY_SRC_OVERLAY;

    TRACE("texture %p, flags %#x, color_key %p.\n", texture, flags, color_key);

    if (flags & ~all_flags)
    {
        WARN("Invalid flags passed, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (color_key)
    {
        if (flags & WINED3D_CKEY_SRC_BLT)
            texture->src_blt_color_key = *color_key;
        texture->color_key_flags |= flags;
    }
    else
    {
        texture->color_key_flags &= ~flags;
    }

    wined3d_cs_emit_set_color_key(device->cs, texture, flags, color_key);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_update_desc(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *mem, unsigned int pitch)
{
    unsigned int current_row_pitch, current_slice_pitch, width, height;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int i, level, sub_resource_count;
    const struct wined3d_format *format;
    struct wined3d_device *device;
    unsigned int slice_pitch;
    bool update_memory_only;
    bool create_dib = false;

    TRACE("texture %p, sub_resource_idx %u, mem %p, pitch %u.\n", texture, sub_resource_idx, mem, pitch);

    device = texture->resource.device;
    format = texture->resource.format;
    level = sub_resource_idx % texture->level_count;
    sub_resource_count = texture->level_count * texture->layer_count;

    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);
    if (pitch)
        slice_pitch = height * pitch;
    else
        wined3d_format_calculate_pitch(format, 1, width, height, &pitch, &slice_pitch);

    wined3d_texture_get_pitch(texture, level, &current_row_pitch, &current_slice_pitch);
    update_memory_only = (pitch == current_row_pitch && slice_pitch == current_slice_pitch);

    if (sub_resource_count > 1 && !update_memory_only)
    {
        FIXME("Texture has multiple sub-resources, not supported.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Not supported on %s.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->resource.map_count)
    {
        WARN("Texture is mapped.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* We have no way of supporting a pitch that is not a multiple of the pixel
     * byte width short of uploading the texture row-by-row.
     * Fortunately that's not an issue since D3D9Ex doesn't allow a custom pitch
     * for user-memory textures (it always expects packed data) while DirectDraw
     * requires a 4-byte aligned pitch and doesn't support texture formats
     * larger than 4 bytes per pixel nor any format using 3 bytes per pixel.
     * This check is here to verify that the assumption holds. */
    if (pitch % format->byte_count)
    {
        WARN("Pitch unsupported, not a multiple of the texture format byte width.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->d3d_initialized)
        wined3d_cs_emit_unload_resource(device->cs, &texture->resource);
    wined3d_resource_wait_idle(&texture->resource);

    if (texture->dc_info && texture->dc_info[0].dc)
    {
        struct wined3d_texture_idx texture_idx = {texture, sub_resource_idx};

        wined3d_cs_destroy_object(device->cs, wined3d_texture_destroy_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
        create_dib = true;
    }

    texture->sub_resources[sub_resource_idx].user_memory = mem;

    if (update_memory_only)
    {
        for (i = 0; i < sub_resource_count; ++i)
            if (!texture->sub_resources[i].user_memory)
                break;

        if (i == sub_resource_count)
            wined3d_resource_free_sysmem(&texture->resource);
    }
    else
    {
        wined3d_resource_free_sysmem(&texture->resource);

        sub_resource = &texture->sub_resources[sub_resource_idx];

        texture->row_pitch = pitch;
        texture->slice_pitch = slice_pitch;

        if (!(texture->resource.access & WINED3D_RESOURCE_ACCESS_CPU)
                && texture->resource.usage & WINED3DUSAGE_VIDMEM_ACCOUNTING)
            adapter_adjust_memory(device->adapter,  (INT64)texture->slice_pitch - texture->resource.size);
        texture->resource.size = texture->slice_pitch;
        sub_resource->size = texture->slice_pitch;
        sub_resource->locations = WINED3D_LOCATION_DISCARDED;
    }

    if (!mem && !wined3d_resource_prepare_sysmem(&texture->resource))
        ERR("Failed to allocate resource memory.\n");

    wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_SYSMEM);
    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~WINED3D_LOCATION_SYSMEM);

    if (create_dib)
    {
        struct wined3d_texture_idx texture_idx = {texture, sub_resource_idx};

        wined3d_cs_init_object(device->cs, wined3d_texture_create_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    }

    return WINED3D_OK;
}

static void wined3d_texture_force_reload(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    unsigned int i;

    texture->flags &= ~(WINED3D_TEXTURE_RGB_ALLOCATED | WINED3D_TEXTURE_SRGB_ALLOCATED
            | WINED3D_TEXTURE_CONVERTED);
    texture->async.flags &= ~WINED3D_TEXTURE_ASYNC_COLOR_KEY;
    for (i = 0; i < sub_count; ++i)
    {
        wined3d_texture_invalidate_location(texture, i,
                WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
    }
}

BOOL wined3d_texture_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    return texture->texture_ops->texture_prepare_location(texture, sub_resource_idx, context, location);
}

static struct wined3d_texture_sub_resource *wined3d_texture_get_sub_resource(struct wined3d_texture *texture,
        unsigned int sub_resource_idx)
{
    TRACE("texture %p, sub_resource_idx %u.\n", texture, sub_resource_idx);

    if (!wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx))
        return NULL;
    return &texture->sub_resources[sub_resource_idx];
}

HRESULT CDECL wined3d_texture_add_dirty_region(struct wined3d_texture *texture,
        UINT layer, const struct wined3d_box *dirty_region)
{
    TRACE("texture %p, layer %u, dirty_region %s.\n", texture, layer, debug_box(dirty_region));

    if (layer >= texture->layer_count)
    {
        WARN("Invalid layer %u specified.\n", layer);
        return WINED3DERR_INVALIDCALL;
    }

    if (dirty_region && FAILED(wined3d_resource_check_box_dimensions(&texture->resource, 0, dirty_region)))
    {
        WARN("Invalid dirty_region %s specified.\n", debug_box(dirty_region));
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_texture_dirty_region_add(texture, layer, dirty_region);
    wined3d_cs_emit_add_dirty_texture_region(texture->resource.device->cs, texture, layer);

    return WINED3D_OK;
}

struct wined3d_texture * __cdecl wined3d_texture_from_resource(struct wined3d_resource *resource)
{
    return texture_from_resource(resource);
}

static ULONG texture_resource_incref(struct wined3d_resource *resource)
{
    return wined3d_texture_incref(texture_from_resource(resource));
}

static ULONG texture_resource_decref(struct wined3d_resource *resource)
{
    return wined3d_texture_decref(texture_from_resource(resource));
}

static void texture_resource_preload(struct wined3d_resource *resource)
{
    struct wined3d_texture *texture = texture_from_resource(resource);
    struct wined3d_context *context;

    context = context_acquire(resource->device, NULL, 0);
    wined3d_texture_load(texture, context, texture->flags & WINED3D_TEXTURE_IS_SRGB);
    context_release(context);
}

static void texture_resource_unload(struct wined3d_resource *resource)
{
    struct wined3d_texture *texture = texture_from_resource(resource);
    struct wined3d_device *device = resource->device;
    unsigned int location = resource->map_binding;
    struct wined3d_context *context;
    unsigned int sub_count, i;

    TRACE("resource %p.\n", resource);

    /* D3D is not initialised, so no GPU locations should currently exist.
     * Moreover, we may not be able to acquire a valid context. */
    if (!device->d3d_initialized)
        return;

    context = context_acquire(device, NULL, 0);

    if (location == WINED3D_LOCATION_BUFFER)
        location = WINED3D_LOCATION_SYSMEM;

    sub_count = texture->level_count * texture->layer_count;
    for (i = 0; i < sub_count; ++i)
    {
        if (resource->access & WINED3D_RESOURCE_ACCESS_CPU
                && wined3d_texture_load_location(texture, i, context, location))
        {
            wined3d_texture_invalidate_location(texture, i, ~location);
        }
        else
        {
            if (resource->access & WINED3D_RESOURCE_ACCESS_CPU)
                ERR("Discarding %s %p sub-resource %u with resource access %s.\n",
                        debug_d3dresourcetype(resource->type), resource, i,
                        wined3d_debug_resource_access(resource->access));
            wined3d_texture_validate_location(texture, i, WINED3D_LOCATION_DISCARDED);
            wined3d_texture_invalidate_location(texture, i, ~WINED3D_LOCATION_DISCARDED);
        }
    }

    wined3d_texture_unload_location(texture, context, WINED3D_LOCATION_BUFFER);
    wined3d_texture_unload_location(texture, context, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_unload_location(texture, context, WINED3D_LOCATION_TEXTURE_SRGB);
    wined3d_texture_unload_location(texture, context, WINED3D_LOCATION_RB_MULTISAMPLE);
    wined3d_texture_unload_location(texture, context, WINED3D_LOCATION_RB_RESOLVED);

    context_release(context);

    wined3d_texture_force_reload(texture);

    if (texture->resource.bind_count && (texture->resource.bind_flags & WINED3D_BIND_SHADER_RESOURCE))
        device_invalidate_state(device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);

    wined3d_texture_set_dirty(texture);

    resource_unload(&texture->resource);
}

static unsigned int texture_resource_get_sub_resource_count(struct wined3d_resource *resource)
{
    const struct wined3d_texture *texture = texture_from_resource(resource);

    return texture->level_count * texture->layer_count;
}

static HRESULT texture_resource_sub_resource_get_desc(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc)
{
    const struct wined3d_texture *texture = texture_from_resource(resource);

    return wined3d_texture_get_sub_resource_desc(texture, sub_resource_idx, desc);
}

static void texture_resource_sub_resource_get_map_pitch(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    const struct wined3d_texture *texture = texture_from_resource(resource);
    unsigned int level = sub_resource_idx % texture->level_count;

    if (resource->format_attrs & WINED3D_FORMAT_ATTR_BROKEN_PITCH)
    {
        *row_pitch = wined3d_texture_get_level_width(texture, level) * resource->format->byte_count;
        *slice_pitch = wined3d_texture_get_level_height(texture, level) * (*row_pitch);
    }
    else
    {
        wined3d_texture_get_pitch(texture, level, row_pitch, slice_pitch);
    }
}

static HRESULT texture_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        void **map_ptr, const struct wined3d_box *box, uint32_t flags)
{
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_device *device = resource->device;
    struct wined3d_context *context;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;
    unsigned int texture_level;
    BYTE *base_memory;
    BOOL ret = TRUE;

    TRACE("resource %p, sub_resource_idx %u, map_ptr %p, box %s, flags %#x.\n",
            resource, sub_resource_idx, map_ptr, debug_box(box), flags);

    texture = texture_from_resource(resource);
    sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx);

    texture_level = sub_resource_idx % texture->level_count;

    if (texture->flags & WINED3D_TEXTURE_DC_IN_USE)
    {
        WARN("DC is in use.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (sub_resource->map_count)
    {
        WARN("Sub-resource is already mapped.\n");
        return WINED3DERR_INVALIDCALL;
    }

    context = context_acquire(device, NULL, 0);

    if (flags & WINED3D_MAP_DISCARD)
    {
        TRACE("WINED3D_MAP_DISCARD flag passed, marking %s as up to date.\n",
                wined3d_debug_location(resource->map_binding));
        if ((ret = wined3d_texture_prepare_location(texture, sub_resource_idx, context, resource->map_binding)))
            wined3d_texture_validate_location(texture, sub_resource_idx, resource->map_binding);
    }
    else
    {
        if (resource->usage & WINED3DUSAGE_DYNAMIC)
            WARN_(d3d_perf)("Mapping a dynamic texture without WINED3D_MAP_DISCARD.\n");
        if (!texture_level)
        {
            unsigned int i;

            for (i = 0; i < texture->level_count; ++i)
            {
                if (!(ret = wined3d_texture_load_location(texture, sub_resource_idx + i, context, resource->map_binding)))
                    break;
            }
        }
        else
        {
            ret = wined3d_texture_load_location(texture, sub_resource_idx, context, resource->map_binding);
        }
    }

    if (!ret)
    {
        ERR("Failed to prepare location.\n");
        context_release(context);
        return E_OUTOFMEMORY;
    }

    /* We only record dirty regions for the top-most level. */
    if (texture->dirty_regions && flags & WINED3D_MAP_WRITE
            && !(flags & WINED3D_MAP_NO_DIRTY_UPDATE) && !texture_level)
        wined3d_texture_dirty_region_add(texture, sub_resource_idx / texture->level_count, box);

    if (flags & WINED3D_MAP_WRITE)
    {
        if (!texture_level)
        {
            unsigned int i;

            for (i = 0; i < texture->level_count; ++i)
                wined3d_texture_invalidate_location(texture, sub_resource_idx + i, ~resource->map_binding);
        }
        else
        {
            wined3d_texture_invalidate_location(texture, sub_resource_idx, ~resource->map_binding);
        }
    }

    wined3d_texture_get_bo_address(texture, sub_resource_idx, &data, resource->map_binding);
    base_memory = wined3d_context_map_bo_address(context, &data, sub_resource->size, flags);
    sub_resource->map_flags = flags;
    TRACE("Base memory pointer %p.\n", base_memory);

    context_release(context);

    *map_ptr = resource_offset_map_pointer(resource, sub_resource_idx, base_memory, box);

    if (texture->swapchain && texture->swapchain->front_buffer == texture)
    {
        RECT *r = &texture->swapchain->front_buffer_update;

        SetRect(r, box->left, box->top, box->right, box->bottom);
        TRACE("Mapped front buffer %s.\n", wine_dbgstr_rect(r));
    }

    ++resource->map_count;
    ++sub_resource->map_count;

    TRACE("Returning memory %p.\n", *map_ptr);

    return WINED3D_OK;
}

static HRESULT texture_resource_sub_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_device *device = resource->device;
    struct wined3d_context *context;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;
    struct wined3d_range range;

    TRACE("resource %p, sub_resource_idx %u.\n", resource, sub_resource_idx);

    texture = texture_from_resource(resource);
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return E_INVALIDARG;

    if (!sub_resource->map_count)
    {
        WARN("Trying to unmap unmapped sub-resource.\n");
        if (texture->flags & WINED3D_TEXTURE_DC_IN_USE)
            return WINED3D_OK;
        return WINEDDERR_NOTLOCKED;
    }

    context = context_acquire(device, NULL, 0);

    wined3d_texture_get_bo_address(texture, sub_resource_idx, &data, texture->resource.map_binding);
    range.offset = 0;
    range.size = sub_resource->size;
    wined3d_context_unmap_bo_address(context, &data, !!(sub_resource->map_flags & WINED3D_MAP_WRITE), &range);

    context_release(context);

    if (texture->swapchain && texture->swapchain->front_buffer == texture)
    {
        if (!(sub_resource->locations & (WINED3D_LOCATION_DRAWABLE | WINED3D_LOCATION_TEXTURE_RGB)))
            texture->swapchain->swapchain_ops->swapchain_frontbuffer_updated(texture->swapchain);
    }

    --sub_resource->map_count;
    if (!--resource->map_count && texture->update_map_binding)
        wined3d_texture_update_map_binding(texture);

    return WINED3D_OK;
}

static const struct wined3d_resource_ops texture_resource_ops =
{
    texture_resource_incref,
    texture_resource_decref,
    texture_resource_preload,
    texture_resource_unload,
    texture_resource_get_sub_resource_count,
    texture_resource_sub_resource_get_desc,
    texture_resource_sub_resource_get_map_pitch,
    texture_resource_sub_resource_map,
    texture_resource_sub_resource_unmap,
};

HRESULT wined3d_texture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        unsigned int layer_count, unsigned int level_count, uint32_t flags, struct wined3d_device *device,
        void *parent, const struct wined3d_parent_ops *parent_ops, void *sub_resources,
        const struct wined3d_texture_ops *texture_ops)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    struct wined3d_device_parent *device_parent = device->device_parent;
    unsigned int sub_count, i, j, size, offset = 0;
    const struct wined3d_format *format;
    HRESULT hr;

    TRACE("texture %p, resource_type %s, format %s, multisample_type %#x, multisample_quality %#x, "
            "usage %s, bind_flags %s, access %s, width %u, height %u, depth %u, layer_count %u, level_count %u, "
            "flags %#x, device %p, parent %p, parent_ops %p, sub_resources %p, texture_ops %p.\n",
            texture, debug_d3dresourcetype(desc->resource_type), debug_d3dformat(desc->format), desc->multisample_type,
            desc->multisample_quality, debug_d3dusage(desc->usage), wined3d_debug_bind_flags(desc->bind_flags),
            wined3d_debug_resource_access(desc->access), desc->width, desc->height, desc->depth,
            layer_count, level_count, flags, device, parent, parent_ops, sub_resources, texture_ops);

    if (!desc->width || !desc->height || !desc->depth)
        return WINED3DERR_INVALIDCALL;

    if (desc->resource_type == WINED3D_RTYPE_TEXTURE_3D && layer_count != 1)
    {
        ERR("Invalid layer count for volume texture.\n");
        return E_INVALIDARG;
    }

    texture->sub_resources = sub_resources;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n");
        return WINED3DERR_INVALIDCALL;
    }
    format = wined3d_get_format(device->adapter, desc->format, desc->bind_flags);

    if ((desc->usage & WINED3DUSAGE_DYNAMIC) && (desc->usage & (WINED3DUSAGE_MANAGED | WINED3DUSAGE_SCRATCH)))
    {
        WARN("Attempted to create a dynamic texture with usage %s.\n", debug_d3dusage(desc->usage));
        return WINED3DERR_INVALIDCALL;
    }

    if (((desc->width & (desc->width - 1)) || (desc->height & (desc->height - 1)) || (desc->depth & (desc->depth - 1)))
            && !d3d_info->unconditional_npot)
    {
        /* level_count == 0 returns an error as well. */
        if (level_count != 1 || layer_count != 1 || desc->resource_type == WINED3D_RTYPE_TEXTURE_3D)
        {
            if (!(desc->usage & WINED3DUSAGE_SCRATCH))
            {
                WARN("Attempted to create a mipmapped/cube/array/volume NPOT "
                        "texture without unconditional NPOT support.\n");
                return WINED3DERR_INVALIDCALL;
            }

            WARN("Creating a scratch mipmapped/cube/array NPOT texture despite lack of HW support.\n");
        }
        texture->flags |= WINED3D_TEXTURE_COND_NP2;
    }

    if ((desc->width > d3d_info->limits.texture_size || desc->height > d3d_info->limits.texture_size)
            && (desc->bind_flags & WINED3D_BIND_SHADER_RESOURCE))
    {
        /* One of four options:
         * 1: Scale the texture. (Any texture ops would require the texture to
         *    be scaled which is potentially slow.)
         * 2: Set the texture to the maximum size (bad idea).
         * 3: WARN and return WINED3DERR_NOTAVAILABLE.
         * 4: Create the surface, but allow it to be used only for DirectDraw
         *    Blts. Some apps (e.g. Swat 3) create textures with a height of
         *    16 and a width > 3000 and blt 16x16 letter areas from them to
         *    the render target. */
        if (desc->access & WINED3D_RESOURCE_ACCESS_GPU)
        {
            WARN("Dimensions (%ux%u) exceed the maximum texture size.\n", desc->width, desc->height);
            return WINED3DERR_NOTAVAILABLE;
        }

        /* We should never use this surface in combination with OpenGL. */
        TRACE("Creating an oversized (%ux%u) surface.\n", desc->width, desc->height);
    }

    if ((format->attrs & WINED3D_FORMAT_ATTR_PLANAR) && ((desc->width & 1) || (desc->height & 1)))
    {
        WARN("Attempt to create a planar texture with unaligned size %ux%u.\n", desc->width, desc->height);
        return WINED3DERR_INVALIDCALL;
    }

    for (i = 0; i < layer_count; ++i)
    {
        for (j = 0; j < level_count; ++j)
        {
            unsigned int idx = i * level_count + j;

            size = wined3d_format_calculate_size(format, device->surface_alignment,
                    max(1, desc->width >> j), max(1, desc->height >> j), max(1, desc->depth >> j));
            texture->sub_resources[idx].offset = offset;
            texture->sub_resources[idx].size = size;
            offset += size;
        }
        offset = (offset + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1);
    }

    if (!offset)
        return WINED3DERR_INVALIDCALL;

    /* Ensure the last mip-level is at least large enough to hold a single
     * compressed block. It is questionable how useful these mip-levels are to
     * the application with "broken pitch" formats, but we want to avoid
     * memory corruption when loading textures into WINED3D_LOCATION_SYSMEM. */
    if (format->attrs & WINED3D_FORMAT_ATTR_BROKEN_PITCH)
    {
        unsigned int min_size;

        min_size = texture->sub_resources[level_count * layer_count - 1].offset + format->block_byte_count;
        min_size = (min_size + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1);
        if (min_size > offset)
            offset = min_size;
    }

    if (FAILED(hr = resource_init(&texture->resource, device, desc->resource_type, format,
            desc->multisample_type, desc->multisample_quality, desc->usage, desc->bind_flags, desc->access,
            desc->width, desc->height, desc->depth, offset, parent, parent_ops, &texture_resource_ops)))
    {
        static unsigned int once;

        /* DXTn 3D textures are not supported. Do not write the ERR for them. */
        if ((desc->format == WINED3DFMT_DXT1 || desc->format == WINED3DFMT_DXT2 || desc->format == WINED3DFMT_DXT3
                || desc->format == WINED3DFMT_DXT4 || desc->format == WINED3DFMT_DXT5)
                && !(format->caps[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3D_FORMAT_CAP_TEXTURE)
                && desc->resource_type != WINED3D_RTYPE_TEXTURE_3D && !once++)
            ERR_(winediag)("The application tried to create a DXTn texture, but the driver does not support them.\n");

        WARN("Failed to initialize resource, returning %#lx\n", hr);
        return hr;
    }
    wined3d_resource_update_draw_binding(&texture->resource);

    texture->texture_ops = texture_ops;

    texture->layer_count = layer_count;
    texture->level_count = level_count;
    texture->lod = 0;
    texture->flags |= WINED3D_TEXTURE_DOWNLOADABLE;
    if (flags & WINED3D_TEXTURE_CREATE_GET_DC_LENIENT)
    {
        texture->flags |= WINED3D_TEXTURE_GET_DC_LENIENT;
        texture->resource.pin_sysmem = 1;
    }
    if (flags & (WINED3D_TEXTURE_CREATE_GET_DC | WINED3D_TEXTURE_CREATE_GET_DC_LENIENT))
        texture->flags |= WINED3D_TEXTURE_GET_DC;
    if (flags & WINED3D_TEXTURE_CREATE_DISCARD)
        texture->flags |= WINED3D_TEXTURE_DISCARD;
    if (flags & WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS)
    {
        if (!(texture->resource.format_caps & WINED3D_FORMAT_CAP_GEN_MIPMAP))
            WARN("Format doesn't support mipmaps generation, "
                    "ignoring WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS flag.\n");
        else
            texture->flags |= WINED3D_TEXTURE_GENERATE_MIPMAPS;
    }

    if (flags & WINED3D_TEXTURE_CREATE_RECORD_DIRTY_REGIONS)
    {
        if (!(texture->dirty_regions = calloc(texture->layer_count, sizeof(*texture->dirty_regions))))
        {
            wined3d_texture_cleanup_sync(texture);
            return E_OUTOFMEMORY;
        }
        for (i = 0; i < texture->layer_count; ++i)
            wined3d_texture_dirty_region_add(texture, i, NULL);
    }

    if (wined3d_texture_use_pbo(texture, d3d_info))
        texture->resource.map_binding = WINED3D_LOCATION_BUFFER;

    sub_count = level_count * layer_count;
    if (sub_count / layer_count != level_count)
    {
        wined3d_texture_cleanup_sync(texture);
        return E_OUTOFMEMORY;
    }

    if (desc->usage & WINED3DUSAGE_OVERLAY)
    {
        if (!(texture->overlay_info = calloc(sub_count, sizeof(*texture->overlay_info))))
        {
            wined3d_texture_cleanup_sync(texture);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < sub_count; ++i)
        {
            list_init(&texture->overlay_info[i].entry);
            list_init(&texture->overlay_info[i].overlays);
        }
    }

    /* Generate all sub-resources. */
    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_texture_sub_resource *sub_resource;

        sub_resource = &texture->sub_resources[i];
        sub_resource->locations = WINED3D_LOCATION_CLEARED;

        if (FAILED(hr = device_parent->ops->texture_sub_resource_created(device_parent,
                desc->resource_type, texture, i, &sub_resource->parent, &sub_resource->parent_ops)))
        {
            WARN("Failed to create sub-resource parent, hr %#lx.\n", hr);
            sub_resource->parent = NULL;
            wined3d_texture_cleanup_sync(texture);
            return hr;
        }

        TRACE("parent %p, parent_ops %p.\n", sub_resource->parent, sub_resource->parent_ops);

        TRACE("Created sub-resource %u (level %u, layer %u).\n",
                i, i % texture->level_count, i / texture->level_count);

        if (desc->usage & WINED3DUSAGE_OWNDC)
        {
            struct wined3d_texture_idx texture_idx = {texture, i};

            wined3d_cs_init_object(device->cs, wined3d_texture_create_dc, &texture_idx);
            wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
            if (!texture->dc_info || !texture->dc_info[i].dc)
            {
                wined3d_texture_cleanup_sync(texture);
                return WINED3DERR_INVALIDCALL;
            }
        }
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_context_blt(struct wined3d_device_context *context,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, const RECT *dst_rect,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, const RECT *src_rect,
        unsigned int flags, const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_box src_box = {src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1};
    struct wined3d_box dst_box = {dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, 0, 1};
    HRESULT hr;

    TRACE("context %p, dst_texture %p, dst_sub_resource_idx %u, dst_rect %s, src_texture %p, "
            "src_sub_resource_idx %u, src_rect %s, flags %#x, fx %p, filter %s.\n",
            context, dst_texture, dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), src_texture,
            src_sub_resource_idx, wine_dbgstr_rect(src_rect), flags, fx, debug_d3dtexturefiltertype(filter));

    if (!wined3d_texture_validate_sub_resource_idx(dst_texture, dst_sub_resource_idx)
            || dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
        return WINED3DERR_INVALIDCALL;

    if (!wined3d_texture_validate_sub_resource_idx(src_texture, src_sub_resource_idx)
            || src_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
        return WINED3DERR_INVALIDCALL;

    if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT
            && filter != WINED3D_TEXF_LINEAR)
        return WINED3DERR_INVALIDCALL;

    if (FAILED(hr = wined3d_resource_check_box_dimensions(&dst_texture->resource, dst_sub_resource_idx, &dst_box)))
        return hr;

    if (FAILED(hr = wined3d_resource_check_box_dimensions(&src_texture->resource, src_sub_resource_idx, &src_box)))
        return hr;

    if (dst_texture->sub_resources[dst_sub_resource_idx].map_count
            || src_texture->sub_resources[src_sub_resource_idx].map_count)
    {
        WARN("Sub-resource is busy, returning WINEDDERR_SURFACEBUSY.\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if (!src_texture->resource.format->depth_size != !dst_texture->resource.format->depth_size
            || !src_texture->resource.format->stencil_size != !dst_texture->resource.format->stencil_size)
    {
        WARN("Rejecting depth/stencil blit between incompatible formats.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_texture->resource.device != src_texture->resource.device)
    {
        FIXME("Rejecting cross-device blit.\n");
        return E_NOTIMPL;
    }

    wined3d_device_context_emit_blt_sub_resource(context, &dst_texture->resource, dst_sub_resource_idx,
            &dst_box, &src_texture->resource, src_sub_resource_idx, &src_box, flags, fx, filter);

    if (dst_texture->dirty_regions)
        wined3d_texture_add_dirty_region(dst_texture, dst_sub_resource_idx, &dst_box);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_get_overlay_position(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG *x, LONG *y)
{
    struct wined3d_overlay_info *overlay;

    TRACE("texture %p, sub_resource_idx %u, x %p, y %p.\n", texture, sub_resource_idx, x, y);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY)
            || !wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx))
        return WINEDDERR_NOTAOVERLAYSURFACE;

    overlay = &texture->overlay_info[sub_resource_idx];
    if (!overlay->dst_texture)
    {
        TRACE("Overlay not visible.\n");
        *x = 0;
        *y = 0;
        return WINEDDERR_OVERLAYNOTVISIBLE;
    }

    *x = overlay->dst_rect.left;
    *y = overlay->dst_rect.top;

    TRACE("Returning position %ld, %ld.\n", *x, *y);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_set_overlay_position(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG x, LONG y)
{
    struct wined3d_overlay_info *overlay;
    LONG w, h;

    TRACE("texture %p, sub_resource_idx %u, x %ld, y %ld.\n", texture, sub_resource_idx, x, y);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY)
            || !wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx))
        return WINEDDERR_NOTAOVERLAYSURFACE;

    overlay = &texture->overlay_info[sub_resource_idx];
    w = overlay->dst_rect.right - overlay->dst_rect.left;
    h = overlay->dst_rect.bottom - overlay->dst_rect.top;
    SetRect(&overlay->dst_rect, x, y, x + w, y + h);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_update_overlay(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const RECT *src_rect, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect, uint32_t flags)
{
    struct wined3d_overlay_info *overlay;
    unsigned int level, dst_level;

    TRACE("texture %p, sub_resource_idx %u, src_rect %s, dst_texture %p, "
            "dst_sub_resource_idx %u, dst_rect %s, flags %#x.\n",
            texture, sub_resource_idx, wine_dbgstr_rect(src_rect), dst_texture,
            dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), flags);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY) || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx))
        return WINEDDERR_NOTAOVERLAYSURFACE;

    if (!dst_texture || dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !wined3d_texture_validate_sub_resource_idx(dst_texture, dst_sub_resource_idx))
        return WINED3DERR_INVALIDCALL;

    overlay = &texture->overlay_info[sub_resource_idx];

    level = sub_resource_idx % texture->level_count;
    if (src_rect)
        overlay->src_rect = *src_rect;
    else
        SetRect(&overlay->src_rect, 0, 0,
                wined3d_texture_get_level_width(texture, level),
                wined3d_texture_get_level_height(texture, level));

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    if (dst_rect)
        overlay->dst_rect = *dst_rect;
    else
        SetRect(&overlay->dst_rect, 0, 0,
                wined3d_texture_get_level_width(dst_texture, dst_level),
                wined3d_texture_get_level_height(dst_texture, dst_level));

    if (overlay->dst_texture && (overlay->dst_texture != dst_texture
            || overlay->dst_sub_resource_idx != dst_sub_resource_idx || flags & WINEDDOVER_HIDE))
    {
        overlay->dst_texture = NULL;
        list_remove(&overlay->entry);
    }

    if (flags & WINEDDOVER_SHOW)
    {
        if (overlay->dst_texture != dst_texture || overlay->dst_sub_resource_idx != dst_sub_resource_idx)
        {
            overlay->dst_texture = dst_texture;
            overlay->dst_sub_resource_idx = dst_sub_resource_idx;
            list_add_tail(&texture->overlay_info[dst_sub_resource_idx].overlays, &overlay->entry);
        }
    }
    else if (flags & WINEDDOVER_HIDE)
    {
        /* Tests show that the rectangles are erased on hide. */
        SetRectEmpty(&overlay->src_rect);
        SetRectEmpty(&overlay->dst_rect);
        overlay->dst_texture = NULL;
    }

    return WINED3D_OK;
}

struct wined3d_swapchain * CDECL wined3d_texture_get_swapchain(struct wined3d_texture *texture)
{
    return texture->swapchain;
}

void * CDECL wined3d_texture_get_sub_resource_parent(struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    TRACE("texture %p, sub_resource_idx %u.\n", texture, sub_resource_idx);

    if (!wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx))
        return NULL;

    return texture->sub_resources[sub_resource_idx].parent;
}

void CDECL wined3d_texture_set_sub_resource_parent(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("texture %p, sub_resource_idx %u, parent %p.\n", texture, sub_resource_idx, parent);

    if (!wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx))
        return;

    texture->sub_resources[sub_resource_idx].parent = parent;
    texture->sub_resources[sub_resource_idx].parent_ops = parent_ops;
}

HRESULT CDECL wined3d_texture_get_sub_resource_desc(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc)
{
    const struct wined3d_resource *resource;
    unsigned int level_idx;

    TRACE("texture %p, sub_resource_idx %u, desc %p.\n", texture, sub_resource_idx, desc);

    if (!wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx))
        return WINED3DERR_INVALIDCALL;

    resource = &texture->resource;
    desc->format = resource->format->id;
    desc->multisample_type = resource->multisample_type;
    desc->multisample_quality = resource->multisample_quality;
    desc->usage = resource->usage;
    desc->bind_flags = resource->bind_flags;
    desc->access = resource->access;

    level_idx = sub_resource_idx % texture->level_count;
    desc->width = wined3d_texture_get_level_width(texture, level_idx);
    desc->height = wined3d_texture_get_level_height(texture, level_idx);
    desc->depth = wined3d_texture_get_level_depth(texture, level_idx);
    desc->size = texture->sub_resources[sub_resource_idx].size;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_create(struct wined3d_device *device, const struct wined3d_resource_desc *desc,
        UINT layer_count, UINT level_count, uint32_t flags, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture)
{
    unsigned int sub_count = level_count * layer_count;
    unsigned int i;
    HRESULT hr;

    TRACE("device %p, desc %p, layer_count %u, level_count %u, flags %#x, data %p, "
            "parent %p, parent_ops %p, texture %p.\n",
            device, desc, layer_count, level_count, flags, data, parent, parent_ops, texture);

    if (!layer_count)
    {
        WARN("Invalid layer count.\n");
        return E_INVALIDARG;
    }
    if ((desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP) && layer_count != 6)
    {
        ERR("Invalid layer count %u for legacy cubemap.\n", layer_count);
        layer_count = 6;
    }

    if (!level_count)
    {
        WARN("Invalid level count.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->multisample_type != WINED3D_MULTISAMPLE_NONE)
    {
        const struct wined3d_format *format = wined3d_get_format(device->adapter, desc->format, desc->bind_flags);

        if (desc->multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE
                && desc->multisample_quality >= wined3d_popcount(format->multisample_types))
        {
            WARN("Unsupported quality level %u requested for WINED3D_MULTISAMPLE_NON_MASKABLE.\n",
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
        if (desc->multisample_type != WINED3D_MULTISAMPLE_NON_MASKABLE
                && (!(format->multisample_types & 1u << (desc->multisample_type - 1))
                || (desc->multisample_quality && desc->multisample_quality != WINED3D_STANDARD_MULTISAMPLE_PATTERN)))
        {
            WARN("Unsupported multisample type %u quality %u requested.\n", desc->multisample_type,
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    if (data)
    {
        for (i = 0; i < sub_count; ++i)
        {
            if (data[i].data)
                continue;

            WARN("Invalid sub-resource data specified for sub-resource %u.\n", i);
            return E_INVALIDARG;
        }
    }

    if (FAILED(hr = device->adapter->adapter_ops->adapter_create_texture(device, desc,
            layer_count, level_count, flags, parent, parent_ops, texture)))
        return hr;

    /* FIXME: We'd like to avoid ever allocating system memory for the texture
     * in this case. */
    if (data)
    {
        struct wined3d_box box;

        for (i = 0; i < sub_count; ++i)
        {
            wined3d_texture_get_level_box(*texture, i % (*texture)->level_count, &box);
            wined3d_device_context_emit_update_sub_resource(&device->cs->c, &(*texture)->resource,
                    i, &box, data[i].data, data[i].row_pitch, data[i].slice_pitch);
        }
    }

    TRACE("Created texture %p.\n", *texture);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_get_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC *dc)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_dc_info *dc_info;

    TRACE("texture %p, sub_resource_idx %u, dc %p.\n", texture, sub_resource_idx, dc);

    if (!(texture->flags & WINED3D_TEXTURE_GET_DC))
    {
        WARN("Texture does not support GetDC\n");
        /* Don't touch the DC */
        return WINED3DERR_INVALIDCALL;
    }

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Not supported on %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->resource.map_count && !(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        return WINED3DERR_INVALIDCALL;

    if (!(dc_info = texture->dc_info) || !dc_info[sub_resource_idx].dc)
    {
        struct wined3d_texture_idx texture_idx = {texture, sub_resource_idx};

        wined3d_cs_init_object(device->cs, wined3d_texture_create_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
        if (!(dc_info = texture->dc_info) || !dc_info[sub_resource_idx].dc)
            return WINED3DERR_INVALIDCALL;
    }

    if (!(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        texture->flags |= WINED3D_TEXTURE_DC_IN_USE;
    ++texture->resource.map_count;
    ++sub_resource->map_count;

    *dc = dc_info[sub_resource_idx].dc;
    TRACE("Returning dc %p.\n", *dc);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_release_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC dc)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_dc_info *dc_info;

    TRACE("texture %p, sub_resource_idx %u, dc %p.\n", texture, sub_resource_idx, dc);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Not supported on %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    if (!(texture->flags & (WINED3D_TEXTURE_GET_DC_LENIENT | WINED3D_TEXTURE_DC_IN_USE)))
        return WINED3DERR_INVALIDCALL;

    if (!(dc_info = texture->dc_info) || dc_info[sub_resource_idx].dc != dc)
    {
        WARN("Application tries to release invalid DC %p, sub-resource DC is %p.\n",
                dc, dc_info ? dc_info[sub_resource_idx].dc : NULL);
        return WINED3DERR_INVALIDCALL;
    }

    if (!(texture->resource.usage & WINED3DUSAGE_OWNDC))
    {
        struct wined3d_texture_idx texture_idx = {texture, sub_resource_idx};

        wined3d_cs_destroy_object(device->cs, wined3d_texture_destroy_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    }

    --sub_resource->map_count;
    if (!--texture->resource.map_count && texture->update_map_binding)
        wined3d_texture_update_map_binding(texture);
    if (!(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        texture->flags &= ~WINED3D_TEXTURE_DC_IN_USE;

    return WINED3D_OK;
}

void wined3d_texture_upload_from_texture(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z, struct wined3d_texture *src_texture,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box)
{
    unsigned int src_row_pitch, src_slice_pitch;
    unsigned int update_w, update_h, update_d;
    unsigned int src_level, dst_level;
    struct wined3d_context *context;
    struct wined3d_bo_address data;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_texture %p, src_sub_resource_idx %u, src_box %s.\n",
            dst_texture, dst_sub_resource_idx, dst_x, dst_y, dst_z,
            src_texture, src_sub_resource_idx, debug_box(src_box));

    context = context_acquire(dst_texture->resource.device, NULL, 0);

    /* Only load the sub-resource for partial updates. For newly allocated
     * textures the texture wouldn't be the current location, and we'd upload
     * zeroes just to overwrite them again. */
    update_w = src_box->right - src_box->left;
    update_h = src_box->bottom - src_box->top;
    update_d = src_box->back - src_box->front;
    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    if (update_w == wined3d_texture_get_level_width(dst_texture, dst_level)
            && update_h == wined3d_texture_get_level_height(dst_texture, dst_level)
            && update_d == wined3d_texture_get_level_depth(dst_texture, dst_level))
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    else
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);

    src_level = src_sub_resource_idx % src_texture->level_count;
    wined3d_texture_get_memory(src_texture, src_sub_resource_idx, context, &data);
    wined3d_texture_get_pitch(src_texture, src_level, &src_row_pitch, &src_slice_pitch);

    dst_texture->texture_ops->texture_upload_data(context, wined3d_const_bo_address(&data),
            src_texture->resource.format, src_box, src_row_pitch, src_slice_pitch, dst_texture,
            dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, dst_x, dst_y, dst_z);

    context_release(context);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
}

/* Partial downloads are not supported. */
void wined3d_texture_download_from_texture(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx)
{
    unsigned int src_level, dst_level, dst_row_pitch, dst_slice_pitch;
    unsigned int dst_location = dst_texture->resource.map_binding;
    struct wined3d_context *context;
    struct wined3d_bo_address data;
    struct wined3d_box src_box;
    unsigned int src_location;

    context = context_acquire(src_texture->resource.device, NULL, 0);

    wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, dst_location);
    wined3d_texture_get_bo_address(dst_texture, dst_sub_resource_idx, &data, dst_location);

    if (src_texture->sub_resources[src_sub_resource_idx].locations & WINED3D_LOCATION_TEXTURE_RGB)
        src_location = WINED3D_LOCATION_TEXTURE_RGB;
    else
        src_location = WINED3D_LOCATION_TEXTURE_SRGB;
    src_level = src_sub_resource_idx % src_texture->level_count;
    wined3d_texture_get_level_box(src_texture, src_level, &src_box);

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    wined3d_texture_get_pitch(dst_texture, dst_level, &dst_row_pitch, &dst_slice_pitch);

    src_texture->texture_ops->texture_download_data(context, src_texture, src_sub_resource_idx, src_location,
            &src_box, &data, dst_texture->resource.format, 0, 0, 0, dst_row_pitch, dst_slice_pitch);

    context_release(context);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, dst_location);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~dst_location);
}

static void wined3d_texture_set_bo(struct wined3d_texture *texture,
        unsigned sub_resource_idx, struct wined3d_context *context, struct wined3d_bo *bo)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    struct wined3d_bo *prev_bo = sub_resource->bo;

    TRACE("texture %p, sub_resource_idx %u, context %p, bo %p.\n", texture, sub_resource_idx, context, bo);

    if (prev_bo)
    {
        struct wined3d_bo_user *bo_user;

        LIST_FOR_EACH_ENTRY(bo_user, &prev_bo->users, struct wined3d_bo_user, entry)
            bo_user->valid = false;
        list_init(&prev_bo->users);

        assert(list_empty(&bo->users));

        wined3d_context_destroy_bo(context, prev_bo);
        free(prev_bo);
    }

    sub_resource->bo = bo;
}

void wined3d_texture_update_sub_resource(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, const struct upload_bo *upload_bo, const struct wined3d_box *box,
        unsigned int row_pitch, unsigned int slice_pitch)
{
    unsigned int level = sub_resource_idx % texture->level_count;
    unsigned int width = wined3d_texture_get_level_width(texture, level);
    unsigned int height = wined3d_texture_get_level_height(texture, level);
    unsigned int depth = wined3d_texture_get_level_depth(texture, level);
    struct wined3d_box src_box;

    if (upload_bo->flags & UPLOAD_BO_RENAME_ON_UNMAP)
    {
        wined3d_texture_set_bo(texture, sub_resource_idx, context, upload_bo->addr.buffer_object);
        wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_BUFFER);
        wined3d_texture_invalidate_location(texture, sub_resource_idx, ~WINED3D_LOCATION_BUFFER);
        /* Try to free address space if we are not mapping persistently. */
        if (upload_bo->addr.buffer_object->map_ptr)
            wined3d_context_unmap_bo_address(context, (const struct wined3d_bo_address *)&upload_bo->addr, 0, NULL);
    }

    /* Only load the sub-resource for partial updates. */
    if (!box->left && !box->top && !box->front
            && box->right == width && box->bottom == height && box->back == depth)
        wined3d_texture_prepare_location(texture, sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    else
        wined3d_texture_load_location(texture, sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);

    wined3d_box_set(&src_box, 0, 0, box->right - box->left, box->bottom - box->top, 0, box->back - box->front);
    texture->texture_ops->texture_upload_data(context, &upload_bo->addr, texture->resource.format,
            &src_box, row_pitch, slice_pitch, texture, sub_resource_idx,
            WINED3D_LOCATION_TEXTURE_RGB, box->left, box->top, box->front);

    wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
}

struct wined3d_shader_resource_view * CDECL wined3d_texture_acquire_identity_srv(struct wined3d_texture *texture)
{
    struct wined3d_view_desc desc;
    HRESULT hr;

    TRACE("texture %p.\n", texture);

    if (texture->identity_srv)
        return texture->identity_srv;

    desc.format_id = texture->resource.format->id;
    /* The texture owns a reference to the SRV, so we can't have the SRV hold
     * a reference to the texture.
     * At the same time, a view must be destroyed before its texture, and we
     * need a bound SRV to keep the texture alive even if it doesn't have any
     * other references.
     * In order to achieve this we have the objects share reference counts.
     * This means the view doesn't hold a reference to the resource, but any
     * references to the view are forwarded to the resource instead. The view
     * is destroyed manually when all references are released. */
    desc.flags = WINED3D_VIEW_FORWARD_REFERENCE;
    desc.u.texture.level_idx = 0;
    desc.u.texture.level_count = texture->level_count;
    desc.u.texture.layer_idx = 0;
    desc.u.texture.layer_count = texture->layer_count;
    if (FAILED(hr = wined3d_shader_resource_view_create(&desc, &texture->resource,
            NULL, &wined3d_null_parent_ops, &texture->identity_srv)))
    {
        ERR("Failed to create shader resource view, hr %#lx.\n", hr);
        return NULL;
    }
    wined3d_shader_resource_view_decref(texture->identity_srv);

    return texture->identity_srv;
}

static void wined3d_texture_no3d_upload_data(struct wined3d_context *context,
        const struct wined3d_const_bo_address *src_bo_addr, const struct wined3d_format *src_format,
        const struct wined3d_box *src_box, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, unsigned int dst_location,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    FIXME("Not implemented.\n");
}

static void wined3d_texture_no3d_download_data(struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, unsigned int src_location,
        const struct wined3d_box *src_box, const struct wined3d_bo_address *dst_bo_addr,
        const struct wined3d_format *dst_format, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch)
{
    FIXME("Not implemented.\n");
}

static BOOL wined3d_texture_no3d_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    if (location == WINED3D_LOCATION_SYSMEM)
        return texture->sub_resources[sub_resource_idx].user_memory ? TRUE
                : wined3d_resource_prepare_sysmem(&texture->resource);

    FIXME("Unhandled location %s.\n", wined3d_debug_location(location));
    return FALSE;
}

static BOOL wined3d_texture_no3d_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, uint32_t location)
{
    TRACE("texture %p, sub_resource_idx %u, context %p, location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(location));

    if (location == WINED3D_LOCATION_SYSMEM)
        return TRUE;

    ERR("Unhandled location %s.\n", wined3d_debug_location(location));

    return FALSE;
}

static void wined3d_texture_no3d_unload_location(struct wined3d_texture *texture,
        struct wined3d_context *context, unsigned int location)
{
    TRACE("texture %p, context %p, location %s.\n", texture, context, wined3d_debug_location(location));
}

static const struct wined3d_texture_ops wined3d_texture_no3d_ops =
{
    wined3d_texture_no3d_prepare_location,
    wined3d_texture_no3d_load_location,
    wined3d_texture_no3d_unload_location,
    wined3d_texture_no3d_upload_data,
    wined3d_texture_no3d_download_data,
};

HRESULT wined3d_texture_no3d_init(struct wined3d_texture *texture_no3d, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("texture_no3d %p, device %p, desc %p, layer_count %u, "
            "level_count %u, flags %#x, parent %p, parent_ops %p.\n",
            texture_no3d, device, desc, layer_count,
            level_count, flags, parent, parent_ops);

    return wined3d_texture_init(texture_no3d, desc, layer_count, level_count,
            flags, device, parent, parent_ops, &texture_no3d[1], &wined3d_texture_no3d_ops);
}

bool wined3d_rendertarget_view_use_cpu_clear(struct wined3d_rendertarget_view *view)
{
    struct wined3d_resource *resource;
    struct wined3d_texture *texture;
    DWORD locations;

    resource = view->resource;
    if (resource->type == WINED3D_RTYPE_BUFFER)
        return !(resource->access & WINED3D_RESOURCE_ACCESS_GPU);

    texture = texture_from_resource(resource);
    locations = texture->sub_resources[view->sub_resource_idx].locations;
    if (locations & (resource->map_binding | WINED3D_LOCATION_DISCARDED))
        return !(resource->access & WINED3D_RESOURCE_ACCESS_GPU)
                || texture->resource.pin_sysmem;

    return !(resource->access & WINED3D_RESOURCE_ACCESS_GPU)
            && !(texture->flags & WINED3D_TEXTURE_CONVERTED);
}
