/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2011, 2013-2014 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2010 Henri Verbeet for CodeWeavers
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

#define WINED3D_BUFFER_HASDESC      0x01    /* A vertex description has been found. */
#define WINED3D_BUFFER_USE_BO       0x02    /* Use a buffer object for this buffer. */
#define WINED3D_BUFFER_PIN_SYSMEM   0x04    /* Keep a system memory copy for this buffer. */

#define VB_MAXDECLCHANGES     100     /* After that number of decl changes we stop converting */
#define VB_RESETDECLCHANGE    1000    /* Reset the decl changecount after that number of draws */
#define VB_MAXFULLCONVERSIONS 5       /* Number of full conversions before we stop converting */
#define VB_RESETFULLCONVS     20      /* Reset full conversion counts after that number of draws */

static void wined3d_buffer_evict_sysmem(struct wined3d_buffer *buffer)
{
    if (buffer->flags & WINED3D_BUFFER_PIN_SYSMEM)
    {
        TRACE("Not evicting system memory for buffer %p.\n", buffer);
        return;
    }

    TRACE("Evicting system memory for buffer %p.\n", buffer);
    wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_SYSMEM);
    wined3d_resource_free_sysmem(&buffer->resource);
}

static void buffer_invalidate_bo_range(struct wined3d_buffer *buffer, unsigned int offset, unsigned int size)
{
    if (!offset && (!size || size == buffer->resource.size))
        goto invalidate_all;

    if (offset > buffer->resource.size || size > buffer->resource.size - offset)
    {
        WARN("Invalid range specified, invalidating entire buffer.\n");
        goto invalidate_all;
    }

    if (!wined3d_array_reserve((void **)&buffer->maps, &buffer->maps_size,
            buffer->modified_areas + 1, sizeof(*buffer->maps)))
    {
        ERR("Failed to allocate maps array, invalidating entire buffer.\n");
        goto invalidate_all;
    }

    buffer->maps[buffer->modified_areas].offset = offset;
    buffer->maps[buffer->modified_areas].size = size;
    ++buffer->modified_areas;
    return;

invalidate_all:
    buffer->modified_areas = 1;
    buffer->maps[0].offset = 0;
    buffer->maps[0].size = buffer->resource.size;
}

static inline void buffer_clear_dirty_areas(struct wined3d_buffer *This)
{
    This->modified_areas = 0;
}

static BOOL buffer_is_dirty(const struct wined3d_buffer *buffer)
{
    return !!buffer->modified_areas;
}

static BOOL buffer_is_fully_dirty(const struct wined3d_buffer *buffer)
{
    return buffer->modified_areas == 1
            && !buffer->maps->offset && buffer->maps->size == buffer->resource.size;
}

static void wined3d_buffer_validate_location(struct wined3d_buffer *buffer, DWORD location)
{
    TRACE("buffer %p, location %s.\n", buffer, wined3d_debug_location(location));

    if (location & WINED3D_LOCATION_BUFFER)
        buffer_clear_dirty_areas(buffer);

    buffer->locations |= location;

    TRACE("New locations flags are %s.\n", wined3d_debug_location(buffer->locations));
}

static void wined3d_buffer_invalidate_range(struct wined3d_buffer *buffer, DWORD location,
        unsigned int offset, unsigned int size)
{
    TRACE("buffer %p, location %s, offset %u, size %u.\n",
            buffer, wined3d_debug_location(location), offset, size);

    if (location & WINED3D_LOCATION_BUFFER)
        buffer_invalidate_bo_range(buffer, offset, size);

    buffer->locations &= ~location;

    TRACE("New locations flags are %s.\n", wined3d_debug_location(buffer->locations));

    if (!buffer->locations)
        ERR("Buffer %p does not have any up to date location.\n", buffer);
}

void wined3d_buffer_invalidate_location(struct wined3d_buffer *buffer, DWORD location)
{
    wined3d_buffer_invalidate_range(buffer, location, 0, 0);
}

/* Context activation is done by the caller. */
static void wined3d_buffer_gl_bind(struct wined3d_buffer_gl *buffer_gl, struct wined3d_context_gl *context_gl)
{
    wined3d_context_gl_bind_bo(context_gl, buffer_gl->bo.binding, buffer_gl->bo.id);
}

/* Context activation is done by the caller. */
static void wined3d_buffer_gl_destroy_buffer_object(struct wined3d_buffer_gl *buffer_gl,
        struct wined3d_context_gl *context_gl)
{
    struct wined3d_resource *resource = &buffer_gl->b.resource;

    if (!buffer_gl->b.buffer_object)
        return;

    if (context_gl->c.transform_feedback_active && resource->bind_count
            && resource->bind_flags & WINED3D_BIND_STREAM_OUTPUT)
    {
        /* We have to make sure that transform feedback is not active
         * when deleting a potentially bound transform feedback buffer.
         * This may happen when the device is being destroyed. */
        WARN("Deleting buffer object for buffer %p, disabling transform feedback.\n", buffer_gl);
        wined3d_context_gl_end_transform_feedback(context_gl);
    }

    buffer_gl->bo_user.valid = false;
    list_remove(&buffer_gl->bo_user.entry);
    wined3d_context_gl_destroy_bo(context_gl, &buffer_gl->bo);
    buffer_gl->b.buffer_object = 0;
}

/* Context activation is done by the caller. */
static BOOL wined3d_buffer_gl_create_buffer_object(struct wined3d_buffer_gl *buffer_gl,
        struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum usage = GL_STATIC_DRAW;
    GLbitfield gl_storage_flags;
    struct wined3d_bo_gl *bo;
    bool coherent = true;
    GLsizeiptr size;
    GLenum binding;

    TRACE("Creating an OpenGL buffer object for wined3d buffer %p with usage %s.\n",
            buffer_gl, debug_d3dusage(buffer_gl->b.resource.usage));

    size = buffer_gl->b.resource.size;
    binding = wined3d_buffer_gl_binding_from_bind_flags(gl_info, buffer_gl->b.resource.bind_flags);
    if (buffer_gl->b.resource.usage & WINED3DUSAGE_DYNAMIC)
    {
        usage = GL_STREAM_DRAW_ARB;
        coherent = false;
    }
    gl_storage_flags = wined3d_resource_gl_storage_flags(&buffer_gl->b.resource);
    bo = &buffer_gl->bo;
    if (!wined3d_context_gl_create_bo(context_gl, size, binding, usage, coherent, gl_storage_flags, bo))
    {
        ERR("Failed to create OpenGL buffer object.\n");
        buffer_gl->b.flags &= ~WINED3D_BUFFER_USE_BO;
        buffer_clear_dirty_areas(&buffer_gl->b);
        return FALSE;
    }

    list_add_head(&buffer_gl->bo.users, &buffer_gl->bo_user.entry);
    buffer_gl->b.buffer_object = (uintptr_t)bo;
    buffer_invalidate_bo_range(&buffer_gl->b, 0, 0);

    return TRUE;
}

