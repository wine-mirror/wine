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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "windef.h"
#include "winbase.h"
#include "x11drv.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xrender);

#ifdef SONAME_LIBXRENDER

WINE_DECLARE_DEBUG_CHANNEL(winediag);

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
  WXR_FORMAT_ROOT,  /* placeholder for the format to use on the root window */
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

enum glyph_type { GLYPH_INDEX, GLYPH_WCHAR, GLYPH_NBTYPES };

typedef enum { AA_None = 0, AA_Grey, AA_RGB, AA_BGR, AA_VRGB, AA_VBGR, AA_MAXVALUE } AA_Type;

typedef struct
{
    GlyphSet glyphset;
    XRenderPictFormat *font_format;
    int nrealized;
    BOOL *realized;
    XGlyphInfo *gis;
} gsCacheEntryFormat;

typedef struct
{
    LFANDSIZE lfsz;
    gsCacheEntryFormat *format[GLYPH_NBTYPES][AA_MAXVALUE];
    INT count;
    INT next;
} gsCacheEntry;

struct xrender_physdev
{
    struct gdi_physdev dev;
    X11DRV_PDEVICE    *x11dev;
    HRGN               region;
    enum wxr_format    format;
    UINT               aa_flags;
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

static void *xrender_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XRenderAddGlyphs)
MAKE_FUNCPTR(XRenderChangePicture)
MAKE_FUNCPTR(XRenderComposite)
MAKE_FUNCPTR(XRenderCompositeText16)
MAKE_FUNCPTR(XRenderCreateGlyphSet)
MAKE_FUNCPTR(XRenderCreatePicture)
MAKE_FUNCPTR(XRenderSetPictureFilter)
MAKE_FUNCPTR(XRenderFillRectangle)
MAKE_FUNCPTR(XRenderFindFormat)
MAKE_FUNCPTR(XRenderFindVisualFormat)
MAKE_FUNCPTR(XRenderFreeGlyphSet)
MAKE_FUNCPTR(XRenderFreePicture)
MAKE_FUNCPTR(XRenderSetPictureClipRectangles)
#ifdef HAVE_XRENDERCREATELINEARGRADIENT
MAKE_FUNCPTR(XRenderCreateLinearGradient)
#endif
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
MAKE_FUNCPTR(XRenderSetPictureTransform)
#endif
MAKE_FUNCPTR(XRenderQueryExtension)

#undef MAKE_FUNCPTR

static pthread_mutex_t xrender_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static BOOL is_wxrformat_compatible_with_visual( const WineXRenderFormatTemplate *fmt, const XVisualInfo *visual )
{
    if (fmt->depth != visual->depth) return FALSE;
    if ((fmt->redMask << fmt->red) != visual->red_mask) return FALSE;
    if ((fmt->greenMask << fmt->green) != visual->green_mask) return FALSE;
    if ((fmt->blueMask << fmt->blue) != visual->blue_mask) return FALSE;

    /* Non-default visual is only used when alpha is required. */
    if (fmt->alphaMask && visual->visualid == default_visual.visualid) return FALSE;
    return TRUE;
}

