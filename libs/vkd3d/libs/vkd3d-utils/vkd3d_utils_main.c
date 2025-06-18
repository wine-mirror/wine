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

#include "vkd3d_utils_private.h"
#undef D3D12CreateDevice

static const char *debug_d3d_blob_part(D3D_BLOB_PART part)
{
    switch (part)
    {
#define TO_STR(x) case x: return #x
        TO_STR(D3D_BLOB_INPUT_SIGNATURE_BLOB);
        TO_STR(D3D_BLOB_OUTPUT_SIGNATURE_BLOB);
        TO_STR(D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB);
        TO_STR(D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB);
        TO_STR(D3D_BLOB_ALL_SIGNATURE_BLOB);
        TO_STR(D3D_BLOB_DEBUG_INFO);
        TO_STR(D3D_BLOB_LEGACY_SHADER);
        TO_STR(D3D_BLOB_XNA_PREPASS_SHADER);
        TO_STR(D3D_BLOB_XNA_SHADER);
        TO_STR(D3D_BLOB_PDB);
        TO_STR(D3D_BLOB_PRIVATE_DATA);
        TO_STR(D3D_BLOB_ROOT_SIGNATURE);
        TO_STR(D3D_BLOB_DEBUG_NAME);

        TO_STR(D3D_BLOB_TEST_ALTERNATE_SHADER);
        TO_STR(D3D_BLOB_TEST_COMPILE_DETAILS);
        TO_STR(D3D_BLOB_TEST_COMPILE_PERF);
        TO_STR(D3D_BLOB_TEST_COMPILE_REPORT);
#undef TO_STR
        default:
            return vkd3d_dbg_sprintf("<D3D_BLOB_PART %#x>", part);
    }
}

HRESULT WINAPI D3D12GetDebugInterface(REFIID iid, void **debug)
{
    FIXME("iid %s, debug %p stub!\n", debugstr_guid(iid), debug);

    return E_NOTIMPL;
}

HRESULT WINAPI D3D12CreateDeviceVKD3D(IUnknown *adapter, D3D_FEATURE_LEVEL minimum_feature_level,
        REFIID iid, void **device, enum vkd3d_api_version api_version)
{
    struct vkd3d_optional_instance_extensions_info optional_instance_extensions_info;
    struct vkd3d_optional_device_extensions_info optional_device_extensions_info;
    struct vkd3d_instance_create_info instance_create_info;
    struct vkd3d_device_create_info device_create_info;

    static const char * const optional_instance_extensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_android_surface",
        "VK_KHR_wayland_surface",
        "VK_KHR_win32_surface",
        "VK_KHR_xcb_surface",
        "VK_KHR_xlib_surface",
        "VK_EXT_metal_surface",
        "VK_MVK_macos_surface",
        "VK_MVK_ios_surface",
    };
    static const char * const optional_device_extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    struct vkd3d_application_info application_info =
    {
        .type = VKD3D_STRUCTURE_TYPE_APPLICATION_INFO,
        .api_version = api_version,
    };

    TRACE("adapter %p, minimum_feature_level %#x, iid %s, device %p, api_version %#x.\n",
            adapter, minimum_feature_level, debugstr_guid(iid), device, api_version);

    if (adapter)
        FIXME("Ignoring adapter %p.\n", adapter);

    memset(&optional_instance_extensions_info, 0, sizeof(optional_instance_extensions_info));
    optional_instance_extensions_info.type = VKD3D_STRUCTURE_TYPE_OPTIONAL_INSTANCE_EXTENSIONS_INFO;
    optional_instance_extensions_info.next = &application_info;
    optional_instance_extensions_info.extensions = optional_instance_extensions;
    optional_instance_extensions_info.extension_count = ARRAY_SIZE(optional_instance_extensions);

    memset(&instance_create_info, 0, sizeof(instance_create_info));
    instance_create_info.type = VKD3D_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.next = &optional_instance_extensions_info;
    instance_create_info.pfn_signal_event = vkd3d_signal_event;
    instance_create_info.wchar_size = sizeof(WCHAR);

    memset(&optional_device_extensions_info, 0, sizeof(optional_device_extensions_info));
    optional_device_extensions_info.type = VKD3D_STRUCTURE_TYPE_OPTIONAL_DEVICE_EXTENSIONS_INFO;
    optional_device_extensions_info.extensions = optional_device_extensions;
    optional_device_extensions_info.extension_count = ARRAY_SIZE(optional_device_extensions);

    memset(&device_create_info, 0, sizeof(device_create_info));
    device_create_info.type = VKD3D_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.next = &optional_device_extensions_info;
    device_create_info.minimum_feature_level = minimum_feature_level;
    device_create_info.instance_create_info = &instance_create_info;

    return vkd3d_create_device(&device_create_info, iid, device);
}

