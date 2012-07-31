/*
 *    DWrite
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "initguid.h"
#include "dwrite.h"

#include "dwrite_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    }
    return TRUE;
}

static HRESULT WINAPI dwritefactory_QueryInterface(IDWriteFactory *iface, REFIID riid, void **obj)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteFactory))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefactory_AddRef(IDWriteFactory *iface)
{
    return 2;
}

static ULONG WINAPI dwritefactory_Release(IDWriteFactory *iface)
{
    return 1;
}

static HRESULT WINAPI dwritefactory_GetSystemFontCollection(IDWriteFactory *iface,
    IDWriteFontCollection **collection, BOOL check_for_updates)
{
    FIXME("(%p %d): stub\n", collection, check_for_updates);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateCustomFontCollection(IDWriteFactory *iface,
    IDWriteFontCollectionLoader *loader, void const *key, UINT32 key_size, IDWriteFontCollection **collection)
{
    FIXME("(%p %p %u %p): stub\n", loader, key, key_size, collection);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_RegisterFontCollectionLoader(IDWriteFactory *iface,
    IDWriteFontCollectionLoader *loader)
{
    FIXME("(%p): stub\n", loader);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_UnregisterFontCollectionLoader(IDWriteFactory *iface,
    IDWriteFontCollectionLoader *loader)
{
    FIXME("(%p): stub\n", loader);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateFontFileReference(IDWriteFactory *iface,
    WCHAR const *path, FILETIME const *writetime, IDWriteFontFile **font_file)
{
    FIXME("(%s %p %p): stub\n", debugstr_w(path), writetime, font_file);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateCustomFontFileReference(IDWriteFactory *iface,
    void const *reference_key, UINT32 key_size, IDWriteFontFileLoader *loader, IDWriteFontFile **font_file)
{
    FIXME("(%p %u %p %p): stub\n", reference_key, key_size, loader, font_file);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateFontFace(IDWriteFactory *iface,
    DWRITE_FONT_FACE_TYPE facetype, UINT32 files_number, IDWriteFontFile* const* font_files,
    UINT32 index, DWRITE_FONT_SIMULATIONS sim_flags, IDWriteFontFace **font_face)
{
    FIXME("(%d %u %p %u 0x%x %p): stub\n", facetype, files_number, font_files, index, sim_flags, font_face);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateRenderingParams(IDWriteFactory *iface, IDWriteRenderingParams **params)
{
    FIXME("(%p): stub\n", params);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateMonitorRenderingParams(IDWriteFactory *iface, HMONITOR monitor,
    IDWriteRenderingParams **params)
{
    FIXME("(%p %p): stub\n", monitor, params);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateCustomRenderingParams(IDWriteFactory *iface, FLOAT gamma, FLOAT enhancedContrast,
    FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode, IDWriteRenderingParams **params)
{
    FIXME("(%f %f %f %d %d %p): stub\n", gamma, enhancedContrast, cleartype_level, geometry, mode, params);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_RegisterFontFileLoader(IDWriteFactory *iface, IDWriteFontFileLoader *loader)
{
    FIXME("(%p): stub\n", loader);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_UnregisterFontFileLoader(IDWriteFactory *iface, IDWriteFontFileLoader *loader)
{
    FIXME("(%p): stub\n", loader);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateTextFormat(IDWriteFactory *iface, WCHAR const* family_name,
    IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, WCHAR const *locale, IDWriteTextFormat **format)
{
    FIXME("(%s %p %d %d %d %f %s %p): stub\n", debugstr_w(family_name), collection, weight, style, stretch,
        size, debugstr_w(locale), format);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateTypography(IDWriteFactory *iface, IDWriteTypography **typography)
{
    FIXME("(%p): stub\n", typography);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_GetGdiInterop(IDWriteFactory *iface, IDWriteGdiInterop **gdi_interop)
{
    TRACE("(%p)\n", gdi_interop);
    return create_gdiinterop(gdi_interop);
}

static HRESULT WINAPI dwritefactory_CreateTextLayout(IDWriteFactory *iface, WCHAR const* string,
    UINT32 len, IDWriteTextFormat *format, FLOAT max_width, FLOAT max_height, IDWriteTextLayout **layout)
{
    FIXME("(%s %u %p %f %f %p): stub\n", debugstr_w(string), len, format, max_width, max_height, layout);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateGdiCompatibleTextLayout(IDWriteFactory *iface, WCHAR const* string,
    UINT32 len, IDWriteTextFormat *format, FLOAT layout_width, FLOAT layout_height, FLOAT pixels_per_dip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, IDWriteTextLayout **layout)
{
    FIXME("(%s:%u %p %f %f %f %p %d %p): stub\n", debugstr_wn(string, len), len, format, layout_width, layout_height,
        pixels_per_dip, transform, use_gdi_natural, layout);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateEllipsisTrimmingSign(IDWriteFactory *iface, IDWriteTextFormat *format,
    IDWriteInlineObject **trimming_sign)
{
    FIXME("(%p %p): stub\n", format, trimming_sign);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateTextAnalyzer(IDWriteFactory *iface, IDWriteTextAnalyzer **analyzer)
{
    FIXME("(%p): stub\n", analyzer);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateNumberSubstitution(IDWriteFactory *iface, DWRITE_NUMBER_SUBSTITUTION_METHOD method,
    WCHAR const* locale, BOOL ignore_user_override, IDWriteNumberSubstitution **substitution)
{
    FIXME("(%d %s %d %p): stub\n", method, debugstr_w(locale), ignore_user_override, substitution);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory_CreateGlyphRunAnalysis(IDWriteFactory *iface, DWRITE_GLYPH_RUN const *glyph_run,
    FLOAT pixels_per_dip, DWRITE_MATRIX const* transform, DWRITE_RENDERING_MODE rendering_mode,
    DWRITE_MEASURING_MODE measuring_mode, FLOAT baseline_x, FLOAT baseline_y, IDWriteGlyphRunAnalysis **analysis)
{
    FIXME("(%p %f %p %d %d %f %f %p): stub\n", glyph_run, pixels_per_dip, transform, rendering_mode,
        measuring_mode, baseline_x, baseline_y, analysis);
    return E_NOTIMPL;
}

static const struct IDWriteFactoryVtbl dwritefactoryvtbl = {
    dwritefactory_QueryInterface,
    dwritefactory_AddRef,
    dwritefactory_Release,
    dwritefactory_GetSystemFontCollection,
    dwritefactory_CreateCustomFontCollection,
    dwritefactory_RegisterFontCollectionLoader,
    dwritefactory_UnregisterFontCollectionLoader,
    dwritefactory_CreateFontFileReference,
    dwritefactory_CreateCustomFontFileReference,
    dwritefactory_CreateFontFace,
    dwritefactory_CreateRenderingParams,
    dwritefactory_CreateMonitorRenderingParams,
    dwritefactory_CreateCustomRenderingParams,
    dwritefactory_RegisterFontFileLoader,
    dwritefactory_UnregisterFontFileLoader,
    dwritefactory_CreateTextFormat,
    dwritefactory_CreateTypography,
    dwritefactory_GetGdiInterop,
    dwritefactory_CreateTextLayout,
    dwritefactory_CreateGdiCompatibleTextLayout,
    dwritefactory_CreateEllipsisTrimmingSign,
    dwritefactory_CreateTextAnalyzer,
    dwritefactory_CreateNumberSubstitution,
    dwritefactory_CreateGlyphRunAnalysis
};

static IDWriteFactory dwritefactory = { &dwritefactoryvtbl };

HRESULT WINAPI DWriteCreateFactory(DWRITE_FACTORY_TYPE type, REFIID riid, IUnknown **factory)
{
    TRACE("(%d, %s, %p)\n", type, debugstr_guid(riid), factory);

    if (!IsEqualIID(riid, &IID_IDWriteFactory)) return E_FAIL;

    *factory = (IUnknown*)&dwritefactory;

    return S_OK;
}