static int load_xrender_formats(void)
{
    int count = 0;
    unsigned int i;

    for (i = 0; i < WXR_NB_FORMATS; i++)
    {
        XRenderPictFormat templ;

        if (i == WXR_FORMAT_ROOT)
        {
            pict_formats[i] = pXRenderFindVisualFormat(gdi_display,
                                                DefaultVisual( gdi_display, DefaultScreen(gdi_display) ));
            TRACE( "Loaded root pict_format with id=%#lx\n", pict_formats[i]->id );
            continue;
        }
        if (is_wxrformat_compatible_with_visual( &wxr_formats_template[i], &default_visual ))
        {
            pict_formats[i] = pXRenderFindVisualFormat(gdi_display, default_visual.visual);
            if (!pict_formats[i])
            {
                /* Xrender doesn't like DirectColor visuals, try to find a TrueColor one instead */
                if (default_visual.class == DirectColor)
                {
                    XVisualInfo info;
                    if (XMatchVisualInfo( gdi_display, default_visual.screen,
                                          default_visual.depth, TrueColor, &info ))
                    {
                        pict_formats[i] = pXRenderFindVisualFormat(gdi_display, info.visual);
                        if (pict_formats[i]) default_visual = info;
                    }
                }
            }
            if (pict_formats[i]) default_format = i;
        }
        else
        {
            unsigned long mask = 0;
            get_xrender_template(&wxr_formats_template[i], &templ, &mask);
            pict_formats[i] = pXRenderFindFormat(gdi_display, mask, &templ, 0);
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

    if (!client_side_with_render) return NULL;
    if (!(xrender_handle = dlopen(SONAME_LIBXRENDER, RTLD_NOW))) return NULL;

#define LOAD_FUNCPTR(f) if((p##f = dlsym(xrender_handle, #f)) == NULL) return NULL
#define LOAD_OPTIONAL_FUNCPTR(f) p##f = dlsym(xrender_handle, #f)
    LOAD_FUNCPTR(XRenderAddGlyphs);
    LOAD_FUNCPTR(XRenderChangePicture);
    LOAD_FUNCPTR(XRenderComposite);
    LOAD_FUNCPTR(XRenderCompositeText16);
    LOAD_FUNCPTR(XRenderCreateGlyphSet);
    LOAD_FUNCPTR(XRenderCreatePicture);
    LOAD_FUNCPTR(XRenderSetPictureFilter);
    LOAD_FUNCPTR(XRenderFillRectangle);
    LOAD_FUNCPTR(XRenderFindFormat);
    LOAD_FUNCPTR(XRenderFindVisualFormat);
    LOAD_FUNCPTR(XRenderFreeGlyphSet);
    LOAD_FUNCPTR(XRenderFreePicture);
    LOAD_FUNCPTR(XRenderSetPictureClipRectangles);
    LOAD_FUNCPTR(XRenderQueryExtension);
#ifdef HAVE_XRENDERCREATELINEARGRADIENT
    LOAD_OPTIONAL_FUNCPTR(XRenderCreateLinearGradient);
#endif
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
    LOAD_OPTIONAL_FUNCPTR(XRenderSetPictureTransform);
#endif
#undef LOAD_OPTIONAL_FUNCPTR
#undef LOAD_FUNCPTR

    if (!pXRenderQueryExtension(gdi_display, &event_base, &xrender_error_base)) return NULL;

    TRACE("Xrender is up and running error_base = %d\n", xrender_error_base);
    if(!load_xrender_formats()) /* This fails in buggy versions of libXrender.so */
    {
        ERR_(winediag)("Wine has detected that you probably have a buggy version "
                       "of libXrender.  Because of this client side font rendering "
                       "will be disabled.  Please upgrade this library.\n");
        return NULL;
    }

    if (!default_visual.red_mask || !default_visual.green_mask || !default_visual.blue_mask)
    {
        WARN("one or more of the colour masks are 0, disabling XRENDER. Try running in 16-bit mode or higher.\n");
        return NULL;
    }

    glyphsetCache = calloc( sizeof(*glyphsetCache), INIT_CACHE_SIZE );

    glyphsetCacheSize = INIT_CACHE_SIZE;
    lastfree = 0;
    for(i = 0; i < INIT_CACHE_SIZE; i++) {
        glyphsetCache[i].next = i + 1;
        glyphsetCache[i].count = -1;
    }
    glyphsetCache[i-1].next = -1;

    return &xrender_funcs;
}

/* Helper function to convert from a color packed in a 32-bit integer to a XRenderColor */
static void get_xrender_color( struct xrender_physdev *physdev, COLORREF src_color, XRenderColor *dst_color )
{
    if (src_color & (1 << 24))  /* PALETTEINDEX */
    {
        HPALETTE pal = NtGdiGetDCObject( physdev->dev.hdc, NTGDI_OBJ_PAL );
        PALETTEENTRY pal_ent;

        if (!get_palette_entries( pal, LOWORD(src_color), 1, &pal_ent ))
            get_palette_entries( pal, 0, 1, &pal_ent );
        dst_color->red   = pal_ent.peRed   * 257;
        dst_color->green = pal_ent.peGreen * 257;
        dst_color->blue  = pal_ent.peBlue  * 257;
    }
    else
    {
        if (src_color >> 16 == 0x10ff) src_color = 0; /* DIBINDEX */

        dst_color->red   = GetRValue( src_color ) * 257;
        dst_color->green = GetGValue( src_color ) * 257;
        dst_color->blue  = GetBValue( src_color ) * 257;
    }

    if (physdev->format == WXR_FORMAT_MONO && !dst_color->red && !dst_color->green && !dst_color->blue)
        dst_color->alpha = 0;
    else
        dst_color->alpha = 0xffff;
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

static void update_xrender_clipping( struct xrender_physdev *dev, HRGN rgn )
{
    XRenderPictureAttributes pa;
    RGNDATA *data;

    if (!rgn)
    {
        pa.clip_mask = None;
        pXRenderChangePicture( gdi_display, dev->pict, CPClipMask, &pa );
    }
    else if ((data = X11DRV_GetRegionData( rgn, 0 )))
    {
        pXRenderSetPictureClipRectangles( gdi_display, dev->pict,
                                          dev->x11dev->dc_rect.left, dev->x11dev->dc_rect.top,
                                          (XRectangle *)data->Buffer, data->rdh.nCount );
        free( data );
    }
}


static Picture get_xrender_picture( struct xrender_physdev *dev, HRGN clip_rgn, const RECT *clip_rect )
{
    if (!dev->pict && dev->pict_format)
    {
        XRenderPictureAttributes pa;

        pa.subwindow_mode = IncludeInferiors;
        dev->pict = pXRenderCreatePicture( gdi_display, dev->x11dev->drawable,
                                           dev->pict_format, CPSubwindowMode, &pa );
        TRACE( "Allocing pict=%lx dc=%p drawable=%08lx\n",
               dev->pict, dev->dev.hdc, dev->x11dev->drawable );
        dev->update_clip = (dev->region != 0);
    }

    if (clip_rect)
    {
        HRGN rgn = NtGdiCreateRectRgn( clip_rect->left, clip_rect->top, clip_rect->right, clip_rect->bottom );
        if (clip_rgn) NtGdiCombineRgn( rgn, rgn, clip_rgn, RGN_AND );
        if (dev->region) NtGdiCombineRgn( rgn, rgn, dev->region, RGN_AND );
        update_xrender_clipping( dev, rgn );
        NtGdiDeleteObjectApp( rgn );
    }
    else if (clip_rgn)
    {
        if (dev->region)
        {
            HRGN rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
            NtGdiCombineRgn( rgn, clip_rgn, dev->region, RGN_AND );
            update_xrender_clipping( dev, rgn );
            NtGdiDeleteObjectApp( rgn );
        }
        else update_xrender_clipping( dev, clip_rgn );
    }
    else if (dev->update_clip) update_xrender_clipping( dev, dev->region );

    dev->update_clip = (clip_rect || clip_rgn);  /* have to update again if we are using a custom region */
    return dev->pict;
}

static Picture get_xrender_picture_source( struct xrender_physdev *dev, BOOL repeat )
{
    if (!dev->pict_src && dev->pict_format)
    {
        XRenderPictureAttributes pa;

        pa.subwindow_mode = IncludeInferiors;
        pa.repeat = repeat ? RepeatNormal : RepeatNone;
        dev->pict_src = pXRenderCreatePicture( gdi_display, dev->x11dev->drawable,
                                               dev->pict_format, CPSubwindowMode|CPRepeat, &pa );

        TRACE("Allocing pict_src=%lx dc=%p drawable=%08lx repeat=%u\n",
              dev->pict_src, dev->dev.hdc, dev->x11dev->drawable, pa.repeat);
    }

    return dev->pict_src;
}

static void free_xrender_picture( struct xrender_physdev *dev )
{
    if (dev->pict || dev->pict_src)
    {
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
    }
}

/* return a mask picture used to force alpha to 0 */
static Picture get_no_alpha_mask(void)
{
    static Pixmap pixmap;
    static Picture pict;

    pthread_mutex_lock( &xrender_mutex );
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
    pthread_mutex_unlock( &xrender_mutex );
    return pict;
}

static BOOL fontcmp(LFANDSIZE *p1, LFANDSIZE *p2)
{
  if(p1->hash != p2->hash) return TRUE;
  if(memcmp(&p1->devsize, &p2->devsize, sizeof(p1->devsize))) return TRUE;
  if(memcmp(&p1->xform, &p2->xform, sizeof(p1->xform))) return TRUE;
  if(memcmp(&p1->lf, &p2->lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
  return wcsicmp( p1->lf.lfFaceName, p2->lf.lfFaceName );
}

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
    int type, format;

    for (type = 0; type < GLYPH_NBTYPES; type++)
    {
        for(format = 0; format < AA_MAXVALUE; format++) {
            gsCacheEntryFormat * formatEntry;

            if( !glyphsetCache[entry].format[type][format] )
                continue;

            formatEntry = glyphsetCache[entry].format[type][format];

            if(formatEntry->glyphset) {
                pXRenderFreeGlyphSet(gdi_display, formatEntry->glyphset);
                formatEntry->glyphset = 0;
            }
            if(formatEntry->nrealized) {
                free( formatEntry->realized );
                formatEntry->realized = NULL;
                free( formatEntry->gis );
                formatEntry->gis = NULL;
                formatEntry->nrealized = 0;
            }

            free( formatEntry );
            glyphsetCache[entry].format[type][format] = NULL;
        }
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
  
  glyphsetCache = realloc( glyphsetCache,
                           (glyphsetCacheSize + INIT_CACHE_SIZE) * sizeof(*glyphsetCache) );

  for (best = i = glyphsetCacheSize; i < glyphsetCacheSize + INIT_CACHE_SIZE; i++)
  {
    memset( &glyphsetCache[i], 0, sizeof(glyphsetCache[i]) );
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

static int GetCacheEntry( LFANDSIZE *plfsz )
{
    int ret;
    gsCacheEntry *entry;

    if((ret = LookupEntry(plfsz)) != -1) return ret;

    ret = AllocEntry();
    entry = glyphsetCache + ret;
    entry->lfsz = *plfsz;
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
  unsigned int i;

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
    *pwc = RtlUpcaseUnicodeChar( *pwc );
    pwc++;
    *pwc = RtlUpcaseUnicodeChar( *pwc );
    hash ^= two_chars;
    if(!*pwc) break;
  }
  plfsz->hash = hash;
  return;
}

static AA_Type aa_type_from_flags( UINT aa_flags )
{
    switch (aa_flags & 0x7f)
    {
    case GGO_BITMAP:
        return AA_None;
    case WINE_GGO_GRAY16_BITMAP:
        return AA_Grey;
    case WINE_GGO_HRGB_BITMAP:
        return AA_RGB;
    case WINE_GGO_HBGR_BITMAP:
        return AA_BGR;
    case WINE_GGO_VRGB_BITMAP:
        return AA_VRGB;
    case WINE_GGO_VBGR_BITMAP:
        return AA_VBGR;
    default:
        FIXME( "unknown flags %x\n", aa_flags );
        return AA_None;
    }
}

static UINT get_xft_aa_flags( const LOGFONTW *lf )
{
    char *value, *p;
    UINT ret = 0;

    switch (lf->lfQuality)
    {
    case NONANTIALIASED_QUALITY:
    case ANTIALIASED_QUALITY:
        break;
    default:
        if (!(value = XGetDefault( gdi_display, "Xft", "antialias" ))) break;
        TRACE( "got antialias '%s'\n", value );
        for (p = value; *p; p++) if ('A' <= *p && *p <= 'Z') *p += 'a' - 'A'; /* to lower */
        if (value[0] == 'f' || value[0] == 'n' || value[0] == '0' || !strcmp( value, "off" ))
        {
            ret = GGO_BITMAP;
            break;
        }
        ret = GGO_GRAY4_BITMAP;
        /* fall through */
    case CLEARTYPE_QUALITY:
    case CLEARTYPE_NATURAL_QUALITY:
        if (!(value = XGetDefault( gdi_display, "Xft", "rgba" ))) break;
        TRACE( "got rgba '%s'\n", value );
        if (!strcmp( value, "rgb" )) ret = WINE_GGO_HRGB_BITMAP;
        else if (!strcmp( value, "bgr" )) ret = WINE_GGO_HBGR_BITMAP;
        else if (!strcmp( value, "vrgb" )) ret = WINE_GGO_VRGB_BITMAP;
        else if (!strcmp( value, "vbgr" )) ret = WINE_GGO_VBGR_BITMAP;
        else if (!strcmp( value, "none" )) ret = GGO_GRAY4_BITMAP;
        break;
    }
    return ret;
}

/**********************************************************************
 *	     xrenderdrv_SelectFont
 */
static HFONT xrenderdrv_SelectFont( PHYSDEV dev, HFONT hfont, UINT *aa_flags )
{
    LFANDSIZE lfsz;
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectFont );
    DWORD mode;
    HFONT ret;

    NtGdiExtGetObjectW( hfont, sizeof(lfsz.lf), &lfsz.lf );
    if (!*aa_flags) *aa_flags = get_xft_aa_flags( &lfsz.lf );

    ret = next->funcs->pSelectFont( next, hfont, aa_flags );
    if (!ret) return 0;

    switch (*aa_flags)
    {
    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
        physdev->aa_flags = WINE_GGO_GRAY16_BITMAP;
        break;
    case 0:
        physdev->aa_flags = GGO_BITMAP;
        break;
    default:
        physdev->aa_flags = *aa_flags;
        break;
    }

    TRACE("h=%d w=%d weight=%d it=%d charset=%d name=%s\n",
          lfsz.lf.lfHeight, lfsz.lf.lfWidth, lfsz.lf.lfWeight,
          lfsz.lf.lfItalic, lfsz.lf.lfCharSet, debugstr_w(lfsz.lf.lfFaceName));
    lfsz.lf.lfWidth = abs( lfsz.lf.lfWidth );
    lfsz.devsize.cx = X11DRV_XWStoDS( dev->hdc, lfsz.lf.lfWidth );
    lfsz.devsize.cy = X11DRV_YWStoDS( dev->hdc, lfsz.lf.lfHeight );

    NtGdiGetTransform( dev->hdc, 0x204, &lfsz.xform );
    TRACE("font transform %f %f %f %f\n", lfsz.xform.eM11, lfsz.xform.eM12,
          lfsz.xform.eM21, lfsz.xform.eM22);

    NtGdiGetDCDword( dev->hdc, NtGdiGetGraphicsMode, &mode );
    if (mode == GM_COMPATIBLE)
    {
        lfsz.lf.lfOrientation = lfsz.lf.lfEscapement;
        if (lfsz.xform.eM11 * lfsz.xform.eM22 < 0)
            lfsz.lf.lfOrientation = -lfsz.lf.lfOrientation;
    }

    /* Not used fields, would break hashing */
    lfsz.xform.eDx = lfsz.xform.eDy = 0;

    lfsz_calc_hash(&lfsz);

    pthread_mutex_lock( &xrender_mutex );
    if (physdev->cache_index != -1)
        dec_ref_cache( physdev->cache_index );
    physdev->cache_index = GetCacheEntry( &lfsz );
    pthread_mutex_unlock( &xrender_mutex );
    return ret;
}

static void set_physdev_format( struct xrender_physdev *physdev, enum wxr_format format )
{
    if (physdev->x11dev->drawable == DefaultRootWindow( gdi_display ))
        physdev->format = WXR_FORMAT_ROOT;
    else
        physdev->format = format;

    physdev->pict_format = pict_formats[physdev->format];
}

static BOOL create_xrender_dc( PHYSDEV *pdev, enum wxr_format format )
{
    X11DRV_PDEVICE *x11dev = get_x11drv_dev( *pdev );
    struct xrender_physdev *physdev = calloc( 1, sizeof(*physdev) );

    if (!physdev) return FALSE;
    physdev->x11dev = x11dev;
    physdev->cache_index = -1;
    set_physdev_format( physdev, format );
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
static BOOL xrenderdrv_CreateDC( PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW* initData )
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

    pthread_mutex_lock( &xrender_mutex );
    if (physdev->cache_index != -1) dec_ref_cache( physdev->cache_index );
    pthread_mutex_unlock( &xrender_mutex );

    free( physdev );
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
            if (ret)
            {
                const struct x11drv_escape_set_drawable *set = in_data;
                enum wxr_format format = physdev->format;
                unsigned int i;

                if (set->visual.visual)
                {
                    for (i = 0; i < WXR_NB_FORMATS; ++i)
                    {
                        if (!pict_formats[i]) continue;
                        if (is_wxrformat_compatible_with_visual( &wxr_formats_template[i], &set->visual ))
                        {
                            format = i;
                            break;
                        }
                    }
                    if (i == WXR_NB_FORMATS) WARN( "Format not found for drawable visual.\n" );
                }
                free_xrender_picture( physdev );
                set_physdev_format( physdev, format );
            }
            return ret;
        }
    }
    return dev->funcs->pExtEscape( dev, escape, in_count, in_data, out_count, out_data );
}

/***********************************************************************
 *           xrenderdrv_SetDeviceClipping
 */
static void xrenderdrv_SetDeviceClipping( PHYSDEV dev, HRGN rgn )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );

    physdev->region = rgn;
    physdev->update_clip = TRUE;

    dev = GET_NEXT_PHYSDEV( dev, pSetDeviceClipping );
    dev->funcs->pSetDeviceClipping( dev, rgn );
}


/************************************************************************
 *   UploadGlyph
 *
 * Helper to ExtTextOut.  Must be called inside xrender_mutex
 */
static void UploadGlyph(struct xrender_physdev *physDev, UINT glyph, enum glyph_type type)
{
    unsigned int buflen;
    char *buf;
    Glyph gid;
    GLYPHMETRICS gm;
    XGlyphInfo gi;
    gsCacheEntry *entry = glyphsetCache + physDev->cache_index;
    gsCacheEntryFormat *formatEntry;
    UINT ggo_format = physDev->aa_flags;
    AA_Type format = aa_type_from_flags( physDev->aa_flags );
    enum wxr_format wxr_format;
    static const char zero[4];
    static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };

    if (type == GLYPH_INDEX) ggo_format |= GGO_GLYPH_INDEX;
    buflen = NtGdiGetGlyphOutline( physDev->dev.hdc, glyph, ggo_format, &gm, 0, NULL, &identity, FALSE );
    if(buflen == GDI_ERROR) {
        if(format != AA_None) {
            format = AA_None;
            physDev->aa_flags = GGO_BITMAP;
            ggo_format = (ggo_format & GGO_GLYPH_INDEX) | GGO_BITMAP;
            buflen = NtGdiGetGlyphOutline( physDev->dev.hdc, glyph, ggo_format, &gm, 0, NULL,
                                           &identity, FALSE);
        }
        if(buflen == GDI_ERROR) {
            WARN("GetGlyphOutlineW failed using default glyph\n");
            buflen = NtGdiGetGlyphOutline( physDev->dev.hdc, 0, ggo_format, &gm, 0, NULL,
                                           &identity, FALSE );
            if(buflen == GDI_ERROR) {
                WARN("GetGlyphOutlineW failed for default glyph trying for space\n");
                buflen = NtGdiGetGlyphOutline( physDev->dev.hdc, 0x20, ggo_format, &gm, 0, NULL,
                                               &identity, FALSE );
                if(buflen == GDI_ERROR) {
                    ERR("GetGlyphOutlineW for all attempts unable to upload a glyph\n");
                    return;
                }
            }
        }
        TRACE("Turning off antialiasing for this monochrome font\n");
    }

    /* If there is nothing for the current type, we create the entry. */
    if( !entry->format[type][format] ) {
        entry->format[type][format] = calloc( 1, sizeof(gsCacheEntryFormat) );
    }
    formatEntry = entry->format[type][format];

    if(formatEntry->nrealized <= glyph) {
        size_t new_size = (glyph / 128 + 1) * 128;

        formatEntry->realized = realloc( formatEntry->realized, new_size * sizeof(BOOL) );
        memset( formatEntry->realized + formatEntry->nrealized, 0,
                (new_size - formatEntry->nrealized) * sizeof(BOOL) );

        formatEntry->gis = realloc( formatEntry->gis, new_size * sizeof(formatEntry->gis[0]) );
        memset( formatEntry->gis + formatEntry->nrealized, 0,
                (new_size - formatEntry->nrealized) * sizeof(formatEntry->gis[0]) );

        formatEntry->nrealized = new_size;
    }


    if(formatEntry->glyphset == 0) {
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
                /* fall through */
            case AA_None:
                wxr_format = WXR_FORMAT_MONO;
                break;
        }

        formatEntry->font_format = pict_formats[wxr_format];
        formatEntry->glyphset = pXRenderCreateGlyphSet(gdi_display, formatEntry->font_format);
    }


    buf = calloc( 1, buflen );
    if (buflen)
        NtGdiGetGlyphOutline( physDev->dev.hdc, glyph, ggo_format, &gm, buflen, buf, &identity, FALSE );
    else
        gm.gmBlackBoxX = gm.gmBlackBoxY = 0;  /* empty glyph */
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

	pXRenderAddGlyphs(gdi_display, formatEntry->glyphset, &gid, &gi, 1,
                          buflen ? buf : zero, buflen ? buflen : sizeof(zero));
    }

    free( buf );
    formatEntry->gis[glyph] = gi;
}

