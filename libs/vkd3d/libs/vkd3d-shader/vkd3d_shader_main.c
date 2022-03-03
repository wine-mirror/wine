/*
 * Copyright 2017 Józef Kucia for CodeWeavers
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

#include "vkd3d_shader_private.h"
#include "vkd3d_version.h"

#include <stdio.h>
#include <math.h>

void vkd3d_string_buffer_init(struct vkd3d_string_buffer *buffer)
{
    buffer->buffer_size = 16;
    buffer->content_size = 0;
    buffer->buffer = vkd3d_malloc(buffer->buffer_size);
    assert(buffer->buffer);
    memset(buffer->buffer, 0, buffer->buffer_size);
}

void vkd3d_string_buffer_cleanup(struct vkd3d_string_buffer *buffer)
{
    vkd3d_free(buffer->buffer);
}

static void vkd3d_string_buffer_clear(struct vkd3d_string_buffer *buffer)
{
    buffer->buffer[0] = '\0';
    buffer->content_size = 0;
}

static bool vkd3d_string_buffer_resize(struct vkd3d_string_buffer *buffer, int rc)
{
    unsigned int new_buffer_size = rc >= 0 ? buffer->content_size + rc + 1 : buffer->buffer_size * 2;

    if (!vkd3d_array_reserve((void **)&buffer->buffer, &buffer->buffer_size, new_buffer_size, 1))
    {
        ERR("Failed to grow buffer.\n");
        buffer->buffer[buffer->content_size] = '\0';
        return false;
    }
    return true;
}

int vkd3d_string_buffer_vprintf(struct vkd3d_string_buffer *buffer, const char *format, va_list args)
{
    unsigned int rem;
    va_list a;
    int rc;

    for (;;)
    {
        rem = buffer->buffer_size - buffer->content_size;
        va_copy(a, args);
        rc = vsnprintf(&buffer->buffer[buffer->content_size], rem, format, a);
        va_end(a);
        if (rc >= 0 && (unsigned int)rc < rem)
        {
            buffer->content_size += rc;
            return 0;
        }

        if (!vkd3d_string_buffer_resize(buffer, rc))
            return -1;
    }
}

int vkd3d_string_buffer_printf(struct vkd3d_string_buffer *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vkd3d_string_buffer_vprintf(buffer, format, args);
    va_end(args);

    return ret;
}

int vkd3d_string_buffer_print_f32(struct vkd3d_string_buffer *buffer, float f)
{
    unsigned int idx = buffer->content_size + 1;
    int ret;

    if (!(ret = vkd3d_string_buffer_printf(buffer, "%.8e", f)) && isfinite(f))
    {
        if (signbit(f))
            ++idx;
        buffer->buffer[idx] = '.';
    }

    return ret;
}

int vkd3d_string_buffer_print_f64(struct vkd3d_string_buffer *buffer, double d)
{
    unsigned int idx = buffer->content_size + 1;
    int ret;

    if (!(ret = vkd3d_string_buffer_printf(buffer, "%.16e", d)) && isfinite(d))
    {
        if (signbit(d))
            ++idx;
        buffer->buffer[idx] = '.';
    }

    return ret;
}

void vkd3d_string_buffer_trace_(const struct vkd3d_string_buffer *buffer, const char *function)
{
    vkd3d_shader_trace_text_(buffer->buffer, buffer->content_size, function);
}

void vkd3d_shader_trace_text_(const char *text, size_t size, const char *function)
{
    const char *p, *q, *end = text + size;

    if (!TRACE_ON())
        return;

    for (p = text; p < end; p = q)
    {
        if (!(q = memchr(p, '\n', end - p)))
            q = end;
        else
            ++q;
        vkd3d_dbg_printf(VKD3D_DBG_LEVEL_TRACE, function, "%.*s", (int)(q - p), p);
    }
}

void vkd3d_string_buffer_cache_init(struct vkd3d_string_buffer_cache *cache)
{
    memset(cache, 0, sizeof(*cache));
}

void vkd3d_string_buffer_cache_cleanup(struct vkd3d_string_buffer_cache *cache)
{
    unsigned int i;

    for (i = 0; i < cache->count; ++i)
    {
        vkd3d_string_buffer_cleanup(cache->buffers[i]);
        vkd3d_free(cache->buffers[i]);
    }
    vkd3d_free(cache->buffers);
    vkd3d_string_buffer_cache_init(cache);
}

struct vkd3d_string_buffer *vkd3d_string_buffer_get(struct vkd3d_string_buffer_cache *cache)
{
    struct vkd3d_string_buffer *buffer;

    if (!cache->count)
    {
        if (!vkd3d_array_reserve((void **)&cache->buffers, &cache->capacity,
                cache->max_count + 1, sizeof(*cache->buffers)))
            return NULL;
        ++cache->max_count;

        if (!(buffer = vkd3d_malloc(sizeof(*buffer))))
            return NULL;
        vkd3d_string_buffer_init(buffer);
    }
    else
    {
        buffer = cache->buffers[--cache->count];
    }
    vkd3d_string_buffer_clear(buffer);
    return buffer;
}

void vkd3d_string_buffer_release(struct vkd3d_string_buffer_cache *cache, struct vkd3d_string_buffer *buffer)
{
    if (!buffer)
        return;
    assert(cache->count + 1 <= cache->max_count);
    cache->buffers[cache->count++] = buffer;
}

void vkd3d_shader_message_context_init(struct vkd3d_shader_message_context *context,
        enum vkd3d_shader_log_level log_level)
{
    context->log_level = log_level;
    vkd3d_string_buffer_init(&context->messages);
}

void vkd3d_shader_message_context_cleanup(struct vkd3d_shader_message_context *context)
{
    vkd3d_string_buffer_cleanup(&context->messages);
}

void vkd3d_shader_message_context_trace_messages_(const struct vkd3d_shader_message_context *context,
        const char *function)
{
    vkd3d_string_buffer_trace_(&context->messages, function);
}

bool vkd3d_shader_message_context_copy_messages(struct vkd3d_shader_message_context *context, char **out)
{
    char *messages;

    if (!out)
        return true;

    *out = NULL;

    if (!context->messages.content_size)
        return true;

    if (!(messages = vkd3d_malloc(context->messages.content_size + 1)))
        return false;
    memcpy(messages, context->messages.buffer, context->messages.content_size + 1);
    *out = messages;
    return true;
}

void vkd3d_shader_vnote(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_log_level level, const char *format, va_list args)
{
    if (context->log_level < level)
        return;

    if (location)
    {
        const char *source_name = location->source_name ? location->source_name : "<anonymous>";

        if (location->line)
            vkd3d_string_buffer_printf(&context->messages, "%s:%u:%u: ",
                    source_name, location->line, location->column);
        else
            vkd3d_string_buffer_printf(&context->messages, "%s: ", source_name);
    }
    vkd3d_string_buffer_vprintf(&context->messages, format, args);
    vkd3d_string_buffer_printf(&context->messages, "\n");
}

void vkd3d_shader_vwarning(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_error error, const char *format, va_list args)
{
    if (context->log_level < VKD3D_SHADER_LOG_WARNING)
        return;

    if (location)
    {
        const char *source_name = location->source_name ? location->source_name : "<anonymous>";

        if (location->line)
            vkd3d_string_buffer_printf(&context->messages, "%s:%u:%u: W%04u: ",
                    source_name, location->line, location->column, error);
        else
            vkd3d_string_buffer_printf(&context->messages, "%s: W%04u: ", source_name, error);
    }
    else
    {
        vkd3d_string_buffer_printf(&context->messages, "W%04u: ", error);
    }
    vkd3d_string_buffer_vprintf(&context->messages, format, args);
    vkd3d_string_buffer_printf(&context->messages, "\n");
}

void vkd3d_shader_verror(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_error error, const char *format, va_list args)
{
    if (context->log_level < VKD3D_SHADER_LOG_ERROR)
        return;

    if (location)
    {
        const char *source_name = location->source_name ? location->source_name : "<anonymous>";

        if (location->line)
            vkd3d_string_buffer_printf(&context->messages, "%s:%u:%u: E%04u: ",
                    source_name, location->line, location->column, error);
        else
            vkd3d_string_buffer_printf(&context->messages, "%s: E%04u: ", source_name, error);
    }
    else
    {
        vkd3d_string_buffer_printf(&context->messages, "E%04u: ", error);
    }
    vkd3d_string_buffer_vprintf(&context->messages, format, args);
    vkd3d_string_buffer_printf(&context->messages, "\n");
}

void vkd3d_shader_error(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(context, location, error, format, args);
    va_end(args);
}

size_t bytecode_put_bytes(struct vkd3d_bytecode_buffer *buffer, const void *bytes, size_t size)
{
    size_t aligned_size = align(size, 4);
    size_t offset = buffer->size;

    if (buffer->status)
        return offset;

    if (!vkd3d_array_reserve((void **)&buffer->data, &buffer->capacity, offset + aligned_size, 1))
    {
        buffer->status = VKD3D_ERROR_OUT_OF_MEMORY;
        return offset;
    }
    memcpy(buffer->data + offset, bytes, size);
    memset(buffer->data + offset + size, 0xab, aligned_size - size);
    buffer->size = offset + aligned_size;
    return offset;
}

void set_u32(struct vkd3d_bytecode_buffer *buffer, size_t offset, uint32_t value)
{
    if (buffer->status)
        return;

    assert(vkd3d_bound_range(offset, sizeof(value), buffer->size));
    memcpy(buffer->data + offset, &value, sizeof(value));
}

static void vkd3d_shader_dump_blob(const char *path, const char *prefix,
        const char *suffix, const void *data, size_t size)
{
    static LONG shader_id = 0;
    char filename[1024];
    unsigned int id;
    FILE *f;

    id = InterlockedIncrement(&shader_id) - 1;

    snprintf(filename, ARRAY_SIZE(filename), "%s/vkd3d-shader-%s-%u.%s", path, prefix, id, suffix);
    if ((f = fopen(filename, "wb")))
    {
        if (fwrite(data, 1, size, f) != size)
            ERR("Failed to write shader to %s.\n", filename);
        if (fclose(f))
            ERR("Failed to close stream %s.\n", filename);
    }
    else
    {
        ERR("Failed to open %s for dumping shader.\n", filename);
    }
}

static const char *shader_get_source_type_suffix(enum vkd3d_shader_source_type type)
{
    switch (type)
    {
        case VKD3D_SHADER_SOURCE_DXBC_TPF:
            return "dxbc";
        case VKD3D_SHADER_SOURCE_HLSL:
            return "hlsl";
        case VKD3D_SHADER_SOURCE_D3D_BYTECODE:
            return "d3dbc";
        default:
            FIXME("Unhandled source type %#x.\n", type);
            return "bin";
    }
}

void vkd3d_shader_dump_shader(enum vkd3d_shader_source_type source_type,
        enum vkd3d_shader_type shader_type, const struct vkd3d_shader_code *shader)
{
    static bool enabled = true;
    const char *path;

    if (!enabled)
        return;

    if (!(path = getenv("VKD3D_SHADER_DUMP_PATH")))
    {
        enabled = false;
        return;
    }

    vkd3d_shader_dump_blob(path, shader_get_type_prefix(shader_type),
            shader_get_source_type_suffix(source_type), shader->code, shader->size);
}

void vkd3d_shader_parser_init(struct vkd3d_shader_parser *parser,
        struct vkd3d_shader_message_context *message_context, const char *source_name,
        const struct vkd3d_shader_version *version, const struct vkd3d_shader_parser_ops *ops)
{
    parser->message_context = message_context;
    parser->location.source_name = source_name;
    parser->location.line = 1;
    parser->location.column = 0;
    parser->shader_version = *version;
    parser->ops = ops;
}

void VKD3D_PRINTF_FUNC(3, 4) vkd3d_shader_parser_error(struct vkd3d_shader_parser *parser,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(parser->message_context, &parser->location, error, format, args);
    va_end(args);

    parser->failed = true;
}

void VKD3D_PRINTF_FUNC(3, 4) vkd3d_shader_parser_warning(struct vkd3d_shader_parser *parser,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_vwarning(parser->message_context, &parser->location, error, format, args);
    va_end(args);
}

static int vkd3d_shader_validate_compile_info(const struct vkd3d_shader_compile_info *compile_info,
        bool validate_target_type)
{
    const enum vkd3d_shader_source_type *source_types;
    const enum vkd3d_shader_target_type *target_types;
    unsigned int count, i;

    if (compile_info->type != VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO)
    {
        WARN("Invalid structure type %#x.\n", compile_info->type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    source_types = vkd3d_shader_get_supported_source_types(&count);
    for (i = 0; i < count; ++i)
    {
        if (source_types[i] == compile_info->source_type)
            break;
    }
    if (i == count)
    {
        WARN("Invalid shader source type %#x.\n", compile_info->source_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (validate_target_type)
    {
        target_types = vkd3d_shader_get_supported_target_types(compile_info->source_type, &count);
        for (i = 0; i < count; ++i)
        {
            if (target_types[i] == compile_info->target_type)
                break;
        }
        if (i == count)
        {
            WARN("Invalid shader target type %#x.\n", compile_info->target_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    return VKD3D_OK;
}

void vkd3d_shader_free_messages(char *messages)
{
    TRACE("messages %p.\n", messages);

    vkd3d_free(messages);
}

struct vkd3d_shader_scan_context
{
    struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info;
    size_t descriptors_size;

    struct vkd3d_shader_message_context *message_context;
    struct vkd3d_shader_location location;

    struct vkd3d_shader_cf_info
    {
        enum
        {
            VKD3D_SHADER_BLOCK_IF,
            VKD3D_SHADER_BLOCK_LOOP,
            VKD3D_SHADER_BLOCK_SWITCH,
        } type;
        bool inside_block;
        bool has_default;
    } *cf_info;
    size_t cf_info_size;
    size_t cf_info_count;

    struct
    {
        unsigned int id;
        unsigned int descriptor_idx;
    } *uav_ranges;
    size_t uav_ranges_size;
    size_t uav_range_count;

    enum vkd3d_shader_api_version api_version;
};

static void vkd3d_shader_scan_context_init(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info,
        struct vkd3d_shader_message_context *message_context)
{
    unsigned int i;

    memset(context, 0, sizeof(*context));
    context->scan_descriptor_info = scan_descriptor_info;
    context->message_context = message_context;
    context->location.source_name = compile_info->source_name;
    context->location.line = 2; /* Line 1 is the version token. */
    context->api_version = VKD3D_SHADER_API_VERSION_1_2;

    for (i = 0; i < compile_info->option_count; ++i)
    {
        const struct vkd3d_shader_compile_option *option = &compile_info->options[i];

        if (option->name == VKD3D_SHADER_COMPILE_OPTION_API_VERSION)
            context->api_version = option->value;
    }
}

