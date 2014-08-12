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

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

/* PANOSE is 10 bytes in size, need to pack the structure properly */
#include "pshpack2.h"
typedef struct
{
    ULONG version;
    ULONG revision;
    ULONG checksumadj;
    ULONG magic;
    USHORT flags;
    USHORT unitsPerEm;
    ULONGLONG created;
    ULONGLONG modified;
    SHORT xMin;
    SHORT yMin;
    SHORT xMax;
    SHORT yMax;
    USHORT macStyle;
    USHORT lowestRecPPEM;
    SHORT direction_hint;
    SHORT index_format;
    SHORT glyphdata_format;
} TT_HEAD;

typedef struct
{
    ULONG Version;
    ULONG italicAngle;
    SHORT underlinePosition;
    SHORT underlineThickness;
    ULONG fixed_pitch;
    ULONG minmemType42;
    ULONG maxmemType42;
    ULONG minmemType1;
    ULONG maxmemType1;
} TT_POST;

typedef struct
{
    USHORT version;
    SHORT xAvgCharWidth;
    USHORT usWeightClass;
    USHORT usWidthClass;
    SHORT fsType;
    SHORT ySubscriptXSize;
    SHORT ySubscriptYSize;
    SHORT ySubscriptXOffset;
    SHORT ySubscriptYOffset;
    SHORT ySuperscriptXSize;
    SHORT ySuperscriptYSize;
    SHORT ySuperscriptXOffset;
    SHORT ySuperscriptYOffset;
    SHORT yStrikeoutSize;
    SHORT yStrikeoutPosition;
    SHORT sFamilyClass;
    PANOSE panose;
    ULONG ulUnicodeRange1;
    ULONG ulUnicodeRange2;
    ULONG ulUnicodeRange3;
    ULONG ulUnicodeRange4;
    CHAR achVendID[4];
    USHORT fsSelection;
    USHORT usFirstCharIndex;
    USHORT usLastCharIndex;
    /* According to the Apple spec, original version didn't have the below fields,
     * version numbers were taken from the OpenType spec.
     */
    /* version 0 (TrueType 1.5) */
    USHORT sTypoAscender;
    USHORT sTypoDescender;
    USHORT sTypoLineGap;
    USHORT usWinAscent;
    USHORT usWinDescent;
    /* version 1 (TrueType 1.66) */
    ULONG ulCodePageRange1;
    ULONG ulCodePageRange2;
    /* version 2 (OpenType 1.2) */
    SHORT sxHeight;
    SHORT sCapHeight;
    USHORT usDefaultChar;
    USHORT usBreakChar;
    USHORT usMaxContext;
} TT_OS2_V2;
#include "poppack.h"

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define GET_BE_DWORD(x) MAKELONG(GET_BE_WORD(HIWORD(x)), GET_BE_WORD(LOWORD(x)));
#endif

#define MS_MAKE_TAG(ch0, ch1, ch2, ch3) \
                    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
                    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

#define MS_HEAD_TAG MS_MAKE_TAG('h','e','a','d')
#define MS_OS2_TAG  MS_MAKE_TAG('O','S','/','2')
#define MS_POST_TAG MS_MAKE_TAG('p','o','s','t')

struct dwrite_fontcollection {
    IDWriteFontCollection IDWriteFontCollection_iface;

    WCHAR **families;
    UINT32 count;
    int alloc;
};

static IDWriteFontCollection *system_collection;

struct dwrite_fontfamily {
    IDWriteFontFamily IDWriteFontFamily_iface;
    LONG ref;

    WCHAR *familyname;
};

struct dwrite_font {
    IDWriteFont IDWriteFont_iface;
    LONG ref;

    IDWriteFontFamily *family;
    IDWriteFontFace *face;
    DWRITE_FONT_STYLE style;
    DWRITE_FONT_STRETCH stretch;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_METRICS metrics;
    WCHAR *facename;
};