/*************************************************************
 *                 get_tile_pict
 *
 * Returns an appropriate Picture for tiling the text colour.
 * Call and use result within the xrender_mutex
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

        tile->xpm = XCreatePixmap(gdi_display, root_window, 1, 1, pict_format->depth);

        pa.repeat = RepeatNormal;
        tile->pict = pXRenderCreatePicture(gdi_display, tile->xpm, pict_format, CPRepeat, &pa);

        /* init current_color to something different from text_pixel */
        tile->current_color = *color;
        tile->current_color.red ^= 0xffff;

        if (wxr_format == WXR_FORMAT_MONO)
        {
            /* for a 1bpp bitmap we always need a 1 in the tile */
            XRenderColor col;
            col.red = col.green = col.blue = 0;
            col.alpha = 0xffff;
            pXRenderFillRectangle(gdi_display, PictOpSrc, tile->pict, &col, 0, 0, 1, 1);
        }
    }

    if (memcmp( color, &tile->current_color, sizeof(*color) ) && wxr_format != WXR_FORMAT_MONO)
    {
        pXRenderFillRectangle(gdi_display, PictOpSrc, tile->pict, color, 0, 0, 1, 1);
        tile->current_color = *color;
    }
    return tile->pict;
}

/*************************************************************
 *                 get_mask_pict
 *
 * Returns an appropriate Picture for masking with the specified alpha.
 * Call and use result within the xrender_mutex
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

        pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 32 );
        pa.repeat = RepeatNormal;
        pict = pXRenderCreatePicture( gdi_display, pixmap,
                                      pict_formats[WXR_FORMAT_A8R8G8B8], CPRepeat, &pa );
        current_alpha = -1;
    }

    if (alpha != current_alpha)
    {
        XRenderColor col;
        col.red = col.green = col.blue = 0;
        col.alpha = current_alpha = alpha;
        pXRenderFillRectangle( gdi_display, PictOpSrc, pict, &col, 0, 0, 1, 1 );
    }
    return pict;
}

/***********************************************************************
 *           xrenderdrv_ExtTextOut
 */
