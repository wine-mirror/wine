/*
 * Copyright 2013 Henri Verbeet for CodeWeavers
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

#define WINED3D_INITIAL_CS_SIZE 4096

enum wined3d_cs_op
{
    WINED3D_CS_OP_PRESENT,
    WINED3D_CS_OP_CLEAR,
    WINED3D_CS_OP_DRAW,
    WINED3D_CS_OP_SET_VIEWPORT,
    WINED3D_CS_OP_SET_SCISSOR_RECT,
    WINED3D_CS_OP_SET_RENDER_TARGET,
    WINED3D_CS_OP_SET_DEPTH_STENCIL,
    WINED3D_CS_OP_SET_VERTEX_DECLARATION,
    WINED3D_CS_OP_SET_STREAM_SOURCE,
    WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ,
    WINED3D_CS_OP_SET_INDEX_BUFFER,
    WINED3D_CS_OP_SET_TEXTURE,
};

struct wined3d_cs_present
{
    enum wined3d_cs_op opcode;
    HWND dst_window_override;
    struct wined3d_swapchain *swapchain;
    const RECT *src_rect;
    const RECT *dst_rect;
    const RGNDATA *dirty_region;
    DWORD flags;
};

struct wined3d_cs_clear
{
    enum wined3d_cs_op opcode;
    DWORD rect_count;
    const RECT *rects;
    DWORD flags;
    const struct wined3d_color *color;
    float depth;
    DWORD stencil;
};

struct wined3d_cs_draw
{
    enum wined3d_cs_op opcode;
    UINT start_idx;
    UINT index_count;
    UINT start_instance;
    UINT instance_count;
    BOOL indexed;
};

struct wined3d_cs_set_viewport
{
    enum wined3d_cs_op opcode;
    const struct wined3d_viewport *viewport;
};

struct wined3d_cs_set_scissor_rect
{
    enum wined3d_cs_op opcode;
    const RECT *rect;
};

struct wined3d_cs_set_render_target
{
    enum wined3d_cs_op opcode;
    UINT render_target_idx;
    struct wined3d_surface *render_target;
};

struct wined3d_cs_set_depth_stencil
{
    enum wined3d_cs_op opcode;
    struct wined3d_surface *depth_stencil;
};

struct wined3d_cs_set_vertex_declaration
{
    enum wined3d_cs_op opcode;
    struct wined3d_vertex_declaration *declaration;
};

struct wined3d_cs_set_stream_source
{
    enum wined3d_cs_op opcode;
    UINT stream_idx;
    struct wined3d_buffer *buffer;
    UINT offset;
    UINT stride;
};

struct wined3d_cs_set_stream_source_freq
{
    enum wined3d_cs_op opcode;
    UINT stream_idx;
    UINT frequency;
    UINT flags;
};

struct wined3d_cs_set_index_buffer
{
    enum wined3d_cs_op opcode;
    struct wined3d_buffer *buffer;
    enum wined3d_format_id format_id;
};

struct wined3d_cs_set_texture
{
    enum wined3d_cs_op opcode;
    UINT stage;
    struct wined3d_texture *texture;
};

static void wined3d_cs_exec_present(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_present *op = data;
    struct wined3d_swapchain *swapchain;

    swapchain = op->swapchain;
    wined3d_swapchain_set_window(swapchain, op->dst_window_override);

    swapchain->swapchain_ops->swapchain_present(swapchain,
            op->src_rect, op->dst_rect, op->dirty_region, op->flags);
}

void wined3d_cs_emit_present(struct wined3d_cs *cs, struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags)
{
    struct wined3d_cs_present *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_PRESENT;
    op->dst_window_override = dst_window_override;
    op->swapchain = swapchain;
    op->src_rect = src_rect;
    op->dst_rect = dst_rect;
    op->dirty_region = dirty_region;
    op->flags = flags;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_clear(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_clear *op = data;
    struct wined3d_device *device;
    RECT draw_rect;

    device = cs->device;
    wined3d_get_draw_rect(&device->state, &draw_rect);
    device_clear_render_targets(device, device->adapter->gl_info.limits.buffers,
            &device->fb, op->rect_count, op->rects, &draw_rect, op->flags,
            op->color, op->depth, op->stencil);
}

void wined3d_cs_emit_clear(struct wined3d_cs *cs, DWORD rect_count, const RECT *rects,
        DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    struct wined3d_cs_clear *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_CLEAR;
    op->rect_count = rect_count;
    op->rects = rects;
    op->flags = flags;
    op->color = color;
    op->depth = depth;
    op->stencil = stencil;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_draw(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_draw *op = data;

    draw_primitive(cs->device, op->start_idx, op->index_count,
            op->start_instance, op->instance_count, op->indexed);
}

void wined3d_cs_emit_draw(struct wined3d_cs *cs, UINT start_idx, UINT index_count,
        UINT start_instance, UINT instance_count, BOOL indexed)
{
    struct wined3d_cs_draw *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_DRAW;
    op->start_idx = start_idx;
    op->index_count = index_count;
    op->start_instance = start_instance;
    op->instance_count = instance_count;
    op->indexed = indexed;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_viewport(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_viewport *op = data;

    cs->state.viewport = *op->viewport;
    device_invalidate_state(cs->device, STATE_VIEWPORT);
}

void wined3d_cs_emit_set_viewport(struct wined3d_cs *cs, const struct wined3d_viewport *viewport)
{
    struct wined3d_cs_set_viewport *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_VIEWPORT;
    op->viewport = viewport;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_scissor_rect(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_scissor_rect *op = data;

    cs->state.scissor_rect = *op->rect;
    device_invalidate_state(cs->device, STATE_SCISSORRECT);
}

void wined3d_cs_emit_set_scissor_rect(struct wined3d_cs *cs, const RECT *rect)
{
    struct wined3d_cs_set_scissor_rect *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_SCISSOR_RECT;
    op->rect = rect;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_render_target(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_render_target *op = data;

    cs->state.fb->render_targets[op->render_target_idx] = op->render_target;
    device_invalidate_state(cs->device, STATE_FRAMEBUFFER);
}

void wined3d_cs_emit_set_render_target(struct wined3d_cs *cs, UINT render_target_idx,
        struct wined3d_surface *render_target)
{
    struct wined3d_cs_set_render_target *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_RENDER_TARGET;
    op->render_target_idx = render_target_idx;
    op->render_target = render_target;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_depth_stencil(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_depth_stencil *op = data;
    struct wined3d_device *device = cs->device;
    struct wined3d_surface *prev;

    if ((prev = cs->state.fb->depth_stencil))
    {
        if (device->swapchains[0]->desc.flags & WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL
                || prev->flags & SFLAG_DISCARD)
        {
            surface_modify_ds_location(prev, SFLAG_DISCARDED,
                    prev->resource.width, prev->resource.height);
            if (prev == device->onscreen_depth_stencil)
            {
                wined3d_surface_decref(device->onscreen_depth_stencil);
                device->onscreen_depth_stencil = NULL;
            }
        }
    }

    cs->fb.depth_stencil = op->depth_stencil;

    if (!prev != !op->depth_stencil)
    {
        /* Swapping NULL / non NULL depth stencil affects the depth and tests */
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_ZENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    }
    else if (prev && prev->resource.format->depth_size != op->depth_stencil->resource.format->depth_size)
    {
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    }

    device_invalidate_state(device, STATE_FRAMEBUFFER);
}