static void vkd3d_shader_scan_context_cleanup(struct vkd3d_shader_scan_context *context)
{
    vkd3d_free(context->uav_ranges);
    vkd3d_free(context->cf_info);
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_get_current_cf_info(struct vkd3d_shader_scan_context *context)
{
    if (!context->cf_info_count)
        return NULL;
    return &context->cf_info[context->cf_info_count - 1];
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_push_cf_info(struct vkd3d_shader_scan_context *context)
{
    struct vkd3d_shader_cf_info *cf_info;

    if (!vkd3d_array_reserve((void **)&context->cf_info, &context->cf_info_size,
            context->cf_info_count + 1, sizeof(*context->cf_info)))
    {
        ERR("Failed to allocate UAV range.\n");
        return false;
    }

    cf_info = &context->cf_info[context->cf_info_count++];
    memset(cf_info, 0, sizeof(*cf_info));

    return cf_info;
}

static void vkd3d_shader_scan_pop_cf_info(struct vkd3d_shader_scan_context *context)
{
    assert(context->cf_info_count);

    --context->cf_info_count;
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_find_innermost_breakable_cf_info(
        struct vkd3d_shader_scan_context *context)
{
    size_t count = context->cf_info_count;
    struct vkd3d_shader_cf_info *cf_info;

    while (count)
    {
        cf_info = &context->cf_info[--count];
        if (cf_info->type == VKD3D_SHADER_BLOCK_LOOP
                || cf_info->type == VKD3D_SHADER_BLOCK_SWITCH)
            return cf_info;
    }

    return NULL;
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_find_innermost_loop_cf_info(
        struct vkd3d_shader_scan_context *context)
{
    size_t count = context->cf_info_count;
    struct vkd3d_shader_cf_info *cf_info;

    while (count)
    {
        cf_info = &context->cf_info[--count];
        if (cf_info->type == VKD3D_SHADER_BLOCK_LOOP)
            return cf_info;
    }

    return NULL;
}

static struct vkd3d_shader_descriptor_info *vkd3d_shader_scan_get_uav_descriptor_info(
        const struct vkd3d_shader_scan_context *context, unsigned int range_id)
{
    unsigned int i;

    for (i = 0; i < context->uav_range_count; ++i)
    {
        if (context->uav_ranges[i].id == range_id)
            return &context->scan_descriptor_info->descriptors[context->uav_ranges[i].descriptor_idx];
    }

    return NULL;
}

static bool vkd3d_shader_instruction_is_uav_read(const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_opcode handler_idx = instruction->handler_idx;
    return (VKD3DSIH_ATOMIC_AND <= handler_idx && handler_idx <= VKD3DSIH_ATOMIC_XOR)
            || (VKD3DSIH_IMM_ATOMIC_ALLOC <= handler_idx && handler_idx <= VKD3DSIH_IMM_ATOMIC_XOR)
            || handler_idx == VKD3DSIH_LD_UAV_TYPED
            || (handler_idx == VKD3DSIH_LD_RAW && instruction->src[1].reg.type == VKD3DSPR_UAV)
            || (handler_idx == VKD3DSIH_LD_STRUCTURED && instruction->src[2].reg.type == VKD3DSPR_UAV);
}

static void vkd3d_shader_scan_record_uav_read(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_register *reg)
{
    struct vkd3d_shader_descriptor_info *d;

    if (!context->scan_descriptor_info)
        return;

    d = vkd3d_shader_scan_get_uav_descriptor_info(context, reg->idx[0].offset);
    d->flags |= VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_READ;
}

static bool vkd3d_shader_instruction_is_uav_counter(const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_opcode handler_idx = instruction->handler_idx;
    return handler_idx == VKD3DSIH_IMM_ATOMIC_ALLOC
            || handler_idx == VKD3DSIH_IMM_ATOMIC_CONSUME;
}

static void vkd3d_shader_scan_record_uav_counter(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_register *reg)
{
    struct vkd3d_shader_descriptor_info *d;

    if (!context->scan_descriptor_info)
        return;

    d = vkd3d_shader_scan_get_uav_descriptor_info(context, reg->idx[0].offset);
    d->flags |= VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER;
}

static bool vkd3d_shader_scan_add_descriptor(struct vkd3d_shader_scan_context *context,
        enum vkd3d_shader_descriptor_type type, const struct vkd3d_shader_register_range *range,
        enum vkd3d_shader_resource_type resource_type, enum vkd3d_shader_resource_data_type resource_data_type,
        unsigned int flags)
{
    struct vkd3d_shader_scan_descriptor_info *info = context->scan_descriptor_info;
    struct vkd3d_shader_descriptor_info *d;

    if (!vkd3d_array_reserve((void **)&info->descriptors, &context->descriptors_size,
            info->descriptor_count + 1, sizeof(*info->descriptors)))
    {
        ERR("Failed to allocate descriptor info.\n");
        return false;
    }

    d = &info->descriptors[info->descriptor_count];
    d->type = type;
    d->register_space = range->space;
    d->register_index = range->first;
    d->resource_type = resource_type;
    d->resource_data_type = resource_data_type;
    d->flags = flags;
    d->count = (range->last == ~0u) ? ~0u : range->last - range->first + 1;
    ++info->descriptor_count;

    return true;
}

static bool vkd3d_shader_scan_add_uav_range(struct vkd3d_shader_scan_context *context,
        unsigned int id, unsigned int descriptor_idx)
{
    if (!vkd3d_array_reserve((void **)&context->uav_ranges, &context->uav_ranges_size,
            context->uav_range_count + 1, sizeof(*context->uav_ranges)))
    {
        ERR("Failed to allocate UAV range.\n");
        return false;
    }

    context->uav_ranges[context->uav_range_count].id = id;
    context->uav_ranges[context->uav_range_count].descriptor_idx = descriptor_idx;
    ++context->uav_range_count;

    return true;
}

static void vkd3d_shader_scan_constant_buffer_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_constant_buffer *cb = &instruction->declaration.cb;

    if (!context->scan_descriptor_info)
        return;

    vkd3d_shader_scan_add_descriptor(context, VKD3D_SHADER_DESCRIPTOR_TYPE_CBV, &cb->range,
            VKD3D_SHADER_RESOURCE_BUFFER, VKD3D_SHADER_RESOURCE_DATA_UINT, 0);
}

static void vkd3d_shader_scan_sampler_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_sampler *sampler = &instruction->declaration.sampler;
    unsigned int flags;

    if (!context->scan_descriptor_info)
        return;

    if (instruction->flags & VKD3DSI_SAMPLER_COMPARISON_MODE)
        flags = VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE;
    else
        flags = 0;
    vkd3d_shader_scan_add_descriptor(context, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, &sampler->range,
            VKD3D_SHADER_RESOURCE_NONE, VKD3D_SHADER_RESOURCE_DATA_UINT, flags);
}

static void vkd3d_shader_scan_resource_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_resource *resource, enum vkd3d_shader_resource_type resource_type,
        enum vkd3d_shader_resource_data_type resource_data_type)
{
    enum vkd3d_shader_descriptor_type type;

    if (!context->scan_descriptor_info)
        return;

    if (resource->reg.reg.type == VKD3DSPR_UAV)
        type = VKD3D_SHADER_DESCRIPTOR_TYPE_UAV;
    else
        type = VKD3D_SHADER_DESCRIPTOR_TYPE_SRV;
    vkd3d_shader_scan_add_descriptor(context, type, &resource->range, resource_type, resource_data_type, 0);
    if (type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV)
        vkd3d_shader_scan_add_uav_range(context, resource->reg.reg.idx[0].offset,
                context->scan_descriptor_info->descriptor_count - 1);
}

static void vkd3d_shader_scan_typed_resource_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_semantic *semantic = &instruction->declaration.semantic;
    enum vkd3d_shader_resource_data_type resource_data_type;

    if (semantic->resource_data_type[0] != semantic->resource_data_type[1] ||
        semantic->resource_data_type[0] != semantic->resource_data_type[2] ||
        semantic->resource_data_type[0] != semantic->resource_data_type[3])
        FIXME("Resource data types are different (%d, %d, %d, %d).\n",
                semantic->resource_data_type[0],
                semantic->resource_data_type[1],
                semantic->resource_data_type[2],
                semantic->resource_data_type[3]);

    switch (semantic->resource_data_type[0])
    {
        case VKD3D_DATA_UNORM:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_UNORM;
            break;
        case VKD3D_DATA_SNORM:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_SNORM;
            break;
        case VKD3D_DATA_INT:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_INT;
            break;
        case VKD3D_DATA_UINT:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_UINT;
            break;
        case VKD3D_DATA_FLOAT:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_FLOAT;
            break;
        case VKD3D_DATA_MIXED:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_MIXED;
            break;
        case VKD3D_DATA_DOUBLE:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_DOUBLE;
            break;
        case VKD3D_DATA_CONTINUED:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_CONTINUED;
            break;
        default:
            ERR("Invalid resource data type %#x.\n", semantic->resource_data_type[0]);
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_FLOAT;
            break;
    }

    if (context->api_version < VKD3D_SHADER_API_VERSION_1_3
            && resource_data_type >= VKD3D_SHADER_RESOURCE_DATA_MIXED)
    {
        ERR("Invalid resource data type %#x for API version %#x.\n",
                semantic->resource_data_type[0], context->api_version);
        resource_data_type = VKD3D_SHADER_RESOURCE_DATA_FLOAT;
    }

    vkd3d_shader_scan_resource_declaration(context, &semantic->resource,
            semantic->resource_type, resource_data_type);
}