static BOOL xrenderdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags,
                                   const RECT *lprect, LPCWSTR wstr, UINT count, const INT *lpDx )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    gsCacheEntry *entry;
    gsCacheEntryFormat *formatEntry;
    unsigned int idx;
    DWORD text_color;
    Picture pict, tile_pict = 0;
    XGlyphElt16 *elts;
    POINT offset, desired, current;
    int render_op = PictOpOver;
    XRenderColor col;
    RECT rect, bounds;
    enum glyph_type type = (flags & ETO_GLYPH_INDEX) ? GLYPH_INDEX : GLYPH_WCHAR;

    NtGdiGetDCDword( physdev->dev.hdc, NtGdiGetTextColor, &text_color );
    get_xrender_color( physdev, text_color, &col );
    pict = get_xrender_picture( physdev, 0, (flags & ETO_CLIPPED) ? lprect : NULL );

    if(flags & ETO_OPAQUE)
    {
        XRenderColor bg;

        if (physdev->format == WXR_FORMAT_MONO)
            /* use the inverse of the text color */
            bg.red = bg.green = bg.blue = bg.alpha = ~col.alpha;
        else
        {
            DWORD bg_color;
            NtGdiGetDCDword( physdev->dev.hdc, NtGdiGetBkColor, &bg_color );
            get_xrender_color( physdev, bg_color, &bg );
        }

        set_xrender_transformation( pict, 1, 1, 0, 0 );
        pXRenderFillRectangle( gdi_display, PictOpSrc, pict, &bg,
                               physdev->x11dev->dc_rect.left + lprect->left,
                               physdev->x11dev->dc_rect.top + lprect->top,
                               lprect->right - lprect->left,
                               lprect->bottom - lprect->top );
        add_device_bounds( physdev->x11dev, lprect );
    }

    if(count == 0) return TRUE;

    pthread_mutex_lock( &xrender_mutex );

    entry = glyphsetCache + physdev->cache_index;
    formatEntry = entry->format[type][aa_type_from_flags( physdev->aa_flags )];

    for(idx = 0; idx < count; idx++) {
        if( !formatEntry ) {
	    UploadGlyph(physdev, wstr[idx], type);
            /* re-evaluate format entry since aa_flags may have changed */
            formatEntry = entry->format[type][aa_type_from_flags( physdev->aa_flags )];
        } else if( wstr[idx] >= formatEntry->nrealized || formatEntry->realized[wstr[idx]] == FALSE) {
	    UploadGlyph(physdev, wstr[idx], type);
	}
    }
    if (!formatEntry)
    {
        WARN("could not upload requested glyphs\n");
        pthread_mutex_unlock( &xrender_mutex );
        return FALSE;
    }

    TRACE("Writing %s at %d,%d\n", debugstr_wn(wstr,count),
           physdev->x11dev->dc_rect.left + x, physdev->x11dev->dc_rect.top + y);

    elts = malloc( sizeof(XGlyphElt16) * count );

    /* There's a bug in XRenderCompositeText that ignores the xDst and yDst parameters.
       So we pass zeros to the function and move to our starting position using the first
       element of the elts array. */

    desired.x = physdev->x11dev->dc_rect.left + x;
    desired.y = physdev->x11dev->dc_rect.top + y;
    offset.x = offset.y = 0;
    current.x = current.y = 0;

    tile_pict = get_tile_pict(physdev->format, &col);

    /* FIXME the mapping of Text/BkColor onto 1 or 0 needs investigation.
     */
    if (physdev->format == WXR_FORMAT_MONO && col.red == 0 && col.green == 0 && col.blue == 0)
        render_op = PictOpOutReverse; /* This gives us 'black' text */

    reset_bounds( &bounds );
    for(idx = 0; idx < count; idx++)
    {
        elts[idx].glyphset = formatEntry->glyphset;
        elts[idx].chars = wstr + idx;
        elts[idx].nchars = 1;
        elts[idx].xOff = desired.x - current.x;
        elts[idx].yOff = desired.y - current.y;

        current.x += (elts[idx].xOff + formatEntry->gis[wstr[idx]].xOff);
        current.y += (elts[idx].yOff + formatEntry->gis[wstr[idx]].yOff);

        rect.left   = desired.x - physdev->x11dev->dc_rect.left - formatEntry->gis[wstr[idx]].x;
        rect.top    = desired.y - physdev->x11dev->dc_rect.top - formatEntry->gis[wstr[idx]].y;
        rect.right  = rect.left + formatEntry->gis[wstr[idx]].width;
        rect.bottom = rect.top  + formatEntry->gis[wstr[idx]].height;
        add_bounds_rect( &bounds, &rect );

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

    /* Make sure we don't have any transforms set from a previous call */
    set_xrender_transformation(pict, 1, 1, 0, 0);
    pXRenderCompositeText16(gdi_display, render_op,
                            tile_pict,
                            pict,
                            formatEntry->font_format,
                            0, 0, 0, 0, elts, count);
    free( elts );

    pthread_mutex_unlock( &xrender_mutex );
    add_device_bounds( physdev->x11dev, &bounds );
    return TRUE;
}

