/*
 * Copyright 2017-2019 Józef Kucia for CodeWeavers
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

#ifndef __VKD3D_SHADER_H
#define __VKD3D_SHADER_H

#include <stdbool.h>
#include <stdint.h>
#include <vkd3d_types.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * \file vkd3d_shader.h
 *
 * \since 1.2
 *
 * This file contains definitions for the vkd3d-shader library.
 *
 * The vkd3d-shader library provides multiple utilities related to the
 * compilation, transformation, and reflection of GPU shaders.
 */

/** \since 1.3 */
enum vkd3d_shader_api_version
{
    VKD3D_SHADER_API_VERSION_1_0,
    VKD3D_SHADER_API_VERSION_1_1,
    VKD3D_SHADER_API_VERSION_1_2,
    VKD3D_SHADER_API_VERSION_1_3,
    VKD3D_SHADER_API_VERSION_1_4,
    VKD3D_SHADER_API_VERSION_1_5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_API_VERSION),
};

/** The type of a chained structure. */
enum vkd3d_shader_structure_type
{
    /** The structure is a vkd3d_shader_compile_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO,
    /** The structure is a vkd3d_shader_interface_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO,
    /** The structure is a vkd3d_shader_scan_descriptor_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO,
    /** The structure is a vkd3d_shader_spirv_domain_shader_target_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_DOMAIN_SHADER_TARGET_INFO,
    /** The structure is a vkd3d_shader_spirv_target_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_TARGET_INFO,
    /** The structure is a vkd3d_shader_transform_feedback_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_TRANSFORM_FEEDBACK_INFO,

    /**
     * The structure is a vkd3d_shader_hlsl_source_info structure.
     * \since 1.3
     */
    VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO,
    /**
     * The structure is a vkd3d_shader_preprocess_info structure.
     * \since 1.3
     */
    VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO,
    /**
     * The structure is a vkd3d_shader_descriptor_offset_info structure.
     * \since 1.3
     */
    VKD3D_SHADER_STRUCTURE_TYPE_DESCRIPTOR_OFFSET_INFO,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_STRUCTURE_TYPE),
};

/**
 * Determines how buffer UAVs are stored.
 *
 * This also affects UAV counters in Vulkan environments. In OpenGL
 * environments, atomic counter buffers are always used for UAV counters.
 */
enum vkd3d_shader_compile_option_buffer_uav
{
    /** Use buffer textures for buffer UAVs. This is the default value. */
    VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV_STORAGE_TEXEL_BUFFER = 0x00000000,
    /** Use storage buffers for buffer UAVs. */
    VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV_STORAGE_BUFFER       = 0x00000001,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV),
};

/**
 * Determines how typed UAVs are declared.
 * \since 1.5
 */
enum vkd3d_shader_compile_option_typed_uav
{
    /** Use R32(u)i/R32f format for UAVs which are read from. This is the default value. */
    VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_R32     = 0x00000000,
    /**
     * Use Unknown format for UAVs which are read from. This should only be set if
     * shaderStorageImageReadWithoutFormat is enabled in the target environment.
     */
    VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_UNKNOWN = 0x00000001,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV),
};

enum vkd3d_shader_compile_option_formatting_flags
{
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_NONE    = 0x00000000,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_COLOUR  = 0x00000001,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT  = 0x00000002,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_OFFSETS = 0x00000004,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_HEADER  = 0x00000008,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_RAW_IDS = 0x00000010,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_FORMATTING_FLAGS),
};

enum vkd3d_shader_compile_option_name
{
    /**
     * If \a value is nonzero, do not include debug information in the
     * compiled shader. The default value is zero.
     *
     * This option is supported by vkd3d_shader_compile(). However, not all
     * compilers support generating debug information.
     */
    VKD3D_SHADER_COMPILE_OPTION_STRIP_DEBUG = 0x00000001,
    /** \a value is a member of enum vkd3d_shader_compile_option_buffer_uav. */
    VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV  = 0x00000002,
    /** \a value is a member of enum vkd3d_shader_compile_option_formatting_flags. */
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING  = 0x00000003,
    /** \a value is a member of enum vkd3d_shader_api_version. \since 1.3 */
    VKD3D_SHADER_COMPILE_OPTION_API_VERSION = 0x00000004,
    /** \a value is a member of enum vkd3d_shader_compile_option_typed_uav. \since 1.5 */
    VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV   = 0x00000005,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_NAME),
};

/**
 * Various settings which may affect shader compilation or scanning, passed as
 * part of struct vkd3d_shader_compile_info. For more details, see the
 * documentation for individual options.
 */
struct vkd3d_shader_compile_option
{
    /** Name of the option. */
    enum vkd3d_shader_compile_option_name name;
    /**
     * A value associated with the option. The type and interpretation of the
     * value depends on the option in question.
     */
    unsigned int value;
};

/** Describes which shader stages a resource is visible to. */
enum vkd3d_shader_visibility
{
    /** The resource is visible to all shader stages. */
    VKD3D_SHADER_VISIBILITY_ALL = 0,
    /** The resource is visible only to the vertex shader. */
    VKD3D_SHADER_VISIBILITY_VERTEX = 1,
    /** The resource is visible only to the hull shader. */
    VKD3D_SHADER_VISIBILITY_HULL = 2,
    /** The resource is visible only to the domain shader. */
    VKD3D_SHADER_VISIBILITY_DOMAIN = 3,
    /** The resource is visible only to the geometry shader. */
    VKD3D_SHADER_VISIBILITY_GEOMETRY = 4,
    /** The resource is visible only to the pixel shader. */
    VKD3D_SHADER_VISIBILITY_PIXEL = 5,

    /** The resource is visible only to the compute shader. */
    VKD3D_SHADER_VISIBILITY_COMPUTE = 1000000000,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_VISIBILITY),
};

/** A generic structure containing a GPU shader, in text or byte-code format. */
struct vkd3d_shader_code
{
    /**
     * Pointer to the code. Note that textual formats are not null-terminated.
     * Therefore \a size should not include a null terminator, when this
     * structure is passed as input to a vkd3d-shader function, and the
     * allocated string will not include a null terminator when this structure
     * is used as output.
     */
    const void *code;
    /** Size of \a code, in bytes. */
    size_t size;
};

/** The type of a shader resource descriptor. */
enum vkd3d_shader_descriptor_type
{
    /**
     * The descriptor is a shader resource view. In Direct3D assembly, this is
     * bound to a t# register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_SRV     = 0x0,
    /**
     * The descriptor is an unordered access view. In Direct3D assembly, this is
     * bound to a u# register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_UAV     = 0x1,
    /**
     * The descriptor is a constant buffer view. In Direct3D assembly, this is
     * bound to a cb# register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_CBV     = 0x2,
    /**
     * The descriptor is a sampler. In Direct3D assembly, this is bound to an s#
     * register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER = 0x3,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_DESCRIPTOR_TYPE),
};

/**
 * A common structure describing the bind point of a descriptor or descriptor
 * array in the target environment.
 */
struct vkd3d_shader_descriptor_binding
{
    /**
     * The set of the descriptor. If the target environment does not support
     * descriptor sets, this value must be set to 0.
     */
    unsigned int set;
    /** The binding index of the descriptor. */
    unsigned int binding;
    /**
     * The size of this descriptor array. If an offset is specified for this
     * binding by the vkd3d_shader_descriptor_offset_info structure, counting
     * starts at that offset.
     */
    unsigned int count;
};

enum vkd3d_shader_binding_flag
{
    VKD3D_SHADER_BINDING_FLAG_BUFFER = 0x00000001,
    VKD3D_SHADER_BINDING_FLAG_IMAGE  = 0x00000002,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_BINDING_FLAG),
};

