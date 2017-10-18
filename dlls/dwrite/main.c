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

#define COBJMACROS

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "initguid.h"

#include "dwrite_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

static IDWriteFactory5 *shared_factory;
static void release_shared_factory(IDWriteFactory5*);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        init_freetype();
        init_local_fontfile_loader();
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        release_shared_factory(shared_factory);
        release_freetype();
    }
    return TRUE;
}

struct renderingparams {
    IDWriteRenderingParams3 IDWriteRenderingParams3_iface;
    LONG ref;

    FLOAT gamma;
    FLOAT contrast;
    FLOAT grayscalecontrast;
    FLOAT cleartype_level;
    DWRITE_PIXEL_GEOMETRY geometry;
    DWRITE_RENDERING_MODE1 mode;
    DWRITE_GRID_FIT_MODE gridfit;
};

static inline struct renderingparams *impl_from_IDWriteRenderingParams3(IDWriteRenderingParams3 *iface)
{
    return CONTAINING_RECORD(iface, struct renderingparams, IDWriteRenderingParams3_iface);
}

static HRESULT WINAPI renderingparams_QueryInterface(IDWriteRenderingParams3 *iface, REFIID riid, void **obj)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteRenderingParams3) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams2) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams1) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteRenderingParams3_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI renderingparams_AddRef(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI renderingparams_Release(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static FLOAT WINAPI renderingparams_GetGamma(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    TRACE("(%p)\n", This);
    return This->gamma;
}

static FLOAT WINAPI renderingparams_GetEnhancedContrast(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    TRACE("(%p)\n", This);
    return This->contrast;
}

static FLOAT WINAPI renderingparams_GetClearTypeLevel(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    TRACE("(%p)\n", This);
    return This->cleartype_level;
}

static DWRITE_PIXEL_GEOMETRY WINAPI renderingparams_GetPixelGeometry(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    TRACE("(%p)\n", This);
    return This->geometry;
}

static DWRITE_RENDERING_MODE rendering_mode_from_mode1(DWRITE_RENDERING_MODE1 mode)
{
    static const DWRITE_RENDERING_MODE rendering_modes[] = {
        DWRITE_RENDERING_MODE_DEFAULT,           /* DWRITE_RENDERING_MODE1_DEFAULT */
        DWRITE_RENDERING_MODE_ALIASED,           /* DWRITE_RENDERING_MODE1_ALIASED */
        DWRITE_RENDERING_MODE_GDI_CLASSIC,       /* DWRITE_RENDERING_MODE1_GDI_CLASSIC */
        DWRITE_RENDERING_MODE_GDI_NATURAL,       /* DWRITE_RENDERING_MODE1_GDI_NATURAL */
        DWRITE_RENDERING_MODE_NATURAL,           /* DWRITE_RENDERING_MODE1_NATURAL */
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, /* DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC */
        DWRITE_RENDERING_MODE_OUTLINE,           /* DWRITE_RENDERING_MODE1_OUTLINE */
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, /* DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC_DOWNSAMPLED */
    };

    return rendering_modes[mode];
}

static DWRITE_RENDERING_MODE WINAPI renderingparams_GetRenderingMode(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);

    TRACE("(%p)\n", This);

    return rendering_mode_from_mode1(This->mode);
}

static FLOAT WINAPI renderingparams1_GetGrayscaleEnhancedContrast(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    TRACE("(%p)\n", This);
    return This->grayscalecontrast;
}

static DWRITE_GRID_FIT_MODE WINAPI renderingparams2_GetGridFitMode(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    TRACE("(%p)\n", This);
    return This->gridfit;
}

static DWRITE_RENDERING_MODE1 WINAPI renderingparams3_GetRenderingMode1(IDWriteRenderingParams3 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams3(iface);
    TRACE("(%p)\n", This);
    return This->mode;
}

static const struct IDWriteRenderingParams3Vtbl renderingparamsvtbl = {
    renderingparams_QueryInterface,
    renderingparams_AddRef,
    renderingparams_Release,
    renderingparams_GetGamma,
    renderingparams_GetEnhancedContrast,
    renderingparams_GetClearTypeLevel,
    renderingparams_GetPixelGeometry,
    renderingparams_GetRenderingMode,
    renderingparams1_GetGrayscaleEnhancedContrast,
    renderingparams2_GetGridFitMode,
    renderingparams3_GetRenderingMode1
};

static HRESULT create_renderingparams(FLOAT gamma, FLOAT contrast, FLOAT grayscalecontrast, FLOAT cleartype_level,
        DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE1 mode, DWRITE_GRID_FIT_MODE gridfit,
        IDWriteRenderingParams3 **params)
{
    struct renderingparams *This;

    *params = NULL;

    if (gamma <= 0.0f || contrast < 0.0f || grayscalecontrast < 0.0f || cleartype_level < 0.0f)
        return E_INVALIDARG;

    if ((UINT32)gridfit > DWRITE_GRID_FIT_MODE_ENABLED || (UINT32)geometry > DWRITE_PIXEL_GEOMETRY_BGR)
        return E_INVALIDARG;

    This = heap_alloc(sizeof(struct renderingparams));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteRenderingParams3_iface.lpVtbl = &renderingparamsvtbl;
    This->ref = 1;

    This->gamma = gamma;
    This->contrast = contrast;
    This->grayscalecontrast = grayscalecontrast;
    This->cleartype_level = cleartype_level;
    This->geometry = geometry;
    This->mode = mode;
    This->gridfit = gridfit;

    *params = &This->IDWriteRenderingParams3_iface;

    return S_OK;
}

struct localizedpair {
    WCHAR *locale;
    WCHAR *string;
};

struct localizedstrings {
    IDWriteLocalizedStrings IDWriteLocalizedStrings_iface;
    LONG ref;

    struct localizedpair *data;
    UINT32 count;
    UINT32 alloc;
};

static inline struct localizedstrings *impl_from_IDWriteLocalizedStrings(IDWriteLocalizedStrings *iface)
{
    return CONTAINING_RECORD(iface, struct localizedstrings, IDWriteLocalizedStrings_iface);
}

static HRESULT WINAPI localizedstrings_QueryInterface(IDWriteLocalizedStrings *iface, REFIID riid, void **obj)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteLocalizedStrings))
    {
        *obj = iface;
        IDWriteLocalizedStrings_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI localizedstrings_AddRef(IDWriteLocalizedStrings *iface)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI localizedstrings_Release(IDWriteLocalizedStrings *iface)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        unsigned int i;

        for (i = 0; i < This->count; i++) {
            heap_free(This->data[i].locale);
            heap_free(This->data[i].string);
        }

        heap_free(This->data);
        heap_free(This);
    }

    return ref;
}

static UINT32 WINAPI localizedstrings_GetCount(IDWriteLocalizedStrings *iface)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    TRACE("(%p)\n", This);
    return This->count;
}

