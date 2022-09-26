/*
 * Copyright 2016 Józef Kucia for CodeWeavers
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

#include "vkd3d_private.h"
#include "vkd3d_version.h"

struct vkd3d_struct
{
    enum vkd3d_structure_type type;
    const void *next;
};

#define vkd3d_find_struct(c, t) vkd3d_find_struct_(c, VKD3D_STRUCTURE_TYPE_##t)
static const void *vkd3d_find_struct_(const struct vkd3d_struct *chain,
        enum vkd3d_structure_type type)
{
    while (chain)
    {
        if (chain->type == type)
            return chain;

        chain = chain->next;
    }

    return NULL;
}

static uint32_t vkd3d_get_vk_version(void)
{
    int major, minor;

    vkd3d_parse_version(PACKAGE_VERSION, &major, &minor);
    return VK_MAKE_VERSION(major, minor, 0);
}

struct vkd3d_optional_extension_info
{
    const char *extension_name;
    ptrdiff_t vulkan_info_offset;
    bool is_debug_only;
};

#define VK_EXTENSION(name, member) \
        {VK_ ## name ## _EXTENSION_NAME, offsetof(struct vkd3d_vulkan_info, member)}
#define VK_DEBUG_EXTENSION(name, member) \
        {VK_ ## name ## _EXTENSION_NAME, offsetof(struct vkd3d_vulkan_info, member), true}

static const struct vkd3d_optional_extension_info optional_instance_extensions[] =
{
    /* KHR extensions */
    VK_EXTENSION(KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2, KHR_get_physical_device_properties2),
    /* EXT extensions */
    VK_DEBUG_EXTENSION(EXT_DEBUG_REPORT, EXT_debug_report),
};

static const char * const required_device_extensions[] =
{
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
};

static const struct vkd3d_optional_extension_info optional_device_extensions[] =
{
    /* KHR extensions */
    VK_EXTENSION(KHR_DEDICATED_ALLOCATION, KHR_dedicated_allocation),
    VK_EXTENSION(KHR_DRAW_INDIRECT_COUNT, KHR_draw_indirect_count),
    VK_EXTENSION(KHR_GET_MEMORY_REQUIREMENTS_2, KHR_get_memory_requirements2),
    VK_EXTENSION(KHR_IMAGE_FORMAT_LIST, KHR_image_format_list),
    VK_EXTENSION(KHR_MAINTENANCE3, KHR_maintenance3),
    VK_EXTENSION(KHR_PUSH_DESCRIPTOR, KHR_push_descriptor),
    VK_EXTENSION(KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE, KHR_sampler_mirror_clamp_to_edge),
    VK_EXTENSION(KHR_TIMELINE_SEMAPHORE, KHR_timeline_semaphore),
    /* EXT extensions */
    VK_EXTENSION(EXT_CALIBRATED_TIMESTAMPS, EXT_calibrated_timestamps),
    VK_EXTENSION(EXT_CONDITIONAL_RENDERING, EXT_conditional_rendering),
    VK_EXTENSION(EXT_DEBUG_MARKER, EXT_debug_marker),
    VK_EXTENSION(EXT_DEPTH_CLIP_ENABLE, EXT_depth_clip_enable),
    VK_EXTENSION(EXT_DESCRIPTOR_INDEXING, EXT_descriptor_indexing),
    VK_EXTENSION(EXT_ROBUSTNESS_2, EXT_robustness2),
    VK_EXTENSION(EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION, EXT_shader_demote_to_helper_invocation),
    VK_EXTENSION(EXT_SHADER_STENCIL_EXPORT, EXT_shader_stencil_export),
    VK_EXTENSION(EXT_TEXEL_BUFFER_ALIGNMENT, EXT_texel_buffer_alignment),
    VK_EXTENSION(EXT_TRANSFORM_FEEDBACK, EXT_transform_feedback),
    VK_EXTENSION(EXT_VERTEX_ATTRIBUTE_DIVISOR, EXT_vertex_attribute_divisor),
};

static HRESULT vkd3d_create_vk_descriptor_heap_layout(struct d3d12_device *device, unsigned int index)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flags_info;
    VkDescriptorSetLayoutCreateInfo set_desc;
    VkDescriptorBindingFlagsEXT set_flags;
    VkDescriptorSetLayoutBinding binding;
    VkResult vr;

    binding.binding = 0;
    binding.descriptorType = device->vk_descriptor_heap_layouts[index].type;
    binding.descriptorCount = device->vk_descriptor_heap_layouts[index].count;
    binding.stageFlags = VK_SHADER_STAGE_ALL;
    binding.pImmutableSamplers = NULL;

    set_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_desc.pNext = &flags_info;
    set_desc.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    set_desc.bindingCount = 1;
    set_desc.pBindings = &binding;

    set_flags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
            | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT
            | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

    flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    flags_info.pNext = NULL;
    flags_info.bindingCount = 1;
    flags_info.pBindingFlags = &set_flags;

    if ((vr = VK_CALL(vkCreateDescriptorSetLayout(device->vk_device, &set_desc, NULL,
            &device->vk_descriptor_heap_layouts[index].vk_set_layout))) < 0)
    {
        WARN("Failed to create Vulkan descriptor set layout, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static void vkd3d_vk_descriptor_heap_layouts_cleanup(struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    enum vkd3d_vk_descriptor_set_index set;

    for (set = 0; set < ARRAY_SIZE(device->vk_descriptor_heap_layouts); ++set)
        VK_CALL(vkDestroyDescriptorSetLayout(device->vk_device, device->vk_descriptor_heap_layouts[set].vk_set_layout,
                NULL));
}

static HRESULT vkd3d_vk_descriptor_heap_layouts_init(struct d3d12_device *device)
{
    static const struct vkd3d_vk_descriptor_heap_layout vk_descriptor_heap_layouts[VKD3D_SET_INDEX_COUNT] =
    {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, false, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, false, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        {VK_DESCRIPTOR_TYPE_SAMPLER, false, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER},
        /* UAV counters */
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
    };
    const struct vkd3d_device_descriptor_limits *limits = &device->vk_info.descriptor_limits;
    enum vkd3d_vk_descriptor_set_index set;
    HRESULT hr;

    for (set = 0; set < ARRAY_SIZE(device->vk_descriptor_heap_layouts); ++set)
        device->vk_descriptor_heap_layouts[set] = vk_descriptor_heap_layouts[set];

    if (!device->use_vk_heaps)
        return S_OK;

    for (set = 0; set < ARRAY_SIZE(device->vk_descriptor_heap_layouts); ++set)
    {
        switch (device->vk_descriptor_heap_layouts[set].type)
        {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                device->vk_descriptor_heap_layouts[set].count = limits->uniform_buffer_max_descriptors;
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                device->vk_descriptor_heap_layouts[set].count = limits->sampled_image_max_descriptors;
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                device->vk_descriptor_heap_layouts[set].count = limits->storage_image_max_descriptors;
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                device->vk_descriptor_heap_layouts[set].count = limits->sampler_max_descriptors;
                break;
            default:
                ERR("Unhandled descriptor type %#x.\n", device->vk_descriptor_heap_layouts[set].type);
                break;
        }

        if (FAILED(hr = vkd3d_create_vk_descriptor_heap_layout(device, set)))
        {
            vkd3d_vk_descriptor_heap_layouts_cleanup(device);
            return hr;
        }
    }

    return S_OK;
}

static unsigned int get_spec_version(const VkExtensionProperties *extensions,
        unsigned int count, const char *extension_name)
{
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (!strcmp(extensions[i].extensionName, extension_name))
            return extensions[i].specVersion;
    }
    return 0;
}

static bool is_extension_disabled(const char *extension_name)
{
    const char *disabled_extensions;

    if (!(disabled_extensions = getenv("VKD3D_DISABLE_EXTENSIONS")))
        return false;

    return vkd3d_debug_list_has_member(disabled_extensions, extension_name);
}

static bool has_extension(const VkExtensionProperties *extensions,
        unsigned int count, const char *extension_name)
{
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (is_extension_disabled(extension_name))
        {
            WARN("Extension %s is disabled.\n", debugstr_a(extension_name));
            continue;
        }
        if (!strcmp(extensions[i].extensionName, extension_name))
            return true;
    }
    return false;
}

static unsigned int vkd3d_check_extensions(const VkExtensionProperties *extensions, unsigned int count,
        const char * const *required_extensions, unsigned int required_extension_count,
        const struct vkd3d_optional_extension_info *optional_extensions, unsigned int optional_extension_count,
        const char * const *user_extensions, unsigned int user_extension_count,
        const char * const *optional_user_extensions, unsigned int optional_user_extension_count,
        bool *user_extension_supported, struct vkd3d_vulkan_info *vulkan_info, const char *extension_type,
        bool is_debug_enabled)
{
    unsigned int extension_count = 0;
    unsigned int i;

    for (i = 0; i < required_extension_count; ++i)
    {
        if (!has_extension(extensions, count, required_extensions[i]))
            ERR("Required %s extension %s is not supported.\n",
                    extension_type, debugstr_a(required_extensions[i]));
        ++extension_count;
    }

    for (i = 0; i < optional_extension_count; ++i)
    {
        const char *extension_name = optional_extensions[i].extension_name;
        ptrdiff_t offset = optional_extensions[i].vulkan_info_offset;
        bool *supported = (void *)((uintptr_t)vulkan_info + offset);

        if (!is_debug_enabled && optional_extensions[i].is_debug_only)
        {
            *supported = false;
            TRACE("Skipping debug-only extension %s.\n", debugstr_a(extension_name));
            continue;
        }

        if ((*supported = has_extension(extensions, count, extension_name)))
        {
            TRACE("Found %s extension.\n", debugstr_a(extension_name));
            ++extension_count;
        }
    }

    for (i = 0; i < user_extension_count; ++i)
    {
        if (!has_extension(extensions, count, user_extensions[i]))
            ERR("Required user %s extension %s is not supported.\n",
                    extension_type, debugstr_a(user_extensions[i]));
        ++extension_count;
    }

    assert(!optional_user_extension_count || user_extension_supported);
    for (i = 0; i < optional_user_extension_count; ++i)
    {
        if (has_extension(extensions, count, optional_user_extensions[i]))
        {
            user_extension_supported[i] = true;
            ++extension_count;
        }
        else
        {
            user_extension_supported[i] = false;
            WARN("Optional user %s extension %s is not supported.\n",
                    extension_type, debugstr_a(optional_user_extensions[i]));
        }
    }

    return extension_count;
}

static unsigned int vkd3d_append_extension(const char *extensions[],
        unsigned int extension_count, const char *extension_name)
{
    unsigned int i;

    /* avoid duplicates */
    for (i = 0; i < extension_count; ++i)
    {
        if (!strcmp(extensions[i], extension_name))
            return extension_count;
    }

    extensions[extension_count++] = extension_name;
    return extension_count;
}

static unsigned int vkd3d_enable_extensions(const char *extensions[],
        const char * const *required_extensions, unsigned int required_extension_count,
        const struct vkd3d_optional_extension_info *optional_extensions, unsigned int optional_extension_count,
        const char * const *user_extensions, unsigned int user_extension_count,
        const char * const *optional_user_extensions, unsigned int optional_user_extension_count,
        bool *user_extension_supported, const struct vkd3d_vulkan_info *vulkan_info)
{
    unsigned int extension_count = 0;
    unsigned int i;

    for (i = 0; i < required_extension_count; ++i)
    {
        extensions[extension_count++] = required_extensions[i];
    }
    for (i = 0; i < optional_extension_count; ++i)
    {
        ptrdiff_t offset = optional_extensions[i].vulkan_info_offset;
        const bool *supported = (void *)((uintptr_t)vulkan_info + offset);

        if (*supported)
            extensions[extension_count++] = optional_extensions[i].extension_name;
    }

    for (i = 0; i < user_extension_count; ++i)
    {
        extension_count = vkd3d_append_extension(extensions, extension_count, user_extensions[i]);
    }
    assert(!optional_user_extension_count || user_extension_supported);
    for (i = 0; i < optional_user_extension_count; ++i)
    {
        if (!user_extension_supported[i])
            continue;
        extension_count = vkd3d_append_extension(extensions, extension_count, optional_user_extensions[i]);
    }

    return extension_count;
}