static void vkd3d_shader_scan_error(struct vkd3d_shader_scan_context *context,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(context->message_context, &context->location, error, format, args);
    va_end(args);
}

static int vkd3d_shader_scan_instruction(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_shader_cf_info *cf_info;
    unsigned int i;

    switch (instruction->handler_idx)
    {
        case VKD3DSIH_DCL_CONSTANT_BUFFER:
            vkd3d_shader_scan_constant_buffer_declaration(context, instruction);
            break;
        case VKD3DSIH_DCL_SAMPLER:
            vkd3d_shader_scan_sampler_declaration(context, instruction);
            break;
        case VKD3DSIH_DCL:
        case VKD3DSIH_DCL_UAV_TYPED:
            vkd3d_shader_scan_typed_resource_declaration(context, instruction);
            break;
        case VKD3DSIH_DCL_RESOURCE_RAW:
        case VKD3DSIH_DCL_UAV_RAW:
            vkd3d_shader_scan_resource_declaration(context, &instruction->declaration.raw_resource.resource,
                    VKD3D_SHADER_RESOURCE_BUFFER, VKD3D_SHADER_RESOURCE_DATA_UINT);
            break;
        case VKD3DSIH_DCL_RESOURCE_STRUCTURED:
        case VKD3DSIH_DCL_UAV_STRUCTURED:
            vkd3d_shader_scan_resource_declaration(context, &instruction->declaration.structured_resource.resource,
                    VKD3D_SHADER_RESOURCE_BUFFER, VKD3D_SHADER_RESOURCE_DATA_UINT);
            break;
        case VKD3DSIH_IF:
            cf_info = vkd3d_shader_scan_push_cf_info(context);
            cf_info->type = VKD3D_SHADER_BLOCK_IF;
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_ELSE:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context)) || cf_info->type != VKD3D_SHADER_BLOCK_IF)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘else’ instruction without corresponding ‘if’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_ENDIF:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context)) || cf_info->type != VKD3D_SHADER_BLOCK_IF)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘endif’ instruction without corresponding ‘if’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            vkd3d_shader_scan_pop_cf_info(context);
            break;
        case VKD3DSIH_LOOP:
            cf_info = vkd3d_shader_scan_push_cf_info(context);
            cf_info->type = VKD3D_SHADER_BLOCK_LOOP;
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_ENDLOOP:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context)) || cf_info->type != VKD3D_SHADER_BLOCK_LOOP)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘endloop’ instruction without corresponding ‘loop’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            vkd3d_shader_scan_pop_cf_info(context);
            break;
        case VKD3DSIH_SWITCH:
            cf_info = vkd3d_shader_scan_push_cf_info(context);
            cf_info->type = VKD3D_SHADER_BLOCK_SWITCH;
            break;
        case VKD3DSIH_ENDSWITCH:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context))
                    || cf_info->type != VKD3D_SHADER_BLOCK_SWITCH || cf_info->inside_block)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘endswitch’ instruction without corresponding ‘switch’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            vkd3d_shader_scan_pop_cf_info(context);
            break;
        case VKD3DSIH_CASE:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context))
                    || cf_info->type != VKD3D_SHADER_BLOCK_SWITCH)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘case’ instruction outside switch block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_DEFAULT:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context))
                    || cf_info->type != VKD3D_SHADER_BLOCK_SWITCH)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘default’ instruction outside switch block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            if (cf_info->has_default)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered duplicate ‘default’ instruction inside the current switch block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = true;
            cf_info->has_default = true;
            break;
        case VKD3DSIH_BREAK:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_breakable_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘break’ instruction outside breakable block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = false;
            break;
        case VKD3DSIH_BREAKP:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_loop_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘breakp’ instruction outside loop.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            break;
        case VKD3DSIH_CONTINUE:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_loop_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘continue’ instruction outside loop.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = false;
            break;
        case VKD3DSIH_CONTINUEP:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_loop_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘continue’ instruction outside loop.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            break;
        case VKD3DSIH_RET:
            if (context->cf_info_count)
                context->cf_info[context->cf_info_count - 1].inside_block = false;
            break;
        default:
            break;
    }

    if (vkd3d_shader_instruction_is_uav_read(instruction))
    {
        for (i = 0; i < instruction->dst_count; ++i)
        {
            if (instruction->dst[i].reg.type == VKD3DSPR_UAV)
                vkd3d_shader_scan_record_uav_read(context, &instruction->dst[i].reg);
        }
        for (i = 0; i < instruction->src_count; ++i)
        {
            if (instruction->src[i].reg.type == VKD3DSPR_UAV)
                vkd3d_shader_scan_record_uav_read(context, &instruction->src[i].reg);
        }
    }

    if (vkd3d_shader_instruction_is_uav_counter(instruction))
        vkd3d_shader_scan_record_uav_counter(context, &instruction->src[0].reg);

    ++context->location.line;
    return VKD3D_OK;
}

