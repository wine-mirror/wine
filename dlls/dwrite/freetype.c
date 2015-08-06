/*
 *    FreeType integration
 *
 * Copyright 2014-2015 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#include FT_CACHE_H
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#endif /* HAVE_FT2BUILD_H */

#include "windef.h"
#include "dwrite_2.h"
#include "wine/library.h"
#include "wine/debug.h"

#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#ifdef HAVE_FREETYPE

static CRITICAL_SECTION freetype_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &freetype_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": freetype_cs") }
};
static CRITICAL_SECTION freetype_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static void *ft_handle = NULL;
static FT_Library library = 0;
static FTC_Manager cache_manager = 0;
static FTC_CMapCache cmap_cache = 0;
static FTC_ImageCache image_cache = 0;
typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(FT_Done_FreeType);
MAKE_FUNCPTR(FT_Get_First_Char);
MAKE_FUNCPTR(FT_Get_Kerning);
MAKE_FUNCPTR(FT_Get_Sfnt_Table);
MAKE_FUNCPTR(FT_Glyph_Get_CBox);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Library_Version);
MAKE_FUNCPTR(FT_Load_Glyph);
MAKE_FUNCPTR(FT_New_Memory_Face);
MAKE_FUNCPTR(FT_Outline_Copy);
MAKE_FUNCPTR(FT_Outline_Done);
MAKE_FUNCPTR(FT_Outline_Get_Bitmap);
MAKE_FUNCPTR(FT_Outline_New);
MAKE_FUNCPTR(FT_Outline_Transform);
MAKE_FUNCPTR(FT_Outline_Translate);
MAKE_FUNCPTR(FTC_CMapCache_Lookup);
MAKE_FUNCPTR(FTC_CMapCache_New);
MAKE_FUNCPTR(FTC_ImageCache_Lookup);
MAKE_FUNCPTR(FTC_ImageCache_New);
MAKE_FUNCPTR(FTC_Manager_New);
MAKE_FUNCPTR(FTC_Manager_Done);
MAKE_FUNCPTR(FTC_Manager_LookupFace);
MAKE_FUNCPTR(FTC_Manager_LookupSize);
MAKE_FUNCPTR(FTC_Manager_RemoveFaceID);
#undef MAKE_FUNCPTR

static FT_Error face_requester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *face)
{
    IDWriteFontFace *fontface = (IDWriteFontFace*)face_id;
    IDWriteFontFileStream *stream;
    IDWriteFontFile *file;
    const void *data_ptr;
    UINT32 index, count;
    FT_Error fterror;
    UINT64 data_size;
    void *context;
    HRESULT hr;

    *face = NULL;

    count = 1;
    hr = IDWriteFontFace_GetFiles(fontface, &count, &file);
    if (FAILED(hr))
        return FT_Err_Ok;

    hr = get_filestream_from_file(file, &stream);
    IDWriteFontFile_Release(file);
    if (FAILED(hr))
        return FT_Err_Ok;

    hr = IDWriteFontFileStream_GetFileSize(stream, &data_size);
    if (FAILED(hr)) {
        fterror = FT_Err_Invalid_Stream_Read;
        goto fail;
    }

    hr = IDWriteFontFileStream_ReadFileFragment(stream, &data_ptr, 0, data_size, &context);
    if (FAILED(hr)) {
        fterror = FT_Err_Invalid_Stream_Read;
        goto fail;
    }

    index = IDWriteFontFace_GetIndex(fontface);
    fterror = pFT_New_Memory_Face(library, data_ptr, data_size, index, face);
    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

fail:
    IDWriteFontFileStream_Release(stream);

    return fterror;
}

BOOL init_freetype(void)
{
    FT_Version_t FT_Version;

    ft_handle = wine_dlopen(SONAME_LIBFREETYPE, RTLD_NOW, NULL, 0);
    if (!ft_handle) {
        WINE_MESSAGE("Wine cannot find the FreeType font library.\n");
	return FALSE;
    }

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(ft_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
    LOAD_FUNCPTR(FT_Done_FreeType)
    LOAD_FUNCPTR(FT_Get_First_Char)
    LOAD_FUNCPTR(FT_Get_Kerning)
    LOAD_FUNCPTR(FT_Get_Sfnt_Table)
    LOAD_FUNCPTR(FT_Glyph_Get_CBox)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Library_Version)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_New_Memory_Face)
    LOAD_FUNCPTR(FT_Outline_Copy)
    LOAD_FUNCPTR(FT_Outline_Done)
    LOAD_FUNCPTR(FT_Outline_Get_Bitmap)
    LOAD_FUNCPTR(FT_Outline_New)
    LOAD_FUNCPTR(FT_Outline_Transform)
    LOAD_FUNCPTR(FT_Outline_Translate)
    LOAD_FUNCPTR(FTC_CMapCache_Lookup)
    LOAD_FUNCPTR(FTC_CMapCache_New)
    LOAD_FUNCPTR(FTC_ImageCache_Lookup)
    LOAD_FUNCPTR(FTC_ImageCache_New)
    LOAD_FUNCPTR(FTC_Manager_New)
    LOAD_FUNCPTR(FTC_Manager_Done)
    LOAD_FUNCPTR(FTC_Manager_LookupFace)
    LOAD_FUNCPTR(FTC_Manager_LookupSize)
    LOAD_FUNCPTR(FTC_Manager_RemoveFaceID)