struct dwrite_fontface {
    IDWriteFontFace IDWriteFontFace_iface;
    LONG ref;

    LOGFONTW logfont;
};

struct dwrite_fontfile {
    IDWriteFontFile IDWriteFontFile_iface;
    LONG ref;

    IDWriteFontFileLoader *loader;
    void *reference_key;
    UINT32 key_size;
};

static HRESULT create_fontfamily(const WCHAR *familyname, IDWriteFontFamily **family);

static inline struct dwrite_fontface *impl_from_IDWriteFontFace(IDWriteFontFace *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontface, IDWriteFontFace_iface);
}

static inline struct dwrite_font *impl_from_IDWriteFont(IDWriteFont *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_font, IDWriteFont_iface);
}

static inline struct dwrite_fontfile *impl_from_IDWriteFontFile(IDWriteFontFile *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfile, IDWriteFontFile_iface);
}

static inline struct dwrite_fontfamily *impl_from_IDWriteFontFamily(IDWriteFontFamily *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfamily, IDWriteFontFamily_iface);
}

static inline struct dwrite_fontcollection *impl_from_IDWriteFontCollection(IDWriteFontCollection *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontcollection, IDWriteFontCollection_iface);
}

static HRESULT WINAPI dwritefontface_QueryInterface(IDWriteFontFace *iface, REFIID riid, void **obj)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFace))
    {
        *obj = iface;
        IDWriteFontFace_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontface_AddRef(IDWriteFontFace *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefontface_Release(IDWriteFontFace *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static DWRITE_FONT_FACE_TYPE WINAPI dwritefontface_GetType(IDWriteFontFace *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FONT_FACE_TYPE_UNKNOWN;
}

static HRESULT WINAPI dwritefontface_GetFiles(IDWriteFontFace *iface, UINT32 *number_of_files,
    IDWriteFontFile **fontfiles)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%p %p): stub\n", This, number_of_files, fontfiles);
    return E_NOTIMPL;
}

static UINT32 WINAPI dwritefontface_GetIndex(IDWriteFontFace *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static DWRITE_FONT_SIMULATIONS WINAPI dwritefontface_GetSimulations(IDWriteFontFace *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p): stub\n", This);
    return DWRITE_FONT_SIMULATIONS_NONE;
}

static BOOL WINAPI dwritefontface_IsSymbolFont(IDWriteFontFace *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p): stub\n", This);
    return FALSE;
}

static void WINAPI dwritefontface_GetMetrics(IDWriteFontFace *iface, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%p): stub\n", This, metrics);
}

static UINT16 WINAPI dwritefontface_GetGlyphCount(IDWriteFontFace *iface)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static HRESULT WINAPI dwritefontface_GetDesignGlyphMetrics(IDWriteFontFace *iface,
    UINT16 const *glyph_indices, UINT32 glyph_count, DWRITE_GLYPH_METRICS *metrics, BOOL is_sideways)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%p %u %p %d): stub\n", This, glyph_indices, glyph_count, metrics, is_sideways);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_GetGlyphIndices(IDWriteFontFace *iface, UINT32 const *codepoints,
    UINT32 count, UINT16 *glyph_indices)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    HFONT hfont;
    WCHAR *str;
    HDC hdc;
    unsigned int i;

    TRACE("(%p)->(%p %u %p)\n", This, codepoints, count, glyph_indices);

    str = heap_alloc(count*sizeof(WCHAR));
    if (!str) return E_OUTOFMEMORY;

    for (i = 0; i < count; i++)
        str[i] = codepoints[i] < 0x10000 ? codepoints[i] : '?';

    hdc = CreateCompatibleDC(0);
    hfont = CreateFontIndirectW(&This->logfont);
    SelectObject(hdc, hfont);

    GetGlyphIndicesW(hdc, str, count, glyph_indices, 0);
    heap_free(str);

    DeleteDC(hdc);
    DeleteObject(hfont);

    return S_OK;
}