VKD3D_UTILS_API HRESULT WINAPI D3D12CreateDevice(IUnknown *adapter,
        D3D_FEATURE_LEVEL minimum_feature_level, REFIID iid, void **device)
{
    return D3D12CreateDeviceVKD3D(adapter, minimum_feature_level, iid, device, VKD3D_API_VERSION_1_0);
}

HRESULT WINAPI D3D12CreateRootSignatureDeserializer(const void *data, SIZE_T data_size,
        REFIID iid, void **deserializer)
{
    TRACE("data %p, data_size %"PRIuPTR", iid %s, deserializer %p.\n",
            data, (uintptr_t)data_size, debugstr_guid(iid), deserializer);

    return vkd3d_create_root_signature_deserializer(data, data_size, iid, deserializer);
}

HRESULT WINAPI D3D12CreateVersionedRootSignatureDeserializer(const void *data, SIZE_T data_size,
        REFIID iid,void **deserializer)
{
    TRACE("data %p, data_size %"PRIuPTR", iid %s, deserializer %p.\n",
            data, (uintptr_t)data_size, debugstr_guid(iid), deserializer);

    return vkd3d_create_versioned_root_signature_deserializer(data, data_size, iid, deserializer);
}

HRESULT WINAPI D3D12SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC *desc,
        D3D_ROOT_SIGNATURE_VERSION version, ID3DBlob **blob, ID3DBlob **error_blob)
{
    TRACE("desc %p, version %#x, blob %p, error_blob %p.\n", desc, version, blob, error_blob);

    return vkd3d_serialize_root_signature(desc, version, blob, error_blob);
}

HRESULT WINAPI D3D12SerializeVersionedRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *desc,
        ID3DBlob **blob, ID3DBlob **error_blob)
{
    TRACE("desc %p, blob %p, error_blob %p.\n", desc, blob, error_blob);

    return vkd3d_serialize_versioned_root_signature(desc, blob, error_blob);
}

static int open_include(const char *filename, bool local, const char *parent_data, void *context,
        struct vkd3d_shader_code *code)
{
    ID3DInclude *iface = context;
    unsigned int size = 0;

    if (!iface)
        return VKD3D_ERROR;

    memset(code, 0, sizeof(*code));
    if (FAILED(ID3DInclude_Open(iface, local ? D3D_INCLUDE_LOCAL : D3D_INCLUDE_SYSTEM,
            filename, parent_data, &code->code, &size)))
        return VKD3D_ERROR;

    code->size = size;
    return VKD3D_OK;
}

static void close_include(const struct vkd3d_shader_code *code, void *context)
{
    ID3DInclude *iface = context;

    ID3DInclude_Close(iface, code->code);
}

static enum vkd3d_shader_target_type get_target_for_profile(const char *profile)
{
    size_t profile_len, i;

    static const char * const d3dbc_profiles[] =
    {
        "ps.1.",
        "ps.2.",
        "ps.3.",

        "ps_1_",
        "ps_2_",
        "ps_3_",

        "vs.1.",
        "vs.2.",
        "vs.3.",

        "vs_1_",
        "vs_2_",
        "vs_3_",

        "tx_1_",
    };

    static const char * const fx_profiles[] =
    {
        "fx_2_0",
        "fx_4_0",
        "fx_4_1",
        "fx_5_0",
    };

    profile_len = strlen(profile);
    for (i = 0; i < ARRAY_SIZE(d3dbc_profiles); ++i)
    {
        size_t len = strlen(d3dbc_profiles[i]);

        if (len <= profile_len && !memcmp(profile, d3dbc_profiles[i], len))
            return VKD3D_SHADER_TARGET_D3D_BYTECODE;
    }

    for (i = 0; i < ARRAY_SIZE(fx_profiles); ++i)
    {
        if (!strcmp(profile, fx_profiles[i]))
            return VKD3D_SHADER_TARGET_FX;
    }

    return VKD3D_SHADER_TARGET_DXBC_TPF;
}

