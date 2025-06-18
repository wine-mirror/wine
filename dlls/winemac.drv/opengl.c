/*
 * Mac driver OpenGL support
 *
 * Copyright 2012 Alexandre Julliard
 * Copyright 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "macdrv.h"

#include "winuser.h"
#include "winternl.h"
#include "winnt.h"
#include "wine/debug.h"
#include "wine/opengl_driver.h"

#define GL_SILENCE_DEPRECATION
#define __gl_h_
#define __gltypes_h_
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#include <OpenGL/CGLRenderers.h>
#include <dlfcn.h>

WINE_DEFAULT_DEBUG_CHANNEL(wgl);


struct gl_info {
    char *glExtensions;

    char wglExtensions[4096];

    GLint max_viewport_dims[2];

    unsigned int max_major, max_minor;
};

static struct gl_info gl_info;


struct macdrv_context
{
    struct list             entry;
    int                     format;
    GLint                   renderer_id;
    macdrv_opengl_context   context;
    CGLContextObj           cglcontext;
    HWND                    draw_hwnd;
    macdrv_view             draw_view;
    RECT                    draw_rect;
    RECT                    draw_rect_win_dpi;
    CGLPBufferObj           draw_pbuffer;
    GLenum                  draw_pbuffer_face;
    GLint                   draw_pbuffer_level;
    macdrv_view             read_view;
    RECT                    read_rect;
    CGLPBufferObj           read_pbuffer;
    int                     swap_interval;
    LONG                    view_moved;
    unsigned int            last_flush_time;
    UINT                    major;
};

static struct list context_list = LIST_INIT(context_list);
static pthread_mutex_t context_mutex = PTHREAD_MUTEX_INITIALIZER;

static CFMutableDictionaryRef dc_pbuffers;
static pthread_mutex_t dc_pbuffers_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *opengl_handle;
static const struct opengl_funcs *funcs;
static const struct opengl_driver_funcs macdrv_driver_funcs;

static void (*pglCopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y,
                                 GLsizei width);
static void (*pglCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
static void (*pglFlushRenderAPPLE)(void);
static const GLubyte *(*pglGetString)(GLenum name);
static PFN_glGetIntegerv pglGetIntegerv;
static void (*pglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height,
                             GLenum format, GLenum type, void *pixels);
static void (*pglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);


struct color_mode {
    GLint   mode;
    int     bits_per_pixel;
    GLint   color_bits; /* including alpha_bits */
    int     red_bits, red_shift;
    int     green_bits, green_shift;
    int     blue_bits, blue_shift;
    GLint   alpha_bits, alpha_shift;
    BOOL    is_float;
    int     color_ordering;
};

/* The value of "color_ordering" is somewhat arbitrary.  It incorporates some
   observations of the behavior of Windows systems, but also subjective judgments
   about what color formats are more "normal" than others.

   On at least some Windows systems, integer color formats are listed before
   floating-point formats.  Within the integer formats, higher color bits were
   usually listed before lower color bits, while for floating-point formats it
   was the reverse.  However, that leads D3D to select 64-bit integer formats in
   preference to 32-bit formats when the latter would be sufficient.  It seems
   that a 32-bit format is much more likely to be normally used in that case.

   Also, there are certain odd color formats supported on the Mac which seem like
   they would be less appropriate than more common ones.  For instance, the color
   formats with alpha in a separate byte (e.g. kCGLRGB888A8Bit with R8G8B8 in one
   32-bit value and A8 in a separate 8-bit value) and the formats with 10-bit RGB
   components.

   For two color formats which differ only in whether or not they have alpha bits,
   we use the same ordering.  pixel_format_comparator() gives alpha bits a
   different weight than color formats.
 */
static const struct color_mode color_modes[] = {
    { kCGLRGB444Bit,        16,     12,     4,  8,      4,  4,      4,  0,      0,  0,  FALSE,  5 },
    { kCGLARGB4444Bit,      16,     16,     4,  8,      4,  4,      4,  0,      4,  12, FALSE,  5 },
    { kCGLRGB444A8Bit,      24,     20,     4,  8,      4,  4,      4,  0,      8,  16, FALSE,  10 },
    { kCGLRGB555Bit,        16,     15,     5,  10,     5,  5,      5,  0,      0,  0,  FALSE,  4 },
    { kCGLARGB1555Bit,      16,     16,     5,  10,     5,  5,      5,  0,      1,  15, FALSE,  4 },
    { kCGLRGB555A8Bit,      24,     23,     5,  10,     5,  5,      5,  0,      8,  16, FALSE,  9 },
    { kCGLRGB565Bit,        16,     16,     5,  11,     6,  5,      5,  0,      0,  0,  FALSE,  3 },
    { kCGLRGB565A8Bit,      24,     24,     5,  11,     6,  5,      5,  0,      8,  16, FALSE,  8 },
    { kCGLRGB888Bit,        32,     24,     8,  16,     8,  8,      8,  0,      0,  0,  FALSE,  0 },
    { kCGLARGB8888Bit,      32,     32,     8,  16,     8,  8,      8,  0,      8,  24, FALSE,  0 },
    { kCGLRGB888A8Bit,      40,     32,     8,  16,     8,  8,      8,  0,      8,  32, FALSE,  7 },
    { kCGLRGB101010Bit,     32,     30,     10, 20,     10, 10,     10, 0,      0,  0,  FALSE,  6 },
    { kCGLARGB2101010Bit,   32,     32,     10, 20,     10, 10,     10, 0,      2,  30, FALSE,  6 },
    { kCGLRGB101010_A8Bit,  40,     38,     10, 20,     10, 10,     10, 0,      8,  32, FALSE,  11 },
    { kCGLRGB121212Bit,     48,     36,     12, 24,     12, 12,     12, 0,      0,  0,  FALSE,  2 },
    { kCGLARGB12121212Bit,  48,     48,     12, 24,     12, 12,     12, 0,      12, 36, FALSE,  2 },
    { kCGLRGB161616Bit,     64,     48,     16, 48,     16, 32,     16, 16,     0,  0,  FALSE,  1 },
    { kCGLRGBA16161616Bit,  64,     64,     16, 48,     16, 32,     16, 16,     16, 0,  FALSE,  1 },
    { kCGLRGBFloat64Bit,    64,     48,     16, 32,     16, 16,     16, 0,      0,  0,  TRUE,   12 },
    { kCGLRGBAFloat64Bit,   64,     64,     16, 48,     16, 32,     16, 16,     16, 0,  TRUE,   12 },
    { kCGLRGBFloat128Bit,   128,    96,     32, 96,     32, 64,     32, 32,     0,  0,  TRUE,   13 },
    { kCGLRGBAFloat128Bit,  128,    128,    32, 96,     32, 64,     32, 32,     32, 0,  TRUE,   13 },
    { kCGLRGBFloat256Bit,   256,    192,    64, 192,    64, 128,    64, 64,     0,  0,  TRUE,   14 },
    { kCGLRGBAFloat256Bit,  256,    256,    64, 192,    64, 128,    64, 64,     64, 0,  TRUE,   15 },
};


static const struct {
    GLint   mode;
    int     bits;
} depth_stencil_modes[] = {
    { kCGL0Bit,     0 },
    { kCGL1Bit,     1 },
    { kCGL2Bit,     2 },
    { kCGL3Bit,     3 },
    { kCGL4Bit,     4 },
    { kCGL5Bit,     5 },
    { kCGL6Bit,     6 },
    { kCGL8Bit,     8 },
    { kCGL10Bit,    10 },
    { kCGL12Bit,    12 },
    { kCGL16Bit,    16 },
    { kCGL24Bit,    24 },
    { kCGL32Bit,    32 },
    { kCGL48Bit,    48 },
    { kCGL64Bit,    64 },
    { kCGL96Bit,    96 },
    { kCGL128Bit,   128 },
};


typedef struct {
    GLint   renderer_id;
    GLint   buffer_modes;
    GLint   color_modes;
    GLint   accum_modes;
    GLint   depth_modes;
    GLint   stencil_modes;
    GLint   max_aux_buffers;
    GLint   max_sample_buffers;
    GLint   max_samples;
    BOOL    offscreen;
    BOOL    accelerated;
    BOOL    backing_store;
    BOOL    window;
    BOOL    online;
} renderer_properties;


typedef struct {
    unsigned int window:1;
    unsigned int pbuffer:1;
    unsigned int accelerated:1;
    unsigned int color_mode:5; /* index into color_modes table */
    unsigned int aux_buffers:3;
    unsigned int depth_bits:8;
    unsigned int stencil_bits:8;
    unsigned int accum_mode:5; /* 1 + index into color_modes table (0 means no accum buffer) */
    unsigned int double_buffer:1;
    unsigned int stereo:1;
    unsigned int sample_buffers:1;
    unsigned int samples:5;
    unsigned int backing_store:1;
} pixel_format;


typedef union
{
    pixel_format    format;
    UInt64          code;
} pixel_format_or_code;
C_ASSERT(sizeof(((pixel_format_or_code*)0)->format) <= sizeof(((pixel_format_or_code*)0)->code));


static pixel_format *pixel_formats;
static int nb_formats, nb_displayable_formats;


static const char* debugstr_attrib(int attrib, int value)
{
    static const struct {
        int attrib;
        const char *name;
    } attrib_names[] = {
#define ATTRIB(a) { a, #a }
        ATTRIB(WGL_ACCELERATION_ARB),
        ATTRIB(WGL_ACCUM_ALPHA_BITS_ARB),
        ATTRIB(WGL_ACCUM_BITS_ARB),
        ATTRIB(WGL_ACCUM_BLUE_BITS_ARB),
        ATTRIB(WGL_ACCUM_GREEN_BITS_ARB),
        ATTRIB(WGL_ACCUM_RED_BITS_ARB),
        ATTRIB(WGL_ALPHA_BITS_ARB),
        ATTRIB(WGL_ALPHA_SHIFT_ARB),
        ATTRIB(WGL_AUX_BUFFERS_ARB),
        ATTRIB(WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV),
        ATTRIB(WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV),
        ATTRIB(WGL_BIND_TO_TEXTURE_RGB_ARB),
        ATTRIB(WGL_BIND_TO_TEXTURE_RGBA_ARB),
        ATTRIB(WGL_BLUE_BITS_ARB),
        ATTRIB(WGL_BLUE_SHIFT_ARB),
        ATTRIB(WGL_COLOR_BITS_ARB),
        ATTRIB(WGL_DEPTH_BITS_ARB),
        ATTRIB(WGL_DOUBLE_BUFFER_ARB),
        ATTRIB(WGL_DRAW_TO_BITMAP_ARB),
        ATTRIB(WGL_DRAW_TO_PBUFFER_ARB),
        ATTRIB(WGL_DRAW_TO_WINDOW_ARB),
        ATTRIB(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB),
        ATTRIB(WGL_GREEN_BITS_ARB),
        ATTRIB(WGL_GREEN_SHIFT_ARB),
        ATTRIB(WGL_NEED_PALETTE_ARB),
        ATTRIB(WGL_NEED_SYSTEM_PALETTE_ARB),
        ATTRIB(WGL_NUMBER_OVERLAYS_ARB),
        ATTRIB(WGL_NUMBER_PIXEL_FORMATS_ARB),
        ATTRIB(WGL_NUMBER_UNDERLAYS_ARB),
        ATTRIB(WGL_PIXEL_TYPE_ARB),
        ATTRIB(WGL_RED_BITS_ARB),
        ATTRIB(WGL_RED_SHIFT_ARB),
        ATTRIB(WGL_RENDERER_ID_WINE),
        ATTRIB(WGL_SAMPLE_BUFFERS_ARB),
        ATTRIB(WGL_SAMPLES_ARB),
        ATTRIB(WGL_SHARE_ACCUM_ARB),
        ATTRIB(WGL_SHARE_DEPTH_ARB),
        ATTRIB(WGL_SHARE_STENCIL_ARB),
        ATTRIB(WGL_STENCIL_BITS_ARB),
        ATTRIB(WGL_STEREO_ARB),
        ATTRIB(WGL_SUPPORT_GDI_ARB),
        ATTRIB(WGL_SUPPORT_OPENGL_ARB),
        ATTRIB(WGL_SWAP_LAYER_BUFFERS_ARB),
        ATTRIB(WGL_SWAP_METHOD_ARB),
        ATTRIB(WGL_TRANSPARENT_ALPHA_VALUE_ARB),
        ATTRIB(WGL_TRANSPARENT_ARB),
        ATTRIB(WGL_TRANSPARENT_BLUE_VALUE_ARB),
        ATTRIB(WGL_TRANSPARENT_GREEN_VALUE_ARB),
        ATTRIB(WGL_TRANSPARENT_INDEX_VALUE_ARB),
        ATTRIB(WGL_TRANSPARENT_RED_VALUE_ARB),
#undef ATTRIB
    };
    int i;
    const char *attrib_name = NULL;
    const char *value_name = NULL;

    for (i = 0; i < ARRAY_SIZE(attrib_names); i++)
    {
        if (attrib_names[i].attrib == attrib)
        {
            attrib_name = attrib_names[i].name;
            break;
        }
    }

    if (!attrib_name)
        attrib_name = wine_dbg_sprintf("Attrib 0x%04x", attrib);

    switch (attrib)
    {
        case WGL_ACCELERATION_ARB:
            switch (value)
            {
                case WGL_FULL_ACCELERATION_ARB:     value_name = "WGL_FULL_ACCELERATION_ARB"; break;
                case WGL_GENERIC_ACCELERATION_ARB:  value_name = "WGL_GENERIC_ACCELERATION_ARB"; break;
                case WGL_NO_ACCELERATION_ARB:       value_name = "WGL_NO_ACCELERATION_ARB"; break;
            }
            break;
        case WGL_PIXEL_TYPE_ARB:
            switch (value)
            {
                case WGL_TYPE_COLORINDEX_ARB:           value_name = "WGL_TYPE_COLORINDEX_ARB"; break;
                case WGL_TYPE_RGBA_ARB:                 value_name = "WGL_TYPE_RGBA_ARB"; break;
                case WGL_TYPE_RGBA_FLOAT_ARB:           value_name = "WGL_TYPE_RGBA_FLOAT_ARB"; break;
                case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT:  value_name = "WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT"; break;
            }
            break;
        case WGL_SWAP_METHOD_ARB:
            switch (value)
            {
                case WGL_SWAP_COPY_ARB:         value_name = "WGL_SWAP_COPY_ARB"; break;
                case WGL_SWAP_EXCHANGE_ARB:     value_name = "WGL_SWAP_EXCHANGE_ARB"; break;
                case WGL_SWAP_UNDEFINED_ARB:    value_name = "WGL_SWAP_UNDEFINED_ARB"; break;
            }
            break;
    }

    if (!value_name)
        value_name = wine_dbg_sprintf("%d / 0x%04x", value, value);

    return wine_dbg_sprintf("%40s: %s", attrib_name, value_name);
}


