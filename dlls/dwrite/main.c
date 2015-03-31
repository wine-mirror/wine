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

static IDWriteFactory2 *shared_factory;
static void release_shared_factory(IDWriteFactory2*);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        init_freetype();
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        release_shared_factory(shared_factory);
        release_freetype();
    }
    return TRUE;
}

struct renderingparams {
    IDWriteRenderingParams2 IDWriteRenderingParams2_iface;
    LONG ref;

    FLOAT gamma;
    FLOAT contrast;
    FLOAT grayscalecontrast;
    FLOAT cleartype_level;
    DWRITE_PIXEL_GEOMETRY geometry;
    DWRITE_RENDERING_MODE mode;
    DWRITE_GRID_FIT_MODE gridfit;
};

static inline struct renderingparams *impl_from_IDWriteRenderingParams2(IDWriteRenderingParams2 *iface)
{
    return CONTAINING_RECORD(iface, struct renderingparams, IDWriteRenderingParams2_iface);
}

static HRESULT WINAPI renderingparams_QueryInterface(IDWriteRenderingParams2 *iface, REFIID riid, void **obj)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteRenderingParams2) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams1) ||
        IsEqualIID(riid, &IID_IDWriteRenderingParams) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteRenderingParams2_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI renderingparams_AddRef(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI renderingparams_Release(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static FLOAT WINAPI renderingparams_GetGamma(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    TRACE("(%p)\n", This);
    return This->gamma;
}

static FLOAT WINAPI renderingparams_GetEnhancedContrast(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    TRACE("(%p)\n", This);
    return This->contrast;
}

static FLOAT WINAPI renderingparams_GetClearTypeLevel(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    TRACE("(%p)\n", This);
    return This->cleartype_level;
}

static DWRITE_PIXEL_GEOMETRY WINAPI renderingparams_GetPixelGeometry(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    TRACE("(%p)\n", This);
    return This->geometry;
}

static DWRITE_RENDERING_MODE WINAPI renderingparams_GetRenderingMode(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    TRACE("(%p)\n", This);
    return This->mode;
}

static FLOAT WINAPI renderingparams_GetGrayscaleEnhancedContrast(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    TRACE("(%p)\n", This);
    return This->grayscalecontrast;
}

static DWRITE_GRID_FIT_MODE WINAPI renderingparams_GetGridFitMode(IDWriteRenderingParams2 *iface)
{
    struct renderingparams *This = impl_from_IDWriteRenderingParams2(iface);
    TRACE("(%p)\n", This);
    return This->gridfit;
}

static const struct IDWriteRenderingParams2Vtbl renderingparamsvtbl = {
    renderingparams_QueryInterface,
    renderingparams_AddRef,
    renderingparams_Release,
    renderingparams_GetGamma,
    renderingparams_GetEnhancedContrast,
    renderingparams_GetClearTypeLevel,
    renderingparams_GetPixelGeometry,
    renderingparams_GetRenderingMode,
    renderingparams_GetGrayscaleEnhancedContrast,
    renderingparams_GetGridFitMode
};

static HRESULT create_renderingparams(FLOAT gamma, FLOAT contrast, FLOAT grayscalecontrast, FLOAT cleartype_level,
    DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode, DWRITE_GRID_FIT_MODE gridfit, IDWriteRenderingParams2 **params)
{
    struct renderingparams *This;

    *params = NULL;

    This = heap_alloc(sizeof(struct renderingparams));
    if (!This) return E_OUTOFMEMORY;

    This->IDWriteRenderingParams2_iface.lpVtbl = &renderingparamsvtbl;
    This->ref = 1;

    This->gamma = gamma;
    This->contrast = contrast;
    This->grayscalecontrast = grayscalecontrast;
    This->cleartype_level = cleartype_level;
    This->geometry = geometry;
    This->mode = mode;
    This->gridfit = gridfit;

    *params = &This->IDWriteRenderingParams2_iface;

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
    IDWriteFactory2 IDWriteFactory2_iface;
    LONG ref;

    IDWriteFontCollection *system_collection;
    IDWriteFontCollection *eudc_collection;
    IDWriteGdiInterop *gdiinterop;

    IDWriteLocalFontFileLoader* localfontfileloader;
    struct list localfontfaces;

    struct list collection_loaders;
    struct list file_loaders;
};

static inline struct dwritefactory *impl_from_IDWriteFactory2(IDWriteFactory2 *iface)
{
    return CONTAINING_RECORD(iface, struct dwritefactory, IDWriteFactory2_iface);
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
    if (factory->eudc_collection)
        IDWriteFontCollection_Release(factory->eudc_collection);
    if (factory->gdiinterop)
        release_gdiinterop(factory->gdiinterop);
    heap_free(factory);
}

static void release_shared_factory(IDWriteFactory2 *iface)
{
    struct dwritefactory *factory;
    if (!iface) return;
    factory = impl_from_IDWriteFactory2(iface);
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

static HRESULT WINAPI dwritefactory_QueryInterface(IDWriteFactory2 *iface, REFIID riid, void **obj)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFactory2) ||
        IsEqualIID(riid, &IID_IDWriteFactory1) ||
        IsEqualIID(riid, &IID_IDWriteFactory) ||
        IsEqualIID(riid, &IID_IUnknown))
   {
        *obj = iface;
        IDWriteFactory2_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritefactory_AddRef(IDWriteFactory2 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritefactory_Release(IDWriteFactory2 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        release_dwritefactory(This);

    return ref;
}

static HRESULT WINAPI dwritefactory_GetSystemFontCollection(IDWriteFactory2 *iface,
    IDWriteFontCollection **collection, BOOL check_for_updates)
{
    HRESULT hr = S_OK;
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
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

static HRESULT WINAPI dwritefactory_CreateCustomFontCollection(IDWriteFactory2 *iface,
    IDWriteFontCollectionLoader *loader, void const *key, UINT32 key_size, IDWriteFontCollection **collection)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
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

    hr = IDWriteFontCollectionLoader_CreateEnumeratorFromKey(found->loader, (IDWriteFactory*)iface, key, key_size, &enumerator);
    if (FAILED(hr))
        return hr;

    hr = create_font_collection(iface, enumerator, FALSE, collection);
    IDWriteFontFileEnumerator_Release(enumerator);
    return hr;
}

static HRESULT WINAPI dwritefactory_RegisterFontCollectionLoader(IDWriteFactory2 *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
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

static HRESULT WINAPI dwritefactory_UnregisterFontCollectionLoader(IDWriteFactory2 *iface,
    IDWriteFontCollectionLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
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

static HRESULT WINAPI dwritefactory_CreateFontFileReference(IDWriteFactory2 *iface,
    WCHAR const *path, FILETIME const *writetime, IDWriteFontFile **font_file)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    UINT32 key_size;
    HRESULT hr;
    void *key;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(path), writetime, font_file);

    if (!This->localfontfileloader)
    {
        hr = create_localfontfileloader(&This->localfontfileloader);
        if (FAILED(hr))
            return hr;
    }

    /* get a reference key used by local loader */
    hr = get_local_refkey(path, writetime, &key, &key_size);
    if (FAILED(hr))
        return hr;

    hr = create_font_file((IDWriteFontFileLoader*)This->localfontfileloader, key, key_size, font_file);
    heap_free(key);

    return hr;
}

static HRESULT WINAPI dwritefactory_CreateCustomFontFileReference(IDWriteFactory2 *iface,
    void const *reference_key, UINT32 key_size, IDWriteFontFileLoader *loader, IDWriteFontFile **font_file)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);

    TRACE("(%p)->(%p %u %p %p)\n", This, reference_key, key_size, loader, font_file);

    if (!loader || !factory_get_file_loader(This, loader))
        return E_INVALIDARG;

    return create_font_file(loader, reference_key, key_size, font_file);
}

static HRESULT WINAPI dwritefactory_CreateFontFace(IDWriteFactory2 *iface,
    DWRITE_FONT_FACE_TYPE req_facetype, UINT32 files_number, IDWriteFontFile* const* font_files,
    UINT32 index, DWRITE_FONT_SIMULATIONS simulations, IDWriteFontFace **font_face)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFileLoader *loader;
    struct fontfacecached *cached;
    struct list *fontfaces;
    IDWriteFontFace2 *face;
    UINT32 key_size, count;
    BOOL is_supported;
    const void *key;
    HRESULT hr;

    TRACE("(%p)->(%d %u %p %u 0x%x %p)\n", This, req_facetype, files_number, font_files, index, simulations, font_face);

    *font_face = NULL;

    if (!is_face_type_supported(req_facetype))
        return E_INVALIDARG;

    if (req_facetype != DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION && index)
        return E_INVALIDARG;

    /* check actual file/face type */
    is_supported = FALSE;
    face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    hr = IDWriteFontFile_Analyze(*font_files, &is_supported, &file_type, &face_type, &count);
    if (FAILED(hr))
        return hr;

    if (!is_supported || (face_type != req_facetype))
        return E_FAIL;

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

    hr = create_fontface(req_facetype, files_number, font_files, index, simulations, &face);
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

static HRESULT WINAPI dwritefactory_CreateRenderingParams(IDWriteFactory2 *iface, IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    HMONITOR monitor;
    POINT pt;

    TRACE("(%p)->(%p)\n", This, params);

    pt.x = pt.y = 0;
    monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    return IDWriteFactory2_CreateMonitorRenderingParams(iface, monitor, params);
}

static HRESULT WINAPI dwritefactory_CreateMonitorRenderingParams(IDWriteFactory2 *iface, HMONITOR monitor,
    IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    IDWriteRenderingParams2 *params2;
    static int fixme_once = 0;
    HRESULT hr;

    TRACE("(%p)->(%p %p)\n", This, monitor, params);

    if (!fixme_once++)
        FIXME("(%p): monitor setting ignored\n", monitor);

    hr = IDWriteFactory2_CreateCustomRenderingParams(iface, 0.0, 0.0, 1.0, 0.0, DWRITE_PIXEL_GEOMETRY_FLAT, DWRITE_RENDERING_MODE_DEFAULT,
        DWRITE_GRID_FIT_MODE_DEFAULT, &params2);
    *params = (IDWriteRenderingParams*)params2;
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateCustomRenderingParams(IDWriteFactory2 *iface, FLOAT gamma, FLOAT enhancedContrast,
    FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode, IDWriteRenderingParams **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    IDWriteRenderingParams2 *params2;
    HRESULT hr;

    TRACE("(%p)->(%f %f %f %d %d %p)\n", This, gamma, enhancedContrast, cleartype_level, geometry, mode, params);

    hr = IDWriteFactory2_CreateCustomRenderingParams(iface, gamma, enhancedContrast, 1.0, cleartype_level, geometry,
        mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params2);
    *params = (IDWriteRenderingParams*)params2;
    return hr;
}

static HRESULT WINAPI dwritefactory_RegisterFontFileLoader(IDWriteFactory2 *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
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

static HRESULT WINAPI dwritefactory_UnregisterFontFileLoader(IDWriteFactory2 *iface, IDWriteFontFileLoader *loader)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
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

static HRESULT WINAPI dwritefactory_CreateTextFormat(IDWriteFactory2 *iface, WCHAR const* family_name,
    IDWriteFontCollection *collection, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch, FLOAT size, WCHAR const *locale, IDWriteTextFormat **format)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    IDWriteFontCollection *syscollection = NULL;
    HRESULT hr;

    TRACE("(%p)->(%s %p %d %d %d %f %s %p)\n", This, debugstr_w(family_name), collection, weight, style, stretch,
        size, debugstr_w(locale), format);

    if (!collection) {
        hr = IDWriteFactory2_GetSystemFontCollection(iface, &syscollection, FALSE);
        if (FAILED(hr))
            return hr;
    }

    hr = create_textformat(family_name, collection ? collection : syscollection, weight, style, stretch, size, locale, format);
    if (syscollection)
        IDWriteFontCollection_Release(syscollection);
    return hr;
}

static HRESULT WINAPI dwritefactory_CreateTypography(IDWriteFactory2 *iface, IDWriteTypography **typography)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    TRACE("(%p)->(%p)\n", This, typography);
    return create_typography(typography);
}

static HRESULT WINAPI dwritefactory_GetGdiInterop(IDWriteFactory2 *iface, IDWriteGdiInterop **gdi_interop)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);

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

