/*
 * Functions to use the XRender extension
 *
 * Copyright 2001, 2002 Huw D M Davies for CodeWeavers
 * Copyright 2009 Roderick Colenbrander
 * Copyright 2011 Alexandre Julliard
 *
 * Some parts also:
 * Copyright 2000 Keith Packard, member of The XFree86 Project, Inc.
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "x11drv.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

int using_client_side_fonts = FALSE;

WINE_DEFAULT_DEBUG_CHANNEL(xrender);

#ifdef SONAME_LIBXRENDER

static BOOL X11DRV_XRender_Installed = FALSE;

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#ifndef RepeatNone  /* added in 0.10 */
#define RepeatNone    0
#define RepeatNormal  1
#define RepeatPad     2
#define RepeatReflect 3
#endif

enum wxr_format
{
  WXR_FORMAT_MONO,
  WXR_FORMAT_GRAY,
  WXR_FORMAT_X1R5G5B5,
  WXR_FORMAT_X1B5G5R5,
  WXR_FORMAT_R5G6B5,
  WXR_FORMAT_B5G6R5,
  WXR_FORMAT_R8G8B8,
  WXR_FORMAT_B8G8R8,
  WXR_FORMAT_A8R8G8B8,
  WXR_FORMAT_B8G8R8A8,
  WXR_FORMAT_X8R8G8B8,
  WXR_FORMAT_B8G8R8X8,
  WXR_NB_FORMATS,
  WXR_INVALID_FORMAT = WXR_NB_FORMATS
};

typedef struct wine_xrender_format_template
{
    unsigned int depth;
    unsigned int alpha;
    unsigned int alphaMask;
    unsigned int red;
    unsigned int redMask;
    unsigned int green;
    unsigned int greenMask;
    unsigned int blue;
    unsigned int blueMask;
} WineXRenderFormatTemplate;

static const WineXRenderFormatTemplate wxr_formats_template[WXR_NB_FORMATS] =
{
    /* Format               depth   alpha   mask    red     mask    green   mask    blue    mask*/
/* WXR_FORMAT_MONO     */ { 1,      0,      0x01,   0,      0,      0,      0,      0,      0       },
/* WXR_FORMAT_GRAY     */ { 8,      0,      0xff,   0,      0,      0,      0,      0,      0       },
/* WXR_FORMAT_X1R5G5B5 */ { 16,     0,      0,      10,     0x1f,   5,      0x1f,   0,      0x1f    },
/* WXR_FORMAT_X1B5G5R5 */ { 16,     0,      0,      0,      0x1f,   5,      0x1f,   10,     0x1f    },
/* WXR_FORMAT_R5G6B5   */ { 16,     0,      0,      11,     0x1f,   5,      0x3f,   0,      0x1f    },
/* WXR_FORMAT_B5G6R5   */ { 16,     0,      0,      0,      0x1f,   5,      0x3f,   11,     0x1f    },
/* WXR_FORMAT_R8G8B8   */ { 24,     0,      0,      16,     0xff,   8,      0xff,   0,      0xff    },
/* WXR_FORMAT_B8G8R8   */ { 24,     0,      0,      0,      0xff,   8,      0xff,   16,     0xff    },
/* WXR_FORMAT_A8R8G8B8 */ { 32,     24,     0xff,   16,     0xff,   8,      0xff,   0,      0xff    },
/* WXR_FORMAT_B8G8R8A8 */ { 32,     0,      0xff,   8,      0xff,   16,     0xff,   24,     0xff    },
/* WXR_FORMAT_X8R8G8B8 */ { 32,     0,      0,      16,     0xff,   8,      0xff,   0,      0xff    },
/* WXR_FORMAT_B8G8R8X8 */ { 32,     0,      0,      8,      0xff,   16,     0xff,   24,     0xff    },
};

static const ColorShifts wxr_color_shifts[WXR_NB_FORMATS] =
{
    /* format                phys red    phys green  phys blue   log red     log green   log blue */
/* WXR_FORMAT_MONO     */ { { 0,0,0 },  { 0,0,0 },  { 0,0,0 },  { 0,0,0 },  { 0,0,0 },  { 0,0,0 }  },
/* WXR_FORMAT_GRAY     */ { { 0,0,0 },  { 0,0,0 },  { 0,0,0 },  { 0,0,0 },  { 0,0,0 },  { 0,0,0 }  },
/* WXR_FORMAT_X1R5G5B5 */ { {10,5,31},  { 5,5,31},  { 0,5,31},  {10,5,31},  { 5,5,31},  { 0,5,31}  },
/* WXR_FORMAT_X1B5G5R5 */ { { 0,5,31},  { 5,5,31},  {10,5,31},  { 0,5,31},  { 5,5,31},  {10,5,31}  },
/* WXR_FORMAT_R5G6B5   */ { {11,5,31},  { 5,6,63},  { 0,5,31},  {11,5,31},  { 5,6,63},  { 0,5,31}  },
/* WXR_FORMAT_B5G6R5   */ { { 0,5,31},  { 5,6,63},  {11,5,31},  { 0,5,31},  { 5,6,63},  {11,5,31}  },
/* WXR_FORMAT_R8G8B8   */ { {16,8,255}, { 8,8,255}, { 0,8,255}, {16,8,255}, { 8,8,255}, { 0,8,255} },
/* WXR_FORMAT_B8G8R8   */ { { 0,8,255}, { 8,8,255}, {16,8,255}, { 0,8,255}, { 8,8,255}, {16,8,255} },
/* WXR_FORMAT_A8R8G8B8 */ { {16,8,255}, { 8,8,255}, { 0,8,255}, {16,8,255}, { 8,8,255}, { 0,8,255} },
/* WXR_FORMAT_B8G8R8A8 */ { { 8,8,255}, {16,8,255}, {24,8,255}, { 8,8,255}, {16,8,255}, {24,8,255} },
/* WXR_FORMAT_X8R8G8B8 */ { {16,8,255}, { 8,8,255}, { 0,8,255}, {16,8,255}, { 8,8,255}, { 0,8,255} },
/* WXR_FORMAT_B8G8R8X8 */ { { 8,8,255}, {16,8,255}, {24,8,255}, { 8,8,255}, {16,8,255}, {24,8,255} },
};

static enum wxr_format default_format = WXR_INVALID_FORMAT;
static XRenderPictFormat *pict_formats[WXR_NB_FORMATS + 1 /* invalid format */];

typedef struct
{
    LOGFONTW lf;
    XFORM    xform;
    SIZE     devsize;  /* size in device coords */
    DWORD    hash;
} LFANDSIZE;

#define INITIAL_REALIZED_BUF_SIZE 128

typedef enum { AA_None = 0, AA_Grey, AA_RGB, AA_BGR, AA_VRGB, AA_VBGR, AA_MAXVALUE } AA_Type;

typedef struct
{
    GlyphSet glyphset;
    XRenderPictFormat *font_format;
    int nrealized;
    BOOL *realized;
    void **bitmaps;
    XGlyphInfo *gis;
} gsCacheEntryFormat;

typedef struct
{
    LFANDSIZE lfsz;
    AA_Type aa_default;
    gsCacheEntryFormat * format[AA_MAXVALUE];
    INT count;
    INT next;
} gsCacheEntry;

struct xrender_physdev
{
    struct gdi_physdev dev;
    X11DRV_PDEVICE    *x11dev;
    enum wxr_format    format;
    int                cache_index;
    BOOL               update_clip;
    Picture            pict;
    Picture            pict_src;
    XRenderPictFormat *pict_format;
};

static inline struct xrender_physdev *get_xrender_dev( PHYSDEV dev )
{
    return (struct xrender_physdev *)dev;
}

static const struct gdi_dc_funcs xrender_funcs;

static gsCacheEntry *glyphsetCache = NULL;
static DWORD glyphsetCacheSize = 0;
static INT lastfree = -1;
static INT mru = -1;

#define INIT_CACHE_SIZE 10

static int antialias = 1;

static void *xrender_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XRenderAddGlyphs)
MAKE_FUNCPTR(XRenderComposite)
MAKE_FUNCPTR(XRenderCompositeText16)
MAKE_FUNCPTR(XRenderCreateGlyphSet)
MAKE_FUNCPTR(XRenderCreatePicture)
MAKE_FUNCPTR(XRenderFillRectangle)
MAKE_FUNCPTR(XRenderFindFormat)
MAKE_FUNCPTR(XRenderFindVisualFormat)
MAKE_FUNCPTR(XRenderFreeGlyphSet)
MAKE_FUNCPTR(XRenderFreePicture)
MAKE_FUNCPTR(XRenderSetPictureClipRectangles)
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
MAKE_FUNCPTR(XRenderSetPictureTransform)
#endif
MAKE_FUNCPTR(XRenderQueryExtension)

#ifdef SONAME_LIBFONTCONFIG
#include <fontconfig/fontconfig.h>
MAKE_FUNCPTR(FcConfigSubstitute)
MAKE_FUNCPTR(FcDefaultSubstitute)
MAKE_FUNCPTR(FcFontMatch)
MAKE_FUNCPTR(FcInit)
MAKE_FUNCPTR(FcPatternCreate)
MAKE_FUNCPTR(FcPatternDestroy)
MAKE_FUNCPTR(FcPatternAddInteger)
MAKE_FUNCPTR(FcPatternAddString)
MAKE_FUNCPTR(FcPatternGetBool)
MAKE_FUNCPTR(FcPatternGetInteger)
MAKE_FUNCPTR(FcPatternGetString)
static void *fontconfig_handle;
static BOOL fontconfig_installed;
#endif

#undef MAKE_FUNCPTR

static CRITICAL_SECTION xrender_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &xrender_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": xrender_cs") }
};
static CRITICAL_SECTION xrender_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (ULONG)_x4 << 24 ) |     \
            ( (ULONG)_x3 << 16 ) |     \
            ( (ULONG)_x2 <<  8 ) |     \
              (ULONG)_x1         )

#define MS_GASP_TAG MS_MAKE_TAG('g', 'a', 's', 'p')

#define GASP_GRIDFIT 0x01
#define GASP_DOGRAY  0x02

#ifdef WORDS_BIGENDIAN
#define get_be_word(x) (x)
#define NATIVE_BYTE_ORDER MSBFirst
#else
#define get_be_word(x) RtlUshortByteSwap(x)
#define NATIVE_BYTE_ORDER LSBFirst
#endif

static BOOL has_alpha( enum wxr_format format )
{
    return (format == WXR_FORMAT_A8R8G8B8 || format == WXR_FORMAT_B8G8R8A8);
}

static enum wxr_format get_format_without_alpha( enum wxr_format format )
{
    switch (format)
    {
    case WXR_FORMAT_A8R8G8B8: return WXR_FORMAT_X8R8G8B8;
    case WXR_FORMAT_B8G8R8A8: return WXR_FORMAT_B8G8R8X8;
    default: return format;
    }
}

static BOOL get_xrender_template(const WineXRenderFormatTemplate *fmt, XRenderPictFormat *templ, unsigned long *mask)
{
    templ->id = 0;
    templ->type = PictTypeDirect;
    templ->depth = fmt->depth;
    templ->direct.alpha = fmt->alpha;
    templ->direct.alphaMask = fmt->alphaMask;
    templ->direct.red = fmt->red;
    templ->direct.redMask = fmt->redMask;
    templ->direct.green = fmt->green;
    templ->direct.greenMask = fmt->greenMask;
    templ->direct.blue = fmt->blue;
    templ->direct.blueMask = fmt->blueMask;
    templ->colormap = 0;

    *mask = PictFormatType | PictFormatDepth | PictFormatAlpha | PictFormatAlphaMask | PictFormatRed | PictFormatRedMask | PictFormatGreen | PictFormatGreenMask | PictFormatBlue | PictFormatBlueMask;

    return TRUE;
}

static BOOL is_wxrformat_compatible_with_default_visual(const WineXRenderFormatTemplate *fmt)
{
    if(fmt->depth != screen_depth)
        return FALSE;
    if( (fmt->redMask << fmt->red) != visual->red_mask)
        return FALSE;
    if( (fmt->greenMask << fmt->green) != visual->green_mask)
        return FALSE;
    if( (fmt->blueMask << fmt->blue) != visual->blue_mask)
        return FALSE;

    /* We never select a default ARGB visual */
    if(fmt->alphaMask)
        return FALSE;

    return TRUE;
}

static int load_xrender_formats(void)
{
    int count = 0;
    unsigned int i;

    for (i = 0; i < WXR_NB_FORMATS; i++)
    {
        XRenderPictFormat templ;

        if(is_wxrformat_compatible_with_default_visual(&wxr_formats_template[i]))
        {
            wine_tsx11_lock();
            pict_formats[i] = pXRenderFindVisualFormat(gdi_display, visual);
            if (!pict_formats[i])
            {
                /* Xrender doesn't like DirectColor visuals, try to find a TrueColor one instead */
                if (visual->class == DirectColor)
                {
                    XVisualInfo info;
                    if (XMatchVisualInfo( gdi_display, DefaultScreen(gdi_display),
                                          screen_depth, TrueColor, &info ))
                    {
                        pict_formats[i] = pXRenderFindVisualFormat(gdi_display, info.visual);
                        if (pict_formats[i]) visual = info.visual;
                    }
                }
            }
            wine_tsx11_unlock();
            if (pict_formats[i]) default_format = i;
        }
        else
        {
            unsigned long mask = 0;
            get_xrender_template(&wxr_formats_template[i], &templ, &mask);

            wine_tsx11_lock();
            pict_formats[i] = pXRenderFindFormat(gdi_display, mask, &templ, 0);
            wine_tsx11_unlock();
        }
        if (pict_formats[i])
        {
            count++;
            TRACE("Loaded pict_format with id=%#lx for wxr_format=%#x\n", pict_formats[i]->id, i);
        }
    }
    return count;
}

/***********************************************************************
 *   X11DRV_XRender_Init
 *
 * Let's see if our XServer has the extension available
 *
 */