static BOOL buffer_process_converted_attribute(struct wined3d_buffer *buffer,
        const enum wined3d_buffer_conversion_type conversion_type,
        const struct wined3d_stream_info_element *attrib, DWORD *stride_this_run)
{
    const struct wined3d_format *format = attrib->format;
    BOOL ret = FALSE;
    unsigned int i;
    DWORD_PTR data;

    /* Check for some valid situations which cause us pain. One is if the buffer is used for
     * constant attributes(stride = 0), the other one is if the buffer is used on two streams
     * with different strides. In the 2nd case we might have to drop conversion entirely,
     * it is possible that the same bytes are once read as FLOAT2 and once as UBYTE4N.
     */
    if (!attrib->stride)
    {
        FIXME("%s used with stride 0, let's hope we get the vertex stride from somewhere else.\n",
                debug_d3dformat(format->id));
    }
    else if (attrib->stride != *stride_this_run && *stride_this_run)
    {
        FIXME("Got two concurrent strides, %d and %d.\n", attrib->stride, *stride_this_run);
    }
    else
    {
        *stride_this_run = attrib->stride;
        if (buffer->stride != *stride_this_run)
        {
            /* We rely that this happens only on the first converted attribute that is found,
             * if at all. See above check
             */
            TRACE("Reconverting because converted attributes occur, and the stride changed.\n");
            buffer->stride = *stride_this_run;
            heap_free(buffer->conversion_map);
            buffer->conversion_map = heap_calloc(buffer->stride, sizeof(*buffer->conversion_map));
            ret = TRUE;
        }
    }

    data = ((DWORD_PTR)attrib->data.addr) % buffer->stride;
    for (i = 0; i < format->byte_count; ++i)
    {
        DWORD_PTR idx = (data + i) % buffer->stride;
        if (buffer->conversion_map[idx] != conversion_type)
        {
            TRACE("Byte %lu in vertex changed:\n", idx);
            TRACE("    It was type %#x, is %#x now.\n", buffer->conversion_map[idx], conversion_type);
            ret = TRUE;
            buffer->conversion_map[idx] = conversion_type;
        }
    }

    return ret;
}

#define WINED3D_BUFFER_FIXUP_D3DCOLOR   0x01
#define WINED3D_BUFFER_FIXUP_XYZRHW     0x02

static BOOL buffer_check_attribute(struct wined3d_buffer *This, const struct wined3d_stream_info *si,
        const struct wined3d_state *state, UINT attrib_idx, DWORD fixup_flags, DWORD *stride_this_run)
{
    const struct wined3d_stream_info_element *attrib = &si->elements[attrib_idx];
    enum wined3d_format_id format;
    BOOL ret = FALSE;

    /* Ignore attributes that do not have our vbo. After that check we can be sure that the attribute is
     * there, on nonexistent attribs the vbo is 0.
     */
    if (!(si->use_map & (1u << attrib_idx))
            || state->streams[attrib->stream_idx].buffer != This)
        return FALSE;

    format = attrib->format->id;
    /* Look for newly appeared conversion */
    if (fixup_flags & WINED3D_BUFFER_FIXUP_D3DCOLOR && format == WINED3DFMT_B8G8R8A8_UNORM)
    {
        ret = buffer_process_converted_attribute(This, CONV_D3DCOLOR, attrib, stride_this_run);
    }
    else if (fixup_flags & WINED3D_BUFFER_FIXUP_XYZRHW && si->position_transformed)
    {
        if (format != WINED3DFMT_R32G32B32A32_FLOAT)
        {
            FIXME("Unexpected format %s for transformed position.\n", debug_d3dformat(format));
            return FALSE;
        }

        ret = buffer_process_converted_attribute(This, CONV_POSITIONT, attrib, stride_this_run);
    }
    else if (This->conversion_map)
    {
        ret = buffer_process_converted_attribute(This, CONV_NONE, attrib, stride_this_run);
    }

    return ret;
}

static BOOL buffer_find_decl(struct wined3d_buffer *This, const struct wined3d_stream_info *si,
        const struct wined3d_state *state, DWORD fixup_flags)
{
    UINT stride_this_run = 0;
    BOOL ret = FALSE;

    /* In d3d7 the vertex buffer declaration NEVER changes because it is stored in the d3d7 vertex buffer.
     * Once we have our declaration there is no need to look it up again. Index buffers also never need
     * conversion, so once the (empty) conversion structure is created don't bother checking again
     */
    if (This->flags & WINED3D_BUFFER_HASDESC)
    {
        if(This->resource.usage & WINED3DUSAGE_STATICDECL) return FALSE;
    }

    if (!fixup_flags)
    {
        TRACE("No fixup required.\n");
        if(This->conversion_map)
        {
            heap_free(This->conversion_map);
            This->conversion_map = NULL;
            This->stride = 0;
            return TRUE;
        }

        return FALSE;
    }

    TRACE("Finding vertex buffer conversion information\n");
    /* Certain declaration types need some fixups before we can pass them to
     * opengl. This means D3DCOLOR attributes with fixed function vertex
     * processing, FLOAT4 POSITIONT with fixed function, and FLOAT16 if
     * GL_ARB_half_float_vertex is not supported.
     *
     * Note for d3d8 and d3d9:
     * The vertex buffer FVF doesn't help with finding them, we have to use
     * the decoded vertex declaration and pick the things that concern the
     * current buffer. A problem with this is that this can change between
     * draws, so we have to validate the information and reprocess the buffer
     * if it changes, and avoid false positives for performance reasons.
     * WineD3D doesn't even know the vertex buffer any more, it is managed
     * by the client libraries and passed to SetStreamSource and ProcessVertices
     * as needed.
     *
     * We have to distinguish between vertex shaders and fixed function to
     * pick the way we access the strided vertex information.
     *
     * This code sets up a per-byte array with the size of the detected
     * stride of the arrays in the buffer. For each byte we have a field
     * that marks the conversion needed on this byte. For example, the
     * following declaration with fixed function vertex processing:
     *
     *      POSITIONT, FLOAT4
     *      NORMAL, FLOAT3
     *      DIFFUSE, FLOAT16_4
     *      SPECULAR, D3DCOLOR
     *
     * Will result in
     * {                 POSITIONT                    }{             NORMAL                }{    DIFFUSE          }{SPECULAR }
     * [P][P][P][P][P][P][P][P][P][P][P][P][P][P][P][P][0][0][0][0][0][0][0][0][0][0][0][0][F][F][F][F][F][F][F][F][C][C][C][C]
     *
     * Where in this example map P means 4 component position conversion, 0
     * means no conversion, F means FLOAT16_2 conversion and C means D3DCOLOR
     * conversion (red / blue swizzle).
     *
     * If we're doing conversion and the stride changes we have to reconvert
     * the whole buffer. Note that we do not mind if the semantic changes,
     * we only care for the conversion type. So if the NORMAL is replaced
     * with a TEXCOORD, nothing has to be done, or if the DIFFUSE is replaced
     * with a D3DCOLOR BLENDWEIGHT we can happily dismiss the change. Some
     * conversion types depend on the semantic as well, for example a FLOAT4
     * texcoord needs no conversion while a FLOAT4 positiont needs one
     */

    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_POSITION,
            fixup_flags, &stride_this_run) || ret;
    fixup_flags &= ~WINED3D_BUFFER_FIXUP_XYZRHW;

    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_BLENDWEIGHT,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_BLENDINDICES,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_NORMAL,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_DIFFUSE,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_SPECULAR,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD0,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD1,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD2,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD3,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD4,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD5,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD6,
            fixup_flags, &stride_this_run) || ret;
    ret = buffer_check_attribute(This, si, state, WINED3D_FFP_TEXCOORD7,
            fixup_flags, &stride_this_run) || ret;

    if (!stride_this_run && This->conversion_map)
    {
        /* Sanity test */
        if (!ret)
            ERR("no converted attributes found, old conversion map exists, and no declaration change?\n");
        heap_free(This->conversion_map);
        This->conversion_map = NULL;
        This->stride = 0;
    }

    if (ret) TRACE("Conversion information changed\n");

    return ret;
}

