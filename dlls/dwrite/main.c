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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "initguid.h"

#include "dwrite_private.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

static IDWriteFactory *shared_factory;
static void release_shared_factory(IDWriteFactory*);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        release_shared_factory(shared_factory);
    }
    return TRUE;
}

struct renderingparams {
    IDWriteRenderingParams IDWriteRenderingParams_iface;
    LONG ref;

    FLOAT gamma;
    FLOAT enh_contrast;
    FLOAT cleartype_level;
    DWRITE_PIXEL_GEOMETRY geometry;
    DWRITE_RENDERING_MODE mode;
};

static inline struct renderingparams *impl_from_IDWriteRenderingParams(IDWriteRenderingParams *iface)
{
    return CONTAINING_RECORD(iface, struct renderingparams, IDWriteRenderingParams_iface);
}

static HRESULT WINAPI renderingparams_QueryInterface(IDWriteRenderingParams *iface, REFIID riid, void **obj)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteRenderingParams))
    {
        *obj = iface;
        IDWriteRenderingParams_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI renderingparams_AddRef(IDWriteRenderingParams *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI renderingparams_Release(IDWriteRenderingParams *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static FLOAT WINAPI renderingparams_GetGamma(IDWriteRenderingParams *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);
    TRACE("(%p)\n", This);
    return This->gamma;
}

static FLOAT WINAPI renderingparams_GetEnhancedContrast(IDWriteRenderingParams *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);
    TRACE("(%p)\n", This);
    return This->enh_contrast;
}

static FLOAT WINAPI renderingparams_GetClearTypeLevel(IDWriteRenderingParams *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);
    TRACE("(%p)\n", This);
    return This->cleartype_level;
}

static DWRITE_PIXEL_GEOMETRY WINAPI renderingparams_GetPixelGeometry(IDWriteRenderingParams *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);
    TRACE("(%p)\n", This);
    return This->geometry;
}

static DWRITE_RENDERING_MODE WINAPI renderingparams_GetRenderingMode(IDWriteRenderingParams *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams(iface);
    TRACE("(%p)\n", This);
    return This->mode;
}

static const struct IDWriteRenderingParamsVtbl renderingparamsvtbl = {
    renderingparams_QueryInterface,
    renderingparams_AddRef,
    renderingparams_Release,
    renderingparams_GetGamma,
    renderingparams_GetEnhancedContrast,
    renderingparams_GetClearTypeLevel,
    renderingparams_GetPixelGeometry,
    renderingparams_GetRenderingMode
};

static HRESULT create_renderingparams(FLOAT gamma, FLOAT enhancedContrast, FLOAT cleartype_level,
    DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode, IDWriteRenderingParams **params)
{
    struct renderingparams *This;

    *params = NULL;

    This = heap_alloc(sizeof(struct renderingparams));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteRenderingParams_iface.lpVtbl = &renderingparamsvtbl;
    This->ref = 1;

    This->gamma = gamma;
    This->enh_contrast = enhancedContrast;
    This->cleartype_level = cleartype_level;
    This->geometry = geometry;
    This->mode = mode;

    *params = &This->IDWriteRenderingParams_iface;

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
    FIXME("(%p)->(%s %p %p): stub\n", This, debugstr_w(locale_name), index, exists);
    return E_NOTIMPL;
}

static HRESULT WINAPI localizedstrings_GetLocaleNameLength(IDWriteLocalizedStrings *iface, UINT32 index, UINT32 *length)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    FIXME("(%p)->(%u %p): stub\n", This, index, length);
    return E_NOTIMPL;
}

