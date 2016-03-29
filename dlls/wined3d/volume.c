/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
 * Copyright 2013 Stefan Dösinger for CodeWeavers
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

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

static BOOL volume_prepare_system_memory(struct wined3d_volume *volume)
{
    if (volume->resource.heap_memory)
        return TRUE;

    if (!wined3d_resource_allocate_sysmem(&volume->resource))
    {
        ERR("Failed to allocate system memory.\n");
        return FALSE;
    }
    return TRUE;
}

/* Context activation is done by the caller. Context may be NULL in
 * WINED3D_NO3D mode. */
BOOL wined3d_volume_prepare_location(struct wined3d_volume *volume,
        struct wined3d_context *context, DWORD location)
{
    struct wined3d_texture *texture = volume->container;

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return volume_prepare_system_memory(volume);

        case WINED3D_LOCATION_BUFFER:
            wined3d_texture_prepare_buffer_object(texture, volume->texture_level, context->gl_info);
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_RGB:
            wined3d_texture_prepare_texture(texture, context, FALSE);
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_SRGB:
            wined3d_texture_prepare_texture(texture, context, TRUE);
            return TRUE;

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

/* This call just uploads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
void wined3d_volume_upload_data(struct wined3d_volume *volume, const struct wined3d_context *context,
        const struct wined3d_const_bo_address *data)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_texture *texture = volume->container;
    const struct wined3d_format *format = texture->resource.format;
    UINT width = volume->resource.width;
    UINT height = volume->resource.height;
    UINT depth = volume->resource.depth;
    const void *mem = data->addr;
    void *converted_mem = NULL;

    TRACE("volume %p, context %p, level %u, format %s (%#x).\n",
            volume, context, volume->texture_level, debug_d3dformat(format->id),
            format->id);

    if (format->convert)
    {
        UINT dst_row_pitch, dst_slice_pitch;
        UINT src_row_pitch, src_slice_pitch;

        if (data->buffer_object)
            ERR("Loading a converted volume from a PBO.\n");
        if (texture->resource.format_flags & WINED3DFMT_FLAG_BLOCKS)
            ERR("Converting a block-based format.\n");

        dst_row_pitch = width * format->conv_byte_count;
        dst_slice_pitch = dst_row_pitch * height;

        wined3d_texture_get_pitch(texture, volume->texture_level, &src_row_pitch, &src_slice_pitch);

        converted_mem = HeapAlloc(GetProcessHeap(), 0, dst_slice_pitch * depth);
        format->convert(data->addr, converted_mem, src_row_pitch, src_slice_pitch,
                dst_row_pitch, dst_slice_pitch, width, height, depth);
        mem = converted_mem;
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    GL_EXTCALL(glTexSubImage3D(GL_TEXTURE_3D, volume->texture_level, 0, 0, 0,
            width, height, depth,
            format->glFormat, format->glType, mem));
    checkGLcall("glTexSubImage3D");

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    HeapFree(GetProcessHeap(), 0, converted_mem);
}

void wined3d_volume_invalidate_location(struct wined3d_volume *volume, DWORD location)
{
    struct wined3d_texture_sub_resource *sub_resource;

    TRACE("Volume %p, clearing %s.\n", volume, wined3d_debug_location(location));

    sub_resource = &volume->container->sub_resources[volume->texture_level];
    if (location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
        wined3d_texture_set_dirty(volume->container);
    sub_resource->locations &= ~location;

    TRACE("new location flags are %s.\n", wined3d_debug_location(sub_resource->locations));
}

/* Context activation is done by the caller. */
static void wined3d_volume_download_data(struct wined3d_volume *volume,
        const struct wined3d_context *context, const struct wined3d_bo_address *data)
{
    const struct wined3d_format *format = volume->container->resource.format;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (format->convert)
    {
        FIXME("Attempting to download a converted volume, format %s.\n",
                debug_d3dformat(format->id));
        return;
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_3D, volume->texture_level,
            format->glFormat, format->glType, data->addr);
    checkGLcall("glGetTexImage");

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

}

static void wined3d_volume_evict_sysmem(struct wined3d_volume *volume)
{
    wined3d_resource_free_sysmem(&volume->resource);
    wined3d_volume_invalidate_location(volume, WINED3D_LOCATION_SYSMEM);
}

static DWORD volume_access_from_location(DWORD location)
{
    switch (location)
    {
        case WINED3D_LOCATION_DISCARDED:
            return 0;

        case WINED3D_LOCATION_SYSMEM:
            return WINED3D_RESOURCE_ACCESS_CPU;

        case WINED3D_LOCATION_BUFFER:
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            return WINED3D_RESOURCE_ACCESS_GPU;

        default:
            FIXME("Unhandled location %#x.\n", location);
            return 0;
    }
}

/* Context activation is done by the caller. */
static void wined3d_volume_srgb_transfer(struct wined3d_volume *volume,
        struct wined3d_context *context, BOOL dest_is_srgb)
{
    struct wined3d_bo_address data;
    /* Optimizations are possible, but the effort should be put into either
     * implementing EXT_SRGB_DECODE in the driver or finding out why we
     * picked the wrong copy for the original upload and fixing that.
     *
     * Also keep in mind that we want to avoid using resource.heap_memory
     * for DEFAULT pool surfaces. */