HRESULT WINAPI D3DCompile2VKD3D(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *macros, ID3DInclude *include, const char *entry_point,
        const char *profile, UINT flags, UINT effect_flags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader_blob,
        ID3DBlob **messages_blob, unsigned int compiler_version)
{
    struct vkd3d_shader_preprocess_info preprocess_info;
    struct vkd3d_shader_hlsl_source_info hlsl_info;
    struct vkd3d_shader_compile_option options[7];
    struct vkd3d_shader_compile_info compile_info;
    struct vkd3d_shader_compile_option *option;
    struct vkd3d_shader_code byte_code;
    const D3D_SHADER_MACRO *macro;
    char *messages;
    HRESULT hr;
    int ret;

    TRACE("data %p, data_size %"PRIuPTR", filename %s, macros %p, include %p, entry_point %s, "
            "profile %s, flags %#x, effect_flags %#x, secondary_flags %#x, secondary_data %p, "
            "secondary_data_size %"PRIuPTR", shader_blob %p, messages_blob %p, compiler_version %u.\n",
            data, (uintptr_t)data_size, debugstr_a(filename), macros, include, debugstr_a(entry_point),
            debugstr_a(profile), flags, effect_flags, secondary_flags, secondary_data,
            (uintptr_t)secondary_data_size, shader_blob, messages_blob, compiler_version);

    if (flags & ~(D3DCOMPILE_DEBUG | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR))
        FIXME("Ignoring flags %#x.\n", flags);
    if (effect_flags & ~D3DCOMPILE_EFFECT_CHILD_EFFECT)
        FIXME("Ignoring effect flags %#x.\n", effect_flags);
    if (secondary_flags)
        FIXME("Ignoring secondary flags %#x.\n", secondary_flags);

    if (messages_blob)
        *messages_blob = NULL;

    option = &options[0];
    option->name = VKD3D_SHADER_COMPILE_OPTION_API_VERSION;
    option->value = VKD3D_SHADER_API_VERSION_1_16;

    compile_info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    compile_info.next = &preprocess_info;
    compile_info.source.code = data;
    compile_info.source.size = data_size;
    compile_info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    compile_info.target_type = get_target_for_profile(profile);
    compile_info.options = options;
    compile_info.option_count = 1;
    compile_info.log_level = VKD3D_SHADER_LOG_INFO;
    compile_info.source_name = filename;

    preprocess_info.type = VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO;
    preprocess_info.next = &hlsl_info;
    preprocess_info.macros = (const struct vkd3d_shader_macro *)macros;
    preprocess_info.macro_count = 0;
    if (macros)
    {
        for (macro = macros; macro->Name; ++macro)
            ++preprocess_info.macro_count;
    }
    preprocess_info.pfn_open_include = open_include;
    preprocess_info.pfn_close_include = close_include;
    preprocess_info.include_context = include;

    hlsl_info.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO;
    hlsl_info.next = NULL;
    hlsl_info.profile = profile;
    hlsl_info.entry_point = entry_point;
    hlsl_info.secondary_code.code = secondary_data;
    hlsl_info.secondary_code.size = secondary_data_size;

    if (!(flags & D3DCOMPILE_DEBUG))
    {
        option = &options[compile_info.option_count++];
        option->name = VKD3D_SHADER_COMPILE_OPTION_STRIP_DEBUG;
        option->value = true;
    }

    if (flags & (D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR))
    {
        option = &options[compile_info.option_count++];
        option->name = VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ORDER;
        option->value = 0;
        if (flags & D3DCOMPILE_PACK_MATRIX_ROW_MAJOR)
            option->value |= VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ROW_MAJOR;
        else if (flags & D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR)
            option->value |= VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_COLUMN_MAJOR;
    }

    if (flags & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY)
    {
        option = &options[compile_info.option_count++];
        option->name = VKD3D_SHADER_COMPILE_OPTION_BACKWARD_COMPATIBILITY;
        option->value = VKD3D_SHADER_COMPILE_OPTION_BACKCOMPAT_MAP_SEMANTIC_NAMES;
    }

    if (effect_flags & D3DCOMPILE_EFFECT_CHILD_EFFECT)
    {
        option = &options[compile_info.option_count++];
        option->name = VKD3D_SHADER_COMPILE_OPTION_CHILD_EFFECT;
        option->value = true;
    }

    if (compiler_version <= 39)
    {
        option = &options[compile_info.option_count++];
        option->name = VKD3D_SHADER_COMPILE_OPTION_INCLUDE_EMPTY_BUFFERS_IN_EFFECTS;
        option->value = true;
    }

    if (compiler_version < 42)
    {
        option = &options[compile_info.option_count++];
        option->name = VKD3D_SHADER_COMPILE_OPTION_WARN_IMPLICIT_TRUNCATION;
        option->value = false;
    }

    ret = vkd3d_shader_compile(&compile_info, &byte_code, &messages);

    if (messages && messages_blob)
    {
        if (FAILED(hr = vkd3d_blob_create(messages, strlen(messages), messages_blob)))
        {
            vkd3d_shader_free_messages(messages);
            vkd3d_shader_free_shader_code(&byte_code);
            return hr;
        }
        messages = NULL;
    }
    vkd3d_shader_free_messages(messages);

    if (!ret)
    {
        /* Unlike other effect profiles fx_4_x is using DXBC container. */
        if (!strcmp(profile, "fx_4_0") || !strcmp(profile, "fx_4_1"))
        {
            struct vkd3d_shader_dxbc_section_desc section = { .tag = TAG_FX10, .data = byte_code };
            struct vkd3d_shader_code dxbc;

            ret = vkd3d_shader_serialize_dxbc(1, &section, &dxbc, NULL);
            vkd3d_shader_free_shader_code(&byte_code);
            if (ret)
                return hresult_from_vkd3d_result(ret);

            byte_code = dxbc;
        }

        if (FAILED(hr = vkd3d_blob_create((void *)byte_code.code, byte_code.size, shader_blob)))
        {
            vkd3d_shader_free_shader_code(&byte_code);
            return hr;
        }
    }

    return hresult_from_vkd3d_result(ret);
}