static HRESULT WINAPI localizedstrings_GetLocaleName(IDWriteLocalizedStrings *iface, UINT32 index, WCHAR *locale_name, UINT32 size)
{
    struct localizedstrings *This = impl_from_IDWriteLocalizedStrings(iface);
    FIXME("(%p)->(%u %p %u): stub\n", This, index, locale_name, size);
    return E_NOTIMPL;
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

    if (This->count == This->alloc) {
        This->alloc *= 2;
        This->data = heap_realloc(This->data, This->alloc*sizeof(struct localizedpair));
    }

    This->data[This->count].locale = heap_strdupW(locale);
    This->data[This->count].string = heap_strdupW(string);
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

struct collectionloader
{
    struct list entry;
    IDWriteFontCollectionLoader *loader;
};

struct fontfacecached
{
    struct list entry;
    IDWriteFontFace *fontface;
};

struct fileloader
{
    struct list entry;
    struct list fontfaces;
    IDWriteFontFileLoader *loader;
};

struct dwritefactory {
    IDWriteFactory IDWriteFactory_iface;
    LONG ref;

    IDWriteFontCollection *system_collection;
    IDWriteGdiInterop *gdiinterop;

    IDWriteLocalFontFileLoader* localfontfileloader;
    struct list localfontfaces;

    struct list collection_loaders;
    struct list file_loaders;
};

static inline struct dwritefactory *impl_from_IDWriteFactory(IDWriteFactory *iface)
{
    return CONTAINING_RECORD(iface, struct dwritefactory, IDWriteFactory_iface);
}

static void release_fontface_cache(struct list *fontfaces)
{
    struct fontfacecached *fontface, *fontface2;
    LIST_FOR_EACH_ENTRY_SAFE(fontface, fontface2, fontfaces, struct fontfacecached, entry) {
        list_remove(&fontface->entry);
        IDWriteFontFace_Release(fontface->fontface);
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

    if (factory->localfontfileloader)
        IDWriteLocalFontFileLoader_Release(factory->localfontfileloader);
    release_fontface_cache(&factory->localfontfaces);

    LIST_FOR_EACH_ENTRY_SAFE(loader, loader2, &factory->collection_loaders, struct collectionloader, entry) {
        list_remove(&loader->entry);
        IDWriteFontCollectionLoader_Release(loader->loader);
        heap_free(loader);
    }

    LIST_FOR_EACH_ENTRY_SAFE(fileloader, fileloader2, &factory->file_loaders, struct fileloader, entry)
        release_fileloader(fileloader);

    if (factory->system_collection)
        IDWriteFontCollection_Release(factory->system_collection);
    if (factory->gdiinterop)
        release_gdiinterop(factory->gdiinterop);
    heap_free(factory);
}

static void release_shared_factory(IDWriteFactory *iface)
{
    struct dwritefactory *factory;
    if (!iface) return;
    factory = impl_from_IDWriteFactory(iface);
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

static struct collectionloader *factory_get_collection_loader(struct dwritefactory *factory, IDWriteFontCollectionLoader *loader)
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

static HRESULT WINAPI dwritefactory_QueryInterface(IDWriteFactory *iface, REFIID riid, void **obj)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDWriteFactory))
    {
        *obj = iface;
        IDWriteFactory_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefactory_AddRef(IDWriteFactory *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefactory_Release(IDWriteFactory *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        release_dwritefactory(This);

    return ref;
}

static HRESULT WINAPI dwritefactory_GetSystemFontCollection(IDWriteFactory *iface,
    IDWriteFontCollection **collection, BOOL check_for_updates)
{
    HRESULT hr = S_OK;
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%p %d)\n", This, collection, check_for_updates);

    if (check_for_updates)
        FIXME("checking for system font updates not implemented\n");

    if (!This->system_collection)
        hr = get_system_fontcollection(iface, &This->system_collection);

    if (SUCCEEDED(hr))
        IDWriteFontCollection_AddRef(This->system_collection);

    *collection = This->system_collection;

    return hr;
}

static HRESULT WINAPI dwritefactory_CreateCustomFontCollection(IDWriteFactory *iface,
    IDWriteFontCollectionLoader *loader, void const *key, UINT32 key_size, IDWriteFontCollection **collection)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
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

    hr = IDWriteFontCollectionLoader_CreateEnumeratorFromKey(found->loader, iface, key, key_size, &enumerator);
    if (FAILED(hr))
        return hr;

    hr = create_font_collection(iface, enumerator, collection);
    IDWriteFontFileEnumerator_Release(enumerator);
    return hr;
}

static HRESULT WINAPI dwritefactory_RegisterFontCollectionLoader(IDWriteFactory *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
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

static HRESULT WINAPI dwritefactory_UnregisterFontCollectionLoader(IDWriteFactory *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
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

static HRESULT WINAPI dwritefactory_CreateFontFileReference(IDWriteFactory *iface,
    WCHAR const *path, FILETIME const *writetime, IDWriteFontFile **font_file)
{
    HRESULT hr;
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(path), writetime, font_file);

    if (!This->localfontfileloader)
    {
        hr = create_localfontfileloader(&This->localfontfileloader);
        if (FAILED(hr))
            return hr;
    }
    return create_font_file((IDWriteFontFileLoader*)This->localfontfileloader, path, sizeof(WCHAR) * (strlenW(path)+1), font_file);
}

static HRESULT WINAPI dwritefactory_CreateCustomFontFileReference(IDWriteFactory *iface,
    void const *reference_key, UINT32 key_size, IDWriteFontFileLoader *loader, IDWriteFontFile **font_file)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);

    TRACE("(%p)->(%p %u %p %p)\n", This, reference_key, key_size, loader, font_file);

    if (!loader || !factory_get_file_loader(This, loader))
        return E_INVALIDARG;

    return create_font_file(loader, reference_key, key_size, font_file);
}

