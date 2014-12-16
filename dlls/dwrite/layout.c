/*
 *    Text format and layout
 *
 * Copyright 2012, 2014 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "dwrite_private.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

struct dwrite_textformat_data {
    WCHAR *family_name;
    UINT32 family_len;
    WCHAR *locale;
    UINT32 locale_len;

    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;

    DWRITE_PARAGRAPH_ALIGNMENT paralign;
    DWRITE_READING_DIRECTION readingdir;
    DWRITE_WORD_WRAPPING wrapping;
    DWRITE_TEXT_ALIGNMENT textalignment;
    DWRITE_FLOW_DIRECTION flow;
    DWRITE_LINE_SPACING_METHOD spacingmethod;

    FLOAT spacing;
    FLOAT baseline;
    FLOAT fontsize;

    DWRITE_TRIMMING trimming;
    IDWriteInlineObject *trimmingsign;

    IDWriteFontCollection *collection;
};

enum layout_range_attr_kind {
    LAYOUT_RANGE_ATTR_WEIGHT,
    LAYOUT_RANGE_ATTR_STYLE,
    LAYOUT_RANGE_ATTR_STRETCH,
    LAYOUT_RANGE_ATTR_FONTSIZE,
    LAYOUT_RANGE_ATTR_EFFECT,
    LAYOUT_RANGE_ATTR_INLINE,
    LAYOUT_RANGE_ATTR_UNDERLINE,
    LAYOUT_RANGE_ATTR_STRIKETHROUGH,
    LAYOUT_RANGE_ATTR_FONTCOLL
};

struct layout_range_attr_value {
    DWRITE_TEXT_RANGE range;
    union {
        DWRITE_FONT_WEIGHT weight;
        DWRITE_FONT_STYLE style;
        DWRITE_FONT_STRETCH stretch;
        FLOAT fontsize;
        IDWriteInlineObject *object;
        IUnknown *effect;
        BOOL underline;
        BOOL strikethrough;
        IDWriteFontCollection *collection;
    } u;
};

struct layout_range {
    struct list entry;
    DWRITE_TEXT_RANGE range;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    FLOAT fontsize;
    DWRITE_FONT_STRETCH stretch;
    IDWriteInlineObject *object;
    IUnknown *effect;
    BOOL underline;
    BOOL strikethrough;
    IDWriteFontCollection *collection;
};

struct dwrite_textlayout {
    IDWriteTextLayout2 IDWriteTextLayout2_iface;
    LONG ref;

    WCHAR *str;
    UINT32 len;
    struct dwrite_textformat_data format;
    FLOAT  maxwidth;
    FLOAT  maxheight;
    struct list ranges;
};

struct dwrite_textformat {
    IDWriteTextFormat1 IDWriteTextFormat1_iface;
    LONG ref;
    struct dwrite_textformat_data format;
};

struct dwrite_trimmingsign {
    IDWriteInlineObject IDWriteInlineObject_iface;
    LONG ref;
};

struct dwrite_typography {
    IDWriteTypography IDWriteTypography_iface;
    LONG ref;

    DWRITE_FONT_FEATURE *features;
    UINT32 allocated;
    UINT32 count;
};

static const IDWriteTextFormat1Vtbl dwritetextformatvtbl;

static void release_format_data(struct dwrite_textformat_data *data)
{
    if (data->collection) IDWriteFontCollection_Release(data->collection);
    if (data->trimmingsign) IDWriteInlineObject_Release(data->trimmingsign);
    heap_free(data->family_name);
    heap_free(data->locale);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextLayout2(IDWriteTextLayout2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextLayout2_iface);
}

static inline struct dwrite_textformat *impl_from_IDWriteTextFormat1(IDWriteTextFormat1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textformat, IDWriteTextFormat1_iface);
}

static inline struct dwrite_textformat *unsafe_impl_from_IDWriteTextFormat1(IDWriteTextFormat1 *iface)
{
    return iface->lpVtbl == &dwritetextformatvtbl ? impl_from_IDWriteTextFormat1(iface) : NULL;
}

static inline struct dwrite_trimmingsign *impl_from_IDWriteInlineObject(IDWriteInlineObject *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_trimmingsign, IDWriteInlineObject_iface);
}

static inline struct dwrite_typography *impl_from_IDWriteTypography(IDWriteTypography *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_typography, IDWriteTypography_iface);
}

/* To be used in IDWriteTextLayout methods to validate and fix passed range */
static inline BOOL validate_text_range(struct dwrite_textlayout *layout, DWRITE_TEXT_RANGE *r)
{
    if (r->startPosition >= layout->len)
        return FALSE;

    if (r->startPosition + r->length > layout->len)
        r->length = layout->len - r->startPosition;

    return TRUE;
}

static BOOL is_same_layout_attrvalue(struct layout_range const *range, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    switch (attr) {
    case LAYOUT_RANGE_ATTR_WEIGHT:
        return range->weight == value->u.weight;
    case LAYOUT_RANGE_ATTR_STYLE:
        return range->style == value->u.style;
    case LAYOUT_RANGE_ATTR_STRETCH:
        return range->stretch == value->u.stretch;
    case LAYOUT_RANGE_ATTR_FONTSIZE:
        return range->fontsize == value->u.fontsize;
    case LAYOUT_RANGE_ATTR_INLINE:
        return range->object == value->u.object;
    case LAYOUT_RANGE_ATTR_EFFECT:
        return range->effect == value->u.effect;
    case LAYOUT_RANGE_ATTR_UNDERLINE:
        return range->underline == value->u.underline;
    case LAYOUT_RANGE_ATTR_STRIKETHROUGH:
        return range->strikethrough == value->u.strikethrough;
    case LAYOUT_RANGE_ATTR_FONTCOLL:
        return range->collection == value->u.collection;
    default:
        ;
    }

    return FALSE;
}

static inline BOOL is_same_layout_attributes(struct layout_range const *left, struct layout_range const *right)
{
    return left->weight == right->weight &&
           left->style  == right->style &&
           left->stretch == right->stretch &&
           left->fontsize == right->fontsize &&
           left->object == right->object &&
           left->effect == right->effect &&
           left->underline == right->underline &&
           left->strikethrough == right->strikethrough &&
           left->collection == right->collection;
}

static inline BOOL is_same_text_range(const DWRITE_TEXT_RANGE *left, const DWRITE_TEXT_RANGE *right)
{
    return left->startPosition == right->startPosition && left->length == right->length;
}

