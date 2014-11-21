/*
 * Copyright 2012 Nikolay Sivov for CodeWeavers
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

#include "dwrite_2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline void *heap_realloc_zero(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(const WCHAR *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR heap_strdupnW(const WCHAR *str, UINT32 len)
{
    WCHAR *ret = NULL;

    if (len)
    {
        ret = heap_alloc((len+1)*sizeof(WCHAR));
        if(ret)
        {
            memcpy(ret, str, len*sizeof(WCHAR));
            ret[len] = 0;
        }
    }

    return ret;
}

static inline const char *debugstr_range(const DWRITE_TEXT_RANGE *range)
{
    return wine_dbg_sprintf("%u:%u", range->startPosition, range->length);
}

static inline unsigned short get_table_entry(const unsigned short *table, WCHAR ch)
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

extern HRESULT convert_fontface_to_logfont(IDWriteFontFace*, LOGFONTW*) DECLSPEC_HIDDEN;
extern HRESULT create_numbersubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD,const WCHAR *locale,BOOL,IDWriteNumberSubstitution**) DECLSPEC_HIDDEN;
extern HRESULT create_textformat(const WCHAR*,IDWriteFontCollection*,DWRITE_FONT_WEIGHT,DWRITE_FONT_STYLE,DWRITE_FONT_STRETCH,
                                 FLOAT,const WCHAR*,IDWriteTextFormat**) DECLSPEC_HIDDEN;
extern HRESULT create_textlayout(const WCHAR*,UINT32,IDWriteTextFormat*,FLOAT,FLOAT,IDWriteTextLayout**) DECLSPEC_HIDDEN;
extern HRESULT create_trimmingsign(IDWriteInlineObject**) DECLSPEC_HIDDEN;
extern HRESULT create_typography(IDWriteTypography**) DECLSPEC_HIDDEN;
extern HRESULT create_gdiinterop(IDWriteFactory*,IDWriteGdiInterop**) DECLSPEC_HIDDEN;
extern void    release_gdiinterop(IDWriteGdiInterop*) DECLSPEC_HIDDEN;
extern HRESULT create_localizedstrings(IDWriteLocalizedStrings**) DECLSPEC_HIDDEN;
extern HRESULT add_localizedstring(IDWriteLocalizedStrings*,const WCHAR*,const WCHAR*) DECLSPEC_HIDDEN;
extern HRESULT clone_localizedstring(IDWriteLocalizedStrings *iface, IDWriteLocalizedStrings **strings) DECLSPEC_HIDDEN;
extern HRESULT get_system_fontcollection(IDWriteFactory*,IDWriteFontCollection**) DECLSPEC_HIDDEN;
extern HRESULT get_textanalyzer(IDWriteTextAnalyzer**) DECLSPEC_HIDDEN;
extern HRESULT create_font_file(IDWriteFontFileLoader *loader, const void *reference_key, UINT32 key_size, IDWriteFontFile **font_file) DECLSPEC_HIDDEN;
extern HRESULT create_localfontfileloader(IDWriteLocalFontFileLoader** iface) DECLSPEC_HIDDEN;
extern HRESULT create_fontface(DWRITE_FONT_FACE_TYPE,UINT32,IDWriteFontFile* const*,UINT32,DWRITE_FONT_SIMULATIONS,IDWriteFontFace2**) DECLSPEC_HIDDEN;
extern HRESULT create_font_collection(IDWriteFactory*,IDWriteFontFileEnumerator*,BOOL,IDWriteFontCollection**) DECLSPEC_HIDDEN;
extern BOOL    is_system_collection(IDWriteFontCollection*) DECLSPEC_HIDDEN;
extern HRESULT get_local_refkey(const WCHAR*,const FILETIME*,void**,UINT32*) DECLSPEC_HIDDEN;

/* Opentype font table functions */
extern HRESULT opentype_analyze_font(IDWriteFontFileStream*,UINT32*,DWRITE_FONT_FILE_TYPE*,DWRITE_FONT_FACE_TYPE*,BOOL*) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_font_table(IDWriteFontFileStream*,DWRITE_FONT_FACE_TYPE,UINT32,UINT32,const void**,void**,UINT32*,BOOL*) DECLSPEC_HIDDEN;
extern void opentype_cmap_get_glyphindex(void*,UINT32,UINT16*) DECLSPEC_HIDDEN;
extern HRESULT opentype_cmap_get_unicode_ranges(void*,UINT32,DWRITE_UNICODE_RANGE*,UINT32*) DECLSPEC_HIDDEN;
extern void opentype_get_font_properties(const void*,const void*,DWRITE_FONT_STRETCH*,DWRITE_FONT_WEIGHT*,DWRITE_FONT_STYLE*) DECLSPEC_HIDDEN;
extern void opentype_get_font_metrics(const void*,const void*,const void*,DWRITE_FONT_METRICS1*) DECLSPEC_HIDDEN;
extern HRESULT opentype_get_font_strings_from_id(const void*,DWRITE_INFORMATIONAL_STRING_ID,IDWriteLocalizedStrings**) DECLSPEC_HIDDEN;

