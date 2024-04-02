/*
 * Copyright 2018 Henri Verbeet for CodeWeavers
 * Copyright 2019 Józef Kucia for CodeWeavers
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
#define LIBVKD3D_SHADER_SOURCE
#include <vkd3d_shader.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

static const struct wined3d_shader_backend_ops spirv_shader_backend_vk;

static const struct vkd3d_shader_compile_option spirv_compile_options[] =
{
    {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_3},
};

struct shader_spirv_resource_bindings
{
    struct vkd3d_shader_resource_binding *bindings;
    SIZE_T bindings_size, binding_count;

    struct vkd3d_shader_uav_counter_binding uav_counters[MAX_UNORDERED_ACCESS_VIEWS];
    SIZE_T uav_counter_count;

    VkDescriptorSetLayoutBinding *vk_bindings;
    SIZE_T vk_bindings_size, vk_binding_count;

    size_t binding_base[WINED3D_SHADER_TYPE_COUNT];
    enum wined3d_shader_type so_stage;
};

struct shader_spirv_priv
{
    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct wined3d_fragment_pipe_ops *fragment_pipe;
    bool ffp_proj_control;

    struct shader_spirv_resource_bindings bindings;
};

struct shader_spirv_compile_arguments
{
    union
    {
        struct
        {
            uint32_t alpha_swizzle;
            unsigned int sample_count;
            bool dual_source_blending;
        } fs;
    } u;
};

struct shader_spirv_graphics_program_variant_vk
{
    struct shader_spirv_compile_arguments compile_args;
    const struct wined3d_stream_output_desc *so_desc;
    size_t binding_base;

    VkShaderModule vk_module;
};

struct shader_spirv_graphics_program_vk
{
    struct shader_spirv_graphics_program_variant_vk *variants;
    SIZE_T variants_size, variant_count;

    struct vkd3d_shader_scan_descriptor_info descriptor_info;
};

struct shader_spirv_compute_program_vk
{
    VkShaderModule vk_module;
    VkPipeline vk_pipeline;
    VkPipelineLayout vk_pipeline_layout;
    VkDescriptorSetLayout vk_set_layout;

    struct vkd3d_shader_scan_descriptor_info descriptor_info;
};

struct wined3d_shader_spirv_compile_args
{
    struct vkd3d_shader_spirv_target_info spirv_target;
    struct vkd3d_shader_parameter sample_count;
    unsigned int ps_alpha_swizzle[WINED3D_MAX_RENDER_TARGETS];
};

struct wined3d_shader_spirv_shader_interface
{
    struct vkd3d_shader_interface_info vkd3d_interface;
    struct vkd3d_shader_transform_feedback_info xfb_info;
};

static enum vkd3d_shader_visibility vkd3d_shader_visibility_from_wined3d(enum wined3d_shader_type shader_type)
{
    switch (shader_type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            return VKD3D_SHADER_VISIBILITY_VERTEX;
        case WINED3D_SHADER_TYPE_HULL:
            return VKD3D_SHADER_VISIBILITY_HULL;
        case WINED3D_SHADER_TYPE_DOMAIN:
            return VKD3D_SHADER_VISIBILITY_DOMAIN;
        case WINED3D_SHADER_TYPE_GEOMETRY:
            return VKD3D_SHADER_VISIBILITY_GEOMETRY;
        case WINED3D_SHADER_TYPE_PIXEL:
            return VKD3D_SHADER_VISIBILITY_PIXEL;
        case WINED3D_SHADER_TYPE_COMPUTE:
            return VKD3D_SHADER_VISIBILITY_COMPUTE;
        default:
            ERR("Invalid shader type %s.\n", debug_shader_type(shader_type));
            return VKD3D_SHADER_VISIBILITY_ALL;
    }
}

static void shader_spirv_handle_instruction(const struct wined3d_shader_instruction *ins)
{
}

static void shader_spirv_compile_arguments_init(struct shader_spirv_compile_arguments *args,
        const struct wined3d_context *context, const struct wined3d_shader *shader,
        const struct wined3d_state *state, unsigned int sample_count)
{
    struct wined3d_rendertarget_view *rtv;
    unsigned int i;

    memset(args, 0, sizeof(*args));

    switch (shader->reg_maps.shader_version.type)
    {
        case WINED3D_SHADER_TYPE_PIXEL:
            for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
            {
                if (!(rtv = state->fb.render_targets[i]) || rtv->format->id == WINED3DFMT_NULL)
                    continue;
                if (rtv->format->id == WINED3DFMT_A8_UNORM && !is_identity_fixup(rtv->format->color_fixup))
                    args->u.fs.alpha_swizzle |= 1u << i;
            }
            args->u.fs.sample_count = sample_count;
            args->u.fs.dual_source_blending = state->blend_state && state->blend_state->dual_source;
            break;

        default:
            break;
    }
}

static void shader_spirv_init_compile_args(struct wined3d_shader_spirv_compile_args *args,
        struct vkd3d_shader_interface_info *vkd3d_interface, enum vkd3d_shader_spirv_environment environment,
        enum wined3d_shader_type shader_type, const struct shader_spirv_compile_arguments *compile_args)
{
    unsigned int i;

    memset(args, 0, sizeof(*args));
    args->spirv_target.type = VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_TARGET_INFO;
    args->spirv_target.next = vkd3d_interface;
    args->spirv_target.entry_point = "main";
    args->spirv_target.environment = environment;

    if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
    {
        unsigned int rt_alpha_swizzle = compile_args->u.fs.alpha_swizzle;
        struct vkd3d_shader_parameter *shader_parameter;

        shader_parameter = &args->sample_count;
        shader_parameter->name = VKD3D_SHADER_PARAMETER_NAME_RASTERIZER_SAMPLE_COUNT;
        shader_parameter->type = VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT;
        shader_parameter->data_type = VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32;
        shader_parameter->u.immediate_constant.u.u32 = compile_args->u.fs.sample_count;

        args->spirv_target.dual_source_blending = compile_args->u.fs.dual_source_blending;

        args->spirv_target.parameter_count = 1;
        args->spirv_target.parameters = shader_parameter;

        for (i = 0; i < ARRAY_SIZE(args->ps_alpha_swizzle); ++i)
        {
            if (rt_alpha_swizzle && (1u << i))
                args->ps_alpha_swizzle[i] = VKD3D_SHADER_SWIZZLE(W, X, Y, Z);
            else
                args->ps_alpha_swizzle[i] = VKD3D_SHADER_NO_SWIZZLE;
        }

        args->spirv_target.output_swizzles = args->ps_alpha_swizzle;
        args->spirv_target.output_swizzle_count = ARRAY_SIZE(args->ps_alpha_swizzle);
    }
}

static const char *get_line(const char **ptr)
{
    const char *p, *q;

    p = *ptr;
    if (!(q = strstr(p, "\n")))
    {
        if (!*p) return NULL;
        *ptr += strlen(p);
        return p;
    }
    *ptr = q + 1;

    return p;
}

static void shader_spirv_init_shader_interface_vk(struct wined3d_shader_spirv_shader_interface *iface,
        const struct shader_spirv_resource_bindings *b, const struct wined3d_stream_output_desc *so_desc)
{
    memset(iface, 0, sizeof(*iface));
    iface->vkd3d_interface.type = VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO;

    if (so_desc)
    {
        iface->xfb_info.type = VKD3D_SHADER_STRUCTURE_TYPE_TRANSFORM_FEEDBACK_INFO;
        iface->xfb_info.next = NULL;

        iface->xfb_info.elements = (const struct vkd3d_shader_transform_feedback_element *)so_desc->elements;
        iface->xfb_info.element_count = so_desc->element_count;
        iface->xfb_info.buffer_strides = so_desc->buffer_strides;
        iface->xfb_info.buffer_stride_count = so_desc->buffer_stride_count;

        iface->vkd3d_interface.next = &iface->xfb_info;
    }

    iface->vkd3d_interface.bindings = b->bindings;
    iface->vkd3d_interface.binding_count = b->binding_count;

    iface->vkd3d_interface.uav_counters = b->uav_counters;
    iface->vkd3d_interface.uav_counter_count = b->uav_counter_count;
}

static VkShaderModule shader_spirv_compile_shader(struct wined3d_context_vk *context_vk,
        const struct wined3d_shader_desc *shader_desc, enum wined3d_shader_type shader_type,
        const struct shader_spirv_compile_arguments *args, const struct shader_spirv_resource_bindings *bindings,
        const struct wined3d_stream_output_desc *so_desc)
{
    struct wined3d_shader_spirv_compile_args compile_args;
    struct wined3d_shader_spirv_shader_interface iface;
    VkShaderModuleCreateInfo shader_create_info;
    struct vkd3d_shader_compile_info info;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_device_vk *device_vk;
    struct vkd3d_shader_code spirv;
    VkShaderModule module;
    char *messages;
    VkResult vr;
    int ret;

    shader_spirv_init_shader_interface_vk(&iface, bindings, so_desc);
    shader_spirv_init_compile_args(&compile_args, &iface.vkd3d_interface,
            VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0, shader_type, args);

    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next = &compile_args.spirv_target;
    info.source.code = shader_desc->byte_code;
    info.source.size = shader_desc->byte_code_size;
    info.source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;
    info.target_type = VKD3D_SHADER_TARGET_SPIRV_BINARY;
    info.options = spirv_compile_options;
    info.option_count = ARRAY_SIZE(spirv_compile_options);
    info.log_level = VKD3D_SHADER_LOG_WARNING;
    info.source_name = NULL;

    ret = vkd3d_shader_compile(&info, &spirv, &messages);
    if (messages && *messages && FIXME_ON(d3d_shader))
    {
        const char *ptr = messages;
        const char *line;

        FIXME("Shader log:\n");
        while ((line = get_line(&ptr)))
        {
            FIXME("    %.*s", (int)(ptr - line), line);
        }
        FIXME("\n");
    }
    vkd3d_shader_free_messages(messages);

    if (ret < 0)
    {
        ERR("Failed to compile DXBC, ret %d.\n", ret);
        return VK_NULL_HANDLE;
    }

    device_vk = wined3d_device_vk(context_vk->c.device);
    vk_info = &device_vk->vk_info;

    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext = NULL;
    shader_create_info.flags = 0;
    shader_create_info.codeSize = spirv.size;
    shader_create_info.pCode = spirv.code;
    if ((vr = VK_CALL(vkCreateShaderModule(device_vk->vk_device, &shader_create_info, NULL, &module))) < 0)
    {
        vkd3d_shader_free_shader_code(&spirv);
        WARN("Failed to create Vulkan shader module, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    vkd3d_shader_free_shader_code(&spirv);

    return module;
}

static struct shader_spirv_graphics_program_variant_vk *shader_spirv_find_graphics_program_variant_vk(
        struct shader_spirv_priv *priv, struct wined3d_context_vk *context_vk, struct wined3d_shader *shader,
        const struct wined3d_state *state, const struct shader_spirv_resource_bindings *bindings)
{
    enum wined3d_shader_type shader_type = shader->reg_maps.shader_version.type;
    struct shader_spirv_graphics_program_variant_vk *variant_vk;
    size_t binding_base = bindings->binding_base[shader_type];
    const struct wined3d_stream_output_desc *so_desc = NULL;
    struct shader_spirv_graphics_program_vk *program_vk;
    struct shader_spirv_compile_arguments args;
    struct wined3d_shader_desc shader_desc;
    size_t variant_count, i;

    shader_spirv_compile_arguments_init(&args, &context_vk->c, shader, state, context_vk->sample_count);
    if (bindings->so_stage == shader_type)
        so_desc = state->shader[WINED3D_SHADER_TYPE_GEOMETRY]->u.gs.so_desc;

    if (!(program_vk = shader->backend_data))
        return NULL;

    variant_count = program_vk->variant_count;
    for (i = 0; i < variant_count; ++i)
    {
        variant_vk = &program_vk->variants[i];
        if (variant_vk->so_desc == so_desc && variant_vk->binding_base == binding_base
                && !memcmp(&variant_vk->compile_args, &args, sizeof(args)))
            return variant_vk;
    }

    if (!wined3d_array_reserve((void **)&program_vk->variants, &program_vk->variants_size,
            variant_count + 1, sizeof(*program_vk->variants)))
        return NULL;

    variant_vk = &program_vk->variants[variant_count];
    variant_vk->compile_args = args;
    variant_vk->binding_base = binding_base;

    shader_desc.byte_code = shader->byte_code;
    shader_desc.byte_code_size = shader->byte_code_size;

    if (!(variant_vk->vk_module = shader_spirv_compile_shader(context_vk, &shader_desc, shader_type, &args,
            bindings, so_desc)))
        return NULL;
    ++program_vk->variant_count;

    return variant_vk;
}

static struct shader_spirv_compute_program_vk *shader_spirv_find_compute_program_vk(struct shader_spirv_priv *priv,
        struct wined3d_context_vk *context_vk, struct wined3d_shader *shader,
        const struct shader_spirv_resource_bindings *bindings)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct shader_spirv_compute_program_vk *program;
    struct wined3d_pipeline_layout_vk *layout;
    VkComputePipelineCreateInfo pipeline_info;
    struct wined3d_shader_desc shader_desc;
    VkResult vr;

    if (!(program = shader->backend_data))
        return NULL;

    if (program->vk_module)
        return program;

    shader_desc.byte_code = shader->byte_code;
    shader_desc.byte_code_size = shader->byte_code_size;

    if (!(program->vk_module = shader_spirv_compile_shader(context_vk, &shader_desc, WINED3D_SHADER_TYPE_COMPUTE,
            NULL, bindings, NULL)))
        return NULL;

    if (!(layout = wined3d_context_vk_get_pipeline_layout(context_vk,
            bindings->vk_bindings, bindings->vk_binding_count)))
    {
        VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
        program->vk_module = VK_NULL_HANDLE;
        return NULL;
    }
    program->vk_set_layout = layout->vk_set_layout;
    program->vk_pipeline_layout = layout->vk_pipeline_layout;

    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = NULL;
    pipeline_info.flags = 0;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.pNext = NULL;
    pipeline_info.stage.flags = 0;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.pName = "main";
    pipeline_info.stage.pSpecializationInfo = NULL;
    pipeline_info.stage.module = program->vk_module;
    pipeline_info.layout = program->vk_pipeline_layout;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;
    if ((vr = VK_CALL(vkCreateComputePipelines(device_vk->vk_device,
            VK_NULL_HANDLE, 1, &pipeline_info, NULL, &program->vk_pipeline))) < 0)
    {
        ERR("Failed to create Vulkan compute pipeline, vr %s.\n", wined3d_debug_vkresult(vr));
        VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
        program->vk_module = VK_NULL_HANDLE;
        return NULL;
    }

    return program;
}

static void shader_spirv_resource_bindings_cleanup(struct shader_spirv_resource_bindings *bindings)
{
    heap_free(bindings->vk_bindings);
    heap_free(bindings->bindings);
}

static bool shader_spirv_resource_bindings_add_vk_binding(struct shader_spirv_resource_bindings *bindings,
        VkDescriptorType vk_type, VkShaderStageFlagBits vk_stage, size_t *binding_idx)
{
    SIZE_T binding_count = bindings->vk_binding_count;
    VkDescriptorSetLayoutBinding *binding;

    if (!wined3d_array_reserve((void **)&bindings->vk_bindings, &bindings->vk_bindings_size,
            binding_count + 1, sizeof(*bindings->vk_bindings)))
        return false;

    *binding_idx = binding_count;
    binding = &bindings->vk_bindings[binding_count];
    binding->binding = binding_count;
    binding->descriptorType = vk_type;
    binding->descriptorCount = 1;
    binding->stageFlags = vk_stage;
    binding->pImmutableSamplers = NULL;
    ++bindings->vk_binding_count;

    return true;
}

static bool shader_spirv_resource_bindings_add_binding(struct shader_spirv_resource_bindings *bindings,
        enum vkd3d_shader_descriptor_type vkd3d_type, VkDescriptorType vk_type, size_t register_idx,
        enum vkd3d_shader_visibility vkd3d_visibility, VkShaderStageFlagBits vk_stage,
        uint32_t flags, size_t *binding_idx)
{
    SIZE_T binding_count = bindings->binding_count;
    struct vkd3d_shader_resource_binding *binding;

    if (!wined3d_array_reserve((void **)&bindings->bindings, &bindings->bindings_size,
            binding_count + 1, sizeof(*bindings->bindings)))
        return false;

    if (!shader_spirv_resource_bindings_add_vk_binding(bindings, vk_type, vk_stage, binding_idx))
        return false;

    binding = &bindings->bindings[binding_count];
    binding->type = vkd3d_type;
    binding->register_space = 0;
    binding->register_index = register_idx;
    binding->shader_visibility = vkd3d_visibility;
    binding->flags = flags;
    binding->binding.set = 0;
    binding->binding.binding = *binding_idx;
    binding->binding.count = 1;
    ++bindings->binding_count;

    return true;
}

static bool shader_spirv_resource_bindings_add_uav_counter_binding(struct shader_spirv_resource_bindings *bindings,
        size_t register_idx, enum vkd3d_shader_visibility vkd3d_visibility,
        VkShaderStageFlagBits vk_stage, size_t *binding_idx)
{
    SIZE_T uav_counter_count = bindings->uav_counter_count;
    struct vkd3d_shader_uav_counter_binding *counter;

    if (uav_counter_count >= ARRAY_SIZE(bindings->uav_counters))
        return false;

    if (!shader_spirv_resource_bindings_add_vk_binding(bindings,
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, vk_stage, binding_idx))
        return false;

    counter = &bindings->uav_counters[uav_counter_count];
    counter->register_space = 0;
    counter->register_index = register_idx;
    counter->shader_visibility = vkd3d_visibility;
    counter->binding.set = 0;
    counter->binding.binding = *binding_idx;
    counter->binding.count = 1;
    counter->offset = 0;
    ++bindings->uav_counter_count;

    return true;
}

static bool wined3d_shader_resource_bindings_add_binding(struct wined3d_shader_resource_bindings *bindings,
        enum wined3d_shader_type shader_type, enum wined3d_shader_descriptor_type shader_descriptor_type,
        size_t resource_idx, enum wined3d_shader_resource_type resource_type,
        enum wined3d_data_type resource_data_type, size_t binding_idx)
{
    struct wined3d_shader_resource_binding *binding;
    SIZE_T binding_count = bindings->count;

    if (!wined3d_array_reserve((void **)&bindings->bindings, &bindings->size,
            binding_count + 1, sizeof(*bindings->bindings)))
        return false;

    binding = &bindings->bindings[binding_count];
    binding->shader_type = shader_type;
    binding->shader_descriptor_type = shader_descriptor_type;
    binding->resource_idx = resource_idx;
    binding->resource_type = resource_type;
    binding->resource_data_type = resource_data_type;
    binding->binding_idx = binding_idx;
    ++bindings->count;

    return true;
}

static VkDescriptorType vk_descriptor_type_from_vkd3d(enum vkd3d_shader_descriptor_type type,
        enum vkd3d_shader_resource_type resource_type)
{
    switch (type)
    {
        case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        case VKD3D_SHADER_DESCRIPTOR_TYPE_SRV:
            if (resource_type == VKD3D_SHADER_RESOURCE_BUFFER)
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

        case VKD3D_SHADER_DESCRIPTOR_TYPE_UAV:
            if (resource_type == VKD3D_SHADER_RESOURCE_BUFFER)
                return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

        case VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_SAMPLER;

        default:
            FIXME("Unhandled descriptor type %#x.\n", type);
            return ~0u;
    }
}

static enum wined3d_shader_descriptor_type wined3d_descriptor_type_from_vkd3d(enum vkd3d_shader_descriptor_type type)
{
    switch (type)
    {
        case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
            return WINED3D_SHADER_DESCRIPTOR_TYPE_CBV;

        case VKD3D_SHADER_DESCRIPTOR_TYPE_SRV:
            return WINED3D_SHADER_DESCRIPTOR_TYPE_SRV;

        case VKD3D_SHADER_DESCRIPTOR_TYPE_UAV:
            return WINED3D_SHADER_DESCRIPTOR_TYPE_UAV;

        case VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
            return WINED3D_SHADER_DESCRIPTOR_TYPE_SAMPLER;

        default:
            FIXME("Unhandled descriptor type %#x.\n", type);
            return ~0u;
    }
}

static enum wined3d_shader_resource_type wined3d_shader_resource_type_from_vkd3d(enum vkd3d_shader_resource_type t)
{
    return (enum wined3d_shader_resource_type)t;
}

static enum wined3d_data_type wined3d_data_type_from_vkd3d(enum vkd3d_shader_resource_data_type t)
{
    switch (t)
    {
        case VKD3D_SHADER_RESOURCE_DATA_UNORM:
            return WINED3D_DATA_UNORM;
        case VKD3D_SHADER_RESOURCE_DATA_SNORM:
            return WINED3D_DATA_SNORM;
        case VKD3D_SHADER_RESOURCE_DATA_INT:
            return WINED3D_DATA_INT;
        case VKD3D_SHADER_RESOURCE_DATA_UINT:
            return WINED3D_DATA_UINT;
        case VKD3D_SHADER_RESOURCE_DATA_FLOAT:
            return WINED3D_DATA_FLOAT;
        case VKD3D_SHADER_RESOURCE_DATA_MIXED:
            return WINED3D_DATA_UINT;
        default:
            FIXME("Unhandled resource data type %#x.\n", t);
            return WINED3D_DATA_FLOAT;
    }
}

static bool shader_spirv_resource_bindings_init(struct shader_spirv_resource_bindings *bindings,
        struct wined3d_shader_resource_bindings *wined3d_bindings,
        const struct wined3d_state *state, uint32_t shader_mask)
{
    struct vkd3d_shader_scan_descriptor_info *descriptor_info;
    enum wined3d_shader_descriptor_type wined3d_type;
    enum vkd3d_shader_visibility shader_visibility;
    enum wined3d_shader_type shader_type;
    VkDescriptorType vk_descriptor_type;
    VkShaderStageFlagBits vk_stage;
    struct wined3d_shader *shader;
    size_t binding_idx;
    unsigned int i;

    bindings->binding_count = 0;
    bindings->uav_counter_count = 0;
    bindings->vk_binding_count = 0;
    bindings->so_stage = WINED3D_SHADER_TYPE_GEOMETRY;
    wined3d_bindings->count = 0;

    for (shader_type = 0; shader_type < WINED3D_SHADER_TYPE_COUNT; ++shader_type)
    {
        bindings->binding_base[shader_type] = bindings->vk_binding_count;

        if (!(shader_mask & (1u << shader_type)) || !(shader = state->shader[shader_type]))
            continue;

        if (shader_type == WINED3D_SHADER_TYPE_COMPUTE)
        {
            descriptor_info = &((struct shader_spirv_compute_program_vk *)shader->backend_data)->descriptor_info;
        }
        else
        {
            descriptor_info = &((struct shader_spirv_graphics_program_vk *)shader->backend_data)->descriptor_info;
            if (shader_type == WINED3D_SHADER_TYPE_GEOMETRY && !shader->function)
                bindings->so_stage = WINED3D_SHADER_TYPE_VERTEX;
        }

        vk_stage = vk_shader_stage_from_wined3d(shader_type);
        shader_visibility = vkd3d_shader_visibility_from_wined3d(shader_type);

        for (i = 0; i < descriptor_info->descriptor_count; ++i)
        {
            struct vkd3d_shader_descriptor_info *d = &descriptor_info->descriptors[i];
            uint32_t flags;

            if (d->register_space)
            {
                WARN("Unsupported register space %u.\n", d->register_space);
                return false;
            }

            if (d->resource_type == VKD3D_SHADER_RESOURCE_BUFFER)
                flags = VKD3D_SHADER_BINDING_FLAG_BUFFER;
            else
                flags = VKD3D_SHADER_BINDING_FLAG_IMAGE;

            vk_descriptor_type = vk_descriptor_type_from_vkd3d(d->type, d->resource_type);
            if (!shader_spirv_resource_bindings_add_binding(bindings, d->type, vk_descriptor_type,
                    d->register_index, shader_visibility, vk_stage, flags, &binding_idx))
                return false;

            wined3d_type = wined3d_descriptor_type_from_vkd3d(d->type);
            if (!wined3d_shader_resource_bindings_add_binding(wined3d_bindings, shader_type,
                    wined3d_type, d->register_index, wined3d_shader_resource_type_from_vkd3d(d->resource_type),
                    wined3d_data_type_from_vkd3d(d->resource_data_type), binding_idx))
                return false;

            if (d->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                    && (d->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER))
            {
                if (!shader_spirv_resource_bindings_add_uav_counter_binding(bindings,
                        d->register_index, shader_visibility, vk_stage, &binding_idx))
                    return false;
                if (!wined3d_shader_resource_bindings_add_binding(wined3d_bindings,
                        shader_type, WINED3D_SHADER_DESCRIPTOR_TYPE_UAV_COUNTER, d->register_index,
                        WINED3D_SHADER_RESOURCE_BUFFER, WINED3D_DATA_UINT, binding_idx))
                    return false;
            }
        }
    }

    return true;
}

static void shader_spirv_scan_shader(struct wined3d_shader *shader,
        struct vkd3d_shader_scan_descriptor_info *descriptor_info)
{
    struct vkd3d_shader_compile_info info;
    char *messages;
    int ret;

    memset(descriptor_info, 0, sizeof(*descriptor_info));
    descriptor_info->type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO;

    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next = descriptor_info;
    info.source.code = shader->byte_code;
    info.source.size = shader->byte_code_size;
    info.source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;
    info.target_type = VKD3D_SHADER_TARGET_SPIRV_BINARY;
    info.options = spirv_compile_options;
    info.option_count = ARRAY_SIZE(spirv_compile_options);
    info.log_level = VKD3D_SHADER_LOG_WARNING;
    info.source_name = NULL;

    if ((ret = vkd3d_shader_scan(&info, &messages)) < 0)
        ERR("Failed to scan shader, ret %d.\n", ret);
    if (messages && *messages && FIXME_ON(d3d_shader))
    {
        const char *ptr = messages;
        const char *line;

        FIXME("Shader log:\n");
        while ((line = get_line(&ptr)))
        {
            FIXME("    %.*s", (int)(ptr - line), line);
        }
        FIXME("\n");
    }
    vkd3d_shader_free_messages(messages);
}

static void shader_spirv_precompile_compute(struct wined3d_shader *shader)
{
    struct shader_spirv_compute_program_vk *program_vk;

    if (!(program_vk = shader->backend_data))
    {
        if (!(program_vk = heap_alloc_zero(sizeof(*program_vk))))
            ERR("Failed to allocate program.\n");
        shader->backend_data = program_vk;
    }

    shader_spirv_scan_shader(shader, &program_vk->descriptor_info);
}

static void shader_spirv_precompile(void *shader_priv, struct wined3d_shader *shader)
{
    struct shader_spirv_graphics_program_vk *program_vk;

    TRACE("shader_priv %p, shader %p.\n", shader_priv, shader);

    if (shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_COMPUTE)
    {
        shader_spirv_precompile_compute(shader);
        return;
    }

    if (!(program_vk = shader->backend_data))
    {
        if (!(program_vk = heap_alloc_zero(sizeof(*program_vk))))
            ERR("Failed to allocate program.\n");
        shader->backend_data = program_vk;
    }

    shader_spirv_scan_shader(shader, &program_vk->descriptor_info);
}

static void shader_spirv_select(void *shader_priv, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    struct shader_spirv_graphics_program_variant_vk *variant_vk;
    struct shader_spirv_resource_bindings *bindings;
    size_t binding_base[WINED3D_SHADER_TYPE_COUNT];
    struct wined3d_pipeline_layout_vk *layout_vk;
    struct shader_spirv_priv *priv = shader_priv;
    enum wined3d_shader_type shader_type;
    struct wined3d_shader *shader;

    priv->vertex_pipe->vp_enable(context, !use_vs(state));
    priv->fragment_pipe->fp_enable(context, !use_ps(state));

    bindings = &priv->bindings;
    memcpy(binding_base, bindings->binding_base, sizeof(bindings->binding_base));
    if (!shader_spirv_resource_bindings_init(bindings, &context_vk->graphics.bindings,
            state, ~(1u << WINED3D_SHADER_TYPE_COMPUTE)))
    {
        ERR("Failed to initialise shader resource bindings.\n");
        goto fail;
    }
    if (context->shader_update_mask & (1u << WINED3D_SHADER_TYPE_GEOMETRY))
        context->shader_update_mask |= 1u << bindings->so_stage;

    layout_vk = wined3d_context_vk_get_pipeline_layout(context_vk, bindings->vk_bindings, bindings->vk_binding_count);
    context_vk->graphics.vk_set_layout = layout_vk->vk_set_layout;
    context_vk->graphics.vk_pipeline_layout = layout_vk->vk_pipeline_layout;

    for (shader_type = 0; shader_type < ARRAY_SIZE(context_vk->graphics.vk_modules); ++shader_type)
    {
        if (!(context->shader_update_mask & (1u << shader_type)) && (!context_vk->graphics.vk_modules[shader_type]
                || binding_base[shader_type] == bindings->binding_base[shader_type]))
            continue;

        if (!(shader = state->shader[shader_type]) || !shader->function)
        {
            context_vk->graphics.vk_modules[shader_type] = VK_NULL_HANDLE;
            continue;
        }

        if (!(variant_vk = shader_spirv_find_graphics_program_variant_vk(priv, context_vk, shader, state, bindings)))
            goto fail;
        context_vk->graphics.vk_modules[shader_type] = variant_vk->vk_module;
    }

    return;

fail:
    context_vk->graphics.vk_set_layout = VK_NULL_HANDLE;
    context_vk->graphics.vk_pipeline_layout = VK_NULL_HANDLE;
}

static void shader_spirv_select_compute(void *shader_priv,
        struct wined3d_context *context, const struct wined3d_state *state)
{
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    struct shader_spirv_compute_program_vk *program;
    struct shader_spirv_priv *priv = shader_priv;
    struct wined3d_shader *shader;

    if (!shader_spirv_resource_bindings_init(&priv->bindings,
            &context_vk->compute.bindings, state, 1u << WINED3D_SHADER_TYPE_COMPUTE))
        ERR("Failed to initialise shader resource bindings.\n");

    if ((shader = state->shader[WINED3D_SHADER_TYPE_COMPUTE]))
        program = shader_spirv_find_compute_program_vk(priv, context_vk, shader, &priv->bindings);
    else
        program = NULL;

    if (program)
    {
        context_vk->compute.vk_pipeline = program->vk_pipeline;
        context_vk->compute.vk_set_layout = program->vk_set_layout;
        context_vk->compute.vk_pipeline_layout = program->vk_pipeline_layout;
    }
    else
    {
        context_vk->compute.vk_pipeline = VK_NULL_HANDLE;
        context_vk->compute.vk_set_layout = VK_NULL_HANDLE;
        context_vk->compute.vk_pipeline_layout = VK_NULL_HANDLE;
    }
}

static void shader_spirv_disable(void *shader_priv, struct wined3d_context *context)
{
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    struct shader_spirv_priv *priv = shader_priv;

    priv->vertex_pipe->vp_enable(context, false);
    priv->fragment_pipe->fp_enable(context, false);

    context_vk->compute.vk_pipeline = VK_NULL_HANDLE;
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

static void shader_spirv_invalidate_compute_program(struct wined3d_context_vk *context_vk,
        const struct shader_spirv_compute_program_vk *program)
{
    if (context_vk->compute.vk_pipeline == program->vk_pipeline)
    {
        context_vk->c.shader_update_mask |= (1u << WINED3D_SHADER_TYPE_COMPUTE);
        context_vk->compute.vk_pipeline = VK_NULL_HANDLE;
    }
}

static void shader_spirv_invalidate_contexts_compute_program(struct wined3d_device *device,
    const struct shader_spirv_compute_program_vk *program)
{
    unsigned int i;

    for (i = 0; i < device->context_count; ++i)
    {
        shader_spirv_invalidate_compute_program(wined3d_context_vk(device->contexts[i]), program);
    }
}

static void shader_spirv_invalidate_graphics_program_variant(struct wined3d_context_vk *context_vk,
        const struct shader_spirv_graphics_program_variant_vk *variant)
{
    enum wined3d_shader_type shader_type;

    for (shader_type = 0; shader_type < WINED3D_SHADER_TYPE_GRAPHICS_COUNT; ++shader_type)
    {
        if (context_vk->graphics.vk_modules[shader_type] != variant->vk_module)
            continue;

        context_vk->graphics.vk_modules[shader_type] = VK_NULL_HANDLE;
        context_vk->c.shader_update_mask |= (1u << shader_type);
    }
}

static void shader_spirv_invalidate_contexts_graphics_program_variant(struct wined3d_device *device,
        const struct shader_spirv_graphics_program_variant_vk *variant)
{
    unsigned int i;

    for (i = 0; i < device->context_count; ++i)
    {
        shader_spirv_invalidate_graphics_program_variant(wined3d_context_vk(device->contexts[i]), variant);
    }
}

static void shader_spirv_destroy_compute_vk(struct wined3d_shader *shader)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(shader->device);
    struct shader_spirv_compute_program_vk *program = shader->backend_data;
    struct wined3d_context_vk *context_vk = &device_vk->context_vk;
    struct wined3d_vk_info *vk_info = &device_vk->vk_info;

    shader_spirv_invalidate_contexts_compute_program(&device_vk->d, program);
    wined3d_context_vk_destroy_vk_pipeline(context_vk, program->vk_pipeline, context_vk->current_command_buffer.id);
    VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
    vkd3d_shader_free_scan_descriptor_info(&program->descriptor_info);
    shader->backend_data = NULL;
    heap_free(program);
}

static void shader_spirv_destroy(struct wined3d_shader *shader)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(shader->device);
    struct shader_spirv_graphics_program_variant_vk *variant_vk;
    struct wined3d_vk_info *vk_info = &device_vk->vk_info;
    struct shader_spirv_graphics_program_vk *program_vk;
    size_t i;

    if (!shader->backend_data)
        return;

    if (shader->reg_maps.shader_version.type == WINED3D_SHADER_TYPE_COMPUTE)
    {
        shader_spirv_destroy_compute_vk(shader);
        return;
    }

    program_vk = shader->backend_data;
    for (i = 0; i < program_vk->variant_count; ++i)
    {
        variant_vk = &program_vk->variants[i];
        shader_spirv_invalidate_contexts_graphics_program_variant(&device_vk->d, variant_vk);
        VK_CALL(vkDestroyShaderModule(device_vk->vk_device, variant_vk->vk_module, NULL));
    }
    heap_free(program_vk->variants);
    vkd3d_shader_free_scan_descriptor_info(&program_vk->descriptor_info);

    shader->backend_data = NULL;
    heap_free(program_vk);
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
    memset(&priv->bindings, 0, sizeof(priv->bindings));

    device->vertex_priv = vertex_priv;
    device->fragment_priv = fragment_priv;
    device->shader_priv = priv;

    return WINED3D_OK;
}

static void shader_spirv_free(struct wined3d_device *device, struct wined3d_context *context)
{
    struct shader_spirv_priv *priv = device->shader_priv;

    shader_spirv_resource_bindings_cleanup(&priv->bindings);
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
    caps->vs_version = min(wined3d_settings.max_sm_vs, 5);
    caps->hs_version = min(wined3d_settings.max_sm_hs, 5);
    caps->ds_version = min(wined3d_settings.max_sm_ds, 5);
    caps->gs_version = min(wined3d_settings.max_sm_gs, 5);
    caps->ps_version = min(wined3d_settings.max_sm_ps, 5);
    caps->cs_version = min(wined3d_settings.max_sm_cs, 5);

    caps->vs_uniform_count = WINED3D_MAX_VS_CONSTS_F;
    caps->ps_uniform_count = WINED3D_MAX_PS_CONSTS_F;
    caps->ps_1x_max_value = FLT_MAX;
    caps->varying_count = 0;
    caps->wined3d_caps = WINED3D_SHADER_CAP_VS_CLIPPING
            | WINED3D_SHADER_CAP_SRGB_WRITE
            | WINED3D_SHADER_CAP_FULL_FFP_VARYINGS;
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

static uint64_t shader_spirv_compile(struct wined3d_context *context, const struct wined3d_shader_desc *shader_desc,
        enum wined3d_shader_type shader_type)
{
    struct shader_spirv_resource_bindings bindings = {0};
    return (uint64_t)shader_spirv_compile_shader(wined3d_context_vk(context), shader_desc, shader_type, NULL, &bindings, NULL);
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
    .shader_compile = shader_spirv_compile,
};

const struct wined3d_shader_backend_ops *wined3d_spirv_shader_backend_init_vk(void)
{
    TRACE("Using %s.\n", vkd3d_shader_get_version(NULL, NULL));

    return &spirv_shader_backend_vk;
}

static void spirv_vertex_pipe_vk_vp_enable(const struct wined3d_context *context, BOOL enable)
{
    /* Nothing to do. */
}