/* multiply the alpha channel of a picture */
static void multiply_alpha( Picture pict, XRenderPictFormat *format, int alpha,
                            int x, int y, int width, int height )
{
    XRenderPictureAttributes pa;
    Pixmap src_pixmap, mask_pixmap;
    Picture src_pict, mask_pict;
    XRenderColor color;

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
}

/* Helper function for (stretched) blitting using xrender */
static void xrender_blit( int op, Picture src_pict, Picture mask_pict, Picture dst_pict,
                          int x_src, int y_src, int width_src, int height_src,
                          int x_dst, int y_dst, int width_dst, int height_dst,
                          double xscale, double yscale )
{
    int x_offset, y_offset;

    if (width_src < 0)
    {
        x_src += width_src + 1;
    }
    if (height_src < 0)
    {
        y_src += height_src + 1;
    }
    if (width_dst < 0)
    {
        x_dst += width_dst + 1;
        width_dst = -width_dst;
    }
    if (height_dst < 0)
    {
        y_dst += height_dst + 1;
        height_dst = -height_dst;
    }

    /* When we need to scale we perform scaling and source_x / source_y translation using a transformation matrix.
     * This is needed because XRender is inaccurate in combination with scaled source coordinates passed to XRenderComposite.
     * In all other cases we do use XRenderComposite for translation as it is faster than using a transformation matrix. */
    if(xscale != 1.0 || yscale != 1.0)
    {
        /* In case of mirroring we need a source x- and y-offset because without the pixels will be
         * in the wrong quadrant of the x-y plane.
         */
        x_offset = (xscale < 0) ? -width_dst : 0;
        y_offset = (yscale < 0) ? -height_dst : 0;
        set_xrender_transformation(src_pict, xscale, yscale, x_src, y_src);
    }
    else
    {
        x_offset = x_src;
        y_offset = y_src;
        set_xrender_transformation(src_pict, 1, 1, 0, 0);
    }
    pXRenderSetPictureFilter( gdi_display, src_pict, FilterBilinear, 0, 0 );
    pXRenderComposite( gdi_display, op, src_pict, mask_pict, dst_pict,
                       x_offset, y_offset, 0, 0, x_dst, y_dst, width_dst, height_dst );
}