static HRESULT WINAPI localizedstrings_FindLocaleName(IDWriteLocalizedStrings *iface,
    WCHAR const *locale_name, UINT32 *index, BOOL *exists)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    UINT32 i;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(locale_name), index, exists);

    *exists = FALSE;
    *index = ~0;

    for (i = 0; i < This->count; i++) {
        if (!strcmpiW(This->data[i].locale, locale_name)) {
            *exists = TRUE;
            *index = i;
            break;
        }
    }

    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetLocaleNameLength(IDWriteLocalizedStrings *iface, UINT32 index, UINT32 *length)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("(%p)->(%u %p)\n", This, index, length);

    if (index >= This->count) {
        *length = (UINT32)-1;
        return E_FAIL;
    }

    *length = strlenW(This->data[index].locale);
    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetLocaleName(IDWriteLocalizedStrings *iface, UINT32 index, WCHAR *buffer, UINT32 size)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("(%p)->(%u %p %u)\n", This, index, buffer, size);

    if (index >= This->count) {
        if (buffer) *buffer = 0;
        return E_FAIL;
    }

    if (size < strlenW(This->data[index].locale)+1) {
        if (buffer) *buffer = 0;
        return E_NOT_SUFFICIENT_BUFFER;
    }

    strcpyW(buffer, This->data[index].locale);
    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetStringLength(IDWriteLocalizedStrings *iface, UINT32 index, UINT32 *length)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("(%p)->(%u %p)\n", This, index, length);

    if (index >= This->count) {
        *length = (UINT32)-1;
        return E_FAIL;
    }

    *length = strlenW(This->data[index].string);
    return S_OK;
}

static HRESULT WINAPI localizedstrings_GetString(IDWriteLocalizedStrings *iface, UINT32 index, WCHAR *buffer, UINT32 size)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);

    TRACE("(%p)->(%u %p %u)\n", This, index, buffer, size);

    if (index >= This->count) {
        if (buffer) *buffer = 0;
        return E_FAIL;
    }

    if (size < strlenW(This->data[index].string)+1) {
        if (buffer) *buffer = 0;
        return E_NOT_SUFFICIENT_BUFFER;
    }

    strcpyW(buffer, This->data[index].string);
    return S_OK;
}

static const IDWriteLocalizedStringsVtbl localizedstringsvtbl = {
    localizedstrings_QueryInterface,
    localizedstrings_AddRef,
    localizedstrings_Release,
    localizedstrings_GetCount,
    localizedstrings_FindLocaleName,
    localizedstrings_GetLocaleNameLength,
    localizedstrings_GetLocaleName,
    localizedstrings_GetStringLength,
    localizedstrings_GetString
};

HRESULT create_localizedstrings(IDWriteLocalizedStrings **strings)
{
    struct localizedstrings *This;

    *strings = NULL;

    This = heap_alloc(sizeof(struct localizedstrings));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteLocalizedStrings_iface.lpVtbl = &localizedstringsvtbl;
    This->ref = 1;
    This->count = 0;
    This->data = heap_alloc_zero(sizeof(struct localizedpair));
    if (!This->data) {
        heap_free(This);
        return E_OUTOFMEMORY;
    }
    This->alloc = 1;

    *strings = &This->IDWriteLocalizedStrings_iface;

    return S_OK;
}

HRESULT add_localizedstring(IDWriteLocalizedStrings *iface, const WCHAR *locale, const WCHAR *string)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    UINT32 i;

    /* make sure there's no duplicates */
    for (i = 0; i < This->count; i++)
        if (!strcmpW(This->data[i].locale, locale))
            return S_OK;

    if (This->count == This->alloc) {
        void *ptr;

        ptr = heap_realloc(This->data, 2*This->alloc*sizeof(struct localizedpair));
        if (!ptr)
            return E_OUTOFMEMORY;

        This->alloc *= 2;
        This->data = ptr;
    }

    This->data[This->count].locale = heap_strdupW(locale);
    This->data[This->count].string = heap_strdupW(string);
    if (!This->data[This->count].locale || !This->data[This->count].string) {
        heap_free(This->data[This->count].locale);
        heap_free(This->data[This->count].string);
        return E_OUTOFMEMORY;
    }

    This->count++;

    return S_OK;
}

HRESULT clone_localizedstring(IDWriteLocalizedStrings *iface, IDWriteLocalizedStrings **ret)
{
    struct localizedstrings *strings, *strings_clone;
    int i;

    *ret = NULL;

    if (!iface)
        return S_FALSE;

    strings = impl_from_IDWriteLocalizedStrings(iface);
    strings_clone = heap_alloc(sizeof(struct localizedstrings));
    if (!strings_clone) return E_OUTOFMEMORY;

    strings_clone->IDWriteLocalizedStrings_iface.lpVtbl = &localizedstringsvtbl;
    strings_clone->ref = 1;
    strings_clone->count = strings->count;
    strings_clone->data = heap_alloc(sizeof(struct localizedpair) * strings_clone->count);
    if (!strings_clone->data) {
        heap_free(strings_clone);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < strings_clone->count; i++)
    {
        strings_clone->data[i].locale = heap_strdupW(strings->data[i].locale);
        strings_clone->data[i].string = heap_strdupW(strings->data[i].string);
    }
    strings_clone->alloc = strings_clone->count;

    *ret = &strings_clone->IDWriteLocalizedStrings_iface;

    return S_OK;
}

void set_en_localizedstring(IDWriteLocalizedStrings *iface, const WCHAR *string)
{
    static const WCHAR enusW[] = {'e','n','-','U','S',0};
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    UINT32 i;

    for (i = 0; i < This->count; i++) {
        if (!strcmpiW(This->data[i].locale, enusW)) {
            heap_free(This->data[i].string);
            This->data[i].string = heap_strdupW(string);
            break;
        }
    }
}

struct collectionloader
{
    struct list entry;
    IDWriteFontCollectionLoader *loader;
};

struct fileloader
{
    struct list entry;
    struct list fontfaces;
    IDWriteFontFileLoader *loader;
};

struct dwritefactory {
    IDWriteFactory5 IDWriteFactory5_iface;
    LONG ref;

    IDWriteFontCollection1 *system_collection;
    IDWriteFontCollection1 *eudc_collection;
    IDWriteGdiInterop1 *gdiinterop;
    IDWriteFontFallback *fallback;

    IDWriteFontFileLoader *localfontfileloader;
    struct list localfontfaces;

    struct list collection_loaders;
    struct list file_loaders;

    CRITICAL_SECTION cs;
};

static inline struct dwritefactory *impl_from_IDWriteFactory5(IDWriteFactory5 *iface)
{
    return CONTAINING_RECORD(iface, struct dwritefactory, IDWriteFactory5_iface);
}

static void release_fontface_cache(struct list *fontfaces)
{
    struct fontfacecached *fontface, *fontface2;

    LIST_FOR_EACH_ENTRY_SAFE(fontface, fontface2, fontfaces, struct fontfacecached, entry) {
        list_remove(&fontface->entry);
        fontface_detach_from_cache(fontface->fontface);
        heap_free(fontface);
    }
}

static void release_fileloader(struct fileloader *fileloader)
{
    list_remove(&fileloader->entry);
    release_fontface_cache(&fileloader->fontfaces);
    IDWriteFontFileLoader_Release(fileloader->loader);
    heap_free(fileloader);
}