static HRESULT WINAPI dwritefactory_CreateFontFace(IDWriteFactory *iface,
    DWRITE_FONT_FACE_TYPE facetype, UINT32 files_number, IDWriteFontFile* const* font_files,
    UINT32 index, DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFace **font_face)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    IDWriteFontFileLoader *loader;
    struct fontfacecached *cached;
    struct list *fontfaces;
    IDWriteFontFace2 *face;
    const void *key;
    UINT32 key_size;
    HRESULT hr;

    TRACE("(%p)->(%d %u %p %u 0x%x %p)\n", This, facetype, files_number, font_files, index, simulations, font_face);

    *font_face = NULL;

    if (facetype != DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION && index)
        return E_INVALIDARG;

    hr = IDWriteFontFile_GetReferenceKey(*font_files, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFile_GetLoader(*font_files, &loader);
    if (FAILED(hr))
        return hr;

    if (loader == (IDWriteFontFileLoader*)This->localfontfileloader) {
        fontfaces = &This->localfontfaces;
        IDWriteFontFileLoader_Release(loader);
    }
    else {
        struct fileloader *fileloader = factory_get_file_loader(This, loader);
        IDWriteFontFileLoader_Release(loader);
        if (!fileloader)
            return E_INVALIDARG;
        fontfaces = &fileloader->fontfaces;
    }

    /* search through cache list */
    LIST_FOR_EACH_ENTRY(cached, fontfaces, struct fontfacecached, entry) {
        UINT32 cached_key_size, count = 1, cached_face_index;
        DWRITE_FONT_SIMULATIONS cached_simulations;
        const void *cached_key;
        IDWriteFontFile *file;

        cached_face_index = IDWriteFontFace_GetIndex(cached->fontface);
        cached_simulations = IDWriteFontFace_GetSimulations(cached->fontface);

        /* skip earlier */
        if (cached_face_index != index || cached_simulations != simulations)
            continue;

        hr = IDWriteFontFace_GetFiles(cached->fontface, &count, &file);
        if (FAILED(hr))
            return hr;

        hr = IDWriteFontFile_GetReferenceKey(file, &cached_key, &cached_key_size);
        IDWriteFontFile_Release(file);
        if (FAILED(hr))
            return hr;

        if (cached_key_size == key_size && !memcmp(cached_key, key, key_size)) {
            TRACE("returning cached fontface %p\n", cached->fontface);
            *font_face = cached->fontface;
            IDWriteFontFace_AddRef(*font_face);
            return S_OK;
        }
    }

    hr = create_fontface(facetype, files_number, font_files, index, simulations, &face);
    if (FAILED(hr))
        return hr;

    /* new cache entry */
    cached = heap_alloc(sizeof(*cached));
    if (!cached) {
        IDWriteFontFace2_Release(face);
        return hr;
    }

    cached->fontface = (IDWriteFontFace*)face;
    list_add_tail(fontfaces, &cached->entry);

    *font_face = cached->fontface;
    IDWriteFontFace_AddRef(*font_face);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateRenderingParams(IDWriteFactory *iface, IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    HMONITOR monitor;
    POINT pt;

    TRACE("(%p)->(%p)\n", This, params);

    pt.x = pt.y = 0;
    monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    return IDWriteFactory_CreateMonitorRenderingParams(iface, monitor, params);
}

static HRESULT WINAPI dwritefactory_CreateMonitorRenderingParams(IDWriteFactory *iface, HMONITOR monitor,
    IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    static int fixme_once = 0;

    TRACE("(%p)->(%p %p)\n", This, monitor, params);

    if (!fixme_once++)
        FIXME("(%p): monitor setting ignored\n", monitor);
    return IDWriteFactory_CreateCustomRenderingParams(iface, 0.0, 0.0, 0.0, DWRITE_PIXEL_GEOMETRY_FLAT,
        DWRITE_RENDERING_MODE_DEFAULT, params);
}

static HRESULT WINAPI dwritefactory_CreateCustomRenderingParams(IDWriteFactory *iface, FLOAT gamma, FLOAT enhancedContrast,
    FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode, IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%f %f %f %d %d %p)\n", This, gamma, enhancedContrast, cleartype_level, geometry, mode, params);
    return create_renderingparams(gamma, enhancedContrast, cleartype_level, geometry, mode, params);
}

