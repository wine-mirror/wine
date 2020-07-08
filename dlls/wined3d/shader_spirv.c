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

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

static const struct wined3d_shader_backend_ops spirv_shader_backend_vk;

struct shader_spirv_resource_bindings
{
    VkDescriptorSetLayoutBinding *vk_bindings;
    SIZE_T vk_bindings_size, vk_binding_count;

    size_t binding_base[WINED3D_SHADER_TYPE_COUNT];
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
        } fs;

        struct
        {
            enum wined3d_tessellator_output_primitive output_primitive;
            enum wined3d_tessellator_partitioning partitioning;
        } tes;
    } u;
};

struct shader_spirv_graphics_program_variant_vk
{
    struct shader_spirv_compile_arguments compile_args;
    size_t binding_base;

    VkShaderModule vk_module;
};

struct shader_spirv_graphics_program_vk
{
    struct shader_spirv_graphics_program_variant_vk *variants;
    SIZE_T variants_size, variant_count;
};

struct shader_spirv_compute_program_vk
{
    VkShaderModule vk_module;
    VkPipeline vk_pipeline;
    VkPipelineLayout vk_pipeline_layout;
    VkDescriptorSetLayout vk_set_layout;
};

static void shader_spirv_handle_instruction(const struct wined3d_shader_instruction *ins)
{
}

static void shader_spirv_compile_arguments_init(struct shader_spirv_compile_arguments *args,
        const struct wined3d_context *context, const struct wined3d_shader *shader,
        const struct wined3d_state *state, unsigned int sample_count)
{
    const struct wined3d_shader *hull_shader;
    struct wined3d_rendertarget_view *rtv;
    unsigned int i;

    memset(args, 0, sizeof(*args));

    switch (shader->reg_maps.shader_version.type)
    {
        case WINED3D_SHADER_TYPE_DOMAIN:
            hull_shader = state->shader[WINED3D_SHADER_TYPE_HULL];
            args->u.tes.output_primitive = hull_shader->u.hs.tessellator_output_primitive;
            if (args->u.tes.output_primitive == WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CW)
                args->u.tes.output_primitive = WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CCW;
            else if (args->u.tes.output_primitive == WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CCW)
                args->u.tes.output_primitive = WINED3D_TESSELLATOR_OUTPUT_TRIANGLE_CW;
            args->u.tes.partitioning = hull_shader->u.hs.tessellator_partitioning;
            break;

        case WINED3D_SHADER_TYPE_PIXEL:
            for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
            {
                if (!(rtv = state->fb.render_targets[i]) || rtv->format->id == WINED3DFMT_NULL)
                    continue;
                if (rtv->format->id == WINED3DFMT_A8_UNORM && !is_identity_fixup(rtv->format->color_fixup))
                    args->u.fs.alpha_swizzle |= 1u << i;
            }
            args->u.fs.sample_count = sample_count;
            break;

        default:
            break;
    }
}

static VkShaderModule shader_spirv_compile(struct wined3d_context_vk *context_vk,
        struct wined3d_shader *shader, const struct shader_spirv_compile_arguments *args,
        const struct shader_spirv_resource_bindings *bindings)
{
    FIXME("Not implemented.\n");

    return VK_NULL_HANDLE;
}

