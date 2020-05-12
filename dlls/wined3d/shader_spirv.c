/*
 * Copyright 2018 Henri Verbeet for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

static const struct wined3d_shader_backend_ops spirv_shader_backend_vk;

struct shader_spirv_priv
{
    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct wined3d_fragment_pipe_ops *fragment_pipe;
    bool ffp_proj_control;
};

static void shader_spirv_handle_instruction(const struct wined3d_shader_instruction *ins)
{
}

static void shader_spirv_precompile(void *shader_priv, struct wined3d_shader *shader)
{
    WARN("Not implemented.\n");
}

static void shader_spirv_select(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    struct shader_spirv_priv *priv = shader_priv;

    priv->vertex_pipe->vp_enable(context, !use_vs(state));
    priv->fragment_pipe->fp_enable(context, !use_ps(state));
}

static void shader_spirv_select_compute(void *shader_priv,
        struct wined3d_context *context, const struct wined3d_state *state)
{
    FIXME("Not implemented.\n");
}

static void shader_spirv_disable(void *shader_priv, struct wined3d_context *context)
{
    struct shader_spirv_priv *priv = shader_priv;

    priv->vertex_pipe->vp_enable(context, false);
    priv->fragment_pipe->fp_enable(context, false);

    context->shader_update_mask = (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY)
            | (1u << WINED3D_SHADER_TYPE_HULL)
            | (1u << WINED3D_SHADER_TYPE_DOMAIN)
            | (1u << WINED3D_SHADER_TYPE_COMPUTE);
}

static void shader_spirv_update_float_vertex_constants(struct wined3d_device *device, UINT start, UINT count)
{
    WARN("Not implemented.\n");
}

static void shader_spirv_update_float_pixel_constants(struct wined3d_device *device, UINT start, UINT count)
{
    WARN("Not implemented.\n");
}

static void shader_spirv_load_constants(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    WARN("Not implemented.\n");
}

static void shader_spirv_destroy(struct wined3d_shader *shader)
{
}

static HRESULT shader_spirv_alloc(struct wined3d_device *device,
        const struct wined3d_vertex_pipe_ops *vertex_pipe, const struct wined3d_fragment_pipe_ops *fragment_pipe)
{
    struct fragment_caps fragment_caps;
    void *vertex_priv, *fragment_priv;
    struct shader_spirv_priv *priv;

    if (!(priv = heap_alloc(sizeof(*priv))))
        return E_OUTOFMEMORY;

    if (!(vertex_priv = vertex_pipe->vp_alloc(&spirv_shader_backend_vk, priv)))
    {
        ERR("Failed to initialise vertex pipe.\n");
        heap_free(priv);
        return E_FAIL;
    }

    if (!(fragment_priv = fragment_pipe->alloc_private(&spirv_shader_backend_vk, priv)))
    {
        ERR("Failed to initialise fragment pipe.\n");
        vertex_pipe->vp_free(device, NULL);
        heap_free(priv);
        return E_FAIL;
    }

    priv->vertex_pipe = vertex_pipe;
    priv->fragment_pipe = fragment_pipe;
    fragment_pipe->get_caps(device->adapter, &fragment_caps);
    priv->ffp_proj_control = fragment_caps.wined3d_caps & WINED3D_FRAGMENT_CAP_PROJ_CONTROL;

    device->vertex_priv = vertex_priv;
    device->fragment_priv = fragment_priv;
    device->shader_priv = priv;

    return WINED3D_OK;
}

static void shader_spirv_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct shader_spirv_priv *priv = device->shader_priv;

    priv->fragment_pipe->free_private(device, context);
    priv->vertex_pipe->vp_free(device, context);
    heap_free(priv);
}

static BOOL shader_spirv_allocate_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void shader_spirv_free_context_data(struct wined3d_context *context)
{
}

static void shader_spirv_init_context_state(struct wined3d_context *context)
{
}

static void shader_spirv_get_caps(const struct wined3d_adapter *adapter, struct shader_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
}

static BOOL shader_spirv_color_fixup_supported(struct color_fixup_desc fixup)
{
    return is_identity_fixup(fixup);
}

static BOOL shader_spirv_has_ffp_proj_control(void *shader_priv)
{
    struct shader_spirv_priv *priv = shader_priv;

    return priv->ffp_proj_control;
}

static const struct wined3d_shader_backend_ops spirv_shader_backend_vk =
{
    .shader_handle_instruction = shader_spirv_handle_instruction,
    .shader_precompile = shader_spirv_precompile,
    .shader_select = shader_spirv_select,
    .shader_select_compute = shader_spirv_select_compute,
    .shader_disable = shader_spirv_disable,
    .shader_update_float_vertex_constants = shader_spirv_update_float_vertex_constants,
    .shader_update_float_pixel_constants = shader_spirv_update_float_pixel_constants,
    .shader_load_constants = shader_spirv_load_constants,
    .shader_destroy = shader_spirv_destroy,
    .shader_alloc_private = shader_spirv_alloc,
    .shader_free_private = shader_spirv_free,
    .shader_allocate_context_data = shader_spirv_allocate_context_data,
    .shader_free_context_data = shader_spirv_free_context_data,
    .shader_init_context_state = shader_spirv_init_context_state,
    .shader_get_caps = shader_spirv_get_caps,
    .shader_color_fixup_supported = shader_spirv_color_fixup_supported,
    .shader_has_ffp_proj_control = shader_spirv_has_ffp_proj_control,
};

const struct wined3d_shader_backend_ops *wined3d_spirv_shader_backend_init_vk(void)
{
    return &spirv_shader_backend_vk;
}