#undef LOAD_FUNCPTR

    if (pFT_Init_FreeType(&library) != 0) {
        ERR("Can't init FreeType library\n");
	wine_dlclose(ft_handle, NULL, 0);
        ft_handle = NULL;
	return FALSE;
    }
    pFT_Library_Version(library, &FT_Version.major, &FT_Version.minor, &FT_Version.patch);

    /* init cache manager */
    if (pFTC_Manager_New(library, 0, 0, 0, &face_requester, NULL, &cache_manager) != 0 ||
        pFTC_CMapCache_New(cache_manager, &cmap_cache) != 0 ||
        pFTC_ImageCache_New(cache_manager, &image_cache) != 0) {

        ERR("Failed to init FreeType cache\n");
        pFTC_Manager_Done(cache_manager);
        pFT_Done_FreeType(library);
        wine_dlclose(ft_handle, NULL, 0);
        ft_handle = NULL;
        return FALSE;
    }

    TRACE("FreeType version is %d.%d.%d\n", FT_Version.major, FT_Version.minor, FT_Version.patch);
    return TRUE;

sym_not_found:
    WINE_MESSAGE("Wine cannot find certain functions that it needs from FreeType library.\n");
    wine_dlclose(ft_handle, NULL, 0);
    ft_handle = NULL;
    return FALSE;
}

void release_freetype(void)
{
    pFTC_Manager_Done(cache_manager);
    pFT_Done_FreeType(library);
}

void freetype_notify_cacheremove(IDWriteFontFace2 *fontface)
{
    EnterCriticalSection(&freetype_cs);
    pFTC_Manager_RemoveFaceID(cache_manager, fontface);
    LeaveCriticalSection(&freetype_cs);
}

HRESULT freetype_get_design_glyph_metrics(IDWriteFontFace2 *fontface, UINT16 unitsperEm, UINT16 glyph, DWRITE_GLYPH_METRICS *ret)
{
    FTC_ScalerRec scaler;
    FT_Size size;

    scaler.face_id = fontface;
    scaler.width  = unitsperEm;
    scaler.height = unitsperEm;
    scaler.pixel = 1;
    scaler.x_res = 0;
    scaler.y_res = 0;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_Manager_LookupSize(cache_manager, &scaler, &size) == 0) {
         if (pFT_Load_Glyph(size->face, glyph, FT_LOAD_NO_SCALE) == 0) {
             FT_Glyph_Metrics *metrics = &size->face->glyph->metrics;

             ret->leftSideBearing = metrics->horiBearingX;
             ret->advanceWidth = metrics->horiAdvance;
             ret->rightSideBearing = metrics->horiAdvance - metrics->horiBearingX - metrics->width;
             ret->topSideBearing = metrics->vertBearingY;
             ret->advanceHeight = metrics->vertAdvance;
             ret->bottomSideBearing = metrics->vertAdvance - metrics->vertBearingY - metrics->height;
             ret->verticalOriginY = metrics->height + metrics->vertBearingY;
         }
    }
    LeaveCriticalSection(&freetype_cs);

    return S_OK;
}

BOOL freetype_is_monospaced(IDWriteFontFace2 *fontface)
{
    BOOL is_monospaced = FALSE;
    FT_Face face;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_Manager_LookupFace(cache_manager, fontface, &face) == 0)
        is_monospaced = FT_IS_FIXED_WIDTH(face);
    LeaveCriticalSection(&freetype_cs);

    return is_monospaced;
}

static inline void ft_vector_to_d2d_point(const FT_Vector *v, D2D1_POINT_2F *p)
{
    p->x = v->x / 64.0;
    p->y = v->y / 64.0;
}

