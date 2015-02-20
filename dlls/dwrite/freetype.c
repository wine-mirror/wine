/*
 *    FreeType integration
 *
 * Copyright 2014 Nikolay Sivov for CodeWeavers
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
typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(FT_Done_FreeType);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Library_Version);
MAKE_FUNCPTR(FT_Load_Glyph);
MAKE_FUNCPTR(FT_New_Memory_Face);
MAKE_FUNCPTR(FT_Outline_Transform);
MAKE_FUNCPTR(FTC_CMapCache_Lookup);
MAKE_FUNCPTR(FTC_CMapCache_New);
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
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Library_Version)
    LOAD_FUNCPTR(FT_Load_Glyph)
    LOAD_FUNCPTR(FT_New_Memory_Face)
    LOAD_FUNCPTR(FT_Outline_Transform)
    LOAD_FUNCPTR(FTC_CMapCache_Lookup)
    LOAD_FUNCPTR(FTC_CMapCache_New)
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
        pFTC_CMapCache_New(cache_manager, &cmap_cache) != 0) {

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

static HRESULT get_outline_data(const FT_Outline *outline, struct glyph_outline **ret)
{
    short i, j, contour = 0;
    D2D1_POINT_2F *points;
    UINT16 count = 0;
    UINT8 *tags;
    HRESULT hr;

    /* we need all curves in cubic format */
    for (i = 0; i < outline->n_points; i++) {
        /* control point */
        if (!(outline->tags[i] & FT_Curve_Tag_On)) {
            if (!(outline->tags[i] & FT_Curve_Tag_Cubic)) {
                count++;
            }
        }
        count++;
    }

    hr = new_glyph_outline(count, ret);
    if (FAILED(hr))
        return hr;

    points = (*ret)->points;
    tags = (*ret)->tags;

    ft_vector_to_d2d_point(outline->points, points);
    tags[0] = OUTLINE_POINT_START;

    for (i = 1, j = 1; i < outline->n_points; i++, j++) {
        /* mark start of new contour */
        if (tags[j-1] & OUTLINE_POINT_END)
            tags[j] = OUTLINE_POINT_START;
        else
            tags[j] = 0;

        if (outline->tags[i] & FT_Curve_Tag_On) {
            ft_vector_to_d2d_point(outline->points+i, points+j);
            tags[j] |= OUTLINE_POINT_LINE;
        }
        else {
            /* third order curve */
            if (outline->tags[i] & FT_Curve_Tag_Cubic) {
                /* store 3 points, advance 3 points */

                ft_vector_to_d2d_point(outline->points+i,   points+j);
                ft_vector_to_d2d_point(outline->points+i+1, points+j+1);
                ft_vector_to_d2d_point(outline->points+i+2, points+j+2);

                i += 2;
            }
            else {
                FT_Vector vec;

                /* Convert the quadratic Beziers to cubic Beziers.
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

                /* r1 */
                vec.x = 2 * outline->points[i].x + outline->points[i-1].x;
                vec.y = 2 * outline->points[i].y + outline->points[i-1].y;
                ft_vector_to_d2d_point(&vec, points+j);
                points[j].x /= 3.0;
                points[j].y /= 3.0;

                /* r2 */
                vec.x = 2 * outline->points[i].x + outline->points[i+1].x;
                vec.y = 2 * outline->points[i].y + outline->points[i+1].y;
                ft_vector_to_d2d_point(&vec, points+j+1);
                points[j+1].x /= 3.0;
                points[j+1].y /= 3.0;

                /* r3 */
                ft_vector_to_d2d_point(outline->points+i+1, points+j+2);

                i++;
            }

            tags[j] = tags[j+1] = tags[j+2] = OUTLINE_POINT_BEZIER;
            j += 2;
        }

        /* mark end point */
        if (i < outline->n_points && outline->contours[contour] == i) {
            tags[j] |= OUTLINE_POINT_END;
            contour++;
        }
    }

    return S_OK;
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
             FT_Matrix m;

             m.xx = 1 << 16;
             m.xy = simulations & DWRITE_FONT_SIMULATIONS_OBLIQUE ? (1 << 16) / 3 : 0;
             m.yx = 0;
             m.yy = -(1 << 16); /* flip Y axis */

             pFT_Outline_Transform(outline, &m);

             hr = get_outline_data(outline, ret);
             if (hr == S_OK)
                 (*ret)->advance = size->face->glyph->metrics.horiAdvance >> 6;
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

#endif /* HAVE_FREETYPE */