static HRESULT WINAPI dwritefontface_TryGetFontTable(IDWriteFontFace *iface, UINT32 table_tag,
    const void **table_data, UINT32 *table_size, void **context, BOOL *exists)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%u %p %p %p %p): stub\n", This, table_tag, table_data, table_size, context, exists);
    return E_NOTIMPL;
}

static void WINAPI dwritefontface_ReleaseFontTable(IDWriteFontFace *iface, void *table_context)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%p): stub\n", This, table_context);
}

static HRESULT WINAPI dwritefontface_GetGlyphRunOutline(IDWriteFontFace *iface, FLOAT emSize,
    UINT16 const *glyph_indices, FLOAT const* glyph_advances, DWRITE_GLYPH_OFFSET const *glyph_offsets,
    UINT32 glyph_count, BOOL is_sideways, BOOL is_rtl, IDWriteGeometrySink *geometrysink)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%f %p %p %p %u %d %d %p): stub\n", This, emSize, glyph_indices, glyph_advances, glyph_offsets,
        glyph_count, is_sideways, is_rtl, geometrysink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_GetRecommendedRenderingMode(IDWriteFontFace *iface, FLOAT emSize,
    FLOAT pixels_per_dip, DWRITE_MEASURING_MODE mode, IDWriteRenderingParams* params, DWRITE_RENDERING_MODE* rendering_mode)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%f %f %d %p %p): stub\n", This, emSize, pixels_per_dip, mode, params, rendering_mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_GetGdiCompatibleMetrics(IDWriteFontFace *iface, FLOAT emSize, FLOAT pixels_per_dip,
    DWRITE_MATRIX const *transform, DWRITE_FONT_METRICS *metrics)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%f %f %p %p): stub\n", This, emSize, pixels_per_dip, transform, metrics);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefontface_GetGdiCompatibleGlyphMetrics(IDWriteFontFace *iface, FLOAT emSize, FLOAT pixels_per_dip,
    DWRITE_MATRIX const *transform, BOOL use_gdi_natural, UINT16 const *glyph_indices, UINT32 glyph_count,
    DWRITE_GLYPH_METRICS *metrics, BOOL is_sideways)
{
    struct dwrite_fontface *This = impl_from_IDWriteFontFace(iface);
    FIXME("(%p)->(%f %f %p %d %p %u %p %d): stub\n", This, emSize, pixels_per_dip, transform, use_gdi_natural, glyph_indices,
        glyph_count, metrics, is_sideways);
    return E_NOTIMPL;
}

static const IDWriteFontFaceVtbl dwritefontfacevtbl = {
    dwritefontface_QueryInterface,
    dwritefontface_AddRef,
    dwritefontface_Release,
    dwritefontface_GetType,
    dwritefontface_GetFiles,
    dwritefontface_GetIndex,
    dwritefontface_GetSimulations,
    dwritefontface_IsSymbolFont,
    dwritefontface_GetMetrics,
    dwritefontface_GetGlyphCount,
    dwritefontface_GetDesignGlyphMetrics,
    dwritefontface_GetGlyphIndices,
    dwritefontface_TryGetFontTable,
    dwritefontface_ReleaseFontTable,
    dwritefontface_GetGlyphRunOutline,
    dwritefontface_GetRecommendedRenderingMode,
    dwritefontface_GetGdiCompatibleMetrics,
    dwritefontface_GetGdiCompatibleGlyphMetrics
};

static HRESULT create_fontface(struct dwrite_font *font, IDWriteFontFace **face)
{
    struct dwrite_fontface *This;

    *face = NULL;

    This = heap_alloc(sizeof(struct dwrite_fontface));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFace_iface.lpVtbl = &dwritefontfacevtbl;
    This->ref = 1;

    memset(&This->logfont, 0, sizeof(This->logfont));
    This->logfont.lfItalic = font->style == DWRITE_FONT_STYLE_ITALIC;
    /* weight values from DWRITE_FONT_WEIGHT match values used for LOGFONT */
    This->logfont.lfWeight = font->weight;
    strcpyW(This->logfont.lfFaceName, font->facename);

    *face = &This->IDWriteFontFace_iface;

    return S_OK;
}