static void spirv_vertex_pipe_vk_vp_get_caps(const struct wined3d_adapter *adapter, struct wined3d_vertex_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
    caps->xyzrhw = TRUE;
    caps->ffp_generic_attributes = TRUE;
}

static uint32_t spirv_vertex_pipe_vk_vp_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return 0;
}

static void *spirv_vertex_pipe_vk_vp_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    if (shader_backend != &spirv_shader_backend_vk)
    {
        FIXME("SPIR-V vertex pipe without SPIR-V shader backend not implemented.\n");
        return NULL;
    }

    return shader_priv;
}

static void spirv_vertex_pipe_vk_vp_free(struct wined3d_device *device, struct wined3d_context *context)
{
    /* Nothing to do. */
}

static const struct wined3d_state_entry_template spirv_vertex_pipe_vk_vp_states[] =
{
    {STATE_RENDER(WINED3D_RS_RANGEFOGENABLE),           {STATE_RENDER(WINED3D_RS_RANGEFOGENABLE),           state_nop}},
    {STATE_RENDER(WINED3D_RS_CLIPPING),                 {STATE_RENDER(WINED3D_RS_CLIPPING),                 state_nop}},
    {STATE_RENDER(WINED3D_RS_LIGHTING),                 {STATE_RENDER(WINED3D_RS_LIGHTING),                 state_nop}},
    {STATE_RENDER(WINED3D_RS_AMBIENT),                  {STATE_RENDER(WINED3D_RS_AMBIENT),                  state_nop}},
    {STATE_RENDER(WINED3D_RS_COLORVERTEX),              {STATE_RENDER(WINED3D_RS_COLORVERTEX),              state_nop}},
    {STATE_RENDER(WINED3D_RS_LOCALVIEWER),              {STATE_RENDER(WINED3D_RS_LOCALVIEWER),              state_nop}},
    {STATE_RENDER(WINED3D_RS_NORMALIZENORMALS),         {STATE_RENDER(WINED3D_RS_NORMALIZENORMALS),         state_nop}},
    {STATE_RENDER(WINED3D_RS_DIFFUSEMATERIALSOURCE),    {STATE_RENDER(WINED3D_RS_DIFFUSEMATERIALSOURCE),    state_nop}},
    {STATE_RENDER(WINED3D_RS_SPECULARMATERIALSOURCE),   {STATE_RENDER(WINED3D_RS_SPECULARMATERIALSOURCE),   state_nop}},
    {STATE_RENDER(WINED3D_RS_AMBIENTMATERIALSOURCE),    {STATE_RENDER(WINED3D_RS_AMBIENTMATERIALSOURCE),    state_nop}},
    {STATE_RENDER(WINED3D_RS_EMISSIVEMATERIALSOURCE),   {STATE_RENDER(WINED3D_RS_EMISSIVEMATERIALSOURCE),   state_nop}},
    {STATE_RENDER(WINED3D_RS_VERTEXBLEND),              {STATE_RENDER(WINED3D_RS_VERTEXBLEND),              state_nop}},
    {STATE_RENDER(WINED3D_RS_CLIPPLANEENABLE),          {STATE_RENDER(WINED3D_RS_CLIPPLANEENABLE),          state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSIZE),                {STATE_RENDER(WINED3D_RS_POINTSIZE),                state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),            {STATE_RENDER(WINED3D_RS_POINTSIZE_MIN),            state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),         {STATE_RENDER(WINED3D_RS_POINTSCALEENABLE),         state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSCALE_A),             {STATE_RENDER(WINED3D_RS_POINTSCALE_A),             state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSCALE_B),             {STATE_RENDER(WINED3D_RS_POINTSCALE_B),             state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSCALE_C),             {STATE_RENDER(WINED3D_RS_POINTSCALE_C),             state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSIZE_MAX),            {STATE_RENDER(WINED3D_RS_POINTSIZE_MAX),            state_nop}},
    {STATE_RENDER(WINED3D_RS_INDEXEDVERTEXBLENDENABLE), {STATE_RENDER(WINED3D_RS_INDEXEDVERTEXBLENDENABLE), state_nop}},
    {STATE_RENDER(WINED3D_RS_TWEENFACTOR),              {STATE_RENDER(WINED3D_RS_TWEENFACTOR),              state_nop}},
    {STATE_MATERIAL,                                    {STATE_MATERIAL,                                    state_nop}},
    {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),          {STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX),          state_nop}},
    {STATE_LIGHT_TYPE,                                  {STATE_LIGHT_TYPE,                                  state_nop}},
    {0}, /* Terminate */
};