/**********************************************************************
 *              active_displays_mask
 */
static CGOpenGLDisplayMask active_displays_mask(void)
{
    CGError err;
    CGDirectDisplayID displays[32];
    uint32_t count, i;
    CGOpenGLDisplayMask mask;

    err = CGGetActiveDisplayList(ARRAY_SIZE(displays), displays, &count);
    if (err != kCGErrorSuccess)
    {
        displays[0] = CGMainDisplayID();
        count = 1;
    }

    mask = 0;
    for (i = 0; i < count; i++)
        mask |= CGDisplayIDToOpenGLDisplayMask(displays[i]);

    return mask;
}


static BOOL get_renderer_property(CGLRendererInfoObj renderer_info, GLint renderer_index,
                                  CGLRendererProperty property, GLint *value)
{
    CGLError err = CGLDescribeRenderer(renderer_info, renderer_index, property, value);
    if (err != kCGLNoError)
        WARN("CGLDescribeRenderer failed for property %d: %d %s\n", property, err, CGLErrorString(err));
    return (err == kCGLNoError);
}


static void get_renderer_properties(CGLRendererInfoObj renderer_info, int renderer_index, renderer_properties* properties)
{
    GLint value;

    memset(properties, 0, sizeof(*properties));

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPRendererID, &value))
        properties->renderer_id = value & kCGLRendererIDMatchingMask;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPBufferModes, &value))
        properties->buffer_modes = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPColorModes, &value))
        properties->color_modes = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPAccumModes, &value))
        properties->accum_modes = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPDepthModes, &value))
        properties->depth_modes = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPStencilModes, &value))
        properties->stencil_modes = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPMaxAuxBuffers, &value))
        properties->max_aux_buffers = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPMaxSampleBuffers, &value))
        properties->max_sample_buffers = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPMaxSamples, &value))
        properties->max_samples = value;

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPOffScreen, &value))
        properties->offscreen = (value != 0);

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPAccelerated, &value))
        properties->accelerated = (value != 0);

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPBackingStore, &value))
        properties->backing_store = (value != 0);

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPWindow, &value))
        properties->window = (value != 0);

    if (get_renderer_property(renderer_info, renderer_index, kCGLRPOnline, &value))
        properties->online = (value != 0);
}


static void dump_renderer(const renderer_properties* renderer)
{
    int i;

    TRACE("Renderer ID: 0x%08x\n", renderer->renderer_id);
    TRACE("Buffer modes:\n");
    TRACE("    Monoscopic:    %s\n", (renderer->buffer_modes & kCGLMonoscopicBit) ? "YES" : "NO");
    TRACE("    Stereoscopic:  %s\n", (renderer->buffer_modes & kCGLStereoscopicBit) ? "YES" : "NO");
    TRACE("    Single buffer: %s\n", (renderer->buffer_modes & kCGLSingleBufferBit) ? "YES" : "NO");
    TRACE("    Double buffer: %s\n", (renderer->buffer_modes & kCGLDoubleBufferBit) ? "YES" : "NO");

    TRACE("Color buffer modes:\n");
    for (i = 0; i < ARRAY_SIZE(color_modes); i++)
    {
        if (renderer->color_modes & color_modes[i].mode)
        {
            TRACE("    Color size %d, Alpha size %d", color_modes[i].color_bits, color_modes[i].alpha_bits);
            if (color_modes[i].is_float)
                TRACE(", Float");
            TRACE("\n");
        }
    }

    TRACE("Accumulation buffer sizes: { ");
    for (i = 0; i < ARRAY_SIZE(color_modes); i++)
    {
        if (renderer->accum_modes & color_modes[i].mode)
            TRACE("%d, ", color_modes[i].color_bits);
    }
    TRACE("}\n");

    TRACE("Depth buffer sizes: { ");
    for (i = 0; i < ARRAY_SIZE(depth_stencil_modes); i++)
    {
        if (renderer->depth_modes & depth_stencil_modes[i].mode)
            TRACE("%d, ", depth_stencil_modes[i].bits);
    }
    TRACE("}\n");

    TRACE("Stencil buffer sizes: { ");
    for (i = 0; i < ARRAY_SIZE(depth_stencil_modes); i++)
    {
        if (renderer->stencil_modes & depth_stencil_modes[i].mode)
            TRACE("%d, ", depth_stencil_modes[i].bits);
    }
    TRACE("}\n");

    TRACE("Max. Auxiliary Buffers: %d\n", renderer->max_aux_buffers);
    TRACE("Max. Sample Buffers: %d\n", renderer->max_sample_buffers);
    TRACE("Max. Samples: %d\n", renderer->max_samples);
    TRACE("Offscreen: %s\n", renderer->offscreen ? "YES" : "NO");
    TRACE("Accelerated: %s\n", renderer->accelerated ? "YES" : "NO");
    TRACE("Backing store: %s\n", renderer->backing_store ? "YES" : "NO");
    TRACE("Window: %s\n", renderer->window ? "YES" : "NO");
    TRACE("Online: %s\n", renderer->online ? "YES" : "NO");
}


static inline UInt64 code_for_pixel_format(const pixel_format* format)
{
    pixel_format_or_code pfc;

    pfc.code = 0;
    pfc.format = *format;
    return pfc.code;
}


static inline pixel_format pixel_format_for_code(UInt64 code)
{
    pixel_format_or_code pfc;

    pfc.code = code;
    return pfc.format;
}


static const char *debugstr_pf(const pixel_format *pf)
{
    return wine_dbg_sprintf("w/p/a %u/%u/%u col %u%s/%u dp/stn/ac/ax/b/db/str %u/%u/%u/%u/%u/%u/%u samp %u/%u %017llx",
                            pf->window,
                            pf->pbuffer,
                            pf->accelerated,
                            color_modes[pf->color_mode].color_bits,
                            (color_modes[pf->color_mode].is_float ? "f" : ""),
                            color_modes[pf->color_mode].alpha_bits,
                            pf->depth_bits,
                            pf->stencil_bits,
                            pf->accum_mode ? color_modes[pf->accum_mode - 1].color_bits : 0,
                            pf->aux_buffers,
                            pf->backing_store,
                            pf->double_buffer,
                            pf->stereo,
                            pf->sample_buffers,
                            pf->samples,
                            code_for_pixel_format(pf));
}


static unsigned int best_color_mode(GLint modes, GLint color_size, GLint alpha_size, GLint color_float)
{
    int best = -1;
    int i;

    for (i = 0; i < ARRAY_SIZE(color_modes); i++)
    {
        if ((modes & color_modes[i].mode) &&
            color_modes[i].color_bits >= color_size &&
            color_modes[i].alpha_bits >= alpha_size &&
            !color_modes[i].is_float == !color_float)
        {
            if (best < 0) /* no existing best choice */
                best = i;
            else if (color_modes[i].color_bits == color_size &&
                     color_modes[i].alpha_bits == alpha_size) /* candidate is exact match */
            {
                /* prefer it over a best which isn't exact or which has a higher bpp */
                if (color_modes[best].color_bits != color_size ||
                    color_modes[best].alpha_bits != alpha_size ||
                    color_modes[i].bits_per_pixel < color_modes[best].bits_per_pixel)
                    best = i;
            }
            else if (color_modes[i].color_bits < color_modes[best].color_bits ||
                     (color_modes[i].color_bits == color_modes[best].color_bits &&
                      color_modes[i].alpha_bits < color_modes[best].alpha_bits)) /* prefer closer */
                best = i;
        }
    }

    if (best < 0)
    {
        /* Couldn't find a match.  Return first one that renderer supports. */
        for (i = 0; i < ARRAY_SIZE(color_modes); i++)
        {
            if (modes & color_modes[i].mode)
                return i;
        }
    }

    return best;
}


static unsigned int best_accum_mode(GLint modes, GLint accum_size)
{
    int best = -1;
    int i;

    for (i = 0; i < ARRAY_SIZE(color_modes); i++)
    {
        if ((modes & color_modes[i].mode) && color_modes[i].color_bits >= accum_size)
        {
            /* Prefer the fewest color bits, then prefer more alpha bits, then
               prefer more bits per pixel. */
            if (best < 0)
                best = i;
            else if (color_modes[i].color_bits < color_modes[best].color_bits)
                best = i;
            else if (color_modes[i].color_bits == color_modes[best].color_bits)
            {
                if (color_modes[i].alpha_bits > color_modes[best].alpha_bits)
                    best = i;
                else if (color_modes[i].alpha_bits == color_modes[best].alpha_bits &&
                         color_modes[i].bits_per_pixel > color_modes[best].bits_per_pixel)
                    best = i;
            }
        }
    }

    if (best < 0)
    {
        /* Couldn't find a match.  Return last one that renderer supports. */
        for (i = ARRAY_SIZE(color_modes) - 1; i >= 0; i--)
        {
            if (modes & color_modes[i].mode)
                return i;
        }
    }

    return best;
}