static HRESULT vkd3d_init_instance_caps(struct vkd3d_instance *instance,
        const struct vkd3d_instance_create_info *create_info,
        uint32_t *instance_extension_count, bool **user_extension_supported)
{
    const struct vkd3d_vk_global_procs *vk_procs = &instance->vk_global_procs;
    const struct vkd3d_optional_instance_extensions_info *optional_extensions;
    struct vkd3d_vulkan_info *vulkan_info = &instance->vk_info;
    VkExtensionProperties *vk_extensions;
    uint32_t count;
    VkResult vr;

    memset(vulkan_info, 0, sizeof(*vulkan_info));
    *instance_extension_count = 0;

    if ((vr = vk_procs->vkEnumerateInstanceExtensionProperties(NULL, &count, NULL)) < 0)
    {
        ERR("Failed to enumerate instance extensions, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }
    if (!count)
        return S_OK;

    if (!(vk_extensions = vkd3d_calloc(count, sizeof(*vk_extensions))))
        return E_OUTOFMEMORY;

    TRACE("Enumerating %u instance extensions.\n", count);
    if ((vr = vk_procs->vkEnumerateInstanceExtensionProperties(NULL, &count, vk_extensions)) < 0)
    {
        ERR("Failed to enumerate instance extensions, vr %d.\n", vr);
        vkd3d_free(vk_extensions);
        return hresult_from_vk_result(vr);
    }

    optional_extensions = vkd3d_find_struct(create_info->next, OPTIONAL_INSTANCE_EXTENSIONS_INFO);
    if (optional_extensions && optional_extensions->extension_count)
    {
        if (!(*user_extension_supported = vkd3d_calloc(optional_extensions->extension_count, sizeof(bool))))
        {
            vkd3d_free(vk_extensions);
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *user_extension_supported = NULL;
    }

    *instance_extension_count = vkd3d_check_extensions(vk_extensions, count, NULL, 0,
            optional_instance_extensions, ARRAY_SIZE(optional_instance_extensions),
            create_info->instance_extensions, create_info->instance_extension_count,
            optional_extensions ? optional_extensions->extensions : NULL,
            optional_extensions ? optional_extensions->extension_count : 0,
            *user_extension_supported, vulkan_info, "instance",
            instance->config_flags & VKD3D_CONFIG_FLAG_VULKAN_DEBUG);

    vkd3d_free(vk_extensions);
    return S_OK;
}

static HRESULT vkd3d_init_vk_global_procs(struct vkd3d_instance *instance,
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr)
{
    HRESULT hr;

    if (!vkGetInstanceProcAddr)
    {
        if (!(instance->libvulkan = vkd3d_dlopen(SONAME_LIBVULKAN)))
        {
            ERR("Failed to load libvulkan: %s.\n", vkd3d_dlerror());
            return E_FAIL;
        }

        if (!(vkGetInstanceProcAddr = vkd3d_dlsym(instance->libvulkan, "vkGetInstanceProcAddr")))
        {
            ERR("Could not load function pointer for vkGetInstanceProcAddr().\n");
            vkd3d_dlclose(instance->libvulkan);
            instance->libvulkan = NULL;
            return E_FAIL;
        }
    }
    else
    {
        instance->libvulkan = NULL;
    }

    if (FAILED(hr = vkd3d_load_vk_global_procs(&instance->vk_global_procs, vkGetInstanceProcAddr)))
    {
        if (instance->libvulkan)
            vkd3d_dlclose(instance->libvulkan);
        instance->libvulkan = NULL;
        return hr;
    }

    return S_OK;
}

static VkBool32 VKAPI_PTR vkd3d_debug_report_callback(VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT object_type, uint64_t object, size_t location,
        int32_t message_code, const char *layer_prefix, const char *message, void *user_data)
{
    FIXME("%s\n", debugstr_a(message));
    return VK_FALSE;
}

static void vkd3d_init_debug_report(struct vkd3d_instance *instance)
{
    const struct vkd3d_vk_instance_procs *vk_procs = &instance->vk_procs;
    VkDebugReportCallbackCreateInfoEXT callback_info;
    VkInstance vk_instance = instance->vk_instance;
    VkDebugReportCallbackEXT callback;
    VkResult vr;

    callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    callback_info.pNext = NULL;
    callback_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    callback_info.pfnCallback = vkd3d_debug_report_callback;
    callback_info.pUserData = NULL;
    if ((vr = VK_CALL(vkCreateDebugReportCallbackEXT(vk_instance, &callback_info, NULL, &callback))) < 0)
    {
        WARN("Failed to create debug report callback, vr %d.\n", vr);
        return;
    }

    instance->vk_debug_callback = callback;
}

static const struct vkd3d_debug_option vkd3d_config_options[] =
{
    {"virtual_heaps", VKD3D_CONFIG_FLAG_VIRTUAL_HEAPS}, /* always use virtual descriptor heaps */
    {"vk_debug", VKD3D_CONFIG_FLAG_VULKAN_DEBUG}, /* enable Vulkan debug extensions */
};

static uint64_t vkd3d_init_config_flags(void)
{
    uint64_t config_flags;
    const char *config;

    config = getenv("VKD3D_CONFIG");
    config_flags = vkd3d_parse_debug_options(config, vkd3d_config_options, ARRAY_SIZE(vkd3d_config_options));

    if (config_flags)
        TRACE("VKD3D_CONFIG='%s'.\n", config);

    return config_flags;
}

/* TICKSPERSEC from Wine */
#define VKD3D_DEFAULT_HOST_TICKS_PER_SECOND 10000000

static HRESULT vkd3d_instance_init(struct vkd3d_instance *instance,
        const struct vkd3d_instance_create_info *create_info)
{
    const struct vkd3d_vk_global_procs *vk_global_procs = &instance->vk_global_procs;
    const struct vkd3d_optional_instance_extensions_info *optional_extensions;
    const struct vkd3d_application_info *vkd3d_application_info;
    const struct vkd3d_host_time_domain_info *time_domain_info;
    bool *user_extension_supported = NULL;
    VkApplicationInfo application_info;
    VkInstanceCreateInfo instance_info;
    char application_name[PATH_MAX];
    uint32_t extension_count;
    const char **extensions;
    VkInstance vk_instance;
    VkResult vr;
    HRESULT hr;

    TRACE("Build: " PACKAGE_STRING VKD3D_VCS_ID ".\n");

    if (!create_info->pfn_signal_event)
    {
        ERR("Invalid signal event function pointer.\n");
        return E_INVALIDARG;
    }
    if (!create_info->pfn_create_thread != !create_info->pfn_join_thread)
    {
        ERR("Invalid create/join thread function pointers.\n");
        return E_INVALIDARG;
    }
    if (create_info->wchar_size != 2 && create_info->wchar_size != 4)
    {
        ERR("Unexpected WCHAR size %zu.\n", create_info->wchar_size);
        return E_INVALIDARG;
    }

    instance->signal_event = create_info->pfn_signal_event;
    instance->create_thread = create_info->pfn_create_thread;
    instance->join_thread = create_info->pfn_join_thread;
    instance->wchar_size = create_info->wchar_size;

    instance->config_flags = vkd3d_init_config_flags();

    if (FAILED(hr = vkd3d_init_vk_global_procs(instance, create_info->pfn_vkGetInstanceProcAddr)))
    {
        ERR("Failed to initialize Vulkan global procs, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = vkd3d_init_instance_caps(instance, create_info,
            &extension_count, &user_extension_supported)))
    {
        if (instance->libvulkan)
            vkd3d_dlclose(instance->libvulkan);
        return hr;
    }

    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = NULL;
    application_info.pApplicationName = NULL;
    application_info.applicationVersion = 0;
    application_info.pEngineName = PACKAGE_NAME;
    application_info.engineVersion = vkd3d_get_vk_version();
    application_info.apiVersion = VK_API_VERSION_1_0;
    instance->api_version = VKD3D_API_VERSION_1_0;

    if ((vkd3d_application_info = vkd3d_find_struct(create_info->next, APPLICATION_INFO)))
    {
        if (vkd3d_application_info->application_name)
            application_info.pApplicationName = vkd3d_application_info->application_name;
        else if (vkd3d_get_program_name(application_name))
            application_info.pApplicationName = application_name;
        application_info.applicationVersion = vkd3d_application_info->application_version;
        if (vkd3d_application_info->engine_name)
        {
            application_info.pEngineName = vkd3d_application_info->engine_name;
            application_info.engineVersion = vkd3d_application_info->engine_version;
        }
        instance->api_version = vkd3d_application_info->api_version;
    }
    else if (vkd3d_get_program_name(application_name))
    {
        application_info.pApplicationName = application_name;
    }

    TRACE("Application: %s.\n", debugstr_a(application_info.pApplicationName));
    TRACE("vkd3d API version: %u.\n", instance->api_version);

    if (!(extensions = vkd3d_calloc(extension_count, sizeof(*extensions))))
    {
        if (instance->libvulkan)
            vkd3d_dlclose(instance->libvulkan);
        vkd3d_free(user_extension_supported);
        return E_OUTOFMEMORY;
    }

    optional_extensions = vkd3d_find_struct(create_info->next, OPTIONAL_INSTANCE_EXTENSIONS_INFO);

    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &application_info;
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = NULL;
    instance_info.enabledExtensionCount = vkd3d_enable_extensions(extensions, NULL, 0,
            optional_instance_extensions, ARRAY_SIZE(optional_instance_extensions),
            create_info->instance_extensions, create_info->instance_extension_count,
            optional_extensions ? optional_extensions->extensions : NULL,
            optional_extensions ? optional_extensions->extension_count : 0,
            user_extension_supported, &instance->vk_info);
    instance_info.ppEnabledExtensionNames = extensions;
    vkd3d_free(user_extension_supported);

    vr = vk_global_procs->vkCreateInstance(&instance_info, NULL, &vk_instance);
    vkd3d_free(extensions);
    if (vr < 0)
    {
        ERR("Failed to create Vulkan instance, vr %d.\n", vr);
        if (instance->libvulkan)
            vkd3d_dlclose(instance->libvulkan);
        return hresult_from_vk_result(vr);
    }

    if (FAILED(hr = vkd3d_load_vk_instance_procs(&instance->vk_procs, vk_global_procs, vk_instance)))
    {
        ERR("Failed to load instance procs, hr %#x.\n", hr);
        if (instance->vk_procs.vkDestroyInstance)
            instance->vk_procs.vkDestroyInstance(vk_instance, NULL);
        if (instance->libvulkan)
            vkd3d_dlclose(instance->libvulkan);
        return hr;
    }

    if ((time_domain_info = vkd3d_find_struct(create_info->next, HOST_TIME_DOMAIN_INFO)))
        instance->host_ticks_per_second = time_domain_info->ticks_per_second;
    else
        instance->host_ticks_per_second = VKD3D_DEFAULT_HOST_TICKS_PER_SECOND;

    instance->vk_instance = vk_instance;

    TRACE("Created Vulkan instance %p.\n", vk_instance);

    instance->refcount = 1;

    instance->vk_debug_callback = VK_NULL_HANDLE;
    if (instance->vk_info.EXT_debug_report)
        vkd3d_init_debug_report(instance);

    return S_OK;
}

HRESULT vkd3d_create_instance(const struct vkd3d_instance_create_info *create_info,
        struct vkd3d_instance **instance)
{
    struct vkd3d_instance *object;
    HRESULT hr;

    TRACE("create_info %p, instance %p.\n", create_info, instance);

    if (!create_info || !instance)
        return E_INVALIDARG;
    if (create_info->type != VKD3D_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
    {
        WARN("Invalid structure type %#x.\n", create_info->type);
        return E_INVALIDARG;
    }

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = vkd3d_instance_init(object, create_info)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created instance %p.\n", object);

    *instance = object;

    return S_OK;
}

static void vkd3d_destroy_instance(struct vkd3d_instance *instance)
{
    const struct vkd3d_vk_instance_procs *vk_procs = &instance->vk_procs;
    VkInstance vk_instance = instance->vk_instance;

    if (instance->vk_debug_callback)
        VK_CALL(vkDestroyDebugReportCallbackEXT(vk_instance, instance->vk_debug_callback, NULL));

    VK_CALL(vkDestroyInstance(vk_instance, NULL));

    if (instance->libvulkan)
        vkd3d_dlclose(instance->libvulkan);

    vkd3d_free(instance);
}

ULONG vkd3d_instance_incref(struct vkd3d_instance *instance)
{
    ULONG refcount = InterlockedIncrement(&instance->refcount);

    TRACE("%p increasing refcount to %u.\n", instance, refcount);

    return refcount;
}

ULONG vkd3d_instance_decref(struct vkd3d_instance *instance)
{
    ULONG refcount = InterlockedDecrement(&instance->refcount);

    TRACE("%p decreasing refcount to %u.\n", instance, refcount);

    if (!refcount)
        vkd3d_destroy_instance(instance);

    return refcount;
}

VkInstance vkd3d_instance_get_vk_instance(struct vkd3d_instance *instance)
{
    return instance->vk_instance;
}

struct vkd3d_physical_device_info
{
    /* properties */
    VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties;
    VkPhysicalDeviceMaintenance3Properties maintenance3_properties;
    VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT texel_buffer_alignment_properties;
    VkPhysicalDeviceTransformFeedbackPropertiesEXT xfb_properties;
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT vertex_divisor_properties;

    VkPhysicalDeviceProperties2KHR properties2;

    /* features */
    VkPhysicalDeviceConditionalRenderingFeaturesEXT conditional_rendering_features;
    VkPhysicalDeviceDepthClipEnableFeaturesEXT depth_clip_features;
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing_features;
    VkPhysicalDeviceRobustness2FeaturesEXT robustness2_features;
    VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT demote_features;
    VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT texel_buffer_alignment_features;
    VkPhysicalDeviceTransformFeedbackFeaturesEXT xfb_features;
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT vertex_divisor_features;
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timeline_semaphore_features;

    VkPhysicalDeviceFeatures2 features2;
};

static void vkd3d_physical_device_info_init(struct vkd3d_physical_device_info *info, struct d3d12_device *device)
{
    const struct vkd3d_vk_instance_procs *vk_procs = &device->vkd3d_instance->vk_procs;
    VkPhysicalDeviceConditionalRenderingFeaturesEXT *conditional_rendering_features;
    VkPhysicalDeviceDescriptorIndexingPropertiesEXT *descriptor_indexing_properties;
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT *vertex_divisor_properties;
    VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT *buffer_alignment_properties;
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT *descriptor_indexing_features;
    VkPhysicalDeviceRobustness2FeaturesEXT *robustness2_features;
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *vertex_divisor_features;
    VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT *buffer_alignment_features;
    VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT *demote_features;
    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR *timeline_semaphore_features;
    VkPhysicalDeviceDepthClipEnableFeaturesEXT *depth_clip_features;
    VkPhysicalDeviceMaintenance3Properties *maintenance3_properties;
    VkPhysicalDeviceTransformFeedbackPropertiesEXT *xfb_properties;
    VkPhysicalDevice physical_device = device->vk_physical_device;
    VkPhysicalDeviceTransformFeedbackFeaturesEXT *xfb_features;
    struct vkd3d_vulkan_info *vulkan_info = &device->vk_info;

    memset(info, 0, sizeof(*info));
    conditional_rendering_features = &info->conditional_rendering_features;
    depth_clip_features = &info->depth_clip_features;
    descriptor_indexing_features = &info->descriptor_indexing_features;
    robustness2_features = &info->robustness2_features;
    descriptor_indexing_properties = &info->descriptor_indexing_properties;
    maintenance3_properties = &info->maintenance3_properties;
    demote_features = &info->demote_features;
    buffer_alignment_features = &info->texel_buffer_alignment_features;
    buffer_alignment_properties = &info->texel_buffer_alignment_properties;
    vertex_divisor_features = &info->vertex_divisor_features;
    vertex_divisor_properties = &info->vertex_divisor_properties;
    timeline_semaphore_features = &info->timeline_semaphore_features;
    xfb_features = &info->xfb_features;
    xfb_properties = &info->xfb_properties;

    info->features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    conditional_rendering_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
    vk_prepend_struct(&info->features2, conditional_rendering_features);
    depth_clip_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
    vk_prepend_struct(&info->features2, depth_clip_features);
    descriptor_indexing_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    vk_prepend_struct(&info->features2, descriptor_indexing_features);
    robustness2_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    vk_prepend_struct(&info->features2, robustness2_features);
    demote_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT;
    vk_prepend_struct(&info->features2, demote_features);
    buffer_alignment_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT;
    vk_prepend_struct(&info->features2, buffer_alignment_features);
    xfb_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
    vk_prepend_struct(&info->features2, xfb_features);
    vertex_divisor_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT;
    vk_prepend_struct(&info->features2, vertex_divisor_features);
    timeline_semaphore_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
    vk_prepend_struct(&info->features2, timeline_semaphore_features);

    if (vulkan_info->KHR_get_physical_device_properties2)
        VK_CALL(vkGetPhysicalDeviceFeatures2KHR(physical_device, &info->features2));
    else
        VK_CALL(vkGetPhysicalDeviceFeatures(physical_device, &info->features2.features));

    info->properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

    maintenance3_properties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
    vk_prepend_struct(&info->properties2, maintenance3_properties);
    descriptor_indexing_properties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;
    vk_prepend_struct(&info->properties2, descriptor_indexing_properties);
    buffer_alignment_properties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES_EXT;
    vk_prepend_struct(&info->properties2, buffer_alignment_properties);
    xfb_properties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT;
    vk_prepend_struct(&info->properties2, xfb_properties);
    vertex_divisor_properties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT;
    vk_prepend_struct(&info->properties2, vertex_divisor_properties);

    if (vulkan_info->KHR_get_physical_device_properties2)
        VK_CALL(vkGetPhysicalDeviceProperties2KHR(physical_device, &info->properties2));
    else
        VK_CALL(vkGetPhysicalDeviceProperties(physical_device, &info->properties2.properties));
}

static void vkd3d_trace_physical_device_properties(const VkPhysicalDeviceProperties *properties)
{
    const uint32_t driver_version = properties->driverVersion;
    const uint32_t api_version = properties->apiVersion;

    TRACE("Device name: %s.\n", properties->deviceName);
    TRACE("Vendor ID: %#x, Device ID: %#x.\n", properties->vendorID, properties->deviceID);
    TRACE("Driver version: %#x (%u.%u.%u, %u.%u.%u.%u).\n", driver_version,
            VK_VERSION_MAJOR(driver_version), VK_VERSION_MINOR(driver_version), VK_VERSION_PATCH(driver_version),
            driver_version >> 22, (driver_version >> 14) & 0xff, (driver_version >> 6) & 0xff, driver_version & 0x3f);
    TRACE("API version: %u.%u.%u.\n",
            VK_VERSION_MAJOR(api_version), VK_VERSION_MINOR(api_version), VK_VERSION_PATCH(api_version));
}

static void vkd3d_trace_physical_device(VkPhysicalDevice device,
        const struct vkd3d_physical_device_info *info,
        const struct vkd3d_vk_instance_procs *vk_procs)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkQueueFamilyProperties *queue_properties;
    unsigned int i, j;
    uint32_t count;

    vkd3d_trace_physical_device_properties(&info->properties2.properties);

    VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL));
    TRACE("Queue families [%u]:\n", count);

    if (!(queue_properties = vkd3d_calloc(count, sizeof(VkQueueFamilyProperties))))
        return;
    VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queue_properties));

    for (i = 0; i < count; ++i)
    {
        TRACE(" Queue family [%u]: flags %s, count %u, timestamp bits %u, image transfer granularity %s.\n",
                i, debug_vk_queue_flags(queue_properties[i].queueFlags),
                queue_properties[i].queueCount, queue_properties[i].timestampValidBits,
                debug_vk_extent_3d(queue_properties[i].minImageTransferGranularity));
    }
    vkd3d_free(queue_properties);

    VK_CALL(vkGetPhysicalDeviceMemoryProperties(device, &memory_properties));
    for (i = 0; i < memory_properties.memoryHeapCount; ++i)
    {
        const VkMemoryHeap *heap = &memory_properties.memoryHeaps[i];
        TRACE("Memory heap [%u]: size %#"PRIx64" (%"PRIu64" MiB), flags %s, memory types:\n",
                i, heap->size, heap->size / 1024 / 1024, debug_vk_memory_heap_flags(heap->flags));
        for (j = 0; j < memory_properties.memoryTypeCount; ++j)
        {
            const VkMemoryType *type = &memory_properties.memoryTypes[j];
            if (type->heapIndex != i)
                continue;
            TRACE("  Memory type [%u]: flags %s.\n", j, debug_vk_memory_property_flags(type->propertyFlags));
        }
    }
}