enum vkd3d_shader_parameter_type
{
    VKD3D_SHADER_PARAMETER_TYPE_UNKNOWN,
    VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT,
    VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_PARAMETER_TYPE),
};

enum vkd3d_shader_parameter_data_type
{
    VKD3D_SHADER_PARAMETER_DATA_TYPE_UNKNOWN,
    VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_PARAMETER_DATA_TYPE),
};

enum vkd3d_shader_parameter_name
{
    VKD3D_SHADER_PARAMETER_NAME_UNKNOWN,
    VKD3D_SHADER_PARAMETER_NAME_RASTERIZER_SAMPLE_COUNT,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_PARAMETER_NAME),
};

struct vkd3d_shader_parameter_immediate_constant
{
    union
    {
        uint32_t u32;
    } u;
};

struct vkd3d_shader_parameter_specialization_constant
{
    uint32_t id;
};

struct vkd3d_shader_parameter
{
    enum vkd3d_shader_parameter_name name;
    enum vkd3d_shader_parameter_type type;
    enum vkd3d_shader_parameter_data_type data_type;
    union
    {
        struct vkd3d_shader_parameter_immediate_constant immediate_constant;
        struct vkd3d_shader_parameter_specialization_constant specialization_constant;
    } u;
};

/**
 * Describes the mapping of a single resource or resource array to its binding
 * point in the target environment.
 *
 * For example, to map a Direct3D SRV with register space 2, register "t3" to
 * a Vulkan descriptor in set 4 and with binding 5, set the following members:
 * - \a type = VKD3D_SHADER_DESCRIPTOR_TYPE_SRV
 * - \a register_space = 2
 * - \a register_index = 3
 * - \a binding.set = 4
 * - \a binding.binding = 5
 * - \a binding.count = 1
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_resource_binding
{
    /** The type of this descriptor. */
    enum vkd3d_shader_descriptor_type type;
    /**
     * Register space of the Direct3D resource. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int register_space;
    /** Register index of the DXBC resource. */
    unsigned int register_index;
    /** Shader stage(s) to which the resource is visible. */
    enum vkd3d_shader_visibility shader_visibility;
    /** A combination of zero or more elements of vkd3d_shader_binding_flag. */
    unsigned int flags;

    /** The binding in the target environment. */
    struct vkd3d_shader_descriptor_binding binding;
};

#define VKD3D_SHADER_DUMMY_SAMPLER_INDEX ~0u

/**
 * Describes the mapping of a Direct3D resource-sampler pair to a combined
 * sampler (i.e. sampled image).
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_combined_resource_sampler
{
    /**
     * Register space of the Direct3D resource. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int resource_space;
    /** Register index of the Direct3D resource. */
    unsigned int resource_index;
    /**
     * Register space of the Direct3D sampler. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int sampler_space;
    /** Register index of the Direct3D sampler. */
    unsigned int sampler_index;
    /** Shader stage(s) to which the resource is visible. */
    enum vkd3d_shader_visibility shader_visibility;
    /** A combination of zero or more elements of vkd3d_shader_binding_flag. */
    unsigned int flags;

    /** The binding in the target environment. */
    struct vkd3d_shader_descriptor_binding binding;
};

/**
 * Describes the mapping of a single Direct3D UAV counter.
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_uav_counter_binding
{
    /**
     * Register space of the Direct3D UAV descriptor. If the source format does
     * not support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int register_space;
    /** Register index of the Direct3D UAV descriptor. */
    unsigned int register_index;
    /** Shader stage(s) to which the UAV counter is visible. */
    enum vkd3d_shader_visibility shader_visibility;

    /** The binding in the target environment. */
    struct vkd3d_shader_descriptor_binding binding;
    unsigned int offset;
};

/**
 * Describes the mapping of a Direct3D constant buffer to a range of push
 * constants in the target environment.
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_push_constant_buffer
{
    /**
     * Register space of the Direct3D resource. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int register_space;
    /** Register index of the Direct3D resource. */
    unsigned int register_index;
    /** Shader stage(s) to which the resource is visible. */
    enum vkd3d_shader_visibility shader_visibility;

    /** Offset, in bytes, of the target push constants. */
    unsigned int offset;
    /** Size, in bytes, of the target push constants. */
    unsigned int size;
};

/**
 * A chained structure describing the interface between a compiled shader and
 * the target environment.
 *
 * For example, when compiling Direct3D shader byte code to SPIR-V, this
 * structure contains mappings from Direct3D descriptor registers to SPIR-V
 * descriptor bindings.
 *
 * This structure is optional. If omitted, vkd3d_shader_compile() will use a
 * default mapping, in which resources are mapped to sequential bindings in
 * register set 0.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 */
struct vkd3d_shader_interface_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /** Pointer to an array of bindings for shader resource descriptors. */
    const struct vkd3d_shader_resource_binding *bindings;
    /** Size, in elements, of \ref bindings. */
    unsigned int binding_count;

    /** Pointer to an array of bindings for push constant buffers. */
    const struct vkd3d_shader_push_constant_buffer *push_constant_buffers;
    /** Size, in elements, of \ref push_constant_buffers. */
    unsigned int push_constant_buffer_count;

    /** Pointer to an array of bindings for combined samplers. */
    const struct vkd3d_shader_combined_resource_sampler *combined_samplers;
    /** Size, in elements, of \ref combined_samplers. */
    unsigned int combined_sampler_count;

    /** Pointer to an array of bindings for UAV counters. */
    const struct vkd3d_shader_uav_counter_binding *uav_counters;
    /** Size, in elements, of \ref uav_counters. */
    unsigned int uav_counter_count;
};

struct vkd3d_shader_transform_feedback_element
{
    unsigned int stream_index;
    const char *semantic_name;
    unsigned int semantic_index;
    uint8_t component_index;
    uint8_t component_count;
    uint8_t output_slot;
};

/* Extends vkd3d_shader_interface_info. */
struct vkd3d_shader_transform_feedback_info
{
    enum vkd3d_shader_structure_type type;
    const void *next;

    const struct vkd3d_shader_transform_feedback_element *elements;
    unsigned int element_count;
    const unsigned int *buffer_strides;
    unsigned int buffer_stride_count;
};

struct vkd3d_shader_descriptor_offset
{
    unsigned int static_offset;
    unsigned int dynamic_offset_index;
};

/**
 * A chained structure containing descriptor offsets.
 *
 * This structure is optional.
 *
 * This structure extends vkd3d_shader_interface_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.3
 */
struct vkd3d_shader_descriptor_offset_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_DESCRIPTOR_OFFSET_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /**
     * Byte offset within the push constants of an array of 32-bit
     * descriptor array offsets. See the description of 'binding_offsets'
     * below.
     */
    unsigned int descriptor_table_offset;
    /** Size, in elements, of the descriptor table push constant array. */
    unsigned int descriptor_table_count;

    /**
     * Pointer to an array of struct vkd3d_shader_descriptor_offset objects.
     * The 'static_offset' field contains an offset into the descriptor arrays
     * referenced by the 'bindings' array in struct vkd3d_shader_interface_info.
     * This allows mapping multiple shader resource arrays to a single binding
     * point in the target environment.
     *
     * 'dynamic_offset_index' in struct vkd3d_shader_descriptor_offset allows
     * offsets to be set at runtime. The 32-bit descriptor table push constant
     * at this index will be added to 'static_offset' to calculate the final
     * binding offset.
     *
     * If runtime offsets are not required, set all 'dynamic_offset_index'
     * values to \c ~0u and 'descriptor_table_count' to zero.
     *
     * For example, to map Direct3D constant buffer registers 'cb0[0:3]' and
     * 'cb1[6:7]' to descriptors 8-12 and 4-5 in the Vulkan descriptor array in
     * descriptor set 3 and with binding 2, set the following values in the
     * 'bindings' array in struct vkd3d_shader_interface_info:
     *
     * \code
     * type = VKD3D_SHADER_DESCRIPTOR_TYPE_CBV
     * register_space = 0
     * register_index = 0
     * binding.set = 3
     * binding.binding = 2
     * binding.count = 4
     *
     * type = VKD3D_SHADER_DESCRIPTOR_TYPE_CBV
     * register_space = 0
     * register_index = 6
     * binding.set = 3
     * binding.binding = 2
     * binding.count = 2
     * \endcode
     *
     * and then pass \c {8, \c 4} as static binding offsets here.
     *
     * This field may be NULL, in which case the corresponding offsets are
     * specified to be 0.
     */
    const struct vkd3d_shader_descriptor_offset *binding_offsets;

    /**
     * Pointer to an array of offsets into the descriptor arrays referenced by
     * the 'uav_counters' array in struct vkd3d_shader_interface_info. This
     * works the same way as \ref binding_offsets above.
     */
    const struct vkd3d_shader_descriptor_offset *uav_counter_offsets;
};