/* Allocates range and inits it with default values from text format. */
static struct layout_range *alloc_layout_range(struct dwrite_textlayout *layout, const DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;

    range = heap_alloc(sizeof(*range));
    if (!range) return NULL;

    range->range = *r;
    range->weight = layout->format.weight;
    range->style  = layout->format.style;
    range->stretch = layout->format.stretch;
    range->fontsize = layout->format.fontsize;
    range->object = NULL;
    range->effect = NULL;
    range->underline = FALSE;
    range->strikethrough = FALSE;
    range->collection = layout->format.collection;
    if (range->collection)
        IDWriteFontCollection_AddRef(range->collection);

    return range;
}

static struct layout_range *alloc_layout_range_from(struct layout_range *from, const DWRITE_TEXT_RANGE *r)
{
    struct layout_range *range;

    range = heap_alloc(sizeof(*range));
    if (!range) return NULL;

    *range = *from;
    range->range = *r;

    /* update refcounts */
    if (range->object)
        IDWriteInlineObject_AddRef(range->object);
    if (range->effect)
        IUnknown_AddRef(range->effect);
    if (range->collection)
        IDWriteFontCollection_AddRef(range->collection);

    return range;
}

static void free_layout_range(struct layout_range *range)
{
    if (!range)
        return;
    if (range->object)
        IDWriteInlineObject_Release(range->object);
    if (range->effect)
        IUnknown_Release(range->effect);
    if (range->collection)
        IDWriteFontCollection_Release(range->collection);
    heap_free(range);
}

static void free_layout_ranges_list(struct dwrite_textlayout *layout)
{
    struct layout_range *cur, *cur2;
    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &layout->ranges, struct layout_range, entry) {
        list_remove(&cur->entry);
        free_layout_range(cur);
    }
}

static struct layout_range *find_outer_range(struct dwrite_textlayout *layout, const DWRITE_TEXT_RANGE *range)
{
    struct layout_range *cur;

    LIST_FOR_EACH_ENTRY(cur, &layout->ranges, struct layout_range, entry) {

        if (cur->range.startPosition > range->startPosition)
            return NULL;

        if ((cur->range.startPosition + cur->range.length < range->startPosition + range->length) &&
            (range->startPosition < cur->range.startPosition + cur->range.length))
            return NULL;
        if (cur->range.startPosition + cur->range.length >= range->startPosition + range->length)
            return cur;
    }

    return NULL;
}

static struct layout_range *get_layout_range_by_pos(struct dwrite_textlayout *layout, UINT32 pos)
{
    struct layout_range *cur;

    LIST_FOR_EACH_ENTRY(cur, &layout->ranges, struct layout_range, entry) {
        DWRITE_TEXT_RANGE *r = &cur->range;
        if (r->startPosition <= pos && pos < r->startPosition + r->length)
            return cur;
    }

    return NULL;
}

static BOOL set_layout_range_attrval(struct layout_range *dest, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    BOOL changed = FALSE;

    switch (attr) {
    case LAYOUT_RANGE_ATTR_WEIGHT:
        changed = dest->weight != value->u.weight;
        dest->weight = value->u.weight;
        break;
    case LAYOUT_RANGE_ATTR_STYLE:
        changed = dest->style != value->u.style;
        dest->style = value->u.style;
        break;
    case LAYOUT_RANGE_ATTR_STRETCH:
        changed = dest->stretch != value->u.stretch;
        dest->stretch = value->u.stretch;
        break;
    case LAYOUT_RANGE_ATTR_FONTSIZE:
        changed = dest->fontsize != value->u.fontsize;
        dest->fontsize = value->u.fontsize;
        break;
    case LAYOUT_RANGE_ATTR_INLINE:
        changed = dest->object != value->u.object;
        if (changed && dest->object)
            IDWriteInlineObject_Release(dest->object);
        dest->object = value->u.object;
        if (dest->object)
            IDWriteInlineObject_AddRef(dest->object);
        break;
    case LAYOUT_RANGE_ATTR_EFFECT:
        changed = dest->effect != value->u.effect;
        if (changed && dest->effect)
            IUnknown_Release(dest->effect);
        dest->effect = value->u.effect;
        if (dest->effect)
            IUnknown_AddRef(dest->effect);
        break;
    case LAYOUT_RANGE_ATTR_UNDERLINE:
        changed = dest->underline != value->u.underline;
        dest->underline = value->u.underline;
        break;
    case LAYOUT_RANGE_ATTR_STRIKETHROUGH:
        changed = dest->strikethrough != value->u.strikethrough;
        dest->strikethrough = value->u.strikethrough;
        break;
    case LAYOUT_RANGE_ATTR_FONTCOLL:
        changed = dest->collection != value->u.collection;
        if (changed && dest->collection)
            IDWriteFontCollection_Release(dest->collection);
        dest->collection = value->u.collection;
        if (dest->collection)
            IDWriteFontCollection_AddRef(dest->collection);
        break;
    default:
        ;
    }

    return changed;
}

static inline BOOL is_in_layout_range(const DWRITE_TEXT_RANGE *outer, const DWRITE_TEXT_RANGE *inner)
{
    return (inner->startPosition >= outer->startPosition) &&
           (inner->startPosition + inner->length <= outer->startPosition + outer->length);
}

static inline HRESULT return_range(const struct layout_range *range, DWRITE_TEXT_RANGE *r)
{
    if (r) *r = range->range;
    return S_OK;
}