static HRESULT WINAPI dwritefactory_CreateTextLayout(IDWriteFactory2 *iface, WCHAR const* string,
    UINT32 len, IDWriteTextFormat *format, FLOAT max_width, FLOAT max_height, IDWriteTextLayout **layout)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    TRACE("(%p)->(%s %u %p %f %f %p)\n", This, debugstr_w(string), len, format, max_width, max_height, layout);

    if (!format) return E_INVALIDARG;
    return create_textlayout(string, len, format, max_width, max_height, layout);
}

static HRESULT WINAPI dwritefactory_CreateGdiCompatibleTextLayout(IDWriteFactory2 *iface, WCHAR const* string,
    UINT32 len, IDWriteTextFormat *format, FLOAT layout_width, FLOAT layout_height, FLOAT pixels_per_dip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, IDWriteTextLayout **layout)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);

    TRACE("(%p)->(%s:%u %p %f %f %f %p %d %p)\n", This, debugstr_wn(string, len), len, format, layout_width, layout_height,
        pixels_per_dip, transform, use_gdi_natural, layout);

    if (!format) return E_INVALIDARG;
    return create_gdicompat_textlayout(string, len, format, layout_width, layout_height, pixels_per_dip, transform,
        use_gdi_natural, layout);
}

static HRESULT WINAPI dwritefactory_CreateEllipsisTrimmingSign(IDWriteFactory2 *iface, IDWriteTextFormat *format,
    IDWriteInlineObject **trimming_sign)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    FIXME("(%p)->(%p %p): semi-stub\n", This, format, trimming_sign);
    return create_trimmingsign(trimming_sign);
}