    WARN_(d3d_perf)("Performing slow rgb/srgb volume transfer.\n");
    data.buffer_object = 0;
    data.addr = HeapAlloc(GetProcessHeap(), 0, volume->resource.size);
    if (!data.addr)
        return;

    wined3d_texture_bind_and_dirtify(volume->container, context, !dest_is_srgb);
    wined3d_volume_download_data(volume, context, &data);
    wined3d_texture_bind_and_dirtify(volume->container, context, dest_is_srgb);
    wined3d_volume_upload_data(volume, context, wined3d_const_bo_address(&data));

    HeapFree(GetProcessHeap(), 0, data.addr);
}

static BOOL wined3d_volume_can_evict(const struct wined3d_volume *volume)
{
    struct wined3d_texture *texture = volume->container;

    if (texture->resource.pool != WINED3D_POOL_MANAGED)
        return FALSE;
    if (texture->download_count >= 10)
        return FALSE;
    if (texture->resource.format->convert)
        return FALSE;

    return TRUE;
}

/* Context activation is done by the caller. */
BOOL wined3d_volume_load_location(struct wined3d_volume *volume,
        struct wined3d_context *context, DWORD location)
{
    DWORD required_access = volume_access_from_location(location);
    unsigned int sub_resource_idx = volume->texture_level;
    struct wined3d_texture *texture = volume->container;
    struct wined3d_texture_sub_resource *sub_resource;

    sub_resource = &texture->sub_resources[sub_resource_idx];
    TRACE("Volume %p, loading %s, have %s.\n", volume, wined3d_debug_location(location),
        wined3d_debug_location(sub_resource->locations));

    if ((sub_resource->locations & location) == location)
    {
        TRACE("Location(s) already up to date.\n");
        return TRUE;
    }

    if ((texture->resource.access_flags & required_access) != required_access)
    {
        ERR("Operation requires %#x access, but volume only has %#x.\n",
                required_access, texture->resource.access_flags);
        return FALSE;
    }

    if (!wined3d_volume_prepare_location(volume, context, location))
        return FALSE;

    if (sub_resource->locations & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("Volume previously discarded, nothing to do.\n");
        wined3d_volume_invalidate_location(volume, WINED3D_LOCATION_DISCARDED);
        goto done;
    }