HRESULT WINAPI D3DCompile2(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *macros, ID3DInclude *include, const char *entry_point,
        const char *profile, UINT flags, UINT effect_flags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader_blob,
        ID3DBlob **messages_blob)
{
    TRACE("data %p, data_size %"PRIuPTR", filename %s, macros %p, include %p, entry_point %s, "
            "profile %s, flags %#x, effect_flags %#x, secondary_flags %#x, secondary_data %p, "
            "secondary_data_size %"PRIuPTR", shader_blob %p, messages_blob %p.\n",
            data, (uintptr_t)data_size, debugstr_a(filename), macros, include, debugstr_a(entry_point),
            debugstr_a(profile), flags, effect_flags, secondary_flags, secondary_data,
            (uintptr_t)secondary_data_size, shader_blob, messages_blob);

    return D3DCompile2VKD3D(data, data_size, filename, macros, include,
            entry_point, profile, flags, effect_flags, secondary_flags,
            secondary_data, secondary_data_size, shader_blob, messages_blob, 47);
}

HRESULT WINAPI D3DCompile(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *macros, ID3DInclude *include, const char *entrypoint,
        const char *profile, UINT flags, UINT effect_flags, ID3DBlob **shader, ID3DBlob **error_messages)
{
    TRACE("data %p, data_size %"PRIuPTR", filename %s, macros %p, include %p, entrypoint %s, "
            "profile %s, flags %#x, effect_flags %#x, shader %p, error_messages %p.\n",
            data, (uintptr_t)data_size, debugstr_a(filename), macros, include, debugstr_a(entrypoint),
            debugstr_a(profile), flags, effect_flags, shader, error_messages);

    return D3DCompile2(data, data_size, filename, macros, include, entrypoint, profile, flags,
            effect_flags, 0, NULL, 0, shader, error_messages);
}

HRESULT WINAPI D3DPreprocess(const void *data, SIZE_T size, const char *filename,
        const D3D_SHADER_MACRO *macros, ID3DInclude *include,
        ID3DBlob **preprocessed_blob, ID3DBlob **messages_blob)
{
    struct vkd3d_shader_preprocess_info preprocess_info;
    struct vkd3d_shader_compile_info compile_info;
    struct vkd3d_shader_code preprocessed_code;
    const D3D_SHADER_MACRO *macro;
    char *messages;
    HRESULT hr;
    int ret;

    static const struct vkd3d_shader_compile_option options[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_16},
    };

    TRACE("data %p, size %"PRIuPTR", filename %s, macros %p, include %p, preprocessed_blob %p, messages_blob %p.\n",
            data, (uintptr_t)size, debugstr_a(filename), macros, include, preprocessed_blob, messages_blob);

    if (messages_blob)
        *messages_blob = NULL;

    compile_info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    compile_info.next = &preprocess_info;
    compile_info.source.code = data;
    compile_info.source.size = size;
    compile_info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    compile_info.target_type = VKD3D_SHADER_TARGET_NONE;
    compile_info.options = options;
    compile_info.option_count = ARRAY_SIZE(options);
    compile_info.log_level = VKD3D_SHADER_LOG_INFO;
    compile_info.source_name = filename;

    preprocess_info.type = VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO;
    preprocess_info.next = NULL;
    preprocess_info.macros = (const struct vkd3d_shader_macro *)macros;
    preprocess_info.macro_count = 0;
    if (macros)
    {
        for (macro = macros; macro->Name; ++macro)
            ++preprocess_info.macro_count;
    }
    preprocess_info.pfn_open_include = open_include;
    preprocess_info.pfn_close_include = close_include;
    preprocess_info.include_context = include;

    ret = vkd3d_shader_preprocess(&compile_info, &preprocessed_code, &messages);

    if (messages && messages_blob)
    {
        if (FAILED(hr = vkd3d_blob_create(messages, strlen(messages), messages_blob)))
        {
            vkd3d_shader_free_messages(messages);
            vkd3d_shader_free_shader_code(&preprocessed_code);
            return hr;
        }
        messages = NULL;
    }
    vkd3d_shader_free_messages(messages);

    if (!ret)
    {
        if (FAILED(hr = vkd3d_blob_create((void *)preprocessed_code.code, preprocessed_code.size, preprocessed_blob)))
        {
            vkd3d_shader_free_shader_code(&preprocessed_code);
            return hr;
        }
    }

    return hresult_from_vkd3d_result(ret);
}