HRESULT convert_fontface_to_logfont(IDWriteFontFace *face, LOGFONTW *logfont)
{
    struct dwrite_fontface *fontface = impl_from_IDWriteFontFace(face);

    *logfont = fontface->logfont;

    return S_OK;
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
        if (This->face) IDWriteFontFace_Release(This->face);
        IDWriteFontFamily_Release(This->family);
        heap_free(This->facename);
        heap_free(This);
    }

    return ref;
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
    TRACE("(%p)\n", This);
    return This->weight;
}

static DWRITE_FONT_STRETCH WINAPI dwritefont_GetStretch(IDWriteFont *iface)
{
    struct dwrite_font *This = impl_from_IDWriteFont(iface);
    TRACE("(%p)\n", This);
    return This->stretch;
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

    TRACE("(%p)->(%p)\n", This, metrics);
    *metrics = This->metrics;
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

    TRACE("(%p)->(%p)\n", This, face);

    if (!This->face)
    {
        HRESULT hr = create_fontface(This, &This->face);
        if (FAILED(hr)) return hr;
    }

    *face = This->face;
    IDWriteFontFace_AddRef(*face);

    return S_OK;
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
    {
        heap_free(This->familyname);
        heap_free(This);
    }

    return ref;
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
    static const WCHAR enusW[] = {'e','n','-','u','s',0};
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, names);

    hr = create_localizedstrings(names);
    if (FAILED(hr)) return hr;

    return add_localizedstring(*names, enusW, This->familyname);
}

static HRESULT WINAPI dwritefontfamily_GetFirstMatchingFont(IDWriteFontFamily *iface, DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFont **font)
{
    struct dwrite_fontfamily *This = impl_from_IDWriteFontFamily(iface);
    LOGFONTW lf;

    TRACE("(%p)->(%d %d %d %p)\n", This, weight, stretch, style, font);

    memset(&lf, 0, sizeof(lf));
    lf.lfWeight = weight;
    lf.lfItalic = style == DWRITE_FONT_STYLE_ITALIC;
    strcpyW(lf.lfFaceName, This->familyname);

    return create_font_from_logfont(&lf, font);
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

static HRESULT WINAPI dwritefontcollection_QueryInterface(IDWriteFontCollection *iface, REFIID riid, void **obj)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteFontCollection))
    {
        *obj = iface;
        IDWriteFontCollection_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritesysfontcollection_AddRef(IDWriteFontCollection *iface)
{
    return 2;
}

static ULONG WINAPI dwritesysfontcollection_Release(IDWriteFontCollection *iface)
{
    return 1;
}

static UINT32 WINAPI dwritefontcollection_GetFontFamilyCount(IDWriteFontCollection *iface)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    TRACE("(%p)\n", This);
    return This->count;
}

static HRESULT WINAPI dwritefontcollection_GetFontFamily(IDWriteFontCollection *iface, UINT32 index, IDWriteFontFamily **family)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);

    TRACE("(%p)->(%u %p)\n", This, index, family);

    if (index >= This->count)
    {
        *family = NULL;
        return E_FAIL;
    }

    return create_fontfamily(This->families[index], family);
}

static HRESULT WINAPI dwritefontcollection_FindFamilyName(IDWriteFontCollection *iface, const WCHAR *name, UINT32 *index, BOOL *exists)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    UINT32 i;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(name), index, exists);

    for (i = 0; i < This->count; i++)
        if (!strcmpW(This->families[i], name))
        {
            *index = i;
            *exists = TRUE;
            return S_OK;
        }

    *index = (UINT32)-1;
    *exists = FALSE;

    return S_OK;
}