static int scan_with_parser(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context, struct vkd3d_shader_parser *parser)
{
    struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info;
    struct vkd3d_shader_instruction instruction;
    struct vkd3d_shader_scan_context context;
    int ret;

    if ((scan_descriptor_info = vkd3d_find_struct(compile_info->next, SCAN_DESCRIPTOR_INFO)))
    {
        scan_descriptor_info->descriptors = NULL;
        scan_descriptor_info->descriptor_count = 0;
    }

    vkd3d_shader_scan_context_init(&context, compile_info, scan_descriptor_info, message_context);

    if (TRACE_ON())
    {
        vkd3d_shader_trace(parser);
        vkd3d_shader_parser_reset(parser);
    }

    while (!vkd3d_shader_parser_is_end(parser))
    {
        vkd3d_shader_parser_read_instruction(parser, &instruction);

        if (instruction.handler_idx == VKD3DSIH_INVALID)
        {
            WARN("Encountered unrecognized or invalid instruction.\n");
            if (scan_descriptor_info)
                vkd3d_shader_free_scan_descriptor_info(scan_descriptor_info);
            ret = VKD3D_ERROR_INVALID_SHADER;
            goto done;
        }

        if ((ret = vkd3d_shader_scan_instruction(&context, &instruction)) < 0)
        {
            if (scan_descriptor_info)
                vkd3d_shader_free_scan_descriptor_info(scan_descriptor_info);
            goto done;
        }
    }

    ret = parser->failed ? VKD3D_ERROR_INVALID_SHADER : VKD3D_OK;

done:
    vkd3d_shader_scan_context_cleanup(&context);
    return ret;
}