static void release_dwritefactory(struct dwritefactory *factory)
{
    struct fileloader *fileloader, *fileloader2;
    struct collectionloader *loader, *loader2;

    EnterCriticalSection(&factory->cs);
    release_fontface_cache(&factory->localfontfaces);
    LeaveCriticalSection(&factory->cs);

    LIST_FOR_EACH_ENTRY_SAFE(loader, loader2, &factory->collection_loaders, struct collectionloader, entry) {
        list_remove(&loader->entry);
        IDWriteFontCollectionLoader_Release(loader->loader);
        heap_free(loader);
    }

    LIST_FOR_EACH_ENTRY_SAFE(fileloader, fileloader2, &factory->file_loaders, struct fileloader, entry)
        release_fileloader(fileloader);

    if (factory->system_collection)
        IDWriteFontCollection1_Release(factory->system_collection);
    if (factory->eudc_collection)
        IDWriteFontCollection1_Release(factory->eudc_collection);
    if (factory->fallback)
        release_system_fontfallback(factory->fallback);

    factory->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&factory->cs);
    heap_free(factory);
}

static void release_shared_factory(IDWriteFactory5 *iface)
{
    struct dwritefactory *factory;
    if (!iface) return;
    factory = impl_from_IDWriteFactory5(iface);
    release_dwritefactory(factory);
}

static struct fileloader *factory_get_file_loader(struct dwritefactory *factory, IDWriteFontFileLoader *loader)
{
    struct fileloader *entry, *found = NULL;

    LIST_FOR_EACH_ENTRY(entry, &factory->file_loaders, struct fileloader, entry) {
        if (entry->loader == loader) {
            found = entry;
            break;
        }
    }

    return found;
}

static struct collectionloader *factory_get_collection_loader(struct dwritefactory *factory,
        IDWriteFontCollectionLoader *loader)
{
    struct collectionloader *entry, *found = NULL;

    LIST_FOR_EACH_ENTRY(entry, &factory->collection_loaders, struct collectionloader, entry) {
        if (entry->loader == loader) {
            found = entry;
            break;
        }
    }

    return found;
}

static IDWriteFontCollection1 *factory_get_system_collection(struct dwritefactory *factory)
{
    IDWriteFontCollection1 *collection;
    HRESULT hr;

    if (factory->system_collection) {
        IDWriteFontCollection1_AddRef(factory->system_collection);
        return factory->system_collection;
    }

    if (FAILED(hr = get_system_fontcollection(&factory->IDWriteFactory5_iface, &collection))) {
        WARN("Failed to create system font collection, hr %#x.\n", hr);
        return NULL;
    }

    if (InterlockedCompareExchangePointer((void **)&factory->system_collection, collection, NULL))
        IDWriteFontCollection1_Release(collection);

    return factory->system_collection;
}

static HRESULT WINAPI dwritefactory_QueryInterface(IDWriteFactory5 *iface, REFIID riid, void **obj)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFactory5) ||
        IsEqualIID(riid, &IID_IDWriteFactory4) ||
        IsEqualIID(riid, &IID_IDWriteFactory3) ||
        IsEqualIID(riid, &IID_IDWriteFactory2) ||
        IsEqualIID(riid, &IID_IDWriteFactory1) ||
        IsEqualIID(riid, &IID_IDWriteFactory) ||
        IsEqualIID(riid, &IID_IUnknown))
   {
        *obj = iface;
        IDWriteFactory5_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefactory_AddRef(IDWriteFactory5 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefactory_Release(IDWriteFactory5 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        release_dwritefactory(This);

    return ref;
}

static HRESULT WINAPI dwritefactory_GetSystemFontCollection(IDWriteFactory5 *iface,
    IDWriteFontCollection **collection, BOOL check_for_updates)
{
    return IDWriteFactory5_GetSystemFontCollection(iface, FALSE, (IDWriteFontCollection1 **)collection,
            check_for_updates);
}

static HRESULT WINAPI dwritefactory_CreateCustomFontCollection(IDWriteFactory5 *iface,
    IDWriteFontCollectionLoader *loader, void const *key, UINT32 key_size, IDWriteFontCollection **collection)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    IDWriteFontFileEnumerator *enumerator;
    struct collectionloader *found;
    HRESULT hr;

    TRACE("(%p)->(%p %p %u %p)\n", This, loader, key, key_size, collection);

    *collection = NULL;

    if (!loader)
        return E_INVALIDARG;

    found = factory_get_collection_loader(This, loader);
    if (!found)
        return E_INVALIDARG;

    hr = IDWriteFontCollectionLoader_CreateEnumeratorFromKey(found->loader, (IDWriteFactory*)iface,
            key, key_size, &enumerator);
    if (FAILED(hr))
        return hr;

    hr = create_font_collection(iface, enumerator, FALSE, (IDWriteFontCollection1**)collection);
    IDWriteFontFileEnumerator_Release(enumerator);
    return hr;
}

static HRESULT WINAPI dwritefactory_RegisterFontCollectionLoader(IDWriteFactory5 *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct collectionloader *entry;

    TRACE("(%p)->(%p)\n", This, loader);

    if (!loader)
        return E_INVALIDARG;

    if (factory_get_collection_loader(This, loader))
        return DWRITE_E_ALREADYREGISTERED;

    entry = heap_alloc(sizeof(*entry));
    if (!entry)
        return E_OUTOFMEMORY;

    entry->loader = loader;
    IDWriteFontCollectionLoader_AddRef(loader);
    list_add_tail(&This->collection_loaders, &entry->entry);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_UnregisterFontCollectionLoader(IDWriteFactory5 *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct collectionloader *found;

    TRACE("(%p)->(%p)\n", This, loader);

    if (!loader)
        return E_INVALIDARG;

    found = factory_get_collection_loader(This, loader);
    if (!found)
        return E_INVALIDARG;

    IDWriteFontCollectionLoader_Release(found->loader);
    list_remove(&found->entry);
    heap_free(found);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateFontFileReference(IDWriteFactory5 *iface,
    WCHAR const *path, FILETIME const *writetime, IDWriteFontFile **font_file)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    UINT32 key_size;
    HRESULT hr;
    void *key;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(path), writetime, font_file);

    *font_file = NULL;

    /* Get a reference key in local file loader format. */
    hr = get_local_refkey(path, writetime, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = create_font_file(This->localfontfileloader, key, key_size, font_file);
    heap_free(key);

    return hr;
}

static HRESULT WINAPI dwritefactory_CreateCustomFontFileReference(IDWriteFactory5 *iface,
    void const *reference_key, UINT32 key_size, IDWriteFontFileLoader *loader, IDWriteFontFile **font_file)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p %u %p %p)\n", This, reference_key, key_size, loader, font_file);

    *font_file = NULL;

    if (!loader || !(factory_get_file_loader(This, loader) || This->localfontfileloader == loader))
        return E_INVALIDARG;

    return create_font_file(loader, reference_key, key_size, font_file);
}

void factory_lock(IDWriteFactory5 *iface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory5(iface);
    EnterCriticalSection(&factory->cs);
}

void factory_unlock(IDWriteFactory5 *iface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory5(iface);
    LeaveCriticalSection(&factory->cs);
}