/* Set attribute value for given range, does all needed splitting/merging of existing ranges. */
static HRESULT set_layout_range_attr(struct dwrite_textlayout *layout, enum layout_range_attr_kind attr, struct layout_range_attr_value *value)
{
    struct layout_range *outer, *right, *left, *cur;
    struct list *ranges = &layout->ranges;
    BOOL changed = FALSE;
    DWRITE_TEXT_RANGE r;

    /* If new range is completely within existing range, split existing range in two */
    if ((outer = find_outer_range(layout, &value->range))) {

        /* no need to add same range */
        if (is_same_layout_attrvalue(outer, attr, value))
            return S_OK;

        /* for matching range bounds just replace data */
        if (is_same_text_range(&outer->range, &value->range)) {
            changed = set_layout_range_attrval(outer, attr, value);
            goto done;
        }

        /* add new range to the left */
        if (value->range.startPosition == outer->range.startPosition) {
            left = alloc_layout_range_from(outer, &value->range);
            if (!left) return E_OUTOFMEMORY;

            changed = set_layout_range_attrval(left, attr, value);
            list_add_before(&outer->entry, &left->entry);
            outer->range.startPosition += value->range.length;
            outer->range.length -= value->range.length;
            goto done;
        }

        /* add new range to the right */
        if (value->range.startPosition + value->range.length == outer->range.startPosition + outer->range.length) {
            right = alloc_layout_range_from(outer, &value->range);
            if (!right) return E_OUTOFMEMORY;

            changed = set_layout_range_attrval(right, attr, value);
            list_add_after(&outer->entry, &right->entry);
            outer->range.length -= value->range.length;
            goto done;
        }

        r.startPosition = value->range.startPosition + value->range.length;
        r.length = outer->range.length + outer->range.startPosition - r.startPosition;

        /* right part */
        right = alloc_layout_range_from(outer, &r);
        /* new range in the middle */
        cur = alloc_layout_range_from(outer, &value->range);
        if (!right || !cur) {
            free_layout_range(right);
            free_layout_range(cur);
            return E_OUTOFMEMORY;
        }

        /* reuse container range as a left part */
        outer->range.length = value->range.startPosition - outer->range.startPosition;

        /* new part */
        set_layout_range_attrval(cur, attr, value);

        list_add_after(&outer->entry, &cur->entry);
        list_add_after(&cur->entry, &right->entry);

        return S_OK;
    }

    /* Now it's only possible that given range contains some existing ranges, fully or partially.
       Update all of them. */
    left = get_layout_range_by_pos(layout, value->range.startPosition);
    if (left->range.startPosition == value->range.startPosition)
        changed = set_layout_range_attrval(left, attr, value);
    else /* need to split */ {
        r.startPosition = value->range.startPosition;
        r.length = left->range.length - value->range.startPosition + left->range.startPosition;
        left->range.length -= r.length;
        cur = alloc_layout_range_from(left, &r);
        changed = set_layout_range_attrval(cur, attr, value);
        list_add_after(&left->entry, &cur->entry);
    }
    cur = LIST_ENTRY(list_next(ranges, &left->entry), struct layout_range, entry);

    /* for all existing ranges covered by new one update value */
    while (is_in_layout_range(&value->range, &cur->range)) {
        changed = set_layout_range_attrval(cur, attr, value);
        cur = LIST_ENTRY(list_next(ranges, &cur->entry), struct layout_range, entry);
    }

    /* it's possible rightmost range intersects */
    if (cur && (cur->range.startPosition < value->range.startPosition + value->range.length)) {
        r.startPosition = cur->range.startPosition;
        r.length = value->range.startPosition + value->range.length - cur->range.startPosition;
        left = alloc_layout_range_from(cur, &r);
        changed = set_layout_range_attrval(left, attr, value);
        cur->range.startPosition += left->range.length;
        cur->range.length -= left->range.length;
        list_add_before(&cur->entry, &left->entry);
    }

done:
    if (changed) {
        struct list *next, *i;

        i = list_head(ranges);
        while ((next = list_next(ranges, i))) {
            struct layout_range *next_range = LIST_ENTRY(next, struct layout_range, entry);

            cur = LIST_ENTRY(i, struct layout_range, entry);
            if (is_same_layout_attributes(cur, next_range)) {
                /* remove similar range */
                cur->range.length += next_range->range.length;
                list_remove(next);
                free_layout_range(next_range);
            }
            else
                i = list_next(ranges, i);
        }
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_QueryInterface(IDWriteTextLayout2 *iface, REFIID riid, void **obj)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTextLayout2) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout1) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteTextLayout2_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_AddRef(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetextlayout_Release(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        free_layout_ranges_list(This);
        release_format_data(&This->format);
        heap_free(This->str);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritetextlayout_SetTextAlignment(IDWriteTextLayout2 *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetParagraphAlignment(IDWriteTextLayout2 *iface, DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetWordWrapping(IDWriteTextLayout2 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, wrapping);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetReadingDirection(IDWriteTextLayout2 *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFlowDirection(IDWriteTextLayout2 *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetIncrementalTabStop(IDWriteTextLayout2 *iface, FLOAT tabstop)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%f): stub\n", This, tabstop);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetTrimming(IDWriteTextLayout2 *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %p): stub\n", This, trimming, trimming_sign);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetLineSpacing(IDWriteTextLayout2 *iface, DWRITE_LINE_SPACING_METHOD spacing,
    FLOAT line_spacing, FLOAT baseline)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d %f %f): stub\n", This, spacing, line_spacing, baseline);
    return E_NOTIMPL;
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextlayout_GetTextAlignment(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.textalignment;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextlayout_GetParagraphAlignment(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.paralign;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextlayout_GetWordWrapping(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p): stub\n", This);
    return This->format.wrapping;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextlayout_GetReadingDirection(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.readingdir;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextlayout_GetFlowDirection(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.flow;
}

static FLOAT WINAPI dwritetextlayout_GetIncrementalTabStop(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static HRESULT WINAPI dwritetextlayout_GetTrimming(IDWriteTextLayout2 *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%p %p)\n", This, options, trimming_sign);

    *options = This->format.trimming;
    *trimming_sign = This->format.trimmingsign;
    if (*trimming_sign)
        IDWriteInlineObject_AddRef(*trimming_sign);
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_GetLineSpacing(IDWriteTextLayout2 *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%p %p %p)\n", This, method, spacing, baseline);

    *method = This->format.spacingmethod;
    *spacing = This->format.spacing;
    *baseline = This->format.baseline;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_GetFontCollection(IDWriteTextLayout2 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%p)\n", This, collection);

    *collection = This->format.collection;
    if (*collection)
        IDWriteFontCollection_AddRef(*collection);
    return S_OK;
}

static UINT32 WINAPI dwritetextlayout_GetFontFamilyNameLength(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.family_len;
}

static HRESULT WINAPI dwritetextlayout_GetFontFamilyName(IDWriteTextLayout2 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.family_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.family_name);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextlayout_GetFontWeight(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.weight;
}

static DWRITE_FONT_STYLE WINAPI dwritetextlayout_GetFontStyle(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.style;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextlayout_GetFontStretch(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.stretch;
}

static FLOAT WINAPI dwritetextlayout_GetFontSize(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.fontsize;
}

static UINT32 WINAPI dwritetextlayout_GetLocaleNameLength(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->format.locale_len;
}

static HRESULT WINAPI dwritetextlayout_GetLocaleName(IDWriteTextLayout2 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.locale_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.locale);
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetMaxWidth(IDWriteTextLayout2 *iface, FLOAT maxWidth)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%.1f)\n", This, maxWidth);

    if (maxWidth < 0.0)
        return E_INVALIDARG;

    This->maxwidth = maxWidth;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetMaxHeight(IDWriteTextLayout2 *iface, FLOAT maxHeight)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)->(%.1f)\n", This, maxHeight);

    if (maxHeight < 0.0)
        return E_INVALIDARG;

    This->maxheight = maxHeight;
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetFontCollection(IDWriteTextLayout2 *iface, IDWriteFontCollection* collection, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%p %s)\n", This, collection, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.collection = collection;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_FONTCOLL, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontFamilyName(IDWriteTextLayout2 *iface, WCHAR const *name, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(name), debugstr_range(&range));
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFontWeight(IDWriteTextLayout2 *iface, DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, weight, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.weight = weight;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_WEIGHT, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontStyle(IDWriteTextLayout2 *iface, DWRITE_FONT_STYLE style, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, style, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.style = style;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_STYLE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontStretch(IDWriteTextLayout2 *iface, DWRITE_FONT_STRETCH stretch, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, stretch, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.stretch = stretch;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_STRETCH, &value);
}

static HRESULT WINAPI dwritetextlayout_SetFontSize(IDWriteTextLayout2 *iface, FLOAT size, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%.2f %s)\n", This, size, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.fontsize = size;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_FONTSIZE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetUnderline(IDWriteTextLayout2 *iface, BOOL underline, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, underline, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.underline = underline;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_UNDERLINE, &value);
}

static HRESULT WINAPI dwritetextlayout_SetStrikethrough(IDWriteTextLayout2 *iface, BOOL strikethrough, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%d %s)\n", This, strikethrough, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.underline = strikethrough;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_STRIKETHROUGH, &value);
}

static HRESULT WINAPI dwritetextlayout_SetDrawingEffect(IDWriteTextLayout2 *iface, IUnknown* effect, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value value;

    TRACE("(%p)->(%p %s)\n", This, effect, debugstr_range(&range));

    if (!validate_text_range(This, &range))
        return S_OK;

    value.range = range;
    value.u.effect = effect;
    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_EFFECT, &value);
}

static HRESULT WINAPI dwritetextlayout_SetInlineObject(IDWriteTextLayout2 *iface, IDWriteInlineObject *object, DWRITE_TEXT_RANGE r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range_attr_value attr;

    TRACE("(%p)->(%p %s)\n", This, object, debugstr_range(&r));

    if (!validate_text_range(This, &r))
        return S_OK;

    attr.range = r;
    attr.u.object = object;

    return set_layout_range_attr(This, LAYOUT_RANGE_ATTR_INLINE, &attr);
}

static HRESULT WINAPI dwritetextlayout_SetTypography(IDWriteTextLayout2 *iface, IDWriteTypography* typography, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %s): stub\n", This, typography, debugstr_range(&range));
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetLocaleName(IDWriteTextLayout2 *iface, WCHAR const* locale, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%s %s): stub\n", This, debugstr_w(locale), debugstr_range(&range));
    return E_NOTIMPL;
}

static FLOAT WINAPI dwritetextlayout_GetMaxWidth(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->maxwidth;
}

static FLOAT WINAPI dwritetextlayout_GetMaxHeight(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    TRACE("(%p)\n", This);
    return This->maxheight;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontCollection(IDWriteTextLayout2 *iface, UINT32 position,
    IDWriteFontCollection** collection, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, collection, r);

    range = get_layout_range_by_pos(This, position);
    *collection = range ? range->collection : NULL;
    if (*collection)
        IDWriteFontCollection_AddRef(*collection);

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyNameLength(IDWriteTextLayout2 *iface,
    UINT32 pos, UINT32* len, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d %p %p): stub\n", This, pos, len, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyName(IDWriteTextLayout2 *iface,
    UINT32 position, WCHAR* name, UINT32 name_size, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %p %u %p): stub\n", This, position, name, name_size, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontWeight(IDWriteTextLayout2 *iface,
    UINT32 position, DWRITE_FONT_WEIGHT *weight, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, weight, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *weight = range->weight;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStyle(IDWriteTextLayout2 *iface,
    UINT32 position, DWRITE_FONT_STYLE *style, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, style, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *style = range->style;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStretch(IDWriteTextLayout2 *iface,
    UINT32 position, DWRITE_FONT_STRETCH *stretch, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, stretch, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *stretch = range->stretch;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontSize(IDWriteTextLayout2 *iface,
    UINT32 position, FLOAT *size, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, size, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *size = range->fontsize;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetUnderline(IDWriteTextLayout2 *iface,
    UINT32 position, BOOL *underline, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, underline, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *underline = range->underline;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetStrikethrough(IDWriteTextLayout2 *iface,
    UINT32 position, BOOL *strikethrough, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, strikethrough, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *strikethrough = range->strikethrough;

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetDrawingEffect(IDWriteTextLayout2 *iface,
    UINT32 position, IUnknown **effect, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, effect, r);

    if (position >= This->len)
        return S_OK;

    range = get_layout_range_by_pos(This, position);
    *effect = range->effect;
    if (*effect)
        IUnknown_AddRef(*effect);

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetInlineObject(IDWriteTextLayout2 *iface,
    UINT32 position, IDWriteInlineObject **object, DWRITE_TEXT_RANGE *r)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    struct layout_range *range;

    TRACE("(%p)->(%u %p %p)\n", This, position, object, r);

    range = get_layout_range_by_pos(This, position);
    *object = range ? range->object : NULL;
    if (*object)
        IDWriteInlineObject_AddRef(*object);

    return return_range(range, r);
}

static HRESULT WINAPI dwritetextlayout_GetTypography(IDWriteTextLayout2 *iface,
    UINT32 position, IDWriteTypography** typography, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, typography, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleNameLength(IDWriteTextLayout2 *iface,
    UINT32 position, UINT32* length, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, length, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleName(IDWriteTextLayout2 *iface,
    UINT32 position, WCHAR* name, UINT32 name_size, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %p %u %p): stub\n", This, position, name, name_size, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_Draw(IDWriteTextLayout2 *iface,
    void *context, IDWriteTextRenderer* renderer, FLOAT originX, FLOAT originY)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %p %f %f): stub\n", This, context, renderer, originX, originY);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetLineMetrics(IDWriteTextLayout2 *iface,
    DWRITE_LINE_METRICS *metrics, UINT32 max_count, UINT32 *actual_count)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %u %p): stub\n", This, metrics, max_count, actual_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetMetrics(IDWriteTextLayout2 *iface, DWRITE_TEXT_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetOverhangMetrics(IDWriteTextLayout2 *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, overhangs);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetClusterMetrics(IDWriteTextLayout2 *iface,
    DWRITE_CLUSTER_METRICS *metrics, UINT32 max_count, UINT32* act_count)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %u %p): stub\n", This, metrics, max_count, act_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_DetermineMinWidth(IDWriteTextLayout2 *iface, FLOAT* min_width)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, min_width);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestPoint(IDWriteTextLayout2 *iface,
    FLOAT pointX, FLOAT pointY, BOOL* is_trailinghit, BOOL* is_inside, DWRITE_HIT_TEST_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%f %f %p %p %p): stub\n", This, pointX, pointY, is_trailinghit, is_inside, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextPosition(IDWriteTextLayout2 *iface,
    UINT32 textPosition, BOOL is_trailinghit, FLOAT* pointX, FLOAT* pointY, DWRITE_HIT_TEST_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %d %p %p %p): stub\n", This, textPosition, is_trailinghit, pointX, pointY, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextRange(IDWriteTextLayout2 *iface,
    UINT32 textPosition, UINT32 textLength, FLOAT originX, FLOAT originY,
    DWRITE_HIT_TEST_METRICS *metrics, UINT32 max_metricscount, UINT32* actual_metricscount)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %u %f %f %p %u %p): stub\n", This, textPosition, textLength, originX, originY, metrics,
        max_metricscount, actual_metricscount);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout1_SetPairKerning(IDWriteTextLayout2 *iface, BOOL is_pairkerning_enabled,
        DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d %s): stub\n", This, is_pairkerning_enabled, debugstr_range(&range));
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout1_GetPairKerning(IDWriteTextLayout2 *iface, UINT32 position, BOOL *is_pairkerning_enabled,
        DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p %p): stub\n", This, is_pairkerning_enabled, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout1_SetCharacterSpacing(IDWriteTextLayout2 *iface, FLOAT leading_spacing, FLOAT trailing_spacing,
    FLOAT minimum_advance_width, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%f %f %f %s): stub\n", This, leading_spacing, trailing_spacing, minimum_advance_width, debugstr_range(&range));
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout1_GetCharacterSpacing(IDWriteTextLayout2 *iface, UINT32 position, FLOAT* leading_spacing,
    FLOAT* trailing_spacing, FLOAT* minimum_advance_width, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%u %p %p %p %p): stub\n", This, position, leading_spacing, trailing_spacing, minimum_advance_width, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout2_GetMetrics(IDWriteTextLayout2 *iface, DWRITE_TEXT_METRICS1 *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout2_SetVerticalGlyphOrientation(IDWriteTextLayout2 *iface, DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, orientation);
    return E_NOTIMPL;
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextlayout2_GetVerticalGlyphOrientation(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT;
}

