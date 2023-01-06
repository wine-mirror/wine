/*
 * Direct3D wine OpenGL include file
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2004 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2007 Roderick Colenbrander
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

#ifndef __WINE_WINED3D_GL_H
#define __WINE_WINED3D_GL_H

#define WINE_GLAPI __stdcall

#include <stdbool.h>
#include <stdint.h>

#include "wine/wgl.h"
#include "wine/wgl_driver.h"

#define GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI 0x8837  /* not in the gl spec */

/* OpenGL extensions. */
enum wined3d_gl_extension
{
    WINED3D_GL_EXT_NONE,

    /* APPLE */
    APPLE_FENCE,
    APPLE_FLOAT_PIXELS,
    APPLE_FLUSH_BUFFER_RANGE,
    APPLE_FLUSH_RENDER,
    APPLE_RGB_422,
    APPLE_YCBCR_422,
    /* ARB */
    ARB_BASE_INSTANCE,
    ARB_BLEND_FUNC_EXTENDED,
    ARB_BUFFER_STORAGE,
    ARB_CLEAR_BUFFER_OBJECT,
    ARB_CLEAR_TEXTURE,
    ARB_CLIP_CONTROL,
    ARB_COLOR_BUFFER_FLOAT,
    ARB_COMPUTE_SHADER,
    ARB_CONSERVATIVE_DEPTH,
    ARB_COPY_BUFFER,
    ARB_COPY_IMAGE,
    ARB_CULL_DISTANCE,
    ARB_DEBUG_OUTPUT,
    ARB_DEPTH_BUFFER_FLOAT,
    ARB_DEPTH_CLAMP,
    ARB_DEPTH_TEXTURE,
    ARB_DERIVATIVE_CONTROL,
    ARB_DRAW_BUFFERS,
    ARB_DRAW_BUFFERS_BLEND,
    ARB_DRAW_ELEMENTS_BASE_VERTEX,
    ARB_DRAW_INDIRECT,
    ARB_DRAW_INSTANCED,
    ARB_ES2_COMPATIBILITY,
    ARB_ES3_COMPATIBILITY,
    ARB_EXPLICIT_ATTRIB_LOCATION,
    ARB_FRAGMENT_COORD_CONVENTIONS,
    ARB_FRAGMENT_LAYER_VIEWPORT,
    ARB_FRAGMENT_PROGRAM,
    ARB_FRAGMENT_SHADER,
    ARB_FRAMEBUFFER_NO_ATTACHMENTS,
    ARB_FRAMEBUFFER_OBJECT,
    ARB_FRAMEBUFFER_SRGB,
    ARB_GEOMETRY_SHADER4,
    ARB_GPU_SHADER5,
    ARB_HALF_FLOAT_PIXEL,
    ARB_HALF_FLOAT_VERTEX,
    ARB_INSTANCED_ARRAYS,
    ARB_INTERNALFORMAT_QUERY,
    ARB_INTERNALFORMAT_QUERY2,
    ARB_MAP_BUFFER_ALIGNMENT,
    ARB_MAP_BUFFER_RANGE,
    ARB_MULTISAMPLE,
    ARB_MULTITEXTURE,
    ARB_OCCLUSION_QUERY,
    ARB_PIPELINE_STATISTICS_QUERY,
    ARB_PIXEL_BUFFER_OBJECT,
    ARB_POINT_PARAMETERS,
    ARB_POINT_SPRITE,
    ARB_POLYGON_OFFSET_CLAMP,
    ARB_PROVOKING_VERTEX,
    ARB_QUERY_BUFFER_OBJECT,
    ARB_SAMPLE_SHADING,
    ARB_SAMPLER_OBJECTS,
    ARB_SEAMLESS_CUBE_MAP,
    ARB_SHADER_ATOMIC_COUNTERS,
    ARB_SHADER_VIEWPORT_LAYER_ARRAY,
    ARB_SHADER_BIT_ENCODING,
    ARB_SHADER_IMAGE_LOAD_STORE,
    ARB_SHADER_IMAGE_SIZE,
    ARB_SHADER_STORAGE_BUFFER_OBJECT,
    ARB_SHADER_TEXTURE_IMAGE_SAMPLES,
    ARB_SHADER_TEXTURE_LOD,
    ARB_SHADING_LANGUAGE_100,
    ARB_SHADING_LANGUAGE_420PACK,
    ARB_SHADING_LANGUAGE_PACKING,
    ARB_SHADOW,
    ARB_STENCIL_TEXTURING,
    ARB_SYNC,
    ARB_TESSELLATION_SHADER,
    ARB_TEXTURE_BORDER_CLAMP,
    ARB_TEXTURE_BUFFER_OBJECT,
    ARB_TEXTURE_BUFFER_RANGE,
    ARB_TEXTURE_COMPRESSION,
    ARB_TEXTURE_COMPRESSION_BPTC,
    ARB_TEXTURE_COMPRESSION_RGTC,
    ARB_TEXTURE_CUBE_MAP,
    ARB_TEXTURE_CUBE_MAP_ARRAY,
    ARB_TEXTURE_ENV_COMBINE,
    ARB_TEXTURE_ENV_DOT3,
    ARB_TEXTURE_FILTER_ANISOTROPIC,
    ARB_TEXTURE_FLOAT,
    ARB_TEXTURE_GATHER,
    ARB_TEXTURE_MIRRORED_REPEAT,
    ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE,
    ARB_TEXTURE_MULTISAMPLE,
    ARB_TEXTURE_NON_POWER_OF_TWO,
    ARB_TEXTURE_QUERY_LEVELS,
    ARB_TEXTURE_RECTANGLE,
    ARB_TEXTURE_RG,
    ARB_TEXTURE_RGB10_A2UI,
    ARB_TEXTURE_STORAGE,
    ARB_TEXTURE_STORAGE_MULTISAMPLE,
    ARB_TEXTURE_SWIZZLE,
    ARB_TEXTURE_VIEW,
    ARB_TIMER_QUERY,
    ARB_TRANSFORM_FEEDBACK2,
    ARB_TRANSFORM_FEEDBACK3,
    ARB_UNIFORM_BUFFER_OBJECT,
    ARB_VERTEX_ARRAY_BGRA,
    ARB_VERTEX_BUFFER_OBJECT,
    ARB_VERTEX_PROGRAM,
    ARB_VERTEX_SHADER,
    ARB_VERTEX_TYPE_10F_11F_11F_REV,
    ARB_VERTEX_TYPE_2_10_10_10_REV,
    ARB_VIEWPORT_ARRAY,
    ARB_TEXTURE_BARRIER,
    /* ATI */
    ATI_FRAGMENT_SHADER,
    ATI_SEPARATE_STENCIL,
    ATI_TEXTURE_COMPRESSION_3DC,
    ATI_TEXTURE_ENV_COMBINE3,
    ATI_TEXTURE_MIRROR_ONCE,
    /* EXT */
    EXT_BLEND_COLOR,
    EXT_BLEND_EQUATION_SEPARATE,
    EXT_BLEND_FUNC_SEPARATE,
    EXT_BLEND_MINMAX,
    EXT_BLEND_SUBTRACT,
    EXT_DEPTH_BOUNDS_TEST,
    EXT_DRAW_BUFFERS2,
    EXT_FOG_COORD,
    EXT_FRAMEBUFFER_BLIT,
    EXT_FRAMEBUFFER_MULTISAMPLE,
    EXT_FRAMEBUFFER_MULTISAMPLE_BLIT_SCALED,
    EXT_FRAMEBUFFER_OBJECT,
    EXT_GPU_PROGRAM_PARAMETERS,
    EXT_GPU_SHADER4,
    EXT_MEMORY_OBJECT,
    EXT_PACKED_DEPTH_STENCIL,
    EXT_PACKED_FLOAT,
    EXT_POINT_PARAMETERS,
    EXT_PROVOKING_VERTEX,
    EXT_SECONDARY_COLOR,
    EXT_STENCIL_TWO_SIDE,
    EXT_STENCIL_WRAP,
    EXT_TEXTURE3D,
    EXT_TEXTURE_ARRAY,
    EXT_TEXTURE_COMPRESSION_RGTC,
    EXT_TEXTURE_COMPRESSION_S3TC,
    EXT_TEXTURE_ENV_COMBINE,
    EXT_TEXTURE_ENV_DOT3,
    EXT_TEXTURE_INTEGER,
    EXT_TEXTURE_LOD_BIAS,
    EXT_TEXTURE_MIRROR_CLAMP,
    EXT_TEXTURE_SHADOW_LOD,
    EXT_TEXTURE_SHARED_EXPONENT,
    EXT_TEXTURE_SNORM,
    EXT_TEXTURE_SRGB,
    EXT_TEXTURE_SRGB_DECODE,
    /* NVIDIA */
    NV_FENCE,
    NV_FOG_DISTANCE,
    NV_FRAGMENT_PROGRAM,
    NV_FRAGMENT_PROGRAM2,
    NV_FRAGMENT_PROGRAM_OPTION,
    NV_HALF_FLOAT,
    NV_LIGHT_MAX_EXPONENT,
    NV_POINT_SPRITE,
    NV_REGISTER_COMBINERS,
    NV_REGISTER_COMBINERS2,
    NV_TEXGEN_REFLECTION,
    NV_TEXTURE_ENV_COMBINE4,
    NV_TEXTURE_SHADER,
    NV_TEXTURE_SHADER2,
    NV_VERTEX_PROGRAM,
    NV_VERTEX_PROGRAM1_1,
    NV_VERTEX_PROGRAM2,
    NV_VERTEX_PROGRAM2_OPTION,
    NV_VERTEX_PROGRAM3,
    NV_TEXTURE_BARRIER,
    /* WGL extensions */
    WGL_ARB_PIXEL_FORMAT,
    WGL_EXT_SWAP_CONTROL,
    WGL_WINE_PIXEL_FORMAT_PASSTHROUGH,
    WGL_WINE_QUERY_RENDERER,
    /* Internally used */
    WINED3D_GL_BLEND_EQUATION,
    WINED3D_GL_LEGACY_CONTEXT,
    WINED3D_GL_NORMALIZED_TEXRECT,
    WINED3D_GL_PRIMITIVE_QUERY,
    WINED3D_GL_VERSION_2_0,
    WINED3D_GL_VERSION_3_2,
    WINED3D_GLSL_130,