static const struct wined3d_vertex_pipe_ops spirv_vertex_pipe_vk =
{
    .vp_enable = spirv_vertex_pipe_vk_vp_enable,
    .vp_get_caps = spirv_vertex_pipe_vk_vp_get_caps,
    .vp_get_emul_mask = spirv_vertex_pipe_vk_vp_get_emul_mask,
    .vp_alloc = spirv_vertex_pipe_vk_vp_alloc,
    .vp_free = spirv_vertex_pipe_vk_vp_free,
    .vp_states = spirv_vertex_pipe_vk_vp_states,
};

const struct wined3d_vertex_pipe_ops *wined3d_spirv_vertex_pipe_init_vk(void)
{
    return &spirv_vertex_pipe_vk;
}

static void spirv_fragment_pipe_vk_fp_enable(const struct wined3d_context *context, BOOL enable)
{
    /* Nothing to do. */
}

static void spirv_fragment_pipe_vk_fp_get_caps(const struct wined3d_adapter *adapter, struct fragment_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
}

static uint32_t spirv_fragment_pipe_vk_fp_get_emul_mask(const struct wined3d_gl_info *gl_info)
{
    return 0;
}

static void *spirv_fragment_pipe_vk_fp_alloc(const struct wined3d_shader_backend_ops *shader_backend, void *shader_priv)
{
    if (shader_backend != &spirv_shader_backend_vk)
    {
        FIXME("SPIR-V fragment pipe without SPIR-V shader backend not implemented.\n");
        return NULL;
    }

    return shader_priv;
}