static HRESULT WINAPI dwritefontcollection_GetFontFromFontFace(IDWriteFontCollection *iface, IDWriteFontFace *face, IDWriteFont **font)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    FIXME("(%p)->(%p %p): stub\n", This, face, font);
    return E_NOTIMPL;
}

static const IDWriteFontCollectionVtbl systemfontcollectionvtbl = {
    dwritefontcollection_QueryInterface,
    dwritesysfontcollection_AddRef,
    dwritesysfontcollection_Release,
    dwritefontcollection_GetFontFamilyCount,
    dwritefontcollection_GetFontFamily,
    dwritefontcollection_FindFamilyName,
    dwritefontcollection_GetFontFromFontFace
};

static HRESULT add_family_syscollection(struct dwrite_fontcollection *collection, const WCHAR *family)
{
    /* check for duplicate family name */
    if (collection->count && !strcmpW(collection->families[collection->count-1], family)) return S_OK;

    /* double array length */
    if (collection->count == collection->alloc)
    {
        collection->alloc *= 2;
        collection->families = heap_realloc(collection->families, collection->alloc*sizeof(WCHAR*));
    }

    collection->families[collection->count++] = heap_strdupW(family);
    TRACE("family name %s\n", debugstr_w(family));

    return S_OK;
}

static INT CALLBACK enum_font_families(const LOGFONTW *lf, const TEXTMETRICW *tm, DWORD type, LPARAM lParam)
{
    struct dwrite_fontcollection *collection = (struct dwrite_fontcollection*)lParam;
    return add_family_syscollection(collection, lf->lfFaceName) == S_OK;
}

static void release_font_collection(IDWriteFontCollection *iface)
{
    struct dwrite_fontcollection *This = impl_from_IDWriteFontCollection(iface);
    unsigned int i;

    for (i = 0; i < This->count; i++)
        heap_free(This->families[i]);
    heap_free(This->families);
    heap_free(This);
}

void release_system_fontcollection(void)
{
    if (system_collection)
        release_font_collection(system_collection);
}

HRESULT get_system_fontcollection(IDWriteFontCollection **collection)
{
    if (!system_collection)
    {
        struct dwrite_fontcollection *This;
        LOGFONTW lf;
        HDC hdc;

        *collection = NULL;

        This = heap_alloc(sizeof(struct dwrite_fontcollection));
        if (!This) return E_OUTOFMEMORY;

        This->IDWriteFontCollection_iface.lpVtbl = &systemfontcollectionvtbl;
        This->alloc = 50;
        This->count = 0;
        This->families = heap_alloc(This->alloc*sizeof(WCHAR*));

        TRACE("building system font collection:\n");

        hdc = CreateCompatibleDC(0);
        memset(&lf, 0, sizeof(lf));
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = DEFAULT_PITCH;
        lf.lfFaceName[0] = 0;
        EnumFontFamiliesExW(hdc, &lf, enum_font_families, (LPARAM)This, 0);
        DeleteDC(hdc);

        if (InterlockedCompareExchangePointer((void**)&system_collection, &This->IDWriteFontCollection_iface, NULL))
            release_font_collection(&This->IDWriteFontCollection_iface);
    }

    *collection = system_collection;

    return S_OK;
}

static HRESULT create_fontfamily(const WCHAR *familyname, IDWriteFontFamily **family)
{
    struct dwrite_fontfamily *This;

    *family = NULL;

    This = heap_alloc(sizeof(struct dwrite_fontfamily));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFamily_iface.lpVtbl = &fontfamilyvtbl;
    This->ref = 1;
    This->familyname = heap_strdupW(familyname);

    *family = &This->IDWriteFontFamily_iface;

    return S_OK;
}