const struct gdi_dc_funcs *X11DRV_XRender_Init(void)
{
    int event_base, i;

    if (client_side_with_render &&
	(xrender_handle = wine_dlopen(SONAME_LIBXRENDER, RTLD_NOW, NULL, 0)))
    {

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(xrender_handle, #f, NULL, 0)) == NULL) goto sym_not_found;
LOAD_FUNCPTR(XRenderAddGlyphs)
LOAD_FUNCPTR(XRenderComposite)
LOAD_FUNCPTR(XRenderCompositeText16)
LOAD_FUNCPTR(XRenderCreateGlyphSet)
LOAD_FUNCPTR(XRenderCreatePicture)
LOAD_FUNCPTR(XRenderFillRectangle)
LOAD_FUNCPTR(XRenderFindFormat)
LOAD_FUNCPTR(XRenderFindVisualFormat)
LOAD_FUNCPTR(XRenderFreeGlyphSet)
LOAD_FUNCPTR(XRenderFreePicture)
LOAD_FUNCPTR(XRenderSetPictureClipRectangles)
LOAD_FUNCPTR(XRenderQueryExtension)
#undef LOAD_FUNCPTR
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
#define LOAD_OPTIONAL_FUNCPTR(f) p##f = wine_dlsym(xrender_handle, #f, NULL, 0);
LOAD_OPTIONAL_FUNCPTR(XRenderSetPictureTransform)
#undef LOAD_OPTIONAL_FUNCPTR
#endif

        wine_tsx11_lock();
        X11DRV_XRender_Installed = pXRenderQueryExtension(gdi_display, &event_base, &xrender_error_base);
        wine_tsx11_unlock();
        if(X11DRV_XRender_Installed) {
            TRACE("Xrender is up and running error_base = %d\n", xrender_error_base);
            if(!load_xrender_formats()) /* This fails in buggy versions of libXrender.so */
            {
                wine_tsx11_unlock();
                WINE_MESSAGE(
                    "Wine has detected that you probably have a buggy version\n"
                    "of libXrender.so .  Because of this client side font rendering\n"
                    "will be disabled.  Please upgrade this library.\n");
                X11DRV_XRender_Installed = FALSE;
                return NULL;
            }

            if (!visual->red_mask || !visual->green_mask || !visual->blue_mask) {
                WARN("one or more of the colour masks are 0, disabling XRENDER. Try running in 16-bit mode or higher.\n");
                X11DRV_XRender_Installed = FALSE;
            }
        }
    }

#ifdef SONAME_LIBFONTCONFIG
    if ((fontconfig_handle = wine_dlopen(SONAME_LIBFONTCONFIG, RTLD_NOW, NULL, 0)))
    {
#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(fontconfig_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
        LOAD_FUNCPTR(FcConfigSubstitute);
        LOAD_FUNCPTR(FcDefaultSubstitute);
        LOAD_FUNCPTR(FcFontMatch);
        LOAD_FUNCPTR(FcInit);
        LOAD_FUNCPTR(FcPatternCreate);
        LOAD_FUNCPTR(FcPatternDestroy);
        LOAD_FUNCPTR(FcPatternAddInteger);
        LOAD_FUNCPTR(FcPatternAddString);
        LOAD_FUNCPTR(FcPatternGetBool);
        LOAD_FUNCPTR(FcPatternGetInteger);
        LOAD_FUNCPTR(FcPatternGetString);
#undef LOAD_FUNCPTR
        fontconfig_installed = pFcInit();
    }
    else TRACE( "cannot find the fontconfig library " SONAME_LIBFONTCONFIG "\n" );
#endif

sym_not_found:
    if(X11DRV_XRender_Installed || client_side_with_core)
    {
	glyphsetCache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				  sizeof(*glyphsetCache) * INIT_CACHE_SIZE);

	glyphsetCacheSize = INIT_CACHE_SIZE;
	lastfree = 0;
	for(i = 0; i < INIT_CACHE_SIZE; i++) {
	  glyphsetCache[i].next = i + 1;
	  glyphsetCache[i].count = -1;
	}
	glyphsetCache[i-1].next = -1;
	using_client_side_fonts = 1;

	if(!X11DRV_XRender_Installed) {
	    TRACE("Xrender is not available on your XServer, client side rendering with the core protocol instead.\n");
	    if(screen_depth <= 8 || !client_side_antialias_with_core)
	        antialias = 0;
	} else {
	    if(screen_depth <= 8 || !client_side_antialias_with_render)
	        antialias = 0;
	}
        return &xrender_funcs;
    }
    TRACE("Using X11 core fonts\n");
    return NULL;
}

/* Helper function to convert from a color packed in a 32-bit integer to a XRenderColor */
static void get_xrender_color( XRenderPictFormat *pf, int src_color, XRenderColor *dst_color )
{
    if(pf->direct.redMask)
        dst_color->red = ((src_color >> pf->direct.red) & pf->direct.redMask) * 65535/pf->direct.redMask;
    else
       dst_color->red = 0;

    if(pf->direct.greenMask)
        dst_color->green = ((src_color >> pf->direct.green) & pf->direct.greenMask) * 65535/pf->direct.greenMask;
    else
        dst_color->green = 0;

    if(pf->direct.blueMask)
        dst_color->blue = ((src_color >> pf->direct.blue) & pf->direct.blueMask) * 65535/pf->direct.blueMask;
    else
        dst_color->blue = 0;

    dst_color->alpha = 0xffff;
}

static enum wxr_format get_xrender_format_from_color_shifts(int depth, ColorShifts *shifts)
{
    int redMask, greenMask, blueMask;
    unsigned int i;

    if (depth == 1) return WXR_FORMAT_MONO;

    /* physDevs of a depth <=8, don't have color_shifts set and XRender can't handle those except for 1-bit */
    if (!shifts) return default_format;

    redMask   = shifts->physicalRed.max << shifts->physicalRed.shift;
    greenMask = shifts->physicalGreen.max << shifts->physicalGreen.shift;
    blueMask  = shifts->physicalBlue.max << shifts->physicalBlue.shift;

    /* Try to locate a format which matches the specification of the dibsection. */
    for(i = 0; i < WXR_NB_FORMATS; i++)
    {
        if( depth     == wxr_formats_template[i].depth &&
            redMask   == (wxr_formats_template[i].redMask << wxr_formats_template[i].red) &&
            greenMask == (wxr_formats_template[i].greenMask << wxr_formats_template[i].green) &&
            blueMask  == (wxr_formats_template[i].blueMask << wxr_formats_template[i].blue) )
            return i;
    }

    /* This should not happen because when we reach 'shifts' must have been set and we only allows shifts which are backed by X */
    ERR("No XRender format found for %u %08x/%08x/%08x\n", depth, redMask, greenMask, blueMask);
    return WXR_INVALID_FORMAT;
}

static enum wxr_format get_xrender_format_from_bitmapinfo( const BITMAPINFO *info )
{
    if (info->bmiHeader.biPlanes != 1) return WXR_INVALID_FORMAT;

    switch (info->bmiHeader.biBitCount)
    {
    case 1:
        return WXR_FORMAT_MONO;
    case 4:
    case 8:
        break;
    case 24:
        if (info->bmiHeader.biCompression != BI_RGB) break;
        return WXR_FORMAT_R8G8B8;
    case 16:
    case 32:
        if (info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            DWORD *colors = (DWORD *)((char *)info + info->bmiHeader.biSize);
            unsigned int i;

            for (i = 0; i < WXR_NB_FORMATS; i++)
            {
                if (info->bmiHeader.biBitCount == wxr_formats_template[i].depth &&
                    colors[0] == (wxr_formats_template[i].redMask << wxr_formats_template[i].red) &&
                    colors[1] == (wxr_formats_template[i].greenMask << wxr_formats_template[i].green) &&
                    colors[2] == (wxr_formats_template[i].blueMask << wxr_formats_template[i].blue))
                    return i;
            }
            break;
        }
        if (info->bmiHeader.biCompression != BI_RGB) break;
        return (info->bmiHeader.biBitCount == 16) ? WXR_FORMAT_X1R5G5B5 : WXR_FORMAT_A8R8G8B8;
    }
    return WXR_INVALID_FORMAT;
}

static enum wxr_format get_bitmap_format( int bpp )
{
    enum wxr_format format = WXR_INVALID_FORMAT;

    if (bpp == screen_bpp)
    {
        switch (bpp)
        {
        case 16: format = WXR_FORMAT_R5G6B5; break;
        case 24: format = WXR_FORMAT_R8G8B8; break;
        case 32: format = WXR_FORMAT_A8R8G8B8; break;
        }
    }
    return format;
}

/* Set the x/y scaling and x/y offsets in the transformation matrix of the source picture */
static void set_xrender_transformation(Picture src_pict, double xscale, double yscale, int xoffset, int yoffset)
{
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
    XTransform xform = {{
        { XDoubleToFixed(xscale), XDoubleToFixed(0), XDoubleToFixed(xoffset) },
        { XDoubleToFixed(0), XDoubleToFixed(yscale), XDoubleToFixed(yoffset) },
        { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1) }
    }};

    pXRenderSetPictureTransform(gdi_display, src_pict, &xform);
#endif
}

/* check if we can use repeating instead of scaling for the specified source DC */
static BOOL use_source_repeat( struct xrender_physdev *dev )
{
    return (dev->x11dev->bitmap &&
            dev->x11dev->drawable_rect.right - dev->x11dev->drawable_rect.left == 1 &&
            dev->x11dev->drawable_rect.bottom - dev->x11dev->drawable_rect.top == 1);
}

static Picture get_xrender_picture( struct xrender_physdev *dev, HRGN clip_rgn, const RECT *clip_rect )
{
    if (!dev->pict && dev->pict_format)
    {
        XRenderPictureAttributes pa;

        wine_tsx11_lock();
        pa.subwindow_mode = IncludeInferiors;
        dev->pict = pXRenderCreatePicture( gdi_display, dev->x11dev->drawable,
                                           dev->pict_format, CPSubwindowMode, &pa );
        wine_tsx11_unlock();
        TRACE( "Allocing pict=%lx dc=%p drawable=%08lx\n",
               dev->pict, dev->dev.hdc, dev->x11dev->drawable );
        dev->update_clip = TRUE;
    }

    if (dev->update_clip)
    {
        RGNDATA *clip_data;
        HRGN rgn = 0;

        if (clip_rect)
        {
            rgn = CreateRectRgnIndirect( clip_rect );
            if (clip_rgn) CombineRgn( rgn, rgn, clip_rgn, RGN_AND );
            CombineRgn( rgn, rgn, dev->x11dev->region, RGN_AND );
        }
        else if (clip_rgn)
        {
            rgn = CreateRectRgn( 0, 0, 0, 0 );
            CombineRgn( rgn, clip_rgn, dev->x11dev->region, RGN_AND );
        }

        if ((clip_data = X11DRV_GetRegionData( rgn ? rgn : dev->x11dev->region, 0 )))
        {
            wine_tsx11_lock();
            pXRenderSetPictureClipRectangles( gdi_display, dev->pict,
                                              dev->x11dev->dc_rect.left, dev->x11dev->dc_rect.top,
                                              (XRectangle *)clip_data->Buffer, clip_data->rdh.nCount );
            wine_tsx11_unlock();
            HeapFree( GetProcessHeap(), 0, clip_data );
        }
        dev->update_clip = (rgn != 0);  /* have to update again if we are using a custom region */
        if (rgn) DeleteObject( rgn );
    }
    return dev->pict;
}

static Picture get_xrender_picture_source( struct xrender_physdev *dev, BOOL repeat )
{
    if (!dev->pict_src && dev->pict_format)
    {
        XRenderPictureAttributes pa;

        wine_tsx11_lock();
        pa.subwindow_mode = IncludeInferiors;
        pa.repeat = repeat ? RepeatNormal : RepeatNone;
        dev->pict_src = pXRenderCreatePicture( gdi_display, dev->x11dev->drawable,
                                               dev->pict_format, CPSubwindowMode|CPRepeat, &pa );
        wine_tsx11_unlock();

        TRACE("Allocing pict_src=%lx dc=%p drawable=%08lx repeat=%u\n",
              dev->pict_src, dev->dev.hdc, dev->x11dev->drawable, pa.repeat);
    }

    return dev->pict_src;
}

static void free_xrender_picture( struct xrender_physdev *dev )
{
    if (dev->pict || dev->pict_src)
    {
        wine_tsx11_lock();
        XFlush( gdi_display );
        if (dev->pict)
        {
            TRACE("freeing pict = %lx dc = %p\n", dev->pict, dev->dev.hdc);
            pXRenderFreePicture(gdi_display, dev->pict);
            dev->pict = 0;
        }
        if(dev->pict_src)
        {
            TRACE("freeing pict = %lx dc = %p\n", dev->pict_src, dev->dev.hdc);
            pXRenderFreePicture(gdi_display, dev->pict_src);
            dev->pict_src = 0;
        }
        wine_tsx11_unlock();
    }
}

/* return a mask picture used to force alpha to 0 */
static Picture get_no_alpha_mask(void)
{
    static Pixmap pixmap;
    static Picture pict;

    wine_tsx11_lock();
    if (!pict)
    {
        XRenderPictureAttributes pa;
        XRenderColor col;

        pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 32 );
        pa.repeat = RepeatNormal;
        pa.component_alpha = True;
        pict = pXRenderCreatePicture( gdi_display, pixmap, pict_formats[WXR_FORMAT_A8R8G8B8],
                                      CPRepeat|CPComponentAlpha, &pa );
        col.red = col.green = col.blue = 0xffff;
        col.alpha = 0;
        pXRenderFillRectangle( gdi_display, PictOpSrc, pict, &col, 0, 0, 1, 1 );
    }
    wine_tsx11_unlock();
    return pict;
}

