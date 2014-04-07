/*
 *    Text format and layout
 *
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "dwrite.h"
#include "dwrite_private.h"

#include "wine/debug.h"

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
    FLOAT size;

    IDWriteFontCollection *collection;
};

struct dwrite_textlayout {
    IDWriteTextLayout IDWriteTextLayout_iface;
    LONG ref;

    WCHAR *str;
    UINT32 len;
    struct dwrite_textformat_data format;
};

struct dwrite_textformat {
    IDWriteTextFormat IDWriteTextFormat_iface;
    LONG ref;
    struct dwrite_textformat_data format;
};

static const IDWriteTextFormatVtbl dwritetextformatvtbl;

static void release_format_data(struct dwrite_textformat_data *data)
{
    if (data->collection) IDWriteFontCollection_Release(data->collection);
    heap_free(data->family_name);
    heap_free(data->locale);
}

static inline struct dwrite_textlayout *impl_from_IDWriteTextLayout(IDWriteTextLayout *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textlayout, IDWriteTextLayout_iface);
}

static inline struct dwrite_textformat *impl_from_IDWriteTextFormat(IDWriteTextFormat *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_textformat, IDWriteTextFormat_iface);
}

static inline struct dwrite_textformat *unsafe_impl_from_IDWriteTextFormat(IDWriteTextFormat *iface)
{
    return iface->lpVtbl == &dwritetextformatvtbl ? impl_from_IDWriteTextFormat(iface) : NULL;
}

static HRESULT WINAPI dwritetextlayout_QueryInterface(IDWriteTextLayout *iface, REFIID riid, void **obj)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat) ||
        IsEqualIID(riid, &IID_IDWriteTextLayout))
    {
        *obj = iface;
        IDWriteTextLayout_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextlayout_AddRef(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetextlayout_Release(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        release_format_data(&This->format);
        heap_free(This->str);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritetextlayout_SetTextAlignment(IDWriteTextLayout *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetParagraphAlignment(IDWriteTextLayout *iface, DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetWordWrapping(IDWriteTextLayout *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d): stub\n", This, wrapping);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetReadingDirection(IDWriteTextLayout *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFlowDirection(IDWriteTextLayout *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetIncrementalTabStop(IDWriteTextLayout *iface, FLOAT tabstop)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%f): stub\n", This, tabstop);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetTrimming(IDWriteTextLayout *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %p): stub\n", This, trimming, trimming_sign);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetLineSpacing(IDWriteTextLayout *iface, DWRITE_LINE_SPACING_METHOD spacing,
    FLOAT line_spacing, FLOAT baseline)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d %f %f): stub\n", This, spacing, line_spacing, baseline);
    return E_NOTIMPL;
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextlayout_GetTextAlignment(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_TEXT_ALIGNMENT_LEADING;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextlayout_GetParagraphAlignment(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextlayout_GetWordWrapping(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_WORD_WRAPPING_NO_WRAP;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextlayout_GetReadingDirection(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextlayout_GetFlowDirection(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM;
}

static FLOAT WINAPI dwritetextlayout_GetIncrementalTabStop(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static HRESULT WINAPI dwritetextlayout_GetTrimming(IDWriteTextLayout *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %p): stub\n", This, options, trimming_sign);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetLineSpacing(IDWriteTextLayout *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %p %p): stub\n", This, method, spacing, baseline);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetFontCollection(IDWriteTextLayout *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p): stub\n", This, collection);
    return E_NOTIMPL;
}

static UINT32 WINAPI dwritetextlayout_GetFontFamilyNameLength(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static HRESULT WINAPI dwritetextlayout_GetFontFamilyName(IDWriteTextLayout *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %u): stub\n", This, name, size);
    return E_NOTIMPL;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextlayout_GetFontWeight(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FONT_WEIGHT_NORMAL;
}

static DWRITE_FONT_STYLE WINAPI dwritetextlayout_GetFontStyle(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FONT_STYLE_NORMAL;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextlayout_GetFontStretch(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FONT_STRETCH_NORMAL;
}

static FLOAT WINAPI dwritetextlayout_GetFontSize(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static UINT32 WINAPI dwritetextlayout_GetLocaleNameLength(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    TRACE("(%p)\n", This);
    return This->format.locale_len;
}

static HRESULT WINAPI dwritetextlayout_GetLocaleName(IDWriteTextLayout *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.locale_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.locale);
    return S_OK;
}

static HRESULT WINAPI dwritetextlayout_SetMaxWidth(IDWriteTextLayout *iface, FLOAT maxWidth)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%f): stub\n", This, maxWidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetMaxHeight(IDWriteTextLayout *iface, FLOAT maxHeight)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%f): stub\n", This, maxHeight);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFontCollection(IDWriteTextLayout *iface, IDWriteFontCollection* collection, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %u:%u): stub\n", This, collection, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFontFamilyName(IDWriteTextLayout *iface, WCHAR const *name, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%s %u:%u): stub\n", This, debugstr_w(name), range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFontWeight(IDWriteTextLayout *iface, DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d %u:%u): stub\n", This, weight, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFontStyle(IDWriteTextLayout *iface, DWRITE_FONT_STYLE style, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d %u:%u): stub\n", This, style, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFontStretch(IDWriteTextLayout *iface, DWRITE_FONT_STRETCH stretch, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d %u:%u): stub\n", This, stretch, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetFontSize(IDWriteTextLayout *iface, FLOAT size, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%f %u:%u): stub\n", This, size, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetUnderline(IDWriteTextLayout *iface, BOOL underline, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d %u:%u): stub\n", This, underline, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetStrikethrough(IDWriteTextLayout *iface, BOOL strikethrough, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d %u:%u): stub\n", This, strikethrough, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetDrawingEffect(IDWriteTextLayout *iface, IUnknown* effect, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %u:%u): stub\n", This, effect, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetInlineObject(IDWriteTextLayout *iface, IDWriteInlineObject *object, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %u:%u): stub\n", This, object, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetTypography(IDWriteTextLayout *iface, IDWriteTypography* typography, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %u:%u): stub\n", This, typography, range.startPosition, range.length);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_SetLocaleName(IDWriteTextLayout *iface, WCHAR const* locale, DWRITE_TEXT_RANGE range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%s %u:%u): stub\n", This, debugstr_w(locale), range.startPosition, range.length);
    return E_NOTIMPL;
}

static FLOAT WINAPI dwritetextlayout_GetMaxWidth(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static FLOAT WINAPI dwritetextlayout_GetMaxHeight(IDWriteTextLayout *iface)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontCollection(IDWriteTextLayout *iface, UINT32 pos,
    IDWriteFontCollection** collection, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %p): stub\n", This, collection, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyNameLength(IDWriteTextLayout *iface,
    UINT32 pos, UINT32* len, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%d %p %p): stub\n", This, pos, len, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontFamilyName(IDWriteTextLayout *iface,
    UINT32 position, WCHAR* name, UINT32 name_size, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %u %p): stub\n", This, position, name, name_size, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontWeight(IDWriteTextLayout *iface,
    UINT32 position, DWRITE_FONT_WEIGHT *weight, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, weight, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStyle(IDWriteTextLayout *iface,
    UINT32 currentPosition, DWRITE_FONT_STYLE *style, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, currentPosition, style, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontStretch(IDWriteTextLayout *iface,
    UINT32 position, DWRITE_FONT_STRETCH *stretch, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, stretch, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetFontSize(IDWriteTextLayout *iface,
    UINT32 position, FLOAT *size, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, size, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetUnderline(IDWriteTextLayout *iface,
    UINT32 position, BOOL *has_underline, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, has_underline, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetStrikethrough(IDWriteTextLayout *iface,
    UINT32 position, BOOL *has_strikethrough, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, has_strikethrough, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetDrawingEffect(IDWriteTextLayout *iface,
    UINT32 position, IUnknown **effect, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, effect, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetInlineObject(IDWriteTextLayout *iface,
    UINT32 position, IDWriteInlineObject **object, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, object, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetTypography(IDWriteTextLayout *iface,
    UINT32 position, IDWriteTypography** typography, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, typography, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleNameLength(IDWriteTextLayout *iface,
    UINT32 position, UINT32* length, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %p): stub\n", This, position, length, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_layout_GetLocaleName(IDWriteTextLayout *iface,
    UINT32 position, WCHAR* name, UINT32 name_size, DWRITE_TEXT_RANGE *range)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %p %u %p): stub\n", This, position, name, name_size, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_Draw(IDWriteTextLayout *iface,
    void *context, IDWriteTextRenderer* renderer, FLOAT originX, FLOAT originY)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %p %f %f): stub\n", This, context, renderer, originX, originY);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetLineMetrics(IDWriteTextLayout *iface,
    DWRITE_LINE_METRICS *metrics, UINT32 max_count, UINT32 *actual_count)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %u %p): stub\n", This, metrics, max_count, actual_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetMetrics(IDWriteTextLayout *iface, DWRITE_TEXT_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetOverhangMetrics(IDWriteTextLayout *iface, DWRITE_OVERHANG_METRICS *overhangs)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p): stub\n", This, overhangs);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_GetClusterMetrics(IDWriteTextLayout *iface,
    DWRITE_CLUSTER_METRICS *metrics, UINT32 max_count, UINT32* act_count)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p %u %p): stub\n", This, metrics, max_count, act_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_DetermineMinWidth(IDWriteTextLayout *iface, FLOAT* min_width)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%p): stub\n", This, min_width);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestPoint(IDWriteTextLayout *iface,
    FLOAT pointX, FLOAT pointY, BOOL* is_trailinghit, BOOL* is_inside, DWRITE_HIT_TEST_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%f %f %p %p %p): stub\n", This, pointX, pointY, is_trailinghit, is_inside, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextPosition(IDWriteTextLayout *iface,
    UINT32 textPosition, BOOL is_trailinghit, FLOAT* pointX, FLOAT* pointY, DWRITE_HIT_TEST_METRICS *metrics)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %d %p %p %p): stub\n", This, textPosition, is_trailinghit, pointX, pointY, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextlayout_HitTestTextRange(IDWriteTextLayout *iface,
    UINT32 textPosition, UINT32 textLength, FLOAT originX, FLOAT originY,
    DWRITE_HIT_TEST_METRICS *metrics, UINT32 max_metricscount, UINT32* actual_metricscount)
{
    struct dwrite_textlayout *This = impl_from_IDWriteTextLayout(iface);
    FIXME("(%p)->(%u %u %f %f %p %u %p): stub\n", This, textPosition, textLength, originX, originY, metrics,
        max_metricscount, actual_metricscount);
    return E_NOTIMPL;
}

static const IDWriteTextLayoutVtbl dwritetextlayoutvtbl = {
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
    dwritetextlayout_HitTestTextRange
};

static void layout_format_from_textformat(struct dwrite_textlayout *layout, IDWriteTextFormat *format)
{
    struct dwrite_textformat *f;

    memset(&layout->format, 0, sizeof(layout->format));

    if ((f = unsafe_impl_from_IDWriteTextFormat(format)))
    {
        layout->format = f->format;
        layout->format.locale = heap_strdupW(f->format.locale);
        layout->format.family_name = heap_strdupW(f->format.family_name);
    }
    else
    {
        UINT32 locale_len, family_len;

        layout->format.weight  = IDWriteTextFormat_GetFontWeight(format);
        layout->format.style   = IDWriteTextFormat_GetFontStyle(format);
        layout->format.stretch = IDWriteTextFormat_GetFontStretch(format);
        layout->format.size    = IDWriteTextFormat_GetFontSize(format);
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

HRESULT create_textlayout(const WCHAR *str, UINT32 len, IDWriteTextFormat *format, IDWriteTextLayout **layout)
{
    struct dwrite_textlayout *This;

    *layout = NULL;

    This = heap_alloc(sizeof(struct dwrite_textlayout));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteTextLayout_iface.lpVtbl = &dwritetextlayoutvtbl;
    This->ref = 1;
    This->str = heap_strdupnW(str, len);
    This->len = len;
    layout_format_from_textformat(This, format);

    *layout = &This->IDWriteTextLayout_iface;

    return S_OK;
}

static HRESULT WINAPI dwritetextformat_QueryInterface(IDWriteTextFormat *iface, REFIID riid, void **obj)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteTextFormat))
    {
        *obj = iface;
        IDWriteTextFormat_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextformat_AddRef(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritetextformat_Release(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        release_format_data(&This->format);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritetextformat_SetTextAlignment(IDWriteTextFormat *iface, DWRITE_TEXT_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetParagraphAlignment(IDWriteTextFormat *iface, DWRITE_PARAGRAPH_ALIGNMENT alignment)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%d): stub\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetWordWrapping(IDWriteTextFormat *iface, DWRITE_WORD_WRAPPING wrapping)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%d): stub\n", This, wrapping);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetReadingDirection(IDWriteTextFormat *iface, DWRITE_READING_DIRECTION direction)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetFlowDirection(IDWriteTextFormat *iface, DWRITE_FLOW_DIRECTION direction)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%d): stub\n", This, direction);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetIncrementalTabStop(IDWriteTextFormat *iface, FLOAT tabstop)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%f): stub\n", This, tabstop);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetTrimming(IDWriteTextFormat *iface, DWRITE_TRIMMING const *trimming,
    IDWriteInlineObject *trimming_sign)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%p %p): stub\n", This, trimming, trimming_sign);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_SetLineSpacing(IDWriteTextFormat *iface, DWRITE_LINE_SPACING_METHOD spacing,
    FLOAT line_spacing, FLOAT baseline)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%d %f %f): stub\n", This, spacing, line_spacing, baseline);
    return E_NOTIMPL;
}

static DWRITE_TEXT_ALIGNMENT WINAPI dwritetextformat_GetTextAlignment(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.textalignment;
}

static DWRITE_PARAGRAPH_ALIGNMENT WINAPI dwritetextformat_GetParagraphAlignment(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.paralign;
}

static DWRITE_WORD_WRAPPING WINAPI dwritetextformat_GetWordWrapping(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.wrapping;
}

static DWRITE_READING_DIRECTION WINAPI dwritetextformat_GetReadingDirection(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.readingdir;
}

static DWRITE_FLOW_DIRECTION WINAPI dwritetextformat_GetFlowDirection(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.flow;
}

static FLOAT WINAPI dwritetextformat_GetIncrementalTabStop(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p): stub\n", This);
    return 0.0;
}

static HRESULT WINAPI dwritetextformat_GetTrimming(IDWriteTextFormat *iface, DWRITE_TRIMMING *options,
    IDWriteInlineObject **trimming_sign)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    FIXME("(%p)->(%p %p): stub\n", This, options, trimming_sign);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextformat_GetLineSpacing(IDWriteTextFormat *iface, DWRITE_LINE_SPACING_METHOD *method,
    FLOAT *spacing, FLOAT *baseline)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)->(%p %p %p)\n", This, method, spacing, baseline);

    *method = This->format.spacingmethod;
    *spacing = This->format.spacing;
    *baseline = This->format.baseline;
    return S_OK;
}

static HRESULT WINAPI dwritetextformat_GetFontCollection(IDWriteTextFormat *iface, IDWriteFontCollection **collection)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);

    TRACE("(%p)->(%p)\n", This, collection);

    *collection = This->format.collection;
    IDWriteFontCollection_AddRef(*collection);

    return S_OK;
}

static UINT32 WINAPI dwritetextformat_GetFontFamilyNameLength(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.family_len;
}

static HRESULT WINAPI dwritetextformat_GetFontFamilyName(IDWriteTextFormat *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.family_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.family_name);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritetextformat_GetFontWeight(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.weight;
}

static DWRITE_FONT_STYLE WINAPI dwritetextformat_GetFontStyle(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.style;
}

static DWRITE_FONT_STRETCH WINAPI dwritetextformat_GetFontStretch(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.stretch;
}

static FLOAT WINAPI dwritetextformat_GetFontSize(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.size;
}

static UINT32 WINAPI dwritetextformat_GetLocaleNameLength(IDWriteTextFormat *iface)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);
    TRACE("(%p)\n", This);
    return This->format.locale_len;
}

static HRESULT WINAPI dwritetextformat_GetLocaleName(IDWriteTextFormat *iface, WCHAR *name, UINT32 size)
{
    struct dwrite_textformat *This = impl_from_IDWriteTextFormat(iface);

    TRACE("(%p)->(%p %u)\n", This, name, size);

    if (size <= This->format.locale_len) return E_NOT_SUFFICIENT_BUFFER;
    strcpyW(name, This->format.locale);
    return S_OK;
}

static const IDWriteTextFormatVtbl dwritetextformatvtbl = {
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
    dwritetextformat_GetLocaleName
};

HRESULT create_textformat(const WCHAR *family_name, IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, const WCHAR *locale, IDWriteTextFormat **format)
{
    struct dwrite_textformat *This;

    *format = NULL;

    This = heap_alloc(sizeof(struct dwrite_textformat));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteTextFormat_iface.lpVtbl = &dwritetextformatvtbl;
    This->ref = 1;
    This->format.family_name = heap_strdupW(family_name);
    This->format.family_len = strlenW(family_name);
    This->format.locale = heap_strdupW(locale);
    This->format.locale_len = strlenW(locale);
    This->format.weight = weight;
    This->format.style = style;
    This->format.size = size;
    This->format.stretch = stretch;
    This->format.textalignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    This->format.paralign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    This->format.wrapping = DWRITE_WORD_WRAPPING_WRAP;
    This->format.readingdir = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    This->format.flow = DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM;
    This->format.spacingmethod = DWRITE_LINE_SPACING_METHOD_DEFAULT;
    This->format.spacing = 0.0;
    This->format.baseline = 0.0;

    if (collection)
    {
        This->format.collection = collection;
        IDWriteFontCollection_AddRef(collection);
    }
    else
    {
        HRESULT hr = get_system_fontcollection(&This->format.collection);
        if (hr != S_OK)
        {
            IDWriteTextFormat_Release(&This->IDWriteTextFormat_iface);
            return hr;
        }
    }

    *format = &This->IDWriteTextFormat_iface;

    return S_OK;
}