static int scan_dxbc(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_parser *parser;
    int ret;

    if ((ret = vkd3d_shader_sm4_parser_create(compile_info, message_context, &parser)) < 0)
    {
        WARN("Failed to initialise shader parser.\n");
        return ret;
    }

    ret = scan_with_parser(compile_info, message_context, parser);
    vkd3d_shader_parser_destroy(parser);

    return ret;
}

static int scan_d3dbc(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_parser *parser;
    int ret;

    if ((ret = vkd3d_shader_sm1_parser_create(compile_info, message_context, &parser)) < 0)
    {
        WARN("Failed to initialise shader parser.\n");
        return ret;
    }

    ret = scan_with_parser(compile_info, message_context, parser);
    vkd3d_shader_parser_destroy(parser);

    return ret;
}

int vkd3d_shader_scan(const struct vkd3d_shader_compile_info *compile_info, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("compile_info %p, messages %p.\n", compile_info, messages);

    if (messages)
        *messages = NULL;

    if ((ret = vkd3d_shader_validate_compile_info(compile_info, false)) < 0)
        return ret;

    vkd3d_shader_message_context_init(&message_context, compile_info->log_level);

    switch (compile_info->source_type)
    {
        case VKD3D_SHADER_SOURCE_DXBC_TPF:
            ret = scan_dxbc(compile_info, &message_context);
            break;

        case VKD3D_SHADER_SOURCE_HLSL:
            FIXME("HLSL support not implemented.\n");
            ret = VKD3D_ERROR_NOT_IMPLEMENTED;
            break;

        case VKD3D_SHADER_SOURCE_D3D_BYTECODE:
            ret = scan_d3dbc(compile_info, &message_context);
            break;

        default:
            ERR("Unsupported source type %#x.\n", compile_info->source_type);
            ret = VKD3D_ERROR_INVALID_ARGUMENT;
            break;
    }

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&message_context);
    return ret;
}