/** The format of a shader to be compiled or scanned. */
enum vkd3d_shader_source_type
{
    /**
     * The shader has no type or is to be ignored. This is not a valid value
     * for vkd3d_shader_compile() or vkd3d_shader_scan().
     */
    VKD3D_SHADER_SOURCE_NONE,
    /**
     * A 'Tokenized Program Format' shader embedded in a DXBC container. This is
     * the format used for Direct3D shader model 4 and 5 shaders.
     */
    VKD3D_SHADER_SOURCE_DXBC_TPF,
    /** High-Level Shader Language source code. \since 1.3 */
    VKD3D_SHADER_SOURCE_HLSL,
    /**
     * Legacy Direct3D byte-code. This is the format used for Direct3D shader
     * model 1, 2, and 3 shaders. \since 1.3
     */
    VKD3D_SHADER_SOURCE_D3D_BYTECODE,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SOURCE_TYPE),
};

/** The output format of a compiled shader. */
enum vkd3d_shader_target_type
{
    /**
     * The shader has no type or is to be ignored. This is not a valid value
     * for vkd3d_shader_compile() or vkd3d_shader_scan().
     */
    VKD3D_SHADER_TARGET_NONE,
    /**
     * A SPIR-V shader in binary form. This is the format used for Vulkan
     * shaders.
     */
    VKD3D_SHADER_TARGET_SPIRV_BINARY,
    VKD3D_SHADER_TARGET_SPIRV_TEXT,
    /**
     * Direct3D shader assembly. \since 1.3
     */
    VKD3D_SHADER_TARGET_D3D_ASM,
    /**
     * Legacy Direct3D byte-code. This is the format used for Direct3D shader
     * model 1, 2, and 3 shaders. \since 1.3
     */
    VKD3D_SHADER_TARGET_D3D_BYTECODE,
    /**
     * A 'Tokenized Program Format' shader embedded in a DXBC container. This is
     * the format used for Direct3D shader model 4 and 5 shaders. \since 1.3
     */
    VKD3D_SHADER_TARGET_DXBC_TPF,
    /**
     * An 'OpenGL Shading Language' shader. \since 1.3
     */
    VKD3D_SHADER_TARGET_GLSL,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TARGET_TYPE),
};

/**
 * Describes the minimum severity of compilation messages returned by
 * vkd3d_shader_compile() and similar functions.
 */
enum vkd3d_shader_log_level
{
    /** No messages will be returned. */
    VKD3D_SHADER_LOG_NONE,
    /** Only fatal errors which prevent successful compilation will be returned. */
    VKD3D_SHADER_LOG_ERROR,
    /** Non-fatal warnings and fatal errors will be returned. */
    VKD3D_SHADER_LOG_WARNING,
    /**
     * All messages, including general informational messages, will be returned.
     */
    VKD3D_SHADER_LOG_INFO,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_LOG_LEVEL),
};

/**
 * A chained structure containing compilation parameters.
 */
struct vkd3d_shader_compile_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO. */
    enum vkd3d_shader_structure_type type;
    /**
     * Optional pointer to a structure containing further parameters. For a list
     * of valid structures, refer to the respective function documentation. If
     * no further parameters are needed, this field should be set to NULL.
     */
    const void *next;

    /** Input source code or byte code. */
    struct vkd3d_shader_code source;

    /** Format of the input code passed in \ref source. */
    enum vkd3d_shader_source_type source_type;
    /** Desired output format. */
    enum vkd3d_shader_target_type target_type;

    /**
     * Pointer to an array of compilation options. This field is ignored if
     * \ref option_count is zero, but must be valid otherwise.
     *
     * If the same option is specified multiple times, only the last value is
     * used.
     *
     * Options not relevant to or not supported by a particular shader compiler
     * or scanner will be ignored.
     */
    const struct vkd3d_shader_compile_option *options;
    /** Size, in elements, of \ref options. */
    unsigned int option_count;

    /** Minimum severity of messages returned from the shader function. */
    enum vkd3d_shader_log_level log_level;
    /**
     * Name of the initial source file, which may be used in error messages or
     * debug information. This parameter is optional and may be NULL.
     */
    const char *source_name;
};

enum vkd3d_shader_spirv_environment
{
    VKD3D_SHADER_SPIRV_ENVIRONMENT_NONE,
    VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5,
    VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0, /* default target */

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SPIRV_ENVIRONMENT),
};

enum vkd3d_shader_spirv_extension
{
    VKD3D_SHADER_SPIRV_EXTENSION_NONE,
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_DEMOTE_TO_HELPER_INVOCATION,
    /** \since 1.3 */
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_DESCRIPTOR_INDEXING,
    /** \since 1.3 */
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_STENCIL_EXPORT,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SPIRV_EXTENSION),
};

/* Extends vkd3d_shader_compile_info. */
struct vkd3d_shader_spirv_target_info
{
    enum vkd3d_shader_structure_type type;
    const void *next;

    const char *entry_point; /* "main" if NULL. */

    enum vkd3d_shader_spirv_environment environment;

    const enum vkd3d_shader_spirv_extension *extensions;
    unsigned int extension_count;

    const struct vkd3d_shader_parameter *parameters;
    unsigned int parameter_count;

    bool dual_source_blending;
    const unsigned int *output_swizzles;
    unsigned int output_swizzle_count;
};

enum vkd3d_shader_tessellator_output_primitive
{
    VKD3D_SHADER_TESSELLATOR_OUTPUT_POINT        = 0x1,
    VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE         = 0x2,
    VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW  = 0x3,
    VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW = 0x4,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TESSELLATOR_OUTPUT_PRIMITIVE),
};

enum vkd3d_shader_tessellator_partitioning
{
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_INTEGER         = 0x1,
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_POW2            = 0x2,
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD  = 0x3,
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN = 0x4,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TESSELLATOR_PARTITIONING),
};

/* Extends vkd3d_shader_spirv_target_info. */
struct vkd3d_shader_spirv_domain_shader_target_info
{
    enum vkd3d_shader_structure_type type;
    const void *next;

    enum vkd3d_shader_tessellator_output_primitive output_primitive;
    enum vkd3d_shader_tessellator_partitioning partitioning;
};

/**
 * A single preprocessor macro, passed as part of struct
 * vkd3d_shader_preprocess_info.
 */
struct vkd3d_shader_macro
{
    /**
     * Pointer to a null-terminated string containing the name of a macro. This
     * macro must not be a parameterized (i.e. function-like) macro. If this
     * field is not a valid macro identifier, this macro will be ignored.
     */
    const char *name;
    /**
     * Optional pointer to a null-terminated string containing the expansion of
     * the macro. This field may be set to NULL, in which case the macro has an
     * empty expansion.
     */
    const char *value;
};

