/*
 *    Font and collections
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

#include "dwrite.h"
#include "dwrite_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

struct dwrite_fontfamily {
    IDWriteFontFamily IDWriteFontFamily_iface;
    LONG ref;
};

struct dwrite_font {
    IDWriteFont IDWriteFont_iface;
    LONG ref;

    IDWriteFontFamily *family;
    DWRITE_FONT_STYLE style;
};

static inline struct dwrite_font *impl_from_IDWriteFont(IDWriteFont *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_font, IDWriteFont_iface);
}

static inline struct dwrite_fontfamily *impl_from_IDWriteFontFamily(IDWriteFontFamily *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfamily, IDWriteFontFamily_iface);
}

static HRESULT WINAPI dwritefont_QueryInterface(IDWriteFont *iface, REFIID riid, void **obj)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFont))
    {
        *obj = iface;
        IDWriteFont_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefont_AddRef(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefont_Release(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IDWriteFontFamily_Release(This->family);
        heap_free(This);
    }

    return S_OK;
}

static HRESULT WINAPI dwritefont_GetFontFamily(IDWriteFont *iface, IDWriteFontFamily **family)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    TRACE("(%p)->(%p)\n", This, family);

    *family = This->family;
    IDWriteFontFamily_AddRef(*family);
    return S_OK;
}

static DWRITE_FONT_WEIGHT WINAPI dwritefont_GetWeight(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static DWRITE_FONT_STRETCH WINAPI dwritefont_GetStretch(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FONT_STRETCH_UNDEFINED;
}

static DWRITE_FONT_STYLE WINAPI dwritefont_GetStyle(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    TRACE("(%p)\n", This);
    return This->style;
}

static BOOL WINAPI dwritefont_IsSymbolFont(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static HRESULT WINAPI dwritefont_GetFaceNames(IDWriteFont *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p)->(%p): stub\n", This, names);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefont_GetInformationalStrings(IDWriteFont *iface,
    DWRITE_INFORMATIONAL_STRING_ID stringid, IDWriteLocalizedStrings **strings, BOOL *exists)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p)->(%d %p %p): stub\n", This, stringid, strings, exists);
    return E_NOTIMPL;
}

static DWRITE_FONT_SIMULATIONS WINAPI dwritefont_GetSimulations(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FONT_SIMULATIONS_NONE;
}

static void WINAPI dwritefont_GetMetrics(IDWriteFont *iface, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
}

static HRESULT WINAPI dwritefont_HasCharacter(IDWriteFont *iface, UINT32 value, BOOL *exists)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p)->(0x%08x %p): stub\n", This, value, exists);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefont_CreateFontFace(IDWriteFont *iface, IDWriteFontFace **face)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    FIXME("(%p)->(%p): stub\n", This, face);
    return E_NOTIMPL;
}

static const IDWriteFontVtbl dwritefontvtbl = {
    dwritefont_QueryInterface,
    dwritefont_AddRef,
    dwritefont_Release,
    dwritefont_GetFontFamily,
    dwritefont_GetWeight,
    dwritefont_GetStretch,
    dwritefont_GetStyle,
    dwritefont_IsSymbolFont,
    dwritefont_GetFaceNames,
    dwritefont_GetInformationalStrings,
    dwritefont_GetSimulations,
    dwritefont_GetMetrics,
    dwritefont_HasCharacter,
    dwritefont_CreateFontFace
};

static HRESULT WINAPI dwritefontfamily_QueryInterface(IDWriteFontFamily *iface, REFIID riid, void **obj)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteFontList) ||
        IsEqualIID(riid, &IID_IDWriteFontFamily))
    {
        *obj = iface;
        IDWriteFontFamily_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontfamily_AddRef(IDWriteFontFamily *iface)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefontfamily_Release(IDWriteFontFamily *iface)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return S_OK;
}

static HRESULT WINAPI dwritefontfamily_GetFontCollection(IDWriteFontFamily *iface, IDWriteFontCollection **collection)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    FIXME("(%p)->(%p): stub\n", This, collection);
    return E_NOTIMPL;
}

static UINT32 WINAPI dwritefontfamily_GetFontCount(IDWriteFontFamily *iface)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static HRESULT WINAPI dwritefontfamily_GetFont(IDWriteFontFamily *iface, UINT32 index, IDWriteFont **font)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    FIXME("(%p)->(%u %p): stub\n", This, index, font);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontfamily_GetFamilyNames(IDWriteFontFamily *iface, IDWriteLocalizedStrings **names)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    FIXME("(%p)->(%p): stub\n", This, names);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontfamily_GetFirstMatchingFont(IDWriteFontFamily *iface, DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFont **font)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    FIXME("(%p)->(%d %d %d %p): stub\n", This, weight, stretch, style, font);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontfamily_GetMatchingFonts(IDWriteFontFamily *iface, DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFontList **fonts)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    FIXME("(%p)->(%d %d %d %p): stub\n", This, weight, stretch, style, fonts);
    return E_NOTIMPL;
}

static const IDWriteFontFamilyVtbl fontfamilyvtbl = {
    dwritefontfamily_QueryInterface,
    dwritefontfamily_AddRef,
    dwritefontfamily_Release,
    dwritefontfamily_GetFontCollection,
    dwritefontfamily_GetFontCount,
    dwritefontfamily_GetFont,
    dwritefontfamily_GetFamilyNames,
    dwritefontfamily_GetFirstMatchingFont,
    dwritefontfamily_GetMatchingFonts
};

static HRESULT create_fontfamily(IDWriteFontFamily **family)
{
    struct dwrite_fontfamily *This;

    *family = NULL;

    This = heap_alloc(sizeof(struct dwrite_fontfamily));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFamily_iface.lpVtbl = &fontfamilyvtbl;
    This->ref = 1;

    *family = &This->IDWriteFontFamily_iface;

    return S_OK;
}

HRESULT create_font_from_logfont(const LOGFONTW *logfont, IDWriteFont **font)
{
    struct dwrite_font *This;
    IDWriteFontFamily *family;
    HRESULT hr;

    *font = NULL;

    hr = create_fontfamily(&family);
    if (hr != S_OK) return hr;

    This = heap_alloc(sizeof(struct dwrite_font));
    if (!This)
    {
        IDWriteFontFamily_Release(family);
        return E_OUTOFMEMORY;
    }

    This->IDWriteFont_iface.lpVtbl = &dwritefontvtbl;
    This->ref = 1;
    This->family = family;
    This->style = logfont->lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

    *font = &This->IDWriteFont_iface;

    return S_OK;
}