static void get_font_properties(struct dwrite_font *font, HDC hdc)
{
    TT_OS2_V2 tt_os2;
    TT_HEAD tt_head;
    TT_POST tt_post;
    LONG size;

    /* default stretch and weight to normal */
    font->stretch = DWRITE_FONT_STRETCH_NORMAL;
    font->weight = DWRITE_FONT_WEIGHT_NORMAL;

    memset(&font->metrics, 0, sizeof(font->metrics));

    size = GetFontData(hdc, MS_OS2_TAG, 0, NULL, 0);
    if (size != GDI_ERROR)
    {
        if (size > sizeof(tt_os2)) size = sizeof(tt_os2);

        memset(&tt_os2, 0, sizeof(tt_os2));
        if (GetFontData(hdc, MS_OS2_TAG, 0, &tt_os2, size) != size) return;

        /* DWRITE_FONT_STRETCH enumeration values directly match font data values */
        if (GET_BE_WORD(tt_os2.usWidthClass) <= DWRITE_FONT_STRETCH_ULTRA_EXPANDED)
            font->stretch = GET_BE_WORD(tt_os2.usWidthClass);

        font->weight = GET_BE_WORD(tt_os2.usWeightClass);
        TRACE("stretch=%d, weight=%d\n", font->stretch, font->weight);

        font->metrics.ascent    = GET_BE_WORD(tt_os2.sTypoAscender);
        font->metrics.descent   = GET_BE_WORD(tt_os2.sTypoDescender);
        font->metrics.lineGap   = GET_BE_WORD(tt_os2.sTypoLineGap);
        font->metrics.capHeight = GET_BE_WORD(tt_os2.sCapHeight);
        font->metrics.xHeight   = GET_BE_WORD(tt_os2.sxHeight);
        font->metrics.strikethroughPosition  = GET_BE_WORD(tt_os2.yStrikeoutPosition);
        font->metrics.strikethroughThickness = GET_BE_WORD(tt_os2.yStrikeoutSize);
    }

    memset(&tt_head, 0, sizeof(tt_head));
    if (GetFontData(hdc, MS_HEAD_TAG, 0, &tt_head, sizeof(tt_head)) != GDI_ERROR)
    {
        font->metrics.designUnitsPerEm = GET_BE_WORD(tt_head.unitsPerEm);
    }

    memset(&tt_post, 0, sizeof(tt_post));
    if (GetFontData(hdc, MS_POST_TAG, 0, &tt_post, sizeof(tt_post)) != GDI_ERROR)
    {
        font->metrics.underlinePosition = GET_BE_WORD(tt_post.underlinePosition);
        font->metrics.underlineThickness = GET_BE_WORD(tt_post.underlineThickness);
    }
}