/* Helper function for (stretched) mono->color blitting using xrender */
static void xrender_mono_blit( Picture src_pict, Picture dst_pict,
                               enum wxr_format dst_format, XRenderColor *fg, XRenderColor *bg,
                               int x_src, int y_src, int width_src, int height_src,
                               int x_dst, int y_dst, int width_dst, int height_dst,
                               double xscale, double yscale )
{
    Picture tile_pict;
    int x_offset, y_offset;
    XRenderColor color;

    if (width_src < 0)
    {
        x_src += width_src + 1;
        width_src = -width_src;
    }
    if (height_src < 0)
    {
        y_src += height_src + 1;
        height_src = -height_src;
    }
    if (width_dst < 0)
    {
        x_dst += width_dst + 1;
        width_dst = -width_dst;
    }
    if (height_dst < 0)
    {
        y_dst += height_dst + 1;
        height_dst = -height_dst;
    }

    /* When doing a mono->color blit, the source data is used as mask, and the source picture
     * contains a 1x1 picture for tiling. The source data effectively acts as an alpha channel to
     * the tile data.
     */
    pthread_mutex_lock( &xrender_mutex );
    color = *bg;
    color.alpha = 0xffff;  /* tile pict needs 100% alpha */
    tile_pict = get_tile_pict( dst_format, &color );

    pXRenderFillRectangle( gdi_display, PictOpSrc, dst_pict, fg, x_dst, y_dst, width_dst, height_dst );

    if (xscale != 1.0 || yscale != 1.0)
    {
        /* In case of mirroring we need a source x- and y-offset because without the pixels will be
         * in the wrong quadrant of the x-y plane.
         */
        x_offset = (xscale < 0) ? -width_dst : 0;
        y_offset = (yscale < 0) ? -height_dst : 0;
        set_xrender_transformation(src_pict, xscale, yscale, x_src, y_src);
    }
    else
    {
        x_offset = x_src;
        y_offset = y_src;
        set_xrender_transformation(src_pict, 1, 1, 0, 0);
    }
    pXRenderComposite(gdi_display, PictOpOver, tile_pict, src_pict, dst_pict,
                      0, 0, x_offset, y_offset, x_dst, y_dst, width_dst, height_dst );
    pthread_mutex_unlock( &xrender_mutex );

    /* force the alpha channel for background pixels, it has been set to 100% by the tile */
    if (bg->alpha != 0xffff && (dst_format == WXR_FORMAT_A8R8G8B8 || dst_format == WXR_FORMAT_B8G8R8A8))
        multiply_alpha( dst_pict, pict_formats[dst_format], bg->alpha,
                        x_dst, y_dst, width_dst, height_dst );
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
    GC gc;
    XImage *image;

    image = XCreateImage( gdi_display, default_visual.visual, depth, ZPixmap, 0, NULL,
                          info->bmiHeader.biWidth, height, 32, 0 );
    if (!image) return ERROR_OUTOFMEMORY;

    ret = copy_image_bits( info, (format == WXR_FORMAT_R8G8B8), image, bits, &dst_bits, src, NULL, ~0u );
    if (ret) return ret;

    image->data = dst_bits.ptr;

    *use_repeat = (width == 1 && height == 1);
    pa.repeat = *use_repeat ? RepeatNormal : RepeatNone;

    *pixmap = XCreatePixmap( gdi_display, root_window, width, height, depth );
    gc = XCreateGC( gdi_display, *pixmap, 0, NULL );
    XPutImage( gdi_display, *pixmap, gc, image, src->visrect.left, 0, 0, 0, width, height );
    *pict = pXRenderCreatePicture( gdi_display, *pixmap, pict_formats[format], CPRepeat, &pa );
    XFreeGC( gdi_display, gc );

    /* make coordinates relative to the pixmap */
    src->x -= src->visrect.left;
    src->y -= src->visrect.top;
    OffsetRect( &src->visrect, -src->visrect.left, -src->visrect.top );

    image->data = NULL;
    XDestroyImage( image );
    if (dst_bits.free) dst_bits.free( &dst_bits );
    return ret;
}

static void xrender_stretch_blit( struct xrender_physdev *physdev_src, struct xrender_physdev *physdev_dst,
                                  Drawable drawable, const struct bitblt_coords *src,
                                  const struct bitblt_coords *dst )
{
    int x_dst, y_dst;
    Picture src_pict = 0, dst_pict, mask_pict = 0;
    double xscale = src->width / (double)dst->width;
    double yscale = src->height / (double)dst->height;

    if (drawable)  /* using an intermediate pixmap */
    {
        x_dst = dst->x;
        y_dst = dst->y;
        dst_pict = pXRenderCreatePicture( gdi_display, drawable, physdev_dst->pict_format, 0, NULL );
    }
    else
    {
        x_dst = physdev_dst->x11dev->dc_rect.left + dst->x;
        y_dst = physdev_dst->x11dev->dc_rect.top + dst->y;
        dst_pict = get_xrender_picture( physdev_dst, 0, &dst->visrect );
    }

    src_pict = get_xrender_picture_source( physdev_src, FALSE );

    /* mono -> color */
    if (physdev_src->format == WXR_FORMAT_MONO && physdev_dst->format != WXR_FORMAT_MONO)
    {
        DWORD text_color, bg_color;
        XRenderColor fg, bg;

        NtGdiGetDCDword( physdev_dst->dev.hdc, NtGdiGetTextColor, &text_color );
        NtGdiGetDCDword( physdev_dst->dev.hdc, NtGdiGetBkColor, &bg_color );
        get_xrender_color( physdev_dst, text_color, &fg );
        get_xrender_color( physdev_dst, bg_color, &bg );
        fg.alpha = bg.alpha = 0;

        xrender_mono_blit( src_pict, dst_pict, physdev_dst->format, &fg, &bg,
                           physdev_src->x11dev->dc_rect.left + src->x,
                           physdev_src->x11dev->dc_rect.top + src->y,
                           src->width, src->height, x_dst, y_dst, dst->width, dst->height, xscale, yscale );
    }
    else /* color -> color (can be at different depths) or mono -> mono */
    {
        if (physdev_dst->pict_format->depth == 32 && physdev_src->pict_format->depth < 32)
            mask_pict = get_no_alpha_mask();

        xrender_blit( PictOpSrc, src_pict, mask_pict, dst_pict,
                      physdev_src->x11dev->dc_rect.left + src->x,
                      physdev_src->x11dev->dc_rect.top + src->y,
                      src->width, src->height, x_dst, y_dst, dst->width, dst->height, xscale, yscale );
    }

    if (drawable) pXRenderFreePicture( gdi_display, dst_pict );
}


static void xrender_put_image( Pixmap src_pixmap, Picture src_pict, Picture mask_pict, HRGN clip,
                               XRenderPictFormat *dst_format, struct xrender_physdev *physdev,
                               Drawable drawable, struct bitblt_coords *src,
                               struct bitblt_coords *dst, BOOL use_repeat )
{
    int x_dst, y_dst;
    Picture dst_pict;
    double xscale, yscale;

    if (drawable)  /* using an intermediate pixmap */
    {
        RGNDATA *clip_data = NULL;

        if (clip) clip_data = X11DRV_GetRegionData( clip, 0 );
        x_dst = dst->x;
        y_dst = dst->y;
        dst_pict = pXRenderCreatePicture( gdi_display, drawable, dst_format, 0, NULL );
        if (clip_data)
            pXRenderSetPictureClipRectangles( gdi_display, dst_pict, 0, 0,
                                              (XRectangle *)clip_data->Buffer, clip_data->rdh.nCount );
        free( clip_data );
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

    xrender_blit( PictOpSrc, src_pict, mask_pict, dst_pict, src->x, src->y, src->width, src->height,
                  x_dst, y_dst, dst->width, dst->height, xscale, yscale );

    if (drawable) pXRenderFreePicture( gdi_display, dst_pict );
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

    /* XRender is of no use for color -> mono */
    if (physdev_dst->format == WXR_FORMAT_MONO && physdev_src->format != WXR_FORMAT_MONO)
        goto x11drv_fallback;

    /* if not stretching, we only need to handle format conversion */
    if (!stretch && physdev_dst->format == physdev_src->format) goto x11drv_fallback;

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

        tmpGC = XCreateGC( gdi_display, physdev_dst->x11dev->drawable, 0, NULL );
        XSetSubwindowMode( gdi_display, tmpGC, IncludeInferiors );
        XSetGraphicsExposures( gdi_display, tmpGC, False );
        tmp_pixmap = XCreatePixmap( gdi_display, root_window, tmp.visrect.right - tmp.visrect.left,
                                    tmp.visrect.bottom - tmp.visrect.top, physdev_dst->pict_format->depth );

        xrender_stretch_blit( physdev_src, physdev_dst, tmp_pixmap, src, &tmp );
        execute_rop( physdev_dst->x11dev, tmp_pixmap, tmpGC, &dst->visrect, rop );

        XFreePixmap( gdi_display, tmp_pixmap );
        XFreeGC( gdi_display, tmpGC );
    }
    else xrender_stretch_blit( physdev_src, physdev_dst, 0, src, dst );