/* Convert the quadratic Beziers to cubic Beziers. */
static void get_cubic_glyph_outline(const FT_Outline *outline, short point, short first_pt,
    short contour, FT_Vector *cubic_control)
{
    /*
       The parametric eqn for a cubic Bezier is, from PLRM:
       r(t) = at^3 + bt^2 + ct + r0
       with the control points:
       r1 = r0 + c/3
       r2 = r1 + (c + b)/3
       r3 = r0 + c + b + a

       A quadratic Bezier has the form:
       p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

       So equating powers of t leads to:
       r1 = 2/3 p1 + 1/3 p0
       r2 = 2/3 p1 + 1/3 p2
       and of course r0 = p0, r3 = p2
    */

    /* FIXME: Possible optimization in endpoint calculation
       if there are two consecutive curves */
    cubic_control[0] = outline->points[point-1];
    if (!(outline->tags[point-1] & FT_Curve_Tag_On)) {
        cubic_control[0].x += outline->points[point].x + 1;
        cubic_control[0].y += outline->points[point].y + 1;
        cubic_control[0].x >>= 1;
        cubic_control[0].y >>= 1;
    }
    if (point+1 > outline->contours[contour])
        cubic_control[3] = outline->points[first_pt];
    else {
        cubic_control[3] = outline->points[point+1];
        if (!(outline->tags[point+1] & FT_Curve_Tag_On)) {
            cubic_control[3].x += outline->points[point].x + 1;
            cubic_control[3].y += outline->points[point].y + 1;
            cubic_control[3].x >>= 1;
            cubic_control[3].y >>= 1;
        }
    }

    /* r1 = 1/3 p0 + 2/3 p1
       r2 = 1/3 p2 + 2/3 p1 */
    cubic_control[1].x = (2 * outline->points[point].x + 1) / 3;
    cubic_control[1].y = (2 * outline->points[point].y + 1) / 3;
    cubic_control[2] = cubic_control[1];
    cubic_control[1].x += (cubic_control[0].x + 1) / 3;
    cubic_control[1].y += (cubic_control[0].y + 1) / 3;
    cubic_control[2].x += (cubic_control[3].x + 1) / 3;
    cubic_control[2].y += (cubic_control[3].y + 1) / 3;
}

static short get_outline_data(const FT_Outline *outline, struct glyph_outline *ret)
{
    short contour, point = 0, first_pt, count;

    for (contour = 0, count = 0; contour < outline->n_contours; contour++) {
        first_pt = point;
        if (ret) {
            ft_vector_to_d2d_point(&outline->points[point], &ret->points[count]);
            ret->tags[count] = OUTLINE_POINT_START;
            if (count)
                ret->tags[count-1] |= OUTLINE_POINT_END;
        }

        point++;
        count++;

        while (point <= outline->contours[contour]) {
            do {
                if (outline->tags[point] & FT_Curve_Tag_On) {
                    if (ret) {
                        ft_vector_to_d2d_point(&outline->points[point], &ret->points[count]);
                        ret->tags[count] |= OUTLINE_POINT_LINE;
                    }

                    point++;
                    count++;
                }
                else {

                    if (ret) {
                        FT_Vector cubic_control[4];

                        get_cubic_glyph_outline(outline, point, first_pt, contour, cubic_control);
                        ft_vector_to_d2d_point(&cubic_control[1], &ret->points[count]);
                        ft_vector_to_d2d_point(&cubic_control[2], &ret->points[count+1]);
                        ft_vector_to_d2d_point(&cubic_control[3], &ret->points[count+2]);
                        ret->tags[count] = OUTLINE_POINT_BEZIER;
                        ret->tags[count+1] = OUTLINE_POINT_BEZIER;
                        ret->tags[count+2] = OUTLINE_POINT_BEZIER;
                    }

                    count += 3;
                    point++;
                }
            } while (point <= outline->contours[contour] &&
                    (outline->tags[point] & FT_Curve_Tag_On) ==
                    (outline->tags[point-1] & FT_Curve_Tag_On));

            if (point <= outline->contours[contour] &&
               outline->tags[point] & FT_Curve_Tag_On)
            {
                /* This is the closing pt of a bezier, but we've already
                   added it, so just inc point and carry on */
                point++;
            }
        }
    }

    if (ret)
        ret->tags[count-1] |= OUTLINE_POINT_END;

    return count;
}