HRESULT create_font_from_logfont(const LOGFONTW *logfont, IDWriteFont **font)
{
    const WCHAR* facename, *familyname;
    struct dwrite_font *This;
    IDWriteFontFamily *family;
    OUTLINETEXTMETRICW *otm;
    HRESULT hr;
    HFONT hfont;
    HDC hdc;
    int ret;

    *font = NULL;

    This = heap_alloc(sizeof(struct dwrite_font));
    if (!This) return E_OUTOFMEMORY;

    hfont = CreateFontIndirectW(logfont);
    if (!hfont)
    {
        heap_free(This);
        return DWRITE_E_NOFONT;
    }

    hdc = CreateCompatibleDC(0);
    SelectObject(hdc, hfont);

    ret = GetOutlineTextMetricsW(hdc, 0, NULL);
    otm = heap_alloc(ret);
    if (!otm)
    {
        heap_free(This);
        DeleteDC(hdc);
        DeleteObject(hfont);
        return E_OUTOFMEMORY;
    }
    otm->otmSize = ret;
    ret = GetOutlineTextMetricsW(hdc, otm->otmSize, otm);

    get_font_properties(This, hdc);

    DeleteDC(hdc);
    DeleteObject(hfont);

    facename = (WCHAR*)((char*)otm + (ptrdiff_t)otm->otmpFaceName);
    familyname = (WCHAR*)((char*)otm + (ptrdiff_t)otm->otmpFamilyName);
    TRACE("facename=%s, familyname=%s\n", debugstr_w(facename), debugstr_w(familyname));

    hr = create_fontfamily(familyname, &family);
    heap_free(otm);
    if (hr != S_OK)
    {
        heap_free(This);
        return hr;
    }

    This->IDWriteFont_iface.lpVtbl = &dwritefontvtbl;
    This->ref = 1;
    This->face = NULL;
    This->family = family;
    This->style = logfont->lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    This->facename = heap_strdupW(logfont->lfFaceName);

    *font = &This->IDWriteFont_iface;

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_QueryInterface(IDWriteFontFile *iface, REFIID riid, void **obj)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFile))
    {
        *obj = iface;
        IDWriteFontFile_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefontfile_AddRef(IDWriteFontFile *iface)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefontfile_Release(IDWriteFontFile *iface)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        IDWriteFontFileLoader_Release(This->loader);
        heap_free(This->reference_key);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dwritefontfile_GetReferenceKey(IDWriteFontFile *iface, const void **fontFileReferenceKey, UINT32 *fontFileReferenceKeySize)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    TRACE("(%p)->(%p, %p)\n", This, fontFileReferenceKey, fontFileReferenceKeySize);
    *fontFileReferenceKey = This->reference_key;
    *fontFileReferenceKeySize = This->key_size;

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_GetLoader(IDWriteFontFile *iface, IDWriteFontFileLoader **fontFileLoader)
{
    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    TRACE("(%p)->(%p)\n", This, fontFileLoader);
    *fontFileLoader = This->loader;
    IDWriteFontFileLoader_AddRef(This->loader);

    return S_OK;
}

static HRESULT WINAPI dwritefontfile_Analyze(IDWriteFontFile *iface, BOOL *isSupportedFontType, DWRITE_FONT_FILE_TYPE *fontFileType, DWRITE_FONT_FACE_TYPE *fontFaceType, UINT32 *numberOfFaces)
{
    HRESULT hr;
    const void *font_data;
    void *context;
    IDWriteFontFileStream *stream;

    struct dwrite_fontfile *This = impl_from_IDWriteFontFile(iface);
    FIXME("(%p)->(%p, %p, %p, %p): Stub\n", This, isSupportedFontType, fontFileType, fontFaceType, numberOfFaces);

    *isSupportedFontType = FALSE;
    *fontFileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    if (fontFaceType)
        *fontFaceType = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    *numberOfFaces = 0;

    hr = IDWriteFontFileLoader_CreateStreamFromKey(This->loader, This->reference_key, This->key_size, &stream);
    if (FAILED(hr))
        return S_OK;
    hr = IDWriteFontFileStream_ReadFileFragment(stream, &font_data, 0, 28, &context);
    if (SUCCEEDED(hr))
    {
        hr = analyze_opentype_font(font_data, numberOfFaces, fontFileType, fontFaceType, isSupportedFontType);
        IDWriteFontFileStream_ReleaseFileFragment(stream, context);
    }
    /* TODO: Further Analysis */
    IDWriteFontFileStream_Release(stream);
    return S_OK;
}

static const IDWriteFontFileVtbl dwritefontfilevtbl = {
    dwritefontfile_QueryInterface,
    dwritefontfile_AddRef,
    dwritefontfile_Release,
    dwritefontfile_GetReferenceKey,
    dwritefontfile_GetLoader,
    dwritefontfile_Analyze,
};

HRESULT create_font_file(IDWriteFontFileLoader *loader, const void *reference_key, UINT32 key_size, IDWriteFontFile **font_file)
{
    struct dwrite_fontfile *This;

    This = heap_alloc(sizeof(struct dwrite_fontfile));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteFontFile_iface.lpVtbl = &dwritefontfilevtbl;
    This->ref = 1;
    IDWriteFontFileLoader_AddRef(loader);
    This->loader = loader;
    This->reference_key = heap_alloc(key_size);
    memcpy(This->reference_key, reference_key, key_size);
    This->key_size = key_size;

    *font_file = &This->IDWriteFontFile_iface;

    return S_OK;
}