HRESULT factory_get_cached_fontface(IDWriteFactory5 *iface, IDWriteFontFile * const *font_files, UINT32 index,
        DWRITE_FONT_SIMULATIONS simulations, struct list **cached_list, REFIID riid, void **obj)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory5(iface);
    struct fontfacecached *cached;
    IDWriteFontFileLoader *loader;
    struct list *fontfaces;
    UINT32 key_size;
    const void *key;
    HRESULT hr;

    *obj = NULL;
    *cached_list = NULL;

    hr = IDWriteFontFile_GetReferenceKey(*font_files, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFile_GetLoader(*font_files, &loader);
    if (FAILED(hr))
        return hr;

    if (loader == factory->localfontfileloader) {
        fontfaces = &factory->localfontfaces;
        IDWriteFontFileLoader_Release(loader);
    }
    else {
        struct fileloader *fileloader = factory_get_file_loader(factory, loader);
        IDWriteFontFileLoader_Release(loader);
        if (!fileloader)
            return E_INVALIDARG;
        fontfaces = &fileloader->fontfaces;
    }

    *cached_list = fontfaces;

    EnterCriticalSection(&factory->cs);

    /* search through cache list */
    LIST_FOR_EACH_ENTRY(cached, fontfaces, struct fontfacecached, entry) {
        UINT32 cached_key_size, count = 1, cached_face_index;
        DWRITE_FONT_SIMULATIONS cached_simulations;
        const void *cached_key;
        IDWriteFontFile *file;

        cached_face_index = IDWriteFontFace4_GetIndex(cached->fontface);
        cached_simulations = IDWriteFontFace4_GetSimulations(cached->fontface);

        /* skip earlier */
        if (cached_face_index != index || cached_simulations != simulations)
            continue;

        hr = IDWriteFontFace4_GetFiles(cached->fontface, &count, &file);
        if (FAILED(hr))
            break;

        hr = IDWriteFontFile_GetReferenceKey(file, &cached_key, &cached_key_size);
        IDWriteFontFile_Release(file);
        if (FAILED(hr))
            break;

        if (cached_key_size == key_size && !memcmp(cached_key, key, key_size)) {
            if (FAILED(hr = IDWriteFontFace4_QueryInterface(cached->fontface, riid, obj)))
                WARN("Failed to get %s from fontface, hr %#x.\n", debugstr_guid(riid), hr);

            TRACE("returning cached fontface %p\n", cached->fontface);
            break;
        }
    }

    LeaveCriticalSection(&factory->cs);

    return *obj ? S_OK : S_FALSE;
}

struct fontfacecached *factory_cache_fontface(IDWriteFactory5 *iface, struct list *fontfaces,
        IDWriteFontFace4 *fontface)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory5(iface);
    struct fontfacecached *cached;

    /* new cache entry */
    cached = heap_alloc(sizeof(*cached));
    if (!cached)
        return NULL;

    cached->fontface = fontface;
    EnterCriticalSection(&factory->cs);
    list_add_tail(fontfaces, &cached->entry);
    LeaveCriticalSection(&factory->cs);

    return cached;
}

static HRESULT WINAPI dwritefactory_CreateFontFace(IDWriteFactory5 *iface, DWRITE_FONT_FACE_TYPE req_facetype,
    UINT32 files_number, IDWriteFontFile* const* font_files, UINT32 index, DWRITE_FONT_SIMULATIONS simulations,
    IDWriteFontFace **fontface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFileStream *stream;
    struct fontface_desc desc;
    struct list *fontfaces;
    BOOL is_supported;
    UINT32 face_count;
    HRESULT hr;

    TRACE("(%p)->(%d %u %p %u 0x%x %p)\n", This, req_facetype, files_number, font_files, index,
        simulations, fontface);

    *fontface = NULL;

    if (!is_face_type_supported(req_facetype))
        return E_INVALIDARG;

    if (req_facetype != DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION && index)
        return E_INVALIDARG;

    if (!is_simulation_valid(simulations))
        return E_INVALIDARG;

    if (FAILED(hr = get_filestream_from_file(*font_files, &stream)))
        return hr;

    /* check actual file/face type */
    is_supported = FALSE;
    face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    hr = opentype_analyze_font(stream, &is_supported, &file_type, &face_type, &face_count);
    if (FAILED(hr))
        goto failed;

    if (!is_supported) {
        hr = E_FAIL;
        goto failed;
    }

    if (face_type != req_facetype) {
        hr = DWRITE_E_FILEFORMAT;
        goto failed;
    }

    hr = factory_get_cached_fontface(iface, font_files, index, simulations, &fontfaces,
            &IID_IDWriteFontFace, (void **)fontface);
    if (hr != S_FALSE)
        goto failed;

    desc.factory = iface;
    desc.face_type = req_facetype;
    desc.files = font_files;
    desc.stream = stream;
    desc.files_number = files_number;
    desc.index = index;
    desc.simulations = simulations;
    desc.font_data = NULL;
    hr = create_fontface(&desc, fontfaces, (IDWriteFontFace4 **)fontface);

failed:
    IDWriteFontFileStream_Release(stream);
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateRenderingParams(IDWriteFactory5 *iface, IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    HMONITOR monitor;
    POINT pt;

    TRACE("(%p)->(%p)\n", This, params);

    pt.x = pt.y = 0;
    monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    return IDWriteFactory5_CreateMonitorRenderingParams(iface, monitor, params);
}

static HRESULT WINAPI dwritefactory_CreateMonitorRenderingParams(IDWriteFactory5 *iface, HMONITOR monitor,
    IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    IDWriteRenderingParams3 *params3;
    static int fixme_once = 0;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, monitor, params);

    if (!fixme_once++)
        FIXME("(%p): monitor setting ignored\n", monitor);

    /* FIXME: use actual per-monitor gamma factor */
    hr = IDWriteFactory5_CreateCustomRenderingParams(iface, 2.0f, 0.0f, 1.0f, 0.0f, DWRITE_PIXEL_GEOMETRY_FLAT,
        DWRITE_RENDERING_MODE1_DEFAULT, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams*)params3;
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateCustomRenderingParams(IDWriteFactory5 *iface, FLOAT gamma,
        FLOAT enhancedContrast, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode,
        IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    IDWriteRenderingParams3 *params3;
    HRESULT hr;

    TRACE("(%p)->(%f %f %f %d %d %p)\n", This, gamma, enhancedContrast, cleartype_level, geometry, mode, params);

    if ((UINT32)mode > DWRITE_RENDERING_MODE_OUTLINE) {
        *params = NULL;
        return E_INVALIDARG;
    }

    hr = IDWriteFactory5_CreateCustomRenderingParams(iface, gamma, enhancedContrast, 1.0f, cleartype_level, geometry,
            (DWRITE_RENDERING_MODE1)mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams*)params3;
    return hr;
}

static HRESULT WINAPI dwritefactory_RegisterFontFileLoader(IDWriteFactory5 *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct fileloader *entry;

    TRACE("(%p)->(%p)\n", This, loader);

    if (!loader)
        return E_INVALIDARG;

    if (factory_get_file_loader(This, loader))
        return DWRITE_E_ALREADYREGISTERED;

    entry = heap_alloc(sizeof(*entry));
    if (!entry)
        return E_OUTOFMEMORY;

    entry->loader = loader;
    list_init(&entry->fontfaces);
    IDWriteFontFileLoader_AddRef(loader);
    list_add_tail(&This->file_loaders, &entry->entry);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_UnregisterFontFileLoader(IDWriteFactory5 *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct fileloader *found;

    TRACE("(%p)->(%p)\n", This, loader);

    if (!loader)
        return E_INVALIDARG;

    found = factory_get_file_loader(This, loader);
    if (!found)
        return E_INVALIDARG;

    release_fileloader(found);
    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateTextFormat(IDWriteFactory5 *iface, WCHAR const* family_name,
    IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, WCHAR const *locale, IDWriteTextFormat **format)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    HRESULT hr;

    TRACE("(%p)->(%s %p %d %d %d %f %s %p)\n", This, debugstr_w(family_name), collection, weight, style, stretch,
        size, debugstr_w(locale), format);

    if (collection)
        IDWriteFontCollection_AddRef(collection);
    else {
        collection = (IDWriteFontCollection *)factory_get_system_collection(This);
        if (!collection) {
            *format = NULL;
            return E_FAIL;
        }
    }

    hr = create_textformat(family_name, collection, weight, style, stretch, size, locale, format);
    IDWriteFontCollection_Release(collection);
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateTypography(IDWriteFactory5 *iface, IDWriteTypography **typography)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    TRACE("(%p)->(%p)\n", This, typography);
    return create_typography(typography);
}

static HRESULT WINAPI dwritefactory_GetGdiInterop(IDWriteFactory5 *iface, IDWriteGdiInterop **gdi_interop)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p)\n", This, gdi_interop);

    if (This->gdiinterop)
        IDWriteGdiInterop1_AddRef(This->gdiinterop);
    else
        hr = create_gdiinterop(iface, &This->gdiinterop);

    *gdi_interop = (IDWriteGdiInterop *)This->gdiinterop;

    return hr;
}