HRESULT freetype_get_glyph_outline(IDWriteFontFace2 *fontface, FLOAT emSize, UINT16 index, USHORT simulations, struct glyph_outline **ret)
{
    FTC_ScalerRec scaler;
    HRESULT hr = S_OK;
    FT_Size size;

    scaler.face_id = fontface;
    scaler.width  = emSize;
    scaler.height = emSize;
    scaler.pixel = 1;
    scaler.x_res = 0;
    scaler.y_res = 0;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_Manager_LookupSize(cache_manager, &scaler, &size) == 0) {
         if (pFT_Load_Glyph(size->face, index, FT_LOAD_DEFAULT) == 0) {
             FT_Outline *outline = &size->face->glyph->outline;
             short count;
             FT_Matrix m;

             m.xx = 1 << 16;
             m.xy = simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE ? (1 << 16) / 3 : 0;
             m.yx = 0;
             m.yy = -(1 << 16); /* flip Y axis */

             pFT_Outline_Transform(outline, &m);

             count = get_outline_data(outline, NULL);
             hr = new_glyph_outline(count, ret);
             if (hr == S_OK) {
                 get_outline_data(outline, *ret);
                 (*ret)->advance = size->face->glyph->metrics.horiAdvance >> 6;
             }
         }
    }
    LeaveCriticalSection(&freetype_cs);

    return hr;
}

UINT16 freetype_get_glyphcount(IDWriteFontFace2 *fontface)
{
    UINT16 count = 0;
    FT_Face face;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_Manager_LookupFace(cache_manager, fontface, &face) == 0)
        count = face->num_glyphs;
    LeaveCriticalSection(&freetype_cs);

    return count;
}

UINT16 freetype_get_glyphindex(IDWriteFontFace2 *fontface, UINT32 codepoint)
{
    UINT16 glyph;

    EnterCriticalSection(&freetype_cs);
    glyph = pFTC_CMapCache_Lookup(cmap_cache, fontface, -1, codepoint);
    LeaveCriticalSection(&freetype_cs);

    return glyph;
}

BOOL freetype_has_kerning_pairs(IDWriteFontFace2 *fontface)
{
    BOOL has_kerning_pairs = FALSE;
    FT_Face face;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_Manager_LookupFace(cache_manager, fontface, &face) == 0)
        has_kerning_pairs = FT_HAS_KERNING(face);
    LeaveCriticalSection(&freetype_cs);

    return has_kerning_pairs;
}

INT32 freetype_get_kerning_pair_adjustment(IDWriteFontFace2 *fontface, UINT16 left, UINT16 right)
{
    INT32 adjustment = 0;
    FT_Face face;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_Manager_LookupFace(cache_manager, fontface, &face) == 0) {
        FT_Vector kern;
        if (FT_HAS_KERNING(face)) {
            pFT_Get_Kerning(face, left, right, FT_KERNING_UNSCALED, &kern);
            adjustment = kern.x;
        }
    }
    LeaveCriticalSection(&freetype_cs);

    return adjustment;
}

void freetype_get_glyph_bbox(IDWriteFontFace2 *fontface, FLOAT emSize, UINT16 index, BOOL nohint, RECT *ret)
{
    FTC_ImageTypeRec imagetype;
    FT_BBox bbox = { 0 };
    FT_Glyph glyph;

    imagetype.face_id = fontface;
    imagetype.width = 0;
    imagetype.height = emSize;
    imagetype.flags = FT_LOAD_DEFAULT;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_ImageCache_Lookup(image_cache, &imagetype, index, &glyph, NULL) == 0)
        pFT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &bbox);
    LeaveCriticalSection(&freetype_cs);

    /* flip Y axis */
    ret->left = bbox.xMin;
    ret->right = bbox.xMax;
    ret->top = -bbox.yMax;
    ret->bottom = -bbox.yMin;
}