    add_device_bounds( physdev_dst->x11dev, &dst->visrect );
    return TRUE;

x11drv_fallback:
    return X11DRV_StretchBlt( &physdev_dst->x11dev->dev, dst, &physdev_src->x11dev->dev, src, rop );
}


/***********************************************************************
 *           xrenderdrv_PutImage
 */
static DWORD xrenderdrv_PutImage( PHYSDEV dev, HRGN clip, BITMAPINFO *info,
                                  const struct gdi_image_bits *bits, struct bitblt_coords *src,
                                  struct bitblt_coords *dst, DWORD rop )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    DWORD ret;
    Pixmap tmp_pixmap;
    GC gc;
    enum wxr_format src_format, dst_format;
    XRenderPictFormat *pict_format;
    Pixmap src_pixmap;
    Picture src_pict, mask_pict = 0;
    BOOL use_repeat;

    dst_format = physdev->format;
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

        if (rop != SRCCOPY)
        {
            BOOL restore_region = add_extra_clipping_region( physdev->x11dev, clip );

            /* make coordinates relative to tmp pixmap */
            tmp = *dst;
            tmp.x -= tmp.visrect.left;
            tmp.y -= tmp.visrect.top;
            OffsetRect( &tmp.visrect, -tmp.visrect.left, -tmp.visrect.top );

            gc = XCreateGC( gdi_display, physdev->x11dev->drawable, 0, NULL );
            XSetSubwindowMode( gdi_display, gc, IncludeInferiors );
            XSetGraphicsExposures( gdi_display, gc, False );
            tmp_pixmap = XCreatePixmap( gdi_display, root_window,
                                        tmp.visrect.right - tmp.visrect.left,
                                        tmp.visrect.bottom - tmp.visrect.top,
                                        physdev->pict_format->depth );

            xrender_put_image( src_pixmap, src_pict, mask_pict, NULL, physdev->pict_format,
                               NULL, tmp_pixmap, src, &tmp, use_repeat );
            execute_rop( physdev->x11dev, tmp_pixmap, gc, &dst->visrect, rop );

            XFreePixmap( gdi_display, tmp_pixmap );
            XFreeGC( gdi_display, gc );
            if (restore_region) restore_clipping_region( physdev->x11dev );
        }
        else xrender_put_image( src_pixmap, src_pict, mask_pict, clip,
                                physdev->pict_format, physdev, 0, src, dst, use_repeat );

        add_device_bounds( physdev->x11dev, &dst->visrect );

        pXRenderFreePicture( gdi_display, src_pict );
        XFreePixmap( gdi_display, src_pixmap );
    }
    return ret;

update_format:
    if (info->bmiHeader.biHeight > 0) info->bmiHeader.biHeight = -info->bmiHeader.biHeight;
    set_color_info( pict_formats[dst_format], info );
    return ERROR_BAD_FORMAT;

x11drv_fallback:
    dev = GET_NEXT_PHYSDEV( dev, pPutImage );
    return dev->funcs->pPutImage( dev, clip, info, bits, src, dst, rop );
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

    format = get_xrender_format_from_bitmapinfo( info );
    if (!(func.AlphaFormat & AC_SRC_ALPHA))
        format = get_format_without_alpha( format );
    else if (format != WXR_FORMAT_A8R8G8B8 || info->bmiHeader.biCompression != BI_RGB)
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

        if (!use_repeat)
        {
            xscale = src->width / (double)dst->width;
            yscale = src->height / (double)dst->height;
        }
        else xscale = yscale = 1;  /* no scaling needed with a repeating source */

        dst_pict = get_xrender_picture( physdev, 0, &dst->visrect );

        pthread_mutex_lock( &xrender_mutex );
        mask_pict = get_mask_pict( func.SourceConstantAlpha * 257 );

        xrender_blit( PictOpOver, src_pict, mask_pict, dst_pict,
                      src->x, src->y, src->width, src->height,
                      physdev->x11dev->dc_rect.left + dst->x,
                      physdev->x11dev->dc_rect.top + dst->y,
                      dst->width, dst->height, xscale, yscale );

        pXRenderFreePicture( gdi_display, src_pict );
        XFreePixmap( gdi_display, src_pixmap );

        pthread_mutex_unlock( &xrender_mutex );
        add_device_bounds( physdev->x11dev, &dst->visrect );
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

    if (src_dev->funcs != dst_dev->funcs)
    {
        dst_dev = GET_NEXT_PHYSDEV( dst_dev, pAlphaBlend );
        return dst_dev->funcs->pAlphaBlend( dst_dev, dst, src_dev, src, blendfn );
    }

    if ((blendfn.AlphaFormat & AC_SRC_ALPHA) && physdev_src->format != WXR_FORMAT_A8R8G8B8)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    dst_pict = get_xrender_picture( physdev_dst, 0, &dst->visrect );

    xscale = src->width / (double)dst->width;
    yscale = src->height / (double)dst->height;

    src_pict = get_xrender_picture_source( physdev_src, FALSE );

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

        tmp_pixmap = XCreatePixmap( gdi_display, root_window, width, height,
                                    physdev_dst->pict_format->depth );
        tmp_pict = pXRenderCreatePicture( gdi_display, tmp_pixmap, physdev_dst->pict_format, 0, NULL );

        xrender_mono_blit( src_pict, tmp_pict, physdev_dst->format, &fg, &bg,
                           src->visrect.left, src->visrect.top, width, height, 0, 0, width, height, 1, 1 );
    }
    else if (!(blendfn.AlphaFormat & AC_SRC_ALPHA) && physdev_src->pict_format)
    {
        /* we need a source picture with no alpha */
        enum wxr_format format = get_format_without_alpha( physdev_src->format );
        if (format != physdev_src->format)
        {
            pa.subwindow_mode = IncludeInferiors;
            tmp_pict = pXRenderCreatePicture( gdi_display, physdev_src->x11dev->drawable,
                                              pict_formats[format], CPSubwindowMode, &pa );
        }
    }

    if (tmp_pict) src_pict = tmp_pict;

    pthread_mutex_lock( &xrender_mutex );
    mask_pict = get_mask_pict( blendfn.SourceConstantAlpha * 257 );

    xrender_blit( PictOpOver, src_pict, mask_pict, dst_pict,
                  physdev_src->x11dev->dc_rect.left + src->x,
                  physdev_src->x11dev->dc_rect.top + src->y,
                  src->width, src->height,
                  physdev_dst->x11dev->dc_rect.left + dst->x,
                  physdev_dst->x11dev->dc_rect.top + dst->y,
                  dst->width, dst->height, xscale, yscale );

    if (tmp_pict) pXRenderFreePicture( gdi_display, tmp_pict );
    if (tmp_pixmap) XFreePixmap( gdi_display, tmp_pixmap );

    pthread_mutex_unlock( &xrender_mutex );
    add_device_bounds( physdev_dst->x11dev, &dst->visrect );
    return TRUE;
}

/***********************************************************************
 *           xrenderdrv_GradientFill
 */