static HRESULT WINAPI dwritefactory_CreateTextLayout(IDWriteFactory5 *iface, WCHAR const* string,
    UINT32 length, IDWriteTextFormat *format, FLOAT max_width, FLOAT max_height, IDWriteTextLayout **layout)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct textlayout_desc desc;

    TRACE("(%p)->(%s:%u %p %f %f %p)\n", This, debugstr_wn(string, length), length, format, max_width, max_height, layout);

    desc.factory = iface;
    desc.string = string;
    desc.length = length;
    desc.format = format;
    desc.max_width = max_width;
    desc.max_height = max_height;
    desc.is_gdi_compatible = FALSE;
    desc.ppdip = 1.0f;
    desc.transform = NULL;
    desc.use_gdi_natural = FALSE;
    return create_textlayout(&desc, layout);
}

static HRESULT WINAPI dwritefactory_CreateGdiCompatibleTextLayout(IDWriteFactory5 *iface, WCHAR const* string,
    UINT32 length, IDWriteTextFormat *format, FLOAT max_width, FLOAT max_height, FLOAT pixels_per_dip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, IDWriteTextLayout **layout)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct textlayout_desc desc;

    TRACE("(%p)->(%s:%u %p %f %f %f %p %d %p)\n", This, debugstr_wn(string, length), length, format, max_width,
            max_height, pixels_per_dip, transform, use_gdi_natural, layout);

    desc.factory = iface;
    desc.string = string;
    desc.length = length;
    desc.format = format;
    desc.max_width = max_width;
    desc.max_height = max_height;
    desc.is_gdi_compatible = TRUE;
    desc.ppdip = pixels_per_dip;
    desc.transform = transform;
    desc.use_gdi_natural = use_gdi_natural;
    return create_textlayout(&desc, layout);
}

static HRESULT WINAPI dwritefactory_CreateEllipsisTrimmingSign(IDWriteFactory5 *iface, IDWriteTextFormat *format,
    IDWriteInlineObject **trimming_sign)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    TRACE("(%p)->(%p %p)\n", This, format, trimming_sign);
    return create_trimmingsign(iface, format, trimming_sign);
}

static HRESULT WINAPI dwritefactory_CreateTextAnalyzer(IDWriteFactory5 *iface, IDWriteTextAnalyzer **analyzer)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p)\n", This, analyzer);

    *analyzer = get_text_analyzer();

    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateNumberSubstitution(IDWriteFactory5 *iface,
        DWRITE_NUMBER_SUBSTITUTION_METHOD method, WCHAR const* locale, BOOL ignore_user_override,
        IDWriteNumberSubstitution **substitution)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    TRACE("(%p)->(%d %s %d %p)\n", This, method, debugstr_w(locale), ignore_user_override, substitution);
    return create_numbersubstitution(method, locale, ignore_user_override, substitution);
}

static HRESULT WINAPI dwritefactory_CreateGlyphRunAnalysis(IDWriteFactory5 *iface, DWRITE_GLYPH_RUN const *run,
    FLOAT ppdip, DWRITE_MATRIX const* transform, DWRITE_RENDERING_MODE rendering_mode,
    DWRITE_MEASURING_MODE measuring_mode, FLOAT originX, FLOAT originY, IDWriteGlyphRunAnalysis **analysis)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct glyphrunanalysis_desc desc;

    TRACE("(%p)->(%p %.2f %p %d %d %.2f %.2f %p)\n", This, run, ppdip, transform, rendering_mode,
        measuring_mode, originX, originY, analysis);

    if (ppdip <= 0.0f) {
        *analysis = NULL;
        return E_INVALIDARG;
    }

    desc.run = run;
    desc.ppdip = ppdip;
    desc.transform = transform;
    desc.rendering_mode = (DWRITE_RENDERING_MODE1)rendering_mode;
    desc.measuring_mode = measuring_mode;
    desc.gridfit_mode = DWRITE_GRID_FIT_MODE_DEFAULT;
    desc.aa_mode = DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE;
    desc.origin_x = originX;
    desc.origin_y = originY;
    return create_glyphrunanalysis(&desc, analysis);
}

static HRESULT WINAPI dwritefactory1_GetEudcFontCollection(IDWriteFactory5 *iface, IDWriteFontCollection **collection,
    BOOL check_for_updates)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p %d)\n", This, collection, check_for_updates);

    if (check_for_updates)
        FIXME("checking for eudc updates not implemented\n");

    if (This->eudc_collection)
        IDWriteFontCollection1_AddRef(This->eudc_collection);
    else {
        IDWriteFontCollection1 *eudc_collection;

        if (FAILED(hr = get_eudc_fontcollection(iface, &eudc_collection))) {
            *collection = NULL;
            WARN("Failed to get EUDC collection, hr %#x.\n", hr);
            return hr;
        }

        if (InterlockedCompareExchangePointer((void **)&This->eudc_collection, eudc_collection, NULL))
            IDWriteFontCollection1_Release(eudc_collection);
    }

    *collection = (IDWriteFontCollection *)This->eudc_collection;

    return hr;
}