static void spirv_fragment_pipe_vk_fp_free(struct wined3d_device *device, struct wined3d_context *context)
{
    /* Nothing to do. */
}

static BOOL spirv_fragment_pipe_vk_fp_alloc_context_data(struct wined3d_context *context)
{
    return TRUE;
}

static void spirv_fragment_pipe_vk_fp_free_context_data(struct wined3d_context *context)
{
    /* Nothing to do. */
}

static const struct wined3d_state_entry_template spirv_fragment_pipe_vk_fp_states[] =
{
    {STATE_RENDER(WINED3D_RS_SHADEMODE),         {STATE_RENDER(WINED3D_RS_SHADEMODE),         state_nop}},
    {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),   {STATE_RENDER(WINED3D_RS_ALPHATESTENABLE),   state_nop}},
    {STATE_RENDER(WINED3D_RS_ALPHAREF),          {STATE_RENDER(WINED3D_RS_ALPHAREF),          state_nop}},
    {STATE_RENDER(WINED3D_RS_ALPHAFUNC),         {STATE_RENDER(WINED3D_RS_ALPHAFUNC),         state_nop}},
    {STATE_RENDER(WINED3D_RS_FOGENABLE),         {STATE_RENDER(WINED3D_RS_FOGENABLE),         state_nop}},
    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),    {STATE_RENDER(WINED3D_RS_SPECULARENABLE),    state_nop}},
    {STATE_RENDER(WINED3D_RS_FOGCOLOR),          {STATE_RENDER(WINED3D_RS_FOGCOLOR),          state_nop}},
    {STATE_RENDER(WINED3D_RS_FOGTABLEMODE),      {STATE_RENDER(WINED3D_RS_FOGTABLEMODE),      state_nop}},
    {STATE_RENDER(WINED3D_RS_FOGSTART),          {STATE_RENDER(WINED3D_RS_FOGSTART),          state_nop}},
    {STATE_RENDER(WINED3D_RS_FOGEND),            {STATE_RENDER(WINED3D_RS_FOGEND),            state_nop}},
    {STATE_RENDER(WINED3D_RS_FOGDENSITY),        {STATE_RENDER(WINED3D_RS_FOGDENSITY),        state_nop}},
    {STATE_RENDER(WINED3D_RS_COLORKEYENABLE),    {STATE_RENDER(WINED3D_RS_COLORKEYENABLE),    state_nop}},
    {STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),     {STATE_RENDER(WINED3D_RS_TEXTUREFACTOR),     state_nop}},
    {STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),     {STATE_RENDER(WINED3D_RS_FOGVERTEXMODE),     state_nop}},
    {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE), {STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE), state_nop}},
    {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),   {STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE),   state_nop}},
    {STATE_POINT_ENABLE,                         {STATE_POINT_ENABLE,                         state_nop}},
    {STATE_COLOR_KEY,                            {STATE_COLOR_KEY,                            state_nop}},
    {0}, /* Terminate */
};

static const struct wined3d_fragment_pipe_ops spirv_fragment_pipe_vk =
{
    .fp_enable = spirv_fragment_pipe_vk_fp_enable,
    .get_caps = spirv_fragment_pipe_vk_fp_get_caps,
    .get_emul_mask = spirv_fragment_pipe_vk_fp_get_emul_mask,
    .alloc_private = spirv_fragment_pipe_vk_fp_alloc,
    .free_private = spirv_fragment_pipe_vk_fp_free,
    .allocate_context_data = spirv_fragment_pipe_vk_fp_alloc_context_data,
    .free_context_data = spirv_fragment_pipe_vk_fp_free_context_data,
    .color_fixup_supported = shader_spirv_color_fixup_supported,
    .states = spirv_fragment_pipe_vk_fp_states,
};

const struct wined3d_fragment_pipe_ops *wined3d_spirv_fragment_pipe_init_vk(void)
{
    return &spirv_fragment_pipe_vk;
}