static inline unsigned int fixup_d3dcolor(DWORD *dst_color)
{
    DWORD src_color = *dst_color;

    /* Color conversion like in draw_primitive_immediate_mode(). Watch out for
     * endianness. If we want this to work on big-endian machines as well we
     * have to consider more things.
     *
     * 0xff000000: Alpha mask
     * 0x00ff0000: Blue mask
     * 0x0000ff00: Green mask
     * 0x000000ff: Red mask
     */
    *dst_color = 0;
    *dst_color |= (src_color & 0xff00ff00u);         /* Alpha Green */
    *dst_color |= (src_color & 0x00ff0000u) >> 16;   /* Red */
    *dst_color |= (src_color & 0x000000ffu) << 16;   /* Blue */

    return sizeof(*dst_color);
}

static inline unsigned int fixup_transformed_pos(struct wined3d_vec4 *p)
{
    /* rhw conversion like in position_float4(). */
    if (p->w != 1.0f && p->w != 0.0f)
    {
        float w = 1.0f / p->w;
        p->x *= w;
        p->y *= w;
        p->z *= w;
        p->w = w;
    }

    return sizeof(*p);
}

ULONG CDECL wined3d_buffer_incref(struct wined3d_buffer *buffer)
{
    ULONG refcount = InterlockedIncrement(&buffer->resource.ref);

    TRACE("%p increasing refcount to %u.\n", buffer, refcount);

    return refcount;
}

static void buffer_conversion_upload(struct wined3d_buffer *buffer, struct wined3d_context *context)
{
    unsigned int i, j, range_idx, start, end, vertex_count;
    BYTE *data;

    if (!wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM))
    {
        ERR("Failed to load system memory.\n");
        return;
    }
    buffer->flags |= WINED3D_BUFFER_PIN_SYSMEM;

    /* Now for each vertex in the buffer that needs conversion. */
    vertex_count = buffer->resource.size / buffer->stride;

    if (!(data = heap_alloc(buffer->resource.size)))
    {
        ERR("Out of memory.\n");
        return;
    }

    for (range_idx = 0; range_idx < buffer->modified_areas; ++range_idx)
    {
        start = buffer->maps[range_idx].offset;
        end = start + buffer->maps[range_idx].size;

        memcpy(data + start, (BYTE *)buffer->resource.heap_memory + start, end - start);
        for (i = start / buffer->stride; i < min((end / buffer->stride) + 1, vertex_count); ++i)
        {
            for (j = 0; j < buffer->stride;)
            {
                switch (buffer->conversion_map[j])
                {
                    case CONV_NONE:
                        /* Done already */
                        j += sizeof(DWORD);
                        break;
                    case CONV_D3DCOLOR:
                        j += fixup_d3dcolor((DWORD *) (data + i * buffer->stride + j));
                        break;
                    case CONV_POSITIONT:
                        j += fixup_transformed_pos((struct wined3d_vec4 *) (data + i * buffer->stride + j));
                        break;
                    default:
                        FIXME("Unimplemented conversion %d in shifted conversion.\n", buffer->conversion_map[j]);
                        ++j;
                }
            }
        }
    }

    buffer->buffer_ops->buffer_upload_ranges(buffer, context,
            data, 0, buffer->modified_areas, buffer->maps);

    heap_free(data);
}

BOOL wined3d_buffer_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    return buffer->buffer_ops->buffer_prepare_location(buffer, context, location);
}

static void wined3d_buffer_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    buffer->buffer_ops->buffer_unload_location(buffer, context, location);
}

BOOL wined3d_buffer_load_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, DWORD location)
{
    struct wined3d_range range;

    TRACE("buffer %p, context %p, location %s.\n",
            buffer, context, wined3d_debug_location(location));

    if (buffer->locations & location)
    {
        TRACE("Location (%#x) is already up to date.\n", location);
        return TRUE;
    }

    if (!buffer->locations)
    {
        ERR("Buffer %p does not have any up to date location.\n", buffer);
        wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_DISCARDED);
        return wined3d_buffer_load_location(buffer, context, location);
    }

    TRACE("Current buffer location %s.\n", wined3d_debug_location(buffer->locations));

    if (!wined3d_buffer_prepare_location(buffer, context, location))
        return FALSE;

    if (buffer->locations & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("Buffer previously discarded, nothing to do.\n");
        wined3d_buffer_validate_location(buffer, location);
        wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_DISCARDED);
        return TRUE;
    }

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            range.offset = 0;
            range.size = buffer->resource.size;
            buffer->buffer_ops->buffer_download_ranges(buffer, context,
                    buffer->resource.heap_memory, 0, 1, &range);
            break;

        case WINED3D_LOCATION_BUFFER:
            if (!buffer->conversion_map)
                buffer->buffer_ops->buffer_upload_ranges(buffer, context,
                        buffer->resource.heap_memory, 0, buffer->modified_areas, buffer->maps);
            else
                buffer_conversion_upload(buffer, context);
            break;

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }

    wined3d_buffer_validate_location(buffer, location);
    if (buffer->resource.heap_memory && location == WINED3D_LOCATION_BUFFER
            && !(buffer->resource.usage & WINED3DUSAGE_DYNAMIC))
        wined3d_buffer_evict_sysmem(buffer);

    return TRUE;
}

/* Context activation is done by the caller. */
BYTE *wined3d_buffer_load_sysmem(struct wined3d_buffer *buffer, struct wined3d_context *context)
{
    if (wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM))
        buffer->flags |= WINED3D_BUFFER_PIN_SYSMEM;
    return buffer->resource.heap_memory;
}

DWORD wined3d_buffer_get_memory(struct wined3d_buffer *buffer, struct wined3d_context *context,
        struct wined3d_bo_address *data)
{
    unsigned int locations = buffer->locations;

    TRACE("buffer %p, context %p, data %p, locations %s.\n",
            buffer, context, data, wined3d_debug_location(locations));

