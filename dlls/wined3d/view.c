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

static BOOL is_stencil_view_format(const struct wined3d_format *format)
{
    return format->id == WINED3DFMT_X24_TYPELESS_G8_UINT
            || format->id == WINED3DFMT_X32_TYPELESS_G8X24_UINT;
}

static GLenum get_texture_view_target(const struct wined3d_view_desc *desc,
        const struct wined3d_texture *texture)
{
    static const struct
    {
        GLenum texture_target;
        unsigned int view_flags;
        GLenum view_target;
    }
    view_types[] =
    {
        {GL_TEXTURE_2D,       0,                          GL_TEXTURE_2D},
        {GL_TEXTURE_2D,       WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_2D_ARRAY},
        {GL_TEXTURE_2D_ARRAY, 0,                          GL_TEXTURE_2D},
        {GL_TEXTURE_2D_ARRAY, WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_2D_ARRAY},
        {GL_TEXTURE_2D_ARRAY, WINED3D_VIEW_TEXTURE_CUBE,  GL_TEXTURE_CUBE_MAP},
        {GL_TEXTURE_3D,       0,                          GL_TEXTURE_3D},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(view_types); ++i)
    {
        if (view_types[i].texture_target == texture->target && view_types[i].view_flags == desc->flags)
            return view_types[i].view_target;
    }

    FIXME("Unhandled view flags %#x for texture target %#x.\n", desc->flags, texture->target);
    return texture->target;
}

static void create_texture_view(struct wined3d_gl_view *view, GLenum view_target,
        const struct wined3d_view_desc *desc, struct wined3d_texture *texture,
        const struct wined3d_format *view_format)
{
    const struct wined3d_gl_info *gl_info;
    unsigned int layer_idx, layer_count;
    struct wined3d_context *context;
    struct gl_texture *gl_texture;

    view->target = view_target;

    context = context_acquire(texture->resource.device, NULL);
    gl_info = context->gl_info;

    if (!gl_info->supported[ARB_TEXTURE_VIEW])
    {
        context_release(context);
        FIXME("OpenGL implementation does not support texture views.\n");
        return;
    }

    wined3d_texture_prepare_texture(texture, context, FALSE);
    gl_texture = wined3d_texture_get_gl_texture(texture, FALSE);

    layer_idx = desc->u.texture.layer_idx;
    layer_count = desc->u.texture.layer_count;
    if (view_target == GL_TEXTURE_3D && (layer_idx || layer_count != 1))
    {
        FIXME("Depth slice (%u-%u) not supported.\n", layer_idx, layer_count);
        layer_idx = 0;
        layer_count = 1;
    }

    gl_info->gl_ops.gl.p_glGenTextures(1, &view->name);
    GL_EXTCALL(glTextureView(view->name, view->target, gl_texture->name, view_format->glInternal,
            desc->u.texture.level_idx, desc->u.texture.level_count,
            layer_idx, layer_count));
    checkGLcall("Create texture view");

    if (is_stencil_view_format(view_format))
    {
        static const GLint swizzle[] = {GL_ZERO, GL_RED, GL_ZERO, GL_ZERO};

        if (!gl_info->supported[ARB_STENCIL_TEXTURING])
        {
            context_release(context);
            FIXME("OpenGL implementation does not support stencil texturing.\n");
            return;
        }

        context_bind_texture(context, view->target, view->name);
        gl_info->gl_ops.gl.p_glTexParameteriv(view->target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        gl_info->gl_ops.gl.p_glTexParameteri(view->target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
        checkGLcall("Initialize stencil view");

        context_invalidate_state(context, STATE_SHADER_RESOURCE_BINDING);
    }

    context_release(context);
}

static void create_buffer_texture(struct wined3d_gl_view *view,
        struct wined3d_buffer *buffer, const struct wined3d_format *view_format)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    context = context_acquire(buffer->resource.device, NULL);
    gl_info = context->gl_info;
    if (!gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
    {
        FIXME("OpenGL implementation does not support buffer textures.\n");
        context_release(context);
        return;
    }

    wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER);

    view->target = GL_TEXTURE_BUFFER;
    gl_info->gl_ops.gl.p_glGenTextures(1, &view->name);

    context_bind_texture(context, GL_TEXTURE_BUFFER, view->name);
    GL_EXTCALL(glTexBuffer(GL_TEXTURE_BUFFER, view_format->glInternal, buffer->buffer_object));
    checkGLcall("Create buffer texture");

    context_invalidate_state(context, STATE_SHADER_RESOURCE_BINDING);

    context_release(context);
}

ULONG CDECL wined3d_rendertarget_view_incref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

static void wined3d_rendertarget_view_destroy_object(void *object)
{
    HeapFree(GetProcessHeap(), 0, object);
}

ULONG CDECL wined3d_rendertarget_view_decref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        struct wined3d_resource *resource = view->resource;
        struct wined3d_device *device = resource->device;

        /* Call wined3d_object_destroyed() before releasing the resource,
         * since releasing the resource may end up destroying the parent. */
        view->parent_ops->wined3d_object_destroyed(view->parent);
        wined3d_cs_emit_destroy_object(device->cs, wined3d_rendertarget_view_destroy_object, view);
        wined3d_resource_decref(resource);
    }

    return refcount;
}

