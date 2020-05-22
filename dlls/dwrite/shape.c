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
    cache->font->release_font_table(cache->context, cache->gsub.table.context);
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

const struct scriptshaping_ops default_shaping_ops =
{
    NULL,
    default_set_text_glyphs_props
};

static unsigned int shape_select_script(const struct scriptshaping_cache *cache, DWORD kind, const DWORD *scripts,
        unsigned int *script_index)
{
    static const DWORD fallback_scripts[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('D','F','L','T'),
        DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t'),
        DWRITE_MAKE_OPENTYPE_TAG('l','a','t','n'),
        0,
    };
    unsigned int script;

    /* Passed scripts in ascending priority. */
    while (scripts && *scripts)
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

static void shape_add_feature_full(struct shaping_features *features, unsigned int tag, unsigned int flags, unsigned int value)
{
    unsigned int i = features->count;

    if (!dwrite_array_reserve((void **)&features->features, &features->capacity, features->count + 1,
            sizeof(*features->features)))
        return;

    features->features[i].tag = tag;
    features->features[i].flags = flags;
    features->features[i].max_value = value;
    features->features[i].default_value = flags & FEATURE_GLOBAL ? value : 0;
    features->features[i].stage = features->stage;
    features->count++;
}

static void shape_add_feature(struct shaping_features *features, unsigned int tag)
{
    shape_add_feature_full(features, tag, FEATURE_GLOBAL, 1);
}

static int features_sorting_compare(const void *a, const void *b)
{
    const struct shaping_feature *left = a, *right = b;
    return left->tag != right->tag ? (left->tag < right->tag ? -1 : 1) : 0;
};

static void shape_merge_features(struct scriptshaping_context *context, struct shaping_features *features)
{
    const DWRITE_TYPOGRAPHIC_FEATURES **user_features = context->user_features.features;
    unsigned int j = 0, i;

    /* For now only consider global, enabled user features. */
    if (user_features && context->user_features.range_lengths)
    {
        unsigned int flags = context->user_features.range_count == 1 &&
                context->user_features.range_lengths[0] == context->length ? FEATURE_GLOBAL : 0;

        for (i = 0; i < context->user_features.range_count; ++i)
        {
            for (j = 0; j < user_features[i]->featureCount; ++j)
                shape_add_feature_full(features, user_features[i]->features[j].nameTag, flags,
                        user_features[i]->features[j].parameter);
        }
    }

    /* Sort and merge duplicates. */
    qsort(features->features, features->count, sizeof(*features->features), features_sorting_compare);

    for (i = 1; i < features->count; ++i)
    {
        if (features->features[i].tag != features->features[j].tag)
            features->features[++j] = features->features[i];
        else
        {
            if (features->features[i].flags & FEATURE_GLOBAL)
            {
                features->features[j].flags |= FEATURE_GLOBAL;
                features->features[j].max_value = features->features[i].max_value;
                features->features[j].default_value = features->features[i].default_value;
            }
            else
            {
                if (features->features[j].flags & FEATURE_GLOBAL)
                    features->features[j].flags ^= FEATURE_GLOBAL;
                features->features[j].max_value = max(features->features[j].max_value, features->features[i].max_value);
            }
            features->features[j].stage = min(features->features[j].stage, features->features[i].stage);
        }
    }
    features->count = j + 1;
}

HRESULT shape_get_positions(struct scriptshaping_context *context, const unsigned int *scripts)
{
    static const unsigned int common_features[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('a','b','v','m'),
        DWRITE_MAKE_OPENTYPE_TAG('b','l','w','m'),
        DWRITE_MAKE_OPENTYPE_TAG('m','a','r','k'),
        DWRITE_MAKE_OPENTYPE_TAG('m','k','m','k'),
    };
    static const unsigned int horizontal_features[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('c','u','r','s'),
        DWRITE_MAKE_OPENTYPE_TAG('d','i','s','t'),
        DWRITE_MAKE_OPENTYPE_TAG('k','e','r','n'),
    };
    struct scriptshaping_cache *cache = context->cache;
    unsigned int script_index, language_index, script, i;
    struct shaping_features features = { 0 };

    for (i = 0; i < ARRAY_SIZE(common_features); ++i)
        shape_add_feature(&features, common_features[i]);

    /* Horizontal features */
    if (!context->is_sideways)
    {
        for (i = 0; i < ARRAY_SIZE(horizontal_features); ++i)
            shape_add_feature(&features, horizontal_features[i]);
    }

    shape_merge_features(context, &features);

    /* Resolve script tag to actually supported script. */
    if (cache->gpos.table.data)
    {
        if ((script = shape_select_script(cache, MS_GPOS_TAG, scripts, &script_index)))
        {
            DWORD language = context->language_tag;

            if ((language = shape_select_language(cache, MS_GPOS_TAG, script_index, language, &language_index)))
            {
                TRACE("script %s, language %s.\n", debugstr_tag(script), language != ~0u ?
                        debugstr_tag(language) : "deflangsys");
                opentype_layout_apply_gpos_features(context, script_index, language_index, &features);
            }
        }
    }

    for (i = 0; i < context->glyph_count; ++i)
        if (context->u.pos.glyph_props[i].isZeroWidthSpace)
            context->advances[i] = 0.0f;

    heap_free(features.features);

    return S_OK;
}

static unsigned int shape_get_script_lang_index(struct scriptshaping_context *context, const unsigned int *scripts,
        unsigned int table, unsigned int *script_index, unsigned int *language_index)
{
    unsigned int script;

    /* Resolve script tag to actually supported script. */
    if ((script = shape_select_script(context->cache, table, scripts, script_index)))
    {
        unsigned int language = context->language_tag;

        if ((language = shape_select_language(context->cache, table, *script_index, language, language_index)))
            return script;
    }

    return 0;
}

HRESULT shape_get_glyphs(struct scriptshaping_context *context, const unsigned int *scripts)
{
    static const unsigned int common_features[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('c','c','m','p'),
        DWRITE_MAKE_OPENTYPE_TAG('l','o','c','l'),
        DWRITE_MAKE_OPENTYPE_TAG('r','l','i','g'),
    };
    static const unsigned int horizontal_features[] =
    {
        DWRITE_MAKE_OPENTYPE_TAG('c','a','l','t'),
        DWRITE_MAKE_OPENTYPE_TAG('c','l','i','g'),
        DWRITE_MAKE_OPENTYPE_TAG('l','i','g','a'),
        DWRITE_MAKE_OPENTYPE_TAG('r','c','l','t'),
    };
    unsigned int script_index, language_index;
    struct shaping_features features = { 0 };
    unsigned int i;

    if (!context->is_sideways)
    {
        if (context->is_rtl)
        {
            shape_add_feature(&features, DWRITE_MAKE_OPENTYPE_TAG('r','t','l','a'));
            shape_add_feature_full(&features, DWRITE_MAKE_OPENTYPE_TAG('r','t','l','m'), 0, 1);
        }
        else
        {
            shape_add_feature(&features, DWRITE_MAKE_OPENTYPE_TAG('l','t','r','a'));
            shape_add_feature(&features, DWRITE_MAKE_OPENTYPE_TAG('l','t','r','m'));
        }
    }

    for (i = 0; i < ARRAY_SIZE(common_features); ++i)
        shape_add_feature(&features, common_features[i]);

    /* Horizontal features */
    if (!context->is_sideways)
    {
        for (i = 0; i < ARRAY_SIZE(horizontal_features); ++i)
            shape_add_feature(&features, horizontal_features[i]);
    }
    else
        shape_add_feature_full(&features, DWRITE_MAKE_OPENTYPE_TAG('v','e','r','t'), FEATURE_GLOBAL | FEATURE_GLOBAL_SEARCH, 1);

    shape_merge_features(context, &features);

    /* Resolve script tag to actually supported script. */
    shape_get_script_lang_index(context, scripts, MS_GSUB_TAG, &script_index, &language_index);
    opentype_layout_apply_gsub_features(context, script_index, language_index, &features);

    heap_free(features.features);

    return S_OK;
}