/**
 * Type of a callback function which will be used to open preprocessor includes.
 *
 * This callback function is passed as part of struct
 * vkd3d_shader_preprocess_info.
 *
 * If this function fails, vkd3d-shader will emit a compilation error, and the
 * \a pfn_close_include callback will not be called.
 *
 * \param filename Unquoted string used as an argument to the \#include
 * directive.
 *
 * \param local Whether the \#include directive is requesting a local (i.e.
 * double-quoted) or system (i.e. angle-bracketed) include.
 *
 * \param parent_data Unprocessed source code of the file in which this
 * \#include directive is evaluated. This parameter may be NULL.
 *
 * \param context The user-defined pointer passed to struct
 * vkd3d_shader_preprocess_info.
 *
 * \param out Output location for the full contents of the included file. The
 * code need not be allocated using standard vkd3d functions, but must remain
 * valid until the corresponding call to \a pfn_close_include. If this function
 * fails, the contents of this parameter are ignored.
 *
 * \return A member of \ref vkd3d_result.
 */
typedef int (*PFN_vkd3d_shader_open_include)(const char *filename, bool local,
        const char *parent_data, void *context, struct vkd3d_shader_code *out);
/**
 * Type of a callback function which will be used to close preprocessor
 * includes.
 *
 * This callback function is passed as part of struct
 * vkd3d_shader_preprocess_info.
 *
 * \param code Contents of the included file, which were allocated by the
 * \ref pfn_open_include callback. The user must free them.
 *
 * \param context The user-defined pointer passed to struct
 * vkd3d_shader_preprocess_info.
 */
typedef void (*PFN_vkd3d_shader_close_include)(const struct vkd3d_shader_code *code, void *context);

/**
 * A chained structure containing preprocessing parameters.
 *
 * This structure is optional.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.3
 */
struct vkd3d_shader_preprocess_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /**
     * Pointer to an array of predefined macros. Each macro in this array will
     * be expanded as if a corresponding #define statement were prepended to the
     * source code.
     *
     * If the same macro is specified multiple times, only the last value is
     * used.
     */
    const struct vkd3d_shader_macro *macros;
    /** Size, in elements, of \ref macros. */
    unsigned int macro_count;

    /**
     * Optional pointer to a callback function, which will be called in order to
     * evaluate \#include directives. The function receives parameters
     * corresponding to the directive's arguments, and should return the
     * complete text of the included file.
     *
     * If this field is set to NULL, or if this structure is omitted,
     * vkd3d-shader will attempt to open included files using POSIX file APIs.
     *
     * If this field is set to NULL, the \ref pfn_close_include field must also
     * be set to NULL.
     */
    PFN_vkd3d_shader_open_include pfn_open_include;
    /**
     * Optional pointer to a callback function, which will be called whenever an
     * included file is closed. This function will be called exactly once for
     * each successful call to \ref pfn_open_include, and should be used to free
     * any resources allocated thereby.
     *
     * If this field is set to NULL, the \ref pfn_open_include field must also
     * be set to NULL.
     */
    PFN_vkd3d_shader_close_include pfn_close_include;
    /**
     * User-defined pointer which will be passed unmodified to the
     * \ref pfn_open_include and \ref pfn_close_include callbacks.
     */
    void *include_context;
};

/**
 * A chained structure containing HLSL compilation parameters.
 *
 * This structure is optional.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.3
 */
struct vkd3d_shader_hlsl_source_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /**
     * Optional pointer to a null-terminated string containing the shader entry
     * point.
     *
     * If this parameter is NULL, vkd3d-shader uses the entry point "main".
     */
    const char *entry_point;
    struct vkd3d_shader_code secondary_code;
    /**
     * Pointer to a null-terminated string containing the target shader
     * profile.
     */
    const char *profile;
};

/* root signature 1.0 */
enum vkd3d_shader_filter
{
    VKD3D_SHADER_FILTER_MIN_MAG_MIP_POINT                          = 0x000,
    VKD3D_SHADER_FILTER_MIN_MAG_POINT_MIP_LINEAR                   = 0x001,
    VKD3D_SHADER_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT             = 0x004,
    VKD3D_SHADER_FILTER_MIN_POINT_MAG_MIP_LINEAR                   = 0x005,
    VKD3D_SHADER_FILTER_MIN_LINEAR_MAG_MIP_POINT                   = 0x010,
    VKD3D_SHADER_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR            = 0x011,
    VKD3D_SHADER_FILTER_MIN_MAG_LINEAR_MIP_POINT                   = 0x014,
    VKD3D_SHADER_FILTER_MIN_MAG_MIP_LINEAR                         = 0x015,
    VKD3D_SHADER_FILTER_ANISOTROPIC                                = 0x055,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_MIP_POINT               = 0x080,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR        = 0x081,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT  = 0x084,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR        = 0x085,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT        = 0x090,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x091,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT        = 0x094,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR              = 0x095,
    VKD3D_SHADER_FILTER_COMPARISON_ANISOTROPIC                     = 0x0d5,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_MIP_POINT                  = 0x100,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR           = 0x101,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     = 0x104,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR           = 0x105,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT           = 0x110,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    = 0x111,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT           = 0x114,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR                 = 0x115,
    VKD3D_SHADER_FILTER_MINIMUM_ANISOTROPIC                        = 0x155,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_MIP_POINT                  = 0x180,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR           = 0x181,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     = 0x184,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR           = 0x185,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT           = 0x190,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    = 0x191,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT           = 0x194,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR                 = 0x195,
    VKD3D_SHADER_FILTER_MAXIMUM_ANISOTROPIC                        = 0x1d5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_FILTER),
};

enum vkd3d_shader_texture_address_mode
{
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_WRAP        = 0x1,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_MIRROR      = 0x2,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_CLAMP       = 0x3,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_BORDER      = 0x4,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_MIRROR_ONCE = 0x5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TEXTURE_ADDRESS_MODE),
};

enum vkd3d_shader_comparison_func
{
    VKD3D_SHADER_COMPARISON_FUNC_NEVER         = 0x1,
    VKD3D_SHADER_COMPARISON_FUNC_LESS          = 0x2,
    VKD3D_SHADER_COMPARISON_FUNC_EQUAL         = 0x3,
    VKD3D_SHADER_COMPARISON_FUNC_LESS_EQUAL    = 0x4,
    VKD3D_SHADER_COMPARISON_FUNC_GREATER       = 0x5,
    VKD3D_SHADER_COMPARISON_FUNC_NOT_EQUAL     = 0x6,
    VKD3D_SHADER_COMPARISON_FUNC_GREATER_EQUAL = 0x7,
    VKD3D_SHADER_COMPARISON_FUNC_ALWAYS        = 0x8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPARISON_FUNC),
};

enum vkd3d_shader_static_border_colour
{
    VKD3D_SHADER_STATIC_BORDER_COLOUR_TRANSPARENT_BLACK = 0x0,
    VKD3D_SHADER_STATIC_BORDER_COLOUR_OPAQUE_BLACK      = 0x1,
    VKD3D_SHADER_STATIC_BORDER_COLOUR_OPAQUE_WHITE      = 0x2,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_STATIC_BORDER_COLOUR),
};

struct vkd3d_shader_static_sampler_desc
{
    enum vkd3d_shader_filter filter;
    enum vkd3d_shader_texture_address_mode address_u;
    enum vkd3d_shader_texture_address_mode address_v;
    enum vkd3d_shader_texture_address_mode address_w;
    float mip_lod_bias;
    unsigned int max_anisotropy;
    enum vkd3d_shader_comparison_func comparison_func;
    enum vkd3d_shader_static_border_colour border_colour;
    float min_lod;
    float max_lod;
    unsigned int shader_register;
    unsigned int register_space;
    enum vkd3d_shader_visibility shader_visibility;
};