static HRESULT WINAPI dwritefactory_CreateTextAnalyzer(IDWriteFactory2 *iface, IDWriteTextAnalyzer **analyzer)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    TRACE("(%p)->(%p)\n", This, analyzer);
    return get_textanalyzer(analyzer);
}

static HRESULT WINAPI dwritefactory_CreateNumberSubstitution(IDWriteFactory2 *iface, DWRITE_NUMBER_SUBSTITUTION_METHOD method,
    WCHAR const* locale, BOOL ignore_user_override, IDWriteNumberSubstitution **substitution)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    TRACE("(%p)->(%d %s %d %p)\n", This, method, debugstr_w(locale), ignore_user_override, substitution);
    return create_numbersubstitution(method, locale, ignore_user_override, substitution);
}

static HRESULT WINAPI dwritefactory_CreateGlyphRunAnalysis(IDWriteFactory2 *iface, DWRITE_GLYPH_RUN const *glyph_run,
    FLOAT pixels_per_dip, DWRITE_MATRIX const* transform, DWRITE_RENDERING_MODE rendering_mode,
    DWRITE_MEASURING_MODE measuring_mode, FLOAT baseline_x, FLOAT baseline_y, IDWriteGlyphRunAnalysis **analysis)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    FIXME("(%p)->(%p %f %p %d %d %f %f %p): stub\n", This, glyph_run, pixels_per_dip, transform, rendering_mode,
        measuring_mode, baseline_x, baseline_y, analysis);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory1_GetEudcFontCollection(IDWriteFactory2 *iface, IDWriteFontCollection **collection,
    BOOL check_for_updates)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p %d)\n", This, collection, check_for_updates);

    if (check_for_updates)
        FIXME("checking for eudc updates not implemented\n");

    if (!This->eudc_collection)
        hr = get_eudc_fontcollection(iface, &This->eudc_collection);

    if (SUCCEEDED(hr))
        IDWriteFontCollection_AddRef(This->eudc_collection);

    *collection = This->eudc_collection;

    return hr;
}