void freetype_get_glyph_bitmap(IDWriteFontFace2 *fontface, FLOAT emSize, UINT16 index, const RECT *bbox, BYTE *buf)
{
    FTC_ImageTypeRec imagetype;
    FT_Glyph glyph;

    imagetype.face_id = fontface;
    imagetype.width = 0;
    imagetype.height = emSize;
    imagetype.flags = FT_LOAD_DEFAULT;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_ImageCache_Lookup(image_cache, &imagetype, index, &glyph, NULL) == 0) {
        int width = bbox->right - bbox->left;
        int pitch = ((width + 31) >> 5) << 2;
        int height = bbox->bottom - bbox->top;

        if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
            FT_OutlineGlyph outline = (FT_OutlineGlyph)glyph;
            const FT_Outline *src = &outline->outline;
            FT_Bitmap ft_bitmap;
            FT_Outline copy;

            ft_bitmap.width = width;
            ft_bitmap.rows = height;
            ft_bitmap.pitch = pitch;
            ft_bitmap.pixel_mode = FT_PIXEL_MODE_MONO;
            ft_bitmap.buffer = buf;

            /* Note: FreeType will only set 'black' bits for us. */
            memset(buf, 0, height*pitch);
            if (pFT_Outline_New(library, src->n_points, src->n_contours, &copy) == 0) {
                pFT_Outline_Copy(src, &copy);
                pFT_Outline_Translate(&copy, -bbox->left << 6, bbox->bottom << 6);
                pFT_Outline_Get_Bitmap(library, &copy, &ft_bitmap);
                pFT_Outline_Done(library, &copy);
            }
        }
        else if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
            FT_Bitmap *bitmap = &((FT_BitmapGlyph)glyph)->bitmap;
            BYTE *src = bitmap->buffer, *dst = buf;
            int w = min(pitch, (bitmap->width + 7) >> 3);
            int h = min(height, bitmap->rows);

            memset(buf, 0, height*pitch);

            while (h--) {
                memcpy(dst, src, w);
                src += bitmap->pitch;
                dst += pitch;
            }
        }
        else
            FIXME("format %d not handled\n", glyph->format);
    }
    LeaveCriticalSection(&freetype_cs);
}

INT freetype_get_charmap_index(IDWriteFontFace2 *fontface, BOOL *is_symbol)
{
    INT charmap_index = -1;
    FT_Face face;

    *is_symbol = FALSE;

    EnterCriticalSection(&freetype_cs);
    if (pFTC_Manager_LookupFace(cache_manager, fontface, &face) == 0) {
        TT_OS2 *os2 = pFT_Get_Sfnt_Table(face, ft_sfnt_os2);
        FT_Int i;

        if (os2) {
            FT_UInt dummy;
            if (os2->version == 0)
                *is_symbol = pFT_Get_First_Char(face, &dummy) >= 0x100;
            else
                *is_symbol = os2->ulCodePageRange1 & FS_SYMBOL;
        }

        for (i = 0; i < face->num_charmaps; i++)
            if (face->charmaps[i]->encoding == FT_ENCODING_MS_SYMBOL) {
                *is_symbol = TRUE;
                charmap_index = i;
                break;
            }
    }
    LeaveCriticalSection(&freetype_cs);

    return charmap_index;
}

#else /* HAVE_FREETYPE */

BOOL init_freetype(void)
{
    return FALSE;
}

void release_freetype(void)
{
}

void freetype_notify_cacheremove(IDWriteFontFace2 *fontface)
{
}

HRESULT freetype_get_design_glyph_metrics(IDWriteFontFace2 *fontface, UINT16 unitsperEm, UINT16 glyph, DWRITE_GLYPH_METRICS *ret)
{
    return E_NOTIMPL;
}

BOOL freetype_is_monospaced(IDWriteFontFace2 *fontface)
{
    return FALSE;
}

HRESULT freetype_get_glyph_outline(IDWriteFontFace2 *fontface, FLOAT emSize, UINT16 index, USHORT simulations, struct glyph_outline **ret)
{
    *ret = NULL;
    return E_NOTIMPL;
}

UINT16 freetype_get_glyphcount(IDWriteFontFace2 *fontface)
{
    return 0;
}

UINT16 freetype_get_glyphindex(IDWriteFontFace2 *fontface, UINT32 codepoint)
{
    return 0;
}

BOOL freetype_has_kerning_pairs(IDWriteFontFace2 *fontface)
{
    return FALSE;
}

INT32 freetype_get_kerning_pair_adjustment(IDWriteFontFace2 *fontface, UINT16 left, UINT16 right)
{
    return 0;
}

void freetype_get_glyph_bbox(IDWriteFontFace2 *fontface, FLOAT emSize, UINT16 index, BOOL nohint, RECT *ret)
{
    ret->left = ret->right = ret->top = ret->bottom = 0;
}

void freetype_get_glyph_bitmap(IDWriteFontFace2 *fontface, FLOAT emSize, UINT16 index, const RECT *bbox, BYTE *buf)
{
    UINT32 size = (bbox->right - bbox->left)*(bbox->bottom - bbox->top);
    memset(buf, 0, size);
}

INT freetype_get_charmap_index(IDWriteFontFace2 *fontface, BOOL *is_symbol)
{
    *is_symbol = FALSE;
    return -1;
}

#endif /* HAVE_FREETYPE */