struct vkd3d_shader_descriptor_range
{
    enum vkd3d_shader_descriptor_type range_type;
    unsigned int descriptor_count;
    unsigned int base_shader_register;
    unsigned int register_space;
    unsigned int descriptor_table_offset;
};

struct vkd3d_shader_root_descriptor_table
{
    unsigned int descriptor_range_count;
    const struct vkd3d_shader_descriptor_range *descriptor_ranges;
};

struct vkd3d_shader_root_constants
{
    unsigned int shader_register;
    unsigned int register_space;
    unsigned int value_count;
};

struct vkd3d_shader_root_descriptor
{
    unsigned int shader_register;
    unsigned int register_space;
};

enum vkd3d_shader_root_parameter_type
{
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE = 0x0,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS  = 0x1,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV              = 0x2,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV              = 0x3,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV              = 0x4,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_PARAMETER_TYPE),
};

struct vkd3d_shader_root_parameter
{
    enum vkd3d_shader_root_parameter_type parameter_type;
    union
    {
        struct vkd3d_shader_root_descriptor_table descriptor_table;
        struct vkd3d_shader_root_constants constants;
        struct vkd3d_shader_root_descriptor descriptor;
    } u;
    enum vkd3d_shader_visibility shader_visibility;
};

enum vkd3d_shader_root_signature_flags
{
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_NONE                               = 0x00,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = 0x01,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS     = 0x02,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       = 0x04,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     = 0x08,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS   = 0x10,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS      = 0x20,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT                = 0x40,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_SIGNATURE_FLAGS),
};

struct vkd3d_shader_root_signature_desc
{
    unsigned int parameter_count;
    const struct vkd3d_shader_root_parameter *parameters;
    unsigned int static_sampler_count;
    const struct vkd3d_shader_static_sampler_desc *static_samplers;
    enum vkd3d_shader_root_signature_flags flags;
};

/* root signature 1.1 */
enum vkd3d_shader_root_descriptor_flags
{
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_NONE                             = 0x0,
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE                    = 0x2,
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE = 0x4,
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC                      = 0x8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_DESCRIPTOR_FLAGS),
};

enum vkd3d_shader_descriptor_range_flags
{
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_NONE                             = 0x0,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE             = 0x1,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE                    = 0x2,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE = 0x4,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC                      = 0x8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_DESCRIPTOR_RANGE_FLAGS),
};

struct vkd3d_shader_descriptor_range1
{
    enum vkd3d_shader_descriptor_type range_type;
    unsigned int descriptor_count;
    unsigned int base_shader_register;
    unsigned int register_space;
    enum vkd3d_shader_descriptor_range_flags flags;
    unsigned int descriptor_table_offset;
};

struct vkd3d_shader_root_descriptor_table1
{
    unsigned int descriptor_range_count;
    const struct vkd3d_shader_descriptor_range1 *descriptor_ranges;
};

struct vkd3d_shader_root_descriptor1
{
    unsigned int shader_register;
    unsigned int register_space;
    enum vkd3d_shader_root_descriptor_flags flags;
};

struct vkd3d_shader_root_parameter1
{
    enum vkd3d_shader_root_parameter_type parameter_type;
    union
    {
        struct vkd3d_shader_root_descriptor_table1 descriptor_table;
        struct vkd3d_shader_root_constants constants;
        struct vkd3d_shader_root_descriptor1 descriptor;
    } u;
    enum vkd3d_shader_visibility shader_visibility;
};

struct vkd3d_shader_root_signature_desc1
{
    unsigned int parameter_count;
    const struct vkd3d_shader_root_parameter1 *parameters;
    unsigned int static_sampler_count;
    const struct vkd3d_shader_static_sampler_desc *static_samplers;
    enum vkd3d_shader_root_signature_flags flags;
};

enum vkd3d_shader_root_signature_version
{
    VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0 = 0x1,
    VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1 = 0x2,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_SIGNATURE_VERSION),
};

struct vkd3d_shader_versioned_root_signature_desc
{
    enum vkd3d_shader_root_signature_version version;
    union
    {
        struct vkd3d_shader_root_signature_desc v_1_0;
        struct vkd3d_shader_root_signature_desc1 v_1_1;
    } u;
};

/**
 * The type of a shader resource, returned as part of struct
 * vkd3d_shader_descriptor_info.
 */
enum vkd3d_shader_resource_type
{
    /**
     * The type is invalid or not applicable for this descriptor. This value is
     * returned for samplers.
     */
    VKD3D_SHADER_RESOURCE_NONE              = 0x0,
    /** Dimensionless buffer. */
    VKD3D_SHADER_RESOURCE_BUFFER            = 0x1,
    /** 1-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_1D        = 0x2,
    /** 2-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2D        = 0x3,
    /** Multisampled 2-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2DMS      = 0x4,
    /** 3-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_3D        = 0x5,
    /** Cubemap texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_CUBE      = 0x6,
    /** 1-dimensional array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY   = 0x7,
    /** 2-dimensional array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY   = 0x8,
    /** Multisampled 2-dimensional array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY = 0x9,
    /** Cubemap array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY = 0xa,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_RESOURCE_TYPE),
};

/**
 * The type of the data contained in a shader resource, returned as part of
 * struct vkd3d_shader_descriptor_info. All formats are 32-bit.
 */
enum vkd3d_shader_resource_data_type
{
    /** Unsigned normalized integer. */
    VKD3D_SHADER_RESOURCE_DATA_UNORM     = 0x1,
    /** Signed normalized integer. */
    VKD3D_SHADER_RESOURCE_DATA_SNORM     = 0x2,
    /** Signed integer. */
    VKD3D_SHADER_RESOURCE_DATA_INT       = 0x3,
    /** Unsigned integer. */
    VKD3D_SHADER_RESOURCE_DATA_UINT      = 0x4,
    /** IEEE single-precision floating-point. */
    VKD3D_SHADER_RESOURCE_DATA_FLOAT     = 0x5,
    /** Undefined/type-less. \since 1.3 */
    VKD3D_SHADER_RESOURCE_DATA_MIXED     = 0x6,
    /** IEEE double-precision floating-point. \since 1.3 */
    VKD3D_SHADER_RESOURCE_DATA_DOUBLE    = 0x7,
    /** Continuation of the previous component. For example, 64-bit
     * double-precision floating-point data may be returned as two 32-bit
     * components, with the first component (containing the LSB) specified as
     * VKD3D_SHADER_RESOURCE_DATA_DOUBLE, and the second component specified
     * as VKD3D_SHADER_RESOURCE_DATA_CONTINUED. \since 1.3 */
    VKD3D_SHADER_RESOURCE_DATA_CONTINUED = 0x8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_RESOURCE_DATA_TYPE),
};

/**
 * Additional flags describing a shader descriptor, returned as part of struct
 * vkd3d_shader_descriptor_info.
 */
enum vkd3d_shader_descriptor_info_flag
{
    /**
     * The descriptor is a UAV resource, whose counter is read from or written
     * to by the shader.
     */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER             = 0x00000001,
    /** The descriptor is a UAV resource, which is read from by the shader. */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_READ                = 0x00000002,
    /** The descriptor is a comparison sampler. */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE = 0x00000004,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_DESCRIPTOR_INFO_FLAG),
};

/**
 * Describes a single shader descriptor; returned as part of
 * struct vkd3d_shader_scan_descriptor_info.
 */