static int compile_dxbc_tpf(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_scan_descriptor_info scan_descriptor_info;
    struct vkd3d_shader_instruction instruction;
    struct vkd3d_shader_compile_info scan_info;
    struct vkd3d_dxbc_compiler *spirv_compiler;
    struct vkd3d_shader_parser *parser;
    int ret;

    scan_info = *compile_info;
    scan_descriptor_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO;
    scan_descriptor_info.next = scan_info.next;
    scan_info.next = &scan_descriptor_info;

    if ((ret = scan_dxbc(&scan_info, message_context)) < 0)
        return ret;

    if ((ret = vkd3d_shader_sm4_parser_create(compile_info, message_context, &parser)) < 0)
    {
        WARN("Failed to initialise shader parser.\n");
        vkd3d_shader_free_scan_descriptor_info(&scan_descriptor_info);
        return ret;
    }

    vkd3d_shader_dump_shader(compile_info->source_type, parser->shader_version.type, &compile_info->source);

    if (compile_info->target_type == VKD3D_SHADER_TARGET_D3D_ASM)
    {
        vkd3d_shader_free_scan_descriptor_info(&scan_descriptor_info);
        ret = vkd3d_dxbc_binary_to_text(parser, compile_info, out);
        vkd3d_shader_parser_destroy(parser);
        return ret;
    }

    if (compile_info->target_type == VKD3D_SHADER_TARGET_GLSL)
    {
        struct vkd3d_glsl_generator *glsl_generator;

        if (!(glsl_generator = vkd3d_glsl_generator_create(&parser->shader_version,
                message_context, &parser->location)))
        {
            ERR("Failed to create GLSL generator.\n");
            vkd3d_shader_parser_destroy(parser);
            vkd3d_shader_free_scan_descriptor_info(&scan_descriptor_info);
            return VKD3D_ERROR;
        }

        ret = vkd3d_glsl_generator_generate(glsl_generator, parser, out);

        vkd3d_glsl_generator_destroy(glsl_generator);
        vkd3d_shader_parser_destroy(parser);
        vkd3d_shader_free_scan_descriptor_info(&scan_descriptor_info);
        return ret;
    }

    if (!(spirv_compiler = vkd3d_dxbc_compiler_create(&parser->shader_version, &parser->shader_desc,
            compile_info, &scan_descriptor_info, message_context, &parser->location)))
    {
        ERR("Failed to create DXBC compiler.\n");
        vkd3d_shader_parser_destroy(parser);
        vkd3d_shader_free_scan_descriptor_info(&scan_descriptor_info);
        return VKD3D_ERROR;
    }

    while (!vkd3d_shader_parser_is_end(parser))
    {
        vkd3d_shader_parser_read_instruction(parser, &instruction);

        if (instruction.handler_idx == VKD3DSIH_INVALID)
        {
            WARN("Encountered unrecognized or invalid instruction.\n");
            ret = VKD3D_ERROR_INVALID_SHADER;
            break;
        }

        if ((ret = vkd3d_dxbc_compiler_handle_instruction(spirv_compiler, &instruction)) < 0)
            break;
    }

    if (parser->failed)
        ret = VKD3D_ERROR_INVALID_SHADER;

    if (ret >= 0)
        ret = vkd3d_dxbc_compiler_generate_spirv(spirv_compiler, compile_info, out);

    vkd3d_dxbc_compiler_destroy(spirv_compiler);
    vkd3d_shader_parser_destroy(parser);
    vkd3d_shader_free_scan_descriptor_info(&scan_descriptor_info);
    return ret;
}