static HRESULT WINAPI dwritefactory1_CreateCustomRenderingParams(IDWriteFactory5 *iface, FLOAT gamma,
    FLOAT enhcontrast, FLOAT enhcontrast_grayscale, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry,
    DWRITE_RENDERING_MODE mode, IDWriteRenderingParams1** params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    IDWriteRenderingParams3 *params3;
    HRESULT hr;

    TRACE("(%p)->(%.2f %.2f %.2f %.2f %d %d %p)\n", This, gamma, enhcontrast, enhcontrast_grayscale,
        cleartype_level, geometry, mode, params);

    if ((UINT32)mode > DWRITE_RENDERING_MODE_OUTLINE) {
        *params = NULL;
        return E_INVALIDARG;
    }

    hr = IDWriteFactory5_CreateCustomRenderingParams(iface, gamma, enhcontrast, enhcontrast_grayscale,
        cleartype_level, geometry, (DWRITE_RENDERING_MODE1)mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams1*)params3;
    return hr;
}

static HRESULT WINAPI dwritefactory2_GetSystemFontFallback(IDWriteFactory5 *iface, IDWriteFontFallback **fallback)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p)\n", This, fallback);

    *fallback = NULL;

    if (!This->fallback) {
        HRESULT hr = create_system_fontfallback(iface, &This->fallback);
        if (FAILED(hr))
            return hr;
    }

    *fallback = This->fallback;
    IDWriteFontFallback_AddRef(*fallback);
    return S_OK;
}

static HRESULT WINAPI dwritefactory2_CreateFontFallbackBuilder(IDWriteFactory5 *iface,
        IDWriteFontFallbackBuilder **fallbackbuilder)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p)\n", This, fallbackbuilder);

    return create_fontfallback_builder(iface, fallbackbuilder);
}

static HRESULT WINAPI dwritefactory2_TranslateColorGlyphRun(IDWriteFactory5 *iface, FLOAT originX, FLOAT originY,
    const DWRITE_GLYPH_RUN *run, const DWRITE_GLYPH_RUN_DESCRIPTION *rundescr, DWRITE_MEASURING_MODE mode,
    const DWRITE_MATRIX *transform, UINT32 palette, IDWriteColorGlyphRunEnumerator **colorlayers)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    TRACE("(%p)->(%.2f %.2f %p %p %d %p %u %p)\n", This, originX, originY, run, rundescr, mode,
        transform, palette, colorlayers);
    return create_colorglyphenum(originX, originY, run, rundescr, mode, transform, palette, colorlayers);
}

static HRESULT WINAPI dwritefactory2_CreateCustomRenderingParams(IDWriteFactory5 *iface, FLOAT gamma, FLOAT contrast,
    FLOAT grayscalecontrast, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode,
    DWRITE_GRID_FIT_MODE gridfit, IDWriteRenderingParams2 **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    IDWriteRenderingParams3 *params3;
    HRESULT hr;

    TRACE("(%p)->(%.2f %.2f %.2f %.2f %d %d %d %p)\n", This, gamma, contrast, grayscalecontrast, cleartype_level,
        geometry, mode, gridfit, params);

    if ((UINT32)mode > DWRITE_RENDERING_MODE_OUTLINE) {
        *params = NULL;
        return E_INVALIDARG;
    }

    hr = IDWriteFactory5_CreateCustomRenderingParams(iface, gamma, contrast, grayscalecontrast,
        cleartype_level, geometry, (DWRITE_RENDERING_MODE1)mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params3);
    *params = (IDWriteRenderingParams2*)params3;
    return hr;
}

static HRESULT WINAPI dwritefactory2_CreateGlyphRunAnalysis(IDWriteFactory5 *iface, const DWRITE_GLYPH_RUN *run,
    const DWRITE_MATRIX *transform, DWRITE_RENDERING_MODE rendering_mode, DWRITE_MEASURING_MODE measuring_mode,
    DWRITE_GRID_FIT_MODE gridfit_mode, DWRITE_TEXT_ANTIALIAS_MODE aa_mode, FLOAT originX, FLOAT originY,
    IDWriteGlyphRunAnalysis **analysis)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct glyphrunanalysis_desc desc;

    TRACE("(%p)->(%p %p %d %d %d %d %.2f %.2f %p)\n", This, run, transform, rendering_mode, measuring_mode,
        gridfit_mode, aa_mode, originX, originY, analysis);

    desc.run = run;
    desc.ppdip = 1.0f;
    desc.transform = transform;
    desc.rendering_mode = (DWRITE_RENDERING_MODE1)rendering_mode;
    desc.measuring_mode = measuring_mode;
    desc.gridfit_mode = gridfit_mode;
    desc.aa_mode = aa_mode;
    desc.origin_x = originX;
    desc.origin_y = originY;
    return create_glyphrunanalysis(&desc, analysis);
}

static HRESULT WINAPI dwritefactory3_CreateGlyphRunAnalysis(IDWriteFactory5 *iface, DWRITE_GLYPH_RUN const *run,
    DWRITE_MATRIX const *transform, DWRITE_RENDERING_MODE1 rendering_mode, DWRITE_MEASURING_MODE measuring_mode,
    DWRITE_GRID_FIT_MODE gridfit_mode, DWRITE_TEXT_ANTIALIAS_MODE aa_mode, FLOAT originX, FLOAT originY,
    IDWriteGlyphRunAnalysis **analysis)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    struct glyphrunanalysis_desc desc;

    TRACE("(%p)->(%p %p %d %d %d %d %.2f %.2f %p)\n", This, run, transform, rendering_mode, measuring_mode,
        gridfit_mode, aa_mode, originX, originY, analysis);

    desc.run = run;
    desc.ppdip = 1.0f;
    desc.transform = transform;
    desc.rendering_mode = rendering_mode;
    desc.measuring_mode = measuring_mode;
    desc.gridfit_mode = gridfit_mode;
    desc.aa_mode = aa_mode;
    desc.origin_x = originX;
    desc.origin_y = originY;
    return create_glyphrunanalysis(&desc, analysis);
}

static HRESULT WINAPI dwritefactory3_CreateCustomRenderingParams(IDWriteFactory5 *iface, FLOAT gamma, FLOAT contrast,
        FLOAT grayscale_contrast, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY pixel_geometry,
        DWRITE_RENDERING_MODE1 rendering_mode, DWRITE_GRID_FIT_MODE gridfit_mode, IDWriteRenderingParams3 **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%.2f %.2f %.2f %.2f %d %d %d %p)\n", This, gamma, contrast, grayscale_contrast, cleartype_level,
        pixel_geometry, rendering_mode, gridfit_mode, params);

    return create_renderingparams(gamma, contrast, grayscale_contrast, cleartype_level, pixel_geometry, rendering_mode,
        gridfit_mode, params);
}

static HRESULT WINAPI dwritefactory3_CreateFontFaceReference_(IDWriteFactory5 *iface, IDWriteFontFile *file,
        UINT32 index, DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFaceReference **reference)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p %u %x %p)\n", This, file, index, simulations, reference);

    return create_fontfacereference(iface, file, index, simulations, reference);
}