static BOOL fontcmp(LFANDSIZE *p1, LFANDSIZE *p2)
{
  if(p1->hash != p2->hash) return TRUE;
  if(memcmp(&p1->devsize, &p2->devsize, sizeof(p1->devsize))) return TRUE;
  if(memcmp(&p1->xform, &p2->xform, sizeof(p1->xform))) return TRUE;
  if(memcmp(&p1->lf, &p2->lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
  return strcmpiW(p1->lf.lfFaceName, p2->lf.lfFaceName);
}

#if 0
static void walk_cache(void)
{
  int i;

  EnterCriticalSection(&xrender_cs);
  for(i=mru; i >= 0; i = glyphsetCache[i].next)
    TRACE("item %d\n", i);
  LeaveCriticalSection(&xrender_cs);
}
#endif

static int LookupEntry(LFANDSIZE *plfsz)
{
  int i, prev_i = -1;

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    TRACE("%d\n", i);
    if(glyphsetCache[i].count == -1) break; /* reached free list so stop */

    if(!fontcmp(&glyphsetCache[i].lfsz, plfsz)) {
      glyphsetCache[i].count++;
      if(prev_i >= 0) {
	glyphsetCache[prev_i].next = glyphsetCache[i].next;
	glyphsetCache[i].next = mru;
	mru = i;
      }
      TRACE("found font in cache %d\n", i);
      return i;
    }
    prev_i = i;
  }
  TRACE("font not in cache\n");
  return -1;
}

static void FreeEntry(int entry)
{
    int i, format;
  
    for(format = 0; format < AA_MAXVALUE; format++) {
        gsCacheEntryFormat * formatEntry;

        if( !glyphsetCache[entry].format[format] )
            continue;

        formatEntry = glyphsetCache[entry].format[format];

        if(formatEntry->glyphset) {
            wine_tsx11_lock();
            pXRenderFreeGlyphSet(gdi_display, formatEntry->glyphset);
            wine_tsx11_unlock();
            formatEntry->glyphset = 0;
        }
        if(formatEntry->nrealized) {
            HeapFree(GetProcessHeap(), 0, formatEntry->realized);
            formatEntry->realized = NULL;
            if(formatEntry->bitmaps) {
                for(i = 0; i < formatEntry->nrealized; i++)
                    HeapFree(GetProcessHeap(), 0, formatEntry->bitmaps[i]);
                HeapFree(GetProcessHeap(), 0, formatEntry->bitmaps);
                formatEntry->bitmaps = NULL;
            }
            HeapFree(GetProcessHeap(), 0, formatEntry->gis);
            formatEntry->gis = NULL;
            formatEntry->nrealized = 0;
        }

        HeapFree(GetProcessHeap(), 0, formatEntry);
        glyphsetCache[entry].format[format] = NULL;
    }
}

static int AllocEntry(void)
{
  int best = -1, prev_best = -1, i, prev_i = -1;

  if(lastfree >= 0) {
    assert(glyphsetCache[lastfree].count == -1);
    glyphsetCache[lastfree].count = 1;
    best = lastfree;
    lastfree = glyphsetCache[lastfree].next;
    assert(best != mru);
    glyphsetCache[best].next = mru;
    mru = best;

    TRACE("empty space at %d, next lastfree = %d\n", mru, lastfree);
    return mru;
  }

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    if(glyphsetCache[i].count == 0) {
      best = i;
      prev_best = prev_i;
    }
    prev_i = i;
  }

  if(best >= 0) {
    TRACE("freeing unused glyphset at cache %d\n", best);
    FreeEntry(best);
    glyphsetCache[best].count = 1;
    if(prev_best >= 0) {
      glyphsetCache[prev_best].next = glyphsetCache[best].next;
      glyphsetCache[best].next = mru;
      mru = best;
    } else {
      assert(mru == best);
    }
    return mru;
  }

  TRACE("Growing cache\n");
  
  if (glyphsetCache)
    glyphsetCache = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      glyphsetCache,
			      (glyphsetCacheSize + INIT_CACHE_SIZE)
			      * sizeof(*glyphsetCache));
  else
    glyphsetCache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      (glyphsetCacheSize + INIT_CACHE_SIZE)
			      * sizeof(*glyphsetCache));

  for(best = i = glyphsetCacheSize; i < glyphsetCacheSize + INIT_CACHE_SIZE;
      i++) {
    glyphsetCache[i].next = i + 1;
    glyphsetCache[i].count = -1;
  }
  glyphsetCache[i-1].next = -1;
  glyphsetCacheSize += INIT_CACHE_SIZE;

  lastfree = glyphsetCache[best].next;
  glyphsetCache[best].count = 1;
  glyphsetCache[best].next = mru;
  mru = best;
  TRACE("new free cache slot at %d\n", mru);
  return mru;
}

static BOOL get_gasp_flags(HDC hdc, WORD *flags)
{
    DWORD size;
    WORD *gasp, *buffer;
    WORD num_recs;
    DWORD ppem;
    TEXTMETRICW tm;

    *flags = 0;

    size = GetFontData(hdc, MS_GASP_TAG,  0, NULL, 0);
    if(size == GDI_ERROR)
        return FALSE;

    gasp = buffer = HeapAlloc(GetProcessHeap(), 0, size);
    GetFontData(hdc, MS_GASP_TAG,  0, gasp, size);

    GetTextMetricsW(hdc, &tm);
    ppem = abs(X11DRV_YWStoDS(hdc, tm.tmAscent + tm.tmDescent - tm.tmInternalLeading));

    gasp++;
    num_recs = get_be_word(*gasp);
    gasp++;
    while(num_recs--)
    {
        *flags = get_be_word(*(gasp + 1));
        if(ppem <= get_be_word(*gasp))
            break;
        gasp += 2;
    }
    TRACE("got flags %04x for ppem %d\n", *flags, ppem);

    HeapFree(GetProcessHeap(), 0, buffer);
    return TRUE;
}

static AA_Type get_antialias_type( HDC hdc, BOOL subpixel, BOOL hinter )
{
    AA_Type ret;
    WORD flags;
    UINT font_smoothing_type, font_smoothing_orientation;

    if (X11DRV_XRender_Installed && subpixel &&
        SystemParametersInfoW( SPI_GETFONTSMOOTHINGTYPE, 0, &font_smoothing_type, 0) &&
        font_smoothing_type == FE_FONTSMOOTHINGCLEARTYPE)
    {
        if ( SystemParametersInfoW( SPI_GETFONTSMOOTHINGORIENTATION, 0,
                                    &font_smoothing_orientation, 0) &&
             font_smoothing_orientation == FE_FONTSMOOTHINGORIENTATIONBGR)
        {
            ret = AA_BGR;
        }
        else
            ret = AA_RGB;
        /*FIXME
          If the monitor is in portrait mode, ClearType is disabled in the MS Windows (MSDN).
          But, Wine's subpixel rendering can support the portrait mode.
         */
    }
    else if (!hinter || !get_gasp_flags(hdc, &flags) || flags & GASP_DOGRAY)
        ret = AA_Grey;
    else
        ret = AA_None;

    return ret;
}

static int GetCacheEntry( HDC hdc, LFANDSIZE *plfsz )
{
    int ret;
    int format;
    gsCacheEntry *entry;
    static int hinter = -1;
    static int subpixel = -1;
    BOOL font_smoothing;

    if((ret = LookupEntry(plfsz)) != -1) return ret;

    ret = AllocEntry();
    entry = glyphsetCache + ret;
    entry->lfsz = *plfsz;
    for( format = 0; format < AA_MAXVALUE; format++ ) {
        assert( !entry->format[format] );
    }

    if(antialias && plfsz->lf.lfQuality != NONANTIALIASED_QUALITY)
    {
        if(hinter == -1 || subpixel == -1)
        {
            RASTERIZER_STATUS status;
            GetRasterizerCaps(&status, sizeof(status));
            hinter = status.wFlags & WINE_TT_HINTER_ENABLED;
            subpixel = status.wFlags & WINE_TT_SUBPIXEL_RENDERING_ENABLED;
        }

        switch (plfsz->lf.lfQuality)
        {
            case ANTIALIASED_QUALITY:
                entry->aa_default = get_antialias_type( hdc, FALSE, hinter );
                return ret;  /* ignore further configuration */
            case CLEARTYPE_QUALITY:
            case CLEARTYPE_NATURAL_QUALITY:
                entry->aa_default = get_antialias_type( hdc, subpixel, hinter );
                break;
            case DEFAULT_QUALITY:
            case DRAFT_QUALITY:
            case PROOF_QUALITY:
            default:
                if ( SystemParametersInfoW( SPI_GETFONTSMOOTHING, 0, &font_smoothing, 0) &&
                     font_smoothing)
                {
                    entry->aa_default = get_antialias_type( hdc, subpixel, hinter );
                }
                else
                    entry->aa_default = AA_None;
                break;
        }

        font_smoothing = TRUE;  /* default to enabled */
#ifdef SONAME_LIBFONTCONFIG
        if (fontconfig_installed)
        {
            FcPattern *match, *pattern;
            FcResult result;
            char family[LF_FACESIZE * 4];

#if defined(__i386__) && defined(__GNUC__)
            /* fontconfig generates floating point exceptions, mask them */
            WORD cw, default_cw = 0x37f;
            __asm__ __volatile__("fnstcw %0; fldcw %1" : "=m" (cw) : "m" (default_cw));
#endif

            WideCharToMultiByte( CP_UTF8, 0, plfsz->lf.lfFaceName, -1, family, sizeof(family), NULL, NULL );
            pattern = pFcPatternCreate();
            pFcPatternAddString( pattern, FC_FAMILY, (FcChar8 *)family );
            if (plfsz->lf.lfWeight != FW_DONTCARE)
            {
                int weight;
                switch (plfsz->lf.lfWeight)
                {
                case FW_THIN:       weight = FC_WEIGHT_THIN; break;
                case FW_EXTRALIGHT: weight = FC_WEIGHT_EXTRALIGHT; break;
                case FW_LIGHT:      weight = FC_WEIGHT_LIGHT; break;
                case FW_NORMAL:     weight = FC_WEIGHT_NORMAL; break;
                case FW_MEDIUM:     weight = FC_WEIGHT_MEDIUM; break;
                case FW_SEMIBOLD:   weight = FC_WEIGHT_SEMIBOLD; break;
                case FW_BOLD:       weight = FC_WEIGHT_BOLD; break;
                case FW_EXTRABOLD:  weight = FC_WEIGHT_EXTRABOLD; break;
                case FW_HEAVY:      weight = FC_WEIGHT_HEAVY; break;
                default:            weight = (plfsz->lf.lfWeight - 80) / 4; break;
                }
                pFcPatternAddInteger( pattern, FC_WEIGHT, weight );
            }
            pFcPatternAddInteger( pattern, FC_SLANT, plfsz->lf.lfItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN );
            pFcConfigSubstitute( NULL, pattern, FcMatchPattern );
            pFcDefaultSubstitute( pattern );
            if ((match = pFcFontMatch( NULL, pattern, &result )))
            {
                int rgba;
                FcBool antialias;

                if (pFcPatternGetBool( match, FC_ANTIALIAS, 0, &antialias ) != FcResultMatch)
                    antialias = TRUE;
                if (pFcPatternGetInteger( match, FC_RGBA, 0, &rgba ) == FcResultMatch)
                {
                    FcChar8 *file;
                    if (pFcPatternGetString( match, FC_FILE, 0, &file ) != FcResultMatch) file = NULL;

                    TRACE( "fontconfig returned rgba %u antialias %u for font %s file %s\n",
                           rgba, antialias, debugstr_w(plfsz->lf.lfFaceName), debugstr_a((char *)file) );

                    switch (rgba)
                    {
                    case FC_RGBA_RGB:  entry->aa_default = AA_RGB; break;
                    case FC_RGBA_BGR:  entry->aa_default = AA_BGR; break;
                    case FC_RGBA_VRGB: entry->aa_default = AA_VRGB; break;
                    case FC_RGBA_VBGR: entry->aa_default = AA_VBGR; break;
                    case FC_RGBA_NONE: entry->aa_default = AA_Grey; break;
                    }
                }
                if (!antialias) font_smoothing = FALSE;
                pFcPatternDestroy( match );
            }
            pFcPatternDestroy( pattern );

#if defined(__i386__) && defined(__GNUC__)
            __asm__ __volatile__("fnclex; fldcw %0" : : "m" (cw));
#endif
        }
#endif  /* SONAME_LIBFONTCONFIG */

        /* now check Xft resources */
        {
            char *value;
            BOOL antialias = TRUE;

            wine_tsx11_lock();
            if ((value = XGetDefault( gdi_display, "Xft", "antialias" )))
            {
                if (tolower(value[0]) == 'f' || tolower(value[0]) == 'n' ||
                    value[0] == '0' || !strcasecmp( value, "off" ))
                    antialias = FALSE;
            }
            if ((value = XGetDefault( gdi_display, "Xft", "rgba" )))
            {
                TRACE( "Xft resource returned rgba '%s' antialias %u\n", value, antialias );
                if (!strcmp( value, "rgb" )) entry->aa_default = AA_RGB;
                else if (!strcmp( value, "bgr" )) entry->aa_default = AA_BGR;
                else if (!strcmp( value, "vrgb" )) entry->aa_default = AA_VRGB;
                else if (!strcmp( value, "vbgr" )) entry->aa_default = AA_VBGR;
                else if (!strcmp( value, "none" )) entry->aa_default = AA_Grey;
            }
            wine_tsx11_unlock();
            if (!antialias) font_smoothing = FALSE;
        }

        if (!font_smoothing) entry->aa_default = AA_None;

        /* we can't support subpixel without xrender */
        if (!X11DRV_XRender_Installed && entry->aa_default > AA_Grey) entry->aa_default = AA_Grey;
    }
    else
        entry->aa_default = AA_None;

    return ret;
}

static void dec_ref_cache(int index)
{
    assert(index >= 0);
    TRACE("dec'ing entry %d to %d\n", index, glyphsetCache[index].count - 1);
    assert(glyphsetCache[index].count > 0);
    glyphsetCache[index].count--;
}

static void lfsz_calc_hash(LFANDSIZE *plfsz)
{
  DWORD hash = 0, *ptr, two_chars;
  WORD *pwc;
  int i;

  hash ^= plfsz->devsize.cx;
  hash ^= plfsz->devsize.cy;
  for(i = 0, ptr = (DWORD*)&plfsz->xform; i < sizeof(XFORM)/sizeof(DWORD); i++, ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)&plfsz->lf; i < 7; i++, ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)plfsz->lf.lfFaceName; i < LF_FACESIZE/2; i++, ptr++) {
    two_chars = *ptr;
    pwc = (WCHAR *)&two_chars;
    if(!*pwc) break;
    *pwc = toupperW(*pwc);
    pwc++;
    *pwc = toupperW(*pwc);
    hash ^= two_chars;
    if(!*pwc) break;
  }
  plfsz->hash = hash;
  return;
}

/***********************************************************************
 *   X11DRV_XRender_Finalize
 */
void X11DRV_XRender_Finalize(void)
{
    int i;

    EnterCriticalSection(&xrender_cs);
    for(i = mru; i >= 0; i = glyphsetCache[i].next)
	FreeEntry(i);
    LeaveCriticalSection(&xrender_cs);
    DeleteCriticalSection(&xrender_cs);
}

/**********************************************************************
 *	     xrenderdrv_SelectFont
 */