static int compile_hlsl(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_code preprocessed;
    int ret;

    if ((ret = preproc_lexer_parse(compile_info, &preprocessed, message_context)))
        return ret;

    ret = hlsl_compile_shader(&preprocessed, compile_info, out, message_context);

    vkd3d_shader_free_shader_code(&preprocessed);
    return ret;
}

static int compile_d3d_bytecode(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_parser *parser;
    int ret;

    if ((ret = vkd3d_shader_sm1_parser_create(compile_info, message_context, &parser)) < 0)
    {
        WARN("Failed to initialise shader parser.\n");
        return ret;
    }

    vkd3d_shader_dump_shader(compile_info->source_type, parser->shader_version.type, &compile_info->source);

    if (compile_info->target_type == VKD3D_SHADER_TARGET_D3D_ASM)
    {
        ret = vkd3d_dxbc_binary_to_text(parser, compile_info, out);
        vkd3d_shader_parser_destroy(parser);
        return ret;
    }

    return VKD3D_ERROR;
}

int vkd3d_shader_compile(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("compile_info %p, out %p, messages %p.\n", compile_info, out, messages);

    if (messages)
        *messages = NULL;

    if ((ret = vkd3d_shader_validate_compile_info(compile_info, true)) < 0)
        return ret;

    vkd3d_shader_message_context_init(&message_context, compile_info->log_level);

    switch (compile_info->source_type)
    {
        case VKD3D_SHADER_SOURCE_DXBC_TPF:
            ret = compile_dxbc_tpf(compile_info, out, &message_context);
            break;

        case VKD3D_SHADER_SOURCE_HLSL:
            ret = compile_hlsl(compile_info, out, &message_context);
            break;

        case VKD3D_SHADER_SOURCE_D3D_BYTECODE:
            ret = compile_d3d_bytecode(compile_info, out, &message_context);
            break;

        default:
            assert(0);
    }

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&message_context);
    return ret;
}

void vkd3d_shader_free_scan_descriptor_info(struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info)
{
    TRACE("scan_descriptor_info %p.\n", scan_descriptor_info);

    vkd3d_free(scan_descriptor_info->descriptors);
}

void vkd3d_shader_free_shader_code(struct vkd3d_shader_code *shader_code)
{
    TRACE("shader_code %p.\n", shader_code);

    vkd3d_free((void *)shader_code->code);
}

static void vkd3d_shader_free_root_signature_v_1_0(struct vkd3d_shader_root_signature_desc *root_signature)
{
    unsigned int i;

    for (i = 0; i < root_signature->parameter_count; ++i)
    {
        const struct vkd3d_shader_root_parameter *parameter = &root_signature->parameters[i];

        if (parameter->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)parameter->u.descriptor_table.descriptor_ranges);
    }
    vkd3d_free((void *)root_signature->parameters);
    vkd3d_free((void *)root_signature->static_samplers);

    memset(root_signature, 0, sizeof(*root_signature));
}