static HRESULT WINAPI dwritefactory3_CreateFontFaceReference(IDWriteFactory5 *iface, WCHAR const *path,
        FILETIME const *writetime, UINT32 index, DWRITE_FONT_SIMULATIONS simulations,
        IDWriteFontFaceReference **reference)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    IDWriteFontFile *file;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %x, %p)\n", This, debugstr_w(path), writetime, index, simulations, reference);

    hr = IDWriteFactory5_CreateFontFileReference(iface, path, writetime, &file);
    if (FAILED(hr)) {
        *reference = NULL;
        return hr;
    }

    hr = IDWriteFactory5_CreateFontFaceReference_(iface, file, index, simulations, reference);
    IDWriteFontFile_Release(file);
    return hr;
}

static HRESULT WINAPI dwritefactory3_GetSystemFontSet(IDWriteFactory5 *iface, IDWriteFontSet **fontset)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%p): stub\n", This, fontset);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory3_CreateFontSetBuilder(IDWriteFactory5 *iface, IDWriteFontSetBuilder **builder)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%p): stub\n", This, builder);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory3_CreateFontCollectionFromFontSet(IDWriteFactory5 *iface, IDWriteFontSet *fontset,
    IDWriteFontCollection1 **collection)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%p %p): stub\n", This, fontset, collection);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory3_GetSystemFontCollection(IDWriteFactory5 *iface, BOOL include_downloadable,
    IDWriteFontCollection1 **collection, BOOL check_for_updates)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%d %p %d)\n", This, include_downloadable, collection, check_for_updates);

    if (include_downloadable)
        FIXME("remote fonts are not supported\n");

    if (check_for_updates)
        FIXME("checking for system font updates not implemented\n");

    *collection = factory_get_system_collection(This);

    return *collection ? S_OK : E_FAIL;
}

static HRESULT WINAPI dwritefactory3_GetFontDownloadQueue(IDWriteFactory5 *iface, IDWriteFontDownloadQueue **queue)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%p): stub\n", This, queue);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory4_TranslateColorGlyphRun(IDWriteFactory5 *iface, D2D1_POINT_2F baseline_origin,
        DWRITE_GLYPH_RUN const *run, DWRITE_GLYPH_RUN_DESCRIPTION const *run_desc,
        DWRITE_GLYPH_IMAGE_FORMATS desired_formats, DWRITE_MEASURING_MODE measuring_mode, DWRITE_MATRIX const *transform,
        UINT32 palette, IDWriteColorGlyphRunEnumerator1 **layers)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%p %p %u %d %p %u %p): stub\n", This, run, run_desc, desired_formats, measuring_mode,
        transform, palette, layers);

    return E_NOTIMPL;
}

static HRESULT compute_glyph_origins(DWRITE_GLYPH_RUN const *run, DWRITE_MEASURING_MODE measuring_mode,
    D2D1_POINT_2F baseline_origin, DWRITE_MATRIX const *transform,  D2D1_POINT_2F *origins)
{
    IDWriteFontFace1 *fontface1 = NULL;
    DWRITE_FONT_METRICS metrics;
    FLOAT rtl_factor;
    HRESULT hr;
    UINT32 i;

    rtl_factor = run->bidiLevel & 1 ? -1.0f : 1.0f;

    if (run->fontFace) {
        IDWriteFontFace_GetMetrics(run->fontFace, &metrics);
        if (FAILED(hr = IDWriteFontFace_QueryInterface(run->fontFace, &IID_IDWriteFontFace1, (void **)&fontface1)))
            WARN("Failed to get IDWriteFontFace1, %#x.\n", hr);
    }

    for (i = 0; i < run->glyphCount; i++) {
        FLOAT advance;

        /* Use nominal advances if not provided by caller. */
        if (run->glyphAdvances)
            advance = rtl_factor * run->glyphAdvances[i];
        else {
            INT32 a;

            advance = 0.0f;
            switch (measuring_mode)
            {
            case DWRITE_MEASURING_MODE_NATURAL:
                if (SUCCEEDED(IDWriteFontFace1_GetDesignGlyphAdvances(fontface1, 1, run->glyphIndices + i, &a,
                        run->isSideways)))
                    advance = rtl_factor * get_scaled_advance_width(a, run->fontEmSize, &metrics);
                break;
            case DWRITE_MEASURING_MODE_GDI_CLASSIC:
            case DWRITE_MEASURING_MODE_GDI_NATURAL:
                if (SUCCEEDED(IDWriteFontFace1_GetGdiCompatibleGlyphAdvances(fontface1, run->fontEmSize,
                        1.0f, transform, measuring_mode == DWRITE_MEASURING_MODE_GDI_NATURAL,
                        run->isSideways, 1, run->glyphIndices + i, &a)))
                    advance = rtl_factor * floorf(a * run->fontEmSize / metrics.designUnitsPerEm + 0.5f);
                break;
            default:
                ;
            }
        }

        origins[i] = baseline_origin;

        /* Apply offsets. */
        if (run->glyphOffsets) {
            FLOAT advanceoffset = rtl_factor * run->glyphOffsets[i].advanceOffset;
            FLOAT ascenderoffset = -run->glyphOffsets[i].ascenderOffset;

            if (run->isSideways) {
                origins[i].x += ascenderoffset;
                origins[i].y += advanceoffset;
            }
            else {
                origins[i].x += advanceoffset;
                origins[i].y += ascenderoffset;
            }
        }

        if (run->isSideways)
            baseline_origin.y += advance;
        else
            baseline_origin.x += advance;
    }

    if (fontface1)
        IDWriteFontFace1_Release(fontface1);
    return S_OK;
}

static HRESULT WINAPI dwritefactory4_ComputeGlyphOrigins_(IDWriteFactory5 *iface, DWRITE_GLYPH_RUN const *run,
    D2D1_POINT_2F baseline_origin, D2D1_POINT_2F *origins)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p (%f,%f) %p)\n", This, run, baseline_origin.x, baseline_origin.y, origins);

    return compute_glyph_origins(run, DWRITE_MEASURING_MODE_NATURAL, baseline_origin, NULL, origins);
}

static HRESULT WINAPI dwritefactory4_ComputeGlyphOrigins(IDWriteFactory5 *iface, DWRITE_GLYPH_RUN const *run,
    DWRITE_MEASURING_MODE measuring_mode, D2D1_POINT_2F baseline_origin, DWRITE_MATRIX const *transform,
    D2D1_POINT_2F *origins)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p %d (%f,%f) %p %p)\n", This, run, measuring_mode, baseline_origin.x, baseline_origin.y,
        transform, origins);

    return compute_glyph_origins(run, measuring_mode, baseline_origin, transform, origins);
}

static HRESULT WINAPI dwritefactory5_CreateFontSetBuilder(IDWriteFactory5 *iface, IDWriteFontSetBuilder1 **builder)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%p): stub\n", This, builder);

    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory5_CreateInMemoryFontFileLoader(IDWriteFactory5 *iface, IDWriteFontFileLoader **loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p)\n", This, loader);

    return create_inmemory_fileloader(loader);
}