void * CDECL wined3d_rendertarget_view_get_parent(const struct wined3d_rendertarget_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

void * CDECL wined3d_rendertarget_view_get_sub_resource_parent(const struct wined3d_rendertarget_view *view)
{
    struct wined3d_texture *texture;

    TRACE("view %p.\n", view);

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
        return wined3d_buffer_get_parent(buffer_from_resource(view->resource));

    texture = texture_from_resource(view->resource);

    return texture->sub_resources[view->sub_resource_idx].parent;
}

void CDECL wined3d_rendertarget_view_set_parent(struct wined3d_rendertarget_view *view, void *parent)
{
    TRACE("view %p, parent %p.\n", view, parent);

    view->parent = parent;
}

struct wined3d_resource * CDECL wined3d_rendertarget_view_get_resource(const struct wined3d_rendertarget_view *view)
{
    TRACE("view %p.\n", view);

    return view->resource;
}

void wined3d_rendertarget_view_get_drawable_size(const struct wined3d_rendertarget_view *view,
        const struct wined3d_context *context, unsigned int *width, unsigned int *height)
{
    const struct wined3d_texture *texture;

    if (view->resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(view->resource->type));
        *width = 0;
        *height = 0;
        return;
    }

    texture = texture_from_resource(view->resource);
    if (texture->swapchain)
    {
        /* The drawable size of an onscreen drawable is the surface size.
         * (Actually: The window size, but the surface is created in window
         * size.) */
        *width = context->current_rt.texture->resource.width;
        *height = context->current_rt.texture->resource.height;
    }
    else if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        const struct wined3d_swapchain *swapchain = context->swapchain;

        /* The drawable size of a backbuffer / aux buffer offscreen target is
         * the size of the current context's drawable, which is the size of
         * the back buffer of the swapchain the active context belongs to. */
        *width = swapchain->desc.backbuffer_width;
        *height = swapchain->desc.backbuffer_height;
    }
    else
    {
        struct wined3d_surface *rt;

        /* The drawable size of an FBO target is the OpenGL texture size,
         * which is the power of two size. */
        rt = context->current_rt.texture->sub_resources[context->current_rt.sub_resource_idx].u.surface;
        *width = wined3d_texture_get_level_pow2_width(rt->container, rt->texture_level);
        *height = wined3d_texture_get_level_pow2_height(rt->container, rt->texture_level);
    }
}