void wined3d_cs_emit_set_depth_stencil(struct wined3d_cs *cs, struct wined3d_surface *depth_stencil)
{
    struct wined3d_cs_set_depth_stencil *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_DEPTH_STENCIL;
    op->depth_stencil = depth_stencil;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_vertex_declaration(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_vertex_declaration *op = data;

    cs->state.vertex_declaration = op->declaration;
    device_invalidate_state(cs->device, STATE_VDECL);
}

void wined3d_cs_emit_set_vertex_declaration(struct wined3d_cs *cs, struct wined3d_vertex_declaration *declaration)
{
    struct wined3d_cs_set_vertex_declaration *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_VERTEX_DECLARATION;
    op->declaration = declaration;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_stream_source(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_stream_source *op = data;
    struct wined3d_stream_state *stream;
    struct wined3d_buffer *prev;

    stream = &cs->state.streams[op->stream_idx];
    prev = stream->buffer;
    stream->buffer = op->buffer;
    stream->offset = op->offset;
    stream->stride = op->stride;

    if (op->buffer)
        InterlockedIncrement(&op->buffer->resource.bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource.bind_count);

    device_invalidate_state(cs->device, STATE_STREAMSRC);
}

void wined3d_cs_emit_set_stream_source(struct wined3d_cs *cs, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset, UINT stride)
{
    struct wined3d_cs_set_stream_source *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_STREAM_SOURCE;
    op->stream_idx = stream_idx;
    op->buffer = buffer;
    op->offset = offset;
    op->stride = stride;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_stream_source_freq(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_stream_source_freq *op = data;
    struct wined3d_stream_state *stream;

    stream = &cs->state.streams[op->stream_idx];
    stream->frequency = op->frequency;
    stream->flags = op->flags;

    device_invalidate_state(cs->device, STATE_STREAMSRC);
}

void wined3d_cs_emit_set_stream_source_freq(struct wined3d_cs *cs, UINT stream_idx, UINT frequency, UINT flags)
{
    struct wined3d_cs_set_stream_source_freq *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ;
    op->stream_idx = stream_idx;
    op->frequency = frequency;
    op->flags = flags;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_index_buffer(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_index_buffer *op = data;
    struct wined3d_buffer *prev;

    prev = cs->state.index_buffer;
    cs->state.index_buffer = op->buffer;
    cs->state.index_format = op->format_id;

    if (op->buffer)
        InterlockedIncrement(&op->buffer->resource.bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource.bind_count);

    device_invalidate_state(cs->device, STATE_INDEXBUFFER);
}

void wined3d_cs_emit_set_index_buffer(struct wined3d_cs *cs, struct wined3d_buffer *buffer,
        enum wined3d_format_id format_id)
{
    struct wined3d_cs_set_index_buffer *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_INDEX_BUFFER;
    op->buffer = buffer;
    op->format_id = format_id;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_texture(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_d3d_info *d3d_info = &cs->device->adapter->d3d_info;
    const struct wined3d_cs_set_texture *op = data;
    struct wined3d_texture *prev;

    prev = cs->state.textures[op->stage];
    cs->state.textures[op->stage] = op->texture;

    if (op->texture)
    {
        if (InterlockedIncrement(&op->texture->resource.bind_count) == 1)
            op->texture->sampler = op->stage;

        if (!prev || op->texture->target != prev->target)
            device_invalidate_state(cs->device, STATE_PIXELSHADER);

        if (!prev && op->stage < d3d_info->limits.ffp_blend_stages)
        {
            /* The source arguments for color and alpha ops have different
             * meanings when a NULL texture is bound, so the COLOR_OP and
             * ALPHA_OP have to be dirtified. */
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_COLOR_OP));
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_ALPHA_OP));
        }
    }

    if (prev)
    {
        if (InterlockedDecrement(&prev->resource.bind_count) && prev->sampler == op->stage)
        {
            unsigned int i;

            /* Search for other stages the texture is bound to. Shouldn't
             * happen if applications bind textures to a single stage only. */
            TRACE("Searching for other stages the texture is bound to.\n");
            for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
            {
                if (cs->state.textures[i] == prev)
                {
                    TRACE("Texture is also bound to stage %u.\n", i);
                    prev->sampler = i;
                    break;
                }
            }
        }

        if (!op->texture && op->stage < d3d_info->limits.ffp_blend_stages)
        {
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_COLOR_OP));
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_ALPHA_OP));
        }
    }

    device_invalidate_state(cs->device, STATE_SAMPLER(op->stage));
}