static HRESULT WINAPI dwritetextlayout2_SetLastLineWrapping(IDWriteTextLayout2 *iface, BOOL lastline_wrapping_enabled)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, lastline_wrapping_enabled);
    return E_NOTIMPL;
}

static BOOL WINAPI dwritetextlayout2_GetLastLineWrapping(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritetextlayout2_SetOpticalAlignment(IDWriteTextLayout2 *iface, DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextlayout2_GetOpticalAlignment(IDWriteTextLayout2 *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_OPTICAL_ALIGNMENT_NONE;
}

static HRESULT WINAPI dwritetextlayout2_SetFontFallback(IDWriteTextLayout2 *iface, IDWriteFontFallback *fallback)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, fallback);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout2_GetFontFallback(IDWriteTextLayout2 *iface, IDWriteFontFallback **fallback)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout2(iface);
    FIXME("(%p)->(%p): stub\n", This, fallback);
    return E_NOTIMPL;
}

static const IDWriteTextLayout2Vtbl dwritetextlayoutvtbl = {
    dwritetextlayout_QueryInterface,
    dwritetextlayout_AddRef,
    dwritetextlayout_Release,
    dwritetextlayout_SetTextAlignment,
    dwritetextlayout_SetParagraphAlignment,
    dwritetextlayout_SetWordWrapping,
    dwritetextlayout_SetReadingDirection,
    dwritetextlayout_SetFlowDirection,
    dwritetextlayout_SetIncrementalTabStop,
    dwritetextlayout_SetTrimming,
    dwritetextlayout_SetLineSpacing,
    dwritetextlayout_GetTextAlignment,
    dwritetextlayout_GetParagraphAlignment,
    dwritetextlayout_GetWordWrapping,
    dwritetextlayout_GetReadingDirection,
    dwritetextlayout_GetFlowDirection,
    dwritetextlayout_GetIncrementalTabStop,
    dwritetextlayout_GetTrimming,
    dwritetextlayout_GetLineSpacing,
    dwritetextlayout_GetFontCollection,
    dwritetextlayout_GetFontFamilyNameLength,
    dwritetextlayout_GetFontFamilyName,
    dwritetextlayout_GetFontWeight,
    dwritetextlayout_GetFontStyle,
    dwritetextlayout_GetFontStretch,
    dwritetextlayout_GetFontSize,
    dwritetextlayout_GetLocaleNameLength,
    dwritetextlayout_GetLocaleName,
    dwritetextlayout_SetMaxWidth,
    dwritetextlayout_SetMaxHeight,
    dwritetextlayout_SetFontCollection,
    dwritetextlayout_SetFontFamilyName,
    dwritetextlayout_SetFontWeight,
    dwritetextlayout_SetFontStyle,
    dwritetextlayout_SetFontStretch,
    dwritetextlayout_SetFontSize,
    dwritetextlayout_SetUnderline,
    dwritetextlayout_SetStrikethrough,
    dwritetextlayout_SetDrawingEffect,
    dwritetextlayout_SetInlineObject,
    dwritetextlayout_SetTypography,
    dwritetextlayout_SetLocaleName,
    dwritetextlayout_GetMaxWidth,
    dwritetextlayout_GetMaxHeight,
    dwritetextlayout_layout_GetFontCollection,
    dwritetextlayout_layout_GetFontFamilyNameLength,
    dwritetextlayout_layout_GetFontFamilyName,
    dwritetextlayout_layout_GetFontWeight,
    dwritetextlayout_layout_GetFontStyle,
    dwritetextlayout_layout_GetFontStretch,
    dwritetextlayout_layout_GetFontSize,
    dwritetextlayout_GetUnderline,
    dwritetextlayout_GetStrikethrough,
    dwritetextlayout_GetDrawingEffect,
    dwritetextlayout_GetInlineObject,
    dwritetextlayout_GetTypography,
    dwritetextlayout_layout_GetLocaleNameLength,
    dwritetextlayout_layout_GetLocaleName,
    dwritetextlayout_Draw,
    dwritetextlayout_GetLineMetrics,
    dwritetextlayout_GetMetrics,
    dwritetextlayout_GetOverhangMetrics,
    dwritetextlayout_GetClusterMetrics,
    dwritetextlayout_DetermineMinWidth,
    dwritetextlayout_HitTestPoint,
    dwritetextlayout_HitTestTextPosition,
    dwritetextlayout_HitTestTextRange,
    dwritetextlayout1_SetPairKerning,
    dwritetextlayout1_GetPairKerning,
    dwritetextlayout1_SetCharacterSpacing,
    dwritetextlayout1_GetCharacterSpacing,
    dwritetextlayout2_GetMetrics,
    dwritetextlayout2_SetVerticalGlyphOrientation,
    dwritetextlayout2_GetVerticalGlyphOrientation,
    dwritetextlayout2_SetLastLineWrapping,
    dwritetextlayout2_GetLastLineWrapping,
    dwritetextlayout2_SetOpticalAlignment,
    dwritetextlayout2_GetOpticalAlignment,
    dwritetextlayout2_SetFontFallback,
    dwritetextlayout2_GetFontFallback
};