static void vkd3d_shader_free_root_signature_v_1_1(struct vkd3d_shader_root_signature_desc1 *root_signature)
{
    unsigned int i;

    for (i = 0; i < root_signature->parameter_count; ++i)
    {
        const struct vkd3d_shader_root_parameter1 *parameter = &root_signature->parameters[i];

        if (parameter->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)parameter->u.descriptor_table.descriptor_ranges);
    }
    vkd3d_free((void *)root_signature->parameters);
    vkd3d_free((void *)root_signature->static_samplers);

    memset(root_signature, 0, sizeof(*root_signature));
}

void vkd3d_shader_free_root_signature(struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    TRACE("desc %p.\n", desc);

    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
    {
        vkd3d_shader_free_root_signature_v_1_0(&desc->u.v_1_0);
    }
    else if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        vkd3d_shader_free_root_signature_v_1_1(&desc->u.v_1_1);
    }
    else if (desc->version)
    {
        FIXME("Unknown version %#x.\n", desc->version);
        return;
    }

    desc->version = 0;
}

int vkd3d_shader_parse_input_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_signature *signature, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("dxbc {%p, %zu}, signature %p, messages %p.\n", dxbc->code, dxbc->size, signature, messages);

    if (messages)
        *messages = NULL;
    vkd3d_shader_message_context_init(&message_context, VKD3D_SHADER_LOG_INFO);

    ret = shader_parse_input_signature(dxbc->code, dxbc->size, &message_context, signature);
    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;

    vkd3d_shader_message_context_cleanup(&message_context);

    return ret;
}

struct vkd3d_shader_signature_element *vkd3d_shader_find_signature_element(
        const struct vkd3d_shader_signature *signature, const char *semantic_name,
        unsigned int semantic_index, unsigned int stream_index)
{
    struct vkd3d_shader_signature_element *e;
    unsigned int i;

    TRACE("signature %p, semantic_name %s, semantic_index %u, stream_index %u.\n",
            signature, debugstr_a(semantic_name), semantic_index, stream_index);

    e = signature->elements;
    for (i = 0; i < signature->element_count; ++i)
    {
        if (!ascii_strcasecmp(e[i].semantic_name, semantic_name)
                && e[i].semantic_index == semantic_index
                && e[i].stream_index == stream_index)
            return &e[i];
    }

    return NULL;
}

void vkd3d_shader_free_shader_signature(struct vkd3d_shader_signature *signature)
{
    TRACE("signature %p.\n", signature);

    vkd3d_free(signature->elements);
    signature->elements = NULL;
}

const char *vkd3d_shader_get_version(unsigned int *major, unsigned int *minor)
{
    int x, y;

    TRACE("major %p, minor %p.\n", major, minor);

    if (major || minor)
    {
        vkd3d_parse_version(PACKAGE_VERSION, &x, &y);
        if (major)
            *major = x;
        if (minor)
            *minor = y;
    }

    return "vkd3d-shader " PACKAGE_VERSION VKD3D_VCS_ID;
}

const enum vkd3d_shader_source_type *vkd3d_shader_get_supported_source_types(unsigned int *count)
{
    static const enum vkd3d_shader_source_type types[] =
    {
        VKD3D_SHADER_SOURCE_DXBC_TPF,
        VKD3D_SHADER_SOURCE_HLSL,
        VKD3D_SHADER_SOURCE_D3D_BYTECODE,
    };

    TRACE("count %p.\n", count);

    *count = ARRAY_SIZE(types);
    return types;
}

const enum vkd3d_shader_target_type *vkd3d_shader_get_supported_target_types(
        enum vkd3d_shader_source_type source_type, unsigned int *count)
{
    static const enum vkd3d_shader_target_type dxbc_tpf_types[] =
    {
        VKD3D_SHADER_TARGET_SPIRV_BINARY,
#ifdef HAVE_SPIRV_TOOLS
        VKD3D_SHADER_TARGET_SPIRV_TEXT,
#endif
        VKD3D_SHADER_TARGET_D3D_ASM,
#if 0
        VKD3D_SHADER_TARGET_GLSL,
#endif
    };

    static const enum vkd3d_shader_target_type hlsl_types[] =
    {
        VKD3D_SHADER_TARGET_D3D_BYTECODE,
        VKD3D_SHADER_TARGET_DXBC_TPF,
    };

    static const enum vkd3d_shader_target_type d3dbc_types[] =
    {
        VKD3D_SHADER_TARGET_D3D_ASM,
    };

    TRACE("source_type %#x, count %p.\n", source_type, count);

    switch (source_type)
    {
        case VKD3D_SHADER_SOURCE_DXBC_TPF:
            *count = ARRAY_SIZE(dxbc_tpf_types);
            return dxbc_tpf_types;

        case VKD3D_SHADER_SOURCE_HLSL:
            *count = ARRAY_SIZE(hlsl_types);
            return hlsl_types;

        case VKD3D_SHADER_SOURCE_D3D_BYTECODE:
            *count = ARRAY_SIZE(d3dbc_types);
            return d3dbc_types;

        default:
            *count = 0;
            return NULL;
    }
}

int vkd3d_shader_preprocess(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("compile_info %p, out %p, messages %p.\n", compile_info, out, messages);

    if (messages)
        *messages = NULL;

    if ((ret = vkd3d_shader_validate_compile_info(compile_info, false)) < 0)
        return ret;

    vkd3d_shader_message_context_init(&message_context, compile_info->log_level);

    ret = preproc_lexer_parse(compile_info, out, &message_context);

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&message_context);
    return ret;
}