static void enum_renderer_pixel_formats(renderer_properties renderer, CFMutableArrayRef pixel_format_array,
                                        CFMutableSetRef pixel_format_set)
{
    CGLPixelFormatAttribute attribs[64] = {
        kCGLPFAMinimumPolicy,
        kCGLPFAClosestPolicy,
        kCGLPFARendererID, renderer.renderer_id,
        kCGLPFASingleRenderer,
    };
    int n = 5, n_stack[16], n_stack_idx = -1;
    unsigned int tried_pixel_formats = 0, failed_pixel_formats = 0, dupe_pixel_formats = 0,
                 new_pixel_formats = 0;
    pixel_format request;
    unsigned int double_buffer;
    unsigned int accelerated = renderer.accelerated;

    if (accelerated)
    {
        attribs[n++] = kCGLPFAAccelerated;
        attribs[n++] = kCGLPFANoRecovery;
    }
    else if (!allow_software_rendering)
    {
        TRACE("ignoring software renderer because AllowSoftwareRendering is off\n");
        return;
    }

    n_stack[++n_stack_idx] = n;
    for (double_buffer = 0; double_buffer <= 1; double_buffer++)
    {
        unsigned int aux;

        n = n_stack[n_stack_idx];

        if ((!double_buffer && !(renderer.buffer_modes & kCGLSingleBufferBit)) ||
            (double_buffer && !(renderer.buffer_modes & kCGLDoubleBufferBit)))
            continue;

        if (double_buffer)
            attribs[n++] = kCGLPFADoubleBuffer;
        memset(&request, 0, sizeof(request));
        request.accelerated = accelerated;
        request.double_buffer = double_buffer;

        /* Don't bother with in-between aux buffers values: either 0 or max. */
        n_stack[++n_stack_idx] = n;
        for (aux = 0; aux <= renderer.max_aux_buffers; aux += renderer.max_aux_buffers)
        {
            unsigned int color_mode;

            n = n_stack[n_stack_idx];

            attribs[n++] = kCGLPFAAuxBuffers;
            attribs[n++] = aux;
            request.aux_buffers = aux;

            n_stack[++n_stack_idx] = n;
            for (color_mode = 0; color_mode < ARRAY_SIZE(color_modes); color_mode++)
            {
                unsigned int depth_mode;

                n = n_stack[n_stack_idx];

                if (!(renderer.color_modes & color_modes[color_mode].mode))
                    continue;

                attribs[n++] = kCGLPFAColorSize;
                attribs[n++] = color_modes[color_mode].color_bits;
                attribs[n++] = kCGLPFAAlphaSize;
                attribs[n++] = color_modes[color_mode].alpha_bits;
                if (color_modes[color_mode].is_float)
                    attribs[n++] = kCGLPFAColorFloat;
                request.color_mode = color_mode;

                n_stack[++n_stack_idx] = n;
                for (depth_mode = 0; depth_mode < ARRAY_SIZE(depth_stencil_modes); depth_mode++)
                {
                    unsigned int stencil_mode;

                    n = n_stack[n_stack_idx];

                    if (!(renderer.depth_modes & depth_stencil_modes[depth_mode].mode))
                        continue;

                    attribs[n++] = kCGLPFADepthSize;
                    attribs[n++] = depth_stencil_modes[depth_mode].bits;
                    request.depth_bits = depth_stencil_modes[depth_mode].bits;

                    n_stack[++n_stack_idx] = n;
                    for (stencil_mode = 0; stencil_mode < ARRAY_SIZE(depth_stencil_modes); stencil_mode++)
                    {
                        unsigned int stereo;

                        n = n_stack[n_stack_idx];

                        if (!(renderer.stencil_modes & depth_stencil_modes[stencil_mode].mode))
                            continue;
                        if (accelerated && depth_stencil_modes[depth_mode].bits != 24 &&
                            depth_stencil_modes[depth_mode].bits != 32 && stencil_mode > 0)
                            continue;

                        attribs[n++] = kCGLPFAStencilSize;
                        attribs[n++] = depth_stencil_modes[stencil_mode].bits;
                        request.stencil_bits = depth_stencil_modes[stencil_mode].bits;

                        /* FIXME: Could trim search space a bit here depending on GPU.
                                  For ATI Radeon HD 4850, kCGLRGBA16161616Bit implies stereo-capable. */
                        n_stack[++n_stack_idx] = n;
                        for (stereo = 0; stereo <= 1; stereo++)
                        {
                            int accum_mode;

                            n = n_stack[n_stack_idx];

                            if ((!stereo && !(renderer.buffer_modes & kCGLMonoscopicBit)) ||
                                (stereo && !(renderer.buffer_modes & kCGLStereoscopicBit)))
                                continue;

                            if (stereo)
                                attribs[n++] = kCGLPFAStereo;
                            request.stereo = stereo;

                            /* Starts at -1 for a 0 accum size */
                            n_stack[++n_stack_idx] = n;
                            for (accum_mode = -1; accum_mode < (int) ARRAY_SIZE(color_modes); accum_mode++)
                            {
                                unsigned int target_pass;

                                n = n_stack[n_stack_idx];

                                if (accum_mode >= 0)
                                {
                                    if (!(renderer.accum_modes & color_modes[accum_mode].mode))
                                        continue;

                                    attribs[n++] = kCGLPFAAccumSize;
                                    attribs[n++] = color_modes[accum_mode].color_bits;
                                    request.accum_mode = accum_mode + 1;
                                }
                                else
                                    request.accum_mode = 0;

                                /* Targets to request are:
                                        accelerated: window OR window + pbuffer
                                        software: window + pbuffer */
                                n_stack[++n_stack_idx] = n;
                                for (target_pass = 0; target_pass <= accelerated; target_pass++)
                                {
                                    unsigned int samples, max_samples;

                                    n = n_stack[n_stack_idx];

                                    attribs[n++] = kCGLPFAWindow;
                                    request.window = 1;

                                    if (!accelerated || target_pass > 0)
                                    {
                                        attribs[n++] = kCGLPFAPBuffer;
                                        request.pbuffer = 1;
                                    }
                                    else
                                        request.pbuffer = 0;

                                    /* FIXME: Could trim search space a bit here depending on GPU.
                                              For Nvidia GeForce 8800 GT, limited to 4 samples for color_bits >= 128.
                                              For ATI Radeon HD 4850, can't multi-sample for color_bits >= 64 or pbuffer. */
                                    n_stack[++n_stack_idx] = n;
                                    max_samples = renderer.max_sample_buffers ? max(1, renderer.max_samples) : 1;
                                    for (samples = 1; samples <= max_samples; samples *= 2)
                                    {
                                        unsigned int backing_store, min_backing_store, max_backing_store;

                                        n = n_stack[n_stack_idx];

                                        if (samples > 1)
                                        {
                                            attribs[n++] = kCGLPFASampleBuffers;
                                            attribs[n++] = renderer.max_sample_buffers;
                                            attribs[n++] = kCGLPFASamples;
                                            attribs[n++] = samples;
                                            request.sample_buffers = renderer.max_sample_buffers;
                                            request.samples = samples;
                                        }
                                        else
                                            request.sample_buffers = request.samples = 0;

                                        if (renderer.backing_store && double_buffer)
                                        {
                                            /* The software renderer seems to always preserve the backing store, whether
                                               we ask for it or not.  So don't bother not asking for it. */
                                            min_backing_store = accelerated ? 0 : 1;
                                            max_backing_store = 1;
                                        }
                                        else
                                            min_backing_store = max_backing_store = 0;
                                        n_stack[++n_stack_idx] = n;
                                        for (backing_store = min_backing_store; backing_store <= max_backing_store; backing_store++)
                                        {
                                            CGLPixelFormatObj pix;
                                            GLint virtualScreens;
                                            CGLError err;

                                            n = n_stack[n_stack_idx];

                                            if (backing_store)
                                                attribs[n++] = kCGLPFABackingStore;
                                            request.backing_store = backing_store;

                                            attribs[n] = 0;

                                            err = CGLChoosePixelFormat(attribs, &pix, &virtualScreens);
                                            if (err == kCGLNoError && pix)
                                            {
                                                pixel_format pf;
                                                GLint value, color_size, alpha_size, color_float;
                                                UInt64 pf_code;
                                                CFNumberRef code_object;
                                                BOOL dupe;

                                                memset(&pf, 0, sizeof(pf));

                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAAccelerated, &value) == kCGLNoError)
                                                    pf.accelerated = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAAuxBuffers, &value) == kCGLNoError)
                                                    pf.aux_buffers = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFADepthSize, &value) == kCGLNoError)
                                                    pf.depth_bits = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFADoubleBuffer, &value) == kCGLNoError)
                                                    pf.double_buffer = value;
                                                if (pf.double_buffer &&
                                                    CGLDescribePixelFormat(pix, 0, kCGLPFABackingStore, &value) == kCGLNoError)
                                                    pf.backing_store = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAPBuffer, &value) == kCGLNoError)
                                                    pf.pbuffer = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFASampleBuffers, &value) == kCGLNoError)
                                                    pf.sample_buffers = value;
                                                if (pf.sample_buffers &&
                                                    CGLDescribePixelFormat(pix, 0, kCGLPFASamples, &value) == kCGLNoError)
                                                    pf.samples = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAStencilSize, &value) == kCGLNoError)
                                                    pf.stencil_bits = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAStereo, &value) == kCGLNoError)
                                                    pf.stereo = value;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAWindow, &value) == kCGLNoError)
                                                    pf.window = value;

                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAColorSize, &color_size) != kCGLNoError)
                                                    color_size = 0;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAAlphaSize, &alpha_size) != kCGLNoError)
                                                    alpha_size = 0;
                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAColorFloat, &color_float) != kCGLNoError)
                                                    color_float = 0;
                                                pf.color_mode = best_color_mode(renderer.color_modes, color_size, alpha_size, color_float);

                                                if (CGLDescribePixelFormat(pix, 0, kCGLPFAAccumSize, &value) == kCGLNoError && value)
                                                    pf.accum_mode = best_accum_mode(renderer.accum_modes, value) + 1;

                                                CGLReleasePixelFormat(pix);

                                                pf_code = code_for_pixel_format(&pf);

                                                code_object = CFNumberCreate(NULL, kCFNumberSInt64Type, &pf_code);
                                                if ((dupe = CFSetContainsValue(pixel_format_set, code_object)))
                                                    dupe_pixel_formats++;
                                                else
                                                {
                                                    CFSetAddValue(pixel_format_set, code_object);
                                                    CFArrayAppendValue(pixel_format_array, code_object);
                                                    new_pixel_formats++;
                                                }
                                                CFRelease(code_object);

                                                if (pf_code == code_for_pixel_format(&request))
                                                    TRACE("%s%s\n", debugstr_pf(&pf), dupe ? " (duplicate)" : "");
                                                else
                                                {
                                                    TRACE("%s remapped from %s%s\n", debugstr_pf(&pf), debugstr_pf(&request),
                                                          dupe ? " (duplicate)" : "");
                                                }
                                            }
                                            else
                                            {
                                                failed_pixel_formats++;
                                                TRACE("%s failed request err %d %s\n", debugstr_pf(&request), err, err ? CGLErrorString(err) : "");
                                            }

                                            tried_pixel_formats++;
                                        }

                                        n_stack_idx--;
                                    }

                                    n_stack_idx--;
                                }

                                n_stack_idx--;
                            }

                            n_stack_idx--;
                        }

                        n_stack_idx--;
                    }

                    n_stack_idx--;
                }

                n_stack_idx--;
            }

            n_stack_idx--;
        }

        n_stack_idx--;
    }

    n_stack_idx--;

    TRACE("Number of pixel format attribute combinations: %u\n", tried_pixel_formats);
    TRACE(" Number which failed to choose a pixel format: %u\n", failed_pixel_formats);
    TRACE("   Number which chose redundant pixel formats: %u\n", dupe_pixel_formats);
    TRACE("Number of new pixel formats for this renderer: %u\n", new_pixel_formats);
}


/* The docs for WGL_ARB_pixel_format say:
    Indices are assigned to pixel formats in the following order:
    1. Accelerated pixel formats that are displayable
    2. Accelerated pixel formats that are displayable and which have
       extended attributes
    3. Generic pixel formats
    4. Accelerated pixel formats that are non displayable
 */
static int pixel_format_category(pixel_format pf)
{
    /* non-displayable */
    if (!pf.window)
        return 4;

    /* non-accelerated a.k.a. software a.k.a. generic */
    if (!pf.accelerated)
        return 3;

    /* extended attributes that can't be represented in PIXELFORMATDESCRIPTOR */
    if (color_modes[pf.color_mode].is_float)
        return 2;

    /* accelerated, displayable, no extended attributes */
    return 1;
}


static CFComparisonResult pixel_format_comparator(const void *val1, const void *val2, void *context)
{
    CFNumberRef number1 = val1;
    CFNumberRef number2 = val2;
    UInt64 code1, code2;
    pixel_format pf1, pf2;
    int category1, category2;

    CFNumberGetValue(number1, kCFNumberLongLongType, &code1);
    CFNumberGetValue(number2, kCFNumberLongLongType, &code2);
    pf1 = pixel_format_for_code(code1);
    pf2 = pixel_format_for_code(code2);
    category1 = pixel_format_category(pf1);
    category2 = pixel_format_category(pf2);

    if (category1 < category2)
        return kCFCompareLessThan;
    if (category1 > category2)
        return kCFCompareGreaterThan;

    /* Within a category, sort the "best" formats toward the front since that's
       what wglChoosePixelFormatARB() has to do.  The ordering implemented here
       matches at least one Windows 7 machine's behavior.
     */
    /* Accelerated before unaccelerated. */
    if (pf1.accelerated && !pf2.accelerated)
        return kCFCompareLessThan;
    if (!pf1.accelerated && pf2.accelerated)
        return kCFCompareGreaterThan;

    /* Explicit color mode ordering. */
    if (color_modes[pf1.color_mode].color_ordering < color_modes[pf2.color_mode].color_ordering)
        return kCFCompareLessThan;
    if (color_modes[pf1.color_mode].color_ordering > color_modes[pf2.color_mode].color_ordering)
        return kCFCompareGreaterThan;

    /* Non-pbuffer-capable before pbuffer-capable. */
    if (!pf1.pbuffer && pf2.pbuffer)
        return kCFCompareLessThan;
    if (pf1.pbuffer && !pf2.pbuffer)
        return kCFCompareGreaterThan;

    /* Fewer samples before more samples. */
    if (pf1.samples < pf2.samples)
        return kCFCompareLessThan;
    if (pf1.samples > pf2.samples)
        return kCFCompareGreaterThan;

    /* Monoscopic before stereoscopic.  (This is a guess.) */
    if (!pf1.stereo && pf2.stereo)
        return kCFCompareLessThan;
    if (pf1.stereo && !pf2.stereo)
        return kCFCompareGreaterThan;

    /* Single buffered before double buffered. */
    if (!pf1.double_buffer && pf2.double_buffer)
        return kCFCompareLessThan;
    if (pf1.double_buffer && !pf2.double_buffer)
        return kCFCompareGreaterThan;

    /* Possibly-optimized double buffering before backing-store-preserving
       double buffering. */
    if (!pf1.backing_store && pf2.backing_store)
        return kCFCompareLessThan;
    if (pf1.backing_store && !pf2.backing_store)
        return kCFCompareGreaterThan;

    /* Bigger depth buffer before smaller depth buffer. */
    if (pf1.depth_bits > pf2.depth_bits)
        return kCFCompareLessThan;
    if (pf1.depth_bits < pf2.depth_bits)
        return kCFCompareGreaterThan;

    /* Smaller stencil buffer before bigger stencil buffer. */
    if (pf1.stencil_bits < pf2.stencil_bits)
        return kCFCompareLessThan;
    if (pf1.stencil_bits > pf2.stencil_bits)
        return kCFCompareGreaterThan;

    /* Smaller alpha bits before larger alpha bits. */
    if (color_modes[pf1.color_mode].alpha_bits < color_modes[pf2.color_mode].alpha_bits)
        return kCFCompareLessThan;
    if (color_modes[pf1.color_mode].alpha_bits > color_modes[pf2.color_mode].alpha_bits)
        return kCFCompareGreaterThan;

    /* Smaller accum buffer before larger accum buffer.  (This is a guess.) */
    if (pf1.accum_mode)
    {
        if (pf2.accum_mode)
        {
            if (color_modes[pf1.accum_mode - 1].color_bits - color_modes[pf1.accum_mode - 1].alpha_bits <
                color_modes[pf2.accum_mode - 1].color_bits - color_modes[pf2.accum_mode - 1].alpha_bits)
                return kCFCompareLessThan;
            if (color_modes[pf1.accum_mode - 1].color_bits - color_modes[pf1.accum_mode - 1].alpha_bits >
                color_modes[pf2.accum_mode - 1].color_bits - color_modes[pf2.accum_mode - 1].alpha_bits)
                return kCFCompareGreaterThan;

            if (color_modes[pf1.accum_mode - 1].bits_per_pixel < color_modes[pf2.accum_mode - 1].bits_per_pixel)
                return kCFCompareLessThan;
            if (color_modes[pf1.accum_mode - 1].bits_per_pixel > color_modes[pf2.accum_mode - 1].bits_per_pixel)
                return kCFCompareGreaterThan;

            if (color_modes[pf1.accum_mode - 1].alpha_bits < color_modes[pf2.accum_mode - 1].alpha_bits)
                return kCFCompareLessThan;
            if (color_modes[pf1.accum_mode - 1].alpha_bits > color_modes[pf2.accum_mode - 1].alpha_bits)
                return kCFCompareGreaterThan;
        }
        else
            return kCFCompareGreaterThan;
    }
    else if (pf2.accum_mode)
        return kCFCompareLessThan;

    /* Fewer auxiliary buffers before more auxiliary buffers.  (This is a guess.) */
    if (pf1.aux_buffers < pf2.aux_buffers)
        return kCFCompareLessThan;
    if (pf1.aux_buffers > pf2.aux_buffers)
        return kCFCompareGreaterThan;

    /* If we get here, arbitrarily sort based on code. */
    if (code1 < code2)
        return kCFCompareLessThan;
    if (code1 > code2)
        return kCFCompareGreaterThan;
    return kCFCompareEqualTo;
}