static HFONT xrenderdrv_SelectFont( PHYSDEV dev, HFONT hfont )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectFont );
    HFONT ret = next->funcs->pSelectFont( next, hfont );

    if (!ret) return 0;

    if (physdev->x11dev->has_gdi_font)
    {
        LFANDSIZE lfsz;

        GetObjectW( hfont, sizeof(lfsz.lf), &lfsz.lf );

        TRACE("h=%d w=%d weight=%d it=%d charset=%d name=%s\n",
              lfsz.lf.lfHeight, lfsz.lf.lfWidth, lfsz.lf.lfWeight,
              lfsz.lf.lfItalic, lfsz.lf.lfCharSet, debugstr_w(lfsz.lf.lfFaceName));
        lfsz.lf.lfWidth = abs( lfsz.lf.lfWidth );
        lfsz.devsize.cx = X11DRV_XWStoDS( dev->hdc, lfsz.lf.lfWidth );
        lfsz.devsize.cy = X11DRV_YWStoDS( dev->hdc, lfsz.lf.lfHeight );

        GetTransform( dev->hdc, 0x204, &lfsz.xform );
        TRACE("font transform %f %f %f %f\n", lfsz.xform.eM11, lfsz.xform.eM12,
              lfsz.xform.eM21, lfsz.xform.eM22);

        /* Not used fields, would break hashing */
        lfsz.xform.eDx = lfsz.xform.eDy = 0;

        lfsz_calc_hash(&lfsz);

        EnterCriticalSection(&xrender_cs);
        if (physdev->cache_index != -1)
            dec_ref_cache( physdev->cache_index );
        physdev->cache_index = GetCacheEntry( dev->hdc, &lfsz );
        LeaveCriticalSection(&xrender_cs);
    }
    else
    {
        EnterCriticalSection( &xrender_cs );
        if (physdev->cache_index != -1) dec_ref_cache( physdev->cache_index );
        physdev->cache_index = -1;
        LeaveCriticalSection( &xrender_cs );
    }
    return ret;
}

static BOOL create_xrender_dc( PHYSDEV *pdev, enum wxr_format format )
{
    X11DRV_PDEVICE *x11dev = get_x11drv_dev( *pdev );
    struct xrender_physdev *physdev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physdev) );

    if (!physdev) return FALSE;
    physdev->x11dev = x11dev;
    physdev->cache_index = -1;
    physdev->format = format;
    physdev->pict_format = pict_formats[format];
    push_dc_driver( pdev, &physdev->dev, &xrender_funcs );
    return TRUE;
}

/* store the color mask data in the bitmap info structure */
static void set_color_info( XRenderPictFormat *format, BITMAPINFO *info )
{
    DWORD *colors = (DWORD *)((char *)info + info->bmiHeader.biSize);

    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = pixmap_formats[format->depth]->bits_per_pixel;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biClrUsed     = 0;

    switch (info->bmiHeader.biBitCount)
    {
    case 16:
        colors[0] = format->direct.redMask   << format->direct.red;
        colors[1] = format->direct.greenMask << format->direct.green;
        colors[2] = format->direct.blueMask  << format->direct.blue;
        info->bmiHeader.biCompression = BI_BITFIELDS;
        break;
    case 32:
        colors[0] = format->direct.redMask   << format->direct.red;
        colors[1] = format->direct.greenMask << format->direct.green;
        colors[2] = format->direct.blueMask  << format->direct.blue;
        if (colors[0] != 0xff0000 || colors[1] != 0x00ff00 || colors[2] != 0x0000ff)
            info->bmiHeader.biCompression = BI_BITFIELDS;
        break;
    }
}


/**********************************************************************
 *	     xrenderdrv_CreateDC
 */
static BOOL xrenderdrv_CreateDC( PHYSDEV *pdev, LPCWSTR driver, LPCWSTR device,
                                 LPCWSTR output, const DEVMODEW* initData )
{
    return create_xrender_dc( pdev, default_format );
}

/**********************************************************************
 *	     xrenderdrv_CreateCompatibleDC
 */
static BOOL xrenderdrv_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    if (orig)  /* chain to x11drv first */
    {
        orig = GET_NEXT_PHYSDEV( orig, pCreateCompatibleDC );
        if (!orig->funcs->pCreateCompatibleDC( orig, pdev )) return FALSE;
    }
    /* otherwise we have been called by x11drv */

    return create_xrender_dc( pdev, WXR_FORMAT_MONO );
}

/**********************************************************************
 *	     xrenderdrv_DeleteDC
 */
static BOOL xrenderdrv_DeleteDC( PHYSDEV dev )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );

    free_xrender_picture( physdev );

    EnterCriticalSection( &xrender_cs );
    if (physdev->cache_index != -1) dec_ref_cache( physdev->cache_index );
    LeaveCriticalSection( &xrender_cs );

    HeapFree( GetProcessHeap(), 0, physdev );
    return TRUE;
}

/**********************************************************************
 *           xrenderdrv_ExtEscape
 */
static INT xrenderdrv_ExtEscape( PHYSDEV dev, INT escape, INT in_count, LPCVOID in_data,
                                 INT out_count, LPVOID out_data )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );

    dev = GET_NEXT_PHYSDEV( dev, pExtEscape );

    if (escape == X11DRV_ESCAPE && in_data && in_count >= sizeof(enum x11drv_escape_codes))
    {
        if (*(const enum x11drv_escape_codes *)in_data == X11DRV_SET_DRAWABLE)
        {
            BOOL ret = dev->funcs->pExtEscape( dev, escape, in_count, in_data, out_count, out_data );
            if (ret) free_xrender_picture( physdev );  /* pict format doesn't change, only drawable */
            return ret;
        }
    }
    return dev->funcs->pExtEscape( dev, escape, in_count, in_data, out_count, out_data );
}

/****************************************************************************
 *	  xrenderdrv_CopyBitmap
 */
static BOOL xrenderdrv_CopyBitmap( HBITMAP src, HBITMAP dst )
{
    return X11DRV_CopyBitmap( src, dst );
}

/****************************************************************************
 *	  xrenderdrv_CreateBitmap
 */
static BOOL xrenderdrv_CreateBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    enum wxr_format format;
    BITMAP bitmap;

    if (!GetObjectW( hbitmap, sizeof(bitmap), &bitmap )) return FALSE;

    if (bitmap.bmPlanes != 1) return FALSE;
    format = get_bitmap_format( bitmap.bmBitsPixel );

    if (pict_formats[format])
        return X11DRV_create_phys_bitmap( hbitmap, &bitmap, pict_formats[format]->depth,
                                          TRUE, &wxr_color_shifts[format] );

    dev = GET_NEXT_PHYSDEV( dev, pCreateBitmap );
    return dev->funcs->pCreateBitmap( dev, hbitmap );
}

/****************************************************************************
 *	  xrenderdrv_DeleteBitmap
 */
static BOOL xrenderdrv_DeleteBitmap( HBITMAP hbitmap )
{
    return X11DRV_DeleteBitmap( hbitmap );
}

/***********************************************************************
 *           xrenderdrv_SelectBitmap
 */
static HBITMAP xrenderdrv_SelectBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    HBITMAP ret;
    struct xrender_physdev *physdev = get_xrender_dev( dev );

    dev = GET_NEXT_PHYSDEV( dev, pSelectBitmap );
    ret = dev->funcs->pSelectBitmap( dev, hbitmap );
    if (ret)
    {
        free_xrender_picture( physdev );
        physdev->format = get_xrender_format_from_color_shifts( physdev->x11dev->depth,
                                                                physdev->x11dev->color_shifts );
        physdev->pict_format = pict_formats[physdev->format];
    }
    return ret;
}

/***********************************************************************
 *           xrenderdrv_GetImage
 */
static DWORD xrenderdrv_GetImage( PHYSDEV dev, HBITMAP hbitmap, BITMAPINFO *info,
                                  struct gdi_image_bits *bits, struct bitblt_coords *src )
{
    if (hbitmap) return X11DRV_GetImage( dev, hbitmap, info, bits, src );
    dev = GET_NEXT_PHYSDEV( dev, pGetImage );
    return dev->funcs->pGetImage( dev, hbitmap, info, bits, src );
}

/***********************************************************************
 *           xrenderdrv_SetDeviceClipping
 */
static void xrenderdrv_SetDeviceClipping( PHYSDEV dev, HRGN vis_rgn, HRGN clip_rgn )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );

    physdev->update_clip = TRUE;

    dev = GET_NEXT_PHYSDEV( dev, pSetDeviceClipping );
    dev->funcs->pSetDeviceClipping( dev, vis_rgn, clip_rgn );
}


BOOL X11DRV_XRender_SetPhysBitmapDepth(X_PHYSBITMAP *physBitmap, int bits_pixel, const DIBSECTION *dib)
{
    XRenderPictFormat *pict_format;
    ColorShifts shifts;
    const DWORD *bitfields;
    static const DWORD bitfields_32[3] = {0xff0000, 0x00ff00, 0x0000ff};
    static const DWORD bitfields_16[3] = {0x7c00, 0x03e0, 0x001f};


    /* When XRender is not around we can only use the screen_depth and when needed we perform depth conversion
     * in software. Further we also return the screen depth for paletted formats or TrueColor formats with a low
     * number of bits because XRender can't handle paletted formats and 8-bit TrueColor does not exist for XRender. */
    if (!X11DRV_XRender_Installed || bits_pixel <= 8)
        return FALSE;

    if(dib->dsBmih.biCompression == BI_BITFIELDS)
        bitfields = dib->dsBitfields;
    else if(bits_pixel == 24 || bits_pixel == 32)
        bitfields = bitfields_32;
    else
        bitfields = bitfields_16;

    X11DRV_PALETTE_ComputeColorShifts(&shifts, bitfields[0], bitfields[1], bitfields[2]);
    pict_format = pict_formats[get_xrender_format_from_color_shifts(dib->dsBm.bmBitsPixel, &shifts)];

    /* Common formats should be in our picture format table. */
    if (!pict_format)
    {
        TRACE("Unhandled dibsection format bpp=%d, redMask=%x, greenMask=%x, blueMask=%x\n",
              dib->dsBm.bmBitsPixel, bitfields[0], bitfields[1], bitfields[2]);
        return FALSE;
    }

    physBitmap->depth = pict_format->depth;
    physBitmap->trueColor = TRUE;
    physBitmap->color_shifts = shifts;
    return TRUE;
}

/************************************************************************
 *   UploadGlyph
 *
 * Helper to ExtTextOut.  Must be called inside xrender_cs
 */
static void UploadGlyph(struct xrender_physdev *physDev, int glyph, AA_Type format)
{
    unsigned int buflen;
    char *buf;
    Glyph gid;
    GLYPHMETRICS gm;
    XGlyphInfo gi;
    gsCacheEntry *entry = glyphsetCache + physDev->cache_index;
    gsCacheEntryFormat *formatEntry;
    UINT ggo_format = GGO_GLYPH_INDEX;
    enum wxr_format wxr_format;
    static const char zero[4];
    static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };

    switch(format) {
    case AA_Grey:
	ggo_format |= WINE_GGO_GRAY16_BITMAP;
	break;
    case AA_RGB:
	ggo_format |= WINE_GGO_HRGB_BITMAP;
	break;
    case AA_BGR:
	ggo_format |= WINE_GGO_HBGR_BITMAP;
	break;
    case AA_VRGB:
	ggo_format |= WINE_GGO_VRGB_BITMAP;
	break;
    case AA_VBGR:
	ggo_format |= WINE_GGO_VBGR_BITMAP;
	break;

    default:
        ERR("aa = %d - not implemented\n", format);
    case AA_None:
        ggo_format |= GGO_BITMAP;
	break;
    }

    buflen = GetGlyphOutlineW(physDev->dev.hdc, glyph, ggo_format, &gm, 0, NULL, &identity);
    if(buflen == GDI_ERROR) {
        if(format != AA_None) {
            format = AA_None;
            entry->aa_default = AA_None;
            ggo_format = GGO_GLYPH_INDEX | GGO_BITMAP;
            buflen = GetGlyphOutlineW(physDev->dev.hdc, glyph, ggo_format, &gm, 0, NULL, &identity);
        }
        if(buflen == GDI_ERROR) {
            WARN("GetGlyphOutlineW failed using default glyph\n");
            buflen = GetGlyphOutlineW(physDev->dev.hdc, 0, ggo_format, &gm, 0, NULL, &identity);
            if(buflen == GDI_ERROR) {
                WARN("GetGlyphOutlineW failed for default glyph trying for space\n");
                buflen = GetGlyphOutlineW(physDev->dev.hdc, 0x20, ggo_format, &gm, 0, NULL, &identity);
                if(buflen == GDI_ERROR) {
                    ERR("GetGlyphOutlineW for all attempts unable to upload a glyph\n");
                    return;
                }
            }
        }
        TRACE("Turning off antialiasing for this monochrome font\n");
    }

    /* If there is nothing for the current type, we create the entry. */
    if( !entry->format[format] ) {
        entry->format[format] = HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          sizeof(gsCacheEntryFormat));
    }
    formatEntry = entry->format[format];

    if(formatEntry->nrealized <= glyph) {
        formatEntry->nrealized = (glyph / 128 + 1) * 128;

	if (formatEntry->realized)
	    formatEntry->realized = HeapReAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->realized,
				      formatEntry->nrealized * sizeof(BOOL));
	else
	    formatEntry->realized = HeapAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->nrealized * sizeof(BOOL));

	if(!X11DRV_XRender_Installed) {
	  if (formatEntry->bitmaps)
	    formatEntry->bitmaps = HeapReAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->bitmaps,
				      formatEntry->nrealized * sizeof(formatEntry->bitmaps[0]));
	  else
	    formatEntry->bitmaps = HeapAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->nrealized * sizeof(formatEntry->bitmaps[0]));
        }
        if (formatEntry->gis)
	    formatEntry->gis = HeapReAlloc(GetProcessHeap(),
				   HEAP_ZERO_MEMORY,
				   formatEntry->gis,
				   formatEntry->nrealized * sizeof(formatEntry->gis[0]));
        else
	    formatEntry->gis = HeapAlloc(GetProcessHeap(),
				   HEAP_ZERO_MEMORY,
                                   formatEntry->nrealized * sizeof(formatEntry->gis[0]));
    }


    if(formatEntry->glyphset == 0 && X11DRV_XRender_Installed) {
        switch(format) {
            case AA_Grey:
                wxr_format = WXR_FORMAT_GRAY;
                break;

            case AA_RGB:
            case AA_BGR:
            case AA_VRGB:
            case AA_VBGR:
                wxr_format = WXR_FORMAT_A8R8G8B8;
                break;

            default:
                ERR("aa = %d - not implemented\n", format);
            case AA_None:
                wxr_format = WXR_FORMAT_MONO;
                break;
        }

        wine_tsx11_lock();
        formatEntry->font_format = pict_formats[wxr_format];
        formatEntry->glyphset = pXRenderCreateGlyphSet(gdi_display, formatEntry->font_format);
        wine_tsx11_unlock();
    }


    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen);
    GetGlyphOutlineW(physDev->dev.hdc, glyph, ggo_format, &gm, buflen, buf, &identity);
    formatEntry->realized[glyph] = TRUE;

    TRACE("buflen = %d. Got metrics: %dx%d adv=%d,%d origin=%d,%d\n",
	  buflen,
	  gm.gmBlackBoxX, gm.gmBlackBoxY, gm.gmCellIncX, gm.gmCellIncY,
	  gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);

    gi.width = gm.gmBlackBoxX;
    gi.height = gm.gmBlackBoxY;
    gi.x = -gm.gmptGlyphOrigin.x;
    gi.y = gm.gmptGlyphOrigin.y;
    gi.xOff = gm.gmCellIncX;
    gi.yOff = gm.gmCellIncY;

    if(TRACE_ON(xrender)) {
        int pitch, i, j;
	char output[300];
	unsigned char *line;

	if(format == AA_None) {
	    pitch = ((gi.width + 31) / 32) * 4;
	    for(i = 0; i < gi.height; i++) {
	        line = (unsigned char*) buf + i * pitch;
		output[0] = '\0';
		for(j = 0; j < pitch * 8; j++) {
	            strcat(output, (line[j / 8] & (1 << (7 - (j % 8)))) ? "#" : " ");
		}
		TRACE("%s\n", output);
	    }
	} else {
	    static const char blks[] = " .:;!o*#";
	    char str[2];

	    str[1] = '\0';
	    pitch = ((gi.width + 3) / 4) * 4;
	    for(i = 0; i < gi.height; i++) {
	        line = (unsigned char*) buf + i * pitch;
		output[0] = '\0';
		for(j = 0; j < pitch; j++) {
		    str[0] = blks[line[j] >> 5];
		    strcat(output, str);
		}
		TRACE("%s\n", output);
	    }
	}
    }


    if(formatEntry->glyphset) {
        if(format == AA_None && BitmapBitOrder(gdi_display) != MSBFirst) {
	    unsigned char *byte = (unsigned char*) buf, c;
	    int i = buflen;

	    while(i--) {
	        c = *byte;

		/* magic to flip bit order */
		c = ((c << 1) & 0xaa) | ((c >> 1) & 0x55);
		c = ((c << 2) & 0xcc) | ((c >> 2) & 0x33);
		c = ((c << 4) & 0xf0) | ((c >> 4) & 0x0f);

		*byte++ = c;
	    }
	}
        else if ( format != AA_Grey &&
                  ImageByteOrder (gdi_display) != NATIVE_BYTE_ORDER)
        {
            unsigned int i, *data = (unsigned int *)buf;
            for (i = buflen / sizeof(int); i; i--, data++) *data = RtlUlongByteSwap(*data);
        }
	gid = glyph;

        /*
          XRenderCompositeText seems to ignore 0x0 glyphs when
          AA_None, which means we lose the advance width of glyphs
          like the space.  We'll pretend that such glyphs are 1x1
          bitmaps.
        */

        if(buflen == 0)
            gi.width = gi.height = 1;

        wine_tsx11_lock();
	pXRenderAddGlyphs(gdi_display, formatEntry->glyphset, &gid, &gi, 1,
                          buflen ? buf : zero, buflen ? buflen : sizeof(zero));
	wine_tsx11_unlock();
	HeapFree(GetProcessHeap(), 0, buf);
    } else {
        formatEntry->bitmaps[glyph] = buf;
    }

    formatEntry->gis[glyph] = gi;
}

