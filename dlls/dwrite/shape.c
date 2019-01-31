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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

#define MS_GPOS_TAG DWRITE_MAKE_OPENTYPE_TAG('G','P','O','S')

struct scriptshaping_cache *create_scriptshaping_cache(void *context, const struct shaping_font_ops *font_ops)
{
    struct scriptshaping_cache *cache;

    cache = heap_alloc_zero(sizeof(*cache));
    if (!cache)
        return NULL;

    cache->font = font_ops;
    cache->context = context;

    opentype_layout_scriptshaping_cache_init(cache);
    cache->upem = cache->font->get_font_upem(cache->context);

    return cache;
}

void release_scriptshaping_cache(struct scriptshaping_cache *cache)
{
    if (!cache)
        return;

    cache->font->release_font_table(cache->context, cache->gdef.table.context);
    cache->font->release_font_table(cache->context, cache->gpos.table.context);
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

static DWORD shape_select_script(const struct scriptshaping_cache *cache, DWORD kind, const DWORD *scripts,
        unsigned int *script_index)
{
    static const DWORD fallback_scripts[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('D','F','L','T'),
        DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t'),
        DWRITE_MAKE_OPENTYPE_TAG('l','a','t','n'),
        0,
    };
    DWORD script;

    /* Passed scripts in ascending priority. */
    while (*scripts)
    {
        if ((script = opentype_layout_find_script(cache, kind, *scripts, script_index)))
            return script;

        scripts++;
    }

    /* 'DFLT' -> 'dflt' -> 'latn' */
    scripts = fallback_scripts;
    while (*scripts)
    {
        if ((script = opentype_layout_find_script(cache, kind, *scripts, script_index)))
            return script;
        scripts++;
    }

    return 0;
}

static DWORD shape_select_language(const struct scriptshaping_cache *cache, DWORD kind, unsigned int script_index,
        DWORD language, unsigned int *language_index)
{
    /* Specified language -> 'dflt'. */
    if ((language = opentype_layout_find_language(cache, kind, language, script_index, language_index)))
        return language;

    if ((language = opentype_layout_find_language(cache, kind, DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t'),
            script_index, language_index)))
        return language;

    return 0;
}

HRESULT shape_get_positions(struct scriptshaping_context *context, const DWORD *scripts,
        const struct shaping_features *features)
{
    struct scriptshaping_cache *cache = context->cache;
    unsigned int script_index, language_index;
    unsigned int i;
    DWORD script;

    /* Resolve script tag to actually supported script. */
    if (cache->gpos.table.data)
    {
        if ((script = shape_select_script(cache, MS_GPOS_TAG, scripts, &script_index)))
        {
            DWORD language = context->language_tag;

            if ((language = shape_select_language(cache, MS_GPOS_TAG, script_index, language, &language_index)))
            {
                TRACE("script %s, language %s.\n", debugstr_tag(script),
                        language != ~0u ? debugstr_tag(language) : "deflangsys");
                opentype_layout_apply_gpos_features(context, script_index, language_index, features);
            }
        }
    }

    for (i = 0; i < context->glyph_count; ++i)
        if (context->u.pos.glyph_props[i].isZeroWidthSpace)
            context->advances[i] = 0.0f;

    return S_OK;
}