static void layout_format_from_textformat(struct dwrite_textlayout *layout, IDWriteTextFormat *format)
{
    struct dwrite_textformat *f;

    memset(&layout->format, 0, sizeof(layout->format));

    if ((f = unsafe_impl_from_IDWriteTextFormat1((IDWriteTextFormat1*)format)))
    {
        layout->format = f->format;
        layout->format.locale = heap_strdupW(f->format.locale);
        layout->format.family_name = heap_strdupW(f->format.family_name);
        if (layout->format.trimmingsign)
            IDWriteInlineObject_AddRef(layout->format.trimmingsign);
    }
    else
    {
        UINT32 locale_len, family_len;

        layout->format.weight  = IDWriteTextFormat_GetFontWeight(format);
        layout->format.style   = IDWriteTextFormat_GetFontStyle(format);
        layout->format.stretch = IDWriteTextFormat_GetFontStretch(format);
        layout->format.fontsize= IDWriteTextFormat_GetFontSize(format);
        layout->format.textalignment = IDWriteTextFormat_GetTextAlignment(format);
        layout->format.paralign = IDWriteTextFormat_GetParagraphAlignment(format);
        layout->format.wrapping = IDWriteTextFormat_GetWordWrapping(format);
        layout->format.readingdir = IDWriteTextFormat_GetReadingDirection(format);
        layout->format.flow = IDWriteTextFormat_GetFlowDirection(format);
        IDWriteTextFormat_GetLineSpacing(format,
            &layout->format.spacingmethod,
            &layout->format.spacing,
            &layout->format.baseline
        );
        IDWriteTextFormat_GetTrimming(format, &layout->format.trimming, &layout->format.trimmingsign);

        /* locale name and length */
        locale_len = IDWriteTextFormat_GetLocaleNameLength(format);
        layout->format.locale  = heap_alloc((locale_len+1)*sizeof(WCHAR));
        IDWriteTextFormat_GetLocaleName(format, layout->format.locale, locale_len+1);
        layout->format.locale_len = locale_len;

        /* font family name and length */
        family_len = IDWriteTextFormat_GetFontFamilyNameLength(format);
        layout->format.family_name = heap_alloc((family_len+1)*sizeof(WCHAR));
        IDWriteTextFormat_GetFontFamilyName(format, layout->format.family_name, family_len+1);
        layout->format.family_len = family_len;
    }

    IDWriteTextFormat_GetFontCollection(format, &layout->format.collection);
}