static void SharpGlyphMono(struct xrender_physdev *physDev, INT x, INT y,
			    void *bitmap, XGlyphInfo *gi)
{
    unsigned char   *srcLine = bitmap, *src;
    unsigned char   bits, bitsMask;
    int             width = gi->width;
    int             stride = ((width + 31) & ~31) >> 3;
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;

    TRACE("%d, %d\n", x, y);
    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bitsMask = 0x80;    /* FreeType is always MSB first */
        bits = *src++;
        
        xspan = x;
        while (w)
        {
            if (bits & bitsMask)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (bits & bitsMask);
                XFillRectangle (gdi_display, physDev->x11dev->drawable,
                                physDev->x11dev->gc, xspan, y, lenspan, 1);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (!(bits & bitsMask));
            }
        }
        y++;
    }
}

static void SharpGlyphGray(struct xrender_physdev *physDev, INT x, INT y,
			    void *bitmap, XGlyphInfo *gi)
{
    unsigned char   *srcLine = bitmap, *src, bits;
    int             width = gi->width;
    int             stride = ((width + 3) & ~3);
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;

    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bits = *src++;
        xspan = x;
        while (w)
        {
            if (bits >= 0x80)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bits = *src++;
                } while (bits >= 0x80);
                XFillRectangle (gdi_display, physDev->x11dev->drawable,
                                physDev->x11dev->gc, xspan, y, lenspan, 1);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bits = *src++;
                } while (bits < 0x80);
            }
        }
        y++;
    }
}


static void ExamineBitfield (DWORD mask, int *shift, int *len)
{
    int s, l;

    s = 0;
    while ((mask & 1) == 0)
    {
        mask >>= 1;
        s++;
    }
    l = 0;
    while ((mask & 1) == 1)
    {
        mask >>= 1;
        l++;
    }
    *shift = s;
    *len = l;
}

static DWORD GetField (DWORD pixel, int shift, int len)
{
    pixel = pixel & (((1 << (len)) - 1) << shift);
    pixel = pixel << (32 - (shift + len)) >> 24;
    while (len < 8)
    {
        pixel |= (pixel >> len);
        len <<= 1;
    }
    return pixel;
}


static DWORD PutField (DWORD pixel, int shift, int len)
{
    shift = shift - (8 - len);
    if (len <= 8)
        pixel &= (((1 << len) - 1) << (8 - len));
    if (shift < 0)
        pixel >>= -shift;
    else
        pixel <<= shift;
    return pixel;
}

static void SmoothGlyphGray(XImage *image, int x, int y, void *bitmap, XGlyphInfo *gi,
			    int color)
{
    int             r_shift, r_len;
    int             g_shift, g_len;
    int             b_shift, b_len;
    BYTE            *maskLine, *mask, m;
    int             maskStride;
    DWORD           pixel;
    int             width, height;
    int             w, tx;
    BYTE            src_r, src_g, src_b;

    x -= gi->x;
    y -= gi->y;
    width = gi->width;
    height = gi->height;

    maskLine = bitmap;
    maskStride = (width + 3) & ~3;

    ExamineBitfield (image->red_mask, &r_shift, &r_len);
    ExamineBitfield (image->green_mask, &g_shift, &g_len);
    ExamineBitfield (image->blue_mask, &b_shift, &b_len);

    src_r = GetField(color, r_shift, r_len);
    src_g = GetField(color, g_shift, g_len);
    src_b = GetField(color, b_shift, b_len);
    
    for(; height--; y++)
    {
        mask = maskLine;
        maskLine += maskStride;
        w = width;
        tx = x;

	if(y < 0) continue;
	if(y >= image->height) break;

        for(; w--; tx++)
        {
	    if(tx >= image->width) break;

            m = *mask++;
	    if(tx < 0) continue;

	    if (m == 0xff)
		XPutPixel (image, tx, y, color);
	    else if (m)
	    {
	        BYTE r, g, b;

		pixel = XGetPixel (image, tx, y);

		r = GetField(pixel, r_shift, r_len);
		r = ((BYTE)~m * (WORD)r + (BYTE)m * (WORD)src_r) >> 8;
		g = GetField(pixel, g_shift, g_len);
		g = ((BYTE)~m * (WORD)g + (BYTE)m * (WORD)src_g) >> 8;
		b = GetField(pixel, b_shift, b_len);
		b = ((BYTE)~m * (WORD)b + (BYTE)m * (WORD)src_b) >> 8;

		pixel = (PutField (r, r_shift, r_len) |
			 PutField (g, g_shift, g_len) |
			 PutField (b, b_shift, b_len));
		XPutPixel (image, tx, y, pixel);
	    }
        }
    }
}

/*************************************************************
 *                 get_tile_pict
 *
 * Returns an appropriate Picture for tiling the text colour.
 * Call and use result within the xrender_cs
 */
static Picture get_tile_pict( enum wxr_format wxr_format, const XRenderColor *color)
{
    static struct
    {
        Pixmap xpm;
        Picture pict;
        XRenderColor current_color;
    } tiles[WXR_NB_FORMATS], *tile;

    tile = &tiles[wxr_format];

    if(!tile->xpm)
    {
        XRenderPictureAttributes pa;
        XRenderPictFormat *pict_format = pict_formats[wxr_format];

        wine_tsx11_lock();
        tile->xpm = XCreatePixmap(gdi_display, root_window, 1, 1, pict_format->depth);

        pa.repeat = RepeatNormal;
        tile->pict = pXRenderCreatePicture(gdi_display, tile->xpm, pict_format, CPRepeat, &pa);
        wine_tsx11_unlock();

        /* init current_color to something different from text_pixel */
        tile->current_color = *color;
        tile->current_color.red ^= 0xffff;

        if (wxr_format == WXR_FORMAT_MONO)
        {
            /* for a 1bpp bitmap we always need a 1 in the tile */
            XRenderColor col;
            col.red = col.green = col.blue = 0;
            col.alpha = 0xffff;
            wine_tsx11_lock();
            pXRenderFillRectangle(gdi_display, PictOpSrc, tile->pict, &col, 0, 0, 1, 1);
            wine_tsx11_unlock();
        }
    }

    if (memcmp( color, &tile->current_color, sizeof(*color) ) && wxr_format != WXR_FORMAT_MONO)
    {
        wine_tsx11_lock();
        pXRenderFillRectangle(gdi_display, PictOpSrc, tile->pict, color, 0, 0, 1, 1);
        wine_tsx11_unlock();
        tile->current_color = *color;
    }
    return tile->pict;
}

/*************************************************************
 *                 get_mask_pict
 *
 * Returns an appropriate Picture for masking with the specified alpha.
 * Call and use result within the xrender_cs
 */
static Picture get_mask_pict( int alpha )
{
    static Pixmap pixmap;
    static Picture pict;
    static int current_alpha;

    if (alpha == 0xffff) return 0;  /* don't need a mask for alpha==1.0 */

    if (!pixmap)
    {
        XRenderPictureAttributes pa;

        wine_tsx11_lock();
        pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 32 );
        pa.repeat = RepeatNormal;
        pict = pXRenderCreatePicture( gdi_display, pixmap,
                                      pict_formats[WXR_FORMAT_A8R8G8B8], CPRepeat, &pa );
        wine_tsx11_unlock();
        current_alpha = -1;
    }

    if (alpha != current_alpha)
    {
        XRenderColor col;
        col.red = col.green = col.blue = 0;
        col.alpha = current_alpha = alpha;
        wine_tsx11_lock();
        pXRenderFillRectangle( gdi_display, PictOpSrc, pict, &col, 0, 0, 1, 1 );
        wine_tsx11_unlock();
    }
    return pict;
}

static int XRenderErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

/***********************************************************************
 *           xrenderdrv_ExtTextOut
 */