static struct shader_spirv_graphics_program_variant_vk *shader_spirv_find_graphics_program_variant_vk(
        struct shader_spirv_priv *priv, struct wined3d_context_vk *context_vk, struct wined3d_shader *shader,
        const struct wined3d_state *state, const struct shader_spirv_resource_bindings *bindings)
{
    size_t binding_base = bindings->binding_base[shader->reg_maps.shader_version.type];
    struct shader_spirv_graphics_program_variant_vk *variant_vk;
    struct shader_spirv_graphics_program_vk *program_vk;
    struct shader_spirv_compile_arguments args;
    size_t variant_count, i;

    shader_spirv_compile_arguments_init(&args, &context_vk->c, shader, state, context_vk->sample_count);

    if (!(program_vk = shader->backend_data))
    {
        if (!(program_vk = heap_alloc_zero(sizeof(*program_vk))))
            return NULL;
        shader->backend_data = program_vk;
    }

    variant_count = program_vk->variant_count;
    for (i = 0; i < variant_count; ++i)
    {
        variant_vk = &program_vk->variants[i];
        if (variant_vk->binding_base == binding_base
                && !memcmp(&variant_vk->compile_args, &args, sizeof(args)))
            return variant_vk;
    }

    if (!wined3d_array_reserve((void **)&program_vk->variants, &program_vk->variants_size,
            variant_count + 1, sizeof(*program_vk->variants)))
        return NULL;

    variant_vk = &program_vk->variants[variant_count];
    variant_vk->compile_args = args;
    variant_vk->binding_base = binding_base;
    if (!(variant_vk->vk_module = shader_spirv_compile(context_vk, shader, &args, bindings)))
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
    VkResult vr;

    if ((program = shader->backend_data))
        return program;

    if (!(program = heap_alloc(sizeof(*program))))
        return NULL;

    if (!(program->vk_module = shader_spirv_compile(context_vk, shader, NULL, bindings)))
    {
        heap_free(program);
        return NULL;
    }

    if (!(layout = wined3d_context_vk_get_pipeline_layout(context_vk,
            bindings->vk_bindings, bindings->vk_binding_count)))
    {
        VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
        heap_free(program);
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
        heap_free(program);
        return NULL;
    }

    shader->backend_data = program;

    return program;
}

static void shader_spirv_resource_bindings_cleanup(struct shader_spirv_resource_bindings *bindings)
{
    heap_free(bindings->vk_bindings);
}