    if (locations & WINED3D_LOCATION_DISCARDED)
    {
        locations = ((buffer->flags & WINED3D_BUFFER_USE_BO) ? WINED3D_LOCATION_BUFFER : WINED3D_LOCATION_SYSMEM);
        if (!wined3d_buffer_prepare_location(buffer, context, locations))
        {
            data->buffer_object = 0;
            data->addr = NULL;
            return 0;
        }
        wined3d_buffer_validate_location(buffer, locations);
        wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_DISCARDED);
    }
    if (locations & WINED3D_LOCATION_BUFFER)
    {
        data->buffer_object = buffer->buffer_object;
        data->addr = NULL;
        return WINED3D_LOCATION_BUFFER;
    }
    if (locations & WINED3D_LOCATION_SYSMEM)
    {
        data->buffer_object = 0;
        data->addr = buffer->resource.heap_memory;
        return WINED3D_LOCATION_SYSMEM;
    }

    ERR("Unexpected locations %s.\n", wined3d_debug_location(locations));
    data->buffer_object = 0;
    data->addr = NULL;
    return 0;
}

static void buffer_resource_unload(struct wined3d_resource *resource)
{
    struct wined3d_buffer *buffer = buffer_from_resource(resource);

    TRACE("buffer %p.\n", buffer);

    if (buffer->buffer_object)
    {
        struct wined3d_context *context;

        context = context_acquire(resource->device, NULL, 0);

        wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM);
        wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_BUFFER);
        wined3d_buffer_unload_location(buffer, context, WINED3D_LOCATION_BUFFER);
        buffer_clear_dirty_areas(buffer);

        context_release(context);

        heap_free(buffer->conversion_map);
        buffer->conversion_map = NULL;
        buffer->stride = 0;
        buffer->conversion_stride = 0;
        buffer->flags &= ~WINED3D_BUFFER_HASDESC;
    }

    resource_unload(resource);
}

static void wined3d_buffer_drop_bo(struct wined3d_buffer *buffer)
{
    buffer->flags &= ~WINED3D_BUFFER_USE_BO;
    buffer_resource_unload(&buffer->resource);
}

static void wined3d_buffer_destroy_object(void *object)
{
    struct wined3d_buffer *buffer = object;
    struct wined3d_context *context;

    TRACE("buffer %p.\n", buffer);

    if (buffer->buffer_object)
    {
        context = context_acquire(buffer->resource.device, NULL, 0);
        wined3d_buffer_unload_location(buffer, context, WINED3D_LOCATION_BUFFER);
        context_release(context);
    }
    heap_free(buffer->conversion_map);
    heap_free(buffer->maps);
}

void wined3d_buffer_cleanup(struct wined3d_buffer *buffer)
{
    wined3d_cs_destroy_object(buffer->resource.device->cs, wined3d_buffer_destroy_object, buffer);
    resource_cleanup(&buffer->resource);
}

ULONG CDECL wined3d_buffer_decref(struct wined3d_buffer *buffer)
{
    ULONG refcount = InterlockedDecrement(&buffer->resource.ref);

    TRACE("%p decreasing refcount to %u.\n", buffer, refcount);

    if (!refcount)
    {
        buffer->resource.parent_ops->wined3d_object_destroyed(buffer->resource.parent);
        buffer->resource.device->adapter->adapter_ops->adapter_destroy_buffer(buffer);
    }

    return refcount;
}

void * CDECL wined3d_buffer_get_parent(const struct wined3d_buffer *buffer)
{
    TRACE("buffer %p.\n", buffer);

    return buffer->resource.parent;
}

/* Context activation is done by the caller. */
void wined3d_buffer_load(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    BOOL decl_changed = FALSE;

    TRACE("buffer %p.\n", buffer);

    if (buffer->resource.map_count && buffer->map_ptr)
    {
        FIXME("Buffer is mapped through buffer object, not loading.\n");
        return;
    }
    else if (buffer->resource.map_count)
    {
        WARN("Loading mapped buffer.\n");
    }

    /* TODO: Make converting independent from VBOs */
    if (!(buffer->flags & WINED3D_BUFFER_USE_BO))
    {
        /* Not doing any conversion */
        return;
    }

    if (!wined3d_buffer_prepare_location(buffer, context, WINED3D_LOCATION_BUFFER))
    {
        ERR("Failed to prepare buffer location.\n");
        return;
    }

    /* Reading the declaration makes only sense if we have valid state information
     * (i.e., if this function is called during draws). */
    if (state)
    {
        DWORD fixup_flags = 0;

        if (!use_vs(state))
        {
            if (!d3d_info->vertex_bgra && !d3d_info->ffp_generic_attributes)
                fixup_flags |= WINED3D_BUFFER_FIXUP_D3DCOLOR;
            if (!d3d_info->xyzrhw)
                fixup_flags |= WINED3D_BUFFER_FIXUP_XYZRHW;
        }

        decl_changed = buffer_find_decl(buffer, &context->stream_info, state, fixup_flags);
        buffer->flags |= WINED3D_BUFFER_HASDESC;
    }

    if (!decl_changed && !(buffer->flags & WINED3D_BUFFER_HASDESC && buffer_is_dirty(buffer)))
    {
        ++buffer->draw_count;
        if (buffer->draw_count > VB_RESETDECLCHANGE)
            buffer->decl_change_count = 0;
        if (buffer->draw_count > VB_RESETFULLCONVS)
            buffer->full_conversion_count = 0;
        return;
    }

    /* If applications change the declaration over and over, reconverting all the time is a huge
     * performance hit. So count the declaration changes and release the VBO if there are too many
     * of them (and thus stop converting)
     */
    if (decl_changed)
    {
        ++buffer->decl_change_count;
        buffer->draw_count = 0;

        if (buffer->decl_change_count > VB_MAXDECLCHANGES
                || (buffer->conversion_map && (buffer->resource.usage & WINED3DUSAGE_DYNAMIC)))
        {
            FIXME("Too many declaration changes or converting dynamic buffer, stopping converting.\n");
            wined3d_buffer_drop_bo(buffer);
            return;
        }

        /* The declaration changed, reload the whole buffer. */
        WARN("Reloading buffer because of a vertex declaration change.\n");
        buffer_invalidate_bo_range(buffer, 0, 0);
    }
    else
    {
        /* However, it is perfectly fine to change the declaration every now and then. We don't want a game that
         * changes it every minute drop the VBO after VB_MAX_DECL_CHANGES minutes. So count draws without
         * decl changes and reset the decl change count after a specific number of them
         */
        if (buffer->conversion_map && buffer_is_fully_dirty(buffer))
        {
            ++buffer->full_conversion_count;
            if (buffer->full_conversion_count > VB_MAXFULLCONVERSIONS)
            {
                FIXME("Too many full buffer conversions, stopping converting.\n");
                wined3d_buffer_drop_bo(buffer);
                return;
            }
        }
        else
        {
            ++buffer->draw_count;
            if (buffer->draw_count > VB_RESETDECLCHANGE)
                buffer->decl_change_count = 0;
            if (buffer->draw_count > VB_RESETFULLCONVS)
                buffer->full_conversion_count = 0;
        }
    }

    if (!wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER))
        ERR("Failed to load buffer location.\n");
}

struct wined3d_resource * CDECL wined3d_buffer_get_resource(struct wined3d_buffer *buffer)
{
    TRACE("buffer %p.\n", buffer);

    return &buffer->resource;
}