/* Events */

#ifdef _WIN32

HANDLE vkd3d_create_event(void)
{
    return CreateEventA(NULL, FALSE, FALSE, NULL);
}

HRESULT vkd3d_signal_event(HANDLE event)
{
    SetEvent(event);
    return S_OK;
}

unsigned int vkd3d_wait_event(HANDLE event, unsigned int milliseconds)
{
    return WaitForSingleObject(event, milliseconds);
}

void vkd3d_destroy_event(HANDLE event)
{
    CloseHandle(event);
}

#else  /* _WIN32 */

#include <pthread.h>

struct vkd3d_event
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    BOOL is_signaled;
};

HANDLE vkd3d_create_event(void)
{
    struct vkd3d_event *event;
    int rc;

    TRACE(".\n");

    if (!(event = vkd3d_malloc(sizeof(*event))))
        return NULL;

    if ((rc = pthread_mutex_init(&event->mutex, NULL)))
    {
        ERR("Failed to initialize mutex, error %d.\n", rc);
        vkd3d_free(event);
        return NULL;
    }
    if ((rc = pthread_cond_init(&event->cond, NULL)))
    {
        ERR("Failed to initialize condition variable, error %d.\n", rc);
        pthread_mutex_destroy(&event->mutex);
        vkd3d_free(event);
        return NULL;
    }

    event->is_signaled = false;

    TRACE("Created event %p.\n", event);

    return event;
}