static HRESULT WINAPI dwritefactory_RegisterFontFileLoader(IDWriteFactory *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    struct fileloader *entry;

    TRACE("(%p)->(%p)\n", This, loader);

    if (!loader)
        return E_INVALIDARG;

    if ((IDWriteFontFileLoader*)This->localfontfileloader == loader)
        return S_OK;

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

static HRESULT WINAPI dwritefactory_UnregisterFontFileLoader(IDWriteFactory *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    struct fileloader *found;

    TRACE("(%p)->(%p)\n", This, loader);

    if (!loader)
        return E_INVALIDARG;

    if ((IDWriteFontFileLoader*)This->localfontfileloader == loader)
        return S_OK;

    found = factory_get_file_loader(This, loader);
    if (!found)
        return E_INVALIDARG;

    release_fileloader(found);
    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateTextFormat(IDWriteFactory *iface, WCHAR const* family_name,
    IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, WCHAR const *locale, IDWriteTextFormat **format)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%s %p %d %d %d %f %s %p)\n", This, debugstr_w(family_name), collection, weight, style, stretch,
        size, debugstr_w(locale), format);

    if (!collection)
    {
        HRESULT hr = IDWriteFactory_GetSystemFontCollection(iface, &collection, FALSE);
        if (hr != S_OK)
            return hr;
        /* Our ref count is 1 too many, since we will add ref in create_textformat */
        IDWriteFontCollection_Release(This->system_collection);
    }
    return create_textformat(family_name, collection, weight, style, stretch, size, locale, format);
}

static HRESULT WINAPI dwritefactory_CreateTypography(IDWriteFactory *iface, IDWriteTypography **typography)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%p)\n", This, typography);
    return create_typography(typography);
}

static HRESULT WINAPI dwritefactory_GetGdiInterop(IDWriteFactory *iface, IDWriteGdiInterop **gdi_interop)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);

    TRACE("(%p)->(%p)\n", This, gdi_interop);

    *gdi_interop = NULL;

    if (!This->gdiinterop) {
        HRESULT hr = create_gdiinterop(iface, &This->gdiinterop);
        if (FAILED(hr))
            return hr;
    }

    *gdi_interop = This->gdiinterop;
    IDWriteGdiInterop_AddRef(*gdi_interop);

    return S_OK;
}

static HRESULT WINAPI dwritefactory_CreateTextLayout(IDWriteFactory *iface, WCHAR const* string,
    UINT32 len, IDWriteTextFormat *format, FLOAT max_width, FLOAT max_height, IDWriteTextLayout **layout)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%s %u %p %f %f %p)\n", This, debugstr_w(string), len, format, max_width, max_height, layout);

    if (!format) return E_INVALIDARG;
    return create_textlayout(string, len, format, max_width, max_height, layout);
}

static HRESULT WINAPI dwritefactory_CreateGdiCompatibleTextLayout(IDWriteFactory *iface, WCHAR const* string,
    UINT32 len, IDWriteTextFormat *format, FLOAT layout_width, FLOAT layout_height, FLOAT pixels_per_dip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, IDWriteTextLayout **layout)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    FIXME("(%p)->(%s:%u %p %f %f %f %p %d %p): semi-stub\n", This, debugstr_wn(string, len), len, format, layout_width, layout_height,
        pixels_per_dip, transform, use_gdi_natural, layout);

    if (!format) return E_INVALIDARG;
    return create_textlayout(string, len, format, layout_width, layout_height, layout);
}