void wined3d_cs_emit_set_texture(struct wined3d_cs *cs, UINT stage, struct wined3d_texture *texture)
{
    struct wined3d_cs_set_texture *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_TEXTURE;
    op->stage = stage;
    op->texture = texture;

    cs->ops->submit(cs);
}

static void (* const wined3d_cs_op_handlers[])(struct wined3d_cs *cs, const void *data) =
{
    /* WINED3D_CS_OP_PRESENT                */ wined3d_cs_exec_present,
    /* WINED3D_CS_OP_CLEAR                  */ wined3d_cs_exec_clear,
    /* WINED3D_CS_OP_DRAW                   */ wined3d_cs_exec_draw,
    /* WINED3D_CS_OP_SET_VIEWPORT           */ wined3d_cs_exec_set_viewport,
    /* WINED3D_CS_OP_SET_SCISSOR_RECT       */ wined3d_cs_exec_set_scissor_rect,
    /* WINED3D_CS_OP_SET_RENDER_TARGET      */ wined3d_cs_exec_set_render_target,
    /* WINED3D_CS_OP_SET_DEPTH_STENCIL      */ wined3d_cs_exec_set_depth_stencil,
    /* WINED3D_CS_OP_SET_VERTEX_DECLARATION */ wined3d_cs_exec_set_vertex_declaration,
    /* WINED3D_CS_OP_SET_STREAM_SOURCE      */ wined3d_cs_exec_set_stream_source,
    /* WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ */ wined3d_cs_exec_set_stream_source_freq,
    /* WINED3D_CS_OP_SET_INDEX_BUFFER       */ wined3d_cs_exec_set_index_buffer,
    /* WINED3D_CS_OP_SET_TEXTURE            */ wined3d_cs_exec_set_texture,
};