static UINT macdrv_init_pixel_formats(UINT *onscreen_count)
{
    CGLRendererInfoObj renderer_info;
    GLint rendererCount;
    CGLError err;
    CFMutableSetRef pixel_format_set;
    CFMutableArrayRef pixel_format_array;
    int i;
    CFRange range;

    TRACE("()\n");

    err = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()), &renderer_info, &rendererCount);
    if (err)
    {
        WARN("CGLQueryRendererInfo failed (%d) %s\n", err, CGLErrorString(err));
        return 0;
    }

    pixel_format_set = CFSetCreateMutable(NULL, 0, &kCFTypeSetCallBacks);
    if (!pixel_format_set)
    {
        WARN("CFSetCreateMutable failed\n");
        CGLDestroyRendererInfo(renderer_info);
        return 0;
    }

    pixel_format_array = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    if (!pixel_format_array)
    {
        WARN("CFArrayCreateMutable failed\n");
        CFRelease(pixel_format_set);
        CGLDestroyRendererInfo(renderer_info);
        return 0;
    }

    for (i = 0; i < rendererCount; i++)
    {
        renderer_properties renderer;

        get_renderer_properties(renderer_info, i, &renderer);
        if (TRACE_ON(wgl))
        {
            TRACE("renderer_properties %d:\n", i);
            dump_renderer(&renderer);
        }

        enum_renderer_pixel_formats(renderer, pixel_format_array, pixel_format_set);
    }

    CFRelease(pixel_format_set);
    CGLDestroyRendererInfo(renderer_info);

    range = CFRangeMake(0, CFArrayGetCount(pixel_format_array));
    if (range.length)
    {
        pixel_formats = malloc(range.length * sizeof(*pixel_formats));
        if (pixel_formats)
        {
            CFArraySortValues(pixel_format_array, range, pixel_format_comparator, NULL);
            for (i = 0; i < range.length; i++)
            {
                CFNumberRef number = CFArrayGetValueAtIndex(pixel_format_array, i);
                UInt64 code;

                CFNumberGetValue(number, kCFNumberLongLongType, &code);
                pixel_formats[i] = pixel_format_for_code(code);
                if (pixel_formats[i].window)
                    nb_displayable_formats++;
            }

            nb_formats = range.length;
            TRACE("Total number of unique pixel formats: %d\n", nb_formats);
        }
        else
        {
            WARN("failed to allocate pixel format list\n");
            nb_formats = 0;
        }
    }
    else
        WARN("got no pixel formats\n");

    CFRelease(pixel_format_array);

    *onscreen_count = nb_displayable_formats;
    return nb_formats;
}


static inline BOOL is_valid_pixel_format(int format)
{
    return format > 0 && format <= nb_formats;
}


static inline BOOL is_displayable_pixel_format(int format)
{
    return format > 0 && format <= nb_displayable_formats;
}


static const pixel_format *get_pixel_format(int format, BOOL allow_nondisplayable)
{
    /* Check if the pixel format is valid. Note that it is legal to pass an invalid
     * format in case of probing the number of pixel formats.
     */
    if (is_valid_pixel_format(format) && (is_displayable_pixel_format(format) || allow_nondisplayable))
    {
        TRACE("Returning format %d\n", format);
        return &pixel_formats[format - 1];
    }
    return NULL;
}