static HRESULT WINAPI dwritefactory1_CreateCustomRenderingParams(IDWriteFactory2 *iface, FLOAT gamma,
    FLOAT enhcontrast, FLOAT enhcontrast_grayscale, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry,
    DWRITE_RENDERING_MODE mode, IDWriteRenderingParams1** params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    IDWriteRenderingParams2 *params2;
    HRESULT hr;

    TRACE("(%p)->(%.2f %.2f %.2f %.2f %d %d %p)\n", This, gamma, enhcontrast, enhcontrast_grayscale,
        cleartype_level, geometry, mode, params);
    hr = IDWriteFactory2_CreateCustomRenderingParams(iface, gamma, enhcontrast, enhcontrast_grayscale,
        cleartype_level, geometry, mode, DWRITE_GRID_FIT_MODE_DEFAULT, &params2);
    *params = (IDWriteRenderingParams1*)params2;
    return hr;
}

static HRESULT WINAPI dwritefactory2_GetSystemFontFallback(IDWriteFactory2 *iface, IDWriteFontFallback **fallback)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    FIXME("(%p)->(%p): stub\n", This, fallback);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory2_CreateFontFallbackBuilder(IDWriteFactory2 *iface, IDWriteFontFallbackBuilder **fallbackbuilder)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    FIXME("(%p)->(%p): stub\n", This, fallbackbuilder);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory2_TranslateColorGlyphRun(IDWriteFactory2 *iface, FLOAT originX, FLOAT originY,
    const DWRITE_GLYPH_RUN *run, const DWRITE_GLYPH_RUN_DESCRIPTION *rundescr, DWRITE_MEASURING_MODE mode,
    const DWRITE_MATRIX *transform, UINT32 palette_index, IDWriteColorGlyphRunEnumerator **colorlayers)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    FIXME("(%p)->(%.2f %.2f %p %p %d %p %u %p): stub\n", This, originX, originY, run, rundescr, mode,
        transform, palette_index, colorlayers);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritefactory2_CreateCustomRenderingParams(IDWriteFactory2 *iface, FLOAT gamma, FLOAT contrast,
    FLOAT grayscalecontrast, FLOAT cleartype_level, DWRITE_PIXEL_GEOMETRY geometry, DWRITE_RENDERING_MODE mode,
    DWRITE_GRID_FIT_MODE gridfit, IDWriteRenderingParams2 **params)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    TRACE("(%p)->(%.2f %.2f %.2f %.2f %d %d %d %p)\n", This, gamma, contrast, grayscalecontrast, cleartype_level,
        geometry, mode, gridfit, params);
    return create_renderingparams(gamma, contrast, grayscalecontrast, cleartype_level, geometry, mode, gridfit, params);
}