struct vkd3d_shader_descriptor_info
{
    /** Type of the descriptor (for example, SRV, CBV, UAV, or sampler). */
    enum vkd3d_shader_descriptor_type type;
    /**
     * Register space of the resource, or 0 if the shader does not
     * support multiple register spaces.
     */
    unsigned int register_space;
    /** Register index of the descriptor. */
    unsigned int register_index;
    /** Resource type, if applicable, including its dimension. */
    enum vkd3d_shader_resource_type resource_type;
    /** Data type contained in the resource (for example, float or integer). */
    enum vkd3d_shader_resource_data_type resource_data_type;
    /**
     * Bitwise combination of zero or more members of
     * \ref vkd3d_shader_descriptor_info_flag.
     */
    unsigned int flags;
    /**
     *  Size of this descriptor array, or 1 if a single descriptor.
     *  For an unbounded array this value is ~0u.
     */
    unsigned int count;
};

/**
 * A chained structure enumerating the descriptors declared by a shader.
 *
 * This structure extends vkd3d_shader_compile_info.
 */
struct vkd3d_shader_scan_descriptor_info
{
    /**
     * Input; must be set to VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO.
     */
    enum vkd3d_shader_structure_type type;
    /** Input; optional pointer to a structure containing further parameters. */
    const void *next;

    /** Output; returns a pointer to an array of descriptors. */
    struct vkd3d_shader_descriptor_info *descriptors;
    /** Output; size, in elements, of \ref descriptors. */
    unsigned int descriptor_count;
};

/**
 * Data type of a shader varying, returned as part of struct
 * vkd3d_shader_signature_element.
 */
enum vkd3d_shader_component_type
{
    /** The varying has no type. */
    VKD3D_SHADER_COMPONENT_VOID     = 0x0,
    /** 32-bit unsigned integer. */
    VKD3D_SHADER_COMPONENT_UINT     = 0x1,
    /** 32-bit signed integer. */
    VKD3D_SHADER_COMPONENT_INT      = 0x2,
    /** 32-bit IEEE floating-point. */
    VKD3D_SHADER_COMPONENT_FLOAT    = 0x3,
    /** Boolean. */
    VKD3D_SHADER_COMPONENT_BOOL     = 0x4,
    /** 64-bit IEEE floating-point. */
    VKD3D_SHADER_COMPONENT_DOUBLE   = 0x5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPONENT_TYPE),
};

/** System value semantic, returned as part of struct vkd3d_shader_signature. */
enum vkd3d_shader_sysval_semantic
{
    /** No system value. */
    VKD3D_SHADER_SV_NONE                      = 0x00,
    /** Vertex position; SV_Position in Direct3D. */
    VKD3D_SHADER_SV_POSITION                  = 0x01,
    /** Clip distance; SV_ClipDistance in Direct3D. */
    VKD3D_SHADER_SV_CLIP_DISTANCE             = 0x02,
    /** Cull distance; SV_CullDistance in Direct3D. */
    VKD3D_SHADER_SV_CULL_DISTANCE             = 0x03,
    /** Render target layer; SV_RenderTargetArrayIndex in Direct3D. */
    VKD3D_SHADER_SV_RENDER_TARGET_ARRAY_INDEX = 0x04,
    /** Viewport index; SV_ViewportArrayIndex in Direct3D. */
    VKD3D_SHADER_SV_VIEWPORT_ARRAY_INDEX      = 0x05,
    /** Vertex ID; SV_VertexID in Direct3D. */
    VKD3D_SHADER_SV_VERTEX_ID                 = 0x06,
    /** Primtive ID; SV_PrimitiveID in Direct3D. */
    VKD3D_SHADER_SV_PRIMITIVE_ID              = 0x07,
    /** Instance ID; SV_InstanceID in Direct3D. */
    VKD3D_SHADER_SV_INSTANCE_ID               = 0x08,
    /** Whether the triangle is front-facing; SV_IsFrontFace in Direct3D. */
    VKD3D_SHADER_SV_IS_FRONT_FACE             = 0x09,
    /** Sample index; SV_SampleIndex in Direct3D. */
    VKD3D_SHADER_SV_SAMPLE_INDEX              = 0x0a,
    VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE      = 0x0b,
    VKD3D_SHADER_SV_TESS_FACTOR_QUADINT       = 0x0c,
    VKD3D_SHADER_SV_TESS_FACTOR_TRIEDGE       = 0x0d,
    VKD3D_SHADER_SV_TESS_FACTOR_TRIINT        = 0x0e,
    VKD3D_SHADER_SV_TESS_FACTOR_LINEDET       = 0x0f,
    VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN       = 0x10,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SYSVAL_SEMANTIC),
};

/**
 * Minimum interpolation precision of a shader varying, returned as part of
 * struct vkd3d_shader_signature_element.
 */
enum vkd3d_shader_minimum_precision
{
    VKD3D_SHADER_MINIMUM_PRECISION_NONE      = 0,
    /** 16-bit floating-point. */
    VKD3D_SHADER_MINIMUM_PRECISION_FLOAT_16  = 1,
    /** 10-bit fixed point (2 integer and 8 fractional bits). */
    VKD3D_SHADER_MINIMUM_PRECISION_FIXED_8_2 = 2,
    /** 16-bit signed integer. */
    VKD3D_SHADER_MINIMUM_PRECISION_INT_16    = 4,
    /** 16-bit unsigned integer. */
    VKD3D_SHADER_MINIMUM_PRECISION_UINT_16   = 5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_MINIMUM_PRECISION),
};

/**
 * A single shader varying, returned as part of struct vkd3d_shader_signature.
 */
struct vkd3d_shader_signature_element
{
    /** Semantic name. */
    const char *semantic_name;
    /** Semantic index, or 0 if the semantic is not indexed. */
    unsigned int semantic_index;
    /**
     * Stream index of a geometry shader output semantic. If the signature is
     * not a geometry shader output signature, this field will be set to 0.
     */
    unsigned int stream_index;
    /**
     * System value semantic. If the varying is not a system value, this field
     * will be set to VKD3D_SHADER_SV_NONE.
     */
    enum vkd3d_shader_sysval_semantic sysval_semantic;
    /** Data type. */
    enum vkd3d_shader_component_type component_type;
    /** Register index. */
    unsigned int register_index;
    /** Mask of the register components allocated to this varying. */
    unsigned int mask;
    /**
     * Subset of \ref mask which the shader reads from or writes to. Unlike
     * Direct3D shader bytecode, the mask for output and tessellation signatures
     * is not inverted, i.e. bits set in this field denote components which are
     * written to.
     */
    unsigned int used_mask;
    /** Minimum interpolation precision. */
    enum vkd3d_shader_minimum_precision min_precision;
};

/**
 * Description of a shader input or output signature. This structure is
 * populated by vkd3d_shader_parse_input_signature().
 *
 * The helper function vkd3d_shader_find_signature_element() will look up a
 * varying element by its semantic name, semantic index, and stream index.
 */
struct vkd3d_shader_signature
{
    /** Pointer to an array of varyings. */
    struct vkd3d_shader_signature_element *elements;
    /** Size, in elements, of \ref elements. */
    unsigned int element_count;
};

/** Possible values for a single component of a vkd3d-shader swizzle. */
enum vkd3d_shader_swizzle_component
{
    VKD3D_SHADER_SWIZZLE_X = 0x0,
    VKD3D_SHADER_SWIZZLE_Y = 0x1,
    VKD3D_SHADER_SWIZZLE_Z = 0x2,
    VKD3D_SHADER_SWIZZLE_W = 0x3,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SWIZZLE_COMPONENT),
};

/**
 * A mask selecting one component from a vkd3d-shader swizzle. The component has
 * type \ref vkd3d_shader_swizzle_component.
 */
#define VKD3D_SHADER_SWIZZLE_MASK (0xffu)
/** The offset, in bits, of the nth parameter of a vkd3d-shader swizzle. */
#define VKD3D_SHADER_SWIZZLE_SHIFT(idx) (8u * (idx))

/**
 * A helper macro which returns a vkd3d-shader swizzle with the given
 * components. The components are specified as the suffixes to members of
 * \ref vkd3d_shader_swizzle_component. For example, the swizzle ".xwyy" can be
 * represented as:
 * \code
 * VKD3D_SHADER_SWIZZLE(X, W, Y, Y)
 * \endcode
 */