HRESULT create_textlayout(const WCHAR *str, UINT32 len, IDWriteTextFormat *format, FLOAT maxwidth, FLOAT maxheight, IDWriteTextLayout **layout)
{
    struct dwrite_textlayout *This;
    struct layout_range *range;
    DWRITE_TEXT_RANGE r = { 0, len };

    *layout = NULL;

    This = heap_alloc(sizeof(struct dwrite_textlayout));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteTextLayout2_iface.lpVtbl = &dwritetextlayoutvtbl;
    This->ref = 1;
    This->str = heap_strdupnW(str, len);
    This->len = len;
    This->maxwidth = maxwidth;
    This->maxheight = maxheight;
    layout_format_from_textformat(This, format);

    list_init(&This->ranges);
    range = alloc_layout_range(This, &r);
    if (!range) {
        IDWriteTextLayout2_Release(&This->IDWriteTextLayout2_iface);
        return E_OUTOFMEMORY;
    }
    list_add_head(&This->ranges, &range->entry);

    *layout = (IDWriteTextLayout*)&This->IDWriteTextLayout2_iface;

    return S_OK;
}

static HRESULT WINAPI dwritetrimmingsign_QueryInterface(IDWriteInlineObject *iface, REFIID riid, void **obj)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteInlineObject)) {
        *obj = iface;
        IDWriteInlineObject_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;

}

static ULONG WINAPI dwritetrimmingsign_AddRef(IDWriteInlineObject *iface)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetrimmingsign_Release(IDWriteInlineObject *iface)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI dwritetrimmingsign_Draw(IDWriteInlineObject *iface, void *context, IDWriteTextRenderer *renderer,
    FLOAT originX, FLOAT originY, BOOL is_sideways, BOOL is_rtl, IUnknown *drawing_effect)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    FIXME("(%p)->(%p %p %f %f %d %d %p): stub\n", This, context, renderer, originX, originY, is_sideways, is_rtl, drawing_effect);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetrimmingsign_GetMetrics(IDWriteInlineObject *iface, DWRITE_INLINE_OBJECT_METRICS *metrics)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetrimmingsign_GetOverhangMetrics(IDWriteInlineObject *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);
    FIXME("(%p)->(%p): stub\n", This, overhangs);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetrimmingsign_GetBreakConditions(IDWriteInlineObject *iface, DWRITE_BREAK_CONDITION *before,
        DWRITE_BREAK_CONDITION *after)
{
    struct dwrite_trimmingsign *This = impl_from_IDWriteInlineObject(iface);

    TRACE("(%p)->(%p %p)\n", This, before, after);

    *before = *after = DWRITE_BREAK_CONDITION_NEUTRAL;
    return S_OK;
}