static BOOL init_gl_info(void)
{
    static const char legacy_extensions[] = " WGL_EXT_extensions_string";
    static const char legacy_ext_swap_control[] = " WGL_EXT_swap_control";

    CGDirectDisplayID display = CGMainDisplayID();
    CGOpenGLDisplayMask displayMask = CGDisplayIDToOpenGLDisplayMask(display);
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFADisplayMask, displayMask,
        0
    };
    CGLPixelFormatAttribute core_attribs[] =
    {
        kCGLPFADisplayMask, displayMask,
        kCGLPFAAccelerated,
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        0
    };
    CGLPixelFormatObj pix;
    GLint virtualScreens;
    CGLError err;
    CGLContextObj context;
    CGLContextObj old_context = CGLGetCurrentContext();
    const char *str;
    size_t length;

    err = CGLChoosePixelFormat(attribs, &pix, &virtualScreens);
    if (err != kCGLNoError || !pix)
    {
        WARN("CGLChoosePixelFormat() failed with error %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    err = CGLCreateContext(pix, NULL, &context);
    CGLReleasePixelFormat(pix);
    if (err != kCGLNoError || !context)
    {
        WARN("CGLCreateContext() failed with error %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    err = CGLSetCurrentContext(context);
    if (err != kCGLNoError)
    {
        WARN("CGLSetCurrentContext() failed with error %d %s\n", err, CGLErrorString(err));
        CGLReleaseContext(context);
        return FALSE;
    }

    str = (const char*)pglGetString(GL_EXTENSIONS);
    length = strlen(str) + sizeof(legacy_extensions);
    if (allow_vsync)
        length += strlen(legacy_ext_swap_control);
    gl_info.glExtensions = malloc(length);
    strcpy(gl_info.glExtensions, str);
    strcat(gl_info.glExtensions, legacy_extensions);
    if (allow_vsync)
        strcat(gl_info.glExtensions, legacy_ext_swap_control);

    pglGetIntegerv(GL_MAX_VIEWPORT_DIMS, gl_info.max_viewport_dims);

    str = (const char*)pglGetString(GL_VERSION);
    sscanf(str, "%u.%u", &gl_info.max_major, &gl_info.max_minor);
    TRACE("GL version   : %s\n", str);
    TRACE("GL renderer  : %s\n", pglGetString(GL_RENDERER));

    CGLSetCurrentContext(old_context);
    CGLReleaseContext(context);

    err = CGLChoosePixelFormat(core_attribs, &pix, &virtualScreens);
    if (err != kCGLNoError || !pix)
    {
        WARN("CGLChoosePixelFormat() for a core context failed with error %d %s\n",
             err, CGLErrorString(err));
        return TRUE;
    }

    err = CGLCreateContext(pix, NULL, &context);
    CGLReleasePixelFormat(pix);
    if (err != kCGLNoError || !context)
    {
        WARN("CGLCreateContext() for a core context failed with error %d %s\n",
             err, CGLErrorString(err));
        return TRUE;
    }

    err = CGLSetCurrentContext(context);
    if (err != kCGLNoError)
    {
        WARN("CGLSetCurrentContext() for a core context failed with error %d %s\n",
             err, CGLErrorString(err));
        CGLReleaseContext(context);
        return TRUE;
    }

    str = (const char*)pglGetString(GL_VERSION);
    TRACE("Core context GL version: %s\n", str);
    sscanf(str, "%u.%u", &gl_info.max_major, &gl_info.max_minor);
    CGLSetCurrentContext(old_context);
    CGLReleaseContext(context);

    return TRUE;
}


/**********************************************************************
 *              create_context
 */
static BOOL create_context(struct macdrv_context *context, CGLContextObj share, unsigned int major)
{
    const pixel_format *pf;
    CGLPixelFormatAttribute attribs[64];
    int n = 0;
    CGLPixelFormatObj pix;
    GLint virtualScreens;
    CGLError err;
    BOOL core = major >= 3;

    pf = get_pixel_format(context->format, TRUE /* non-displayable */);
    if (!pf)
    {
        ERR("Invalid pixel format %d, expect problems!\n", context->format);
        RtlSetLastWin32Error(ERROR_INVALID_PIXEL_FORMAT);
        return FALSE;
    }

    attribs[n++] = kCGLPFAMinimumPolicy;
    attribs[n++] = kCGLPFAClosestPolicy;

    if (context->renderer_id)
    {
        attribs[n++] = kCGLPFARendererID;
        attribs[n++] = context->renderer_id;
        attribs[n++] = kCGLPFASingleRenderer;
        attribs[n++] = kCGLPFANoRecovery;
    }

    if (pf->accelerated)
    {
        attribs[n++] = kCGLPFAAccelerated;
        attribs[n++] = kCGLPFANoRecovery;
    }
    else
    {
        attribs[n++] = kCGLPFARendererID;
        attribs[n++] = kCGLRendererGenericFloatID;
    }

    if (pf->double_buffer)
        attribs[n++] = kCGLPFADoubleBuffer;

    if (!core)
    {
        attribs[n++] = kCGLPFAAuxBuffers;
        attribs[n++] = pf->aux_buffers;
    }

    attribs[n++] = kCGLPFAColorSize;
    attribs[n++] = color_modes[pf->color_mode].color_bits;
    attribs[n++] = kCGLPFAAlphaSize;
    attribs[n++] = color_modes[pf->color_mode].alpha_bits;
    if (color_modes[pf->color_mode].is_float)
        attribs[n++] = kCGLPFAColorFloat;

    attribs[n++] = kCGLPFADepthSize;
    attribs[n++] = pf->depth_bits;

    attribs[n++] = kCGLPFAStencilSize;
    attribs[n++] = pf->stencil_bits;

    if (pf->stereo)
        attribs[n++] = kCGLPFAStereo;

    if (pf->accum_mode && !core)
    {
        attribs[n++] = kCGLPFAAccumSize;
        attribs[n++] = color_modes[pf->accum_mode - 1].color_bits;
    }

    if (pf->pbuffer && !core)
        attribs[n++] = kCGLPFAPBuffer;

    if (pf->sample_buffers && pf->samples)
    {
        attribs[n++] = kCGLPFASampleBuffers;
        attribs[n++] = pf->sample_buffers;
        attribs[n++] = kCGLPFASamples;
        attribs[n++] = pf->samples;
    }

    if (pf->backing_store)
        attribs[n++] = kCGLPFABackingStore;

    if (core)
    {
        attribs[n++] = kCGLPFAOpenGLProfile;
        if (major == 3)
            attribs[n++] = (int)kCGLOGLPVersion_GL3_Core;
        else
            attribs[n++] = (int)kCGLOGLPVersion_GL4_Core;
    }

    attribs[n] = 0;

    err = CGLChoosePixelFormat(attribs, &pix, &virtualScreens);
    if (err != kCGLNoError || !pix)
    {
        WARN("CGLChoosePixelFormat() failed with error %d %s\n", err, CGLErrorString(err));
        RtlSetLastWin32Error(ERROR_INVALID_OPERATION);
        return FALSE;
    }

    err = CGLCreateContext(pix, share, &context->cglcontext);
    CGLReleasePixelFormat(pix);
    if (err != kCGLNoError || !context->cglcontext)
    {
        context->cglcontext = NULL;
        WARN("CGLCreateContext() failed with error %d %s\n", err, CGLErrorString(err));
        RtlSetLastWin32Error(ERROR_INVALID_OPERATION);
        return FALSE;
    }

    if (gl_surface_mode == GL_SURFACE_IN_FRONT_TRANSPARENT)
    {
        GLint opacity = 0;
        err = CGLSetParameter(context->cglcontext, kCGLCPSurfaceOpacity, &opacity);
        if (err != kCGLNoError)
            WARN("CGLSetParameter(kCGLCPSurfaceOpacity) failed with error %d %s; leaving opaque\n", err, CGLErrorString(err));
    }
    else if (gl_surface_mode == GL_SURFACE_BEHIND)
    {
        GLint order = -1;
        err = CGLSetParameter(context->cglcontext, kCGLCPSurfaceOrder, &order);
        if (err != kCGLNoError)
            WARN("CGLSetParameter(kCGLCPSurfaceOrder) failed with error %d %s; leaving in front\n", err, CGLErrorString(err));
    }

    context->context = macdrv_create_opengl_context(context->cglcontext);
    CGLReleaseContext(context->cglcontext);
    if (!context->context)
    {
        WARN("macdrv_create_opengl_context() failed\n");
        RtlSetLastWin32Error(ERROR_INVALID_OPERATION);
        return FALSE;
    }
    context->major = major;
    context->swap_interval = INT_MIN;

    TRACE("created context %p/%p/%p\n", context, context->context, context->cglcontext);

    return TRUE;
}

static BOOL macdrv_set_pixel_format(HWND hwnd, int old_format, int new_format, BOOL internal)
{
    struct macdrv_win_data *data;

    TRACE("hwnd %p, old_format %d, new_format %d, internal %u\n", hwnd, old_format, new_format, internal);

    if (!(data = get_win_data(hwnd)))
    {
        FIXME("DC for window %p of other process: not implemented\n", hwnd);
        return FALSE;
    }

    data->pixel_format = new_format;
    release_win_data(data);
    return TRUE;
}


/**********************************************************************
 *              mark_contexts_for_moved_view
 */
static void mark_contexts_for_moved_view(macdrv_view view)
{
    struct macdrv_context *context;

    pthread_mutex_lock(&context_mutex);
    LIST_FOR_EACH_ENTRY(context, &context_list, struct macdrv_context, entry)
    {
        if (context->draw_view == view)
            InterlockedExchange(&context->view_moved, TRUE);
    }
    pthread_mutex_unlock(&context_mutex);
}


/**********************************************************************
 *              sync_context_rect
 */
static BOOL sync_context_rect(struct macdrv_context *context)
{
    BOOL ret = FALSE;
    RECT rect;
    struct macdrv_win_data *data = get_win_data(context->draw_hwnd);

    if (data && data->client_cocoa_view == context->draw_view)
    {
        if (InterlockedCompareExchange(&context->view_moved, FALSE, TRUE))
        {
            rect = data->rects.client;
            OffsetRect(&rect, -data->rects.visible.left, -data->rects.visible.top);
            if (!EqualRect(&context->draw_rect, &rect))
            {
                context->draw_rect = rect;
                ret = TRUE;
            }
        }

        NtUserGetClientRect(context->draw_hwnd, &rect, NtUserGetDpiForWindow(context->draw_hwnd));
        if (!EqualRect(&context->draw_rect_win_dpi, &rect))
        {
            context->draw_rect_win_dpi = rect;
            ret = TRUE;
        }
    }

    release_win_data(data);
    return ret;
}


/**********************************************************************
 *              make_context_current
 */
static void make_context_current(struct macdrv_context *context, BOOL read)
{
    macdrv_view view;
    RECT view_rect;
    CGLPBufferObj pbuffer;

    if (read)
    {
        view = context->read_view;
        view_rect = context->read_rect;
        pbuffer = context->read_pbuffer;
    }
    else
    {
        sync_context_rect(context);

        view = context->draw_view;
        view_rect = context->draw_rect_win_dpi;
        pbuffer = context->draw_pbuffer;
    }

    if (view || !pbuffer)
        macdrv_make_context_current(context->context, view, cgrect_from_rect(view_rect));
    else
    {
        GLint enabled;

        if (CGLIsEnabled(context->cglcontext, kCGLCESurfaceBackingSize, &enabled) == kCGLNoError && enabled)
            CGLDisable(context->cglcontext, kCGLCESurfaceBackingSize);
        CGLSetPBuffer(context->cglcontext, pbuffer, context->draw_pbuffer_face, context->draw_pbuffer_level, 0);
        CGLSetCurrentContext(context->cglcontext);
    }
}


/**********************************************************************
 *              sync_context
 */
static void sync_context(struct macdrv_context *context)
{
    if (sync_context_rect(context))
        make_context_current(context, FALSE);
}


/**********************************************************************
 *              set_swap_interval
 */
static BOOL set_swap_interval(struct macdrv_context *context, long interval)
{
    CGLError err;

    if (!allow_vsync || !context->draw_hwnd) interval = 0;

    if (interval < 0) interval = -interval;
    if (context->swap_interval == interval) return TRUE;

    /* In theory, for single-buffered contexts, there's no such thing as a swap
       so the swap interval shouldn't matter.  But OS X will synchronize flushes
       of single-buffered contexts if the interval is set to non-zero. */
    if (interval && !pixel_formats[context->format - 1].double_buffer)
        interval = 0;

    err = CGLSetParameter(context->cglcontext, kCGLCPSwapInterval, (GLint*)&interval);
    if (err != kCGLNoError)
        WARN("CGLSetParameter(kCGLCPSwapInterval) failed; error %d %s\n", err, CGLErrorString(err));
    context->swap_interval = interval;

    return err == kCGLNoError;
}


/**********************************************************************
 *              get_iokit_display_property
 */
static BOOL get_iokit_display_property(CGLRendererInfoObj renderer_info, GLint renderer, CFStringRef property, GLuint* value)
{
    GLint accelerated;
    GLint display_mask;
    int i;

    if (!get_renderer_property(renderer_info, renderer, kCGLRPAccelerated, &accelerated) || !accelerated)
    {
        TRACE("assuming unaccelerated renderers don't have IOKit properties\n");
        return FALSE;
    }

    if (!get_renderer_property(renderer_info, renderer, kCGLRPDisplayMask, &display_mask))
    {
        WARN("failed to get kCGLRPDisplayMask\n");
        return FALSE;
    }

    for (i = 0; i < sizeof(GLint) * 8; i++)
    {
        GLint this_display_mask = (GLint)(1U << i);
        if (this_display_mask & display_mask)
        {
            CGDirectDisplayID display_id = CGOpenGLDisplayMaskToDisplayID(this_display_mask);
            io_service_t service;
            CFDataRef data;
            uint32_t prop_value;

            if (!display_id)
                continue;
            service = CGDisplayIOServicePort(display_id);
            if (!service)
            {
                WARN("CGDisplayIOServicePort(%u) failed\n", display_id);
                continue;
            }

            data = IORegistryEntrySearchCFProperty(service, kIOServicePlane, property, NULL,
                                                   kIORegistryIterateRecursively | kIORegistryIterateParents);
            if (!data)
            {
                WARN("IORegistryEntrySearchCFProperty(%s) failed for display %u\n", debugstr_cf(property), display_id);
                continue;
            }
            if (CFGetTypeID(data) != CFDataGetTypeID())
            {
                WARN("property %s is not a data object: %s\n", debugstr_cf(property), debugstr_cf(data));
                CFRelease(data);
                continue;
            }
            if (CFDataGetLength(data) != sizeof(prop_value))
            {
                WARN("%s data for display %u has unexpected length %llu\n", debugstr_cf(property), display_id,
                     (unsigned long long)CFDataGetLength(data));
                CFRelease(data);
                continue;
            }

            CFDataGetBytes(data, CFRangeMake(0, sizeof(prop_value)), (UInt8*)&prop_value);
            CFRelease(data);
            *value = prop_value;
            return TRUE;
        }
    }

    return FALSE;
}


/**********************************************************************
 *              create_pixel_format_for_renderer
 *
 * Helper for macdrv_wglQueryRendererIntegerWINE().  Caller is
 * responsible for releasing the pixel format object.
 */
static CGLPixelFormatObj create_pixel_format_for_renderer(CGLRendererInfoObj renderer_info, GLint renderer, BOOL core)
{
    GLint renderer_id;
    CGLPixelFormatAttribute attrs[] = {
        kCGLPFARendererID, 0,
        kCGLPFASingleRenderer,
        0, 0, /* reserve spots for kCGLPFAOpenGLProfile, kCGLOGLPVersion_3_2_Core */
        0
    };
    CGError err;
    CGLPixelFormatObj pixel_format;
    GLint virtual_screens;

    if (core)
    {
        attrs[3] = kCGLPFAOpenGLProfile;
        attrs[4] = (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core;
    }

    if (!get_renderer_property(renderer_info, renderer, kCGLRPRendererID, &renderer_id))
        return NULL;

    attrs[1] = renderer_id;
    err = CGLChoosePixelFormat(attrs, &pixel_format, &virtual_screens);
    if (err != kCGLNoError)
        pixel_format = NULL;
    return pixel_format;
}


/**********************************************************************
 *              map_renderer_index
 *
 * We can't create pixel formats for all renderers listed.  For example,
 * in a dual-GPU system, the integrated GPU is typically unavailable
 * when the discrete GPU is active.
 *
 * This function conceptually creates a list of "good" renderers from the
 * list of all renderers.  It treats the input "renderer" parameter as an
 * index into that list of good renderers and returns the corresponding
 * index into the list of all renderers.
 */
static GLint map_renderer_index(CGLRendererInfoObj renderer_info, GLint renderer_count, GLint renderer)
{
    GLint good_count, i;

    good_count = 0;
    for (i = 0; i < renderer_count; i++)
    {
        CGLPixelFormatObj pix = create_pixel_format_for_renderer(renderer_info, i, FALSE);
        if (pix)
        {
            CGLReleasePixelFormat(pix);
            good_count++;
            if (good_count > renderer)
                break;
        }
        else
            TRACE("skipping bad renderer %d\n", i);
    }

    TRACE("mapped requested renderer %d to index %d\n", renderer, i);
    return i;
}


/**********************************************************************
 *              get_gl_string
 */
static const char* get_gl_string(CGLPixelFormatObj pixel_format, GLenum name)
{
    const char* ret = NULL;
    CGLContextObj context, old_context;
    CGLError err;

    err = CGLCreateContext(pixel_format, NULL, &context);
    if (err == kCGLNoError && context)
    {
        old_context = CGLGetCurrentContext();
        err = CGLSetCurrentContext(context);
        if (err == kCGLNoError)
        {
            ret = (const char*)pglGetString(name);
            CGLSetCurrentContext(old_context);
        }
        else
            WARN("CGLSetCurrentContext failed: %d %s\n", err, CGLErrorString(err));
        CGLReleaseContext(context);
    }
    else
        WARN("CGLCreateContext failed: %d %s\n", err, CGLErrorString(err));

    return ret;
}


/**********************************************************************
 *              get_fallback_renderer_version
 */
static void get_fallback_renderer_version(GLuint *value)
{
    BOOL got_it = FALSE;
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, CFSTR("/System/Library/Frameworks/OpenGL.framework"),
                                                 kCFURLPOSIXPathStyle, TRUE);
    if (url)
    {
        CFBundleRef bundle = CFBundleCreate(NULL, url);
        CFRelease(url);
        if (bundle)
        {
            CFStringRef version = CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey);
            if (version && CFGetTypeID(version) == CFStringGetTypeID())
            {
                size_t len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(version), kCFStringEncodingUTF8);
                char* buf = malloc(len);
                if (buf && CFStringGetCString(version, buf, len, kCFStringEncodingUTF8))
                {
                    unsigned int major, minor, bugfix;
                    int count = sscanf(buf, "%u.%u.%u", &major, &minor, &bugfix);
                    if (count >= 2)
                    {
                        value[0] = major;
                        value[1] = minor;
                        if (count == 3)
                            value[2] = bugfix;
                        else
                            value[2] = 0;
                        got_it = TRUE;
                    }
                }
                free(buf);
            }
            CFRelease(bundle);
        }
    }

    if (!got_it)
    {
        /* Use the version of the OpenGL framework from OS X 10.6, which is the
           earliest version that the Mac driver supports. */
        value[0] = 1;
        value[1] = 6;
        value[2] = 14;
    }
}


/**********************************************************************
 *              parse_renderer_version
 *
 * Get the renderer version from the OpenGL version string.  Assumes
 * the string is of the form
 * <GL major>.<GL minor>[.<GL bugfix>] <vendor>-<major>.<minor>[.<bugfix>]
 * where major, minor, and bugfix are what we're interested in.  This
 * form for the vendor specific information is not generally applicable,
 * but seems reliable on OS X.
 */
static BOOL parse_renderer_version(const char* version, GLuint *value)
{
    const char* p = strchr(version, ' ');
    int count;
    unsigned int major, minor, bugfix;

    if (p) p = strchr(p + 1, '-');
    if (!p) return FALSE;

    count = sscanf(p + 1, "%u.%u.%u", &major, &minor, &bugfix);
    if (count < 2)
        return FALSE;

    value[0] = major;
    value[1] = minor;
    if (count == 3)
        value[2] = bugfix;
    else
        value[2] = 0;

    return TRUE;
}


/**********************************************************************
 *              query_renderer_integer
 */
static BOOL query_renderer_integer(CGLRendererInfoObj renderer_info, GLint renderer, GLenum attribute, GLuint *value)
{
    BOOL ret = FALSE;
    CGLError err;

    if (TRACE_ON(wgl))
    {
        GLint renderer_id;
        if (!get_renderer_property(renderer_info, renderer, kCGLRPRendererID, &renderer_id))
            renderer_id = 0;
        TRACE("renderer %d (ID 0x%08x) attribute 0x%04x value %p\n", renderer, renderer_id, attribute, value);
    }

    switch (attribute)
    {
        case WGL_RENDERER_ACCELERATED_WINE:
            if (!get_renderer_property(renderer_info, renderer, kCGLRPAccelerated, (GLint*)value))
                break;
            *value = !!*value;
            ret = TRUE;
            TRACE("WGL_RENDERER_ACCELERATED_WINE -> %u\n", *value);
            break;

        case WGL_RENDERER_DEVICE_ID_WINE:
            ret = get_iokit_display_property(renderer_info, renderer, CFSTR("device-id"), value);
            if (!ret)
            {
                *value = 0xffffffff;
                ret = TRUE;
            }
            TRACE("WGL_RENDERER_DEVICE_ID_WINE -> 0x%04x\n", *value);
            break;

        case WGL_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_WINE:
        case WGL_RENDERER_OPENGL_CORE_PROFILE_VERSION_WINE:
        {
            BOOL core = (attribute == WGL_RENDERER_OPENGL_CORE_PROFILE_VERSION_WINE);
            CGLPixelFormatObj pixel_format = create_pixel_format_for_renderer(renderer_info, renderer, core);

            if (pixel_format)
            {
                const char* version = get_gl_string(pixel_format, GL_VERSION);

                CGLReleasePixelFormat(pixel_format);
                if (version)
                {
                    unsigned int major, minor;

                    if (sscanf(version, "%u.%u", &major, &minor) == 2)
                    {
                        value[0] = major;
                        value[1] = minor;
                        ret = TRUE;
                    }
                }
            }

            if (!ret)
            {
                value[0] = value[1] = 0;
                ret = TRUE;
            }
            TRACE("%s -> %u.%u\n", core ? "WGL_RENDERER_OPENGL_CORE_PROFILE_VERSION_WINE" :
                  "WGL_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_WINE", value[0], value[1]);
            break;
        }

        case WGL_RENDERER_PREFERRED_PROFILE_WINE:
        {
            CGLPixelFormatObj pixel_format = create_pixel_format_for_renderer(renderer_info, renderer, TRUE);

            if (pixel_format)
            {
                CGLReleasePixelFormat(pixel_format);
                *value = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
                TRACE("WGL_RENDERER_PREFERRED_PROFILE_WINE -> WGL_CONTEXT_CORE_PROFILE_BIT_ARB (0x%04x)\n", *value);
            }
            else
            {
                *value = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                TRACE("WGL_RENDERER_PREFERRED_PROFILE_WINE -> WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB (0x%04x)\n", *value);
            }
            ret = TRUE;
            break;
        }

        case WGL_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_WINE:
            /* FIXME: no API to query this */
            *value = 0;
            ret = TRUE;
            TRACE("WGL_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_WINE -> %u\n", *value);
            break;

        case WGL_RENDERER_VENDOR_ID_WINE:
            ret = get_iokit_display_property(renderer_info, renderer, CFSTR("vendor-id"), value);
            if (!ret)
            {
                *value = 0xffffffff;
                ret = TRUE;
            }
            TRACE("WGL_RENDERER_VENDOR_ID_WINE -> 0x%04x\n", *value);
            break;

        case WGL_RENDERER_VERSION_WINE:
        {
            CGLPixelFormatObj pixel_format = create_pixel_format_for_renderer(renderer_info, renderer, TRUE);

            if (!pixel_format)
                pixel_format = create_pixel_format_for_renderer(renderer_info, renderer, FALSE);
            if (pixel_format)
            {
                const char* version = get_gl_string(pixel_format, GL_VERSION);

                CGLReleasePixelFormat(pixel_format);
                if (version)
                    ret = parse_renderer_version(version, value);
            }

            if (!ret)
            {
                get_fallback_renderer_version(value);
                ret = TRUE;
            }
            TRACE("WGL_RENDERER_VERSION_WINE -> %u.%u.%u\n", value[0], value[1], value[2]);
            break;
        }

        case WGL_RENDERER_VIDEO_MEMORY_WINE:
            err = CGLDescribeRenderer(renderer_info, renderer, kCGLRPVideoMemoryMegabytes, (GLint*)value);
            if (err != kCGLNoError && err != kCGLBadProperty)
                WARN("CGLDescribeRenderer(kCGLRPVideoMemoryMegabytes) failed: %d %s\n", err, CGLErrorString(err));
            if (err != kCGLNoError)
            {
                if (get_renderer_property(renderer_info, renderer, kCGLRPVideoMemory, (GLint*)value))
                    *value /= 1024 * 1024;
                else
                    *value = 0;
            }
            ret = TRUE;
            TRACE("WGL_RENDERER_VIDEO_MEMORY_WINE -> %uMB\n", *value);
            break;

        default:
            FIXME("unrecognized attribute 0x%04x\n", attribute);
            break;
    }

    return ret;
}


/**********************************************************************
 *              macdrv_glCopyColorTable
 *
 * Hook into glCopyColorTable as part of the implementation of
 * wglMakeContextCurrentARB.  If the context has a separate readable,
 * temporarily make that current, do glCopyColorTable, and then set it
 * back to the drawable.  This is modeled after what Mesa GLX's Apple
 * implementation does.
 */
static void macdrv_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y,
                                    GLsizei width)
{
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;

    if (context->read_view || context->read_pbuffer)
        make_context_current(context, TRUE);

    pglCopyColorTable(target, internalformat, x, y, width);

    if (context->read_view || context->read_pbuffer)
        make_context_current(context, FALSE);
}