static HRESULT WINAPI dwritefactory_CreateEllipsisTrimmingSign(IDWriteFactory *iface, IDWriteTextFormat *format,
    IDWriteInlineObject **trimming_sign)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    FIXME("(%p)->(%p %p): semi-stub\n", This, format, trimming_sign);
    return create_trimmingsign(trimming_sign);
}

static HRESULT WINAPI dwritefactory_CreateTextAnalyzer(IDWriteFactory *iface, IDWriteTextAnalyzer **analyzer)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%p)\n", This, analyzer);
    return get_textanalyzer(analyzer);
}

static HRESULT WINAPI dwritefactory_CreateNumberSubstitution(IDWriteFactory *iface, DWRITE_NUMBER_SUBSTITUTION_METHOD method,
    WCHAR const* locale, BOOL ignore_user_override, IDWriteNumberSubstitution **substitution)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)->(%d %s %d %p)\n", This, method, debugstr_w(locale), ignore_user_override, substitution);
    return create_numbersubstitution(method, locale, ignore_user_override, substitution);
}

static HRESULT WINAPI dwritefactory_CreateGlyphRunAnalysis(IDWriteFactory *iface, DWRITE_GLYPH_RUN const *glyph_run,
    FLOAT pixels_per_dip, DWRITE_MATRIX const* transform, DWRITE_RENDERING_MODE rendering_mode,
    DWRITE_MEASURING_MODE measuring_mode, FLOAT baseline_x, FLOAT baseline_y, IDWriteGlyphRunAnalysis **analysis)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    FIXME("(%p)->(%p %f %p %d %d %f %f %p): stub\n", This, glyph_run, pixels_per_dip, transform, rendering_mode,
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

static ULONG WINAPI shareddwritefactory_AddRef(IDWriteFactory *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)\n", This);
    return 2;
}

static ULONG WINAPI shareddwritefactory_Release(IDWriteFactory *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory(iface);
    TRACE("(%p)\n", This);
    return 1;
}

static const struct IDWriteFactoryVtbl shareddwritefactoryvtbl = {
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
    dwritefactory_CreateGlyphRunAnalysis
};

static void init_dwritefactory(struct dwritefactory *factory, const struct IDWriteFactoryVtbl *vtbl)
{
    factory->IDWriteFactory_iface.lpVtbl = vtbl;
    factory->ref = 1;
    factory->localfontfileloader = NULL;
    factory->system_collection = NULL;
    factory->gdiinterop = NULL;

    list_init(&factory->collection_loaders);
    list_init(&factory->file_loaders);
    list_init(&factory->localfontfaces);
}

HRESULT WINAPI DWriteCreateFactory(DWRITE_FACTORY_TYPE type, REFIID riid, IUnknown **ret)
{
    struct dwritefactory *factory;

    TRACE("(%d, %s, %p)\n", type, debugstr_guid(riid), ret);

    *ret = NULL;

    if (!IsEqualIID(riid, &IID_IDWriteFactory)) return E_FAIL;

    if (type == DWRITE_FACTORY_TYPE_SHARED && shared_factory) {
        *ret = (IUnknown*)shared_factory;
        IDWriteFactory_AddRef(shared_factory);
        return S_OK;
    }

    factory = heap_alloc(sizeof(struct dwritefactory));
    if (!factory) return E_OUTOFMEMORY;

    init_dwritefactory(factory, type == DWRITE_FACTORY_TYPE_SHARED ? &shareddwritefactoryvtbl : &dwritefactoryvtbl);

    if (type == DWRITE_FACTORY_TYPE_SHARED)
        if (InterlockedCompareExchangePointer((void**)&shared_factory, factory, NULL)) {
            release_shared_factory(&factory->IDWriteFactory_iface);
            *ret = (IUnknown*)shared_factory;
            IDWriteFactory_AddRef(shared_factory);
            return S_OK;
        }

    *ret = (IUnknown*)&factory->IDWriteFactory_iface;
    return S_OK;
}
