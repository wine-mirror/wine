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

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#include FT_FREETYPE_H
#endif /* HAVE_FT2BUILD_H */

#include "windef.h"
#include "wine/library.h"
#include "wine/debug.h"

#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#ifdef HAVE_FREETYPE

static void *ft_handle = NULL;
static FT_Library library = 0;
typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f = NULL
MAKE_FUNCPTR(FT_Done_Face);
MAKE_FUNCPTR(FT_Init_FreeType);
MAKE_FUNCPTR(FT_Library_Version);
MAKE_FUNCPTR(FT_New_Memory_Face);
#undef MAKE_FUNCPTR

struct ft_fontface
{
    FT_Face face;
};

BOOL init_freetype(void)
{
    FT_Version_t FT_Version;

    ft_handle = wine_dlopen(SONAME_LIBFREETYPE, RTLD_NOW, NULL, 0);
    if (!ft_handle) {
        WINE_MESSAGE("Wine cannot find the FreeType font library.\n");
	return FALSE;
    }

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(ft_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
    LOAD_FUNCPTR(FT_Done_Face)
    LOAD_FUNCPTR(FT_Init_FreeType)
    LOAD_FUNCPTR(FT_Library_Version)
    LOAD_FUNCPTR(FT_New_Memory_Face)
#undef LOAD_FUNCPTR

    if (pFT_Init_FreeType(&library) != 0) {
        ERR("Can't init FreeType library\n");
	wine_dlclose(ft_handle, NULL, 0);
        ft_handle = NULL;
	return FALSE;
    }
    pFT_Library_Version(library, &FT_Version.major, &FT_Version.minor, &FT_Version.patch);

    TRACE("FreeType version is %d.%d.%d\n", FT_Version.major, FT_Version.minor, FT_Version.patch);
    return TRUE;

sym_not_found:
    WINE_MESSAGE("Wine cannot find certain functions that it needs from FreeType library.\n");
    wine_dlclose(ft_handle, NULL, 0);
    ft_handle = NULL;
    return FALSE;
}

HRESULT alloc_ft_fontface(const void *data_ptr, UINT32 data_size, UINT32 index, struct ft_fontface **ftface)
{
    FT_Face face;
    FT_Error err;

    *ftface = heap_alloc_zero(sizeof(struct ft_fontface));
    if (!*ftface)
        return E_OUTOFMEMORY;

    err = pFT_New_Memory_Face(library, data_ptr, data_size, index, &face);
    if (err) {
        ERR("FT_New_Memory_Face rets %d\n", err);
	return E_FAIL;
    }
    (*ftface)->face = face;

    return S_OK;
}

void release_ft_fontface(struct ft_fontface *ftface)
{
    if (!ftface) return;
    pFT_Done_Face(ftface->face);
    heap_free(ftface);
}

#else /* HAVE_FREETYPE */

BOOL init_freetype(void)
{
    return FALSE;
}

HRESULT alloc_ft_fontface(const void *data_ptr, UINT32 data_size, UINT32 index, struct ft_fontface **face)
{
    *face = NULL;
    return S_FALSE;
}

void release_ft_fontface(struct ft_fontface *face)
{
}

#endif /* HAVE_FREETYPE */