static HRESULT buffer_resource_sub_resource_get_desc(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc)
{
    if (sub_resource_idx)
    {
        WARN("Invalid sub_resource_idx %u.\n", sub_resource_idx);
        return E_INVALIDARG;
    }

    desc->format = WINED3DFMT_R8_UNORM;
    desc->multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc->multisample_quality = 0;
    desc->usage = resource->usage;
    desc->bind_flags = resource->bind_flags;
    desc->access = resource->access;
    desc->width = resource->size;
    desc->height = 1;
    desc->depth = 1;
    desc->size = resource->size;
    return S_OK;
}

static void buffer_resource_sub_resource_get_map_pitch(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    *row_pitch = *slice_pitch = resource->size;
}

static HRESULT buffer_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        void **map_ptr, const struct wined3d_box *box, uint32_t flags)
{
    struct wined3d_buffer *buffer = buffer_from_resource(resource);
    struct wined3d_device *device = resource->device;
    struct wined3d_context *context;
    unsigned int offset, size;
    uint8_t *base;
    LONG count;

    TRACE("resource %p, sub_resource_idx %u, map_ptr %p, box %s, flags %#x.\n",
            resource, sub_resource_idx, map_ptr, debug_box(box), flags);

    offset = box->left;
    size = box->right - box->left;

    count = ++resource->map_count;

    if (buffer->buffer_object)
    {
        unsigned int dirty_offset = offset, dirty_size = size;
        struct wined3d_bo_address addr;

        /* DISCARD invalidates the entire buffer, regardless of the specified
         * offset and size. Some applications also depend on the entire buffer
         * being uploaded in that case. Two such applications are Port Royale
         * and Darkstar One. */
        if (flags & WINED3D_MAP_DISCARD)
        {
            dirty_offset = 0;
            dirty_size = 0;
        }

        if (((flags & WINED3D_MAP_WRITE) && !(flags & (WINED3D_MAP_NOOVERWRITE | WINED3D_MAP_DISCARD)))
                || (!(flags & WINED3D_MAP_WRITE) && (buffer->locations & WINED3D_LOCATION_SYSMEM))
                || buffer->flags & WINED3D_BUFFER_PIN_SYSMEM)
        {
            if (!(buffer->locations & WINED3D_LOCATION_SYSMEM))
            {
                context = context_acquire(device, NULL, 0);
                wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM);
                context_release(context);
            }

            if (flags & WINED3D_MAP_WRITE)
                wined3d_buffer_invalidate_range(buffer, WINED3D_LOCATION_BUFFER, dirty_offset, dirty_size);
        }
        else
        {
            context = context_acquire(device, NULL, 0);

            if (flags & WINED3D_MAP_DISCARD)
                wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_BUFFER);
            else
                wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER);

            if (flags & WINED3D_MAP_WRITE)
            {
                wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_SYSMEM);
                buffer_invalidate_bo_range(buffer, dirty_offset, dirty_size);
            }

            if ((flags & WINED3D_MAP_DISCARD) && resource->heap_memory)
                wined3d_buffer_evict_sysmem(buffer);

            if (count == 1)
            {
                addr.buffer_object = buffer->buffer_object;
                addr.addr = 0;
                buffer->map_ptr = wined3d_context_map_bo_address(context, &addr, resource->size, flags);

                if (((DWORD_PTR)buffer->map_ptr) & (RESOURCE_ALIGNMENT - 1))
                {
                    WARN("Pointer %p is not %u byte aligned.\n", buffer->map_ptr, RESOURCE_ALIGNMENT);

                    wined3d_context_unmap_bo_address(context, &addr, 0, NULL);
                    buffer->map_ptr = NULL;

                    if (resource->usage & WINED3DUSAGE_DYNAMIC)
                    {
                        /* The extra copy is more expensive than not using VBOs
                         * at all on the NVIDIA Linux driver, which is the
                         * only driver that returns unaligned pointers. */
                        TRACE("Dynamic buffer, dropping VBO.\n");
                        wined3d_buffer_drop_bo(buffer);
                    }
                    else
                    {
                        TRACE("Falling back to doublebuffered operation.\n");
                        wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM);
                        buffer->flags |= WINED3D_BUFFER_PIN_SYSMEM;
                    }
                    TRACE("New pointer is %p.\n", resource->heap_memory);
                }
            }

            context_release(context);
        }
    }

    base = buffer->map_ptr ? buffer->map_ptr : resource->heap_memory;
    *map_ptr = base + offset;

    TRACE("Returning memory at %p (base %p, offset %u).\n", *map_ptr, base, offset);

    return WINED3D_OK;
}

static HRESULT buffer_resource_sub_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_buffer *buffer = buffer_from_resource(resource);
    unsigned int range_count = buffer->modified_areas;
    struct wined3d_device *device = resource->device;
    struct wined3d_context *context;
    struct wined3d_bo_address addr;

    TRACE("resource %p, sub_resource_idx %u.\n", resource, sub_resource_idx);

    if (sub_resource_idx)
    {
        WARN("Invalid sub_resource_idx %u.\n", sub_resource_idx);
        return E_INVALIDARG;
    }

    if (!resource->map_count)
    {
        WARN("Unmap called without a previous map call.\n");
        return WINED3D_OK;
    }

    if (--resource->map_count)
    {
        /* Delay loading the buffer until everything is unmapped. */
        TRACE("Ignoring unmap.\n");
        return WINED3D_OK;
    }

    if (!buffer->map_ptr)
        return WINED3D_OK;

    context = context_acquire(device, NULL, 0);

    addr.buffer_object = buffer->buffer_object;
    addr.addr = 0;
    wined3d_context_unmap_bo_address(context, &addr, range_count, buffer->maps);

    context_release(context);

    buffer_clear_dirty_areas(buffer);
    buffer->map_ptr = NULL;

    return WINED3D_OK;
}

void wined3d_buffer_copy_bo_address(struct wined3d_buffer *dst_buffer, struct wined3d_context *context,
        unsigned int dst_offset, const struct wined3d_const_bo_address *src_addr, unsigned int size)
{
    struct wined3d_bo_address dst_addr;
    DWORD dst_location;

    dst_location = wined3d_buffer_get_memory(dst_buffer, context, &dst_addr);
    dst_addr.addr += dst_offset;

    wined3d_context_copy_bo_address(context, &dst_addr, (const struct wined3d_bo_address *)src_addr, size);
    wined3d_buffer_invalidate_range(dst_buffer, ~dst_location, dst_offset, size);
}

void wined3d_buffer_copy(struct wined3d_buffer *dst_buffer, unsigned int dst_offset,
        struct wined3d_buffer *src_buffer, unsigned int src_offset, unsigned int size)
{
    struct wined3d_context *context;
    struct wined3d_bo_address src;

    TRACE("dst_buffer %p, dst_offset %u, src_buffer %p, src_offset %u, size %u.\n",
            dst_buffer, dst_offset, src_buffer, src_offset, size);

    context = context_acquire(dst_buffer->resource.device, NULL, 0);

    wined3d_buffer_get_memory(src_buffer, context, &src);
    src.addr += src_offset;

    wined3d_buffer_copy_bo_address(dst_buffer, context, dst_offset, wined3d_const_bo_address(&src), size);

    context_release(context);
}