static BOOL xrenderdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                                   const RECT *lprect, LPCWSTR wstr, UINT count, const INT *lpDx )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    XGCValues xgcval;
    gsCacheEntry *entry;
    gsCacheEntryFormat *formatEntry;
    BOOL retv = FALSE;
    int textPixel, backgroundPixel;
    RGNDATA *saved_region = NULL;
    AA_Type aa_type = AA_None;
    unsigned int idx;
    Picture tile_pict = 0;

    if (!physdev->x11dev->has_gdi_font)
    {
        dev = GET_NEXT_PHYSDEV( dev, pExtTextOut );
        return dev->funcs->pExtTextOut( dev, x, y, flags, lprect, wstr, count, lpDx );
    }

    xgcval.function = GXcopy;
    xgcval.background = physdev->x11dev->backgroundPixel;
    xgcval.fill_style = FillSolid;
    wine_tsx11_lock();
    XChangeGC( gdi_display, physdev->x11dev->gc, GCFunction | GCBackground | GCFillStyle, &xgcval );
    wine_tsx11_unlock();

    X11DRV_LockDIBSection( physdev->x11dev, DIB_Status_GdiMod );

    if(physdev->x11dev->depth == 1) {
        if((physdev->x11dev->textPixel & 0xffffff) == 0) {
	    textPixel = 0;
	    backgroundPixel = 1;
	} else {
	    textPixel = 1;
	    backgroundPixel = 0;
	}
    } else {
        textPixel = physdev->x11dev->textPixel;
	backgroundPixel = physdev->x11dev->backgroundPixel;
    }

    if(flags & ETO_OPAQUE)
    {
        wine_tsx11_lock();
        XSetForeground( gdi_display, physdev->x11dev->gc, backgroundPixel );
        XFillRectangle( gdi_display, physdev->x11dev->drawable, physdev->x11dev->gc,
                        physdev->x11dev->dc_rect.left + lprect->left, physdev->x11dev->dc_rect.top + lprect->top,
                        lprect->right - lprect->left, lprect->bottom - lprect->top );
        wine_tsx11_unlock();
    }

    if(count == 0)
    {
	retv = TRUE;
        goto done_unlock;
    }

    EnterCriticalSection(&xrender_cs);

    entry = glyphsetCache + physdev->cache_index;
    aa_type = entry->aa_default;
    formatEntry = entry->format[aa_type];

    for(idx = 0; idx < count; idx++) {
        if( !formatEntry ) {
	    UploadGlyph(physdev, wstr[idx], aa_type);
            /* re-evaluate antialias since aa_default may have changed */
            aa_type = entry->aa_default;
            formatEntry = entry->format[aa_type];
        } else if( wstr[idx] >= formatEntry->nrealized || formatEntry->realized[wstr[idx]] == FALSE) {
	    UploadGlyph(physdev, wstr[idx], aa_type);
	}
    }
    if (!formatEntry)
    {
        WARN("could not upload requested glyphs\n");
        LeaveCriticalSection(&xrender_cs);
        goto done_unlock;
    }

    TRACE("Writing %s at %d,%d\n", debugstr_wn(wstr,count),
          physdev->x11dev->dc_rect.left + x, physdev->x11dev->dc_rect.top + y);

    if(X11DRV_XRender_Installed)
    {
        XGlyphElt16 *elts = HeapAlloc(GetProcessHeap(), 0, sizeof(XGlyphElt16) * count);
        POINT offset = {0, 0};
        POINT desired, current;
        int render_op = PictOpOver;
        Picture pict = get_xrender_picture( physdev, 0, (flags & ETO_CLIPPED) ? lprect : NULL );
        XRenderColor col;

        /* There's a bug in XRenderCompositeText that ignores the xDst and yDst parameters.
           So we pass zeros to the function and move to our starting position using the first
           element of the elts array. */

        desired.x = physdev->x11dev->dc_rect.left + x;
        desired.y = physdev->x11dev->dc_rect.top + y;
        current.x = current.y = 0;

        get_xrender_color(physdev->pict_format, physdev->x11dev->textPixel, &col);
        tile_pict = get_tile_pict(physdev->format, &col);

	/* FIXME the mapping of Text/BkColor onto 1 or 0 needs investigation.
	 */
	if((physdev->format == WXR_FORMAT_MONO) && (textPixel == 0))
	    render_op = PictOpOutReverse; /* This gives us 'black' text */

        for(idx = 0; idx < count; idx++)
        {
            elts[idx].glyphset = formatEntry->glyphset;
            elts[idx].chars = wstr + idx;
            elts[idx].nchars = 1;
            elts[idx].xOff = desired.x - current.x;
            elts[idx].yOff = desired.y - current.y;

            current.x += (elts[idx].xOff + formatEntry->gis[wstr[idx]].xOff);
            current.y += (elts[idx].yOff + formatEntry->gis[wstr[idx]].yOff);

            if(!lpDx)
            {
                desired.x += formatEntry->gis[wstr[idx]].xOff;
                desired.y += formatEntry->gis[wstr[idx]].yOff;
            }
            else
            {
                if(flags & ETO_PDY)
                {
                    offset.x += lpDx[idx * 2];
                    offset.y += lpDx[idx * 2 + 1];
                }
                else
                    offset.x += lpDx[idx];
                desired.x = physdev->x11dev->dc_rect.left + x + offset.x;
                desired.y = physdev->x11dev->dc_rect.top  + y + offset.y;
            }
        }

        wine_tsx11_lock();
        /* Make sure we don't have any transforms set from a previous call */
        set_xrender_transformation(pict, 1, 1, 0, 0);
        pXRenderCompositeText16(gdi_display, render_op,
                                tile_pict,
                                pict,
                                formatEntry->font_format,
                                0, 0, 0, 0, elts, count);
        wine_tsx11_unlock();
        HeapFree(GetProcessHeap(), 0, elts);
    } else {
        POINT offset = {0, 0};

        if (flags & ETO_CLIPPED)
        {
            HRGN clip_region = CreateRectRgnIndirect( lprect );
            saved_region = add_extra_clipping_region( physdev->x11dev, clip_region );
            DeleteObject( clip_region );
        }

        wine_tsx11_lock();
	XSetForeground( gdi_display, physdev->x11dev->gc, textPixel );

        if(aa_type == AA_None || physdev->x11dev->depth == 1)
        {
            void (* sharp_glyph_fn)(struct xrender_physdev *, INT, INT, void *, XGlyphInfo *);

            if(aa_type == AA_None)
                sharp_glyph_fn = SharpGlyphMono;
            else
                sharp_glyph_fn = SharpGlyphGray;

	    for(idx = 0; idx < count; idx++) {
                sharp_glyph_fn(physdev,
                               physdev->x11dev->dc_rect.left + x + offset.x,
                               physdev->x11dev->dc_rect.top  + y + offset.y,
			       formatEntry->bitmaps[wstr[idx]],
			       &formatEntry->gis[wstr[idx]]);
                if(lpDx)
                {
                    if(flags & ETO_PDY)
                    {
                        offset.x += lpDx[idx * 2];
                        offset.y += lpDx[idx * 2 + 1];
                    }
                    else
                        offset.x += lpDx[idx];
                }
                else
                {
                    offset.x += formatEntry->gis[wstr[idx]].xOff;
                    offset.y += formatEntry->gis[wstr[idx]].yOff;
		}
	    }
	} else {
	    XImage *image;
	    int image_x, image_y, image_off_x, image_off_y, image_w, image_h;
	    RECT extents = {0, 0, 0, 0};
	    POINT cur = {0, 0};
            int w = physdev->x11dev->drawable_rect.right - physdev->x11dev->drawable_rect.left;
            int h = physdev->x11dev->drawable_rect.bottom - physdev->x11dev->drawable_rect.top;

	    TRACE("drawable %dx%d\n", w, h);

	    for(idx = 0; idx < count; idx++) {
	        if(extents.left > cur.x - formatEntry->gis[wstr[idx]].x)
		    extents.left = cur.x - formatEntry->gis[wstr[idx]].x;
		if(extents.top > cur.y - formatEntry->gis[wstr[idx]].y)
		    extents.top = cur.y - formatEntry->gis[wstr[idx]].y;
		if(extents.right < cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width)
		    extents.right = cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width;
		if(extents.bottom < cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height)
		    extents.bottom = cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height;

                if(lpDx)
                {
                    if(flags & ETO_PDY)
                    {
                        cur.x += lpDx[idx * 2];
                        cur.y += lpDx[idx * 2 + 1];
                    }
                    else
                        cur.x += lpDx[idx];
                }
                else
                {
                    cur.x += formatEntry->gis[wstr[idx]].xOff;
                    cur.y += formatEntry->gis[wstr[idx]].yOff;
		}
	    }
	    TRACE("glyph extents %d,%d - %d,%d drawable x,y %d,%d\n", extents.left, extents.top,
		  extents.right, extents.bottom, physdev->x11dev->dc_rect.left + x, physdev->x11dev->dc_rect.top + y);

	    if(physdev->x11dev->dc_rect.left + x + extents.left >= 0) {
	        image_x = physdev->x11dev->dc_rect.left + x + extents.left;
		image_off_x = 0;
	    } else {
	        image_x = 0;
		image_off_x = physdev->x11dev->dc_rect.left + x + extents.left;
	    }
	    if(physdev->x11dev->dc_rect.top + y + extents.top >= 0) {
	        image_y = physdev->x11dev->dc_rect.top + y + extents.top;
		image_off_y = 0;
	    } else {
	        image_y = 0;
		image_off_y = physdev->x11dev->dc_rect.top + y + extents.top;
	    }
	    if(physdev->x11dev->dc_rect.left + x + extents.right < w)
	        image_w = physdev->x11dev->dc_rect.left + x + extents.right - image_x;
	    else
	        image_w = w - image_x;
	    if(physdev->x11dev->dc_rect.top + y + extents.bottom < h)
	        image_h = physdev->x11dev->dc_rect.top + y + extents.bottom - image_y;
	    else
	        image_h = h - image_y;

	    if(image_w <= 0 || image_h <= 0) goto no_image;

	    X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
	    image = XGetImage(gdi_display, physdev->x11dev->drawable,
			      image_x, image_y, image_w, image_h,
			      AllPlanes, ZPixmap);
	    X11DRV_check_error();

	    TRACE("XGetImage(%p, %x, %d, %d, %d, %d, %lx, %x) depth = %d rets %p\n",
		  gdi_display, (int)physdev->x11dev->drawable, image_x, image_y,
		  image_w, image_h, AllPlanes, ZPixmap,
		  physdev->x11dev->depth, image);
	    if(!image) {
	        Pixmap xpm = XCreatePixmap(gdi_display, root_window, image_w, image_h,
					   physdev->x11dev->depth);
		GC gc;
		XGCValues gcv;

		gcv.graphics_exposures = False;
		gc = XCreateGC(gdi_display, xpm, GCGraphicsExposures, &gcv);
		XCopyArea(gdi_display, physdev->x11dev->drawable, xpm, gc, image_x, image_y,
			  image_w, image_h, 0, 0);
		XFreeGC(gdi_display, gc);
		X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
		image = XGetImage(gdi_display, xpm, 0, 0, image_w, image_h, AllPlanes,
				  ZPixmap);
		X11DRV_check_error();
		XFreePixmap(gdi_display, xpm);
	    }
	    if(!image) goto no_image;

	    image->red_mask = visual->red_mask;
	    image->green_mask = visual->green_mask;
	    image->blue_mask = visual->blue_mask;

	    for(idx = 0; idx < count; idx++) {
	        SmoothGlyphGray(image,
                                offset.x + image_off_x - extents.left,
                                offset.y + image_off_y - extents.top,
				formatEntry->bitmaps[wstr[idx]],
				&formatEntry->gis[wstr[idx]],
				physdev->x11dev->textPixel);
                if(lpDx)
                {
                    if(flags & ETO_PDY)
                    {
                        offset.x += lpDx[idx * 2];
                        offset.y += lpDx[idx * 2 + 1];
                    }
                    else
                        offset.x += lpDx[idx];
                }
                else
                {
                    offset.x += formatEntry->gis[wstr[idx]].xOff;
                    offset.y += formatEntry->gis[wstr[idx]].yOff;
                }
	    }
	    XPutImage(gdi_display, physdev->x11dev->drawable, physdev->x11dev->gc, image, 0, 0,
		      image_x, image_y, image_w, image_h);
	    XDestroyImage(image);
	}
    no_image:
	wine_tsx11_unlock();
        restore_clipping_region( physdev->x11dev, saved_region );
    }
    LeaveCriticalSection(&xrender_cs);
    retv = TRUE;

done_unlock:
    X11DRV_UnlockDIBSection( physdev->x11dev, TRUE );
    return retv;
}

/* multiply the alpha channel of a picture */
static void multiply_alpha( Picture pict, XRenderPictFormat *format, int alpha,
                            int x, int y, int width, int height )
{
    XRenderPictureAttributes pa;
    Pixmap src_pixmap, mask_pixmap;
    Picture src_pict, mask_pict;
    XRenderColor color;

    wine_tsx11_lock();
    src_pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, format->depth );
    mask_pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, format->depth );
    pa.repeat = RepeatNormal;
    src_pict = pXRenderCreatePicture( gdi_display, src_pixmap, format, CPRepeat, &pa );
    pa.component_alpha = True;
    mask_pict = pXRenderCreatePicture( gdi_display, mask_pixmap, format, CPRepeat|CPComponentAlpha, &pa );
    color.red = color.green = color.blue = color.alpha = 0xffff;
    pXRenderFillRectangle( gdi_display, PictOpSrc, src_pict, &color, 0, 0, 1, 1 );
    color.alpha = alpha;
    pXRenderFillRectangle( gdi_display, PictOpSrc, mask_pict, &color, 0, 0, 1, 1 );
    pXRenderComposite( gdi_display, PictOpInReverse, src_pict, mask_pict, pict,
                       0, 0, 0, 0, x, y, width, height );
    pXRenderFreePicture( gdi_display, src_pict );
    pXRenderFreePicture( gdi_display, mask_pict );
    XFreePixmap( gdi_display, src_pixmap );
    XFreePixmap( gdi_display, mask_pixmap );
    wine_tsx11_unlock();
}

/* Helper function for (stretched) blitting using xrender */
static void xrender_blit( int op, Picture src_pict, Picture mask_pict, Picture dst_pict,
                          int x_src, int y_src, int x_dst, int y_dst,
                          double xscale, double yscale, int width, int height )
{
    int x_offset, y_offset;

    /* When we need to scale we perform scaling and source_x / source_y translation using a transformation matrix.
     * This is needed because XRender is inaccurate in combination with scaled source coordinates passed to XRenderComposite.
     * In all other cases we do use XRenderComposite for translation as it is faster than using a transformation matrix. */
    wine_tsx11_lock();
    if(xscale != 1.0 || yscale != 1.0)
    {
        /* In case of mirroring we need a source x- and y-offset because without the pixels will be
         * in the wrong quadrant of the x-y plane.
         */
        x_offset = (xscale < 0) ? -width : 0;
        y_offset = (yscale < 0) ? -height : 0;
        set_xrender_transformation(src_pict, xscale, yscale, x_src, y_src);
    }
    else
    {
        x_offset = x_src;
        y_offset = y_src;
        set_xrender_transformation(src_pict, 1, 1, 0, 0);
    }
    pXRenderComposite( gdi_display, op, src_pict, mask_pict, dst_pict,
                       x_offset, y_offset, 0, 0, x_dst, y_dst, width, height );
    wine_tsx11_unlock();
}

/* Helper function for (stretched) mono->color blitting using xrender */
static void xrender_mono_blit( Picture src_pict, Picture dst_pict,
                               enum wxr_format dst_format, XRenderColor *fg, XRenderColor *bg,
                               int x_src, int y_src, int x_dst, int y_dst,
                               double xscale, double yscale, int width, int height )
{
    Picture tile_pict;
    int x_offset, y_offset;
    XRenderColor color;

    /* When doing a mono->color blit, the source data is used as mask, and the source picture
     * contains a 1x1 picture for tiling. The source data effectively acts as an alpha channel to
     * the tile data.
     */
    EnterCriticalSection( &xrender_cs );
    color = *bg;
    color.alpha = 0xffff;  /* tile pict needs 100% alpha */
    tile_pict = get_tile_pict( dst_format, &color );

    wine_tsx11_lock();
    pXRenderFillRectangle( gdi_display, PictOpSrc, dst_pict, fg, x_dst, y_dst, width, height );

    if (xscale != 1.0 || yscale != 1.0)
    {
        /* In case of mirroring we need a source x- and y-offset because without the pixels will be
         * in the wrong quadrant of the x-y plane.
         */
        x_offset = (xscale < 0) ? -width : 0;
        y_offset = (yscale < 0) ? -height : 0;
        set_xrender_transformation(src_pict, xscale, yscale, x_src, y_src);
    }
    else
    {
        x_offset = x_src;
        y_offset = y_src;
        set_xrender_transformation(src_pict, 1, 1, 0, 0);
    }
    pXRenderComposite(gdi_display, PictOpOver, tile_pict, src_pict, dst_pict,
                      0, 0, x_offset, y_offset, x_dst, y_dst, width, height );
    wine_tsx11_unlock();
    LeaveCriticalSection( &xrender_cs );

    /* force the alpha channel for background pixels, it has been set to 100% by the tile */
    if (bg->alpha != 0xffff && (dst_format == WXR_FORMAT_A8R8G8B8 || dst_format == WXR_FORMAT_B8G8R8A8))
        multiply_alpha( dst_pict, pict_formats[dst_format], bg->alpha, x_dst, y_dst, width, height );
}