    WINED3D_GL_EXT_COUNT,
};

struct wined3d_fbo_ops
{
    GLboolean (WINE_GLAPI *glIsRenderbuffer)(GLuint renderbuffer);
    void (WINE_GLAPI *glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (WINE_GLAPI *glDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
    void (WINE_GLAPI *glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
    void (WINE_GLAPI *glRenderbufferStorage)(GLenum target, GLenum internalformat,
            GLsizei width, GLsizei height);
    void (WINE_GLAPI *glRenderbufferStorageMultisample)(GLenum target, GLsizei samples,
            GLenum internalformat, GLsizei width, GLsizei height);
    void (WINE_GLAPI *glGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
    GLboolean (WINE_GLAPI *glIsFramebuffer)(GLuint framebuffer);
    void (WINE_GLAPI *glBindFramebuffer)(GLenum target, GLuint framebuffer);
    void (WINE_GLAPI *glDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
    void (WINE_GLAPI *glGenFramebuffers)(GLsizei n, GLuint *framebuffers);
    GLenum (WINE_GLAPI *glCheckFramebufferStatus)(GLenum target);
    void (WINE_GLAPI *glFramebufferTexture)(GLenum target, GLenum attachment,
            GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture1D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture2D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level);
    void (WINE_GLAPI *glFramebufferTexture3D)(GLenum target, GLenum attachment,
            GLenum textarget, GLuint texture, GLint level, GLint layer);
    void (WINE_GLAPI *glFramebufferTextureLayer)(GLenum target, GLenum attachment,
            GLuint texture, GLint level, GLint layer);
    void (WINE_GLAPI *glFramebufferRenderbuffer)(GLenum target, GLenum attachment,
            GLenum renderbuffertarget, GLuint renderbuffer);
    void (WINE_GLAPI *glGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment,
            GLenum pname, GLint *params);
    void (WINE_GLAPI *glBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    void (WINE_GLAPI *glGenerateMipmap)(GLenum target);
};

struct wined3d_gl_limits
{
    unsigned int buffers;
    unsigned int lights;
    unsigned int textures;
    unsigned int texture_coords;
    unsigned int uniform_blocks[WINED3D_SHADER_TYPE_COUNT];
    unsigned int samplers[WINED3D_SHADER_TYPE_COUNT];
    unsigned int graphics_samplers;
    unsigned int combined_samplers;
    unsigned int general_combiners;
    unsigned int user_clip_distances;
    unsigned int texture_size;
    unsigned int texture3d_size;
    unsigned int anisotropy;
    float shininess;
    unsigned int samples;
    unsigned int vertex_attribs;

    unsigned int texture_buffer_offset_alignment;

    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned int viewport_subpixel_bits;

    unsigned int glsl_varyings;
    unsigned int glsl_vs_float_constants;
    unsigned int glsl_ps_float_constants;

    unsigned int arb_vs_float_constants;
    unsigned int arb_vs_native_constants;
    unsigned int arb_vs_instructions;
    unsigned int arb_vs_temps;
    unsigned int arb_ps_float_constants;
    unsigned int arb_ps_local_constants;
    unsigned int arb_ps_native_constants;
    unsigned int arb_ps_instructions;
    unsigned int arb_ps_temps;
};

void wined3d_gl_limits_get_texture_unit_range(const struct wined3d_gl_limits *gl_limits,
        enum wined3d_shader_type shader_type, unsigned int *base, unsigned int *count);
void wined3d_gl_limits_get_uniform_block_range(const struct wined3d_gl_limits *gl_limits,
        enum wined3d_shader_type shader_type, unsigned int *base, unsigned int *count);

#define WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT       0x00000001
#define WINED3D_QUIRK_GLSL_CLIP_VARYING         0x00000004
#define WINED3D_QUIRK_ALLOWS_SPECULAR_ALPHA     0x00000008
#define WINED3D_QUIRK_NV_CLIP_BROKEN            0x00000010
#define WINED3D_QUIRK_FBO_TEX_UPDATE            0x00000020
#define WINED3D_QUIRK_BROKEN_RGBA16             0x00000040
#define WINED3D_QUIRK_INFO_LOG_SPAM             0x00000080
#define WINED3D_QUIRK_LIMITED_TEX_FILTERING     0x00000100
#define WINED3D_QUIRK_BROKEN_ARB_FOG            0x00000200
#define WINED3D_QUIRK_NO_INDEPENDENT_BIT_DEPTHS 0x00000400

typedef void (WINE_GLAPI *wined3d_ffp_attrib_func)(const void *data);
typedef void (WINE_GLAPI *wined3d_ffp_texcoord_func)(GLenum unit, const void *data);
typedef void (WINE_GLAPI *wined3d_generic_attrib_func)(GLuint idx, const void *data);

struct wined3d_ffp_attrib_ops
{
    wined3d_ffp_attrib_func position[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_attrib_func diffuse[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_attrib_func specular[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_attrib_func normal[WINED3D_FFP_EMIT_COUNT];
    wined3d_ffp_texcoord_func texcoord[WINED3D_FFP_EMIT_COUNT];
    wined3d_generic_attrib_func generic[WINED3D_FFP_EMIT_COUNT];
};

struct wined3d_gl_info
{
    unsigned int selected_gl_version;
    unsigned int glsl_version;
    struct wined3d_gl_limits limits;
    unsigned int reserved_glsl_constants, reserved_arb_constants;
    uint32_t quirks;
    BOOL supported[WINED3D_GL_EXT_COUNT];
    GLint wrap_lookup[WINED3D_TADDRESS_MIRROR_ONCE - WINED3D_TADDRESS_WRAP + 1];
    float filling_convention_offset;

    HGLRC (WINAPI *p_wglCreateContextAttribsARB)(HDC dc, HGLRC share, const GLint *attribs);
    struct wined3d_ffp_attrib_ops ffp_attrib_ops;
    struct opengl_funcs gl_ops;
    struct wined3d_fbo_ops fbo_ops;

    void (WINE_GLAPI *p_glDisableWINE)(GLenum cap);
    void (WINE_GLAPI *p_glEnableWINE)(GLenum cap);
};

void install_gl_compat_wrapper(struct wined3d_gl_info *gl_info, enum wined3d_gl_extension ext);
void print_glsl_info_log(const struct wined3d_gl_info *gl_info, GLuint id, BOOL program);
void shader_glsl_validate_link(const struct wined3d_gl_info *gl_info, GLuint program);
GLenum wined3d_buffer_gl_binding_from_bind_flags(const struct wined3d_gl_info *gl_info, uint32_t bind_flags);
void wined3d_check_gl_call(const struct wined3d_gl_info *gl_info,
        const char *file, unsigned int line, const char *name);

struct min_lookup
{
    GLenum mip[WINED3D_TEXF_LINEAR + 1];
};

extern const struct min_lookup minMipLookup[WINED3D_TEXF_LINEAR + 1];
extern const GLenum magLookup[WINED3D_TEXF_LINEAR + 1];

static inline GLenum wined3d_gl_mag_filter(enum wined3d_texture_filter_type mag_filter)
{
    return magLookup[mag_filter];
}

static inline GLenum wined3d_gl_min_mip_filter(enum wined3d_texture_filter_type min_filter,
        enum wined3d_texture_filter_type mip_filter)
{
    return minMipLookup[min_filter].mip[mip_filter];
}

GLenum wined3d_gl_compare_func(enum wined3d_cmp_func f);

const char *debug_fboattachment(GLenum attachment);
const char *debug_fbostatus(GLenum status);
const char *debug_glerror(GLenum error);

static inline bool wined3d_fence_supported(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[ARB_SYNC] || gl_info->supported[NV_FENCE] || gl_info->supported[APPLE_FENCE];
}

/* Checking of API calls */
/* --------------------- */
#ifndef WINE_NO_DEBUG_MSGS
#define checkGLcall(A)                                              \
    do                                                              \
    {                                                               \
        if (__WINE_IS_DEBUG_ON(_ERR, &__wine_dbch_d3d)              \
                && !gl_info->supported[ARB_DEBUG_OUTPUT])           \
            wined3d_check_gl_call(gl_info, __FILE__, __LINE__, A);  \
    } while(0)
#else
#define checkGLcall(A) do {} while(0)
#endif

struct wined3d_bo_gl
{
    struct wined3d_bo b;

    GLuint id;

    struct wined3d_allocator_block *memory;

    GLsizeiptr size;
    GLenum binding;
    GLenum usage;

    GLbitfield flags;
    uint64_t command_fence_id;
};

static inline struct wined3d_bo_gl *wined3d_bo_gl(struct wined3d_bo *bo)
{
    return CONTAINING_RECORD(bo, struct wined3d_bo_gl, b);
}

static inline GLuint wined3d_bo_gl_id(struct wined3d_bo *bo)
{
    return bo ? wined3d_bo_gl(bo)->id : 0;
}

union wined3d_gl_fence_object
{
    GLuint id;
    GLsync sync;
};

enum wined3d_fence_result
{
    WINED3D_FENCE_OK,
    WINED3D_FENCE_WAITING,
    WINED3D_FENCE_NOT_STARTED,
    WINED3D_FENCE_WRONG_THREAD,
    WINED3D_FENCE_ERROR,
};

struct wined3d_fence
{
    struct list entry;
    union wined3d_gl_fence_object object;
    struct wined3d_context_gl *context_gl;
};

HRESULT wined3d_fence_create(struct wined3d_device *device, struct wined3d_fence **fence);
void wined3d_fence_destroy(struct wined3d_fence *fence);
void wined3d_fence_issue(struct wined3d_fence *fence, struct wined3d_device *device);
enum wined3d_fence_result wined3d_fence_wait(const struct wined3d_fence *fence, struct wined3d_device *device);
enum wined3d_fence_result wined3d_fence_test(const struct wined3d_fence *fence,
        struct wined3d_device *device, uint32_t flags);

#endif /* __WINE_WINED3D_GL */