static BOOL xrenderdrv_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                                     void *grad_array, ULONG ngrad, ULONG mode )
{
#ifdef HAVE_XRENDERCREATELINEARGRADIENT
    static const XFixed stops[2] = { 0, 1 << 16 };
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    XLinearGradient gradient;
    XRenderColor colors[2];
    Picture src_pict, dst_pict;
    unsigned int i;
    const GRADIENT_RECT *rect = grad_array;
    RECT rc;
    POINT pt[2];

    if (!pXRenderCreateLinearGradient) goto fallback;

    /* <= 16-bpp uses dithering */
    if (!physdev->pict_format || physdev->pict_format->depth <= 16) goto fallback;

    switch (mode)
    {
    case GRADIENT_FILL_RECT_H:
    case GRADIENT_FILL_RECT_V:
        for (i = 0; i < ngrad; i++, rect++)
        {
            const TRIVERTEX *v1 = vert_array + rect->UpperLeft;
            const TRIVERTEX *v2 = vert_array + rect->LowerRight;

            colors[0].red   = v1->Red * 257 / 256;
            colors[0].green = v1->Green * 257 / 256;
            colors[0].blue  = v1->Blue * 257 / 256;
            colors[1].red   = v2->Red * 257 / 256;
            colors[1].green = v2->Green * 257 / 256;
            colors[1].blue  = v2->Blue * 257 / 256;
            /* always ignore alpha since otherwise xrender will want to pre-multiply the colors */
            colors[0].alpha = colors[1].alpha = 65535;

            pt[0].x = v1->x;
            pt[0].y = v1->y;
            pt[1].x = v2->x;
            pt[1].y = v2->y;
            lp_to_dp( dev->hdc, pt, 2 );
            if (mode == GRADIENT_FILL_RECT_H)
            {
                gradient.p1.y = gradient.p2.y = 0;
                if (pt[1].x > pt[0].x)
                {
                    gradient.p1.x = 0;
                    gradient.p2.x = (pt[1].x - pt[0].x) << 16;
                }
                else
                {
                    gradient.p1.x = (pt[0].x - pt[1].x) << 16;
                    gradient.p2.x = 0;
                }
            }
            else
            {
                gradient.p1.x = gradient.p2.x = 0;
                if (pt[1].y > pt[0].y)
                {
                    gradient.p1.y = 0;
                    gradient.p2.y = (pt[1].y - pt[0].y) << 16;
                }
                else
                {
                    gradient.p1.y = (pt[0].y - pt[1].y) << 16;
                    gradient.p2.y = 0;
                }
            }

            rc.left   = min( pt[0].x, pt[1].x );
            rc.top    = min( pt[0].y, pt[1].y );
            rc.right  = max( pt[0].x, pt[1].x );
            rc.bottom = max( pt[0].y, pt[1].y );

            TRACE( "%u gradient %s colors %04x,%04x,%04x,%04x -> %04x,%04x,%04x,%04x\n",
                   mode, wine_dbgstr_rect( &rc ),
                   colors[0].red, colors[0].green, colors[0].blue, colors[0].alpha,
                   colors[1].red, colors[1].green, colors[1].blue, colors[1].alpha );

            dst_pict = get_xrender_picture( physdev, 0, NULL );

            src_pict = pXRenderCreateLinearGradient( gdi_display, &gradient, stops, colors, 2 );
            xrender_blit( PictOpSrc, src_pict, 0, dst_pict,
                          0, 0, rc.right - rc.left, rc.bottom - rc.top,
                          physdev->x11dev->dc_rect.left + rc.left,
                          physdev->x11dev->dc_rect.top + rc.top,
                          rc.right - rc.left, rc.bottom - rc.top, 1, 1 );
            pXRenderFreePicture( gdi_display, src_pict );
            add_device_bounds( physdev->x11dev, &rc );
        }
        return TRUE;
    }

fallback:
#endif
    dev = GET_NEXT_PHYSDEV( dev, pGradientFill );
    return dev->funcs->pGradientFill( dev, vert_array, nvert, grad_array, ngrad, mode );
}

/***********************************************************************
 *           xrenderdrv_SelectBrush
 */
static HBRUSH xrenderdrv_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern )
{
    struct xrender_physdev *physdev = get_xrender_dev( dev );
    Pixmap pixmap;
    XVisualInfo vis = default_visual;
    XRenderPictFormat *format = physdev->pict_format;

    if (!pattern) goto x11drv_fallback;
    if (pattern->info->bmiHeader.biBitCount == 1) goto x11drv_fallback;
    if (physdev->format == WXR_FORMAT_MONO) goto x11drv_fallback;

    vis.depth      = format->depth;
    vis.red_mask   = format->direct.redMask   << format->direct.red;
    vis.green_mask = format->direct.greenMask << format->direct.green;
    vis.blue_mask  = format->direct.blueMask  << format->direct.blue;

    pixmap = create_pixmap_from_image( physdev->dev.hdc, &vis, pattern->info,
                                       &pattern->bits, pattern->usage );
    if (!pixmap) return 0;

    if (physdev->x11dev->brush.pixmap) XFreePixmap( gdi_display, physdev->x11dev->brush.pixmap );
    physdev->x11dev->brush.pixmap = pixmap;
    physdev->x11dev->brush.fillStyle = FillTiled;
    physdev->x11dev->brush.pixel = 0;  /* ignored */
    physdev->x11dev->brush.style = BS_PATTERN;
    return hbrush;

x11drv_fallback:
    dev = GET_NEXT_PHYSDEV( dev, pSelectBrush );
    return dev->funcs->pSelectBrush( dev, hbrush, pattern );
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
    NULL,                               /* pChord */
    NULL,                               /* pCloseFigure */
    xrenderdrv_CreateCompatibleDC,      /* pCreateCompatibleDC */
    xrenderdrv_CreateDC,                /* pCreateDC */
    xrenderdrv_DeleteDC,                /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    NULL,                               /* pEndPath */
    NULL,                               /* pEnumFonts */
    xrenderdrv_ExtEscape,               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    xrenderdrv_ExtTextOut,              /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetCharWidthInfo */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    xrenderdrv_GradientFill,            /* pGradientFill */
    NULL,                               /* pInvertRgn */
    NULL,                               /* pLineTo */
    NULL,                               /* pMoveTo */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    NULL,                               /* pPie */
    NULL,                               /* pPolyBezier */
    NULL,                               /* pPolyBezierTo */
    NULL,                               /* pPolyDraw */
    NULL,                               /* pPolyPolygon */
    NULL,                               /* pPolyPolyline */
    NULL,                               /* pPolylineTo */
    xrenderdrv_PutImage,                /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    NULL,                               /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRoundRect */
    NULL,                               /* pSelectBitmap */
    xrenderdrv_SelectBrush,             /* pSelectBrush */
    xrenderdrv_SelectFont,              /* pSelectFont */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBoundsRect */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBitsToDevice */
    xrenderdrv_SetDeviceClipping,       /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    xrenderdrv_StretchBlt,              /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    GDI_PRIORITY_GRAPHICS_DRV + 10      /* priority */
};

#else /* SONAME_LIBXRENDER */

const struct gdi_dc_funcs *X11DRV_XRender_Init(void)
{
    TRACE("XRender support not compiled in.\n");
    return NULL;
}

#endif /* SONAME_LIBXRENDER */