/**********************************************************************
 *              macdrv_glCopyPixels
 *
 * Hook into glCopyPixels as part of the implementation of
 * wglMakeContextCurrentARB.  If the context has a separate readable,
 * temporarily make that current, do glCopyPixels, and then set it back
 * to the drawable.  This is modeled after what Mesa GLX's Apple
 * implementation does.
 */
static void macdrv_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;

    if (context->read_view || context->read_pbuffer)
        make_context_current(context, TRUE);

    pglCopyPixels(x, y, width, height, type);

    if (context->read_view || context->read_pbuffer)
        make_context_current(context, FALSE);
}


static BOOL macdrv_context_flush( void *private, HWND hwnd, HDC hdc, int interval, void (*flush)(void) )
{
    struct macdrv_context *context = private;

    set_swap_interval(context, interval);
    sync_context(context);

    if (skip_single_buffer_flushes)
    {
        const pixel_format *pf = &pixel_formats[context->format - 1];
        unsigned int now = NtGetTickCount();

        TRACE("double buffer %d last flush time %d now %d\n", pf->double_buffer,
              context->last_flush_time, now);
        if (pglFlushRenderAPPLE && !pf->double_buffer && (now - context->last_flush_time) < 17)
        {
            TRACE("calling glFlushRenderAPPLE()\n");
            pglFlushRenderAPPLE();
            return TRUE;
        }
        else
        {
            TRACE("calling glFlush()\n");
            context->last_flush_time = now;
        }
    }

    return FALSE;
}


/**********************************************************************
 *              macdrv_glGetString
 *
 * Hook into glGetString in order to return some legacy WGL extensions
 * that couldn't be advertised via the standard
 * WGL_ARB_extensions_string mechanism. Some programs, especially
 * older ones, expect to find certain older extensions, such as
 * WGL_EXT_extensions_string itself, in the standard GL extensions
 * string, and won't query any other WGL extensions unless they find
 * that particular extension there.
 */
static const GLubyte *macdrv_glGetString(GLenum name)
{
    if (name == GL_EXTENSIONS && gl_info.glExtensions)
        return (const GLubyte *)gl_info.glExtensions;
    else
        return pglGetString(name);
}


/**********************************************************************
 *              macdrv_glReadPixels
 *
 * Hook into glReadPixels as part of the implementation of
 * wglMakeContextCurrentARB.  If the context has a separate readable,
 * temporarily make that current, do glReadPixels, and then set it back
 * to the drawable.  This is modeled after what Mesa GLX's Apple
 * implementation does.
 */
static void macdrv_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                                GLenum format, GLenum type, void *pixels)
{
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;

    if (context->read_view || context->read_pbuffer)
        make_context_current(context, TRUE);

    pglReadPixels(x, y, width, height, format, type, pixels);

    if (context->read_view || context->read_pbuffer)
        make_context_current(context, FALSE);
}


/**********************************************************************
 *              macdrv_glViewport
 *
 * Hook into glViewport as an opportunity to update the OpenGL context
 * if necessary.  This is modeled after what Mesa GLX's Apple
 * implementation does.
 */
static void macdrv_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;

    sync_context(context);
    macdrv_update_opengl_context(context->context);
    pglViewport(x, y, width, height);
}


/***********************************************************************
 *              macdrv_wglBindTexImageARB
 *
 * WGL_ARB_render_texture: wglBindTexImageARB
 */
static UINT macdrv_pbuffer_bind(HDC hdc, void *private, GLenum source)
{
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;
    CGLPBufferObj pbuffer = private;
    CGLError err;

    TRACE("hdc %p pbuffer %p source 0x%x\n", hdc, pbuffer, source);

    if (!context->draw_view && context->draw_pbuffer == pbuffer && source != GL_NONE)
        funcs->p_glFlush();

    err = CGLTexImagePBuffer(context->cglcontext, pbuffer, source);
    if (err != kCGLNoError)
    {
        WARN("CGLTexImagePBuffer failed with err %d %s\n", err, CGLErrorString(err));
        RtlSetLastWin32Error(ERROR_INVALID_OPERATION);
        return GL_FALSE;
    }

    return GL_TRUE;
}

/***********************************************************************
 *              macdrv_wglCreateContextAttribsARB
 *
 * WGL_ARB_create_context: wglCreateContextAttribsARB
 */