static HRESULT wined3d_rendertarget_view_init(struct wined3d_rendertarget_view *view,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &resource->device->adapter->gl_info;

    view->refcount = 1;
    view->parent = parent;
    view->parent_ops = parent_ops;

    view->format = wined3d_get_format(gl_info, desc->format_id, resource->usage);
    view->format_flags = view->format->flags[resource->gl_type];

    if (wined3d_format_is_typeless(view->format))
    {
        WARN("Trying to create view for typeless format %s.\n", debug_d3dformat(view->format->id));
        return E_INVALIDARG;
    }

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
        struct wined3d_texture *texture = texture_from_resource(resource);
        unsigned int depth_or_layer_count;

        if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
            depth_or_layer_count = wined3d_texture_get_level_depth(texture, desc->u.texture.level_idx);
        else
            depth_or_layer_count = texture->layer_count;

        if (desc->u.texture.level_idx >= texture->level_count
                || desc->u.texture.level_count != 1
                || desc->u.texture.layer_idx >= depth_or_layer_count
                || !desc->u.texture.layer_count
                || desc->u.texture.layer_count > depth_or_layer_count - desc->u.texture.layer_idx)
            return E_INVALIDARG;

        view->sub_resource_idx = desc->u.texture.level_idx;
        if (resource->type != WINED3D_RTYPE_TEXTURE_3D)
            view->sub_resource_idx += desc->u.texture.layer_idx * texture->level_count;
        view->buffer_offset = 0;
        view->width = wined3d_texture_get_level_width(texture, desc->u.texture.level_idx);
        view->height = wined3d_texture_get_level_height(texture, desc->u.texture.level_idx);
        view->depth = desc->u.texture.layer_count;
    }
    wined3d_resource_incref(view->resource = resource);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_rendertarget_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view)
{
    struct wined3d_rendertarget_view *object;
    HRESULT hr;

    TRACE("desc %p, resource %p, parent %p, parent_ops %p, view %p.\n",
            desc, resource, parent, parent_ops, view);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_rendertarget_view_init(object, desc, resource, parent, parent_ops)))
    {
        HeapFree(GetProcessHeap(), 0, object);
        WARN("Failed to initialise view, hr %#x.\n", hr);
        return hr;
    }

    TRACE("Created render target view %p.\n", object);
    *view = object;

    return hr;
}

HRESULT CDECL wined3d_rendertarget_view_create_from_sub_resource(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view)
{
    struct wined3d_view_desc desc;

    TRACE("texture %p, sub_resource_idx %u, parent %p, parent_ops %p, view %p.\n",
            texture, sub_resource_idx, parent, parent_ops, view);

    desc.format_id = texture->resource.format->id;
    desc.flags = 0;
    desc.u.texture.level_idx = sub_resource_idx % texture->level_count;
    desc.u.texture.level_count = 1;
    desc.u.texture.layer_idx = sub_resource_idx / texture->level_count;
    desc.u.texture.layer_count = 1;

    return wined3d_rendertarget_view_create(&desc, &texture->resource, parent, parent_ops, view);
}

ULONG CDECL wined3d_shader_resource_view_incref(struct wined3d_shader_resource_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

static void wined3d_shader_resource_view_destroy_object(void *object)
{
    struct wined3d_shader_resource_view *view = object;

    if (view->gl_view.name)
    {
        const struct wined3d_gl_info *gl_info;
        struct wined3d_context *context;

        context = context_acquire(view->resource->device, NULL);
        gl_info = context->gl_info;
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &view->gl_view.name);
        checkGLcall("glDeleteTextures");
        context_release(context);
    }

    HeapFree(GetProcessHeap(), 0, view);
}

ULONG CDECL wined3d_shader_resource_view_decref(struct wined3d_shader_resource_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        struct wined3d_resource *resource = view->resource;
        struct wined3d_device *device = resource->device;

        /* Call wined3d_object_destroyed() before releasing the resource,
         * since releasing the resource may end up destroying the parent. */
        view->parent_ops->wined3d_object_destroyed(view->parent);
        wined3d_cs_emit_destroy_object(device->cs, wined3d_shader_resource_view_destroy_object, view);
        wined3d_resource_decref(resource);
    }

    return refcount;
}