    switch (location)
    {
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            if (sub_resource->locations & WINED3D_LOCATION_SYSMEM)
            {
                struct wined3d_const_bo_address data = {0, volume->resource.heap_memory};
                wined3d_texture_bind_and_dirtify(texture, context,
                        location == WINED3D_LOCATION_TEXTURE_SRGB);
                wined3d_volume_upload_data(volume, context, &data);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_BUFFER)
            {
                struct wined3d_const_bo_address data = {sub_resource->buffer_object, NULL};
                wined3d_texture_bind_and_dirtify(texture, context,
                        location == WINED3D_LOCATION_TEXTURE_SRGB);
                wined3d_volume_upload_data(volume, context, &data);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
            {
                wined3d_volume_srgb_transfer(volume, context, TRUE);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_SRGB)
            {
                wined3d_volume_srgb_transfer(volume, context, FALSE);
            }
            else
            {
                FIXME("Implement texture loading from %s.\n", wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        case WINED3D_LOCATION_SYSMEM:
            if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            {
                struct wined3d_bo_address data = {0, volume->resource.heap_memory};

                if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
                    wined3d_texture_bind_and_dirtify(texture, context, FALSE);
                else
                    wined3d_texture_bind_and_dirtify(texture, context, TRUE);

                wined3d_volume_download_data(volume, context, &data);
                ++texture->download_count;
            }
            else
            {
                FIXME("Implement WINED3D_LOCATION_SYSMEM loading from %s.\n",
                        wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        case WINED3D_LOCATION_BUFFER:
            if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            {
                struct wined3d_bo_address data = {sub_resource->buffer_object, NULL};

                if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
                    wined3d_texture_bind_and_dirtify(texture, context, FALSE);
                else
                    wined3d_texture_bind_and_dirtify(texture, context, TRUE);

                wined3d_volume_download_data(volume, context, &data);
            }
            else
            {
                FIXME("Implement WINED3D_LOCATION_BUFFER loading from %s.\n",
                        wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        default:
            FIXME("Implement %s loading from %s.\n", wined3d_debug_location(location),
                    wined3d_debug_location(sub_resource->locations));
            return FALSE;
    }

done:
    wined3d_texture_validate_location(texture, sub_resource_idx, location);

    if (location != WINED3D_LOCATION_SYSMEM && wined3d_volume_can_evict(volume))
        wined3d_volume_evict_sysmem(volume);

    return TRUE;
}

/* Context activation is done by the caller. */
void wined3d_volume_load(struct wined3d_volume *volume, struct wined3d_context *context, BOOL srgb_mode)
{
    wined3d_texture_prepare_texture(volume->container, context, srgb_mode);
    wined3d_volume_load_location(volume, context,
            srgb_mode ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB);
}

void wined3d_volume_cleanup(struct wined3d_volume *volume)
{
    TRACE("volume %p.\n", volume);

    resource_cleanup(&volume->resource);
}

static void volume_unload(struct wined3d_resource *resource)
{
    struct wined3d_volume *volume = volume_from_resource(resource);
    struct wined3d_texture *texture = volume->container;
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_context *context;

    if (texture->resource.pool == WINED3D_POOL_DEFAULT)
        ERR("Unloading DEFAULT pool volume.\n");

    TRACE("texture %p.\n", resource);

    context = context_acquire(device, NULL);
    if (wined3d_volume_load_location(volume, context, WINED3D_LOCATION_SYSMEM))
    {
        wined3d_volume_invalidate_location(volume, ~WINED3D_LOCATION_SYSMEM);
    }
    else
    {
        ERR("Out of memory when unloading volume %p.\n", volume);
        wined3d_texture_validate_location(texture, volume->texture_level, WINED3D_LOCATION_DISCARDED);
        wined3d_volume_invalidate_location(volume, ~WINED3D_LOCATION_DISCARDED);
    }
    context_release(context);

    /* The texture name is managed by the container. */

    resource_unload(resource);
}

static ULONG volume_resource_incref(struct wined3d_resource *resource)
{
    struct wined3d_volume *volume = volume_from_resource(resource);
    TRACE("Forwarding to container %p.\n", volume->container);

    return wined3d_texture_incref(volume->container);
}

static ULONG volume_resource_decref(struct wined3d_resource *resource)
{
    struct wined3d_volume *volume = volume_from_resource(resource);
    TRACE("Forwarding to container %p.\n", volume->container);

    return wined3d_texture_decref(volume->container);
}

static HRESULT volume_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, DWORD flags)
{
    ERR("Not supported on sub-resources.\n");
    return WINED3DERR_INVALIDCALL;
}

static HRESULT volume_resource_sub_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    ERR("Not supported on sub-resources.\n");
    return WINED3DERR_INVALIDCALL;
}

static const struct wined3d_resource_ops volume_resource_ops =
{
    volume_resource_incref,
    volume_resource_decref,
    volume_unload,
    volume_resource_sub_resource_map,
    volume_resource_sub_resource_unmap,
};

HRESULT wined3d_volume_init(struct wined3d_volume *volume, struct wined3d_texture *container,
        const struct wined3d_resource_desc *desc, UINT level)
{
    struct wined3d_device *device = container->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, desc->format);
    HRESULT hr;
    UINT size;

    /* TODO: Write tests for other resources and move this check
     * to resource_init, if applicable. */
    if (desc->usage & WINED3DUSAGE_DYNAMIC
            && (desc->pool == WINED3D_POOL_MANAGED || desc->pool == WINED3D_POOL_SCRATCH))
    {
        WARN("Attempted to create a DYNAMIC texture in pool %s.\n", debug_d3dpool(desc->pool));
        return WINED3DERR_INVALIDCALL;
    }

    size = wined3d_format_calculate_size(format, device->surface_alignment, desc->width, desc->height, desc->depth);

    if (FAILED(hr = resource_init(&volume->resource, device, WINED3D_RTYPE_VOLUME, format,
            WINED3D_MULTISAMPLE_NONE, 0, desc->usage, desc->pool, desc->width, desc->height, desc->depth,
            size, NULL, &wined3d_null_parent_ops, &volume_resource_ops)))
    {
        WARN("Failed to initialize resource, returning %#x.\n", hr);
        return hr;
    }

    volume->texture_level = level;
    container->sub_resources[level].locations = WINED3D_LOCATION_DISCARDED;

    if (wined3d_texture_use_pbo(container, gl_info))
    {
        wined3d_resource_free_sysmem(&volume->resource);
        volume->resource.map_binding = WINED3D_LOCATION_BUFFER;
    }

    volume->container = container;

    return WINED3D_OK;
}