static void vkd3d_trace_physical_device_limits(const struct vkd3d_physical_device_info *info)
{
    const VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT *divisor_properties;
    const VkPhysicalDeviceLimits *limits = &info->properties2.properties.limits;
    const VkPhysicalDeviceDescriptorIndexingPropertiesEXT *descriptor_indexing;
    const VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT *buffer_alignment;
    const VkPhysicalDeviceMaintenance3Properties *maintenance3;
    const VkPhysicalDeviceTransformFeedbackPropertiesEXT *xfb;

    TRACE("Device limits:\n");
    TRACE("  maxImageDimension1D: %u.\n", limits->maxImageDimension1D);
    TRACE("  maxImageDimension2D: %u.\n", limits->maxImageDimension2D);
    TRACE("  maxImageDimension3D: %u.\n", limits->maxImageDimension3D);
    TRACE("  maxImageDimensionCube: %u.\n", limits->maxImageDimensionCube);
    TRACE("  maxImageArrayLayers: %u.\n", limits->maxImageArrayLayers);
    TRACE("  maxTexelBufferElements: %u.\n", limits->maxTexelBufferElements);
    TRACE("  maxUniformBufferRange: %u.\n", limits->maxUniformBufferRange);
    TRACE("  maxStorageBufferRange: %u.\n", limits->maxStorageBufferRange);
    TRACE("  maxPushConstantsSize: %u.\n", limits->maxPushConstantsSize);
    TRACE("  maxMemoryAllocationCount: %u.\n", limits->maxMemoryAllocationCount);
    TRACE("  maxSamplerAllocationCount: %u.\n", limits->maxSamplerAllocationCount);
    TRACE("  bufferImageGranularity: %#"PRIx64".\n", limits->bufferImageGranularity);
    TRACE("  sparseAddressSpaceSize: %#"PRIx64".\n", limits->sparseAddressSpaceSize);
    TRACE("  maxBoundDescriptorSets: %u.\n", limits->maxBoundDescriptorSets);
    TRACE("  maxPerStageDescriptorSamplers: %u.\n", limits->maxPerStageDescriptorSamplers);
    TRACE("  maxPerStageDescriptorUniformBuffers: %u.\n", limits->maxPerStageDescriptorUniformBuffers);
    TRACE("  maxPerStageDescriptorStorageBuffers: %u.\n", limits->maxPerStageDescriptorStorageBuffers);
    TRACE("  maxPerStageDescriptorSampledImages: %u.\n", limits->maxPerStageDescriptorSampledImages);
    TRACE("  maxPerStageDescriptorStorageImages: %u.\n", limits->maxPerStageDescriptorStorageImages);
    TRACE("  maxPerStageDescriptorInputAttachments: %u.\n", limits->maxPerStageDescriptorInputAttachments);
    TRACE("  maxPerStageResources: %u.\n", limits->maxPerStageResources);
    TRACE("  maxDescriptorSetSamplers: %u.\n", limits->maxDescriptorSetSamplers);
    TRACE("  maxDescriptorSetUniformBuffers: %u.\n", limits->maxDescriptorSetUniformBuffers);
    TRACE("  maxDescriptorSetUniformBuffersDynamic: %u.\n", limits->maxDescriptorSetUniformBuffersDynamic);
    TRACE("  maxDescriptorSetStorageBuffers: %u.\n", limits->maxDescriptorSetStorageBuffers);
    TRACE("  maxDescriptorSetStorageBuffersDynamic: %u.\n", limits->maxDescriptorSetStorageBuffersDynamic);
    TRACE("  maxDescriptorSetSampledImages: %u.\n", limits->maxDescriptorSetSampledImages);
    TRACE("  maxDescriptorSetStorageImages: %u.\n", limits->maxDescriptorSetStorageImages);
    TRACE("  maxDescriptorSetInputAttachments: %u.\n", limits->maxDescriptorSetInputAttachments);
    TRACE("  maxVertexInputAttributes: %u.\n", limits->maxVertexInputAttributes);
    TRACE("  maxVertexInputBindings: %u.\n", limits->maxVertexInputBindings);
    TRACE("  maxVertexInputAttributeOffset: %u.\n", limits->maxVertexInputAttributeOffset);
    TRACE("  maxVertexInputBindingStride: %u.\n", limits->maxVertexInputBindingStride);
    TRACE("  maxVertexOutputComponents: %u.\n", limits->maxVertexOutputComponents);
    TRACE("  maxTessellationGenerationLevel: %u.\n", limits->maxTessellationGenerationLevel);
    TRACE("  maxTessellationPatchSize: %u.\n", limits->maxTessellationPatchSize);
    TRACE("  maxTessellationControlPerVertexInputComponents: %u.\n",
            limits->maxTessellationControlPerVertexInputComponents);
    TRACE("  maxTessellationControlPerVertexOutputComponents: %u.\n",
            limits->maxTessellationControlPerVertexOutputComponents);
    TRACE("  maxTessellationControlPerPatchOutputComponents: %u.\n",
            limits->maxTessellationControlPerPatchOutputComponents);
    TRACE("  maxTessellationControlTotalOutputComponents: %u.\n",
            limits->maxTessellationControlTotalOutputComponents);
    TRACE("  maxTessellationEvaluationInputComponents: %u.\n",
            limits->maxTessellationEvaluationInputComponents);
    TRACE("  maxTessellationEvaluationOutputComponents: %u.\n",
            limits->maxTessellationEvaluationOutputComponents);
    TRACE("  maxGeometryShaderInvocations: %u.\n", limits->maxGeometryShaderInvocations);
    TRACE("  maxGeometryInputComponents: %u.\n", limits->maxGeometryInputComponents);
    TRACE("  maxGeometryOutputComponents: %u.\n", limits->maxGeometryOutputComponents);
    TRACE("  maxGeometryOutputVertices: %u.\n", limits->maxGeometryOutputVertices);
    TRACE("  maxGeometryTotalOutputComponents: %u.\n", limits->maxGeometryTotalOutputComponents);
    TRACE("  maxFragmentInputComponents: %u.\n", limits->maxFragmentInputComponents);
    TRACE("  maxFragmentOutputAttachments: %u.\n", limits->maxFragmentOutputAttachments);
    TRACE("  maxFragmentDualSrcAttachments: %u.\n", limits->maxFragmentDualSrcAttachments);
    TRACE("  maxFragmentCombinedOutputResources: %u.\n", limits->maxFragmentCombinedOutputResources);
    TRACE("  maxComputeSharedMemorySize: %u.\n", limits->maxComputeSharedMemorySize);
    TRACE("  maxComputeWorkGroupCount: %u, %u, %u.\n", limits->maxComputeWorkGroupCount[0],
            limits->maxComputeWorkGroupCount[1], limits->maxComputeWorkGroupCount[2]);
    TRACE("  maxComputeWorkGroupInvocations: %u.\n", limits->maxComputeWorkGroupInvocations);
    TRACE("  maxComputeWorkGroupSize: %u, %u, %u.\n", limits->maxComputeWorkGroupSize[0],
            limits->maxComputeWorkGroupSize[1], limits->maxComputeWorkGroupSize[2]);
    TRACE("  subPixelPrecisionBits: %u.\n", limits->subPixelPrecisionBits);
    TRACE("  subTexelPrecisionBits: %u.\n", limits->subTexelPrecisionBits);
    TRACE("  mipmapPrecisionBits: %u.\n", limits->mipmapPrecisionBits);
    TRACE("  maxDrawIndexedIndexValue: %u.\n", limits->maxDrawIndexedIndexValue);
    TRACE("  maxDrawIndirectCount: %u.\n", limits->maxDrawIndirectCount);
    TRACE("  maxSamplerLodBias: %f.\n", limits->maxSamplerLodBias);
    TRACE("  maxSamplerAnisotropy: %f.\n", limits->maxSamplerAnisotropy);
    TRACE("  maxViewports: %u.\n", limits->maxViewports);
    TRACE("  maxViewportDimensions: %u, %u.\n", limits->maxViewportDimensions[0],
            limits->maxViewportDimensions[1]);
    TRACE("  viewportBoundsRange: %f, %f.\n", limits->viewportBoundsRange[0], limits->viewportBoundsRange[1]);
    TRACE("  viewportSubPixelBits: %u.\n", limits->viewportSubPixelBits);
    TRACE("  minMemoryMapAlignment: %u.\n", (unsigned int)limits->minMemoryMapAlignment);
    TRACE("  minTexelBufferOffsetAlignment: %#"PRIx64".\n", limits->minTexelBufferOffsetAlignment);
    TRACE("  minUniformBufferOffsetAlignment: %#"PRIx64".\n", limits->minUniformBufferOffsetAlignment);
    TRACE("  minStorageBufferOffsetAlignment: %#"PRIx64".\n", limits->minStorageBufferOffsetAlignment);
    TRACE("  minTexelOffset: %d.\n", limits->minTexelOffset);
    TRACE("  maxTexelOffset: %u.\n", limits->maxTexelOffset);
    TRACE("  minTexelGatherOffset: %d.\n", limits->minTexelGatherOffset);
    TRACE("  maxTexelGatherOffset: %u.\n", limits->maxTexelGatherOffset);
    TRACE("  minInterpolationOffset: %f.\n", limits->minInterpolationOffset);
    TRACE("  maxInterpolationOffset: %f.\n", limits->maxInterpolationOffset);
    TRACE("  subPixelInterpolationOffsetBits: %u.\n", limits->subPixelInterpolationOffsetBits);
    TRACE("  maxFramebufferWidth: %u.\n", limits->maxFramebufferWidth);
    TRACE("  maxFramebufferHeight: %u.\n", limits->maxFramebufferHeight);
    TRACE("  maxFramebufferLayers: %u.\n", limits->maxFramebufferLayers);
    TRACE("  framebufferColorSampleCounts: %#x.\n", limits->framebufferColorSampleCounts);
    TRACE("  framebufferDepthSampleCounts: %#x.\n", limits->framebufferDepthSampleCounts);
    TRACE("  framebufferStencilSampleCounts: %#x.\n", limits->framebufferStencilSampleCounts);
    TRACE("  framebufferNoAttachmentsSampleCounts: %#x.\n", limits->framebufferNoAttachmentsSampleCounts);
    TRACE("  maxColorAttachments: %u.\n", limits->maxColorAttachments);
    TRACE("  sampledImageColorSampleCounts: %#x.\n", limits->sampledImageColorSampleCounts);
    TRACE("  sampledImageIntegerSampleCounts: %#x.\n", limits->sampledImageIntegerSampleCounts);
    TRACE("  sampledImageDepthSampleCounts: %#x.\n", limits->sampledImageDepthSampleCounts);
    TRACE("  sampledImageStencilSampleCounts: %#x.\n", limits->sampledImageStencilSampleCounts);
    TRACE("  storageImageSampleCounts: %#x.\n", limits->storageImageSampleCounts);
    TRACE("  maxSampleMaskWords: %u.\n", limits->maxSampleMaskWords);
    TRACE("  timestampComputeAndGraphics: %#x.\n", limits->timestampComputeAndGraphics);
    TRACE("  timestampPeriod: %f.\n", limits->timestampPeriod);
    TRACE("  maxClipDistances: %u.\n", limits->maxClipDistances);
    TRACE("  maxCullDistances: %u.\n", limits->maxCullDistances);
    TRACE("  maxCombinedClipAndCullDistances: %u.\n", limits->maxCombinedClipAndCullDistances);
    TRACE("  discreteQueuePriorities: %u.\n", limits->discreteQueuePriorities);
    TRACE("  pointSizeRange: %f, %f.\n", limits->pointSizeRange[0], limits->pointSizeRange[1]);
    TRACE("  lineWidthRange: %f, %f,\n", limits->lineWidthRange[0], limits->lineWidthRange[1]);
    TRACE("  pointSizeGranularity: %f.\n", limits->pointSizeGranularity);
    TRACE("  lineWidthGranularity: %f.\n", limits->lineWidthGranularity);
    TRACE("  strictLines: %#x.\n", limits->strictLines);
    TRACE("  standardSampleLocations: %#x.\n", limits->standardSampleLocations);
    TRACE("  optimalBufferCopyOffsetAlignment: %#"PRIx64".\n", limits->optimalBufferCopyOffsetAlignment);
    TRACE("  optimalBufferCopyRowPitchAlignment: %#"PRIx64".\n", limits->optimalBufferCopyRowPitchAlignment);
    TRACE("  nonCoherentAtomSize: %#"PRIx64".\n", limits->nonCoherentAtomSize);

    descriptor_indexing = &info->descriptor_indexing_properties;
    TRACE("  VkPhysicalDeviceDescriptorIndexingPropertiesEXT:\n");

    TRACE("    maxUpdateAfterBindDescriptorsInAllPools: %u.\n",
            descriptor_indexing->maxUpdateAfterBindDescriptorsInAllPools);

    TRACE("    shaderUniformBufferArrayNonUniformIndexingNative: %#x.\n",
            descriptor_indexing->shaderUniformBufferArrayNonUniformIndexingNative);
    TRACE("    shaderSampledImageArrayNonUniformIndexingNative: %#x.\n",
            descriptor_indexing->shaderSampledImageArrayNonUniformIndexingNative);
    TRACE("    shaderStorageBufferArrayNonUniformIndexingNative: %#x.\n",
            descriptor_indexing->shaderStorageBufferArrayNonUniformIndexingNative);
    TRACE("    shaderStorageImageArrayNonUniformIndexingNative: %#x.\n",
            descriptor_indexing->shaderStorageImageArrayNonUniformIndexingNative);
    TRACE("    shaderInputAttachmentArrayNonUniformIndexingNative: %#x.\n",
            descriptor_indexing->shaderInputAttachmentArrayNonUniformIndexingNative);

    TRACE("    robustBufferAccessUpdateAfterBind: %#x.\n",
            descriptor_indexing->robustBufferAccessUpdateAfterBind);
    TRACE("    quadDivergentImplicitLod: %#x.\n",
            descriptor_indexing->quadDivergentImplicitLod);

    TRACE("    maxPerStageDescriptorUpdateAfterBindSamplers: %u.\n",
            descriptor_indexing->maxPerStageDescriptorUpdateAfterBindSamplers);
    TRACE("    maxPerStageDescriptorUpdateAfterBindUniformBuffers: %u.\n",
            descriptor_indexing->maxPerStageDescriptorUpdateAfterBindUniformBuffers);
    TRACE("    maxPerStageDescriptorUpdateAfterBindStorageBuffers: %u.\n",
            descriptor_indexing->maxPerStageDescriptorUpdateAfterBindStorageBuffers);
    TRACE("    maxPerStageDescriptorUpdateAfterBindSampledImages: %u.\n",
            descriptor_indexing->maxPerStageDescriptorUpdateAfterBindSampledImages);
    TRACE("    maxPerStageDescriptorUpdateAfterBindStorageImages: %u.\n",
            descriptor_indexing->maxPerStageDescriptorUpdateAfterBindStorageImages);
    TRACE("    maxPerStageDescriptorUpdateAfterBindInputAttachments: %u.\n",
            descriptor_indexing->maxPerStageDescriptorUpdateAfterBindInputAttachments);
    TRACE("    maxPerStageUpdateAfterBindResources: %u.\n",
            descriptor_indexing->maxPerStageUpdateAfterBindResources);

    TRACE("    maxDescriptorSetUpdateAfterBindSamplers: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindSamplers);
    TRACE("    maxDescriptorSetUpdateAfterBindUniformBuffers: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindUniformBuffers);
    TRACE("    maxDescriptorSetUpdateAfterBindUniformBuffersDynamic: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindUniformBuffersDynamic);
    TRACE("    maxDescriptorSetUpdateAfterBindStorageBuffers: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindStorageBuffers);
    TRACE("    maxDescriptorSetUpdateAfterBindStorageBuffersDynamic: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindStorageBuffersDynamic);
    TRACE("    maxDescriptorSetUpdateAfterBindSampledImages: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindSampledImages);
    TRACE("    maxDescriptorSetUpdateAfterBindStorageImages: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindStorageImages);
    TRACE("    maxDescriptorSetUpdateAfterBindInputAttachments: %u.\n",
            descriptor_indexing->maxDescriptorSetUpdateAfterBindInputAttachments);

    maintenance3 = &info->maintenance3_properties;
    TRACE("  VkPhysicalDeviceMaintenance3Properties:\n");
    TRACE("    maxPerSetDescriptors: %u.\n", maintenance3->maxPerSetDescriptors);
    TRACE("    maxMemoryAllocationSize: %#"PRIx64".\n", maintenance3->maxMemoryAllocationSize);

    buffer_alignment = &info->texel_buffer_alignment_properties;
    TRACE("  VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT:\n");
    TRACE("    storageTexelBufferOffsetAlignmentBytes: %#"PRIx64".\n",
            buffer_alignment->storageTexelBufferOffsetAlignmentBytes);
    TRACE("    storageTexelBufferOffsetSingleTexelAlignment: %#x.\n",
            buffer_alignment->storageTexelBufferOffsetSingleTexelAlignment);
    TRACE("    uniformTexelBufferOffsetAlignmentBytes: %#"PRIx64".\n",
            buffer_alignment->uniformTexelBufferOffsetAlignmentBytes);
    TRACE("    uniformTexelBufferOffsetSingleTexelAlignment: %#x.\n",
            buffer_alignment->uniformTexelBufferOffsetSingleTexelAlignment);

    xfb = &info->xfb_properties;
    TRACE("  VkPhysicalDeviceTransformFeedbackPropertiesEXT:\n");
    TRACE("    maxTransformFeedbackStreams: %u.\n", xfb->maxTransformFeedbackStreams);
    TRACE("    maxTransformFeedbackBuffers: %u.\n", xfb->maxTransformFeedbackBuffers);
    TRACE("    maxTransformFeedbackBufferSize: %#"PRIx64".\n", xfb->maxTransformFeedbackBufferSize);
    TRACE("    maxTransformFeedbackStreamDataSize: %u.\n", xfb->maxTransformFeedbackStreamDataSize);
    TRACE("    maxTransformFeedbackBufferDataSize: %u.\n", xfb->maxTransformFeedbackBufferDataSize);
    TRACE("    maxTransformFeedbackBufferDataStride: %u.\n", xfb->maxTransformFeedbackBufferDataStride);
    TRACE("    transformFeedbackQueries: %#x.\n", xfb->transformFeedbackQueries);
    TRACE("    transformFeedbackStreamsLinesTriangles: %#x.\n", xfb->transformFeedbackStreamsLinesTriangles);
    TRACE("    transformFeedbackRasterizationStreamSelect: %#x.\n", xfb->transformFeedbackRasterizationStreamSelect);
    TRACE("    transformFeedbackDraw: %x.\n", xfb->transformFeedbackDraw);

    divisor_properties = &info->vertex_divisor_properties;
    TRACE("  VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT:\n");
    TRACE("    maxVertexAttribDivisor: %u.\n", divisor_properties->maxVertexAttribDivisor);
}

static void vkd3d_trace_physical_device_features(const struct vkd3d_physical_device_info *info)
{
    const VkPhysicalDeviceConditionalRenderingFeaturesEXT *conditional_rendering_features;
    const VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT *demote_features;
    const VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT *buffer_alignment_features;
    const VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *divisor_features;
    const VkPhysicalDeviceDescriptorIndexingFeaturesEXT *descriptor_indexing;
    const VkPhysicalDeviceDepthClipEnableFeaturesEXT *depth_clip_features;
    const VkPhysicalDeviceFeatures *features = &info->features2.features;
    const VkPhysicalDeviceTransformFeedbackFeaturesEXT *xfb;

    TRACE("Device features:\n");
    TRACE("  robustBufferAccess: %#x.\n", features->robustBufferAccess);
    TRACE("  fullDrawIndexUint32: %#x.\n", features->fullDrawIndexUint32);
    TRACE("  imageCubeArray: %#x.\n", features->imageCubeArray);
    TRACE("  independentBlend: %#x.\n", features->independentBlend);
    TRACE("  geometryShader: %#x.\n", features->geometryShader);
    TRACE("  tessellationShader: %#x.\n", features->tessellationShader);
    TRACE("  sampleRateShading: %#x.\n", features->sampleRateShading);
    TRACE("  dualSrcBlend: %#x.\n", features->dualSrcBlend);
    TRACE("  logicOp: %#x.\n", features->logicOp);
    TRACE("  multiDrawIndirect: %#x.\n", features->multiDrawIndirect);
    TRACE("  drawIndirectFirstInstance: %#x.\n", features->drawIndirectFirstInstance);
    TRACE("  depthClamp: %#x.\n", features->depthClamp);
    TRACE("  depthBiasClamp: %#x.\n", features->depthBiasClamp);
    TRACE("  fillModeNonSolid: %#x.\n", features->fillModeNonSolid);
    TRACE("  depthBounds: %#x.\n", features->depthBounds);
    TRACE("  wideLines: %#x.\n", features->wideLines);
    TRACE("  largePoints: %#x.\n", features->largePoints);
    TRACE("  alphaToOne: %#x.\n", features->alphaToOne);
    TRACE("  multiViewport: %#x.\n", features->multiViewport);
    TRACE("  samplerAnisotropy: %#x.\n", features->samplerAnisotropy);
    TRACE("  textureCompressionETC2: %#x.\n", features->textureCompressionETC2);
    TRACE("  textureCompressionASTC_LDR: %#x.\n", features->textureCompressionASTC_LDR);
    TRACE("  textureCompressionBC: %#x.\n", features->textureCompressionBC);
    TRACE("  occlusionQueryPrecise: %#x.\n", features->occlusionQueryPrecise);
    TRACE("  pipelineStatisticsQuery: %#x.\n", features->pipelineStatisticsQuery);
    TRACE("  vertexOipelineStoresAndAtomics: %#x.\n", features->vertexPipelineStoresAndAtomics);
    TRACE("  fragmentStoresAndAtomics: %#x.\n", features->fragmentStoresAndAtomics);
    TRACE("  shaderTessellationAndGeometryPointSize: %#x.\n", features->shaderTessellationAndGeometryPointSize);
    TRACE("  shaderImageGatherExtended: %#x.\n", features->shaderImageGatherExtended);
    TRACE("  shaderStorageImageExtendedFormats: %#x.\n", features->shaderStorageImageExtendedFormats);
    TRACE("  shaderStorageImageMultisample: %#x.\n", features->shaderStorageImageMultisample);
    TRACE("  shaderStorageImageReadWithoutFormat: %#x.\n", features->shaderStorageImageReadWithoutFormat);
    TRACE("  shaderStorageImageWriteWithoutFormat: %#x.\n", features->shaderStorageImageWriteWithoutFormat);
    TRACE("  shaderUniformBufferArrayDynamicIndexing: %#x.\n", features->shaderUniformBufferArrayDynamicIndexing);
    TRACE("  shaderSampledImageArrayDynamicIndexing: %#x.\n", features->shaderSampledImageArrayDynamicIndexing);
    TRACE("  shaderStorageBufferArrayDynamicIndexing: %#x.\n", features->shaderStorageBufferArrayDynamicIndexing);
    TRACE("  shaderStorageImageArrayDynamicIndexing: %#x.\n", features->shaderStorageImageArrayDynamicIndexing);
    TRACE("  shaderClipDistance: %#x.\n", features->shaderClipDistance);
    TRACE("  shaderCullDistance: %#x.\n", features->shaderCullDistance);
    TRACE("  shaderFloat64: %#x.\n", features->shaderFloat64);
    TRACE("  shaderInt64: %#x.\n", features->shaderInt64);
    TRACE("  shaderInt16: %#x.\n", features->shaderInt16);
    TRACE("  shaderResourceResidency: %#x.\n", features->shaderResourceResidency);
    TRACE("  shaderResourceMinLod: %#x.\n", features->shaderResourceMinLod);
    TRACE("  sparseBinding: %#x.\n", features->sparseBinding);
    TRACE("  sparseResidencyBuffer: %#x.\n", features->sparseResidencyBuffer);
    TRACE("  sparseResidencyImage2D: %#x.\n", features->sparseResidencyImage2D);
    TRACE("  sparseResidencyImage3D: %#x.\n", features->sparseResidencyImage3D);
    TRACE("  sparseResidency2Samples: %#x.\n", features->sparseResidency2Samples);
    TRACE("  sparseResidency4Samples: %#x.\n", features->sparseResidency4Samples);
    TRACE("  sparseResidency8Samples: %#x.\n", features->sparseResidency8Samples);
    TRACE("  sparseResidency16Samples: %#x.\n", features->sparseResidency16Samples);
    TRACE("  sparseResidencyAliased: %#x.\n", features->sparseResidencyAliased);
    TRACE("  variableMultisampleRate: %#x.\n", features->variableMultisampleRate);
    TRACE("  inheritedQueries: %#x.\n", features->inheritedQueries);

    descriptor_indexing = &info->descriptor_indexing_features;
    TRACE("  VkPhysicalDeviceDescriptorIndexingFeaturesEXT:\n");

    TRACE("    shaderInputAttachmentArrayDynamicIndexing: %#x.\n",
            descriptor_indexing->shaderInputAttachmentArrayDynamicIndexing);
    TRACE("    shaderUniformTexelBufferArrayDynamicIndexing: %#x.\n",
            descriptor_indexing->shaderUniformTexelBufferArrayDynamicIndexing);
    TRACE("    shaderStorageTexelBufferArrayDynamicIndexing: %#x.\n",
            descriptor_indexing->shaderStorageTexelBufferArrayDynamicIndexing);

    TRACE("    shaderUniformBufferArrayNonUniformIndexing: %#x.\n",
            descriptor_indexing->shaderUniformBufferArrayNonUniformIndexing);
    TRACE("    shaderSampledImageArrayNonUniformIndexing: %#x.\n",
            descriptor_indexing->shaderSampledImageArrayNonUniformIndexing);
    TRACE("    shaderStorageBufferArrayNonUniformIndexing: %#x.\n",
            descriptor_indexing->shaderStorageBufferArrayNonUniformIndexing);
    TRACE("    shaderStorageImageArrayNonUniformIndexing: %#x.\n",
            descriptor_indexing->shaderStorageImageArrayNonUniformIndexing);
    TRACE("    shaderInputAttachmentArrayNonUniformIndexing: %#x.\n",
            descriptor_indexing->shaderInputAttachmentArrayNonUniformIndexing);
    TRACE("    shaderUniformTexelBufferArrayNonUniformIndexing: %#x.\n",
            descriptor_indexing->shaderUniformTexelBufferArrayNonUniformIndexing);
    TRACE("    shaderStorageTexelBufferArrayNonUniformIndexing: %#x.\n",
            descriptor_indexing->shaderStorageTexelBufferArrayNonUniformIndexing);

    TRACE("    descriptorBindingUniformBufferUpdateAfterBind: %#x.\n",
            descriptor_indexing->descriptorBindingUniformBufferUpdateAfterBind);
    TRACE("    descriptorBindingSampledImageUpdateAfterBind: %#x.\n",
            descriptor_indexing->descriptorBindingSampledImageUpdateAfterBind);
    TRACE("    descriptorBindingStorageImageUpdateAfterBind: %#x.\n",
            descriptor_indexing->descriptorBindingStorageImageUpdateAfterBind);
    TRACE("    descriptorBindingStorageBufferUpdateAfterBind: %#x.\n",
            descriptor_indexing->descriptorBindingStorageBufferUpdateAfterBind);
    TRACE("    descriptorBindingUniformTexelBufferUpdateAfterBind: %#x.\n",
            descriptor_indexing->descriptorBindingUniformTexelBufferUpdateAfterBind);
    TRACE("    descriptorBindingStorageTexelBufferUpdateAfterBind: %#x.\n",
            descriptor_indexing->descriptorBindingStorageTexelBufferUpdateAfterBind);

    TRACE("    descriptorBindingUpdateUnusedWhilePending: %#x.\n",
            descriptor_indexing->descriptorBindingUpdateUnusedWhilePending);
    TRACE("    descriptorBindingPartiallyBound: %#x.\n",
            descriptor_indexing->descriptorBindingPartiallyBound);
    TRACE("    descriptorBindingVariableDescriptorCount: %#x.\n",
            descriptor_indexing->descriptorBindingVariableDescriptorCount);
    TRACE("    runtimeDescriptorArray: %#x.\n",
            descriptor_indexing->runtimeDescriptorArray);

    conditional_rendering_features = &info->conditional_rendering_features;
    TRACE("  VkPhysicalDeviceConditionalRenderingFeaturesEXT:\n");
    TRACE("    conditionalRendering: %#x.\n", conditional_rendering_features->conditionalRendering);

    depth_clip_features = &info->depth_clip_features;
    TRACE("  VkPhysicalDeviceDepthClipEnableFeaturesEXT:\n");
    TRACE("    depthClipEnable: %#x.\n", depth_clip_features->depthClipEnable);

    demote_features = &info->demote_features;
    TRACE("  VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT:\n");
    TRACE("    shaderDemoteToHelperInvocation: %#x.\n", demote_features->shaderDemoteToHelperInvocation);

    buffer_alignment_features = &info->texel_buffer_alignment_features;
    TRACE("  VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT:\n");
    TRACE("    texelBufferAlignment: %#x.\n", buffer_alignment_features->texelBufferAlignment);

    xfb = &info->xfb_features;
    TRACE("  VkPhysicalDeviceTransformFeedbackFeaturesEXT:\n");
    TRACE("    transformFeedback: %#x.\n", xfb->transformFeedback);
    TRACE("    geometryStreams: %#x.\n", xfb->geometryStreams);

    divisor_features = &info->vertex_divisor_features;
    TRACE("  VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT:\n");
    TRACE("    vertexAttributeInstanceRateDivisor: %#x.\n",
            divisor_features->vertexAttributeInstanceRateDivisor);
    TRACE("    vertexAttributeInstanceRateZeroDivisor: %#x.\n",
            divisor_features->vertexAttributeInstanceRateZeroDivisor);
}

static void vkd3d_init_feature_level(struct vkd3d_vulkan_info *vk_info,
        const VkPhysicalDeviceFeatures *features,
        const D3D12_FEATURE_DATA_D3D12_OPTIONS *d3d12_options)
{
    bool have_11_0 = true;

#define CHECK_MIN_REQUIREMENT(name, value) \
    if (vk_info->device_limits.name < value) \
        WARN(#name " does not meet feature level 11_0 requirements.\n");
#define CHECK_MAX_REQUIREMENT(name, value) \
    if (vk_info->device_limits.name > value) \
        WARN(#name " does not meet feature level 11_0 requirements.\n");
#define CHECK_FEATURE(name) \
    if (!features->name) \
    { \
        WARN(#name " is not supported.\n"); \
        have_11_0 = false; \
    }

    if (!vk_info->device_limits.timestampComputeAndGraphics)
        WARN("Timestamps are not supported on all graphics and compute queues.\n");

    CHECK_MIN_REQUIREMENT(maxPushConstantsSize, D3D12_MAX_ROOT_COST * sizeof(uint32_t));
    CHECK_MIN_REQUIREMENT(maxComputeSharedMemorySize, D3D12_CS_TGSM_REGISTER_COUNT * sizeof(uint32_t));

    CHECK_MAX_REQUIREMENT(viewportBoundsRange[0], D3D12_VIEWPORT_BOUNDS_MIN);
    CHECK_MIN_REQUIREMENT(viewportBoundsRange[1], D3D12_VIEWPORT_BOUNDS_MAX);
    CHECK_MIN_REQUIREMENT(viewportSubPixelBits, 8);

    CHECK_MIN_REQUIREMENT(maxPerStageDescriptorUniformBuffers,
            D3D12_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);

    CHECK_FEATURE(depthBiasClamp);
    CHECK_FEATURE(depthClamp);
    CHECK_FEATURE(drawIndirectFirstInstance);
    CHECK_FEATURE(dualSrcBlend);
    CHECK_FEATURE(fragmentStoresAndAtomics);
    CHECK_FEATURE(fullDrawIndexUint32);
    CHECK_FEATURE(geometryShader);
    CHECK_FEATURE(imageCubeArray);
    CHECK_FEATURE(independentBlend);
    CHECK_FEATURE(multiDrawIndirect);
    CHECK_FEATURE(multiViewport);
    CHECK_FEATURE(occlusionQueryPrecise);
    CHECK_FEATURE(pipelineStatisticsQuery);
    CHECK_FEATURE(samplerAnisotropy);
    CHECK_FEATURE(sampleRateShading);
    CHECK_FEATURE(shaderClipDistance);
    CHECK_FEATURE(shaderCullDistance);
    CHECK_FEATURE(shaderImageGatherExtended);
    CHECK_FEATURE(shaderStorageImageWriteWithoutFormat);
    CHECK_FEATURE(tessellationShader);

    if (!vk_info->EXT_depth_clip_enable)
        WARN("Depth clip enable is not supported.\n");
    if (!vk_info->EXT_transform_feedback)
        WARN("Stream output is not supported.\n");

    if (!vk_info->EXT_vertex_attribute_divisor)
        WARN("Vertex attribute instance rate divisor is not supported.\n");
    else if (!vk_info->vertex_attrib_zero_divisor)
        WARN("Vertex attribute instance rate zero divisor is not supported.\n");

#undef CHECK_MIN_REQUIREMENT
#undef CHECK_MAX_REQUIREMENT
#undef CHECK_FEATURE

    vk_info->max_feature_level = D3D_FEATURE_LEVEL_11_0;

    if (have_11_0
            && d3d12_options->OutputMergerLogicOp
            && features->vertexPipelineStoresAndAtomics
            && vk_info->device_limits.maxPerStageDescriptorStorageBuffers >= D3D12_UAV_SLOT_COUNT
            && vk_info->device_limits.maxPerStageDescriptorStorageImages >= D3D12_UAV_SLOT_COUNT)
        vk_info->max_feature_level = D3D_FEATURE_LEVEL_11_1;

    /* TODO: MinMaxFiltering */
    if (vk_info->max_feature_level >= D3D_FEATURE_LEVEL_11_1
            && d3d12_options->TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_2
            && d3d12_options->ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_2
            && d3d12_options->TypedUAVLoadAdditionalFormats)
        vk_info->max_feature_level = D3D_FEATURE_LEVEL_12_0;

    if (vk_info->max_feature_level >= D3D_FEATURE_LEVEL_12_0
            && d3d12_options->ROVsSupported
            && d3d12_options->ConservativeRasterizationTier >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1)
        vk_info->max_feature_level = D3D_FEATURE_LEVEL_12_1;

    TRACE("Max feature level: %#x.\n", vk_info->max_feature_level);
}

static void vkd3d_device_descriptor_limits_init(struct vkd3d_device_descriptor_limits *limits,
        const VkPhysicalDeviceLimits *device_limits)
{
    limits->uniform_buffer_max_descriptors = device_limits->maxDescriptorSetUniformBuffers;
    limits->sampled_image_max_descriptors = device_limits->maxDescriptorSetSampledImages;
    limits->storage_buffer_max_descriptors = device_limits->maxDescriptorSetStorageBuffers;
    limits->storage_image_max_descriptors = device_limits->maxDescriptorSetStorageImages;
    limits->sampler_max_descriptors = min(device_limits->maxDescriptorSetSamplers, VKD3D_MAX_DESCRIPTOR_SET_SAMPLERS);
}

static void vkd3d_device_vk_heaps_descriptor_limits_init(struct vkd3d_device_descriptor_limits *limits,
        const VkPhysicalDeviceDescriptorIndexingPropertiesEXT *properties)
{
    const unsigned int root_provision = D3D12_MAX_ROOT_COST / 2;
    unsigned int srv_divisor = 1, uav_divisor = 1;

    /* The total number of populated sampled image or storage image descriptors never exceeds the size of
     * one set (or two sets if every UAV has a counter), but the total size of bound layouts will exceed
     * device limits if each set size is maxDescriptorSet*, because of the D3D12 buffer + image allowance
     * (and UAV counters). Breaking limits for layouts seems to work with RADV and Nvidia drivers at
     * least, but let's try to stay within them if limits are high enough. */
    if (properties->maxDescriptorSetUpdateAfterBindSampledImages >= (1u << 21))
    {
        srv_divisor = 2;
        uav_divisor = properties->maxDescriptorSetUpdateAfterBindSampledImages >= (3u << 20) ? 3 : 2;
    }

    limits->uniform_buffer_max_descriptors = min(properties->maxDescriptorSetUpdateAfterBindUniformBuffers,
            properties->maxPerStageDescriptorUpdateAfterBindUniformBuffers - root_provision);
    limits->sampled_image_max_descriptors = min(properties->maxDescriptorSetUpdateAfterBindSampledImages,
            properties->maxPerStageDescriptorUpdateAfterBindSampledImages / srv_divisor - root_provision);
    limits->storage_buffer_max_descriptors = min(properties->maxDescriptorSetUpdateAfterBindStorageBuffers,
            properties->maxPerStageDescriptorUpdateAfterBindStorageBuffers - root_provision);
    limits->storage_image_max_descriptors = min(properties->maxDescriptorSetUpdateAfterBindStorageImages,
            properties->maxPerStageDescriptorUpdateAfterBindStorageImages / uav_divisor - root_provision);
    limits->sampler_max_descriptors = min(properties->maxDescriptorSetUpdateAfterBindSamplers,
            properties->maxPerStageDescriptorUpdateAfterBindSamplers - root_provision);
    limits->sampler_max_descriptors = min(limits->sampler_max_descriptors, VKD3D_MAX_DESCRIPTOR_SET_SAMPLERS);
}

static bool d3d12_device_supports_typed_uav_load_additional_formats(const struct d3d12_device *device)
{
    const struct vkd3d_vk_instance_procs *vk_procs = &device->vkd3d_instance->vk_procs;
    const struct vkd3d_format *format;
    VkFormatProperties properties;
    unsigned int i;

    static const DXGI_FORMAT additional_formats[] =
    {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SINT,
        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SINT,
    };

    for (i = 0; i < ARRAY_SIZE(additional_formats); ++i)
    {
        format = vkd3d_get_format(device, additional_formats[i], false);
        assert(format);

        VK_CALL(vkGetPhysicalDeviceFormatProperties(device->vk_physical_device, format->vk_format, &properties));
        if (!((properties.linearTilingFeatures | properties.optimalTilingFeatures) & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
            return false;
    }

    return true;
}

static HRESULT vkd3d_init_device_caps(struct d3d12_device *device,
        const struct vkd3d_device_create_info *create_info,
        struct vkd3d_physical_device_info *physical_device_info,
        uint32_t *device_extension_count, bool **user_extension_supported)
{
    const struct vkd3d_vk_instance_procs *vk_procs = &device->vkd3d_instance->vk_procs;
    const struct vkd3d_optional_device_extensions_info *optional_extensions;
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT *descriptor_indexing;
    VkPhysicalDevice physical_device = device->vk_physical_device;
    struct vkd3d_vulkan_info *vulkan_info = &device->vk_info;
    VkExtensionProperties *vk_extensions;
    VkPhysicalDeviceFeatures *features;
    uint32_t count;
    VkResult vr;

    *device_extension_count = 0;

    vkd3d_trace_physical_device(physical_device, physical_device_info, vk_procs);
    vkd3d_trace_physical_device_features(physical_device_info);
    vkd3d_trace_physical_device_limits(physical_device_info);

    features = &physical_device_info->features2.features;

    if (!features->sparseResidencyBuffer || !features->sparseResidencyImage2D)
    {
        features->sparseResidencyBuffer = VK_FALSE;
        features->sparseResidencyImage2D = VK_FALSE;
        physical_device_info->properties2.properties.sparseProperties.residencyNonResidentStrict = VK_FALSE;
    }

    vulkan_info->device_limits = physical_device_info->properties2.properties.limits;
    vulkan_info->sparse_properties = physical_device_info->properties2.properties.sparseProperties;
    vulkan_info->rasterization_stream = physical_device_info->xfb_properties.transformFeedbackRasterizationStreamSelect;
    vulkan_info->transform_feedback_queries = physical_device_info->xfb_properties.transformFeedbackQueries;
    vulkan_info->uav_read_without_format = features->shaderStorageImageReadWithoutFormat;
    vulkan_info->max_vertex_attrib_divisor = max(physical_device_info->vertex_divisor_properties.maxVertexAttribDivisor, 1);

    device->feature_options.DoublePrecisionFloatShaderOps = features->shaderFloat64;
    device->feature_options.OutputMergerLogicOp = features->logicOp;
    /* SPV_KHR_16bit_storage */
    device->feature_options.MinPrecisionSupport = D3D12_SHADER_MIN_PRECISION_SUPPORT_NONE;

    if (!features->sparseBinding)
        device->feature_options.TiledResourcesTier = D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED;
    else if (!device->vk_info.sparse_properties.residencyNonResidentStrict)
        device->feature_options.TiledResourcesTier = D3D12_TILED_RESOURCES_TIER_1;
    else if (!features->sparseResidencyImage3D)
        device->feature_options.TiledResourcesTier = D3D12_TILED_RESOURCES_TIER_2;
    else
        device->feature_options.TiledResourcesTier = D3D12_TILED_RESOURCES_TIER_3;

    /* FIXME: Implement tiled resources. */
    if (device->feature_options.TiledResourcesTier)
    {
        WARN("Tiled resources are not implemented yet.\n");
        device->feature_options.TiledResourcesTier = D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED;
    }

    if (device->vk_info.device_limits.maxPerStageDescriptorSamplers <= 16)
        device->feature_options.ResourceBindingTier = D3D12_RESOURCE_BINDING_TIER_1;
    else if (device->vk_info.device_limits.maxPerStageDescriptorUniformBuffers <= 14)
        device->feature_options.ResourceBindingTier = D3D12_RESOURCE_BINDING_TIER_2;
    else
        device->feature_options.ResourceBindingTier = D3D12_RESOURCE_BINDING_TIER_3;

    device->feature_options.TypedUAVLoadAdditionalFormats = features->shaderStorageImageReadWithoutFormat
            && d3d12_device_supports_typed_uav_load_additional_formats(device);
    /* GL_INTEL_fragment_shader_ordering, no Vulkan equivalent. */
    device->feature_options.ROVsSupported = FALSE;
    /* GL_INTEL_conservative_rasterization, no Vulkan equivalent. */
    device->feature_options.ConservativeRasterizationTier = D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED;
    device->feature_options.MaxGPUVirtualAddressBitsPerResource = 40; /* FIXME */
    device->feature_options.StandardSwizzle64KBSupported = FALSE;
    device->feature_options.CrossNodeSharingTier = D3D12_CROSS_NODE_SHARING_TIER_NOT_SUPPORTED;
    device->feature_options.CrossAdapterRowMajorTextureSupported = FALSE;
    /* SPV_EXT_shader_viewport_index_layer */
    device->feature_options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation = FALSE;
    device->feature_options.ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2;

    /* Shader Model 6 support. */
    device->feature_options1.WaveOps = FALSE;
    device->feature_options1.WaveLaneCountMin = 0;
    device->feature_options1.WaveLaneCountMax = 0;
    device->feature_options1.TotalLaneCount = 0;
    device->feature_options1.ExpandedComputeResourceStates = TRUE;
    device->feature_options1.Int64ShaderOps = features->shaderInt64;

    /* Depth bounds test is enabled in D3D12_DEPTH_STENCIL_DESC1, which is not
     * supported. */
    device->feature_options2.DepthBoundsTestSupported = FALSE;
    /* d3d12_command_list_SetSamplePositions() is not implemented. */
    device->feature_options2.ProgrammableSamplePositionsTier = D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;

    device->feature_options3.CopyQueueTimestampQueriesSupported = FALSE;
    device->feature_options3.CastingFullyTypedFormatSupported = FALSE;
    device->feature_options3.WriteBufferImmediateSupportFlags = D3D12_COMMAND_LIST_SUPPORT_FLAG_NONE;
    device->feature_options3.ViewInstancingTier = D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
    device->feature_options3.BarycentricsSupported = FALSE;

    device->feature_options4.MSAA64KBAlignedTextureSupported = FALSE;
    device->feature_options4.SharedResourceCompatibilityTier = D3D12_SHARED_RESOURCE_COMPATIBILITY_TIER_0;
    /* An SM 6.2 feature. This would require features->shaderInt16 and
     * VK_KHR_shader_float16_int8. */
    device->feature_options4.Native16BitShaderOpsSupported = FALSE;

    device->feature_options5.SRVOnlyTiledResourceTier3 = FALSE;
    device->feature_options5.RenderPassesTier = D3D12_RENDER_PASS_TIER_0;
    device->feature_options5.RaytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;

    if ((vr = VK_CALL(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &count, NULL))) < 0)
    {
        ERR("Failed to enumerate device extensions, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }
    if (!count)
        return S_OK;

    if (!(vk_extensions = vkd3d_calloc(count, sizeof(*vk_extensions))))
        return E_OUTOFMEMORY;

    TRACE("Enumerating %u device extensions.\n", count);
    if ((vr = VK_CALL(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &count, vk_extensions))) < 0)
    {
        ERR("Failed to enumerate device extensions, vr %d.\n", vr);
        vkd3d_free(vk_extensions);
        return hresult_from_vk_result(vr);
    }

    optional_extensions = vkd3d_find_struct(create_info->next, OPTIONAL_DEVICE_EXTENSIONS_INFO);
    if (optional_extensions && optional_extensions->extension_count)
    {
        if (!(*user_extension_supported = vkd3d_calloc(optional_extensions->extension_count, sizeof(bool))))
        {
            vkd3d_free(vk_extensions);
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *user_extension_supported = NULL;
    }

    *device_extension_count = vkd3d_check_extensions(vk_extensions, count,
            required_device_extensions, ARRAY_SIZE(required_device_extensions),
            optional_device_extensions, ARRAY_SIZE(optional_device_extensions),
            create_info->device_extensions, create_info->device_extension_count,
            optional_extensions ? optional_extensions->extensions : NULL,
            optional_extensions ? optional_extensions->extension_count : 0,
            *user_extension_supported, vulkan_info, "device",
            device->vkd3d_instance->config_flags & VKD3D_CONFIG_FLAG_VULKAN_DEBUG);

    if (!physical_device_info->conditional_rendering_features.conditionalRendering)
        vulkan_info->EXT_conditional_rendering = false;
    if (!physical_device_info->depth_clip_features.depthClipEnable)
        vulkan_info->EXT_depth_clip_enable = false;
    if (!physical_device_info->robustness2_features.nullDescriptor)
        vulkan_info->EXT_robustness2 = false;
    if (!physical_device_info->demote_features.shaderDemoteToHelperInvocation)
        vulkan_info->EXT_shader_demote_to_helper_invocation = false;
    if (!physical_device_info->texel_buffer_alignment_features.texelBufferAlignment)
        vulkan_info->EXT_texel_buffer_alignment = false;
    if (!physical_device_info->timeline_semaphore_features.timelineSemaphore)
        vulkan_info->KHR_timeline_semaphore = false;

    vulkan_info->texel_buffer_alignment_properties = physical_device_info->texel_buffer_alignment_properties;

    if (get_spec_version(vk_extensions, count, VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME) >= 3)
    {
        const VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *divisor_features;
        divisor_features = &physical_device_info->vertex_divisor_features;
        if (!divisor_features->vertexAttributeInstanceRateDivisor)
            vulkan_info->EXT_vertex_attribute_divisor = false;
        vulkan_info->vertex_attrib_zero_divisor = divisor_features->vertexAttributeInstanceRateZeroDivisor;
    }
    else
    {
        vulkan_info->vertex_attrib_zero_divisor = false;
    }

    vkd3d_free(vk_extensions);

    device->feature_options.PSSpecifiedStencilRefSupported = vulkan_info->EXT_shader_stencil_export;

    vkd3d_init_feature_level(vulkan_info, features, &device->feature_options);
    if (vulkan_info->max_feature_level < create_info->minimum_feature_level)
    {
        WARN("Feature level %#x is not supported.\n", create_info->minimum_feature_level);
        vkd3d_free(*user_extension_supported);
        *user_extension_supported = NULL;
        return E_INVALIDARG;
    }

    /* Shader extensions. */
    if (vulkan_info->EXT_shader_demote_to_helper_invocation)
    {
        vulkan_info->shader_extension_count = 1;
        vulkan_info->shader_extensions[0] = VKD3D_SHADER_SPIRV_EXTENSION_EXT_DEMOTE_TO_HELPER_INVOCATION;
    }

    if (vulkan_info->EXT_descriptor_indexing)
        vulkan_info->shader_extensions[vulkan_info->shader_extension_count++]
                = VKD3D_SHADER_SPIRV_EXTENSION_EXT_DESCRIPTOR_INDEXING;

    if (vulkan_info->EXT_shader_stencil_export)
        vulkan_info->shader_extensions[vulkan_info->shader_extension_count++]
                = VKD3D_SHADER_SPIRV_EXTENSION_EXT_STENCIL_EXPORT;

    /* Disable unused Vulkan features. */
    features->shaderTessellationAndGeometryPointSize = VK_FALSE;

    descriptor_indexing = &physical_device_info->descriptor_indexing_features;
    descriptor_indexing->shaderInputAttachmentArrayDynamicIndexing = VK_FALSE;
    descriptor_indexing->shaderInputAttachmentArrayNonUniformIndexing = VK_FALSE;

    /* We do not use storage buffers currently. */
    features->shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
    descriptor_indexing->shaderStorageBufferArrayNonUniformIndexing = VK_FALSE;
    descriptor_indexing->descriptorBindingStorageBufferUpdateAfterBind = VK_FALSE;

    if (vulkan_info->EXT_descriptor_indexing && descriptor_indexing
            && (descriptor_indexing->descriptorBindingUniformBufferUpdateAfterBind
            || descriptor_indexing->descriptorBindingStorageBufferUpdateAfterBind
            || descriptor_indexing->descriptorBindingUniformTexelBufferUpdateAfterBind
            || descriptor_indexing->descriptorBindingStorageTexelBufferUpdateAfterBind)
            && !physical_device_info->descriptor_indexing_properties.robustBufferAccessUpdateAfterBind)
    {
        WARN("Disabling robust buffer access for the update after bind feature.\n");
        features->robustBufferAccess = VK_FALSE;
    }

    /* Select descriptor heap implementation. Forcing virtual heaps may be useful if
     * a client allocates descriptor heaps too large for the Vulkan device, or the
     * root signature cost exceeds the available push constant size. Virtual heaps
     * use only enough descriptors for the descriptor tables of the currently bound
     * root signature, and don't require a 32-bit push constant for each table. */
    device->use_vk_heaps = vulkan_info->EXT_descriptor_indexing
            && !(device->vkd3d_instance->config_flags & VKD3D_CONFIG_FLAG_VIRTUAL_HEAPS)
            && descriptor_indexing->descriptorBindingUniformBufferUpdateAfterBind
            && descriptor_indexing->descriptorBindingSampledImageUpdateAfterBind
            && descriptor_indexing->descriptorBindingStorageImageUpdateAfterBind
            && descriptor_indexing->descriptorBindingUniformTexelBufferUpdateAfterBind
            && descriptor_indexing->descriptorBindingStorageTexelBufferUpdateAfterBind;

    if (device->use_vk_heaps)
        vkd3d_device_vk_heaps_descriptor_limits_init(&vulkan_info->descriptor_limits,
                &physical_device_info->descriptor_indexing_properties);
    else
        vkd3d_device_descriptor_limits_init(&vulkan_info->descriptor_limits,
                &physical_device_info->properties2.properties.limits);

    return S_OK;
}

static HRESULT vkd3d_select_physical_device(struct vkd3d_instance *instance,
        unsigned int device_index, VkPhysicalDevice *selected_device)
{
    VkPhysicalDevice dgpu_device = VK_NULL_HANDLE, igpu_device = VK_NULL_HANDLE;
    const struct vkd3d_vk_instance_procs *vk_procs = &instance->vk_procs;
    VkInstance vk_instance = instance->vk_instance;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice *physical_devices;
    uint32_t count;
    unsigned int i;
    VkResult vr;

    count = 0;
    if ((vr = VK_CALL(vkEnumeratePhysicalDevices(vk_instance, &count, NULL))) < 0)
    {
        ERR("Failed to enumerate physical devices, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }
    if (!count)
    {
        ERR("No physical device available.\n");
        return E_FAIL;
    }
    if (!(physical_devices = vkd3d_calloc(count, sizeof(*physical_devices))))
        return E_OUTOFMEMORY;

    TRACE("Enumerating %u physical device(s).\n", count);
    if ((vr = VK_CALL(vkEnumeratePhysicalDevices(vk_instance, &count, physical_devices))) < 0)
    {
        ERR("Failed to enumerate physical devices, vr %d.\n", vr);
        vkd3d_free(physical_devices);
        return hresult_from_vk_result(vr);
    }

    if (device_index != ~0u && device_index >= count)
        WARN("Device index %u is out of range.\n", device_index);

    for (i = 0; i < count; ++i)
    {
        VK_CALL(vkGetPhysicalDeviceProperties(physical_devices[i], &device_properties));
        vkd3d_trace_physical_device_properties(&device_properties);

        if (i == device_index)
            device = physical_devices[i];

        if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && !dgpu_device)
            dgpu_device = physical_devices[i];
        else if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && !igpu_device)
            igpu_device = physical_devices[i];
    }

    if (!device)
        device = dgpu_device ? dgpu_device : igpu_device;
    if (!device)
        device = physical_devices[0];

    vkd3d_free(physical_devices);

    VK_CALL(vkGetPhysicalDeviceProperties(device, &device_properties));
    TRACE("Using device: %s, %#x:%#x.\n", device_properties.deviceName,
            device_properties.vendorID, device_properties.deviceID);

    *selected_device = device;

    return S_OK;
}

/* Vulkan queues */
enum vkd3d_queue_family
{
    VKD3D_QUEUE_FAMILY_DIRECT,
    VKD3D_QUEUE_FAMILY_COMPUTE,
    VKD3D_QUEUE_FAMILY_TRANSFER,

    VKD3D_QUEUE_FAMILY_COUNT,
};

struct vkd3d_device_queue_info
{
    unsigned int family_index[VKD3D_QUEUE_FAMILY_COUNT];
    VkQueueFamilyProperties vk_properties[VKD3D_QUEUE_FAMILY_COUNT];

    unsigned int vk_family_count;
    VkDeviceQueueCreateInfo vk_queue_create_info[VKD3D_QUEUE_FAMILY_COUNT];
};

static void d3d12_device_destroy_vkd3d_queues(struct d3d12_device *device)
{
    if (device->direct_queue)
        vkd3d_queue_destroy(device->direct_queue, device);
    if (device->compute_queue && device->compute_queue != device->direct_queue)
        vkd3d_queue_destroy(device->compute_queue, device);
    if (device->copy_queue && device->copy_queue != device->direct_queue
            && device->copy_queue != device->compute_queue)
        vkd3d_queue_destroy(device->copy_queue, device);

    device->direct_queue = NULL;
    device->compute_queue = NULL;
    device->copy_queue = NULL;
}

static HRESULT d3d12_device_create_vkd3d_queues(struct d3d12_device *device,
        const struct vkd3d_device_queue_info *queue_info)
{
    uint32_t transfer_family_index = queue_info->family_index[VKD3D_QUEUE_FAMILY_TRANSFER];
    uint32_t compute_family_index = queue_info->family_index[VKD3D_QUEUE_FAMILY_COMPUTE];
    uint32_t direct_family_index = queue_info->family_index[VKD3D_QUEUE_FAMILY_DIRECT];
    HRESULT hr;

    device->direct_queue = NULL;
    device->compute_queue = NULL;
    device->copy_queue = NULL;

    device->queue_family_count = 0;
    memset(device->queue_family_indices, 0, sizeof(device->queue_family_indices));

    if (SUCCEEDED((hr = vkd3d_queue_create(device, direct_family_index,
            &queue_info->vk_properties[VKD3D_QUEUE_FAMILY_DIRECT], &device->direct_queue))))
        device->queue_family_indices[device->queue_family_count++] = direct_family_index;
    else
        goto out_destroy_queues;

    if (compute_family_index == direct_family_index)
        device->compute_queue = device->direct_queue;
    else if (SUCCEEDED(hr = vkd3d_queue_create(device, compute_family_index,
            &queue_info->vk_properties[VKD3D_QUEUE_FAMILY_COMPUTE], &device->compute_queue)))
        device->queue_family_indices[device->queue_family_count++] = compute_family_index;
    else
        goto out_destroy_queues;

    if (transfer_family_index == direct_family_index)
        device->copy_queue = device->direct_queue;
    else if (transfer_family_index == compute_family_index)
        device->copy_queue = device->compute_queue;
    else if (SUCCEEDED(hr = vkd3d_queue_create(device, transfer_family_index,
            &queue_info->vk_properties[VKD3D_QUEUE_FAMILY_TRANSFER], &device->copy_queue)))
        device->queue_family_indices[device->queue_family_count++] = transfer_family_index;
    else
        goto out_destroy_queues;

    device->feature_options3.CopyQueueTimestampQueriesSupported = !!device->copy_queue->timestamp_bits;

    return S_OK;

out_destroy_queues:
    d3d12_device_destroy_vkd3d_queues(device);
    return hr;
}

static float queue_priorities[] = {1.0f};

static HRESULT vkd3d_select_queues(const struct vkd3d_instance *vkd3d_instance,
        VkPhysicalDevice physical_device, struct vkd3d_device_queue_info *info)
{
    const struct vkd3d_vk_instance_procs *vk_procs = &vkd3d_instance->vk_procs;
    VkQueueFamilyProperties *queue_properties = NULL;
    VkDeviceQueueCreateInfo *queue_info = NULL;
    unsigned int i;
    uint32_t count;

    memset(info, 0, sizeof(*info));
    for (i = 0; i < ARRAY_SIZE(info->family_index); ++i)
        info->family_index[i] = ~0u;

    VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL));
    if (!(queue_properties = vkd3d_calloc(count, sizeof(*queue_properties))))
        return E_OUTOFMEMORY;
    VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_properties));

    for (i = 0; i < count; ++i)
    {
        enum vkd3d_queue_family vkd3d_family = VKD3D_QUEUE_FAMILY_COUNT;

        if ((queue_properties[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
                == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
        {
            vkd3d_family = VKD3D_QUEUE_FAMILY_DIRECT;
        }
        if ((queue_properties[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
                == VK_QUEUE_COMPUTE_BIT)
        {
            vkd3d_family = VKD3D_QUEUE_FAMILY_COMPUTE;
        }
        if ((queue_properties[i].queueFlags & ~VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_TRANSFER_BIT)
        {
            vkd3d_family = VKD3D_QUEUE_FAMILY_TRANSFER;
        }

        if (vkd3d_family == VKD3D_QUEUE_FAMILY_COUNT)
            continue;

        info->family_index[vkd3d_family] = i;
        info->vk_properties[vkd3d_family] = queue_properties[i];
        queue_info = &info->vk_queue_create_info[vkd3d_family];

        queue_info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info->pNext = NULL;
        queue_info->flags = 0;
        queue_info->queueFamilyIndex = i;
        queue_info->queueCount = 1; /* FIXME: Use multiple queues. */
        queue_info->pQueuePriorities = queue_priorities;
    }

    vkd3d_free(queue_properties);

    if (info->family_index[VKD3D_QUEUE_FAMILY_DIRECT] == ~0u)
    {
        FIXME("Could not find a suitable queue family for a direct command queue.\n");
        return E_FAIL;
    }

    /* No compute-only queue family, reuse the direct queue family with graphics and compute. */
    if (info->family_index[VKD3D_QUEUE_FAMILY_COMPUTE] == ~0u)
    {
        info->family_index[VKD3D_QUEUE_FAMILY_COMPUTE] = info->family_index[VKD3D_QUEUE_FAMILY_DIRECT];
        info->vk_properties[VKD3D_QUEUE_FAMILY_COMPUTE] = info->vk_properties[VKD3D_QUEUE_FAMILY_DIRECT];
    }
    if (info->family_index[VKD3D_QUEUE_FAMILY_TRANSFER] == ~0u)
    {
        info->family_index[VKD3D_QUEUE_FAMILY_TRANSFER] = info->family_index[VKD3D_QUEUE_FAMILY_DIRECT];
        info->vk_properties[VKD3D_QUEUE_FAMILY_TRANSFER] = info->vk_properties[VKD3D_QUEUE_FAMILY_DIRECT];
    }

    /* Compact the array. */
    info->vk_family_count = 1;
    for (i = info->vk_family_count; i < ARRAY_SIZE(info->vk_queue_create_info); ++i)
    {
        if (info->vk_queue_create_info[i].queueCount)
            info->vk_queue_create_info[info->vk_family_count++] = info->vk_queue_create_info[i];
    }

    return S_OK;
}

/* The 4 MiB alignment requirement for MSAA resources was lowered to 64KB on
 * hardware that supports it. This is distinct from the small MSAA requirement
 * which applies to resources of a total size of 4 MiB or less. */
static bool d3d12_is_64k_msaa_supported(struct d3d12_device *device)
{
    D3D12_RESOURCE_ALLOCATION_INFO info;
    D3D12_RESOURCE_DESC resource_desc;

    memset(&resource_desc, 0, sizeof(resource_desc));
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Width = 1024;
    resource_desc.Height = 1025;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resource_desc.SampleDesc.Count = 4;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    /* FIXME: in some cases Vulkan requires 0x20000 or more for non-MSAA
     * resources, which must have 0x10000 in their description, so we might
     * reasonably return true here for 0x20000 or 0x40000. */
    return SUCCEEDED(vkd3d_get_image_allocation_info(device, &resource_desc, &info))
            && info.Alignment <= 0x10000;
}

static HRESULT vkd3d_create_vk_device(struct d3d12_device *device,
        const struct vkd3d_device_create_info *create_info)
{
    const struct vkd3d_vk_instance_procs *vk_procs = &device->vkd3d_instance->vk_procs;
    const struct vkd3d_optional_device_extensions_info *optional_extensions;
    struct vkd3d_physical_device_info physical_device_info;
    struct vkd3d_device_queue_info device_queue_info;
    bool *user_extension_supported = NULL;
    VkPhysicalDevice physical_device;
    VkDeviceCreateInfo device_info;
    unsigned int device_index;
    uint32_t extension_count;
    const char **extensions;
    VkDevice vk_device;
    VkResult vr;
    HRESULT hr;

    TRACE("device %p, create_info %p.\n", device, create_info);

    physical_device = create_info->vk_physical_device;
    device_index = vkd3d_env_var_as_uint("VKD3D_VULKAN_DEVICE", ~0u);
    if ((!physical_device || device_index != ~0u)
            && FAILED(hr = vkd3d_select_physical_device(device->vkd3d_instance, device_index, &physical_device)))
        return hr;

    device->vk_physical_device = physical_device;

    if (FAILED(hr = vkd3d_select_queues(device->vkd3d_instance, physical_device, &device_queue_info)))
        return hr;

    TRACE("Using queue family %u for direct command queues.\n",
            device_queue_info.family_index[VKD3D_QUEUE_FAMILY_DIRECT]);
    TRACE("Using queue family %u for compute command queues.\n",
            device_queue_info.family_index[VKD3D_QUEUE_FAMILY_COMPUTE]);
    TRACE("Using queue family %u for copy command queues.\n",
            device_queue_info.family_index[VKD3D_QUEUE_FAMILY_TRANSFER]);

    VK_CALL(vkGetPhysicalDeviceMemoryProperties(physical_device, &device->memory_properties));

    vkd3d_physical_device_info_init(&physical_device_info, device);

    if (FAILED(hr = vkd3d_init_device_caps(device, create_info, &physical_device_info,
            &extension_count, &user_extension_supported)))
        return hr;

    if (!(extensions = vkd3d_calloc(extension_count, sizeof(*extensions))))
    {
        vkd3d_free(user_extension_supported);
        return E_OUTOFMEMORY;
    }

    optional_extensions = vkd3d_find_struct(create_info->next, OPTIONAL_DEVICE_EXTENSIONS_INFO);

    /* Create device */
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = physical_device_info.features2.pNext;
    device_info.flags = 0;
    device_info.queueCreateInfoCount = device_queue_info.vk_family_count;
    device_info.pQueueCreateInfos = device_queue_info.vk_queue_create_info;
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = NULL;
    device_info.enabledExtensionCount = vkd3d_enable_extensions(extensions,
            required_device_extensions, ARRAY_SIZE(required_device_extensions),
            optional_device_extensions, ARRAY_SIZE(optional_device_extensions),
            create_info->device_extensions, create_info->device_extension_count,
            optional_extensions ? optional_extensions->extensions : NULL,
            optional_extensions ? optional_extensions->extension_count : 0,
            user_extension_supported, &device->vk_info);
    device_info.ppEnabledExtensionNames = extensions;
    device_info.pEnabledFeatures = &physical_device_info.features2.features;
    vkd3d_free(user_extension_supported);

    vr = VK_CALL(vkCreateDevice(physical_device, &device_info, NULL, &vk_device));
    vkd3d_free(extensions);
    if (vr < 0)
    {
        ERR("Failed to create Vulkan device, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    if (FAILED(hr = vkd3d_load_vk_device_procs(&device->vk_procs, vk_procs, vk_device)))
    {
        ERR("Failed to load device procs, hr %#x.\n", hr);
        if (device->vk_procs.vkDestroyDevice)
            device->vk_procs.vkDestroyDevice(vk_device, NULL);
        return hr;
    }

    device->vk_device = vk_device;

    if (FAILED(hr = d3d12_device_create_vkd3d_queues(device, &device_queue_info)))
    {
        ERR("Failed to create queues, hr %#x.\n", hr);
        device->vk_procs.vkDestroyDevice(vk_device, NULL);
        return hr;
    }

    device->feature_options4.MSAA64KBAlignedTextureSupported = d3d12_is_64k_msaa_supported(device);

    TRACE("Created Vulkan device %p.\n", vk_device);

    return hr;
}

static HRESULT d3d12_device_init_pipeline_cache(struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkPipelineCacheCreateInfo cache_info;
    VkResult vr;
    int rc;

    if ((rc = vkd3d_mutex_init(&device->mutex)))
    {
        ERR("Failed to initialize mutex, error %d.\n", rc);
        return hresult_from_errno(rc);
    }

    cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cache_info.pNext = NULL;
    cache_info.flags = 0;
    cache_info.initialDataSize = 0;
    cache_info.pInitialData = NULL;
    if ((vr = VK_CALL(vkCreatePipelineCache(device->vk_device, &cache_info, NULL,
            &device->vk_pipeline_cache))) < 0)
    {
        ERR("Failed to create Vulkan pipeline cache, vr %d.\n", vr);
        device->vk_pipeline_cache = VK_NULL_HANDLE;
    }

    return S_OK;
}

static void d3d12_device_destroy_pipeline_cache(struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    if (device->vk_pipeline_cache)
        VK_CALL(vkDestroyPipelineCache(device->vk_device, device->vk_pipeline_cache, NULL));

    vkd3d_mutex_destroy(&device->mutex);
}

#define VKD3D_VA_FALLBACK_BASE      0x8000000000000000ull
#define VKD3D_VA_SLAB_BASE          0x0000001000000000ull
#define VKD3D_VA_SLAB_SIZE_SHIFT    32
#define VKD3D_VA_SLAB_SIZE          (1ull << VKD3D_VA_SLAB_SIZE_SHIFT)
#define VKD3D_VA_SLAB_COUNT         (64 * 1024)

static D3D12_GPU_VIRTUAL_ADDRESS vkd3d_gpu_va_allocator_allocate_slab(struct vkd3d_gpu_va_allocator *allocator,
        size_t aligned_size, void *ptr)
{
    struct vkd3d_gpu_va_slab *slab;
    D3D12_GPU_VIRTUAL_ADDRESS address;
    unsigned slab_idx;

    slab = allocator->free_slab;
    allocator->free_slab = slab->ptr;
    slab->size = aligned_size;
    slab->ptr = ptr;

    /* It is critical that the multiplication happens in 64-bit to not
     * overflow. */
    slab_idx = slab - allocator->slabs;
    address = VKD3D_VA_SLAB_BASE + slab_idx * VKD3D_VA_SLAB_SIZE;

    TRACE("Allocated address %#"PRIx64", slab %u, size %zu.\n", address, slab_idx, aligned_size);

    return address;
}

static D3D12_GPU_VIRTUAL_ADDRESS vkd3d_gpu_va_allocator_allocate_fallback(struct vkd3d_gpu_va_allocator *allocator,
        size_t alignment, size_t aligned_size, void *ptr)
{
    struct vkd3d_gpu_va_allocation *allocation;
    D3D12_GPU_VIRTUAL_ADDRESS base, ceiling;

    base = allocator->fallback_floor;
    ceiling = ~(D3D12_GPU_VIRTUAL_ADDRESS)0;
    ceiling -= alignment - 1;
    if (aligned_size > ceiling || ceiling - aligned_size < base)
        return 0;

    base = (base + (alignment - 1)) & ~((D3D12_GPU_VIRTUAL_ADDRESS)alignment - 1);

    if (!vkd3d_array_reserve((void **)&allocator->fallback_allocations, &allocator->fallback_allocations_size,
            allocator->fallback_allocation_count + 1, sizeof(*allocator->fallback_allocations)))
        return 0;

    allocation = &allocator->fallback_allocations[allocator->fallback_allocation_count++];
    allocation->base = base;
    allocation->size = aligned_size;
    allocation->ptr = ptr;

    /* This pointer is bumped and never lowered on a free. However, this will
     * only fail once we have exhausted 63 bits of address space. */
    allocator->fallback_floor = base + aligned_size;

    TRACE("Allocated address %#"PRIx64", size %zu.\n", base, aligned_size);

    return base;
}

D3D12_GPU_VIRTUAL_ADDRESS vkd3d_gpu_va_allocator_allocate(struct vkd3d_gpu_va_allocator *allocator,
        size_t alignment, size_t size, void *ptr)
{
    D3D12_GPU_VIRTUAL_ADDRESS address;
    int rc;

    if (size > ~(size_t)0 - (alignment - 1))
        return 0;
    size = align(size, alignment);

    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return 0;
    }

    if (size <= VKD3D_VA_SLAB_SIZE && allocator->free_slab)
        address = vkd3d_gpu_va_allocator_allocate_slab(allocator, size, ptr);
    else
        address = vkd3d_gpu_va_allocator_allocate_fallback(allocator, alignment, size, ptr);

    vkd3d_mutex_unlock(&allocator->mutex);

    return address;
}

static void *vkd3d_gpu_va_allocator_dereference_slab(struct vkd3d_gpu_va_allocator *allocator,
        D3D12_GPU_VIRTUAL_ADDRESS address)
{
    const struct vkd3d_gpu_va_slab *slab;
    D3D12_GPU_VIRTUAL_ADDRESS base_offset;
    unsigned int slab_idx;

    base_offset = address - VKD3D_VA_SLAB_BASE;
    slab_idx = base_offset >> VKD3D_VA_SLAB_SIZE_SHIFT;

    if (slab_idx >= VKD3D_VA_SLAB_COUNT)
    {
        ERR("Invalid slab index %u for address %#"PRIx64".\n", slab_idx, address);
        return NULL;
    }

    slab = &allocator->slabs[slab_idx];
    base_offset -= slab_idx * VKD3D_VA_SLAB_SIZE;
    if (base_offset >= slab->size)
    {
        ERR("Address %#"PRIx64" is %#"PRIx64" bytes into slab %u of size %zu.\n",
                address, base_offset, slab_idx, slab->size);
        return NULL;
    }
    return slab->ptr;
}

static int vkd3d_gpu_va_allocation_compare(const void *k, const void *e)
{
    const struct vkd3d_gpu_va_allocation *allocation = e;
    const D3D12_GPU_VIRTUAL_ADDRESS *address = k;

    if (*address < allocation->base)
        return -1;
    if (*address - allocation->base >= allocation->size)
        return 1;
    return 0;
}

static void *vkd3d_gpu_va_allocator_dereference_fallback(struct vkd3d_gpu_va_allocator *allocator,
        D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct vkd3d_gpu_va_allocation *allocation;

    allocation = bsearch(&address, allocator->fallback_allocations, allocator->fallback_allocation_count,
            sizeof(*allocation), vkd3d_gpu_va_allocation_compare);

    return allocation ? allocation->ptr : NULL;
}

void *vkd3d_gpu_va_allocator_dereference(struct vkd3d_gpu_va_allocator *allocator,
        D3D12_GPU_VIRTUAL_ADDRESS address)
{
    void *ret;
    int rc;

    /* If we land in the non-fallback region, dereferencing VA is lock-less.
     * The base pointer is immutable, and the only way we can have a data race
     * is if some other thread is poking into the
     * slab_mem_allocation[base_index] block. This can only happen if someone
     * is trying to free the entry while we're dereferencing it, which would
     * be a serious application bug. */
    if (address < VKD3D_VA_FALLBACK_BASE)
        return vkd3d_gpu_va_allocator_dereference_slab(allocator, address);

    /* Slow fallback. */
    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return NULL;
    }

    ret = vkd3d_gpu_va_allocator_dereference_fallback(allocator, address);

    vkd3d_mutex_unlock(&allocator->mutex);

    return ret;
}

static void vkd3d_gpu_va_allocator_free_slab(struct vkd3d_gpu_va_allocator *allocator,
        D3D12_GPU_VIRTUAL_ADDRESS address)
{
    D3D12_GPU_VIRTUAL_ADDRESS base_offset;
    struct vkd3d_gpu_va_slab *slab;
    unsigned int slab_idx;

    base_offset = address - VKD3D_VA_SLAB_BASE;
    slab_idx = base_offset >> VKD3D_VA_SLAB_SIZE_SHIFT;

    if (slab_idx >= VKD3D_VA_SLAB_COUNT)
    {
        ERR("Invalid slab index %u for address %#"PRIx64".\n", slab_idx, address);
        return;
    }

    TRACE("Freeing address %#"PRIx64", slab %u.\n", address, slab_idx);

    slab = &allocator->slabs[slab_idx];
    slab->size = 0;
    slab->ptr = allocator->free_slab;
    allocator->free_slab = slab;
}

static void vkd3d_gpu_va_allocator_free_fallback(struct vkd3d_gpu_va_allocator *allocator,
        D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct vkd3d_gpu_va_allocation *allocation;
    unsigned int index;

    allocation = bsearch(&address, allocator->fallback_allocations, allocator->fallback_allocation_count,
            sizeof(*allocation), vkd3d_gpu_va_allocation_compare);

    if (!allocation || allocation->base != address)
    {
        ERR("Address %#"PRIx64" does not match any allocation.\n", address);
        return;
    }

    index = allocation - allocator->fallback_allocations;
    --allocator->fallback_allocation_count;
    if (index != allocator->fallback_allocation_count)
        memmove(&allocator->fallback_allocations[index], &allocator->fallback_allocations[index + 1],
                (allocator->fallback_allocation_count - index) * sizeof(*allocation));
}

void vkd3d_gpu_va_allocator_free(struct vkd3d_gpu_va_allocator *allocator, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    int rc;

    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return;
    }

    if (address < VKD3D_VA_FALLBACK_BASE)
    {
        vkd3d_gpu_va_allocator_free_slab(allocator, address);
        vkd3d_mutex_unlock(&allocator->mutex);
        return;
    }

    vkd3d_gpu_va_allocator_free_fallback(allocator, address);

    vkd3d_mutex_unlock(&allocator->mutex);
}

static bool vkd3d_gpu_va_allocator_init(struct vkd3d_gpu_va_allocator *allocator)
{
    unsigned int i;
    int rc;

    memset(allocator, 0, sizeof(*allocator));
    allocator->fallback_floor = VKD3D_VA_FALLBACK_BASE;

    /* To remain lock-less, we cannot grow the slabs array after the fact. If
     * we commit to a maximum number of allocations here, we can dereference
     * without taking a lock as the base pointer never changes. We would be
     * able to grow more seamlessly using an array of pointers, but that would
     * make dereferencing slightly less efficient. */
    if (!(allocator->slabs = vkd3d_calloc(VKD3D_VA_SLAB_COUNT, sizeof(*allocator->slabs))))
        return false;

    /* Mark all slabs as free. */
    allocator->free_slab = &allocator->slabs[0];
    for (i = 0; i < VKD3D_VA_SLAB_COUNT - 1; ++i)
    {
        allocator->slabs[i].ptr = &allocator->slabs[i + 1];
    }

    if ((rc = vkd3d_mutex_init(&allocator->mutex)))
    {
        ERR("Failed to initialize mutex, error %d.\n", rc);
        vkd3d_free(allocator->slabs);
        return false;
    }

    return true;
}

static void vkd3d_gpu_va_allocator_cleanup(struct vkd3d_gpu_va_allocator *allocator)
{
    int rc;

    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return;
    }
    vkd3d_free(allocator->slabs);
    vkd3d_free(allocator->fallback_allocations);
    vkd3d_mutex_unlock(&allocator->mutex);
    vkd3d_mutex_destroy(&allocator->mutex);
}

/* We could use bsearch() or recursion here, but it probably helps to omit
 * all the extra function calls. */
static struct vkd3d_gpu_descriptor_allocation *vkd3d_gpu_descriptor_allocator_binary_search(
        const struct vkd3d_gpu_descriptor_allocator *allocator, const struct d3d12_desc *desc)
{
    struct vkd3d_gpu_descriptor_allocation *allocations = allocator->allocations;
    const struct d3d12_desc *base;
    size_t centre, count;

    for (count = allocator->allocation_count; count > 1; )
    {
        centre = count >> 1;
        base = allocations[centre].base;
        if (base <= desc)
        {
            allocations += centre;
            count -= centre;
        }
        else
        {
            count = centre;
        }
    }

    return allocations;
}

bool vkd3d_gpu_descriptor_allocator_register_range(struct vkd3d_gpu_descriptor_allocator *allocator,
        const struct d3d12_desc *base, size_t count)
{
    struct vkd3d_gpu_descriptor_allocation *allocation;
    int rc;

    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return false;
    }

    if (!vkd3d_array_reserve((void **)&allocator->allocations, &allocator->allocations_size,
            allocator->allocation_count + 1, sizeof(*allocator->allocations)))
    {
        vkd3d_mutex_unlock(&allocator->mutex);
        return false;
    }

    if (allocator->allocation_count > 1)
        allocation = vkd3d_gpu_descriptor_allocator_binary_search(allocator, base);
    else
        allocation = allocator->allocations;
    allocation += allocator->allocation_count && base > allocation->base;
    memmove(&allocation[1], allocation, (allocator->allocation_count++ - (allocation - allocator->allocations))
            * sizeof(*allocation));

    allocation->base = base;
    allocation->count = count;

    vkd3d_mutex_unlock(&allocator->mutex);

    return true;
}

bool vkd3d_gpu_descriptor_allocator_unregister_range(
        struct vkd3d_gpu_descriptor_allocator *allocator, const struct d3d12_desc *base)
{
    bool found;
    size_t i;
    int rc;

    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return false;
    }

    for (i = 0, found = false; i < allocator->allocation_count; ++i)
    {
        if (allocator->allocations[i].base != base)
            continue;

        memmove(&allocator->allocations[i], &allocator->allocations[i + 1],
                (--allocator->allocation_count - i) * sizeof(allocator->allocations[0]));

        found = true;
        break;
    }

    vkd3d_mutex_unlock(&allocator->mutex);

    return found;
}

static inline const struct vkd3d_gpu_descriptor_allocation *vkd3d_gpu_descriptor_allocator_allocation_from_descriptor(
        const struct vkd3d_gpu_descriptor_allocator *allocator, const struct d3d12_desc *desc)
{
    const struct vkd3d_gpu_descriptor_allocation *allocation;

    allocation = vkd3d_gpu_descriptor_allocator_binary_search(allocator, desc);
    return (desc >= allocation->base && desc - allocation->base < allocation->count) ? allocation : NULL;
}

/* Return the available size from the specified descriptor to the heap end. */
size_t vkd3d_gpu_descriptor_allocator_range_size_from_descriptor(
        struct vkd3d_gpu_descriptor_allocator *allocator, const struct d3d12_desc *desc)
{
    const struct vkd3d_gpu_descriptor_allocation *allocation;
    size_t remaining;
    int rc;

    assert(allocator->allocation_count);

    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return 0;
    }

    remaining = 0;
    if ((allocation = vkd3d_gpu_descriptor_allocator_allocation_from_descriptor(allocator, desc)))
        remaining = allocation->count - (desc - allocation->base);

    vkd3d_mutex_unlock(&allocator->mutex);

    return remaining;
}

struct d3d12_descriptor_heap *vkd3d_gpu_descriptor_allocator_heap_from_descriptor(
        struct vkd3d_gpu_descriptor_allocator *allocator, const struct d3d12_desc *desc)
{
    const struct vkd3d_gpu_descriptor_allocation *allocation;
    int rc;

    if (!allocator->allocation_count)
        return NULL;

    if ((rc = vkd3d_mutex_lock(&allocator->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return NULL;
    }

    allocation = vkd3d_gpu_descriptor_allocator_allocation_from_descriptor(allocator, desc);

    vkd3d_mutex_unlock(&allocator->mutex);

    return allocation ? CONTAINING_RECORD(allocation->base, struct d3d12_descriptor_heap, descriptors)
            : NULL;
}

static bool vkd3d_gpu_descriptor_allocator_init(struct vkd3d_gpu_descriptor_allocator *allocator)
{
    int rc;

    memset(allocator, 0, sizeof(*allocator));
    if ((rc = vkd3d_mutex_init(&allocator->mutex)))
    {
        ERR("Failed to initialise mutex, error %d.\n", rc);
        return false;
    }

    return true;
}

static void vkd3d_gpu_descriptor_allocator_cleanup(struct vkd3d_gpu_descriptor_allocator *allocator)
{
    vkd3d_free(allocator->allocations);
    vkd3d_mutex_destroy(&allocator->mutex);
}

static bool have_vk_time_domain(VkTimeDomainEXT *domains, unsigned int count, VkTimeDomainEXT domain)
{
    unsigned int i;

    for (i = 0; i < count; ++i)
        if (domains[i] == domain)
            return true;

    return false;
}

static void vkd3d_time_domains_init(struct d3d12_device *device)
{
    static const VkTimeDomainEXT host_time_domains[] =
    {
        /* In order of preference */
        VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT,
        VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT,
        VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT,

    };
    const struct vkd3d_vk_instance_procs *vk_procs = &device->vkd3d_instance->vk_procs;
    VkTimeDomainEXT domains[8];
    unsigned int i;
    uint32_t count;
    VkResult vr;

    device->vk_host_time_domain = -1;

    if (!device->vk_info.EXT_calibrated_timestamps)
        return;

    count = ARRAY_SIZE(domains);
    if ((vr = VK_CALL(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(device->vk_physical_device,
            &count, domains))) != VK_SUCCESS && vr != VK_INCOMPLETE)
    {
        WARN("Failed to get calibrated time domains, vr %d.\n", vr);
        return;
    }

    if (vr == VK_INCOMPLETE)
        FIXME("Calibrated time domain list is incomplete.\n");

    if (!have_vk_time_domain(domains, count, VK_TIME_DOMAIN_DEVICE_EXT))
    {
        WARN("Device time domain not found. Calibrated timestamps will not be available.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(host_time_domains); ++i)
    {
        if (!have_vk_time_domain(domains, count, host_time_domains[i]))
            continue;
        device->vk_host_time_domain = host_time_domains[i];
        break;
    }
    if (device->vk_host_time_domain == -1)
        WARN("Found no acceptable host time domain. Calibrated timestamps will not be available.\n");
}

static void vkd3d_init_descriptor_pool_sizes(VkDescriptorPoolSize *pool_sizes,
        const struct vkd3d_device_descriptor_limits *limits)
{
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = min(limits->uniform_buffer_max_descriptors,
            VKD3D_MAX_VIRTUAL_HEAP_DESCRIPTORS_PER_TYPE);
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    pool_sizes[1].descriptorCount = min(limits->sampled_image_max_descriptors,
            VKD3D_MAX_VIRTUAL_HEAP_DESCRIPTORS_PER_TYPE);
    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    pool_sizes[2].descriptorCount = pool_sizes[1].descriptorCount;
    pool_sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    pool_sizes[3].descriptorCount = min(limits->storage_image_max_descriptors,
            VKD3D_MAX_VIRTUAL_HEAP_DESCRIPTORS_PER_TYPE);
    pool_sizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pool_sizes[4].descriptorCount = pool_sizes[3].descriptorCount;
    pool_sizes[5].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    pool_sizes[5].descriptorCount = min(limits->sampler_max_descriptors,
            VKD3D_MAX_VIRTUAL_HEAP_DESCRIPTORS_PER_TYPE);
};

/* ID3D12Device */
static inline struct d3d12_device *impl_from_ID3D12Device(ID3D12Device *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_device, ID3D12Device_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_QueryInterface(ID3D12Device *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12Device)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12Device_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_device_AddRef(ID3D12Device *iface)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    ULONG refcount = InterlockedIncrement(&device->refcount);

    TRACE("%p increasing refcount to %u.\n", device, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d12_device_Release(ID3D12Device *iface)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    ULONG refcount = InterlockedDecrement(&device->refcount);
    size_t i;

    TRACE("%p decreasing refcount to %u.\n", device, refcount);

    if (!refcount)
    {
        const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

        vkd3d_private_store_destroy(&device->private_store);

        vkd3d_cleanup_format_info(device);
        vkd3d_vk_descriptor_heap_layouts_cleanup(device);
        vkd3d_uav_clear_state_cleanup(&device->uav_clear_state, device);
        vkd3d_destroy_null_resources(&device->null_resources, device);
        vkd3d_gpu_va_allocator_cleanup(&device->gpu_va_allocator);
        vkd3d_gpu_descriptor_allocator_cleanup(&device->gpu_descriptor_allocator);
        vkd3d_render_pass_cache_cleanup(&device->render_pass_cache, device);
        d3d12_device_destroy_pipeline_cache(device);
        d3d12_device_destroy_vkd3d_queues(device);
        for (i = 0; i < ARRAY_SIZE(device->desc_mutex); ++i)
            vkd3d_mutex_destroy(&device->desc_mutex[i]);
        VK_CALL(vkDestroyDevice(device->vk_device, NULL));
        if (device->parent)
            IUnknown_Release(device->parent);
        vkd3d_instance_decref(device->vkd3d_instance);

        vkd3d_free(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_GetPrivateData(ID3D12Device *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&device->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_SetPrivateData(ID3D12Device *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&device->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_SetPrivateDataInterface(ID3D12Device *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&device->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_SetName(ID3D12Device *iface, const WCHAR *name)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, device->wchar_size));

    return vkd3d_set_vk_object_name(device, (uint64_t)(uintptr_t)device->vk_device,
            VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, name);
}

static UINT STDMETHODCALLTYPE d3d12_device_GetNodeCount(ID3D12Device *iface)
{
    TRACE("iface %p.\n", iface);

    return 1;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateCommandQueue(ID3D12Device *iface,
        const D3D12_COMMAND_QUEUE_DESC *desc, REFIID riid, void **command_queue)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_command_queue *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, riid %s, command_queue %p.\n",
            iface, desc, debugstr_guid(riid), command_queue);

    if (FAILED(hr = d3d12_command_queue_create(device, desc, &object)))
        return hr;

    return return_interface(&object->ID3D12CommandQueue_iface, &IID_ID3D12CommandQueue,
            riid, command_queue);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateCommandAllocator(ID3D12Device *iface,
        D3D12_COMMAND_LIST_TYPE type, REFIID riid, void **command_allocator)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_command_allocator *object;
    HRESULT hr;

    TRACE("iface %p, type %#x, riid %s, command_allocator %p.\n",
            iface, type, debugstr_guid(riid), command_allocator);

    if (FAILED(hr = d3d12_command_allocator_create(device, type, &object)))
        return hr;

    return return_interface(&object->ID3D12CommandAllocator_iface, &IID_ID3D12CommandAllocator,
            riid, command_allocator);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateGraphicsPipelineState(ID3D12Device *iface,
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc, REFIID riid, void **pipeline_state)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_pipeline_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, riid %s, pipeline_state %p.\n",
            iface, desc, debugstr_guid(riid), pipeline_state);

    if (FAILED(hr = d3d12_pipeline_state_create_graphics(device, desc, &object)))
        return hr;

    return return_interface(&object->ID3D12PipelineState_iface,
            &IID_ID3D12PipelineState, riid, pipeline_state);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateComputePipelineState(ID3D12Device *iface,
        const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, REFIID riid, void **pipeline_state)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_pipeline_state *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, riid %s, pipeline_state %p.\n",
            iface, desc, debugstr_guid(riid), pipeline_state);

    if (FAILED(hr = d3d12_pipeline_state_create_compute(device, desc, &object)))
        return hr;

    return return_interface(&object->ID3D12PipelineState_iface,
            &IID_ID3D12PipelineState, riid, pipeline_state);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateCommandList(ID3D12Device *iface,
        UINT node_mask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *command_allocator,
        ID3D12PipelineState *initial_pipeline_state, REFIID riid, void **command_list)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_command_list *object;
    HRESULT hr;

    TRACE("iface %p, node_mask 0x%08x, type %#x, command_allocator %p, "
            "initial_pipeline_state %p, riid %s, command_list %p.\n",
            iface, node_mask, type, command_allocator,
            initial_pipeline_state, debugstr_guid(riid), command_list);

    if (FAILED(hr = d3d12_command_list_create(device, node_mask, type, command_allocator,
            initial_pipeline_state, &object)))
        return hr;

    return return_interface(&object->ID3D12GraphicsCommandList2_iface,
            &IID_ID3D12GraphicsCommandList2, riid, command_list);
}

/* Direct3D feature levels restrict which formats can be optionally supported. */
static void vkd3d_restrict_format_support_for_feature_level(D3D12_FEATURE_DATA_FORMAT_SUPPORT *format_support)
{
    static const D3D12_FEATURE_DATA_FORMAT_SUPPORT blacklisted_format_features[] =
    {
        {DXGI_FORMAT_B8G8R8A8_TYPELESS, D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW,
                D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,    D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW,
                D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(blacklisted_format_features); ++i)
    {
        if (blacklisted_format_features[i].Format == format_support->Format)
        {
            format_support->Support1 &= ~blacklisted_format_features[i].Support1;
            format_support->Support2 &= ~blacklisted_format_features[i].Support2;
            break;
        }
    }
}

static HRESULT d3d12_device_check_multisample_quality_levels(struct d3d12_device *device,
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS *data)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkImageFormatProperties vk_properties;
    const struct vkd3d_format *format;
    VkSampleCountFlagBits vk_samples;
    VkImageUsageFlags vk_usage = 0;
    VkResult vr;

    TRACE("Format %#x, sample count %u, flags %#x.\n", data->Format, data->SampleCount, data->Flags);

    data->NumQualityLevels = 0;

    if (!(vk_samples = vk_samples_from_sample_count(data->SampleCount)))
        WARN("Invalid sample count %u.\n", data->SampleCount);
    if (!data->SampleCount)
        return E_FAIL;

    if (data->SampleCount == 1)
    {
        data->NumQualityLevels = 1;
        goto done;
    }

    if (data->Format == DXGI_FORMAT_UNKNOWN)
        goto done;

    if (!(format = vkd3d_get_format(device, data->Format, false)))
        format = vkd3d_get_format(device, data->Format, true);
    if (!format)
    {
        FIXME("Unhandled format %#x.\n", data->Format);
        return E_INVALIDARG;
    }
    if (data->Flags)
        FIXME("Ignoring flags %#x.\n", data->Flags);

    if (format->vk_aspect_mask & VK_IMAGE_ASPECT_COLOR_BIT)
        vk_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else
        vk_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    vr = VK_CALL(vkGetPhysicalDeviceImageFormatProperties(device->vk_physical_device,
            format->vk_format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, vk_usage, 0, &vk_properties));
    if (vr == VK_ERROR_FORMAT_NOT_SUPPORTED)
    {
        WARN("Format %#x is not supported.\n", format->dxgi_format);
        goto done;
    }
    if (vr < 0)
    {
        ERR("Failed to get image format properties, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    if (vk_properties.sampleCounts & vk_samples)
        data->NumQualityLevels = 1;

done:
    TRACE("Returning %u quality levels.\n", data->NumQualityLevels);
    return S_OK;
}

bool d3d12_device_is_uma(struct d3d12_device *device, bool *coherent)
{
    unsigned int i;

    if (coherent)
        *coherent = true;

    for (i = 0; i < device->memory_properties.memoryTypeCount; ++i)
    {
        if (!(device->memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            return false;
        if (coherent && !(device->memory_properties.memoryTypes[i].propertyFlags
                & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            *coherent = false;
    }

    return true;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CheckFeatureSupport(ID3D12Device *iface,
        D3D12_FEATURE feature, void *feature_data, UINT feature_data_size)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    TRACE("iface %p, feature %#x, feature_data %p, feature_data_size %u.\n",
            iface, feature, feature_data, feature_data_size);

    switch (feature)
    {
        case D3D12_FEATURE_D3D12_OPTIONS:
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            *data = device->feature_options;

            TRACE("Double precision shader ops %#x.\n", data->DoublePrecisionFloatShaderOps);
            TRACE("Output merger logic op %#x.\n", data->OutputMergerLogicOp);
            TRACE("Shader min precision support %#x.\n", data->MinPrecisionSupport);
            TRACE("Tiled resources tier %#x.\n", data->TiledResourcesTier);
            TRACE("Resource binding tier %#x.\n", data->ResourceBindingTier);
            TRACE("PS specified stencil ref %#x.\n", data->PSSpecifiedStencilRefSupported);
            TRACE("Typed UAV load and additional formats %#x.\n", data->TypedUAVLoadAdditionalFormats);
            TRACE("ROV %#x.\n", data->ROVsSupported);
            TRACE("Conservative rasterization tier %#x.\n", data->ConservativeRasterizationTier);
            TRACE("Max GPU virtual address bits per resource %u.\n", data->MaxGPUVirtualAddressBitsPerResource);
            TRACE("Standard swizzle 64KB %#x.\n", data->StandardSwizzle64KBSupported);
            TRACE("Cross-node sharing tier %#x.\n", data->CrossNodeSharingTier);
            TRACE("Cross-adapter row-major texture %#x.\n", data->CrossAdapterRowMajorTextureSupported);
            TRACE("VP and RT array index from any shader without GS emulation %#x.\n",
                    data->VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation);
            TRACE("Resource heap tier %#x.\n", data->ResourceHeapTier);
            return S_OK;
        }

        case D3D12_FEATURE_ARCHITECTURE:
        {
            D3D12_FEATURE_DATA_ARCHITECTURE *data = feature_data;
            bool coherent;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            if (data->NodeIndex)
            {
                FIXME("Multi-adapter not supported.\n");
                return E_INVALIDARG;
            }

            WARN("Assuming device does not support tile based rendering.\n");
            data->TileBasedRenderer = FALSE;

            data->UMA = d3d12_device_is_uma(device, &coherent);
            data->CacheCoherentUMA = data->UMA && coherent;

            TRACE("Tile based renderer %#x, UMA %#x, cache coherent UMA %#x.\n",
                    data->TileBasedRenderer, data->UMA, data->CacheCoherentUMA);
            return S_OK;
        }

        case D3D12_FEATURE_FEATURE_LEVELS:
        {
            struct vkd3d_vulkan_info *vulkan_info = &device->vk_info;
            D3D12_FEATURE_DATA_FEATURE_LEVELS *data = feature_data;
            unsigned int i;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }
            if (!data->NumFeatureLevels)
                return E_INVALIDARG;

            data->MaxSupportedFeatureLevel = 0;
            for (i = 0; i < data->NumFeatureLevels; ++i)
            {
                D3D_FEATURE_LEVEL fl = data->pFeatureLevelsRequested[i];
                if (data->MaxSupportedFeatureLevel < fl && fl <= vulkan_info->max_feature_level)
                    data->MaxSupportedFeatureLevel = fl;
            }

            TRACE("Max supported feature level %#x.\n", data->MaxSupportedFeatureLevel);
            return S_OK;
        }

        case D3D12_FEATURE_FORMAT_SUPPORT:
        {
            const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
            D3D12_FEATURE_DATA_FORMAT_SUPPORT *data = feature_data;
            VkFormatFeatureFlagBits image_features;
            const struct vkd3d_format *format;
            VkFormatProperties properties;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            data->Support1 = D3D12_FORMAT_SUPPORT1_NONE;
            data->Support2 = D3D12_FORMAT_SUPPORT2_NONE;
            if (!(format = vkd3d_get_format(device, data->Format, false)))
                format = vkd3d_get_format(device, data->Format, true);
            if (!format)
            {
                FIXME("Unhandled format %#x.\n", data->Format);
                return E_INVALIDARG;
            }

            VK_CALL(vkGetPhysicalDeviceFormatProperties(device->vk_physical_device, format->vk_format, &properties));
            image_features = properties.linearTilingFeatures | properties.optimalTilingFeatures;

            if (properties.bufferFeatures)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_BUFFER;
            if (properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER;
            if (data->Format == DXGI_FORMAT_R16_UINT || data->Format == DXGI_FORMAT_R32_UINT)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_IA_INDEX_BUFFER;
            if (image_features)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_TEXTURE1D | D3D12_FORMAT_SUPPORT1_TEXTURE2D
                        | D3D12_FORMAT_SUPPORT1_TEXTURE3D | D3D12_FORMAT_SUPPORT1_TEXTURECUBE;
            if (image_features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            {
                data->Support1 |= D3D12_FORMAT_SUPPORT1_SHADER_LOAD | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD
                        | D3D12_FORMAT_SUPPORT1_SHADER_GATHER;
                if (image_features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
                {
                    data->Support1 |= D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE
                            | D3D12_FORMAT_SUPPORT1_MIP;
                }
                if (format->vk_aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT)
                    data->Support1 |= D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_COMPARISON
                            | D3D12_FORMAT_SUPPORT1_SHADER_GATHER_COMPARISON;
            }
            if (image_features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_RENDER_TARGET | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
            if (image_features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_BLENDABLE;
            if (image_features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL;
            if (image_features & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE;
            if (image_features & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
                data->Support1 |= D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW;

            if (image_features & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)
                data->Support2 |= D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD
                        | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS
                        | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE
                        | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE
                        | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX
                        | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX;

            vkd3d_restrict_format_support_for_feature_level(data);

            TRACE("Format %#x, support1 %#x, support2 %#x.\n", data->Format, data->Support1, data->Support2);
            return S_OK;
        }

        case D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS:
        {
            D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            return d3d12_device_check_multisample_quality_levels(device, data);
        }

        case D3D12_FEATURE_FORMAT_INFO:
        {
            D3D12_FEATURE_DATA_FORMAT_INFO *data = feature_data;
            const struct vkd3d_format *format;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            if (data->Format == DXGI_FORMAT_UNKNOWN)
            {
                data->PlaneCount = 1;
                return S_OK;
            }

            if (!(format = vkd3d_get_format(device, data->Format, false)))
                format = vkd3d_get_format(device, data->Format, true);
            if (!format)
            {
                FIXME("Unhandled format %#x.\n", data->Format);
                return E_INVALIDARG;
            }

            data->PlaneCount = format->plane_count;

            TRACE("Format %#x, plane count %"PRIu8".\n", data->Format, data->PlaneCount);
            return S_OK;
        }

        case D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT:
        {
            const D3D12_FEATURE_DATA_D3D12_OPTIONS *options = &device->feature_options;
            D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            data->MaxGPUVirtualAddressBitsPerResource = options->MaxGPUVirtualAddressBitsPerResource;
            data->MaxGPUVirtualAddressBitsPerProcess = options->MaxGPUVirtualAddressBitsPerResource;

            TRACE("Max GPU virtual address bits per resource %u, Max GPU virtual address bits per process %u.\n",
                    data->MaxGPUVirtualAddressBitsPerResource, data->MaxGPUVirtualAddressBitsPerProcess);
            return S_OK;
        }

        case D3D12_FEATURE_SHADER_MODEL:
        {
            D3D12_FEATURE_DATA_SHADER_MODEL *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            TRACE("Request shader model %#x.\n", data->HighestShaderModel);

            data->HighestShaderModel = D3D_SHADER_MODEL_5_1;

            TRACE("Shader model %#x.\n", data->HighestShaderModel);
            return S_OK;
        }

        case D3D12_FEATURE_D3D12_OPTIONS1:
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS1 *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            *data = device->feature_options1;

            TRACE("Wave ops %#x.\n", data->WaveOps);
            TRACE("Min wave lane count %#x.\n", data->WaveLaneCountMin);
            TRACE("Max wave lane count %#x.\n", data->WaveLaneCountMax);
            TRACE("Total lane count %#x.\n", data->TotalLaneCount);
            TRACE("Expanded compute resource states %#x.\n", data->ExpandedComputeResourceStates);
            TRACE("Int64 shader ops %#x.\n", data->Int64ShaderOps);
            return S_OK;
        }

        case D3D12_FEATURE_ROOT_SIGNATURE:
        {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            TRACE("Root signature requested %#x.\n", data->HighestVersion);
            data->HighestVersion = min(data->HighestVersion, D3D_ROOT_SIGNATURE_VERSION_1_1);
            if (device->vkd3d_instance->api_version < VKD3D_API_VERSION_1_2)
                data->HighestVersion = min(data->HighestVersion, D3D_ROOT_SIGNATURE_VERSION_1_0);

            TRACE("Root signature version %#x.\n", data->HighestVersion);
            return S_OK;
        }

        case D3D12_FEATURE_ARCHITECTURE1:
        {
            D3D12_FEATURE_DATA_ARCHITECTURE1 *data = feature_data;
            bool coherent;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            if (data->NodeIndex)
            {
                FIXME("Multi-adapter not supported.\n");
                return E_INVALIDARG;
            }

            WARN("Assuming device does not support tile based rendering.\n");
            data->TileBasedRenderer = FALSE;

            data->UMA = d3d12_device_is_uma(device, &coherent);
            data->CacheCoherentUMA = data->UMA && coherent;

            WARN("Assuming device does not have an isolated memory management unit.\n");
            data->IsolatedMMU = FALSE;

            TRACE("Tile based renderer %#x, UMA %#x, cache coherent UMA %#x, isolated MMU %#x.\n",
                    data->TileBasedRenderer, data->UMA, data->CacheCoherentUMA, data->IsolatedMMU);
            return S_OK;
        }

        case D3D12_FEATURE_D3D12_OPTIONS2:
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS2 *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            *data = device->feature_options2;

            TRACE("Depth bounds test %#x.\n", data->DepthBoundsTestSupported);
            TRACE("Programmable sample positions tier %#x.\n", data->ProgrammableSamplePositionsTier);
            return S_OK;
        }

        case D3D12_FEATURE_SHADER_CACHE:
        {
            D3D12_FEATURE_DATA_SHADER_CACHE *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            /* FIXME: The d3d12 documentation states that
             * D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO is always supported, but
             * the CachedPSO field of D3D12_GRAPHICS_PIPELINE_STATE_DESC is
             * ignored and GetCachedBlob() is a stub. */
            data->SupportFlags = D3D12_SHADER_CACHE_SUPPORT_NONE;

            TRACE("Shader cache support %#x.\n", data->SupportFlags);
            return S_OK;
        }

        case D3D12_FEATURE_COMMAND_QUEUE_PRIORITY:
        {
            D3D12_FEATURE_DATA_COMMAND_QUEUE_PRIORITY *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            switch (data->CommandListType)
            {
                case D3D12_COMMAND_LIST_TYPE_DIRECT:
                case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                case D3D12_COMMAND_LIST_TYPE_COPY:
                    data->PriorityForTypeIsSupported = FALSE;
                    TRACE("Command list type %#x, priority %u, supported %#x.\n",
                            data->CommandListType, data->Priority, data->PriorityForTypeIsSupported);
                    return S_OK;

                default:
                    FIXME("Unhandled command list type %#x.\n", data->CommandListType);
                    return E_INVALIDARG;
            }
        }

        case D3D12_FEATURE_D3D12_OPTIONS3:
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS3 *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            *data = device->feature_options3;

            TRACE("Copy queue timestamp queries %#x.\n", data->CopyQueueTimestampQueriesSupported);
            TRACE("Casting fully typed format %#x.\n", data->CastingFullyTypedFormatSupported);
            TRACE("Write buffer immediate %#x.\n", data->WriteBufferImmediateSupportFlags);
            TRACE("View instancing tier %#x.\n", data->ViewInstancingTier);
            TRACE("Barycentrics %#x.\n", data->BarycentricsSupported);
            return S_OK;
        }

        case D3D12_FEATURE_EXISTING_HEAPS:
        {
            D3D12_FEATURE_DATA_EXISTING_HEAPS *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            data->Supported = FALSE;

            TRACE("Existing heaps %#x.\n", data->Supported);
            return S_OK;
        }

        case D3D12_FEATURE_D3D12_OPTIONS4:
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS4 *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            *data = device->feature_options4;

            TRACE("64 KiB aligned MSAA textures %#x.\n", data->MSAA64KBAlignedTextureSupported);
            TRACE("Shared resource compatibility tier %#x.\n", data->SharedResourceCompatibilityTier);
            TRACE("Native 16-bit shader ops %#x.\n", data->Native16BitShaderOpsSupported);
            return S_OK;
        }

        case D3D12_FEATURE_SERIALIZATION:
        {
            D3D12_FEATURE_DATA_SERIALIZATION *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            if (data->NodeIndex)
            {
                FIXME("Multi-adapter not supported.\n");
                return E_INVALIDARG;
            }

            data->HeapSerializationTier = D3D12_HEAP_SERIALIZATION_TIER_0;

            TRACE("Heap serialisation tier %#x.\n", data->HeapSerializationTier);
            return S_OK;
        }

        case D3D12_FEATURE_CROSS_NODE:
        {
            D3D12_FEATURE_DATA_CROSS_NODE *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            data->SharingTier = device->feature_options.CrossNodeSharingTier;
            data->AtomicShaderInstructions = FALSE;

            TRACE("Cross node sharing tier %#x.\n", data->SharingTier);
            TRACE("Cross node shader atomics %#x.\n", data->AtomicShaderInstructions);
            return S_OK;
        }

        case D3D12_FEATURE_D3D12_OPTIONS5:
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 *data = feature_data;

            if (feature_data_size != sizeof(*data))
            {
                WARN("Invalid size %u.\n", feature_data_size);
                return E_INVALIDARG;
            }

            *data = device->feature_options5;

            TRACE("SRV tiled resource tier 3 only %#x.\n", data->SRVOnlyTiledResourceTier3);
            TRACE("Render pass tier %#x.\n", data->RenderPassesTier);
            TRACE("Ray tracing tier %#x.\n", data->RaytracingTier);
            return S_OK;
        }

        default:
            FIXME("Unhandled feature %#x.\n", feature);
            return E_NOTIMPL;
    }
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateDescriptorHeap(ID3D12Device *iface,
        const D3D12_DESCRIPTOR_HEAP_DESC *desc, REFIID riid, void **descriptor_heap)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_descriptor_heap *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, riid %s, descriptor_heap %p.\n",
            iface, desc, debugstr_guid(riid), descriptor_heap);

    if (FAILED(hr = d3d12_descriptor_heap_create(device, desc, &object)))
        return hr;

    return return_interface(&object->ID3D12DescriptorHeap_iface,
            &IID_ID3D12DescriptorHeap, riid, descriptor_heap);
}

static UINT STDMETHODCALLTYPE d3d12_device_GetDescriptorHandleIncrementSize(ID3D12Device *iface,
        D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type)
{
    TRACE("iface %p, descriptor_heap_type %#x.\n", iface, descriptor_heap_type);

    switch (descriptor_heap_type)
    {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return sizeof(struct d3d12_desc);

        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            return sizeof(struct d3d12_rtv_desc);

        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            return sizeof(struct d3d12_dsv_desc);

        default:
            FIXME("Unhandled type %#x.\n", descriptor_heap_type);
            return 0;
    }
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateRootSignature(ID3D12Device *iface,
        UINT node_mask, const void *bytecode, SIZE_T bytecode_length,
        REFIID riid, void **root_signature)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_root_signature *object;
    HRESULT hr;

    TRACE("iface %p, node_mask 0x%08x, bytecode %p, bytecode_length %lu, riid %s, root_signature %p.\n",
            iface, node_mask, bytecode, bytecode_length, debugstr_guid(riid), root_signature);

    debug_ignored_node_mask(node_mask);

    if (FAILED(hr = d3d12_root_signature_create(device, bytecode, bytecode_length, &object)))
        return hr;

    return return_interface(&object->ID3D12RootSignature_iface,
            &IID_ID3D12RootSignature, riid, root_signature);
}

static void STDMETHODCALLTYPE d3d12_device_CreateConstantBufferView(ID3D12Device *iface,
        const D3D12_CONSTANT_BUFFER_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_desc tmp = {0};

    TRACE("iface %p, desc %p, descriptor %#lx.\n", iface, desc, descriptor.ptr);

    d3d12_desc_create_cbv(&tmp, device, desc);
    d3d12_desc_write_atomic(d3d12_desc_from_cpu_handle(descriptor), &tmp, device);
}

static void STDMETHODCALLTYPE d3d12_device_CreateShaderResourceView(ID3D12Device *iface,
        ID3D12Resource *resource, const D3D12_SHADER_RESOURCE_VIEW_DESC *desc,
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_desc tmp = {0};

    TRACE("iface %p, resource %p, desc %p, descriptor %#lx.\n",
            iface, resource, desc, descriptor.ptr);

    d3d12_desc_create_srv(&tmp, device, unsafe_impl_from_ID3D12Resource(resource), desc);
    d3d12_desc_write_atomic(d3d12_desc_from_cpu_handle(descriptor), &tmp, device);
}

static void STDMETHODCALLTYPE d3d12_device_CreateUnorderedAccessView(ID3D12Device *iface,
        ID3D12Resource *resource, ID3D12Resource *counter_resource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_desc tmp = {0};

    TRACE("iface %p, resource %p, counter_resource %p, desc %p, descriptor %#lx.\n",
            iface, resource, counter_resource, desc, descriptor.ptr);

    d3d12_desc_create_uav(&tmp, device, unsafe_impl_from_ID3D12Resource(resource),
            unsafe_impl_from_ID3D12Resource(counter_resource), desc);
    d3d12_desc_write_atomic(d3d12_desc_from_cpu_handle(descriptor), &tmp, device);
}

static void STDMETHODCALLTYPE d3d12_device_CreateRenderTargetView(ID3D12Device *iface,
        ID3D12Resource *resource, const D3D12_RENDER_TARGET_VIEW_DESC *desc,
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    TRACE("iface %p, resource %p, desc %p, descriptor %#lx.\n",
            iface, resource, desc, descriptor.ptr);

    d3d12_rtv_desc_create_rtv(d3d12_rtv_desc_from_cpu_handle(descriptor),
            impl_from_ID3D12Device(iface), unsafe_impl_from_ID3D12Resource(resource), desc);
}

static void STDMETHODCALLTYPE d3d12_device_CreateDepthStencilView(ID3D12Device *iface,
        ID3D12Resource *resource, const D3D12_DEPTH_STENCIL_VIEW_DESC *desc,
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    TRACE("iface %p, resource %p, desc %p, descriptor %#lx.\n",
            iface, resource, desc, descriptor.ptr);

    d3d12_dsv_desc_create_dsv(d3d12_dsv_desc_from_cpu_handle(descriptor),
            impl_from_ID3D12Device(iface), unsafe_impl_from_ID3D12Resource(resource), desc);
}

static void STDMETHODCALLTYPE d3d12_device_CreateSampler(ID3D12Device *iface,
        const D3D12_SAMPLER_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_desc tmp = {0};

    TRACE("iface %p, desc %p, descriptor %#lx.\n", iface, desc, descriptor.ptr);

    d3d12_desc_create_sampler(&tmp, device, desc);
    d3d12_desc_write_atomic(d3d12_desc_from_cpu_handle(descriptor), &tmp, device);
}

static void flush_desc_writes(struct d3d12_desc_copy_location locations[][VKD3D_DESCRIPTOR_WRITE_BUFFER_SIZE],
        struct d3d12_desc_copy_info *infos, struct d3d12_descriptor_heap *descriptor_heap, struct d3d12_device *device)
{
    enum vkd3d_vk_descriptor_set_index set;
    for (set = 0; set < VKD3D_SET_INDEX_COUNT; ++set)
    {
        if (!infos[set].count)
            continue;
        d3d12_desc_copy_vk_heap_range(locations[set], &infos[set], descriptor_heap, set, device);
        infos[set].count = 0;
        infos[set].uav_counter = false;
    }
}

static void d3d12_desc_buffered_copy_atomic(struct d3d12_desc *dst, const struct d3d12_desc *src,
        struct d3d12_desc_copy_location locations[][VKD3D_DESCRIPTOR_WRITE_BUFFER_SIZE],
        struct d3d12_desc_copy_info *infos, struct d3d12_descriptor_heap *descriptor_heap, struct d3d12_device *device)
{
    struct d3d12_desc_copy_location *location;
    enum vkd3d_vk_descriptor_set_index set;
    struct vkd3d_mutex *mutex;

    mutex = d3d12_device_get_descriptor_mutex(device, src);
    vkd3d_mutex_lock(mutex);

    if (src->magic == VKD3D_DESCRIPTOR_MAGIC_FREE)
    {
        /* Source must be unlocked first, and therefore can't be used as a null source. */
        static const struct d3d12_desc null = {0};
        vkd3d_mutex_unlock(mutex);
        d3d12_desc_write_atomic(dst, &null, device);
        return;
    }

    set = vkd3d_vk_descriptor_set_index_from_vk_descriptor_type(src->vk_descriptor_type);
    location = &locations[set][infos[set].count++];

    location->src = *src;

    if (location->src.magic & VKD3D_DESCRIPTOR_MAGIC_HAS_VIEW)
        vkd3d_view_incref(location->src.u.view_info.view);

    vkd3d_mutex_unlock(mutex);

    infos[set].uav_counter |= (location->src.magic == VKD3D_DESCRIPTOR_MAGIC_UAV)
            && !!location->src.u.view_info.view->vk_counter_view;
    location->dst = dst;

    if (infos[set].count == ARRAY_SIZE(locations[0]))
    {
        d3d12_desc_copy_vk_heap_range(locations[set], &infos[set], descriptor_heap, set, device);
        infos[set].count = 0;
        infos[set].uav_counter = false;
    }
}

/* Some games, e.g. Control, copy a large number of descriptors per frame, so the
 * speed of this function is critical. */
static void d3d12_device_vk_heaps_copy_descriptors(struct d3d12_device *device,
        UINT dst_descriptor_range_count, const D3D12_CPU_DESCRIPTOR_HANDLE *dst_descriptor_range_offsets,
        const UINT *dst_descriptor_range_sizes,
        UINT src_descriptor_range_count, const D3D12_CPU_DESCRIPTOR_HANDLE *src_descriptor_range_offsets,
        const UINT *src_descriptor_range_sizes)
{
    struct d3d12_desc_copy_location locations[VKD3D_SET_INDEX_COUNT][VKD3D_DESCRIPTOR_WRITE_BUFFER_SIZE];
    unsigned int dst_range_idx, dst_idx, src_range_idx, src_idx;
    /* The locations array is relatively large, and often mostly empty. Keeping these
     * values together in a separate array will likely result in fewer cache misses. */
    struct d3d12_desc_copy_info infos[VKD3D_SET_INDEX_COUNT];
    struct d3d12_descriptor_heap *descriptor_heap = NULL;
    const struct d3d12_desc *src, *heap_base, *heap_end;
    unsigned int dst_range_size, src_range_size;
    struct d3d12_desc *dst;

    descriptor_heap = vkd3d_gpu_descriptor_allocator_heap_from_descriptor(&device->gpu_descriptor_allocator,
            d3d12_desc_from_cpu_handle(dst_descriptor_range_offsets[0]));
    heap_base = (const struct d3d12_desc *)descriptor_heap->descriptors;
    heap_end = heap_base + descriptor_heap->desc.NumDescriptors;

    memset(infos, 0, sizeof(infos));
    dst_range_idx = dst_idx = 0;
    src_range_idx = src_idx = 0;
    while (dst_range_idx < dst_descriptor_range_count && src_range_idx < src_descriptor_range_count)
    {
        dst_range_size = dst_descriptor_range_sizes ? dst_descriptor_range_sizes[dst_range_idx] : 1;
        src_range_size = src_descriptor_range_sizes ? src_descriptor_range_sizes[src_range_idx] : 1;

        dst = d3d12_desc_from_cpu_handle(dst_descriptor_range_offsets[dst_range_idx]);
        src = d3d12_desc_from_cpu_handle(src_descriptor_range_offsets[src_range_idx]);

        if (dst < heap_base || dst >= heap_end)
        {
            flush_desc_writes(locations, infos, descriptor_heap, device);
            descriptor_heap = vkd3d_gpu_descriptor_allocator_heap_from_descriptor(&device->gpu_descriptor_allocator,
                    dst);
            heap_base = (const struct d3d12_desc *)descriptor_heap->descriptors;
            heap_end = heap_base + descriptor_heap->desc.NumDescriptors;
        }

        for (; dst_idx < dst_range_size && src_idx < src_range_size; src_idx++, dst_idx++)
        {
            /* We don't need to lock either descriptor for the identity check. The descriptor
             * mutex is only intended to prevent use-after-free of the vkd3d_view caused by a
             * race condition in the calling app. It is unnecessary to protect this test as it's
             * the app's race condition, not ours. */
            if (dst[dst_idx].magic == src[src_idx].magic && (dst[dst_idx].magic & VKD3D_DESCRIPTOR_MAGIC_HAS_VIEW)
                    && dst[dst_idx].u.view_info.written_serial_id == src[src_idx].u.view_info.view->serial_id)
                continue;
            d3d12_desc_buffered_copy_atomic(&dst[dst_idx], &src[src_idx], locations, infos, descriptor_heap, device);
        }

        if (dst_idx >= dst_range_size)
        {
            ++dst_range_idx;
            dst_idx = 0;
        }
        if (src_idx >= src_range_size)
        {
            ++src_range_idx;
            src_idx = 0;
        }
    }

    flush_desc_writes(locations, infos, descriptor_heap, device);
}

#define VKD3D_DESCRIPTOR_OPTIMISED_COPY_MIN_COUNT 8

static void STDMETHODCALLTYPE d3d12_device_CopyDescriptors(ID3D12Device *iface,
        UINT dst_descriptor_range_count, const D3D12_CPU_DESCRIPTOR_HANDLE *dst_descriptor_range_offsets,
        const UINT *dst_descriptor_range_sizes,
        UINT src_descriptor_range_count, const D3D12_CPU_DESCRIPTOR_HANDLE *src_descriptor_range_offsets,
        const UINT *src_descriptor_range_sizes,
        D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    unsigned int dst_range_idx, dst_idx, src_range_idx, src_idx;
    unsigned int dst_range_size, src_range_size;
    const struct d3d12_desc *src;
    struct d3d12_desc *dst;

    TRACE("iface %p, dst_descriptor_range_count %u, dst_descriptor_range_offsets %p, "
            "dst_descriptor_range_sizes %p, src_descriptor_range_count %u, "
            "src_descriptor_range_offsets %p, src_descriptor_range_sizes %p, "
            "descriptor_heap_type %#x.\n",
            iface, dst_descriptor_range_count, dst_descriptor_range_offsets,
            dst_descriptor_range_sizes, src_descriptor_range_count, src_descriptor_range_offsets,
            src_descriptor_range_sizes, descriptor_heap_type);

    if (descriptor_heap_type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            && descriptor_heap_type != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        FIXME("Unhandled descriptor heap type %#x.\n", descriptor_heap_type);
        return;
    }

    if (!dst_descriptor_range_count)
        return;

    if (device->use_vk_heaps && (dst_descriptor_range_count > 1 || (dst_descriptor_range_sizes
            && dst_descriptor_range_sizes[0] >= VKD3D_DESCRIPTOR_OPTIMISED_COPY_MIN_COUNT)))
    {
        d3d12_device_vk_heaps_copy_descriptors(device, dst_descriptor_range_count, dst_descriptor_range_offsets,
                dst_descriptor_range_sizes, src_descriptor_range_count, src_descriptor_range_offsets,
                src_descriptor_range_sizes);
        return;
    }

    dst_range_idx = dst_idx = 0;
    src_range_idx = src_idx = 0;
    while (dst_range_idx < dst_descriptor_range_count && src_range_idx < src_descriptor_range_count)
    {
        dst_range_size = dst_descriptor_range_sizes ? dst_descriptor_range_sizes[dst_range_idx] : 1;
        src_range_size = src_descriptor_range_sizes ? src_descriptor_range_sizes[src_range_idx] : 1;

        dst = d3d12_desc_from_cpu_handle(dst_descriptor_range_offsets[dst_range_idx]);
        src = d3d12_desc_from_cpu_handle(src_descriptor_range_offsets[src_range_idx]);

        while (dst_idx < dst_range_size && src_idx < src_range_size)
            d3d12_desc_copy(&dst[dst_idx++], &src[src_idx++], device);

        if (dst_idx >= dst_range_size)
        {
            ++dst_range_idx;
            dst_idx = 0;
        }
        if (src_idx >= src_range_size)
        {
            ++src_range_idx;
            src_idx = 0;
        }
    }
}

static void STDMETHODCALLTYPE d3d12_device_CopyDescriptorsSimple(ID3D12Device *iface,
        UINT descriptor_count, const D3D12_CPU_DESCRIPTOR_HANDLE dst_descriptor_range_offset,
        const D3D12_CPU_DESCRIPTOR_HANDLE src_descriptor_range_offset,
        D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type)
{
    TRACE("iface %p, descriptor_count %u, dst_descriptor_range_offset %#lx, "
            "src_descriptor_range_offset %#lx, descriptor_heap_type %#x.\n",
            iface, descriptor_count, dst_descriptor_range_offset.ptr, src_descriptor_range_offset.ptr,
            descriptor_heap_type);

    if (descriptor_count >= VKD3D_DESCRIPTOR_OPTIMISED_COPY_MIN_COUNT)
    {
        struct d3d12_device *device = impl_from_ID3D12Device(iface);
        if (device->use_vk_heaps)
        {
            d3d12_device_vk_heaps_copy_descriptors(device, 1, &dst_descriptor_range_offset,
                    &descriptor_count, 1, &src_descriptor_range_offset, &descriptor_count);
            return;
        }
    }

    d3d12_device_CopyDescriptors(iface, 1, &dst_descriptor_range_offset, &descriptor_count,
            1, &src_descriptor_range_offset, &descriptor_count, descriptor_heap_type);
}

static D3D12_RESOURCE_ALLOCATION_INFO * STDMETHODCALLTYPE d3d12_device_GetResourceAllocationInfo(
        ID3D12Device *iface, D3D12_RESOURCE_ALLOCATION_INFO *info, UINT visible_mask,
        UINT count, const D3D12_RESOURCE_DESC *resource_descs)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    const D3D12_RESOURCE_DESC *desc;
    uint64_t requested_alignment;

    TRACE("iface %p, info %p, visible_mask 0x%08x, count %u, resource_descs %p.\n",
            iface, info, visible_mask, count, resource_descs);

    debug_ignored_node_mask(visible_mask);

    info->SizeInBytes = 0;
    info->Alignment = 0;

    if (count != 1)
    {
        FIXME("Multiple resource descriptions not supported.\n");
        return info;
    }

    desc = &resource_descs[0];

    if (FAILED(d3d12_resource_validate_desc(desc, device)))
    {
        WARN("Invalid resource desc.\n");
        goto invalid;
    }

    if (desc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        info->SizeInBytes = align(desc->Width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        info->Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
        if (FAILED(vkd3d_get_image_allocation_info(device, desc, info)))
        {
            WARN("Failed to get allocation info for texture.\n");
            goto invalid;
        }

        requested_alignment = desc->Alignment
                ? desc->Alignment : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        info->Alignment = max(info->Alignment, requested_alignment);

        info->SizeInBytes = align(info->SizeInBytes, info->Alignment);

        /* Pad by the maximum heap offset increase which may be needed to align to a higher
         * Vulkan requirement an offset supplied by the calling application. This allows
         * us to return the standard D3D12 alignment and adjust resource placement later. */
        if (info->Alignment > requested_alignment)
        {
            info->SizeInBytes += info->Alignment - requested_alignment;
            info->Alignment = requested_alignment;
        }
    }

    TRACE("Size %#"PRIx64", alignment %#"PRIx64".\n", info->SizeInBytes, info->Alignment);

    return info;

invalid:
    info->SizeInBytes = ~(uint64_t)0;

    /* FIXME: Should we support D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT for small MSSA resources? */
    if (desc->SampleDesc.Count != 1)
        info->Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    else
        info->Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    TRACE("Alignment %#"PRIx64".\n", info->Alignment);

    return info;
}

static D3D12_HEAP_PROPERTIES * STDMETHODCALLTYPE d3d12_device_GetCustomHeapProperties(ID3D12Device *iface,
        D3D12_HEAP_PROPERTIES *heap_properties, UINT node_mask, D3D12_HEAP_TYPE heap_type)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    bool coherent;

    TRACE("iface %p, heap_properties %p, node_mask 0x%08x, heap_type %#x.\n",
            iface, heap_properties, node_mask, heap_type);

    debug_ignored_node_mask(node_mask);

    heap_properties->Type = D3D12_HEAP_TYPE_CUSTOM;

    switch (heap_type)
    {
        case D3D12_HEAP_TYPE_DEFAULT:
            heap_properties->CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
            heap_properties->MemoryPoolPreference = d3d12_device_is_uma(device, NULL)
                    ?  D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1;
            break;

        case D3D12_HEAP_TYPE_UPLOAD:
            heap_properties->CPUPageProperty = d3d12_device_is_uma(device, &coherent) && coherent
                    ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
            heap_properties->MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
            break;

        case D3D12_HEAP_TYPE_READBACK:
            heap_properties->CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
            heap_properties->MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
            break;

        default:
            FIXME("Unhandled heap type %#x.\n", heap_type);
            break;
    };

    heap_properties->CreationNodeMask = 1;
    heap_properties->VisibleNodeMask = 1;

    return heap_properties;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateCommittedResource(ID3D12Device *iface,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value, REFIID iid, void **resource)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_resource *object;
    HRESULT hr;

    TRACE("iface %p, heap_properties %p, heap_flags %#x,  desc %p, initial_state %#x, "
            "optimized_clear_value %p, iid %s, resource %p.\n",
            iface, heap_properties, heap_flags, desc, initial_state,
            optimized_clear_value, debugstr_guid(iid), resource);

    if (FAILED(hr = d3d12_committed_resource_create(device, heap_properties, heap_flags,
            desc, initial_state, optimized_clear_value, &object)))
    {
        *resource = NULL;
        return hr;
    }

    return return_interface(&object->ID3D12Resource_iface, &IID_ID3D12Resource, iid, resource);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateHeap(ID3D12Device *iface,
        const D3D12_HEAP_DESC *desc, REFIID iid, void **heap)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_heap *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, iid %s, heap %p.\n",
            iface, desc, debugstr_guid(iid), heap);

    if (FAILED(hr = d3d12_heap_create(device, desc, NULL, &object)))
    {
        *heap = NULL;
        return hr;
    }

    return return_interface(&object->ID3D12Heap_iface, &IID_ID3D12Heap, iid, heap);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreatePlacedResource(ID3D12Device *iface,
        ID3D12Heap *heap, UINT64 heap_offset,
        const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value, REFIID iid, void **resource)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_heap *heap_object;
    struct d3d12_resource *object;
    HRESULT hr;

    TRACE("iface %p, heap %p, heap_offset %#"PRIx64", desc %p, initial_state %#x, "
            "optimized_clear_value %p, iid %s, resource %p.\n",
            iface, heap, heap_offset, desc, initial_state,
            optimized_clear_value, debugstr_guid(iid), resource);

    heap_object = unsafe_impl_from_ID3D12Heap(heap);

    if (FAILED(hr = d3d12_placed_resource_create(device, heap_object, heap_offset,
            desc, initial_state, optimized_clear_value, &object)))
        return hr;

    return return_interface(&object->ID3D12Resource_iface, &IID_ID3D12Resource, iid, resource);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateReservedResource(ID3D12Device *iface,
        const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value, REFIID iid, void **resource)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_resource *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, initial_state %#x, optimized_clear_value %p, iid %s, resource %p.\n",
            iface, desc, initial_state, optimized_clear_value, debugstr_guid(iid), resource);

    if (FAILED(hr = d3d12_reserved_resource_create(device,
            desc, initial_state, optimized_clear_value, &object)))
        return hr;

    return return_interface(&object->ID3D12Resource_iface, &IID_ID3D12Resource, iid, resource);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateSharedHandle(ID3D12Device *iface,
        ID3D12DeviceChild *object, const SECURITY_ATTRIBUTES *attributes, DWORD access,
        const WCHAR *name, HANDLE *handle)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    FIXME("iface %p, object %p, attributes %p, access %#x, name %s, handle %p stub!\n",
            iface, object, attributes, access, debugstr_w(name, device->wchar_size), handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_OpenSharedHandle(ID3D12Device *iface,
        HANDLE handle, REFIID riid, void **object)
{
    FIXME("iface %p, handle %p, riid %s, object %p stub!\n",
            iface, handle, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_OpenSharedHandleByName(ID3D12Device *iface,
        const WCHAR *name, DWORD access, HANDLE *handle)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    FIXME("iface %p, name %s, access %#x, handle %p stub!\n",
            iface, debugstr_w(name, device->wchar_size), access, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_MakeResident(ID3D12Device *iface,
        UINT object_count, ID3D12Pageable * const *objects)
{
    FIXME_ONCE("iface %p, object_count %u, objects %p stub!\n",
            iface, object_count, objects);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_Evict(ID3D12Device *iface,
        UINT object_count, ID3D12Pageable * const *objects)
{
    FIXME_ONCE("iface %p, object_count %u, objects %p stub!\n",
            iface, object_count, objects);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateFence(ID3D12Device *iface,
        UINT64 initial_value, D3D12_FENCE_FLAGS flags, REFIID riid, void **fence)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_fence *object;
    HRESULT hr;

    TRACE("iface %p, initial_value %#"PRIx64", flags %#x, riid %s, fence %p.\n",
            iface, initial_value, flags, debugstr_guid(riid), fence);

    if (FAILED(hr = d3d12_fence_create(device, initial_value, flags, &object)))
        return hr;

    return return_interface(&object->ID3D12Fence_iface, &IID_ID3D12Fence, riid, fence);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_GetDeviceRemovedReason(ID3D12Device *iface)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    TRACE("iface %p.\n", iface);

    return device->removed_reason;
}

static void STDMETHODCALLTYPE d3d12_device_GetCopyableFootprints(ID3D12Device *iface,
        const D3D12_RESOURCE_DESC *desc, UINT first_sub_resource, UINT sub_resource_count,
        UINT64 base_offset, D3D12_PLACED_SUBRESOURCE_FOOTPRINT *layouts,
        UINT *row_counts, UINT64 *row_sizes, UINT64 *total_bytes)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    unsigned int i, sub_resource_idx, miplevel_idx, row_count, row_size, row_pitch;
    unsigned int width, height, depth, plane_count, sub_resources_per_plane;
    const struct vkd3d_format *format;
    uint64_t offset, size, total;

    TRACE("iface %p, desc %p, first_sub_resource %u, sub_resource_count %u, base_offset %#"PRIx64", "
            "layouts %p, row_counts %p, row_sizes %p, total_bytes %p.\n",
            iface, desc, first_sub_resource, sub_resource_count, base_offset,
            layouts, row_counts, row_sizes, total_bytes);

    if (layouts)
        memset(layouts, 0xff, sizeof(*layouts) * sub_resource_count);
    if (row_counts)
        memset(row_counts, 0xff, sizeof(*row_counts) * sub_resource_count);
    if (row_sizes)
        memset(row_sizes, 0xff, sizeof(*row_sizes) * sub_resource_count);
    if (total_bytes)
        *total_bytes = ~(uint64_t)0;

    if (!(format = vkd3d_format_from_d3d12_resource_desc(device, desc, 0)))
    {
        WARN("Invalid format %#x.\n", desc->Format);
        return;
    }

    if (FAILED(d3d12_resource_validate_desc(desc, device)))
    {
        WARN("Invalid resource desc.\n");
        return;
    }

    plane_count = ((format->vk_aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT)
            && (format->vk_aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT)) ? 2 : 1;
    sub_resources_per_plane = d3d12_resource_desc_get_sub_resource_count(desc);

    if (!vkd3d_bound_range(first_sub_resource, sub_resource_count, sub_resources_per_plane * plane_count))
    {
        WARN("Invalid sub-resource range %u-%u for resource.\n", first_sub_resource, sub_resource_count);
        return;
    }

    offset = 0;
    total = 0;
    for (i = 0; i < sub_resource_count; ++i)
    {
        sub_resource_idx = (first_sub_resource + i) % sub_resources_per_plane;
        miplevel_idx = sub_resource_idx % desc->MipLevels;
        width = align(d3d12_resource_desc_get_width(desc, miplevel_idx), format->block_width);
        height = align(d3d12_resource_desc_get_height(desc, miplevel_idx), format->block_height);
        depth = d3d12_resource_desc_get_depth(desc, miplevel_idx);
        row_count = height / format->block_height;
        row_size = (width / format->block_width) * format->byte_count * format->block_byte_count;
        row_pitch = align(row_size, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

        if (layouts)
        {
            layouts[i].Offset = base_offset + offset;
            layouts[i].Footprint.Format = desc->Format;
            layouts[i].Footprint.Width = width;
            layouts[i].Footprint.Height = height;
            layouts[i].Footprint.Depth = depth;
            layouts[i].Footprint.RowPitch = row_pitch;
        }
        if (row_counts)
            row_counts[i] = row_count;
        if (row_sizes)
            row_sizes[i] = row_size;

        size = max(0, row_count - 1) * row_pitch + row_size;
        size = max(0, depth - 1) * align(size, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) + size;

        total = offset + size;
        offset = align(total, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    }
    if (total_bytes)
        *total_bytes = total;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateQueryHeap(ID3D12Device *iface,
        const D3D12_QUERY_HEAP_DESC *desc, REFIID iid, void **heap)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_query_heap *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, iid %s, heap %p.\n",
            iface, desc, debugstr_guid(iid), heap);

    if (FAILED(hr = d3d12_query_heap_create(device, desc, &object)))
        return hr;

    return return_interface(&object->ID3D12QueryHeap_iface, &IID_ID3D12QueryHeap, iid, heap);
}

static HRESULT STDMETHODCALLTYPE d3d12_device_SetStablePowerState(ID3D12Device *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_device_CreateCommandSignature(ID3D12Device *iface,
        const D3D12_COMMAND_SIGNATURE_DESC *desc, ID3D12RootSignature *root_signature,
        REFIID iid, void **command_signature)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);
    struct d3d12_command_signature *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, root_signature %p, iid %s, command_signature %p.\n",
            iface, desc, root_signature, debugstr_guid(iid), command_signature);

    if (FAILED(hr = d3d12_command_signature_create(device, desc, &object)))
        return hr;

    return return_interface(&object->ID3D12CommandSignature_iface,
            &IID_ID3D12CommandSignature, iid, command_signature);
}

static void STDMETHODCALLTYPE d3d12_device_GetResourceTiling(ID3D12Device *iface,
        ID3D12Resource *resource, UINT *total_tile_count,
        D3D12_PACKED_MIP_INFO *packed_mip_info, D3D12_TILE_SHAPE *standard_tile_shape,
        UINT *sub_resource_tiling_count, UINT first_sub_resource_tiling,
        D3D12_SUBRESOURCE_TILING *sub_resource_tilings)
{
    FIXME("iface %p, resource %p, total_tile_count %p, packed_mip_info %p, "
            "standard_title_shape %p, sub_resource_tiling_count %p, "
            "first_sub_resource_tiling %u, sub_resource_tilings %p stub!\n",
            iface, resource, total_tile_count, packed_mip_info, standard_tile_shape,
            sub_resource_tiling_count, first_sub_resource_tiling,
            sub_resource_tilings);
}

static LUID * STDMETHODCALLTYPE d3d12_device_GetAdapterLuid(ID3D12Device *iface, LUID *luid)
{
    struct d3d12_device *device = impl_from_ID3D12Device(iface);

    TRACE("iface %p, luid %p.\n", iface, luid);

    *luid = device->adapter_luid;

    return luid;
}

static const struct ID3D12DeviceVtbl d3d12_device_vtbl =
{
    /* IUnknown methods */
    d3d12_device_QueryInterface,
    d3d12_device_AddRef,
    d3d12_device_Release,
    /* ID3D12Object methods */
    d3d12_device_GetPrivateData,
    d3d12_device_SetPrivateData,
    d3d12_device_SetPrivateDataInterface,
    d3d12_device_SetName,
    /* ID3D12Device methods */
    d3d12_device_GetNodeCount,
    d3d12_device_CreateCommandQueue,
    d3d12_device_CreateCommandAllocator,
    d3d12_device_CreateGraphicsPipelineState,
    d3d12_device_CreateComputePipelineState,
    d3d12_device_CreateCommandList,
    d3d12_device_CheckFeatureSupport,
    d3d12_device_CreateDescriptorHeap,
    d3d12_device_GetDescriptorHandleIncrementSize,
    d3d12_device_CreateRootSignature,
    d3d12_device_CreateConstantBufferView,
    d3d12_device_CreateShaderResourceView,
    d3d12_device_CreateUnorderedAccessView,
    d3d12_device_CreateRenderTargetView,
    d3d12_device_CreateDepthStencilView,
    d3d12_device_CreateSampler,
    d3d12_device_CopyDescriptors,
    d3d12_device_CopyDescriptorsSimple,
    d3d12_device_GetResourceAllocationInfo,
    d3d12_device_GetCustomHeapProperties,
    d3d12_device_CreateCommittedResource,
    d3d12_device_CreateHeap,
    d3d12_device_CreatePlacedResource,
    d3d12_device_CreateReservedResource,
    d3d12_device_CreateSharedHandle,
    d3d12_device_OpenSharedHandle,
    d3d12_device_OpenSharedHandleByName,
    d3d12_device_MakeResident,
    d3d12_device_Evict,
    d3d12_device_CreateFence,
    d3d12_device_GetDeviceRemovedReason,
    d3d12_device_GetCopyableFootprints,
    d3d12_device_CreateQueryHeap,
    d3d12_device_SetStablePowerState,
    d3d12_device_CreateCommandSignature,
    d3d12_device_GetResourceTiling,
    d3d12_device_GetAdapterLuid,
};

struct d3d12_device *unsafe_impl_from_ID3D12Device(ID3D12Device *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d12_device_vtbl);
    return impl_from_ID3D12Device(iface);
}

static HRESULT d3d12_device_init(struct d3d12_device *device,
        struct vkd3d_instance *instance, const struct vkd3d_device_create_info *create_info)
{
    const struct vkd3d_vk_device_procs *vk_procs;
    HRESULT hr;
    size_t i;

    device->ID3D12Device_iface.lpVtbl = &d3d12_device_vtbl;
    device->refcount = 1;

    vkd3d_instance_incref(device->vkd3d_instance = instance);
    device->vk_info = instance->vk_info;
    device->signal_event = instance->signal_event;
    device->wchar_size = instance->wchar_size;

    device->adapter_luid = create_info->adapter_luid;
    device->removed_reason = S_OK;

    device->vk_device = VK_NULL_HANDLE;

    if (FAILED(hr = vkd3d_create_vk_device(device, create_info)))
        goto out_free_instance;

    if (FAILED(hr = d3d12_device_init_pipeline_cache(device)))
        goto out_free_vk_resources;

    if (FAILED(hr = vkd3d_private_store_init(&device->private_store)))
        goto out_free_pipeline_cache;

    if (FAILED(hr = vkd3d_init_format_info(device)))
        goto out_free_private_store;

    if (FAILED(hr = vkd3d_init_null_resources(&device->null_resources, device)))
        goto out_cleanup_format_info;

    if (FAILED(hr = vkd3d_uav_clear_state_init(&device->uav_clear_state, device)))
        goto out_destroy_null_resources;

    if (FAILED(hr = vkd3d_vk_descriptor_heap_layouts_init(device)))
        goto out_cleanup_uav_clear_state;

    vkd3d_render_pass_cache_init(&device->render_pass_cache);
    vkd3d_gpu_descriptor_allocator_init(&device->gpu_descriptor_allocator);
    vkd3d_gpu_va_allocator_init(&device->gpu_va_allocator);
    vkd3d_time_domains_init(device);

    device->blocked_queue_count = 0;

    for (i = 0; i < ARRAY_SIZE(device->desc_mutex); ++i)
        vkd3d_mutex_init(&device->desc_mutex[i]);

    vkd3d_init_descriptor_pool_sizes(device->vk_pool_sizes, &device->vk_info.descriptor_limits);

    if ((device->parent = create_info->parent))
        IUnknown_AddRef(device->parent);

    return S_OK;

out_cleanup_uav_clear_state:
    vkd3d_uav_clear_state_cleanup(&device->uav_clear_state, device);
out_destroy_null_resources:
    vkd3d_destroy_null_resources(&device->null_resources, device);
out_cleanup_format_info:
    vkd3d_cleanup_format_info(device);
out_free_private_store:
    vkd3d_private_store_destroy(&device->private_store);
out_free_pipeline_cache:
    d3d12_device_destroy_pipeline_cache(device);
out_free_vk_resources:
    vk_procs = &device->vk_procs;
    VK_CALL(vkDestroyDevice(device->vk_device, NULL));
out_free_instance:
    vkd3d_instance_decref(device->vkd3d_instance);
    return hr;
}

HRESULT d3d12_device_create(struct vkd3d_instance *instance,
        const struct vkd3d_device_create_info *create_info, struct d3d12_device **device)
{
    struct d3d12_device *object;
    HRESULT hr;

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_device_init(object, instance, create_info)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created device %p.\n", object);

    *device = object;

    return S_OK;
}

void d3d12_device_mark_as_removed(struct d3d12_device *device, HRESULT reason,
        const char *message, ...)
{
    va_list args;

    va_start(args, message);
    WARN("Device %p is lost (reason %#x, \"%s\").\n",
            device, reason, vkd3d_dbg_vsprintf(message, args));
    va_end(args);

    device->removed_reason = reason;
}


#ifdef _WIN32
struct thread_data
{
    PFN_vkd3d_thread main_pfn;
    void *data;
};

static DWORD WINAPI call_thread_main(void *data)
{
    struct thread_data *thread_data = data;
    thread_data->main_pfn(thread_data->data);
    vkd3d_free(thread_data);
    return 0;
}
#endif

HRESULT vkd3d_create_thread(struct vkd3d_instance *instance,
        PFN_vkd3d_thread thread_main, void *data, union vkd3d_thread_handle *thread)
{
    HRESULT hr = S_OK;
    int rc;

    if (instance->create_thread)
    {
        if (!(thread->handle = instance->create_thread(thread_main, data)))
        {
            ERR("Failed to create thread.\n");
            hr = E_FAIL;
        }
    }
    else
    {
#ifdef _WIN32
        struct thread_data *thread_data;

        if (!(thread_data = vkd3d_malloc(sizeof(*thread_data))))
            return E_OUTOFMEMORY;

        thread_data->main_pfn = thread_main;
        thread_data->data = data;
        if (!(thread->handle = CreateThread(NULL, 0, call_thread_main, thread_data, 0, NULL)))
        {
            ERR("Failed to create thread, error %d.\n", GetLastError());
            vkd3d_free(thread_data);
            hr = E_FAIL;
        }
#else
        if ((rc = pthread_create(&thread->pthread, NULL, thread_main, data)))
        {
            ERR("Failed to create thread, error %d.\n", rc);
            hr = hresult_from_errno(rc);
        }
#endif
    }

    return hr;
}

HRESULT vkd3d_join_thread(struct vkd3d_instance *instance, union vkd3d_thread_handle *thread)
{
    HRESULT hr = S_OK;
    int rc;

    if (instance->join_thread)
    {
        if (FAILED(hr = instance->join_thread(thread->handle)))
            ERR("Failed to join thread, hr %#x.\n", hr);
    }
    else
    {
#ifdef _WIN32
        if ((rc = WaitForSingleObject(thread->handle, INFINITE)) != WAIT_OBJECT_0)
        {
            ERR("Failed to wait for thread, ret %#x.\n", rc);
            hr = E_FAIL;
        }
        CloseHandle(thread->handle);
#else
        if ((rc = pthread_join(thread->pthread, NULL)))
        {
            ERR("Failed to join thread, error %d.\n", rc);
            hr = hresult_from_errno(rc);
        }
#endif
    }

    return hr;
}

IUnknown *vkd3d_get_device_parent(ID3D12Device *device)
{
    struct d3d12_device *d3d12_device = impl_from_ID3D12Device(device);

    return d3d12_device->parent;
}

VkDevice vkd3d_get_vk_device(ID3D12Device *device)
{
    struct d3d12_device *d3d12_device = impl_from_ID3D12Device(device);

    return d3d12_device->vk_device;
}

VkPhysicalDevice vkd3d_get_vk_physical_device(ID3D12Device *device)
{
    struct d3d12_device *d3d12_device = impl_from_ID3D12Device(device);

    return d3d12_device->vk_physical_device;
}

struct vkd3d_instance *vkd3d_instance_from_device(ID3D12Device *device)
{
    struct d3d12_device *d3d12_device = impl_from_ID3D12Device(device);

    return d3d12_device->vkd3d_instance;
}