void * CDECL wined3d_shader_resource_view_get_parent(const struct wined3d_shader_resource_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

static HRESULT wined3d_shader_resource_view_init(struct wined3d_shader_resource_view *view,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &resource->device->adapter->gl_info;
    const struct wined3d_format *view_format;
    GLenum view_target;

    view_format = wined3d_get_format(gl_info, desc->format_id, resource->usage);
    if (wined3d_format_is_typeless(view_format)
            && !(view_format->id == WINED3DFMT_R32_TYPELESS && (desc->flags & WINED3D_VIEW_BUFFER_RAW)))
    {
        WARN("Trying to create view for typeless format %s.\n", debug_d3dformat(view_format->id));
        return E_INVALIDARG;
    }

    view->refcount = 1;
    view->parent = parent;
    view->parent_ops = parent_ops;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_buffer *buffer = buffer_from_resource(resource);

        if (desc->format_id == WINED3DFMT_UNKNOWN)
        {
            FIXME("Structured buffer views not supported.\n");
        }
        else if (desc->flags & WINED3D_VIEW_BUFFER_RAW)
        {
            FIXME("Raw buffer views not supported.\n");
        }
        else
        {
            /* FIXME: Support for buffer offsets can be implemented using ARB_texture_buffer_range. */
            if (desc->u.buffer.start_idx
                    || desc->u.buffer.count * view_format->byte_count != buffer->resource.size)
            {
                FIXME("Ignoring buffer range %u-%u.\n", desc->u.buffer.start_idx, desc->u.buffer.count);
            }

            create_buffer_texture(&view->gl_view, buffer, view_format);
        }
    }
    else
    {
        struct wined3d_texture *texture = texture_from_resource(resource);

        if (!desc->u.texture.level_count
                || desc->u.texture.level_idx >= texture->level_count
                || desc->u.texture.level_count > texture->level_count - desc->u.texture.level_idx
                || !desc->u.texture.layer_count
                || desc->u.texture.layer_idx >= texture->layer_count
                || desc->u.texture.layer_count > texture->layer_count - desc->u.texture.layer_idx)
            return E_INVALIDARG;

        view_target = get_texture_view_target(desc, texture);

        if (resource->format->id == view_format->id && texture->target == view_target
                && !desc->u.texture.level_idx && desc->u.texture.level_count == texture->level_count
                && !desc->u.texture.layer_idx && desc->u.texture.layer_count == texture->layer_count
                && !is_stencil_view_format(view_format))
        {
            TRACE("Creating identity shader resource view.\n");
        }
        else if (texture->swapchain && texture->swapchain->desc.backbuffer_count > 1)
        {
            FIXME("Swapchain shader resource views not supported.\n");
        }
        else if (resource->format->typeless_id == view_format->typeless_id
                && resource->format->gl_view_class == view_format->gl_view_class)
        {
            create_texture_view(&view->gl_view, view_target, desc, texture, view_format);
        }
        else
        {
            FIXME("Shader resource view not supported, resource format %s, view format %s.\n",
                    debug_d3dformat(resource->format->id), debug_d3dformat(view_format->id));
        }
    }
    wined3d_resource_incref(view->resource = resource);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_shader_resource_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_shader_resource_view **view)
{
    struct wined3d_shader_resource_view *object;
    HRESULT hr;

    TRACE("desc %p, resource %p, parent %p, parent_ops %p, view %p.\n",
            desc, resource, parent, parent_ops, view);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_shader_resource_view_init(object, desc, resource, parent, parent_ops)))
    {
        HeapFree(GetProcessHeap(), 0, object);
        WARN("Failed to initialise view, hr %#x.\n", hr);
        return hr;
    }

    TRACE("Created shader resource view %p.\n", object);
    *view = object;

    return WINED3D_OK;
}

void wined3d_shader_resource_view_bind(struct wined3d_shader_resource_view *view,
        struct wined3d_context *context)
{
    struct wined3d_texture *texture;

    if (view->gl_view.name)
    {
        context_bind_texture(context, view->gl_view.target, view->gl_view.name);
        return;
    }

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Buffer shader resources not supported.\n");
        return;
    }

    texture = wined3d_texture_from_resource(view->resource);
    wined3d_texture_bind(texture, context, FALSE);
}