unsigned int vkd3d_wait_event(HANDLE event, unsigned int milliseconds)
{
    struct vkd3d_event *impl = event;
    int rc;

    TRACE("event %p, milliseconds %u.\n", event, milliseconds);

    if ((rc = pthread_mutex_lock(&impl->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return VKD3D_WAIT_FAILED;
    }

    if (impl->is_signaled || !milliseconds)
    {
        bool is_signaled = impl->is_signaled;
        impl->is_signaled = false;
        pthread_mutex_unlock(&impl->mutex);
        return is_signaled ? VKD3D_WAIT_OBJECT_0 : VKD3D_WAIT_TIMEOUT;
    }

    if (milliseconds == VKD3D_INFINITE)
    {
        do
        {
            if ((rc = pthread_cond_wait(&impl->cond, &impl->mutex)))
            {
                ERR("Failed to wait on condition variable, error %d.\n", rc);
                pthread_mutex_unlock(&impl->mutex);
                return VKD3D_WAIT_FAILED;
            }
        } while (!impl->is_signaled);

        impl->is_signaled = false;
        pthread_mutex_unlock(&impl->mutex);
        return VKD3D_WAIT_OBJECT_0;
    }

    pthread_mutex_unlock(&impl->mutex);
    FIXME("Timed wait not implemented yet.\n");
    return VKD3D_WAIT_FAILED;
}

HRESULT vkd3d_signal_event(HANDLE event)
{
    struct vkd3d_event *impl = event;
    int rc;

    TRACE("event %p.\n", event);

    if ((rc = pthread_mutex_lock(&impl->mutex)))
    {
        ERR("Failed to lock mutex, error %d.\n", rc);
        return E_FAIL;
    }
    impl->is_signaled = true;
    pthread_cond_signal(&impl->cond);
    pthread_mutex_unlock(&impl->mutex);

    return S_OK;
}

void vkd3d_destroy_event(HANDLE event)
{
    struct vkd3d_event *impl = event;
    int rc;

    TRACE("event %p.\n", event);

    if ((rc = pthread_mutex_destroy(&impl->mutex)))
        ERR("Failed to destroy mutex, error %d.\n", rc);
    if ((rc = pthread_cond_destroy(&impl->cond)))
        ERR("Failed to destroy condition variable, error %d.\n", rc);
    vkd3d_free(impl);
}

#endif  /* _WIN32 */

HRESULT WINAPI D3DCreateBlob(SIZE_T data_size, ID3DBlob **blob)
{
    HRESULT hr;
    void *data;

    TRACE("data_size %"PRIuPTR", blob %p.\n", (uintptr_t)data_size, blob);

    if (!blob)
    {
        WARN("Invalid 'blob' pointer specified.\n");
        return D3DERR_INVALIDCALL;
    }

    if (!(data = vkd3d_calloc(data_size, 1)))
        return E_OUTOFMEMORY;

    if (FAILED(hr = vkd3d_blob_create(data, data_size, blob)))
    {
        WARN("Failed to create blob object, hr %s.\n", debugstr_hresult(hr));
        vkd3d_free(data);
    }
    return hr;
}

static bool check_blob_part(uint32_t tag, D3D_BLOB_PART part)
{
    bool add = false;

    switch (part)
    {
        case D3D_BLOB_INPUT_SIGNATURE_BLOB:
            if (tag == TAG_ISGN)
                add = true;
            break;

        case D3D_BLOB_OUTPUT_SIGNATURE_BLOB:
            if (tag == TAG_OSGN || tag == TAG_OSG5)
                add = true;
            break;

        case D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB:
            if (tag == TAG_ISGN || tag == TAG_OSGN || tag == TAG_OSG5)
                add = true;
            break;

        case D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB:
            if (tag == TAG_PCSG)
                add = true;
            break;

        case D3D_BLOB_ALL_SIGNATURE_BLOB:
            if (tag == TAG_ISGN || tag == TAG_OSGN || tag == TAG_OSG5 || tag == TAG_PCSG)
                add = true;
            break;

        case D3D_BLOB_DEBUG_INFO:
            if (tag == TAG_SDBG)
                add = true;
            break;

        case D3D_BLOB_LEGACY_SHADER:
            if (tag == TAG_AON9)
                add = true;
            break;

        case D3D_BLOB_XNA_PREPASS_SHADER:
            if (tag == TAG_XNAP)
                add = true;
            break;

        case D3D_BLOB_XNA_SHADER:
            if (tag == TAG_XNAS)
                add = true;
            break;

        default:
            FIXME("Unhandled D3D_BLOB_PART %s.\n", debug_d3d_blob_part(part));
            break;
    }

    TRACE("%s tag %s.\n", add ? "Add" : "Skip", debugstr_an((const char *)&tag, 4));

    return add;
}

static HRESULT get_blob_part(const void *data, SIZE_T data_size,
        D3D_BLOB_PART part, unsigned int flags, ID3DBlob **blob)
{
    const struct vkd3d_shader_code src_dxbc = {.code = data, .size = data_size};
    struct vkd3d_shader_dxbc_section_desc *sections;
    struct vkd3d_shader_dxbc_desc src_dxbc_desc;
    struct vkd3d_shader_code dst_dxbc;
    unsigned int section_count, i;
    HRESULT hr;
    int ret;

    if (!data || !data_size || flags || !blob)
    {
        WARN("Invalid arguments: data %p, data_size %"PRIuPTR", flags %#x, blob %p.\n",
                data, (uintptr_t)data_size, flags, blob);
        return D3DERR_INVALIDCALL;
    }

    if (part > D3D_BLOB_TEST_COMPILE_PERF
            || (part < D3D_BLOB_TEST_ALTERNATE_SHADER && part > D3D_BLOB_XNA_SHADER))
    {
        WARN("Invalid D3D_BLOB_PART %s.\n", debug_d3d_blob_part(part));
        return D3DERR_INVALIDCALL;
    }

    if ((ret = vkd3d_shader_parse_dxbc(&src_dxbc, 0, &src_dxbc_desc, NULL)) < 0)
    {
        WARN("Failed to parse source data, ret %d.\n", ret);
        return D3DERR_INVALIDCALL;
    }

    if (!(sections = vkd3d_calloc(src_dxbc_desc.section_count, sizeof(*sections))))
    {
        ERR("Failed to allocate sections memory.\n");
        vkd3d_shader_free_dxbc(&src_dxbc_desc);
        return E_OUTOFMEMORY;
    }

    for (i = 0, section_count = 0; i < src_dxbc_desc.section_count; ++i)
    {
        const struct vkd3d_shader_dxbc_section_desc *src_section = &src_dxbc_desc.sections[i];

        if (check_blob_part(src_section->tag, part))
            sections[section_count++] = *src_section;
    }

    switch (part)
    {
        case D3D_BLOB_INPUT_SIGNATURE_BLOB:
        case D3D_BLOB_OUTPUT_SIGNATURE_BLOB:
        case D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB:
        case D3D_BLOB_DEBUG_INFO:
        case D3D_BLOB_LEGACY_SHADER:
        case D3D_BLOB_XNA_PREPASS_SHADER:
        case D3D_BLOB_XNA_SHADER:
            if (section_count != 1)
                section_count = 0;
            break;

        case D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB:
            if (section_count != 2)
                section_count = 0;
            break;

        case D3D_BLOB_ALL_SIGNATURE_BLOB:
            if (section_count != 3)
                section_count = 0;
            break;

        default:
            FIXME("Unhandled D3D_BLOB_PART %s.\n", debug_d3d_blob_part(part));
            break;
    }

    if (!section_count)
    {
        WARN("Nothing to write into the blob.\n");
        hr = E_FAIL;
        goto done;
    }

    /* Some parts aren't full DXBCs, they contain only the data. */
    if (section_count == 1 && (part == D3D_BLOB_DEBUG_INFO || part == D3D_BLOB_LEGACY_SHADER
            || part == D3D_BLOB_XNA_PREPASS_SHADER || part == D3D_BLOB_XNA_SHADER))
    {
        dst_dxbc = sections[0].data;
    }
    else if ((ret = vkd3d_shader_serialize_dxbc(section_count, sections, &dst_dxbc, NULL)) < 0)
    {
        WARN("Failed to serialise DXBC, ret %d.\n", ret);
        hr = hresult_from_vkd3d_result(ret);
        goto done;
    }

    if (FAILED(hr = D3DCreateBlob(dst_dxbc.size, blob)))
        WARN("Failed to create blob, hr %s.\n", debugstr_hresult(hr));
    else
        memcpy(ID3D10Blob_GetBufferPointer(*blob), dst_dxbc.code, dst_dxbc.size);
    if (dst_dxbc.code != sections[0].data.code)
        vkd3d_shader_free_shader_code(&dst_dxbc);

done:
    vkd3d_free(sections);
    vkd3d_shader_free_dxbc(&src_dxbc_desc);

    return hr;
}

HRESULT WINAPI D3DGetBlobPart(const void *data, SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob)
{
    TRACE("data %p, data_size %"PRIuPTR", part %s, flags %#x, blob %p.\n",
            data, (uintptr_t)data_size, debug_d3d_blob_part(part), flags, blob);

    return get_blob_part(data, data_size, part, flags, blob);
}

HRESULT WINAPI D3DGetDebugInfo(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %"PRIuPTR", blob %p.\n", data, (uintptr_t)data_size, blob);

    return get_blob_part(data, data_size, D3D_BLOB_DEBUG_INFO, 0, blob);
}

HRESULT WINAPI D3DGetInputAndOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %"PRIuPTR", blob %p.\n", data, (uintptr_t)data_size, blob);

    return get_blob_part(data, data_size, D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetInputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %"PRIuPTR", blob %p.\n", data, (uintptr_t)data_size, blob);

    return get_blob_part(data, data_size, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %"PRIuPTR", blob %p.\n", data, (uintptr_t)data_size, blob);

    return get_blob_part(data, data_size, D3D_BLOB_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

static bool check_blob_strip(uint32_t tag, uint32_t flags)
{
    bool add = true;

    switch (tag)
    {
        case TAG_RDEF:
        case TAG_STAT:
            if (flags & D3DCOMPILER_STRIP_REFLECTION_DATA)
                add = false;
            break;

        case TAG_SDBG:
            if (flags & D3DCOMPILER_STRIP_DEBUG_INFO)
                add = false;
            break;

        default:
            break;
    }

    TRACE("%s tag %s.\n", add ? "Add" : "Skip", debugstr_an((const char *)&tag, 4));

    return add;
}

HRESULT WINAPI D3DStripShader(const void *data, SIZE_T data_size, UINT flags, ID3DBlob **blob)
{
    const struct vkd3d_shader_code src_dxbc = {.code = data, .size = data_size};
    struct vkd3d_shader_dxbc_section_desc *sections;
    struct vkd3d_shader_dxbc_desc src_dxbc_desc;
    struct vkd3d_shader_code dst_dxbc;
    unsigned int section_count, i;
    HRESULT hr;
    int ret;

    TRACE("data %p, data_size %"PRIuPTR", flags %#x, blob %p.\n", data, (uintptr_t)data_size, flags, blob);

    if (!blob)
    {
        WARN("Invalid 'blob' pointer specified.\n");
        return E_FAIL;
    }

    if (!data || !data_size)
    {
        WARN("Invalid arguments: data %p, data_size %"PRIuPTR".\n", data, (uintptr_t)data_size);
        return D3DERR_INVALIDCALL;
    }

    if ((ret = vkd3d_shader_parse_dxbc(&src_dxbc, 0, &src_dxbc_desc, NULL)) < 0)
    {
        WARN("Failed to parse source data, ret %d.\n", ret);
        return D3DERR_INVALIDCALL;
    }

    if (!(sections = vkd3d_calloc(src_dxbc_desc.section_count, sizeof(*sections))))
    {
        ERR("Failed to allocate sections memory.\n");
        vkd3d_shader_free_dxbc(&src_dxbc_desc);
        return E_OUTOFMEMORY;
    }

    if (flags & ~(D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO))
        FIXME("Unhandled flags %#x.\n", flags);

    for (i = 0, section_count = 0; i < src_dxbc_desc.section_count; ++i)
    {
        const struct vkd3d_shader_dxbc_section_desc *src_section = &src_dxbc_desc.sections[i];

        if (check_blob_strip(src_section->tag, flags))
            sections[section_count++] = *src_section;
    }

    if ((ret = vkd3d_shader_serialize_dxbc(section_count, sections, &dst_dxbc, NULL)) < 0)
    {
        WARN("Failed to serialise DXBC, ret %d.\n", ret);
        hr = hresult_from_vkd3d_result(ret);
        goto done;
    }

    if (FAILED(hr = D3DCreateBlob(dst_dxbc.size, blob)))
        WARN("Failed to create blob, hr %s.\n", debugstr_hresult(hr));
    else
        memcpy(ID3D10Blob_GetBufferPointer(*blob), dst_dxbc.code, dst_dxbc.size);
    vkd3d_shader_free_shader_code(&dst_dxbc);

done:
    vkd3d_free(sections);
    vkd3d_shader_free_dxbc(&src_dxbc_desc);

    return hr;
}

void vkd3d_utils_set_log_callback(PFN_vkd3d_log callback)
{
    vkd3d_set_log_callback(callback);
    vkd3d_dbg_set_log_callback(callback);
}

HRESULT WINAPI D3DDisassemble(const void *data, SIZE_T data_size,
        UINT flags, const char *comments, ID3DBlob **blob)
{
    enum vkd3d_shader_source_type source_type;
    struct vkd3d_shader_compile_info info;
    struct vkd3d_shader_code output;
    const char *p, *q, *end;
    char *messages;
    HRESULT hr;
    int ret;

    static const struct vkd3d_shader_compile_option options[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_16},
    };

    TRACE("data %p, data_size %"PRIuPTR", flags %#x, comments %p, blob %p.\n",
            data, (uintptr_t)data_size, flags, comments, blob);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    if (comments)
        FIXME("Ignoring comments %s.\n", debugstr_a(comments));

    if (!data_size)
        return E_INVALIDARG;

    if (data_size >= sizeof(uint32_t) && *(uint32_t *)data == TAG_DXBC)
        source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;
    else
        source_type = VKD3D_SHADER_SOURCE_D3D_BYTECODE;

    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next = NULL;
    info.source.code = data;
    info.source.size = data_size;
    info.source_type = source_type;
    info.target_type = VKD3D_SHADER_TARGET_D3D_ASM;
    info.options = options;
    info.option_count = ARRAY_SIZE(options);
    info.log_level = VKD3D_SHADER_LOG_INFO;
    info.source_name = NULL;

    ret = vkd3d_shader_compile(&info, &output, &messages);
    if (messages && *messages && WARN_ON())
    {
        WARN("Compiler log:\n");
        for (p = messages, end = p + strlen(p); p < end; p = q)
        {
            if (!(q = memchr(p, '\n', end - p)))
                q = end;
            else
                ++q;
            WARN("    %.*s", (int)(q - p), p);
        }
        WARN("\n");
    }
    vkd3d_shader_free_messages(messages);

    if (ret < 0)
    {
        WARN("Failed to disassemble shader, ret %d.\n", ret);
        return hresult_from_vkd3d_result(ret);
    }

    if (FAILED(hr = vkd3d_blob_create((void *)output.code, output.size, blob)))
        vkd3d_shader_free_shader_code(&output);

    return hr;
}