static const IDWriteInlineObjectVtbl dwritetrimmingsignvtbl = {
    dwritetrimmingsign_QueryInterface,
    dwritetrimmingsign_AddRef,
    dwritetrimmingsign_Release,
    dwritetrimmingsign_Draw,
    dwritetrimmingsign_GetMetrics,
    dwritetrimmingsign_GetOverhangMetrics,
    dwritetrimmingsign_GetBreakConditions
};

HRESULT create_trimmingsign(IDWriteInlineObject **sign)
{
    struct dwrite_trimmingsign *This;

    *sign = NULL;

    This = heap_alloc(sizeof(struct dwrite_trimmingsign));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteInlineObject_iface.lpVtbl = &dwritetrimmingsignvtbl;
    This->ref = 1;

    *sign = &This->IDWriteInlineObject_iface;

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_QueryInterface(IDWriteTextFormat1 *iface, REFIID riid, void **obj)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTextFormat1) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat)  ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteTextFormat1_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextformat_AddRef(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetextformat_Release(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        release_format_data(&This->format);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritetextformat_SetTextAlignment(IDWriteTextFormat1 *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, alignment);
    This->format.textalignment = alignment;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetParagraphAlignment(IDWriteTextFormat1 *iface, DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, alignment);
    This->format.paralign = alignment;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetWordWrapping(IDWriteTextFormat1 *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, wrapping);
    This->format.wrapping = wrapping;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetReadingDirection(IDWriteTextFormat1 *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, direction);
    This->format.readingdir = direction;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetFlowDirection(IDWriteTextFormat1 *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d)\n", This, direction);
    This->format.flow = direction;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetIncrementalTabStop(IDWriteTextFormat1 *iface, FLOAT tabstop)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%f): stub\n", This, tabstop);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetTrimming(IDWriteTextFormat1 *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p %p)\n", This, trimming, trimming_sign);

    This->format.trimming = *trimming;
    if (This->format.trimmingsign)
        IDWriteInlineObject_Release(This->format.trimmingsign);
    This->format.trimmingsign = trimming_sign;
    if (This->format.trimmingsign)
        IDWriteInlineObject_AddRef(This->format.trimmingsign);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_SetLineSpacing(IDWriteTextFormat1 *iface, DWRITE_LINE_SPACING_METHOD method,
    FLOAT spacing, FLOAT baseline)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%d %f %f)\n", This, method, spacing, baseline);
    This->format.spacingmethod = method;
    This->format.spacing = spacing;
    This->format.baseline = baseline;
    return S_OK;
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextformat_GetTextAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.textalignment;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextformat_GetParagraphAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.paralign;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextformat_GetWordWrapping(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.wrapping;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextformat_GetReadingDirection(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.readingdir;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextformat_GetFlowDirection(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.flow;
}

static FLOAT WINAPI dwritetextformat_GetIncrementalTabStop(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static HRESULT WINAPI dwritetextformat_GetTrimming(IDWriteTextFormat1 *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p %p)\n", This, options, trimming_sign);

    *options = This->format.trimming;
    if ((*trimming_sign = This->format.trimmingsign))
        IDWriteInlineObject_AddRef(*trimming_sign);

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_GetLineSpacing(IDWriteTextFormat1 *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)->(%p %p %p)\n", This, method, spacing, baseline);

    *method = This->format.spacingmethod;
    *spacing = This->format.spacing;
    *baseline = This->format.baseline;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_GetFontCollection(IDWriteTextFormat1 *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p)\n", This, collection);

    *collection = This->format.collection;
    IDWriteFontCollection_AddRef(*collection);

    return S_OK;
}

static UINT32 WINAPI dwritetextformat_GetFontFamilyNameLength(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.family_len;
}

static HRESULT WINAPI dwritetextformat_GetFontFamilyName(IDWriteTextFormat1 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.family_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.family_name);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextformat_GetFontWeight(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.weight;
}

static DWRITE_FONT_STYLE WINAPI dwritetextformat_GetFontStyle(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.style;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextformat_GetFontStretch(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.stretch;
}

static FLOAT WINAPI dwritetextformat_GetFontSize(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.fontsize;
}

static UINT32 WINAPI dwritetextformat_GetLocaleNameLength(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    TRACE("(%p)\n", This);
    return This->format.locale_len;
}

static HRESULT WINAPI dwritetextformat_GetLocaleName(IDWriteTextFormat1 *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.locale_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.locale);
    return S_OK;
}

static HRESULT WINAPI dwritetextformat1_SetVerticalGlyphOrientation(IDWriteTextFormat1 *iface, DWRITE_VERTICAL_GLYPH_ORIENTATION orientation)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, orientation);
    return E_NOTIMPL;
}

static DWRITE_VERTICAL_GLYPH_ORIENTATION WINAPI dwritetextformat1_GetVerticalGlyphOrientation(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_VERTICAL_GLYPH_ORIENTATION_DEFAULT;
}

static HRESULT WINAPI dwritetextformat1_SetLastLineWrapping(IDWriteTextFormat1 *iface, BOOL lastline_wrapping_enabled)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, lastline_wrapping_enabled);
    return E_NOTIMPL;
}

static BOOL WINAPI dwritetextformat1_GetLastLineWrapping(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritetextformat1_SetOpticalAlignment(IDWriteTextFormat1 *iface, DWRITE_OPTICAL_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static DWRITE_OPTICAL_ALIGNMENT WINAPI dwritetextformat1_GetOpticalAlignment(IDWriteTextFormat1 *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_OPTICAL_ALIGNMENT_NONE;
}

static HRESULT WINAPI dwritetextformat1_SetFontFallback(IDWriteTextFormat1 *iface, IDWriteFontFallback *fallback)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%p): stub\n", This, fallback);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat1_GetFontFallback(IDWriteTextFormat1 *iface, IDWriteFontFallback **fallback)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat1(iface);
    FIXME("(%p)->(%p): stub\n", This, fallback);
    return E_NOTIMPL;
}