/* create a pixmap and render picture for an image */
static DWORD create_image_pixmap( BITMAPINFO *info, const struct gdi_image_bits *bits,
                                  struct bitblt_coords *src, enum wxr_format format,
                                  Pixmap *pixmap, Picture *pict, BOOL *use_repeat )
{
    DWORD ret;
    int width = src->visrect.right - src->visrect.left;
    int height = src->visrect.bottom - src->visrect.top;
    int depth = pict_formats[format]->depth;
    struct gdi_image_bits dst_bits;
    XRenderPictureAttributes pa;
    XImage *image;

    wine_tsx11_lock();
    image = XCreateImage( gdi_display, visual, depth, ZPixmap, 0, NULL,
                          info->bmiHeader.biWidth, height, 32, 0 );
    wine_tsx11_unlock();
    if (!image) return ERROR_OUTOFMEMORY;

    ret = copy_image_bits( info, (format == WXR_FORMAT_R8G8B8), image, bits, &dst_bits, src, NULL, ~0u );
    if (ret) return ret;

    image->data = dst_bits.ptr;
    /* hack: make sure the bits are readable if we are reading from a DIB section */
    /* to be removed once we get rid of DIB access protections */
    if (!dst_bits.is_copy) IsBadReadPtr( dst_bits.ptr, image->height * image->bytes_per_line );

    *use_repeat = (width == 1 && height == 1);
    pa.repeat = *use_repeat ? RepeatNormal : RepeatNone;

    wine_tsx11_lock();
    *pixmap = XCreatePixmap( gdi_display, root_window, width, height, depth );
    XPutImage( gdi_display, *pixmap, get_bitmap_gc( depth ), image,
               src->visrect.left, 0, 0, 0, width, height );
    *pict = pXRenderCreatePicture( gdi_display, *pixmap, pict_formats[format], CPRepeat, &pa );
    wine_tsx11_unlock();

    /* make coordinates relative to the pixmap */
    src->x -= src->visrect.left;
    src->y -= src->visrect.top;
    OffsetRect( &src->visrect, -src->visrect.left, -src->visrect.top );

    image->data = NULL;
    wine_tsx11_lock();
    XDestroyImage( image );
    wine_tsx11_unlock();
    if (dst_bits.free) dst_bits.free( &dst_bits );
    return ret;
}

static void xrender_stretch_blit( struct xrender_physdev *physdev_src, struct xrender_physdev *physdev_dst,
                                  Drawable drawable, const struct bitblt_coords *src,
                                  const struct bitblt_coords *dst )
{
    int width = abs( dst->width );
    int height = abs( dst->height );
    int x_src = physdev_src->x11dev->dc_rect.left + src->x;
    int y_src = physdev_src->x11dev->dc_rect.top + src->y;
    int x_dst, y_dst;
    Picture src_pict = 0, dst_pict, mask_pict = 0;
    BOOL use_repeat;
    double xscale, yscale;

    use_repeat = use_source_repeat( physdev_src );
    if (!use_repeat)
    {
        xscale = src->width / (double)dst->width;
        yscale = src->height / (double)dst->height;
    }
    else xscale = yscale = 1;  /* no scaling needed with a repeating source */

    if (drawable)  /* using an intermediate pixmap */
    {
        XRenderPictureAttributes pa;

        x_dst = dst->x;
        y_dst = dst->y;
        pa.repeat = RepeatNone;
        wine_tsx11_lock();
        dst_pict = pXRenderCreatePicture( gdi_display, drawable, physdev_dst->pict_format, CPRepeat, &pa );
        wine_tsx11_unlock();
    }
    else
    {
        x_dst = physdev_dst->x11dev->dc_rect.left + dst->x;
        y_dst = physdev_dst->x11dev->dc_rect.top + dst->y;
        dst_pict = get_xrender_picture( physdev_dst, 0, &dst->visrect );
    }

    if (src->width < 0) x_src += src->width + 1;
    if (src->height < 0) y_src += src->height + 1;
    if (dst->width < 0) x_dst += dst->width + 1;
    if (dst->height < 0) y_dst += dst->height + 1;

    src_pict = get_xrender_picture_source( physdev_src, use_repeat );

    /* mono -> color */
    if (physdev_src->format == WXR_FORMAT_MONO && physdev_dst->format != WXR_FORMAT_MONO)
    {
        XRenderColor fg, bg;

        get_xrender_color( physdev_dst->pict_format, physdev_dst->x11dev->textPixel, &fg );
        get_xrender_color( physdev_dst->pict_format, physdev_dst->x11dev->backgroundPixel, &bg );
        fg.alpha = bg.alpha = 0;

        xrender_mono_blit( src_pict, dst_pict, physdev_dst->format, &fg, &bg,
                           x_src, y_src, x_dst, y_dst, xscale, yscale, width, height );
    }
    else /* color -> color (can be at different depths) or mono -> mono */
    {
        if (physdev_dst->x11dev->depth == 32 && physdev_src->x11dev->depth < 32)
            mask_pict = get_no_alpha_mask();

        xrender_blit( PictOpSrc, src_pict, mask_pict, dst_pict,
                      x_src, y_src, x_dst, y_dst, xscale, yscale, width, height );
    }

    if (drawable)
    {
        wine_tsx11_lock();
        pXRenderFreePicture( gdi_display, dst_pict );
        wine_tsx11_unlock();
    }
}


static void xrender_put_image( Pixmap src_pixmap, Picture src_pict, Picture mask_pict, HRGN clip,
                               XRenderPictFormat *dst_format, struct xrender_physdev *physdev,
                               Drawable drawable, struct bitblt_coords *src,
                               struct bitblt_coords *dst, BOOL use_repeat )
{
    int x_src, y_src, x_dst, y_dst;
    Picture dst_pict;
    XRenderPictureAttributes pa;
    double xscale, yscale;

    if (drawable)  /* using an intermediate pixmap */
    {
        RGNDATA *clip_data = NULL;

        if (clip) clip_data = X11DRV_GetRegionData( clip, 0 );
        x_dst = dst->x;
        y_dst = dst->y;
        pa.repeat = RepeatNone;
        wine_tsx11_lock();
        dst_pict = pXRenderCreatePicture( gdi_display, drawable, dst_format, CPRepeat, &pa );
        if (clip_data)
            pXRenderSetPictureClipRectangles( gdi_display, dst_pict, 0, 0,
                                              (XRectangle *)clip_data->Buffer, clip_data->rdh.nCount );
        wine_tsx11_unlock();
        HeapFree( GetProcessHeap(), 0, clip_data );
    }
    else
    {
        x_dst = physdev->x11dev->dc_rect.left + dst->x;
        y_dst = physdev->x11dev->dc_rect.top + dst->y;
        dst_pict = get_xrender_picture( physdev, clip, &dst->visrect );
    }

    if (!use_repeat)
    {
        xscale = src->width / (double)dst->width;
        yscale = src->height / (double)dst->height;
    }
    else xscale = yscale = 1;  /* no scaling needed with a repeating source */

    x_src = src->x;
    y_src = src->y;
    if (src->width < 0) x_src += src->width + 1;
    if (src->height < 0) y_src += src->height + 1;
    if (dst->width < 0) x_dst += dst->width + 1;
    if (dst->height < 0) y_dst += dst->height + 1;

    xrender_blit( PictOpSrc, src_pict, mask_pict, dst_pict, x_src, y_src, x_dst, y_dst,
                  xscale, yscale, abs( dst->width ), abs( dst->height ));

    if (drawable)
    {
        wine_tsx11_lock();
        pXRenderFreePicture( gdi_display, dst_pict );
        wine_tsx11_unlock();
    }
}


/***********************************************************************
 *           xrenderdrv_StretchBlt
 */
static BOOL xrenderdrv_StretchBlt( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                   PHYSDEV src_dev, struct bitblt_coords *src, DWORD rop )
{
    struct xrender_physdev *physdev_dst = get_xrender_dev( dst_dev );
    struct xrender_physdev *physdev_src = get_xrender_dev( src_dev );
    BOOL stretch = (src->width != dst->width) || (src->height != dst->height);

    if (src_dev->funcs != dst_dev->funcs)
    {
        dst_dev = GET_NEXT_PHYSDEV( dst_dev, pStretchBlt );
        return dst_dev->funcs->pStretchBlt( dst_dev, dst, src_dev, src, rop );
    }

    if (!X11DRV_XRender_Installed) goto x11drv_fallback;

    /* XRender is of no use for color -> mono */
    if (physdev_dst->format == WXR_FORMAT_MONO && physdev_src->format != WXR_FORMAT_MONO)
        goto x11drv_fallback;

    /* if not stretching, we only need to handle format conversion */
    if (!stretch && physdev_dst->format == physdev_src->format) goto x11drv_fallback;

    X11DRV_LockDIBSection( physdev_dst->x11dev, DIB_Status_GdiMod );
    if (physdev_dst != physdev_src) X11DRV_LockDIBSection( physdev_src->x11dev, DIB_Status_GdiMod );

    if (rop != SRCCOPY)
    {
        GC tmpGC;
        Pixmap tmp_pixmap;
        struct bitblt_coords tmp;

        /* make coordinates relative to tmp pixmap */
        tmp = *dst;
        tmp.x -= tmp.visrect.left;
        tmp.y -= tmp.visrect.top;
        OffsetRect( &tmp.visrect, -tmp.visrect.left, -tmp.visrect.top );

        wine_tsx11_lock();
        tmpGC = XCreateGC( gdi_display, physdev_dst->x11dev->drawable, 0, NULL );
        XSetSubwindowMode( gdi_display, tmpGC, IncludeInferiors );
        XSetGraphicsExposures( gdi_display, tmpGC, False );
        tmp_pixmap = XCreatePixmap( gdi_display, root_window, tmp.visrect.right - tmp.visrect.left,
                                    tmp.visrect.bottom - tmp.visrect.top, physdev_dst->x11dev->depth );
        wine_tsx11_unlock();

        xrender_stretch_blit( physdev_src, physdev_dst, tmp_pixmap, src, &tmp );
        execute_rop( physdev_dst->x11dev, tmp_pixmap, tmpGC, &dst->visrect, rop );

        wine_tsx11_lock();
        XFreePixmap( gdi_display, tmp_pixmap );
        XFreeGC( gdi_display, tmpGC );
        wine_tsx11_unlock();
    }
    else xrender_stretch_blit( physdev_src, physdev_dst, 0, src, dst );

    if (physdev_dst != physdev_src) X11DRV_UnlockDIBSection( physdev_src->x11dev, FALSE );
    X11DRV_UnlockDIBSection( physdev_dst->x11dev, TRUE );
    return TRUE;

x11drv_fallback:
    return X11DRV_StretchBlt( &physdev_dst->x11dev->dev, dst, &physdev_src->x11dev->dev, src, rop );
}


/***********************************************************************
 *           xrenderdrv_PutImage
 */
static DWORD xrenderdrv_PutImage( PHYSDEV dev, HBITMAP hbitmap, HRGN clip, BITMAPINFO *info,
                                  const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                  struct bitblt_coords *dst, DWORD rop )
{
    struct xrender_physdev *physdev;
    X_PHYSBITMAP *bitmap;
    DWORD ret;
    Pixmap tmp_pixmap;
    GC gc;
    enum wxr_format src_format, dst_format;
    XRenderPictFormat *pict_format;
    Pixmap src_pixmap;
    Picture src_pict, mask_pict = 0;
    BOOL use_repeat;

    if (!X11DRV_XRender_Installed) goto x11drv_fallback;

    if (hbitmap)
    {
        if (!(bitmap = X11DRV_get_phys_bitmap( hbitmap ))) return ERROR_INVALID_HANDLE;
        physdev = NULL;
        dst_format = get_xrender_format_from_color_shifts( bitmap->depth, &bitmap->color_shifts );
    }
    else
    {
        physdev = get_xrender_dev( dev );
        bitmap = NULL;
        dst_format = physdev->format;
    }

    src_format = get_xrender_format_from_bitmapinfo( info );
    if (!(pict_format = pict_formats[src_format])) goto update_format;

    /* make sure we can create an image with the same bpp */
    if (info->bmiHeader.biBitCount != pixmap_formats[pict_format->depth]->bits_per_pixel)
        goto update_format;

    /* mono <-> color conversions not supported */
    if ((src_format != dst_format) && (src_format == WXR_FORMAT_MONO || dst_format == WXR_FORMAT_MONO))
        goto x11drv_fallback;

    if (!bits) return ERROR_SUCCESS;  /* just querying the format */

    if (!has_alpha( src_format ) && has_alpha( dst_format )) mask_pict = get_no_alpha_mask();

    ret = create_image_pixmap( info, bits, src, src_format, &src_pixmap, &src_pict, &use_repeat );
    if (!ret)
    {
        struct bitblt_coords tmp;

        if (bitmap)
        {
            HRGN rgn = CreateRectRgnIndirect( &dst->visrect );
            if (clip) CombineRgn( rgn, rgn, clip, RGN_AND );

            X11DRV_DIB_Lock( bitmap, DIB_Status_GdiMod );

            xrender_put_image( src_pixmap, src_pict, mask_pict, rgn,
                               pict_formats[dst_format], NULL, bitmap->pixmap, src, dst, use_repeat );

            X11DRV_DIB_Unlock( bitmap, TRUE );
            DeleteObject( rgn );
        }
        else
        {
            X11DRV_LockDIBSection( physdev->x11dev, DIB_Status_GdiMod );

            if (rop != SRCCOPY)
            {
                RGNDATA *clip_data = NULL;

                /* make coordinates relative to tmp pixmap */
                tmp = *dst;
                tmp.x -= tmp.visrect.left;
                tmp.y -= tmp.visrect.top;
                OffsetRect( &tmp.visrect, -tmp.visrect.left, -tmp.visrect.top );

                if (clip) clip_data = add_extra_clipping_region( physdev->x11dev, clip );

                wine_tsx11_lock();
                gc = XCreateGC( gdi_display, physdev->x11dev->drawable, 0, NULL );
                XSetSubwindowMode( gdi_display, gc, IncludeInferiors );
                XSetGraphicsExposures( gdi_display, gc, False );
                tmp_pixmap = XCreatePixmap( gdi_display, root_window, tmp.visrect.right - tmp.visrect.left,
                                            tmp.visrect.bottom - tmp.visrect.top, physdev->x11dev->depth );
                wine_tsx11_unlock();

                xrender_put_image( src_pixmap, src_pict, mask_pict, NULL, physdev->pict_format,
                                   NULL, tmp_pixmap, src, &tmp, use_repeat );
                execute_rop( physdev->x11dev, tmp_pixmap, gc, &dst->visrect, rop );

                wine_tsx11_lock();
                XFreePixmap( gdi_display, tmp_pixmap );
                XFreeGC( gdi_display, gc );
                wine_tsx11_unlock();

                restore_clipping_region( physdev->x11dev, clip_data );
            }
            else xrender_put_image( src_pixmap, src_pict, mask_pict, clip,
                                    physdev->pict_format, physdev, 0, src, dst, use_repeat );

            X11DRV_UnlockDIBSection( physdev->x11dev, TRUE );
        }

        wine_tsx11_lock();
        pXRenderFreePicture( gdi_display, src_pict );
        XFreePixmap( gdi_display, src_pixmap );
        wine_tsx11_unlock();
    }
    return ret;

update_format:
    if (info->bmiHeader.biHeight > 0) info->bmiHeader.biHeight = -info->bmiHeader.biHeight;
    set_color_info( pict_formats[dst_format], info );
    return ERROR_BAD_FORMAT;

x11drv_fallback:
    if (hbitmap) return X11DRV_PutImage( dev, hbitmap, clip, info, bits, src, dst, rop );
    dev = GET_NEXT_PHYSDEV( dev, pPutImage );
    return dev->funcs->pPutImage( dev, hbitmap, clip, info, bits, src, dst, rop );
}