static void *wined3d_cs_st_require_space(struct wined3d_cs *cs, size_t size)
{
    if (size > cs->data_size)
    {
        void *new_data;

        size = max( size, cs->data_size * 2 );
        if (!(new_data = HeapReAlloc(GetProcessHeap(), 0, cs->data, size)))
            return NULL;

        cs->data_size = size;
        cs->data = new_data;
    }

    return cs->data;
}

static void wined3d_cs_st_submit(struct wined3d_cs *cs)
{
    enum wined3d_cs_op opcode = *(const enum wined3d_cs_op *)cs->data;

    wined3d_cs_op_handlers[opcode](cs, cs->data);
}

static const struct wined3d_cs_ops wined3d_cs_st_ops =
{
    wined3d_cs_st_require_space,
    wined3d_cs_st_submit,
};

struct wined3d_cs *wined3d_cs_create(struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_cs *cs;

    if (!(cs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*cs))))
        return NULL;

    if (!(cs->fb.render_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(*cs->fb.render_targets) * gl_info->limits.buffers)))
    {
        HeapFree(GetProcessHeap(), 0, cs);
        return NULL;
    }

    if (FAILED(state_init(&cs->state, &cs->fb, gl_info, &device->adapter->d3d_info,
            WINED3D_STATE_NO_REF | WINED3D_STATE_INIT_DEFAULT)))
    {
        HeapFree(GetProcessHeap(), 0, cs->fb.render_targets);
        HeapFree(GetProcessHeap(), 0, cs);
        return NULL;
    }

    cs->ops = &wined3d_cs_st_ops;
    cs->device = device;

    cs->data_size = WINED3D_INITIAL_CS_SIZE;
    if (!(cs->data = HeapAlloc(GetProcessHeap(), 0, cs->data_size)))
    {
        HeapFree(GetProcessHeap(), 0, cs);
        return NULL;
    }

    return cs;
}

void wined3d_cs_destroy(struct wined3d_cs *cs)
{
    state_cleanup(&cs->state);
    HeapFree(GetProcessHeap(), 0, cs->fb.render_targets);
    HeapFree(GetProcessHeap(), 0, cs);
}