static const IDWriteTextFormat1Vtbl dwritetextformatvtbl = {
    dwritetextformat_QueryInterface,
    dwritetextformat_AddRef,
    dwritetextformat_Release,
    dwritetextformat_SetTextAlignment,
    dwritetextformat_SetParagraphAlignment,
    dwritetextformat_SetWordWrapping,
    dwritetextformat_SetReadingDirection,
    dwritetextformat_SetFlowDirection,
    dwritetextformat_SetIncrementalTabStop,
    dwritetextformat_SetTrimming,
    dwritetextformat_SetLineSpacing,
    dwritetextformat_GetTextAlignment,
    dwritetextformat_GetParagraphAlignment,
    dwritetextformat_GetWordWrapping,
    dwritetextformat_GetReadingDirection,
    dwritetextformat_GetFlowDirection,
    dwritetextformat_GetIncrementalTabStop,
    dwritetextformat_GetTrimming,
    dwritetextformat_GetLineSpacing,
    dwritetextformat_GetFontCollection,
    dwritetextformat_GetFontFamilyNameLength,
    dwritetextformat_GetFontFamilyName,
    dwritetextformat_GetFontWeight,
    dwritetextformat_GetFontStyle,
    dwritetextformat_GetFontStretch,
    dwritetextformat_GetFontSize,
    dwritetextformat_GetLocaleNameLength,
    dwritetextformat_GetLocaleName,
    dwritetextformat1_SetVerticalGlyphOrientation,
    dwritetextformat1_GetVerticalGlyphOrientation,
    dwritetextformat1_SetLastLineWrapping,
    dwritetextformat1_GetLastLineWrapping,
    dwritetextformat1_SetOpticalAlignment,
    dwritetextformat1_GetOpticalAlignment,
    dwritetextformat1_SetFontFallback,
    dwritetextformat1_GetFontFallback
};

HRESULT create_textformat(const WCHAR *family_name, IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, const WCHAR *locale, IDWriteTextFormat **format)
{
    struct dwrite_textformat *This;

    *format = NULL;

    This = heap_alloc(sizeof(struct dwrite_textformat));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteTextFormat1_iface.lpVtbl = &dwritetextformatvtbl;
    This->ref = 1;
    This->format.family_name = heap_strdupW(family_name);
    This->format.family_len = strlenW(family_name);
    This->format.locale = heap_strdupW(locale);
    This->format.locale_len = strlenW(locale);
    This->format.weight = weight;
    This->format.style = style;
    This->format.fontsize = size;
    This->format.stretch = stretch;
    This->format.textalignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    This->format.paralign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    This->format.wrapping = DWRITE_WORD_WRAPPING_WRAP;
    This->format.readingdir = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    This->format.flow = DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM;
    This->format.spacingmethod = DWRITE_LINE_SPACING_METHOD_DEFAULT;
    This->format.spacing = 0.0;
    This->format.baseline = 0.0;
    This->format.trimming.granularity = DWRITE_TRIMMING_GRANULARITY_NONE;
    This->format.trimming.delimiter = 0;
    This->format.trimming.delimiterCount = 0;
    This->format.trimmingsign = NULL;
    This->format.collection = collection;
    IDWriteFontCollection_AddRef(collection);

    *format = (IDWriteTextFormat*)&This->IDWriteTextFormat1_iface;

    return S_OK;
}

static HRESULT WINAPI dwritetypography_QueryInterface(IDWriteTypography *iface, REFIID riid, void **obj)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("(%p)->(%s %p)\n", typography, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTypography) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteTypography_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetypography_AddRef(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    ULONG ref = InterlockedIncrement(&typography->ref);
    TRACE("(%p)->(%d)\n", typography, ref);
    return ref;
}

static ULONG WINAPI dwritetypography_Release(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    ULONG ref = InterlockedDecrement(&typography->ref);

    TRACE("(%p)->(%d)\n", typography, ref);

    if (!ref) {
        heap_free(typography->features);
        heap_free(typography);
    }

    return ref;
}

static HRESULT WINAPI dwritetypography_AddFontFeature(IDWriteTypography *iface, DWRITE_FONT_FEATURE feature)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("(%p)->(%x %u)\n", typography, feature.nameTag, feature.parameter);

    if (typography->count == typography->allocated) {
        DWRITE_FONT_FEATURE *ptr = heap_realloc(typography->features, 2*typography->allocated*sizeof(DWRITE_FONT_FEATURE));
        if (!ptr)
            return E_OUTOFMEMORY;

        typography->features = ptr;
        typography->allocated *= 2;
    }

    typography->features[typography->count++] = feature;
    return S_OK;
}

static UINT32 WINAPI dwritetypography_GetFontFeatureCount(IDWriteTypography *iface)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);
    TRACE("(%p)\n", typography);
    return typography->count;
}

static HRESULT WINAPI dwritetypography_GetFontFeature(IDWriteTypography *iface, UINT32 index, DWRITE_FONT_FEATURE *feature)
{
    struct dwrite_typography *typography = impl_from_IDWriteTypography(iface);

    TRACE("(%p)->(%u %p)\n", typography, index, feature);

    if (index >= typography->count)
        return E_INVALIDARG;

    *feature = typography->features[index];
    return S_OK;
}

static const IDWriteTypographyVtbl dwritetypographyvtbl = {
    dwritetypography_QueryInterface,
    dwritetypography_AddRef,
    dwritetypography_Release,
    dwritetypography_AddFontFeature,
    dwritetypography_GetFontFeatureCount,
    dwritetypography_GetFontFeature
};

HRESULT create_typography(IDWriteTypography **ret)
{
    struct dwrite_typography *typography;

    *ret = NULL;

    typography = heap_alloc(sizeof(*typography));
    if (!typography)
        return E_OUTOFMEMORY;

    typography->IDWriteTypography_iface.lpVtbl = &dwritetypographyvtbl;
    typography->ref = 1;
    typography->allocated = 2;
    typography->count = 0;

    typography->features = heap_alloc(typography->allocated*sizeof(DWRITE_FONT_FEATURE));
    if (!typography->features) {
        heap_free(typography);
        return E_OUTOFMEMORY;
    }

    *ret = &typography->IDWriteTypography_iface;
    return S_OK;
}