static void wined3d_buffer_init_data(struct wined3d_buffer *buffer,
        struct wined3d_device *device, const struct wined3d_sub_resource_data *data)
{
    struct wined3d_resource *resource = &buffer->resource;
    struct wined3d_box box;

    if (buffer->flags & WINED3D_BUFFER_USE_BO)
    {
        wined3d_box_set(&box, 0, 0, resource->size, 1, 0, 1);
        wined3d_device_context_emit_update_sub_resource(&device->cs->c, resource,
                0, &box, data->data, data->row_pitch, data->slice_pitch);
    }
    else
    {
        memcpy(buffer->resource.heap_memory, data->data, resource->size);
        wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_SYSMEM);
        wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_SYSMEM);
    }
}

static ULONG buffer_resource_incref(struct wined3d_resource *resource)
{
    return wined3d_buffer_incref(buffer_from_resource(resource));
}

static ULONG buffer_resource_decref(struct wined3d_resource *resource)
{
    return wined3d_buffer_decref(buffer_from_resource(resource));
}

static void buffer_resource_preload(struct wined3d_resource *resource)
{
    struct wined3d_context *context;

    context = context_acquire(resource->device, NULL, 0);
    wined3d_buffer_load(buffer_from_resource(resource), context, NULL);
    context_release(context);
}

static const struct wined3d_resource_ops buffer_resource_ops =
{
    buffer_resource_incref,
    buffer_resource_decref,
    buffer_resource_preload,
    buffer_resource_unload,
    buffer_resource_sub_resource_get_desc,
    buffer_resource_sub_resource_get_map_pitch,
    buffer_resource_sub_resource_map,
    buffer_resource_sub_resource_unmap,
};

GLenum wined3d_buffer_gl_binding_from_bind_flags(const struct wined3d_gl_info *gl_info, uint32_t bind_flags)
{
    if (!bind_flags)
        return GL_PIXEL_UNPACK_BUFFER;

    if (bind_flags == WINED3D_BIND_INDEX_BUFFER)
        return GL_ELEMENT_ARRAY_BUFFER;

    if (bind_flags & (WINED3D_BIND_SHADER_RESOURCE | WINED3D_BIND_UNORDERED_ACCESS)
            && gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
        return GL_TEXTURE_BUFFER;

    if (bind_flags & WINED3D_BIND_CONSTANT_BUFFER)
        return GL_UNIFORM_BUFFER;

    if (bind_flags & WINED3D_BIND_STREAM_OUTPUT)
        return GL_TRANSFORM_FEEDBACK_BUFFER;

    if (bind_flags & WINED3D_BIND_INDIRECT_BUFFER
            && gl_info->supported[ARB_DRAW_INDIRECT])
        return GL_DRAW_INDIRECT_BUFFER;

    if (bind_flags & ~(WINED3D_BIND_VERTEX_BUFFER | WINED3D_BIND_INDEX_BUFFER))
        FIXME("Unhandled bind flags %#x.\n", bind_flags);

    return GL_ARRAY_BUFFER;
}

static HRESULT wined3d_buffer_init(struct wined3d_buffer *buffer, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops, const struct wined3d_buffer_ops *buffer_ops)
{
    const struct wined3d_format *format = wined3d_get_format(device->adapter, WINED3DFMT_R8_UNORM, desc->bind_flags);
    struct wined3d_resource *resource = &buffer->resource;
    HRESULT hr;

    TRACE("buffer %p, device %p, desc byte_width %u, usage %s, bind_flags %s, "
            "access %s, data %p, parent %p, parent_ops %p.\n",
            buffer, device, desc->byte_width, debug_d3dusage(desc->usage), wined3d_debug_bind_flags(desc->bind_flags),
            wined3d_debug_resource_access(desc->access), data, parent, parent_ops);

    if (!desc->byte_width)
    {
        WARN("Size 0 requested, returning E_INVALIDARG.\n");
        return E_INVALIDARG;
    }

    if (desc->bind_flags & WINED3D_BIND_CONSTANT_BUFFER && desc->byte_width & (WINED3D_CONSTANT_BUFFER_ALIGNMENT - 1))
    {
        WARN("Size %#x is not suitably aligned for constant buffers.\n", desc->byte_width);
        return E_INVALIDARG;
    }

    if (data && !data->data)
    {
        WARN("Invalid sub-resource data specified.\n");
        return E_INVALIDARG;
    }

    if (FAILED(hr = resource_init(resource, device, WINED3D_RTYPE_BUFFER, format,
            WINED3D_MULTISAMPLE_NONE, 0, desc->usage, desc->bind_flags, desc->access,
            desc->byte_width, 1, 1, desc->byte_width, parent, parent_ops, &buffer_resource_ops)))
    {
        WARN("Failed to initialize resource, hr %#x.\n", hr);
        return hr;
    }
    buffer->buffer_ops = buffer_ops;
    buffer->structure_byte_stride = desc->structure_byte_stride;
    buffer->locations = data ? WINED3D_LOCATION_DISCARDED : WINED3D_LOCATION_SYSMEM;

    TRACE("buffer %p, size %#x, usage %#x, memory @ %p.\n",
            buffer, buffer->resource.size, buffer->resource.usage, buffer->resource.heap_memory);

    if (device->create_parms.flags & WINED3DCREATE_SOFTWARE_VERTEXPROCESSING
            || wined3d_resource_access_is_managed(desc->access))
    {
        /* SWvp and managed buffers always return the same pointer in buffer
         * maps and retain data in DISCARD maps. Keep a system memory copy of
         * the buffer to provide the same behavior to the application. */
        TRACE("Pinning system memory.\n");
        buffer->flags |= WINED3D_BUFFER_PIN_SYSMEM;
        buffer->locations = WINED3D_LOCATION_SYSMEM;
    }

    if (buffer->locations & WINED3D_LOCATION_SYSMEM || !(buffer->flags & WINED3D_BUFFER_USE_BO))
    {
        if (!wined3d_resource_prepare_sysmem(&buffer->resource))
            return E_OUTOFMEMORY;
    }

    if (!(buffer->maps = heap_alloc(sizeof(*buffer->maps))))
    {
        ERR("Out of memory.\n");
        buffer_resource_unload(resource);
        resource_cleanup(resource);
        wined3d_resource_wait_idle(resource);
        return E_OUTOFMEMORY;
    }
    buffer->maps_size = 1;

    if (data)
        wined3d_buffer_init_data(buffer, device, data);

    return WINED3D_OK;
}

static BOOL wined3d_buffer_no3d_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    if (location == WINED3D_LOCATION_SYSMEM)
        return wined3d_resource_prepare_sysmem(&buffer->resource);

    FIXME("Unhandled location %s.\n", wined3d_debug_location(location));

    return FALSE;
}

static void wined3d_buffer_no3d_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    TRACE("buffer %p, context %p, location %s.\n", buffer, context, wined3d_debug_location(location));
}

static void wined3d_buffer_no3d_upload_ranges(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const void *data, unsigned int data_offset, unsigned int range_count, const struct wined3d_range *ranges)
{
    FIXME("Not implemented.\n");
}

