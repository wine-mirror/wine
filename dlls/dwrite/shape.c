/*
 *    Glyph shaping support
 *
 * Copyright 2010 Aric Stewart for CodeWeavers
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

#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

struct scriptshaping_cache
{
    IDWriteFontFace *fontface;
    UINT32 language_tag;
};

HRESULT create_scriptshaping_cache(IDWriteFontFace *fontface, const WCHAR *locale, struct scriptshaping_cache **cache)
{
    struct scriptshaping_cache *ret;

    ret = heap_alloc(sizeof(*ret));
    if (!ret)
        return E_OUTOFMEMORY;

    ret->fontface = fontface;
    IDWriteFontFace_AddRef(fontface);

    ret->language_tag = DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t');
    if (locale) {
        WCHAR tag[5];
        if (GetLocaleInfoEx(locale, LOCALE_SOPENTYPELANGUAGETAG, tag, sizeof(tag)/sizeof(WCHAR)))
            ret->language_tag = DWRITE_MAKE_OPENTYPE_TAG(tag[0],tag[1],tag[2],tag[3]);
    }

    *cache = ret;

    return S_OK;
}

void release_scriptshaping_cache(struct scriptshaping_cache *cache)
{
    if (!cache)
        return;
    IDWriteFontFace_Release(cache->fontface);
    heap_free(cache);
}

static void shape_update_clusters_from_glyphprop(UINT32 glyphcount, UINT32 text_len, UINT16 *clustermap, DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props)
{
    UINT32 i;

    for (i = 0; i < glyphcount; i++) {
        if (!glyph_props[i].isClusterStart) {
            UINT32 j;

            for (j = 0; j < text_len; j++) {
                if (clustermap[j] == i) {
                    int k = j;
                    while (k >= 0 && k < text_len && !glyph_props[clustermap[k]].isClusterStart)
                        k--;

                    if (k >= 0 && k < text_len && glyph_props[clustermap[k]].isClusterStart)
                        clustermap[j] = clustermap[k];
                }
            }
        }
    }
}

static int compare_clustersearch(const void *a, const void* b)
{
    UINT16 target = *(UINT16*)a;
    UINT16 index = *(UINT16*)b;
    int ret = 0;

    if (target > index)
        ret = 1;
    else if (target < index)
        ret = -1;

    return ret;
}

/* Maps given glyph position in glyph indices array to text index this glyph represents.
   Lowest possible index is returned.

   clustermap [I] Text index to index in glyph indices array map
   len        [I] Clustermap size
   target     [I] Index in glyph indices array to map
 */
static INT32 map_glyph_to_text_pos(const UINT16 *clustermap, UINT32 len, UINT16 target)
{
    UINT16 *ptr;
    INT32 k;

    ptr = bsearch(&target, clustermap, len, sizeof(UINT16), compare_clustersearch);
    if (!ptr)
        return -1;

    /* get to the beginning */
    for (k = (ptr - clustermap) - 1; k >= 0 && clustermap[k] == target; k--)
        ;
    k++;

    return k;
}

static HRESULT default_set_text_glyphs_props(struct scriptshaping_cache *cache, const WCHAR *text, UINT32 len, UINT16 *clustermap, UINT16 *glyph_indices,
                                     UINT32 glyphcount, DWRITE_SHAPING_TEXT_PROPERTIES *text_props, DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props)
{
    UINT32 i;

    for (i = 0; i < glyphcount; i++) {
        UINT32 char_index[20];
        UINT32 char_count = 0;
        INT32 k;

        k = map_glyph_to_text_pos(clustermap, len, i);
        if (k >= 0) {
            for (; k < len && clustermap[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (char_count == 0)
            continue;

        if (char_count == 1 && isspaceW(text[char_index[0]])) {
            glyph_props[i].justification = SCRIPT_JUSTIFY_BLANK;
            text_props[char_index[0]].isShapedAlone = text[char_index[0]] == ' ';
        }
        else
            glyph_props[i].justification = SCRIPT_JUSTIFY_CHARACTER;
    }

    /* FIXME: update properties using GDEF table */
    shape_update_clusters_from_glyphprop(glyphcount, len, clustermap, glyph_props);

    return S_OK;
}

static HRESULT latn_set_text_glyphs_props(struct scriptshaping_cache *cache, const WCHAR *text, UINT32 len, UINT16 *clustermap, UINT16 *glyph_indices,
                                     UINT32 glyphcount, DWRITE_SHAPING_TEXT_PROPERTIES *text_props, DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props)
{
    HRESULT hr;
    UINT32 i;

    hr = default_set_text_glyphs_props(cache, text, len, clustermap, glyph_indices, glyphcount, text_props, glyph_props);

    for (i = 0; i < glyphcount; i++)
        if (glyph_props[i].isZeroWidthSpace)
            glyph_props[i].justification = SCRIPT_JUSTIFY_NONE;

    return hr;
}

const struct scriptshaping_ops latn_shaping_ops =
{
    NULL,
    latn_set_text_glyphs_props
};

const struct scriptshaping_ops default_shaping_ops =
{
    NULL,
    default_set_text_glyphs_props
};
