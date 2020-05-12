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

struct shader_spirv_resource_bindings
{
    VkDescriptorSetLayoutBinding *vk_bindings;
    SIZE_T vk_bindings_size, vk_binding_count;
};

struct shader_spirv_priv
{
    const struct wined3d_vertex_pipe_ops *vertex_pipe;
    const struct wined3d_fragment_pipe_ops *fragment_pipe;
    bool ffp_proj_control;

    struct shader_spirv_resource_bindings bindings;
};

struct shader_spirv_compute_program_vk
{
    VkShaderModule vk_module;
    VkPipeline vk_pipeline;
};

static void shader_spirv_handle_instruction(const struct wined3d_shader_instruction *ins)
{
}

static VkShaderModule shader_spirv_compile(struct wined3d_context_vk *context_vk,
        struct wined3d_shader *shader, const struct shader_spirv_resource_bindings *bindings)
{
    FIXME("Not implemented.\n");

    return VK_NULL_HANDLE;
}

static struct shader_spirv_compute_program_vk *shader_spirv_find_compute_program_vk(struct shader_spirv_priv *priv,
        struct wined3d_context_vk *context_vk, struct wined3d_shader *shader,
        const struct shader_spirv_resource_bindings *bindings)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct shader_spirv_compute_program_vk *program;
    VkDescriptorSetLayoutCreateInfo set_layout_desc;
    VkPipelineLayoutCreateInfo pipeline_layout_desc;
    VkComputePipelineCreateInfo pipeline_info;
    VkResult vr;

    if ((program = shader->backend_data))
        return program;

    if (!(program = heap_alloc(sizeof(*program))))
        return NULL;

    if (!(program->vk_module = shader_spirv_compile(context_vk, shader, bindings)))
    {
        heap_free(program);
        return NULL;
    }

    if (!context_vk->vk_pipeline_layout)
    {
        set_layout_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set_layout_desc.pNext = NULL;
        set_layout_desc.flags = 0;
        set_layout_desc.bindingCount = 0;
        set_layout_desc.pBindings = NULL;
        if ((vr = VK_CALL(vkCreateDescriptorSetLayout(device_vk->vk_device,
                &set_layout_desc, NULL, &context_vk->vk_set_layout))) < 0)
        {
            WARN("Failed to create Vulkan descriptor set layout, vr %s.\n", wined3d_debug_vkresult(vr));
            VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
            heap_free(program);
            return NULL;
        }

        pipeline_layout_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_desc.pNext = NULL;
        pipeline_layout_desc.flags = 0;
        pipeline_layout_desc.setLayoutCount = 1;
        pipeline_layout_desc.pSetLayouts = &context_vk->vk_set_layout;
        pipeline_layout_desc.pushConstantRangeCount = 0;
        pipeline_layout_desc.pPushConstantRanges = NULL;

        if ((vr = VK_CALL(vkCreatePipelineLayout(device_vk->vk_device,
                &pipeline_layout_desc, NULL, &context_vk->vk_pipeline_layout))) < 0)
        {
            WARN("Failed to create Vulkan pipeline layout, vr %s.\n", wined3d_debug_vkresult(vr));
            VK_CALL(vkDestroyDescriptorSetLayout(device_vk->vk_device, context_vk->vk_set_layout, NULL));
            VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
            heap_free(program);
            return NULL;
        }
    }

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
    pipeline_info.layout = context_vk->vk_pipeline_layout;
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
    struct shader_spirv_priv *priv = shader_priv;

    priv->vertex_pipe->vp_enable(context, !use_vs(state));
    priv->fragment_pipe->fp_enable(context, !use_ps(state));
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
        context_vk->compute.vk_pipeline = program->vk_pipeline;
    else
        context_vk->compute.vk_pipeline = VK_NULL_HANDLE;
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

static void shader_spirv_destroy(struct wined3d_shader *shader)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(shader->device);
    struct wined3d_vk_info *vk_info = &device_vk->vk_info;
    struct shader_spirv_compute_program_vk *program;

    if (!(program = shader->backend_data))
        return;

    shader_spirv_invalidate_contexts_compute_program(&device_vk->d, program);
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, program->vk_pipeline, NULL));
    VK_CALL(vkDestroyShaderModule(device_vk->vk_device, program->vk_module, NULL));
    shader->backend_data = NULL;
    heap_free(program);
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