ULONG CDECL wined3d_unordered_access_view_incref(struct wined3d_unordered_access_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

static void wined3d_unordered_access_view_destroy_object(void *object)
{
    HeapFree(GetProcessHeap(), 0, object);
}

ULONG CDECL wined3d_unordered_access_view_decref(struct wined3d_unordered_access_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        struct wined3d_resource *resource = view->resource;
        struct wined3d_device *device = resource->device;

        /* Call wined3d_object_destroyed() before releasing the resource,
         * since releasing the resource may end up destroying the parent. */
        view->parent_ops->wined3d_object_destroyed(view->parent);
        wined3d_cs_emit_destroy_object(device->cs, wined3d_unordered_access_view_destroy_object, view);
        wined3d_resource_decref(resource);
    }

    return refcount;
}

void * CDECL wined3d_unordered_access_view_get_parent(const struct wined3d_unordered_access_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

void wined3d_unordered_access_view_invalidate_location(struct wined3d_unordered_access_view *view,
        DWORD location)
{
    struct wined3d_resource *resource = view->resource;
    unsigned int i, sub_resource_idx, layer_count;
    struct wined3d_texture *texture;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        return;
    }

    texture = texture_from_resource(resource);

    sub_resource_idx = view->layer_idx * texture->level_count + view->level_idx;
    layer_count = (resource->type != WINED3D_RTYPE_TEXTURE_3D) ? view->layer_count : 1;
    for (i = 0; i < layer_count; ++i, sub_resource_idx += texture->level_count)
        wined3d_texture_invalidate_location(texture, sub_resource_idx, location);
}

static HRESULT wined3d_unordered_access_view_init(struct wined3d_unordered_access_view *view,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &resource->device->adapter->gl_info;

    view->refcount = 1;
    view->parent = parent;
    view->parent_ops = parent_ops;

    view->format = wined3d_get_format(gl_info, desc->format_id, resource->usage);

    if (wined3d_format_is_typeless(view->format)
            && !(view->format->id == WINED3DFMT_R32_TYPELESS && (desc->flags & WINED3D_VIEW_BUFFER_RAW)))
    {
        WARN("Trying to create view for typeless format %s.\n", debug_d3dformat(view->format->id));
        return E_INVALIDARG;
    }

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Buffer unordered access views not supported.\n");
    }
    else
    {
        struct wined3d_texture *texture = texture_from_resource(resource);
        unsigned int depth_or_layer_count;

        if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
            depth_or_layer_count = wined3d_texture_get_level_depth(texture, desc->u.texture.level_idx);
        else
            depth_or_layer_count = texture->layer_count;

        if (desc->u.texture.level_idx >= texture->level_count
                || desc->u.texture.level_count != 1
                || desc->u.texture.layer_idx >= depth_or_layer_count
                || !desc->u.texture.layer_count
                || desc->u.texture.layer_count > depth_or_layer_count - desc->u.texture.layer_idx)
            return E_INVALIDARG;

        if (desc->u.texture.layer_idx || desc->u.texture.layer_count != depth_or_layer_count)
        {
            create_texture_view(&view->gl_view, get_texture_view_target(desc, texture),
                    desc, texture, view->format);
        }

        view->layer_idx = desc->u.texture.layer_idx;
        view->layer_count = desc->u.texture.layer_count;
        view->level_idx = desc->u.texture.level_idx;
    }
    wined3d_resource_incref(view->resource = resource);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_unordered_access_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_unordered_access_view **view)
{
    struct wined3d_unordered_access_view *object;
    HRESULT hr;

    TRACE("desc %p, resource %p, parent %p, parent_ops %p, view %p.\n",
            desc, resource, parent, parent_ops, view);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_unordered_access_view_init(object, desc, resource, parent, parent_ops)))
    {
        HeapFree(GetProcessHeap(), 0, object);
        WARN("Failed to initialise view, hr %#x.\n", hr);
        return hr;
    }

    TRACE("Created unordered access view %p.\n", object);
    *view = object;

    return WINED3D_OK;
}