static HRESULT WINAPI dwritefactory5_CreateHttpFontFileLoader(IDWriteFactory5 *iface, WCHAR const *referrer_url, WCHAR const *extra_headers,
        IDWriteRemoteFontFileLoader **loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%s %s %p): stub\n", This, debugstr_w(referrer_url), wine_dbgstr_w(extra_headers), loader);

    return E_NOTIMPL;
}

static DWRITE_CONTAINER_TYPE WINAPI dwritefactory5_AnalyzeContainerType(IDWriteFactory5 *iface, void const *data, UINT32 data_size)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    TRACE("(%p)->(%p %u)\n", This, data, data_size);

    return opentype_analyze_container_type(data, data_size);
}

static HRESULT WINAPI dwritefactory5_UnpackFontFile(IDWriteFactory5 *iface, DWRITE_CONTAINER_TYPE container_type, void const *data,
        UINT32 data_size, IDWriteFontFileStream **stream)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);

    FIXME("(%p)->(%d %p %u %p): stub\n", This, container_type, data, data_size, stream);

    return E_NOTIMPL;
}

static const struct IDWriteFactory5Vtbl dwritefactoryvtbl = {
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
    dwritefactory_CreateGlyphRunAnalysis,
    dwritefactory1_GetEudcFontCollection,
    dwritefactory1_CreateCustomRenderingParams,
    dwritefactory2_GetSystemFontFallback,
    dwritefactory2_CreateFontFallbackBuilder,
    dwritefactory2_TranslateColorGlyphRun,
    dwritefactory2_CreateCustomRenderingParams,
    dwritefactory2_CreateGlyphRunAnalysis,
    dwritefactory3_CreateGlyphRunAnalysis,
    dwritefactory3_CreateCustomRenderingParams,
    dwritefactory3_CreateFontFaceReference_,
    dwritefactory3_CreateFontFaceReference,
    dwritefactory3_GetSystemFontSet,
    dwritefactory3_CreateFontSetBuilder,
    dwritefactory3_CreateFontCollectionFromFontSet,
    dwritefactory3_GetSystemFontCollection,
    dwritefactory3_GetFontDownloadQueue,
    dwritefactory4_TranslateColorGlyphRun,
    dwritefactory4_ComputeGlyphOrigins_,
    dwritefactory4_ComputeGlyphOrigins,
    dwritefactory5_CreateFontSetBuilder,
    dwritefactory5_CreateInMemoryFontFileLoader,
    dwritefactory5_CreateHttpFontFileLoader,
    dwritefactory5_AnalyzeContainerType,
    dwritefactory5_UnpackFontFile,
};

static ULONG WINAPI shareddwritefactory_AddRef(IDWriteFactory5 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    TRACE("(%p)\n", This);
    return 2;
}

static ULONG WINAPI shareddwritefactory_Release(IDWriteFactory5 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory5(iface);
    TRACE("(%p)\n", This);
    return 1;
}

static const struct IDWriteFactory5Vtbl shareddwritefactoryvtbl = {
    dwritefactory_QueryInterface,
    shareddwritefactory_AddRef,
    shareddwritefactory_Release,
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
    dwritefactory_CreateGlyphRunAnalysis,
    dwritefactory1_GetEudcFontCollection,
    dwritefactory1_CreateCustomRenderingParams,
    dwritefactory2_GetSystemFontFallback,
    dwritefactory2_CreateFontFallbackBuilder,
    dwritefactory2_TranslateColorGlyphRun,
    dwritefactory2_CreateCustomRenderingParams,
    dwritefactory2_CreateGlyphRunAnalysis,
    dwritefactory3_CreateGlyphRunAnalysis,
    dwritefactory3_CreateCustomRenderingParams,
    dwritefactory3_CreateFontFaceReference_,
    dwritefactory3_CreateFontFaceReference,
    dwritefactory3_GetSystemFontSet,
    dwritefactory3_CreateFontSetBuilder,
    dwritefactory3_CreateFontCollectionFromFontSet,
    dwritefactory3_GetSystemFontCollection,
    dwritefactory3_GetFontDownloadQueue,
    dwritefactory4_TranslateColorGlyphRun,
    dwritefactory4_ComputeGlyphOrigins_,
    dwritefactory4_ComputeGlyphOrigins,
    dwritefactory5_CreateFontSetBuilder,
    dwritefactory5_CreateInMemoryFontFileLoader,
    dwritefactory5_CreateHttpFontFileLoader,
    dwritefactory5_AnalyzeContainerType,
    dwritefactory5_UnpackFontFile,
};

static void init_dwritefactory(struct dwritefactory *factory, DWRITE_FACTORY_TYPE type)
{
    factory->IDWriteFactory5_iface.lpVtbl = type == DWRITE_FACTORY_TYPE_SHARED ?
            &shareddwritefactoryvtbl : &dwritefactoryvtbl;
    factory->ref = 1;
    factory->localfontfileloader = get_local_fontfile_loader();
    factory->system_collection = NULL;
    factory->eudc_collection = NULL;
    factory->gdiinterop = NULL;
    factory->fallback = NULL;

    list_init(&factory->collection_loaders);
    list_init(&factory->file_loaders);
    list_init(&factory->localfontfaces);

    InitializeCriticalSection(&factory->cs);
    factory->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": dwritefactory.lock");
}

void factory_detach_fontcollection(IDWriteFactory5 *iface, IDWriteFontCollection1 *collection)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory5(iface);
    InterlockedCompareExchangePointer((void **)&factory->system_collection, NULL, collection);
    InterlockedCompareExchangePointer((void **)&factory->eudc_collection, NULL, collection);
    IDWriteFactory5_Release(iface);
}

void factory_detach_gdiinterop(IDWriteFactory5 *iface, IDWriteGdiInterop1 *interop)
{
    struct dwritefactory *factory = impl_from_IDWriteFactory5(iface);
    factory->gdiinterop = NULL;
    IDWriteFactory5_Release(iface);
}

HRESULT WINAPI DWriteCreateFactory(DWRITE_FACTORY_TYPE type, REFIID riid, IUnknown **ret)
{
    struct dwritefactory *factory;
    HRESULT hr;

    TRACE("(%d, %s, %p)\n", type, debugstr_guid(riid), ret);

    *ret = NULL;

    if (type == DWRITE_FACTORY_TYPE_SHARED && shared_factory)
        return IDWriteFactory5_QueryInterface(shared_factory, riid, (void**)ret);

    factory = heap_alloc(sizeof(struct dwritefactory));
    if (!factory) return E_OUTOFMEMORY;

    init_dwritefactory(factory, type);

    if (type == DWRITE_FACTORY_TYPE_SHARED)
        if (InterlockedCompareExchangePointer((void**)&shared_factory, &factory->IDWriteFactory5_iface, NULL)) {
            release_shared_factory(&factory->IDWriteFactory5_iface);
            return IDWriteFactory5_QueryInterface(shared_factory, riid, (void**)ret);
        }

    hr = IDWriteFactory5_QueryInterface(&factory->IDWriteFactory5_iface, riid, (void**)ret);
    IDWriteFactory5_Release(&factory->IDWriteFactory5_iface);
    return hr;
}