/* BiDi helpers */
extern HRESULT bidi_computelevels(const WCHAR*,UINT32,UINT8,UINT8*,UINT8*) DECLSPEC_HIDDEN;
extern WCHAR bidi_get_mirrored_char(WCHAR) DECLSPEC_HIDDEN;

/* FreeType integration */
struct ft_fontface;
extern BOOL init_freetype(void) DECLSPEC_HIDDEN;
extern HRESULT alloc_ft_fontface(const void*,UINT32,UINT32,struct ft_fontface**) DECLSPEC_HIDDEN;
extern void release_ft_fontface(struct ft_fontface*) DECLSPEC_HIDDEN;

/* Glyph shaping */
enum SCRIPT_JUSTIFY
{
    SCRIPT_JUSTIFY_NONE,
    SCRIPT_JUSTIFY_ARABIC_BLANK,
    SCRIPT_JUSTIFY_CHARACTER,
    SCRIPT_JUSTIFY_RESERVED1,
    SCRIPT_JUSTIFY_BLANK,
    SCRIPT_JUSTIFY_RESERVED2,
    SCRIPT_JUSTIFY_RESERVED3,
    SCRIPT_JUSTIFY_ARABIC_NORMAL,
    SCRIPT_JUSTIFY_ARABIC_KASHIDA,
    SCRIPT_JUSTIFY_ARABIC_ALEF,
    SCRIPT_JUSTIFY_ARABIC_HA,
    SCRIPT_JUSTIFY_ARABIC_RA,
    SCRIPT_JUSTIFY_ARABIC_BA,
    SCRIPT_JUSTIFY_ARABIC_BARA,
    SCRIPT_JUSTIFY_ARABIC_SEEN,
    SCRIPT_JUSTIFY_ARABIC_SEEN_M
};

struct scriptshaping_cache;

struct scriptshaping_context
{
    struct scriptshaping_cache *cache;
    UINT32 language_tag;

    const WCHAR *text;
    UINT32 length;
    BOOL is_rtl;

    UINT32 max_glyph_count;
};

extern HRESULT create_scriptshaping_cache(IDWriteFontFace*,struct scriptshaping_cache**) DECLSPEC_HIDDEN;
extern void release_scriptshaping_cache(struct scriptshaping_cache*) DECLSPEC_HIDDEN;

struct scriptshaping_ops
{
    HRESULT (*contextual_shaping)(struct scriptshaping_context *context, UINT16 *clustermap, UINT16 *glyph_indices, UINT32* actual_glyph_count);
    HRESULT (*set_text_glyphs_props)(struct scriptshaping_context *context, UINT16 *clustermap, UINT16 *glyph_indices,
                                     UINT32 glyphcount, DWRITE_SHAPING_TEXT_PROPERTIES *text_props, DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props);
};

extern const struct scriptshaping_ops default_shaping_ops DECLSPEC_HIDDEN;
extern const struct scriptshaping_ops latn_shaping_ops DECLSPEC_HIDDEN;