static void wined3d_buffer_no3d_download_ranges(struct wined3d_buffer *buffer, struct wined3d_context *context,
        void *data, unsigned int data_offset, unsigned int range_count, const struct wined3d_range *ranges)
{
    FIXME("Not implemented.\n");
}

static const struct wined3d_buffer_ops wined3d_buffer_no3d_ops =
{
    wined3d_buffer_no3d_prepare_location,
    wined3d_buffer_no3d_unload_location,
    wined3d_buffer_no3d_upload_ranges,
    wined3d_buffer_no3d_download_ranges,
};

HRESULT wined3d_buffer_no3d_init(struct wined3d_buffer *buffer_no3d, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("buffer_no3d %p, device %p, desc %p, data %p, parent %p, parent_ops %p.\n",
            buffer_no3d, device, desc, data, parent, parent_ops);

    return wined3d_buffer_init(buffer_no3d, device, desc, data, parent, parent_ops, &wined3d_buffer_no3d_ops);
}

static BOOL wined3d_buffer_gl_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_buffer_gl *buffer_gl = wined3d_buffer_gl(buffer);

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return wined3d_resource_prepare_sysmem(&buffer->resource);

        case WINED3D_LOCATION_BUFFER:
            if (buffer->buffer_object)
                return TRUE;

            if (!(buffer->flags & WINED3D_BUFFER_USE_BO))
            {
                WARN("Trying to create BO for buffer %p with no WINED3D_BUFFER_USE_BO.\n", buffer);
                return FALSE;
            }
            return wined3d_buffer_gl_create_buffer_object(buffer_gl, context_gl);

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static void wined3d_buffer_gl_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    TRACE("buffer %p, context %p, location %s.\n", buffer, context, wined3d_debug_location(location));

    switch (location)
    {
        case WINED3D_LOCATION_BUFFER:
            wined3d_buffer_gl_destroy_buffer_object(wined3d_buffer_gl(buffer), wined3d_context_gl(context));
            break;

        default:
            ERR("Unhandled location %s.\n", wined3d_debug_location(location));
            break;
    }
}

/* Context activation is done by the caller. */
static void wined3d_buffer_gl_upload_ranges(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const void *data, unsigned int data_offset, unsigned int range_count, const struct wined3d_range *ranges)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_buffer_gl *buffer_gl = wined3d_buffer_gl(buffer);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_range *range;

    TRACE("buffer %p, context %p, data %p, data_offset %u, range_count %u, ranges %p.\n",
            buffer, context, data, data_offset, range_count, ranges);

    wined3d_buffer_gl_bind(buffer_gl, context_gl);

    while (range_count--)
    {
        range = &ranges[range_count];
        GL_EXTCALL(glBufferSubData(buffer_gl->bo.binding,
                range->offset, range->size, (BYTE *)data + range->offset - data_offset));
    }
    wined3d_context_gl_reference_bo(context_gl, &buffer_gl->bo);
    checkGLcall("buffer upload");
}

/* Context activation is done by the caller. */
static void wined3d_buffer_gl_download_ranges(struct wined3d_buffer *buffer, struct wined3d_context *context,
        void *data, unsigned int data_offset, unsigned int range_count, const struct wined3d_range *ranges)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_buffer_gl *buffer_gl = wined3d_buffer_gl(buffer);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_range *range;

    TRACE("buffer %p, context %p, data %p, data_offset %u, range_count %u, ranges %p.\n",
            buffer, context, data, data_offset, range_count, ranges);

    wined3d_buffer_gl_bind(buffer_gl, context_gl);

    while (range_count--)
    {
        range = &ranges[range_count];
        GL_EXTCALL(glGetBufferSubData(buffer_gl->bo.binding,
                range->offset, range->size, (BYTE *)data + range->offset - data_offset));
    }
    checkGLcall("buffer download");
}

static const struct wined3d_buffer_ops wined3d_buffer_gl_ops =
{
    wined3d_buffer_gl_prepare_location,
    wined3d_buffer_gl_unload_location,
    wined3d_buffer_gl_upload_ranges,
    wined3d_buffer_gl_download_ranges,
};

HRESULT wined3d_buffer_gl_init(struct wined3d_buffer_gl *buffer_gl, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    TRACE("buffer_gl %p, device %p, desc %p, data %p, parent %p, parent_ops %p.\n",
            buffer_gl, device, desc, data, parent, parent_ops);

    /* Observations show that draw_primitive_immediate_mode() is faster on
     * dynamic vertex buffers than converting + draw_primitive_arrays().
     * (Half-Life 2 and others.) */
    if (!(desc->access & WINED3D_RESOURCE_ACCESS_GPU))
        TRACE("Not creating a BO because the buffer is not GPU accessible.\n");
    else if (!gl_info->supported[ARB_VERTEX_BUFFER_OBJECT])
        TRACE("Not creating a BO because GL_ARB_vertex_buffer is not supported.\n");
    else if (!(gl_info->supported[APPLE_FLUSH_BUFFER_RANGE] || gl_info->supported[ARB_MAP_BUFFER_RANGE])
            && (desc->usage & WINED3DUSAGE_DYNAMIC))
        TRACE("Not creating a BO because the buffer has dynamic usage and no GL support.\n");
    else
        buffer_gl->b.flags |= WINED3D_BUFFER_USE_BO;

    return wined3d_buffer_init(&buffer_gl->b, device, desc, data, parent, parent_ops, &wined3d_buffer_gl_ops);
}

static BOOL wined3d_buffer_vk_create_buffer_object(struct wined3d_buffer_vk *buffer_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_resource *resource = &buffer_vk->b.resource;
    uint32_t bind_flags = resource->bind_flags;
    VkMemoryPropertyFlags memory_type;
    VkBufferUsageFlags usage;

    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (bind_flags & WINED3D_BIND_VERTEX_BUFFER)
        usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_INDEX_BUFFER)
        usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_CONSTANT_BUFFER)
        usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_SHADER_RESOURCE)
        usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_STREAM_OUTPUT)
        usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    if (bind_flags & WINED3D_BIND_UNORDERED_ACCESS)
        usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_INDIRECT_BUFFER)
        usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (bind_flags & (WINED3D_BIND_RENDER_TARGET | WINED3D_BIND_DEPTH_STENCIL))
        FIXME("Ignoring some bind flags %#x.\n", bind_flags);

    memory_type = 0;
    if (resource->access & WINED3D_RESOURCE_ACCESS_MAP_R)
        memory_type |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    else if (resource->access & WINED3D_RESOURCE_ACCESS_MAP_W)
        memory_type |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    else if (!(resource->usage & WINED3DUSAGE_DYNAMIC))
        memory_type |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (!(wined3d_context_vk_create_bo(context_vk, resource->size, usage, memory_type, &buffer_vk->bo)))
    {
        WARN("Failed to create Vulkan buffer.\n");
        return FALSE;
    }

    list_init(&buffer_vk->bo_user.entry);
    list_add_head(&buffer_vk->bo.users, &buffer_vk->bo_user.entry);
    buffer_vk->b.buffer_object = (uintptr_t)&buffer_vk->bo;
    buffer_invalidate_bo_range(&buffer_vk->b, 0, 0);

    return TRUE;
}

