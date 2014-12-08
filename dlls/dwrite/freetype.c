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
MAKE_FUNCPTR(FTC_Manager_New);
MAKE_FUNCPTR(FTC_Manager_Done);
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
    LOAD_FUNCPTR(FTC_Manager_New)
    LOAD_FUNCPTR(FTC_Manager_Done)
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
    if (pFTC_Manager_New(library, 0, 0, 0, &face_requester, NULL, &cache_manager) != 0) {
        ERR("Failed to init FreeType cache\n");
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

#endif /* HAVE_FREETYPE */