static HRESULT WINAPI dwritefactory2_CreateGlyphRunAnalysis(IDWriteFactory2 *iface, const DWRITE_GLYPH_RUN *run,
    const DWRITE_MATRIX *transform, DWRITE_RENDERING_MODE renderingMode, DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GRID_FIT_MODE gridFitMode, DWRITE_TEXT_ANTIALIAS_MODE antialiasMode, FLOAT originX, FLOAT originY,
    IDWriteGlyphRunAnalysis **analysis)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    FIXME("(%p)->(%p %p %d %d %d %d %.2f %.2f %p): stub\n", This, run, transform, renderingMode, measuringMode,
        gridFitMode, antialiasMode, originX, originY, analysis);
    return E_NOTIMPL;
}

static const struct IDWriteFactory2Vtbl dwritefactoryvtbl = {
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
    dwritefactory2_CreateGlyphRunAnalysis
};

static ULONG WINAPI shareddwritefactory_AddRef(IDWriteFactory2 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    TRACE("(%p)\n", This);
    return 2;
}

static ULONG WINAPI shareddwritefactory_Release(IDWriteFactory2 *iface)
{
    struct dwritefactory *This = impl_from_IDWriteFactory2(iface);
    TRACE("(%p)\n", This);
    return 1;
}

static const struct IDWriteFactory2Vtbl shareddwritefactoryvtbl = {
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
    dwritefactory2_CreateGlyphRunAnalysis
};

static void init_dwritefactory(struct dwritefactory *factory, DWRITE_FACTORY_TYPE type)
{
    factory->IDWriteFactory2_iface.lpVtbl = type == DWRITE_FACTORY_TYPE_SHARED ? &shareddwritefactoryvtbl : &dwritefactoryvtbl;
    factory->ref = 1;
    factory->localfontfileloader = NULL;
    factory->system_collection = NULL;
    factory->eudc_collection = NULL;
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

    if (!IsEqualIID(riid, &IID_IDWriteFactory) &&
        !IsEqualIID(riid, &IID_IDWriteFactory1) &&
        !IsEqualIID(riid, &IID_IDWriteFactory2))
        return E_FAIL;

    if (type == DWRITE_FACTORY_TYPE_SHARED && shared_factory) {
        *ret = (IUnknown*)shared_factory;
        IDWriteFactory2_AddRef(shared_factory);
        return S_OK;
    }

    factory = heap_alloc(sizeof(struct dwritefactory));
    if (!factory) return E_OUTOFMEMORY;

    init_dwritefactory(factory, type);

    if (type == DWRITE_FACTORY_TYPE_SHARED)
        if (InterlockedCompareExchangePointer((void**)&shared_factory, &factory->IDWriteFactory2_iface, NULL)) {
            release_shared_factory(&factory->IDWriteFactory2_iface);
            *ret = (IUnknown*)shared_factory;
            IDWriteFactory2_AddRef(shared_factory);
            return S_OK;
        }

    *ret = (IUnknown*)&factory->IDWriteFactory2_iface;
    return S_OK;
}
