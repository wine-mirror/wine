/*
 * Copyright HarfBuzz Project authors
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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

#include "dwrite_private.h"

static const unsigned int arabic_features[] =
{
    DWRITE_MAKE_OPENTYPE_TAG('i','s','o','l'),
    DWRITE_MAKE_OPENTYPE_TAG('f','i','n','a'),
    DWRITE_MAKE_OPENTYPE_TAG('f','i','n','2'),
    DWRITE_MAKE_OPENTYPE_TAG('f','i','n','3'),
    DWRITE_MAKE_OPENTYPE_TAG('m','e','d','i'),
    DWRITE_MAKE_OPENTYPE_TAG('m','e','d','2'),
    DWRITE_MAKE_OPENTYPE_TAG('i','n','i','t'),
};

static void arabic_collect_features(struct scriptshaping_context *context,
        struct shaping_features *features)
{
    unsigned int i;

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('c','c','m','p'), 0);
    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('l','o','c','l'), 0);
    shape_start_next_stage(features);

    for (i = 0; i < ARRAY_SIZE(arabic_features); ++i)
    {
        shape_enable_feature(features, arabic_features[i], 0);
        shape_start_next_stage(features);
    }

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('r','l','i','g'), FEATURE_MANUAL_ZWJ);

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('r','c','l','t'), FEATURE_MANUAL_ZWJ);
    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('c','a','l','t'), FEATURE_MANUAL_ZWJ);
    shape_start_next_stage(features);

    shape_enable_feature(features, DWRITE_MAKE_OPENTYPE_TAG('m','s','e','t'), 0);
}

static void arabic_setup_masks(struct scriptshaping_context *context,
        const struct shaping_features *features)
{
}

const struct shaper arabic_shaper =
{
    arabic_collect_features,
    arabic_setup_masks,
};