static BOOL macdrv_context_create(HDC hdc, int format, void *shared, const int *attrib_list, void **private)
{
    struct macdrv_context *share_context = shared;
    struct macdrv_context *context;
    const int *iptr;
    int major = 1, minor = 0, profile = WGL_CONTEXT_CORE_PROFILE_BIT_ARB, flags = 0;
    BOOL core = FALSE;
    GLint renderer_id = 0;

    TRACE("hdc %p, format %d, share_context %p, attrib_list %p\n", hdc, format, share_context, attrib_list);

    for (iptr = attrib_list; iptr && *iptr; iptr += 2)
    {
        int attr = iptr[0];
        int value = iptr[1];

        TRACE("%s\n", debugstr_attrib(attr, value));

        switch (attr)
        {
            case WGL_CONTEXT_MAJOR_VERSION_ARB:
                major = value;
                break;

            case WGL_CONTEXT_MINOR_VERSION_ARB:
                minor = value;
                break;

            case WGL_CONTEXT_LAYER_PLANE_ARB:
                WARN("WGL_CONTEXT_LAYER_PLANE_ARB attribute ignored\n");
                break;

            case WGL_CONTEXT_FLAGS_ARB:
                flags = value;
                if (flags & ~WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
                    WARN("WGL_CONTEXT_FLAGS_ARB attributes %#x ignored\n",
                         flags & ~WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB);
                break;

            case WGL_CONTEXT_PROFILE_MASK_ARB:
                if (value != WGL_CONTEXT_CORE_PROFILE_BIT_ARB &&
                    value != WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB)
                {
                    WARN("WGL_CONTEXT_PROFILE_MASK_ARB bits %#x invalid\n", value);
                    RtlSetLastWin32Error(ERROR_INVALID_PROFILE_ARB);
                    return FALSE;
                }
                profile = value;
                break;

            case WGL_RENDERER_ID_WINE:
            {
                CGLError err;
                CGLRendererInfoObj renderer_info;
                GLint renderer_count, temp;

                err = CGLQueryRendererInfo(active_displays_mask(), &renderer_info, &renderer_count);
                if (err != kCGLNoError)
                {
                    WARN("CGLQueryRendererInfo failed: %d %s\n", err, CGLErrorString(err));
                    RtlSetLastWin32Error(ERROR_GEN_FAILURE);
                    return FALSE;
                }

                value = map_renderer_index(renderer_info, renderer_count, value);

                if (value >= renderer_count)
                {
                    WARN("WGL_RENDERER_ID_WINE renderer %d exceeds count (%d)\n", value, renderer_count);
                    CGLDestroyRendererInfo(renderer_info);
                    RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }

                if (!get_renderer_property(renderer_info, value, kCGLRPRendererID, &temp))
                {
                    WARN("WGL_RENDERER_ID_WINE failed to get ID of renderer %d\n", value);
                    CGLDestroyRendererInfo(renderer_info);
                    RtlSetLastWin32Error(ERROR_GEN_FAILURE);
                    return FALSE;
                }

                CGLDestroyRendererInfo(renderer_info);

                if (renderer_id && temp != renderer_id)
                {
                    WARN("WGL_RENDERER_ID_WINE requested two different renderers (0x%08x vs. 0x%08x)\n", renderer_id, temp);
                    RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }
                renderer_id = temp;
                break;
            }

            default:
                WARN("Unknown attribute %s.\n", debugstr_attrib(attr, value));
                RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
                return FALSE;
        }
    }

    if ((major == 3 && (minor == 2 || minor == 3)) ||
        (major == 4 && (minor == 0 || minor == 1)))
    {
        if (!(flags & WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB))
        {
            WARN("OS X only supports forward-compatible 3.2+ contexts\n");
            RtlSetLastWin32Error(ERROR_INVALID_VERSION_ARB);
            return FALSE;
        }
        if (profile != WGL_CONTEXT_CORE_PROFILE_BIT_ARB)
        {
            WARN("Compatibility profiles for GL version >= 3.2 not supported\n");
            RtlSetLastWin32Error(ERROR_INVALID_PROFILE_ARB);
            return FALSE;
        }
        if (major > gl_info.max_major ||
            (major == gl_info.max_major && minor > gl_info.max_minor))
        {
            WARN("This GL implementation does not support the requested GL version %u.%u\n",
                 major, minor);
            RtlSetLastWin32Error(ERROR_INVALID_PROFILE_ARB);
            return FALSE;
        }
        core = TRUE;
    }
    else if (major >= 3)
    {
        WARN("Profile version %u.%u not supported\n", major, minor);
        RtlSetLastWin32Error(ERROR_INVALID_VERSION_ARB);
        return FALSE;
    }
    else if (major < 1 || (major == 1 && (minor < 0 || minor > 5)) ||
             (major == 2 && (minor < 0 || minor > 1)))
    {
        WARN("Invalid GL version requested\n");
        RtlSetLastWin32Error(ERROR_INVALID_VERSION_ARB);
        return FALSE;
    }
    if (!core && flags & WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
    {
        WARN("Forward compatible context requested for GL version < 3\n");
        RtlSetLastWin32Error(ERROR_INVALID_VERSION_ARB);
        return FALSE;
    }

    if (!(context = calloc(1, sizeof(*context)))) return FALSE;

    context->format = format;
    context->renderer_id = renderer_id;
    if (!create_context(context, share_context ? share_context->cglcontext : NULL, major))
    {
        free(context);
        return FALSE;
    }

    pthread_mutex_lock(&context_mutex);
    list_add_tail(&context_list, &context->entry);
    pthread_mutex_unlock(&context_mutex);

    *private = context;
    return TRUE;
}

static BOOL macdrv_pbuffer_create(HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                  GLint max_level, GLsizei *width, GLsizei *height, void **private)
{
    CGLError err;

    TRACE("hdc %p, format %d, largest %u, texture_format %#x, texture_target %#x, max_level %#x, width %d, height %d, private %p\n",
          hdc, format, largest, texture_format, texture_target, max_level, *width, *height, private);

    if (!texture_target || !texture_format)
    {
        /* no actual way to turn off ability to texture; use most permissive target */
        texture_target = GL_TEXTURE_RECTANGLE;
        texture_format = GL_RGB;
    }

    err = CGLCreatePBuffer(*width, *height, texture_target, texture_format, max_level, (CGLPBufferObj *)private);
    if (err != kCGLNoError)
    {
        WARN("CGLCreatePBuffer failed; err %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    pthread_mutex_lock(&dc_pbuffers_mutex);
    CFDictionarySetValue(dc_pbuffers, hdc, private);
    pthread_mutex_unlock(&dc_pbuffers_mutex);

    TRACE(" -> %p\n", *private);
    return TRUE;
}

static BOOL macdrv_pbuffer_destroy(HDC hdc, void *private)
{
    TRACE("private %p\n", private);

    pthread_mutex_lock(&dc_pbuffers_mutex);
    CFDictionaryRemoveValue(dc_pbuffers, hdc);
    pthread_mutex_unlock(&dc_pbuffers_mutex);

    CGLReleasePBuffer(private);
    return GL_TRUE;
}


static BOOL macdrv_context_make_current(HDC draw_hdc, HDC read_hdc, void *private)
{
    struct macdrv_context *context = private;
    struct macdrv_win_data *data;
    HWND hwnd;

    TRACE("draw_hdc %p read_hdc %p context %p/%p/%p\n", draw_hdc, read_hdc, context,
          (context ? context->context : NULL), (context ? context->cglcontext : NULL));

    if (!private)
    {
        macdrv_make_context_current(NULL, NULL, CGRectNull);
        NtCurrentTeb()->glReserved2 = NULL;
        return TRUE;
    }

    if ((hwnd = NtUserWindowFromDC(draw_hdc)))
    {
        if (!(data = get_win_data(hwnd)))
        {
            FIXME("draw DC for window %p of other process: not implemented\n", hwnd);
            return FALSE;
        }

        context->draw_hwnd = hwnd;
        context->draw_view = data->client_cocoa_view;
        context->draw_rect = data->rects.client;
        OffsetRect(&context->draw_rect, -data->rects.visible.left, -data->rects.visible.top);
        NtUserGetClientRect(hwnd, &context->draw_rect_win_dpi, NtUserGetDpiForWindow(hwnd));
        context->draw_pbuffer = NULL;
        release_win_data(data);
    }
    else
    {
        CGLPBufferObj pbuffer;

        pthread_mutex_lock(&dc_pbuffers_mutex);
        pbuffer = (CGLPBufferObj)CFDictionaryGetValue(dc_pbuffers, draw_hdc);
        if (!pbuffer)
        {
            WARN("no window or pbuffer for DC\n");
            pthread_mutex_unlock(&dc_pbuffers_mutex);
            RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        context->draw_hwnd = NULL;
        context->draw_view = NULL;
        context->draw_pbuffer = pbuffer;
        pthread_mutex_unlock(&dc_pbuffers_mutex);
    }

    context->read_view = NULL;
    context->read_pbuffer = NULL;
    if (read_hdc && read_hdc != draw_hdc)
    {
        if ((hwnd = NtUserWindowFromDC(read_hdc)))
        {
            if ((data = get_win_data(hwnd)))
            {
                if (data->client_cocoa_view != context->draw_view)
                {
                    context->read_view = data->client_cocoa_view;
                    context->read_rect = data->rects.client;
                    OffsetRect(&context->read_rect, -data->rects.visible.left, -data->rects.visible.top);
                }
                release_win_data(data);
            }
        }
        else
        {
            pthread_mutex_lock(&dc_pbuffers_mutex);
            context->read_pbuffer = (CGLPBufferObj)CFDictionaryGetValue(dc_pbuffers, read_hdc);
            pthread_mutex_unlock(&dc_pbuffers_mutex);
        }
    }

    TRACE("making context current with draw_view %p %s draw_pbuffer %p read_view %p %s read_pbuffer %p format %u\n",
          context->draw_view, wine_dbgstr_rect(&context->draw_rect), context->draw_pbuffer,
          context->read_view, wine_dbgstr_rect(&context->read_rect), context->read_pbuffer, context->format);

    make_context_current(context, FALSE);
    NtCurrentTeb()->glReserved2 = context;

    return TRUE;
}


/**********************************************************************
 *              macdrv_wglQueryCurrentRendererIntegerWINE
 *
 * WGL_WINE_query_renderer: wglQueryCurrentRendererIntegerWINE
 */
static BOOL macdrv_wglQueryCurrentRendererIntegerWINE(GLenum attribute, GLuint *value)
{
    BOOL ret = FALSE;
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;
    CGLPixelFormatObj pixel_format;
    CGLError err;
    GLint virtual_screen;
    GLint display_mask;
    GLint pf_renderer_id;
    CGLRendererInfoObj renderer_info;
    GLint renderer_count;
    GLint renderer;

    TRACE("context %p/%p/%p attribute 0x%04x value %p\n", context, (context ? context->context : NULL),
          (context ? context->cglcontext : NULL), attribute, value);

    if (attribute == WGL_RENDERER_VERSION_WINE)
    {
        if (!parse_renderer_version((const char*)pglGetString(GL_VERSION), value))
            get_fallback_renderer_version(value);
        TRACE("WGL_RENDERER_VERSION_WINE -> %u.%u.%u\n", value[0], value[1], value[2]);
        return TRUE;
    }

    pixel_format = CGLGetPixelFormat(context->cglcontext);
    err = CGLGetVirtualScreen(context->cglcontext, &virtual_screen);
    if (err != kCGLNoError)
    {
        WARN("CGLGetVirtualScreen failed: %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    err = CGLDescribePixelFormat(pixel_format, virtual_screen, kCGLPFADisplayMask, &display_mask);
    if (err != kCGLNoError)
    {
        WARN("CGLDescribePixelFormat(kCGLPFADisplayMask) failed: %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    err = CGLDescribePixelFormat(pixel_format, virtual_screen, kCGLPFARendererID, &pf_renderer_id);
    if (err != kCGLNoError)
    {
        WARN("CGLDescribePixelFormat(kCGLPFARendererID) failed: %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    err = CGLQueryRendererInfo(display_mask, &renderer_info, &renderer_count);
    if (err != kCGLNoError)
    {
        WARN("CGLQueryRendererInfo failed: %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    for (renderer = 0; renderer < renderer_count; renderer++)
    {
        GLint renderer_id;

        if (!get_renderer_property(renderer_info, renderer, kCGLRPRendererID, &renderer_id))
            continue;

        if (renderer_id == pf_renderer_id)
        {
            ret = query_renderer_integer(renderer_info, renderer, attribute, value);
            break;
        }
    }

    if (renderer >= renderer_count)
        ERR("failed to find renderer ID 0x%08x for display mask 0x%08x\n", pf_renderer_id, display_mask);

    CGLDestroyRendererInfo(renderer_info);
    return ret;
}


/**********************************************************************
 *              macdrv_wglQueryCurrentRendererStringWINE
 *
 * WGL_WINE_query_renderer: wglQueryCurrentRendererStringWINE
 */
static const char *macdrv_wglQueryCurrentRendererStringWINE(GLenum attribute)
{
    const char* ret = NULL;
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;

    TRACE("context %p/%p/%p attribute 0x%04x\n", context, (context ? context->context : NULL),
          (context ? context->cglcontext : NULL), attribute);

    switch (attribute)
    {
        case WGL_RENDERER_DEVICE_ID_WINE:
        {
            ret = (const char*)pglGetString(GL_RENDERER);
            TRACE("WGL_RENDERER_DEVICE_ID_WINE -> %s\n", debugstr_a(ret));
            break;
        }

        case WGL_RENDERER_VENDOR_ID_WINE:
        {
            ret = (const char*)pglGetString(GL_VENDOR);
            TRACE("WGL_RENDERER_VENDOR_ID_WINE -> %s\n", debugstr_a(ret));
            break;
        }

        default:
            FIXME("unrecognized attribute 0x%04x\n", attribute);
            break;
    }

    return ret;
}


/**********************************************************************
 *              macdrv_wglQueryRendererIntegerWINE
 *
 * WGL_WINE_query_renderer: wglQueryRendererIntegerWINE
 */
static BOOL macdrv_wglQueryRendererIntegerWINE(HDC dc, GLint renderer, GLenum attribute, GLuint *value)
{
    BOOL ret = FALSE;
    CGLRendererInfoObj renderer_info;
    GLint renderer_count;
    CGLError err;

    TRACE("dc %p renderer %d attribute 0x%04x value %p\n", dc, renderer, attribute, value);

    err = CGLQueryRendererInfo(active_displays_mask(), &renderer_info, &renderer_count);
    if (err != kCGLNoError)
    {
        WARN("CGLQueryRendererInfo failed: %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    renderer = map_renderer_index(renderer_info, renderer_count, renderer);

    if (renderer < renderer_count)
        ret = query_renderer_integer(renderer_info, renderer, attribute, value);
    else
        TRACE("requested information for renderer %d exceeding count %d\n", renderer, renderer_count);

    CGLDestroyRendererInfo(renderer_info);
    return ret;
}


/**********************************************************************
 *              macdrv_wglQueryRendererStringWINE
 *
 * WGL_WINE_query_renderer: wglQueryRendererStringWINE
 */
static const char *macdrv_wglQueryRendererStringWINE(HDC dc, GLint renderer, GLenum attribute)
{
    const char* ret = NULL;
    CGLRendererInfoObj renderer_info;
    GLint renderer_count;
    CGLError err;

    TRACE("dc %p renderer %d attribute 0x%04x\n", dc, renderer, attribute);

    err = CGLQueryRendererInfo(active_displays_mask(), &renderer_info, &renderer_count);
    if (err != kCGLNoError)
    {
        WARN("CGLQueryRendererInfo failed: %d %s\n", err, CGLErrorString(err));
        return FALSE;
    }

    renderer = map_renderer_index(renderer_info, renderer_count, renderer);

    if (renderer >= renderer_count)
    {
        TRACE("requested information for renderer %d exceeding count %d\n", renderer, renderer_count);
        goto done;
    }

    switch (attribute)
    {
        case WGL_RENDERER_DEVICE_ID_WINE:
        case WGL_RENDERER_VENDOR_ID_WINE:
        {
            BOOL device = (attribute == WGL_RENDERER_DEVICE_ID_WINE);
            CGLPixelFormatObj pixel_format = create_pixel_format_for_renderer(renderer_info, renderer, TRUE);

            if (!pixel_format)
                pixel_format = create_pixel_format_for_renderer(renderer_info, renderer, FALSE);
            if (pixel_format)
            {
                ret = get_gl_string(pixel_format, device ? GL_RENDERER : GL_VENDOR);
                CGLReleasePixelFormat(pixel_format);
            }

            TRACE("%s -> %s\n", device ? "WGL_RENDERER_DEVICE_ID_WINE" : "WGL_RENDERER_VENDOR_ID_WINE", debugstr_a(ret));
            break;
        }

        default:
            FIXME("unrecognized attribute 0x%04x\n", attribute);
            break;
    }

done:
    CGLDestroyRendererInfo(renderer_info);
    return ret;
}


static BOOL macdrv_pbuffer_updated(HDC hdc, void *private, GLenum cube_face, GLint mipmap_level)
{
    struct macdrv_context *context = NtCurrentTeb()->glReserved2;
    CGLPBufferObj pbuffer = private;

    TRACE("hdc %p pbuffer %p cube_face %#x mipmap_level %d\n", hdc, pbuffer, cube_face, mipmap_level);

    if (context && context->draw_pbuffer == pbuffer)
    {
        context->draw_pbuffer_face = cube_face;
        context->draw_pbuffer_level = mipmap_level;
        make_context_current(context, FALSE);
    }

    return GL_TRUE;
}

static void register_extension(const char *ext)
{
    if (gl_info.wglExtensions[0])
        strcat(gl_info.wglExtensions, " ");
    strcat(gl_info.wglExtensions, ext);

    TRACE("'%s'\n", ext);
}

static const char *macdrv_init_wgl_extensions(struct opengl_funcs *funcs)
{
    /*
     * ARB Extensions
     */

    if (gluCheckExtension((GLubyte*)"GL_ARB_color_buffer_float", (GLubyte*)gl_info.glExtensions))
    {
        register_extension("WGL_ARB_pixel_format_float");
        register_extension("WGL_ATI_pixel_format_float");
    }

    if (gluCheckExtension((GLubyte*)"GL_ARB_multisample", (GLubyte*)gl_info.glExtensions))
        register_extension("WGL_ARB_multisample");

    if (gluCheckExtension((GLubyte*)"GL_ARB_framebuffer_sRGB", (GLubyte*)gl_info.glExtensions))
        register_extension("WGL_ARB_framebuffer_sRGB");

    if (gluCheckExtension((GLubyte*)"GL_APPLE_pixel_buffer", (GLubyte*)gl_info.glExtensions))
    {
        if (gluCheckExtension((GLubyte*)"GL_ARB_texture_rectangle", (GLubyte*)gl_info.glExtensions) ||
            gluCheckExtension((GLubyte*)"GL_EXT_texture_rectangle", (GLubyte*)gl_info.glExtensions))
            register_extension("WGL_NV_render_texture_rectangle");
    }

    /* Presumably identical to [W]GL_ARB_framebuffer_sRGB, above, but clients may
       check for either, so register them separately. */
    if (gluCheckExtension((GLubyte*)"GL_EXT_framebuffer_sRGB", (GLubyte*)gl_info.glExtensions))
        register_extension("WGL_EXT_framebuffer_sRGB");

    if (gluCheckExtension((GLubyte*)"GL_EXT_packed_float", (GLubyte*)gl_info.glExtensions))
        register_extension("WGL_EXT_pixel_format_packed_float");

    /*
     * WINE-specific WGL Extensions
     */

    register_extension("WGL_WINE_query_renderer");
    funcs->p_wglQueryCurrentRendererIntegerWINE = macdrv_wglQueryCurrentRendererIntegerWINE;
    funcs->p_wglQueryCurrentRendererStringWINE = macdrv_wglQueryCurrentRendererStringWINE;
    funcs->p_wglQueryRendererIntegerWINE = macdrv_wglQueryRendererIntegerWINE;
    funcs->p_wglQueryRendererStringWINE = macdrv_wglQueryRendererStringWINE;

    return gl_info.wglExtensions;
}

/**********************************************************************
 *              macdrv_OpenGLInit
 */
UINT macdrv_OpenGLInit(UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs)
{
    TRACE("()\n");

    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR("version mismatch, opengl32 wants %u but macdrv has %u\n", version, WINE_OPENGL_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }
    funcs = opengl_funcs;

    dc_pbuffers = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
    if (!dc_pbuffers)
    {
        WARN("CFDictionaryCreateMutable failed\n");
        return STATUS_NOT_SUPPORTED;
    }

    opengl_handle = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY|RTLD_LOCAL|RTLD_NOLOAD);
    if (!opengl_handle)
    {
        ERR("Failed to load OpenGL: %s\n", dlerror());
        ERR("OpenGL support is disabled.\n");
        return STATUS_NOT_SUPPORTED;
    }

#define LOAD_FUNCPTR(func) \
        if (!(p##func = dlsym(opengl_handle, #func))) \
        { \
            ERR( "%s not found in libGL, disabling OpenGL.\n", #func ); \
            goto failed; \
        }
    LOAD_FUNCPTR(glCopyPixels);
    LOAD_FUNCPTR(glGetIntegerv);
    LOAD_FUNCPTR(glGetString);
    LOAD_FUNCPTR(glReadPixels);
    LOAD_FUNCPTR(glViewport);
    LOAD_FUNCPTR(glCopyColorTable);

    if (!init_gl_info())
        goto failed;

    if (gluCheckExtension((GLubyte*)"GL_APPLE_flush_render", (GLubyte*)gl_info.glExtensions))
        LOAD_FUNCPTR(glFlushRenderAPPLE);
#undef LOAD_FUNCPTR

    *driver_funcs = &macdrv_driver_funcs;
    return STATUS_SUCCESS;

failed:
    dlclose(opengl_handle);
    opengl_handle = NULL;
    return STATUS_NOT_SUPPORTED;
}


/***********************************************************************
 *              sync_gl_view
 *
 * Synchronize the Mac GL view position with the Windows child window
 * position.
 */
void sync_gl_view(struct macdrv_win_data* data, const struct window_rects *old_rects)
{
    if (data->client_cocoa_view && data->pixel_format)
    {
        RECT old = old_rects->client, new = data->rects.client;

        OffsetRect(&old, -old_rects->visible.left, -old_rects->visible.top);
        OffsetRect(&new, -data->rects.visible.left, -data->rects.visible.top);
        if (!EqualRect(&old, &new))
        {
            TRACE("GL view %p changed position; marking contexts\n", data->client_cocoa_view);
            mark_contexts_for_moved_view(data->client_cocoa_view);
        }
    }
}


static BOOL macdrv_describe_pixel_format(int format, struct wgl_pixel_format *descr)
{
    const pixel_format *pf = pixel_formats + format - 1;
    const struct color_mode *mode;

    if (format <= 0 || format > nb_formats) return FALSE;

    memset(descr, 0, sizeof(*descr));
    descr->pfd.nSize        = sizeof(*descr);
    descr->pfd.nVersion     = 1;

    descr->pfd.dwFlags      = PFD_SUPPORT_OPENGL;
    if (pf->window)         descr->pfd.dwFlags |= PFD_DRAW_TO_WINDOW;
    if (!pf->accelerated)   descr->pfd.dwFlags |= PFD_GENERIC_FORMAT;
    else                    descr->pfd.dwFlags |= PFD_SUPPORT_COMPOSITION;
    if (pf->double_buffer)  descr->pfd.dwFlags |= PFD_DOUBLEBUFFER;
    if (pf->stereo)         descr->pfd.dwFlags |= PFD_STEREO;
    if (pf->backing_store)  descr->pfd.dwFlags |= PFD_SWAP_COPY;

    descr->pfd.iPixelType   = PFD_TYPE_RGBA;

    mode = &color_modes[pf->color_mode];
    /* If the mode doesn't have alpha, return bits per pixel instead of color bits.
       On Windows, color bits sometimes exceeds r+g+b (e.g. it's 32 for an
       R8G8B8A0 pixel format).  If an app depends on that and expects that
       cColorBits >= 32 for such a pixel format, we need to accommodate that. */
    if (mode->alpha_bits)
        descr->pfd.cColorBits = mode->color_bits;
    else
        descr->pfd.cColorBits = mode->bits_per_pixel;
    descr->pfd.cRedBits     = mode->red_bits;
    descr->pfd.cRedShift    = mode->red_shift;
    descr->pfd.cGreenBits   = mode->green_bits;
    descr->pfd.cGreenShift  = mode->green_shift;
    descr->pfd.cBlueBits    = mode->blue_bits;
    descr->pfd.cBlueShift   = mode->blue_shift;
    descr->pfd.cAlphaBits   = mode->alpha_bits;
    descr->pfd.cAlphaShift  = mode->alpha_shift;

    if (pf->accum_mode)
    {
        mode = &color_modes[pf->accum_mode - 1];
        descr->pfd.cAccumBits      = mode->color_bits;
        descr->pfd.cAccumRedBits   = mode->red_bits;
        descr->pfd.cAccumGreenBits = mode->green_bits;
        descr->pfd.cAccumBlueBits  = mode->blue_bits;
        descr->pfd.cAccumAlphaBits = mode->alpha_bits;
    }

    descr->pfd.cDepthBits   = pf->depth_bits;
    descr->pfd.cStencilBits = pf->stencil_bits;
    descr->pfd.cAuxBuffers  = pf->aux_buffers;
    descr->pfd.iLayerType   = PFD_MAIN_PLANE;

    if (pf->double_buffer && pf->backing_store) descr->swap_method = WGL_SWAP_COPY_ARB;
    else descr->swap_method = WGL_SWAP_UNDEFINED_ARB;

    /* WGL_EXT_pixel_format_packed_float may be supported, which should in theory
       make another pixel type available: WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT.
       However, Mac contexts don't support rendering to unsigned floating-point
       formats, even when GL_EXT_packed_float is supported. */
    if (color_modes[pf->color_mode].is_float) descr->pixel_type = WGL_TYPE_RGBA_FLOAT_ARB;
    else descr->pixel_type = WGL_TYPE_RGBA_ARB;

    descr->sample_buffers = pf->sample_buffers;
    descr->samples = pf->samples;

    /* sRGB is only supported for 8-bit integer color components */
    if (color_modes[pf->color_mode].red_bits == 8 &&
        color_modes[pf->color_mode].green_bits == 8 &&
        color_modes[pf->color_mode].blue_bits == 8 &&
        !color_modes[pf->color_mode].is_float)
        descr->framebuffer_srgb_capable = GL_TRUE;
    else
        descr->framebuffer_srgb_capable = GL_FALSE;

    descr->draw_to_pbuffer = pf->pbuffer ? GL_TRUE : GL_FALSE;
    descr->bind_to_texture_rgb = pf->pbuffer ? GL_TRUE : GL_FALSE;
    descr->bind_to_texture_rectangle_rgb = pf->pbuffer ? GL_TRUE : GL_FALSE;

    descr->bind_to_texture_rgba = (pf->pbuffer && color_modes[pf->color_mode].alpha_bits) ? GL_TRUE : GL_FALSE;
    descr->bind_to_texture_rectangle_rgba = (pf->pbuffer && color_modes[pf->color_mode].alpha_bits) ? GL_TRUE : GL_FALSE;

    descr->max_pbuffer_width = gl_info.max_viewport_dims[0];
    descr->max_pbuffer_height = gl_info.max_viewport_dims[1];
    descr->max_pbuffer_pixels = gl_info.max_viewport_dims[0] * gl_info.max_viewport_dims[1];

    return TRUE;
}

static BOOL macdrv_context_destroy(void *private)
{
    struct macdrv_context *context = private;

    TRACE("deleting context %p/%p/%p\n", context, context->context, context->cglcontext);

    pthread_mutex_lock(&context_mutex);
    list_remove(&context->entry);
    pthread_mutex_unlock(&context_mutex);

    macdrv_dispose_opengl_context(context->context);
    free(context);
    return TRUE;
}

static void *macdrv_get_proc_address(const char *name)
{
    /* redirect some standard OpenGL functions */
    if (!strcmp(name, "glCopyPixels")) return macdrv_glCopyPixels;
    if (!strcmp(name, "glGetString")) return macdrv_glGetString;
    if (!strcmp(name, "glReadPixels")) return macdrv_glReadPixels;
    if (!strcmp(name, "glViewport")) return macdrv_glViewport;

    /* redirect some OpenGL extension functions */
    if (!strcmp(name, "glCopyColorTable")) return macdrv_glCopyColorTable;
    return dlsym(opengl_handle, name);
}

static BOOL macdrv_swap_buffers(void *private, HWND hwnd, HDC hdc, int interval)
{
    struct macdrv_context *context = private;
    BOOL match = FALSE;

    TRACE("hdc %p context %p/%p/%p\n", hdc, context, (context ? context->context : NULL),
          (context ? context->cglcontext : NULL));

    if (context)
    {
        set_swap_interval(context, interval);
        sync_context(context);
    }

    if (hwnd)
    {
        struct macdrv_win_data *data;

        if (!(data = get_win_data(hwnd)))
        {
            RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        if (context && context->draw_view == data->client_cocoa_view)
            match = TRUE;

        release_win_data(data);
    }
    else
    {
        CGLPBufferObj pbuffer;

        pthread_mutex_lock(&dc_pbuffers_mutex);
        pbuffer = (CGLPBufferObj)CFDictionaryGetValue(dc_pbuffers, hdc);
        pthread_mutex_unlock(&dc_pbuffers_mutex);

        if (!pbuffer)
        {
            RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        if (context && context->draw_pbuffer == pbuffer)
            match = TRUE;
    }

    if (!match)
    {
        FIXME("current context %p doesn't match hdc %p; can't swap\n", context, hdc);
        return FALSE;
    }

    macdrv_flush_opengl_context(context->context);
    return TRUE;
}

static const struct opengl_driver_funcs macdrv_driver_funcs =
{
    .p_get_proc_address = macdrv_get_proc_address,
    .p_init_pixel_formats = macdrv_init_pixel_formats,
    .p_describe_pixel_format = macdrv_describe_pixel_format,
    .p_init_wgl_extensions = macdrv_init_wgl_extensions,
    .p_set_pixel_format = macdrv_set_pixel_format,
    .p_swap_buffers = macdrv_swap_buffers,
    .p_context_create = macdrv_context_create,
    .p_context_destroy = macdrv_context_destroy,
    .p_context_flush = macdrv_context_flush,
    .p_context_make_current = macdrv_context_make_current,
    .p_pbuffer_create = macdrv_pbuffer_create,
    .p_pbuffer_destroy = macdrv_pbuffer_destroy,
    .p_pbuffer_updated = macdrv_pbuffer_updated,
    .p_pbuffer_bind = macdrv_pbuffer_bind,
};