#define VKD3D_SHADER_SWIZZLE(x, y, z, w) \
        vkd3d_shader_create_swizzle(VKD3D_SHADER_SWIZZLE_ ## x, \
                VKD3D_SHADER_SWIZZLE_ ## y, \
                VKD3D_SHADER_SWIZZLE_ ## z, \
                VKD3D_SHADER_SWIZZLE_ ## w)

/** The identity swizzle ".xyzw". */
#define VKD3D_SHADER_NO_SWIZZLE VKD3D_SHADER_SWIZZLE(X, Y, Z, W)

/** Build a vkd3d-shader swizzle with the given components. */
static inline uint32_t vkd3d_shader_create_swizzle(enum vkd3d_shader_swizzle_component x,
        enum vkd3d_shader_swizzle_component y, enum vkd3d_shader_swizzle_component z,
        enum vkd3d_shader_swizzle_component w)
{
    return ((x & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(0))
            | ((y & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(1))
            | ((z & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(2))
            | ((w & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(3));
}

#ifdef LIBVKD3D_SHADER_SOURCE
# define VKD3D_SHADER_API VKD3D_EXPORT
#else
# define VKD3D_SHADER_API VKD3D_IMPORT
#endif

#ifndef VKD3D_SHADER_NO_PROTOTYPES

/**
 * Returns the current version of this library.
 *
 * \param major Output location for the major version of this library.
 *
 * \param minor Output location for the minor version of this library.
 *
 * \return A human-readable string describing the library name and version. This
 * string is null-terminated and UTF-8 encoded. This may be a pointer to static
 * data in libvkd3d-shader; it should not be freed.
 */
VKD3D_SHADER_API const char *vkd3d_shader_get_version(unsigned int *major, unsigned int *minor);
/**
 * Returns the source types supported, with any target type, by
 * vkd3d_shader_compile(). Future versions of the library may introduce
 * additional source types; callers should ignore unrecognised source types.
 *
 * Use vkd3d_shader_get_supported_target_types() to determine which target types
 * are supported for each source type.
 *
 * \param count Output location for the size, in elements, of the returned
 * array.
 *
 * \return Pointer to an array of source types supported by this version of
 * vkd3d-shader. This array may be a pointer to static data in libvkd3d-shader;
 * it should not be freed.
 */
VKD3D_SHADER_API const enum vkd3d_shader_source_type *vkd3d_shader_get_supported_source_types(unsigned int *count);
/**
 * Returns the target types supported, with the given source type, by
 * vkd3d_shader_compile(). Future versions of the library may introduce
 * additional target types; callers should ignore unrecognised target types.
 *
 * \param source_type Source type for which to enumerate supported target types.
 *
 * \param count Output location for the size, in elements, of the returned
 * array.
 *
 * \return Pointer to an array of target types supported by this version of
 * vkd3d-shader. This array may be a pointer to static data in libvkd3d-shader;
 * it should not be freed.
 */
VKD3D_SHADER_API const enum vkd3d_shader_target_type *vkd3d_shader_get_supported_target_types(
        enum vkd3d_shader_source_type source_type, unsigned int *count);

/**
 * Transform a form of GPU shader source code or byte code into another form of
 * source code or byte code.
 *
 * This version of vkd3d-shader supports the following transformations:
 * - VKD3D_SHADER_SOURCE_DXBC_TPF to VKD3D_SHADER_TARGET_SPIRV_BINARY
 *
 * Supported transformations can also be detected at runtime with the functions
 * vkd3d_shader_get_supported_source_types() and
 * vkd3d_shader_get_supported_target_types().
 *
 * Depending on the source and target types, this function may support the
 * following chained structures:
 * - vkd3d_shader_interface_info
 * - vkd3d_shader_spirv_domain_shader_target_info
 * - vkd3d_shader_spirv_target_info
 * - vkd3d_shader_transform_feedback_info
 *
 * \param compile_info A chained structure containing compilation parameters.
 *
 * \param out A pointer to a vkd3d_shader_code structure in which the compiled
 * code will be stored.
 * \n
 * The compiled shader is allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_shader_code() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the compiler.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * The messages returned can be regulated with the \a log_level member of struct
 * vkd3d_shader_compile_info. Regardless of the requested level, if this
 * parameter is NULL, no compilation messages will be returned.
 * \n
 * If no compilation messages are produced by the compiler, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_compile(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);
/**
 * Free shader messages allocated by another vkd3d-shader function, such as
 * vkd3d_shader_compile().
 *
 * \param messages Messages to free. This pointer is optional and may be NULL,
 * in which case no action will be taken.
 */
VKD3D_SHADER_API void vkd3d_shader_free_messages(char *messages);
/**
 * Free shader code allocated by another vkd3d-shader function, such as
 * vkd3d_shader_compile().
 *
 * This function frees the \ref vkd3d_shader_code.code member, but does not free
 * the structure itself.
 *
 * \param code Code to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_shader_code(struct vkd3d_shader_code *code);

/**
 * Convert a byte code description of a shader root signature to a structural
 * description which can be easily parsed by C code.
 *
 * This function corresponds to
 * ID3D12VersionedRootSignatureDeserializer::GetUnconvertedRootSignatureDesc().
 *
 * This function performs the reverse transformation of
 * vkd3d_shader_serialize_root_signature().
 *
 * This function parses a standalone root signature, and should not be confused
 * with vkd3d_shader_parse_input_signature().
 *
 * \param dxbc Compiled byte code, in DXBC format.
 *
 * \param root_signature Output location in which the decompiled root signature
 * will be stored.
 * \n
 * Members of \a root_signature may be allocated by vkd3d-shader. The signature
 * should be freed with vkd3d_shader_free_root_signature() when no longer
 * needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the compiler.
 * \n
 * This parameter behaves identically to the \a messages parameter of
 * vkd3d_shader_compile().
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_parse_root_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_versioned_root_signature_desc *root_signature, char **messages);
/**
 * Free a structural representation of a shader root signature allocated by
 * vkd3d_shader_convert_root_signature() or vkd3d_shader_parse_root_signature().
 *
 * This function may free members of struct
 * vkd3d_shader_versioned_root_signature_desc, but does not free the structure
 * itself.
 *
 * \param root_signature Signature description to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_root_signature(
        struct vkd3d_shader_versioned_root_signature_desc *root_signature);

/**
 * Convert a structural description of a shader root signature to a byte code
 * format capable of being read by ID3D12Device::CreateRootSignature. The
 * compiled signature is compatible with Microsoft D3D 12.
 *
 * This function corresponds to D3D12SerializeVersionedRootSignature().
 *
 * \param root_signature Description of the root signature.
 *
 * \param dxbc A pointer to a vkd3d_shader_code structure in which the compiled
 * code will be stored.
 * \n
 * The compiled signature is allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_shader_code() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the compiler.
 * \n
 * This parameter behaves identically to the \a messages parameter of
 * vkd3d_shader_compile().
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_serialize_root_signature(
        const struct vkd3d_shader_versioned_root_signature_desc *root_signature,
        struct vkd3d_shader_code *dxbc, char **messages);
/**
 * Convert a structural representation of a root signature to a different
 * version of structural representation.
 *
 * This function corresponds to
 * ID3D12VersionedRootSignatureDeserializer::GetRootSignatureDescAtVersion().
 *
 * \param dst A pointer to a vkd3d_shader_versioned_root_signature_desc
 * structure in which the converted signature will be stored.
 * \n
 * Members of \a dst may be allocated by vkd3d-shader. The signature should be
 * freed with vkd3d_shader_free_root_signature() when no longer needed.
 *
 * \param version The desired version to convert \a src to. This version must
 * not be equal to \a src->version.
 *
 * \param src Input root signature description.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_convert_root_signature(struct vkd3d_shader_versioned_root_signature_desc *dst,
        enum vkd3d_shader_root_signature_version version, const struct vkd3d_shader_versioned_root_signature_desc *src);

/**
 * Parse shader source code or byte code, returning various types of requested
 * information.
 *
 * Currently this function supports the following code types:
 * - VKD3D_SHADER_SOURCE_DXBC_TPF
 *
 * \param compile_info A chained structure containing scan parameters.
 * \n
 * The DXBC_TPF scanner supports the following chained structures:
 * - vkd3d_shader_scan_descriptor_info
 * \n
 * Although the \a compile_info parameter is read-only, chained structures
 * passed to this function need not be, and may serve as output parameters,
 * depending on their structure type.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the compiler.
 * \n
 * This parameter behaves identically to the \a messages parameter of
 * vkd3d_shader_compile().
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_scan(const struct vkd3d_shader_compile_info *compile_info, char **messages);
/**
 * Free members of struct vkd3d_shader_scan_descriptor_info() allocated by
 * vkd3d_shader_scan().
 *
 * This function may free members of vkd3d_shader_scan_descriptor_info, but
 * does not free the structure itself.
 *
 * \param scan_descriptor_info Descriptor information to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_scan_descriptor_info(
        struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info);

/**
 * Read the input signature of a compiled shader, returning a structural
 * description which can be easily parsed by C code.
 *
 * This function parses a compiled shader. To parse a standalone root signature,
 * use vkd3d_shader_parse_root_signature().
 *
 * \param dxbc Compiled byte code, in DXBC format.
 *
 * \param signature Output location in which the parsed root signature will be
 * stored.
 * \n
 * Members of \a signature may be allocated by vkd3d-shader. The signature
 * should be freed with vkd3d_shader_free_shader_signature() when no longer
 * needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the compiler.
 * \n
 * This parameter behaves identically to the \a messages parameter of
 * vkd3d_shader_compile().
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_parse_input_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_signature *signature, char **messages);
/**
 * Find a single element of a parsed input signature.
 *
 * \param signature The parsed input signature. This structure is normally
 * populated by vkd3d_shader_parse_input_signature().
 *
 * \param semantic_name Semantic name of the desired element. This function
 * performs a case-insensitive comparison with respect to the ASCII plane.
 *
 * \param semantic_index Semantic index of the desired element.
 *
 * \param stream_index Geometry shader stream index of the desired element. If
 * the signature is not a geometry shader output signature, this parameter must
 * be set to 0.
 *
 * \return A description of the element matching the requested parameters, or
 * NULL if no such element was found. If not NULL, the return value points into
 * the \a signature parameter and should not be explicitly freed.
 */
VKD3D_SHADER_API struct vkd3d_shader_signature_element *vkd3d_shader_find_signature_element(
        const struct vkd3d_shader_signature *signature, const char *semantic_name,
        unsigned int semantic_index, unsigned int stream_index);
/**
 * Free a structural representation of a shader input signature allocated by
 * vkd3d_shader_parse_input_signature().
 *
 * This function may free members of struct vkd3d_shader_signature, but does not
 * free the structure itself.
 *
 * \param signature Signature description to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_shader_signature(struct vkd3d_shader_signature *signature);

/* 1.3 */

/**
 * Preprocess the given source code.
 *
 * This function supports the following chained structures:
 * - vkd3d_shader_preprocess_info
 *
 * \param compile_info A chained structure containing compilation parameters.
 *
 * \param out A pointer to a vkd3d_shader_code structure in which the
 * preprocessed code will be stored.
 * \n
 * The preprocessed shader is allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_shader_code() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the compiler.
 * \n
 * This parameter behaves identically to the \a messages parameter of
 * vkd3d_shader_compile().
 *
 * \return A member of \ref vkd3d_result.
 *
 * \since 1.3
 */
VKD3D_SHADER_API int vkd3d_shader_preprocess(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);

/**
 * Set a callback to be called when vkd3d-shader outputs debug logging.
 *
 * If NULL, or if this function has not been called, libvkd3d-shader will print
 * all enabled log output to stderr.
 *
 * \param callback Callback function to set.
 *
 * \since 1.4
 */
VKD3D_SHADER_API void vkd3d_shader_set_log_callback(PFN_vkd3d_log callback);

#endif  /* VKD3D_SHADER_NO_PROTOTYPES */

/** Type of vkd3d_shader_get_version(). */
typedef const char *(*PFN_vkd3d_shader_get_version)(unsigned int *major, unsigned int *minor);
/** Type of vkd3d_shader_get_supported_source_types(). */
typedef const enum vkd3d_shader_source_type *(*PFN_vkd3d_shader_get_supported_source_types)(unsigned int *count);
/** Type of vkd3d_shader_get_supported_target_types(). */
typedef const enum vkd3d_shader_target_type *(*PFN_vkd3d_shader_get_supported_target_types)(
        enum vkd3d_shader_source_type source_type, unsigned int *count);

/** Type of vkd3d_shader_compile(). */
typedef int (*PFN_vkd3d_shader_compile)(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);
/** Type of vkd3d_shader_free_messages(). */
typedef void (*PFN_vkd3d_shader_free_messages)(char *messages);
/** Type of vkd3d_shader_free_shader_code(). */
typedef void (*PFN_vkd3d_shader_free_shader_code)(struct vkd3d_shader_code *code);

/** Type of vkd3d_shader_parse_root_signature(). */
typedef int (*PFN_vkd3d_shader_parse_root_signature)(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_versioned_root_signature_desc *root_signature, char **messages);
/** Type of vkd3d_shader_free_root_signature(). */
typedef void (*PFN_vkd3d_shader_free_root_signature)(struct vkd3d_shader_versioned_root_signature_desc *root_signature);

/** Type of vkd3d_shader_serialize_root_signature(). */
typedef int (*PFN_vkd3d_shader_serialize_root_signature)(
        const struct vkd3d_shader_versioned_root_signature_desc *root_signature,
        struct vkd3d_shader_code *dxbc, char **messages);

/** Type of vkd3d_shader_convert_root_signature(). */
typedef int (*PFN_vkd3d_shader_convert_root_signature)(struct vkd3d_shader_versioned_root_signature_desc *dst,
        enum vkd3d_shader_root_signature_version version, const struct vkd3d_shader_versioned_root_signature_desc *src);

/** Type of vkd3d_shader_scan(). */
typedef int (*PFN_vkd3d_shader_scan)(const struct vkd3d_shader_compile_info *compile_info, char **messages);
/** Type of vkd3d_shader_free_scan_descriptor_info(). */
typedef void (*PFN_vkd3d_shader_free_scan_descriptor_info)(
        struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info);

/** Type of vkd3d_shader_parse_input_signature(). */
typedef int (*PFN_vkd3d_shader_parse_input_signature)(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_signature *signature, char **messages);
/** Type of vkd3d_shader_find_signature_element(). */
typedef struct vkd3d_shader_signature_element * (*PFN_vkd3d_shader_find_signature_element)(
        const struct vkd3d_shader_signature *signature, const char *semantic_name,
        unsigned int semantic_index, unsigned int stream_index);
/** Type of vkd3d_shader_free_shader_signature(). */
typedef void (*PFN_vkd3d_shader_free_shader_signature)(struct vkd3d_shader_signature *signature);

/** Type of vkd3d_shader_preprocess(). \since 1.3 */
typedef void (*PFN_vkd3d_shader_preprocess)(struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);

/** Type of vkd3d_shader_set_log_callback(). \since 1.4 */
typedef void (*PFN_vkd3d_shader_set_log_callback)(PFN_vkd3d_log callback);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __VKD3D_SHADER_H */