/***********************************************************************
 *           xrenderdrv_BlendImage
 */
static DWORD xrenderdrv_BlendImage( PHYSDEV dev, BITMAPINFO *info, const struct gdi_image_bits *bits,
                                    struct bitblt_coords *src, struct bitblt_coords *dst,
                                    BLENDFUNCTION func )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    DWORD ret;
    enum wxr_format format;
    XRenderPictFormat *pict_format;
    Picture dst_pict, src_pict, mask_pict;
    Pixmap src_pixmap;
    BOOL use_repeat;

    if (!X11DRV_XRender_Installed)
    {
        dev = GET_NEXT_PHYSDEV( dev, pBlendImage );
        return dev->funcs->pBlendImage( dev, info, bits, src, dst, func );
    }

    format = get_xrender_format_from_bitmapinfo( info );
    if (!(func.AlphaFormat & AC_SRC_ALPHA))
        format = get_format_without_alpha( format );
    else if (format != WXR_FORMAT_A8R8G8B8)
        return ERROR_INVALID_PARAMETER;

    if (!(pict_format = pict_formats[format])) goto update_format;

    /* make sure we can create an image with the same bpp */
    if (info->bmiHeader.biBitCount != pixmap_formats[pict_format->depth]->bits_per_pixel)
        goto update_format;

    if (format == WXR_FORMAT_MONO && physdev->format != WXR_FORMAT_MONO)
        goto update_format;

    if (!bits) return ERROR_SUCCESS;  /* just querying the format */

    ret = create_image_pixmap( info, bits, src, format, &src_pixmap, &src_pict, &use_repeat );
    if (!ret)
    {
        double xscale, yscale;

        X11DRV_LockDIBSection( physdev->x11dev, DIB_Status_GdiMod );

        if (!use_repeat)
        {
            xscale = src->width / (double)dst->width;
            yscale = src->height / (double)dst->height;
        }
        else xscale = yscale = 1;  /* no scaling needed with a repeating source */

        dst_pict = get_xrender_picture( physdev, 0, &dst->visrect );

        EnterCriticalSection( &xrender_cs );
        mask_pict = get_mask_pict( func.SourceConstantAlpha * 257 );

        xrender_blit( PictOpOver, src_pict, mask_pict, dst_pict, src->x, src->y,
                      physdev->x11dev->dc_rect.left + dst->x,
                      physdev->x11dev->dc_rect.top + dst->y,
                      xscale, yscale, dst->width, dst->height );

        wine_tsx11_lock();
        pXRenderFreePicture( gdi_display, src_pict );
        XFreePixmap( gdi_display, src_pixmap );
        wine_tsx11_unlock();

        LeaveCriticalSection( &xrender_cs );

        X11DRV_UnlockDIBSection( physdev->x11dev, TRUE );
    }
    return ret;

update_format:
    if (info->bmiHeader.biHeight > 0) info->bmiHeader.biHeight = -info->bmiHeader.biHeight;
    set_color_info( physdev->pict_format, info );
    return ERROR_BAD_FORMAT;
}


/***********************************************************************
 *           xrenderdrv_AlphaBlend
 */
static BOOL xrenderdrv_AlphaBlend( PHYSDEV dst_dev, struct bitblt_coords *dst,
                                   PHYSDEV src_dev, struct bitblt_coords *src, BLENDFUNCTION blendfn )
{
    struct xrender_physdev *physdev_dst = get_xrender_dev( dst_dev );
    struct xrender_physdev *physdev_src = get_xrender_dev( src_dev );
    Picture dst_pict, src_pict = 0, mask_pict = 0, tmp_pict = 0;
    XRenderPictureAttributes pa;
    Pixmap tmp_pixmap = 0;
    double xscale, yscale;
    BOOL use_repeat;

    if (!X11DRV_XRender_Installed || src_dev->funcs != dst_dev->funcs)
    {
        dst_dev = GET_NEXT_PHYSDEV( dst_dev, pAlphaBlend );
        return dst_dev->funcs->pAlphaBlend( dst_dev, dst, src_dev, src, blendfn );
    }

    if ((blendfn.AlphaFormat & AC_SRC_ALPHA) && physdev_src->format != WXR_FORMAT_A8R8G8B8)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    X11DRV_LockDIBSection( physdev_dst->x11dev, DIB_Status_GdiMod );
    if (physdev_dst != physdev_src) X11DRV_LockDIBSection( physdev_src->x11dev, DIB_Status_GdiMod );

    dst_pict = get_xrender_picture( physdev_dst, 0, &dst->visrect );

    use_repeat = use_source_repeat( physdev_src );
    if (!use_repeat)
    {
        xscale = src->width / (double)dst->width;
        yscale = src->height / (double)dst->height;
    }
    else xscale = yscale = 1;  /* no scaling needed with a repeating source */

    src_pict = get_xrender_picture_source( physdev_src, use_repeat );

    if (physdev_src->format == WXR_FORMAT_MONO && physdev_dst->format != WXR_FORMAT_MONO)
    {
        /* mono -> color blending needs an intermediate color pixmap */
        XRenderColor fg, bg;
        int width = src->visrect.right - src->visrect.left;
        int height = src->visrect.bottom - src->visrect.top;

        /* blending doesn't use the destination DC colors */
        fg.red = fg.green = fg.blue = 0;
        bg.red = bg.green = bg.blue = 0xffff;
        fg.alpha = bg.alpha = 0xffff;

        wine_tsx11_lock();
        tmp_pixmap = XCreatePixmap( gdi_display, root_window, width, height,
                                    physdev_dst->pict_format->depth );
        pa.repeat = use_repeat ? RepeatNormal : RepeatNone;
        tmp_pict = pXRenderCreatePicture( gdi_display, tmp_pixmap, physdev_dst->pict_format,
                                          CPRepeat, &pa );
        wine_tsx11_unlock();

        xrender_mono_blit( src_pict, tmp_pict, physdev_dst->format, &fg, &bg,
                           src->visrect.left, src->visrect.top, 0, 0, 1, 1, width, height );
    }
    else if (!(blendfn.AlphaFormat & AC_SRC_ALPHA) && physdev_src->pict_format)
    {
        /* we need a source picture with no alpha */
        enum wxr_format format = get_format_without_alpha( physdev_src->format );
        if (format != physdev_src->format)
        {
            wine_tsx11_lock();
            pa.subwindow_mode = IncludeInferiors;
            pa.repeat = use_repeat ? RepeatNormal : RepeatNone;
            tmp_pict = pXRenderCreatePicture( gdi_display, physdev_src->x11dev->drawable,
                                              pict_formats[format], CPSubwindowMode|CPRepeat, &pa );
            wine_tsx11_unlock();
        }
    }

    if (tmp_pict) src_pict = tmp_pict;

    EnterCriticalSection( &xrender_cs );
    mask_pict = get_mask_pict( blendfn.SourceConstantAlpha * 257 );

    xrender_blit( PictOpOver, src_pict, mask_pict, dst_pict,
                  physdev_src->x11dev->dc_rect.left + src->x,
                  physdev_src->x11dev->dc_rect.top + src->y,
                  physdev_dst->x11dev->dc_rect.left + dst->x,
                  physdev_dst->x11dev->dc_rect.top + dst->y,
                  xscale, yscale, dst->width, dst->height );

    wine_tsx11_lock();
    if (tmp_pict) pXRenderFreePicture( gdi_display, tmp_pict );
    if (tmp_pixmap) XFreePixmap( gdi_display, tmp_pixmap );
    wine_tsx11_unlock();

    LeaveCriticalSection( &xrender_cs );
    if (physdev_src != physdev_dst) X11DRV_UnlockDIBSection( physdev_src->x11dev, FALSE );
    X11DRV_UnlockDIBSection( physdev_dst->x11dev, TRUE );
    return TRUE;
}

/***********************************************************************
 *           xrenderdrv_SelectBrush
 */
static HBRUSH xrenderdrv_SelectBrush( PHYSDEV dev, HBRUSH hbrush, HBITMAP bitmap,
                                      const BITMAPINFO *info, void *bits, UINT usage )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    X_PHYSBITMAP *physbitmap;
    enum wxr_format format;
    BOOL delete_bitmap = FALSE;
    BITMAP bm;
    Pixmap pixmap;
    Picture src_pict, dst_pict;
    XRenderPictureAttributes pa;

    if (!X11DRV_XRender_Installed) goto x11drv_fallback;
    if (!bitmap && !info) goto x11drv_fallback;
    if (physdev->format == WXR_FORMAT_MONO) goto x11drv_fallback;

    if (!bitmap || !(physbitmap = X11DRV_get_phys_bitmap( bitmap )))
    {
        if (!(bitmap = create_brush_bitmap( physdev->x11dev, info, bits, usage ))) return 0;
        physbitmap = X11DRV_get_phys_bitmap( bitmap );
        delete_bitmap = TRUE;
    }

    format = get_xrender_format_from_color_shifts( physbitmap->depth, &physbitmap->color_shifts );
    if (format == WXR_FORMAT_MONO || !pict_formats[format]) goto x11drv_fallback;

    GetObjectW( bitmap, sizeof(bm), &bm );

    X11DRV_DIB_Lock( physbitmap, DIB_Status_GdiMod );

    wine_tsx11_lock();
    pixmap = XCreatePixmap( gdi_display, root_window, bm.bmWidth, bm.bmHeight,
                            physdev->pict_format->depth );

    pa.repeat = RepeatNone;
    src_pict = pXRenderCreatePicture(gdi_display, physbitmap->pixmap, pict_formats[format], CPRepeat, &pa);
    dst_pict = pXRenderCreatePicture(gdi_display, pixmap, physdev->pict_format, CPRepeat, &pa);

    xrender_blit( PictOpSrc, src_pict, 0, dst_pict, 0, 0, 0, 0, 1.0, 1.0, bm.bmWidth, bm.bmHeight );
    pXRenderFreePicture( gdi_display, src_pict );
    pXRenderFreePicture( gdi_display, dst_pict );

    if (physdev->x11dev->brush.pixmap) XFreePixmap( gdi_display, physdev->x11dev->brush.pixmap );
    physdev->x11dev->brush.pixmap = pixmap;
    physdev->x11dev->brush.fillStyle = FillTiled;
    physdev->x11dev->brush.pixel = 0;  /* ignored */
    wine_tsx11_unlock();

    X11DRV_DIB_Unlock( physbitmap, TRUE );
    if (delete_bitmap) DeleteObject( bitmap );
    return hbrush;

x11drv_fallback:
    if (delete_bitmap) DeleteObject( bitmap );
    dev = GET_NEXT_PHYSDEV( dev, pSelectBrush );
    return dev->funcs->pSelectBrush( dev, hbrush, bitmap, info, bits, usage );
}


static const struct gdi_dc_funcs xrender_funcs =
{
    NULL,                               /* pAbortDoc */
    NULL,                               /* pAbortPath */
    xrenderdrv_AlphaBlend,              /* pAlphaBlend */
    NULL,                               /* pAngleArc */
    NULL,                               /* pArc */
    NULL,                               /* pArcTo */
    NULL,                               /* pBeginPath */
    xrenderdrv_BlendImage,              /* pBlendImage */
    NULL,                               /* pChoosePixelFormat */
    NULL,                               /* pChord */
    NULL,                               /* pCloseFigure */
    xrenderdrv_CopyBitmap,              /* pCopyBitmap */
    xrenderdrv_CreateBitmap,            /* pCreateBitmap */
    xrenderdrv_CreateCompatibleDC,      /* pCreateCompatibleDC */
    xrenderdrv_CreateDC,                /* pCreateDC */
    NULL,                               /* pCreateDIBSection */
    xrenderdrv_DeleteBitmap,            /* pDeleteBitmap */
    xrenderdrv_DeleteDC,                /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pDescribePixelFormat */
    NULL,                               /* pDeviceCapabilities */
    NULL,                               /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    xrenderdrv_ExtEscape,               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    xrenderdrv_ExtTextOut,              /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGdiRealizationInfo */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    xrenderdrv_GetImage,                /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetPixelFormat */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    NULL,                               /* pGradientFill */
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    NULL,                               /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    NULL,                               /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    NULL,                               /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    NULL,                               /* pPolyPolygon */
    NULL,                               /* pPolyPolyline */
    NULL,                               /* pPolygon */
    NULL,                               /* pPolyline */
    NULL,                               /* pPolylineTo */
    xrenderdrv_PutImage,                /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    NULL,                               /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    NULL,                               /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    xrenderdrv_SelectBitmap,            /* pSelectBitmap */
    xrenderdrv_SelectBrush,             /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    xrenderdrv_SelectFont,              /* pSelectFont */
    NULL,                               /* pSelectPalette */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBColorTable */
    NULL,                               /* pSetDIBitsToDevice */
    xrenderdrv_SetDeviceClipping,       /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetPixelFormat */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    xrenderdrv_StretchBlt,              /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pSwapBuffers */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    /* OpenGL not supported */
};

#else /* SONAME_LIBXRENDER */

const struct gdi_dc_funcs *X11DRV_XRender_Init(void)
{
    TRACE("XRender support not compiled in.\n");
    return NULL;
}

void X11DRV_XRender_Finalize(void)
{
}

BOOL X11DRV_XRender_SetPhysBitmapDepth(X_PHYSBITMAP *physBitmap, int bits_pixel, const DIBSECTION *dib)
{
    return FALSE;
}

#endif /* SONAME_LIBXRENDER */
