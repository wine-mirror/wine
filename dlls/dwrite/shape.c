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

struct scriptshaping_cache *create_scriptshaping_cache(void *context, const struct shaping_font_ops *font_ops)
{
    struct scriptshaping_cache *cache;

    cache = heap_alloc_zero(sizeof(*cache));
    if (!cache)
        return NULL;

    cache->font = font_ops;
    cache->context = context;

    cache->upem = cache->font->get_font_upem(cache->context);

    return cache;
}

void release_scriptshaping_cache(struct scriptshaping_cache *cache)
{
    if (!cache)
        return;
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

static HRESULT default_set_text_glyphs_props(struct scriptshaping_context *context, UINT16 *clustermap, UINT16 *glyph_indices,
                                     UINT32 glyphcount, DWRITE_SHAPING_TEXT_PROPERTIES *text_props, DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props)
{
    UINT32 i;

    for (i = 0; i < glyphcount; i++) {
        UINT32 char_index[20];
        UINT32 char_count = 0;
        INT32 k;

        k = map_glyph_to_text_pos(clustermap, context->length, i);
        if (k >= 0) {
            for (; k < context->length && clustermap[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (char_count == 0)
            continue;

        if (char_count == 1 && isspaceW(context->text[char_index[0]])) {
            glyph_props[i].justification = SCRIPT_JUSTIFY_BLANK;
            text_props[char_index[0]].isShapedAlone = context->text[char_index[0]] == ' ';
        }
        else
            glyph_props[i].justification = SCRIPT_JUSTIFY_CHARACTER;
    }

    /* FIXME: update properties using GDEF table */
    shape_update_clusters_from_glyphprop(glyphcount, context->length, clustermap, glyph_props);

    return S_OK;
}

static HRESULT latn_set_text_glyphs_props(struct scriptshaping_context *context, UINT16 *clustermap, UINT16 *glyph_indices,
                                     UINT32 glyphcount, DWRITE_SHAPING_TEXT_PROPERTIES *text_props, DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props)
{
    HRESULT hr;
    UINT32 i;

    hr = default_set_text_glyphs_props(context, clustermap, glyph_indices, glyphcount, text_props, glyph_props);

    for (i = 0; i < glyphcount; i++)
        if (glyph_props[i].isZeroWidthSpace)
            glyph_props[i].justification = SCRIPT_JUSTIFY_NONE;

    return hr;
}

static const DWORD std_gpos_tags[] =
{
    DWRITE_FONT_FEATURE_TAG_KERNING,
    DWRITE_FONT_FEATURE_TAG_MARK_POSITIONING,
    DWRITE_FONT_FEATURE_TAG_MARK_TO_MARK_POSITIONING,
};

static const struct shaping_features std_gpos_features =
{
    std_gpos_tags,
    ARRAY_SIZE(std_gpos_tags),
};

const struct scriptshaping_ops latn_shaping_ops =
{
    NULL,
    latn_set_text_glyphs_props,
    &std_gpos_features,
};

const struct scriptshaping_ops default_shaping_ops =
{
    NULL,
    default_set_text_glyphs_props
};

HRESULT shape_get_positions(struct scriptshaping_context *context, const DWORD *scripts,
        const struct shaping_features *features)
{
    /* FIXME: stub */

    return S_OK;
}