static bool shader_spirv_resource_bindings_add_binding(struct shader_spirv_resource_bindings *bindings,
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

static bool shader_spirv_resource_bindings_init(struct shader_spirv_resource_bindings *bindings,
        struct wined3d_shader_resource_bindings *wined3d_bindings,
        const struct wined3d_state *state, uint32_t shader_mask)
{
    const struct wined3d_shader_resource_info *resource_info;
    const struct wined3d_shader_reg_maps *reg_maps;
    enum wined3d_shader_type shader_type;
    VkDescriptorType vk_descriptor_type;
    size_t binding_idx, register_idx;
    VkShaderStageFlagBits vk_stage;
    struct wined3d_shader *shader;
    unsigned int i;
    uint32_t map;

    bindings->vk_binding_count = 0;
    wined3d_bindings->count = 0;

    for (shader_type = 0; shader_type < WINED3D_SHADER_TYPE_COUNT; ++shader_type)
    {
        bindings->binding_base[shader_type] = bindings->vk_binding_count;

        if (!(shader_mask & (1u << shader_type)) || !(shader = state->shader[shader_type]))
            continue;

        reg_maps = &shader->reg_maps;
        vk_stage = vk_shader_stage_from_wined3d(shader_type);

        map = reg_maps->cb_map;
        while (map)
        {
            register_idx = wined3d_bit_scan(&map);

            if (!shader_spirv_resource_bindings_add_binding(bindings,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vk_stage, &binding_idx))
                return false;
            if (!wined3d_shader_resource_bindings_add_binding(wined3d_bindings,
                    shader_type, WINED3D_SHADER_DESCRIPTOR_TYPE_CBV, register_idx,
                    WINED3D_SHADER_RESOURCE_BUFFER, WINED3D_DATA_UINT, binding_idx))
                return false;
        }

        for (i = 0; i < ARRAY_SIZE(reg_maps->resource_map); ++i)
        {
            map = reg_maps->resource_map[i];
            while (map)
            {
                register_idx = (i << 5) + wined3d_bit_scan(&map);
                resource_info = &reg_maps->resource_info[register_idx];
                if (resource_info->type == WINED3D_SHADER_RESOURCE_BUFFER)
                    vk_descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                else
                    vk_descriptor_type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

                if (!shader_spirv_resource_bindings_add_binding(bindings,
                        vk_descriptor_type, vk_stage, &binding_idx))
                    return false;
                if (!wined3d_shader_resource_bindings_add_binding(wined3d_bindings,
                        shader_type, WINED3D_SHADER_DESCRIPTOR_TYPE_SRV, register_idx,
                        resource_info->type, resource_info->data_type, binding_idx))
                    return false;
            }
        }

        for (register_idx = 0; register_idx < ARRAY_SIZE(reg_maps->uav_resource_info); ++register_idx)
        {
            resource_info = &reg_maps->uav_resource_info[register_idx];
            if (resource_info->type == WINED3D_SHADER_RESOURCE_NONE)
                continue;
            if (resource_info->type == WINED3D_SHADER_RESOURCE_BUFFER)
                vk_descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            else
                vk_descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

            if (!shader_spirv_resource_bindings_add_binding(bindings,
                    vk_descriptor_type, vk_stage, &binding_idx))
                return false;
            if (!wined3d_shader_resource_bindings_add_binding(wined3d_bindings,
                    shader_type, WINED3D_SHADER_DESCRIPTOR_TYPE_UAV, register_idx,
                    resource_info->type, resource_info->data_type, binding_idx))
                return false;

            if (reg_maps->uav_counter_mask & (1u << register_idx))
            {
                if (!shader_spirv_resource_bindings_add_binding(bindings,
                        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, vk_stage, &binding_idx))
                    return false;
                if (!wined3d_shader_resource_bindings_add_binding(wined3d_bindings,
                        shader_type, WINED3D_SHADER_DESCRIPTOR_TYPE_UAV_COUNTER, register_idx,
                        WINED3D_SHADER_RESOURCE_BUFFER, WINED3D_DATA_UINT, binding_idx))
                    return false;
            }
        }

        map = 0;
        for (i = 0; i < reg_maps->sampler_map.count; ++i)
        {
            if (reg_maps->sampler_map.entries[i].sampler_idx != WINED3D_SAMPLER_DEFAULT)
                map |= 1u << reg_maps->sampler_map.entries[i].sampler_idx;
        }

        while (map)
        {
            register_idx = wined3d_bit_scan(&map);

            if (!shader_spirv_resource_bindings_add_binding(bindings,
                    VK_DESCRIPTOR_TYPE_SAMPLER, vk_stage, &binding_idx))
                return false;
            if (!wined3d_shader_resource_bindings_add_binding(wined3d_bindings,
                    shader_type, WINED3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, register_idx,
                    WINED3D_SHADER_RESOURCE_NONE, WINED3D_DATA_SAMPLER, binding_idx))
                return false;
        }
    }

    return true;
}

static void shader_spirv_precompile(void *shader_priv, struct wined3d_shader *shader)
{
    WARN("Not implemented.\n");
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
    memcpy(binding_base, bindings->binding_base, sizeof(bindings));
    if (!shader_spirv_resource_bindings_init(bindings, &context_vk->graphics.bindings,
            state, ~(1u << WINED3D_SHADER_TYPE_COMPUTE)))
    {
        ERR("Failed to initialise shader resource bindings.\n");
        goto fail;
    }

    layout_vk = wined3d_context_vk_get_pipeline_layout(context_vk, bindings->vk_bindings, bindings->vk_binding_count);
    context_vk->graphics.vk_set_layout = layout_vk->vk_set_layout;
    context_vk->graphics.vk_pipeline_layout = layout_vk->vk_pipeline_layout;

    for (shader_type = 0; shader_type < ARRAY_SIZE(context_vk->graphics.vk_modules); ++shader_type)
    {
        if (!(context->shader_update_mask & (1u << shader_type)) && (!context_vk->graphics.vk_modules[shader_type]
                || binding_base[shader_type] == bindings->binding_base[shader_type]))
            continue;

        if (!(shader = state->shader[shader_type]))
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
    struct wined3d_vk_info *vk_info = &device_vk->vk_info;

    shader_spirv_invalidate_contexts_compute_program(&device_vk->d, program);
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, program->vk_pipeline, NULL));
    VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
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

static void spirv_vertex_pipe_vk_vp_enable(const struct wined3d_context *context, BOOL enable)
{
    /* Nothing to do. */
}

static void spirv_vertex_pipe_vk_vp_get_caps(const struct wined3d_adapter *adapter, struct wined3d_vertex_caps *caps)
{
    memset(caps, 0, sizeof(*caps));
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