const VkDescriptorBufferInfo *wined3d_buffer_vk_get_buffer_info(struct wined3d_buffer_vk *buffer_vk)
{
    if (buffer_vk->bo_user.valid)
        return &buffer_vk->buffer_info;

    buffer_vk->buffer_info.buffer = buffer_vk->bo.vk_buffer;
    buffer_vk->buffer_info.offset = buffer_vk->bo.buffer_offset;
    buffer_vk->buffer_info.range = buffer_vk->b.resource.size;
    buffer_vk->bo_user.valid = true;

    return &buffer_vk->buffer_info;
}

static BOOL wined3d_buffer_vk_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return wined3d_resource_prepare_sysmem(&buffer->resource);

        case WINED3D_LOCATION_BUFFER:
            if (buffer->buffer_object)
                return TRUE;

            return wined3d_buffer_vk_create_buffer_object(wined3d_buffer_vk(buffer), wined3d_context_vk(context));

        default:
            FIXME("Unhandled location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static void wined3d_buffer_vk_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    struct wined3d_buffer_vk *buffer_vk = wined3d_buffer_vk(buffer);

    TRACE("buffer %p, context %p, location %s.\n", buffer, context, wined3d_debug_location(location));

    switch (location)
    {
        case WINED3D_LOCATION_BUFFER:
            buffer_vk->bo_user.valid = false;
            list_remove(&buffer_vk->bo_user.entry);
            wined3d_context_vk_destroy_bo(context_vk, &buffer_vk->bo);
            buffer_vk->bo.vk_buffer = VK_NULL_HANDLE;
            buffer_vk->bo.memory = NULL;
            buffer_vk->b.buffer_object = 0u;
            break;

        default:
            ERR("Unhandled location %s.\n", wined3d_debug_location(location));
            break;
    }
}

static void wined3d_buffer_vk_upload_ranges(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const void *data, unsigned int data_offset, unsigned int range_count, const struct wined3d_range *ranges)
{
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    struct wined3d_resource *resource = &buffer->resource;
    struct wined3d_bo_address src, dst;
    const struct wined3d_range *range;
    struct wined3d_bo_vk *dst_bo;
    unsigned int i = range_count;
    uint32_t flags;
    void *map_ptr;

    if (!range_count)
        return;

    dst.buffer_object = buffer->buffer_object;
    dst.addr = NULL;

    flags = WINED3D_MAP_WRITE;
    if (!ranges->offset && ranges->size == resource->size)
        flags |= WINED3D_MAP_DISCARD;

    dst_bo = &wined3d_buffer_vk(buffer)->bo;
    if (!(dst_bo->memory_type & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) || (!(flags & WINED3D_MAP_DISCARD)
            && dst_bo->command_buffer_id > context_vk->completed_command_buffer_id))
    {
        src.buffer_object = 0;
        while (range_count--)
        {
            range = &ranges[range_count];

            src.addr = (uint8_t *)data + range->offset - data_offset;
            dst.addr = (void *)(uintptr_t)range->offset;
            wined3d_context_copy_bo_address(context, &dst, &src, range->size);
        }

        return;
    }

    if (!(map_ptr = wined3d_context_map_bo_address(context, &dst, resource->size, flags)))
    {
        FIXME("Failed to map buffer.\n");
        return;
    }

    while (i--)
    {
        range = &ranges[i];
        memcpy((uint8_t *)map_ptr + range->offset, (uint8_t *)data + range->offset - data_offset, range->size);
    }

    wined3d_context_unmap_bo_address(context, &dst, range_count, ranges);
}

static void wined3d_buffer_vk_download_ranges(struct wined3d_buffer *buffer, struct wined3d_context *context,
        void *data, unsigned int data_offset, unsigned int range_count, const struct wined3d_range *ranges)
{
    FIXME("Not implemented.\n");
}

static const struct wined3d_buffer_ops wined3d_buffer_vk_ops =
{
    wined3d_buffer_vk_prepare_location,
    wined3d_buffer_vk_unload_location,
    wined3d_buffer_vk_upload_ranges,
    wined3d_buffer_vk_download_ranges,
};

HRESULT wined3d_buffer_vk_init(struct wined3d_buffer_vk *buffer_vk, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_vk_info *vk_info = &wined3d_adapter_vk(device->adapter)->vk_info;

    TRACE("buffer_vk %p, device %p, desc %p, data %p, parent %p, parent_ops %p.\n",
            buffer_vk, device, desc, data, parent, parent_ops);

    if ((desc->bind_flags & WINED3D_BIND_STREAM_OUTPUT)
            && !vk_info->supported[WINED3D_VK_EXT_TRANSFORM_FEEDBACK])
    {
        WARN("The Vulkan implementation does not support transform feedback.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->access & WINED3D_RESOURCE_ACCESS_GPU)
        buffer_vk->b.flags |= WINED3D_BUFFER_USE_BO;

    return wined3d_buffer_init(&buffer_vk->b, device, desc, data, parent, parent_ops, &wined3d_buffer_vk_ops);
}

void wined3d_buffer_vk_barrier(struct wined3d_buffer_vk *buffer_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask)
{
    TRACE("buffer_vk %p, context_vk %p, bind_mask %s.\n",
            buffer_vk, context_vk, wined3d_debug_bind_flags(bind_mask));

    if (buffer_vk->bind_mask && buffer_vk->bind_mask != bind_mask)
    {
        const struct wined3d_vk_info *vk_info = context_vk->vk_info;
        VkBufferMemoryBarrier vk_barrier;

        TRACE("    %s -> %s.\n",
                wined3d_debug_bind_flags(buffer_vk->bind_mask), wined3d_debug_bind_flags(bind_mask));

        wined3d_context_vk_end_current_render_pass(context_vk);

        vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vk_barrier.pNext = NULL;
        vk_barrier.srcAccessMask = vk_access_mask_from_bind_flags(buffer_vk->bind_mask);
        vk_barrier.dstAccessMask = vk_access_mask_from_bind_flags(bind_mask);
        vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.buffer = buffer_vk->bo.vk_buffer;
        vk_barrier.offset = buffer_vk->bo.buffer_offset;
        vk_barrier.size = buffer_vk->b.resource.size;
        VK_CALL(vkCmdPipelineBarrier(wined3d_context_vk_get_command_buffer(context_vk),
                vk_pipeline_stage_mask_from_bind_flags(buffer_vk->bind_mask),
                vk_pipeline_stage_mask_from_bind_flags(bind_mask),
                0, 0, NULL, 1, &vk_barrier, 0, NULL));
    }
    buffer_vk->bind_mask = bind_mask;
}

HRESULT CDECL wined3d_buffer_create(struct wined3d_device *device, const struct wined3d_buffer_desc *desc,
        const struct wined3d_sub_resource_data *data, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_buffer **buffer)
{
    TRACE("device %p, desc %p, data %p, parent %p, parent_ops %p, buffer %p.\n",
            device, desc, data, parent, parent_ops, buffer);

    return device->adapter->adapter_ops->adapter_create_buffer(device, desc, data, parent, parent_ops, buffer);
}
