/*
 *    Font related tests
 *
 * Copyright 2012, 2014 Nikolay Sivov for CodeWeavers
 * Copyright 2014 Aric Stewart for CodeWeavers
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

#include "windows.h"
#include "dwrite_2.h"
#include "initguid.h"
#include "d2d1.h"

#include "wine/test.h"

#define MS_MAKE_TAG(ch0, ch1, ch2, ch3) \
                    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
                    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

#define MS_CMAP_TAG MS_MAKE_TAG('c','m','a','p')

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static const WCHAR test_fontfile[] = {'w','i','n','e','_','t','e','s','t','_','f','o','n','t','.','t','t','f',0};
static const WCHAR tahomaW[] = {'T','a','h','o','m','a',0};
static const WCHAR tahomaUppercaseW[] = {'T','A','H','O','M','A',0};
static const WCHAR tahomaStrangecaseW[] = {'t','A','h','O','m','A',0};
static const WCHAR blahW[]  = {'B','l','a','h','!',0};

static IDWriteFactory *create_factory(void)
{
    IDWriteFactory *factory;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    return factory;
}

static WCHAR *create_testfontfile(const WCHAR *filename)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(sizeof(pathW)/sizeof(WCHAR), pathW);
    lstrcatW(pathW, filename);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %d\n", wine_dbgstr_w(pathW),
        GetLastError());

    res = FindResourceA(GetModuleHandleA(NULL), (LPCSTR)MAKEINTRESOURCE(1), (LPCSTR)RT_RCDATA);
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );

    return pathW;
}

#define DELETE_FONTFILE(filename) _delete_testfontfile(filename, __LINE__)
static void _delete_testfontfile(const WCHAR *filename, int line)
{
    BOOL ret = DeleteFileW(filename);
    ok_(__FILE__,line)(ret, "failed to delete file %s, error %d\n", wine_dbgstr_w(filename), GetLastError());
}

struct test_fontenumerator
{
    IDWriteFontFileEnumerator IDWriteFontFileEnumerator_iface;
    LONG ref;

    DWORD index;
    IDWriteFontFile *font_file;
};

static inline struct test_fontenumerator *impl_from_IDWriteFontFileEnumerator(IDWriteFontFileEnumerator* iface)
{
    return CONTAINING_RECORD(iface, struct test_fontenumerator, IDWriteFontFileEnumerator_iface);
}

static HRESULT WINAPI singlefontfileenumerator_QueryInterface(IDWriteFontFileEnumerator *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileEnumerator))
    {
        *obj = iface;
        IDWriteFontFileEnumerator_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI singlefontfileenumerator_AddRef(IDWriteFontFileEnumerator *iface)
{
    struct test_fontenumerator *This = impl_from_IDWriteFontFileEnumerator(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI singlefontfileenumerator_Release(IDWriteFontFileEnumerator *iface)
{
    struct test_fontenumerator *This = impl_from_IDWriteFontFileEnumerator(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if (!ref) {
        IDWriteFontFile_Release(This->font_file);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI singlefontfileenumerator_GetCurrentFontFile(IDWriteFontFileEnumerator *iface, IDWriteFontFile **font_file)
{
    struct test_fontenumerator *This = impl_from_IDWriteFontFileEnumerator(iface);
    IDWriteFontFile_AddRef(This->font_file);
    *font_file = This->font_file;
    return S_OK;
}

static HRESULT WINAPI singlefontfileenumerator_MoveNext(IDWriteFontFileEnumerator *iface, BOOL *current)
{
    struct test_fontenumerator *This = impl_from_IDWriteFontFileEnumerator(iface);

    if (This->index > 1) {
        *current = FALSE;
        return S_OK;
    }

    This->index++;
    *current = TRUE;
    return S_OK;
}

static const struct IDWriteFontFileEnumeratorVtbl singlefontfileenumeratorvtbl =
{
    singlefontfileenumerator_QueryInterface,
    singlefontfileenumerator_AddRef,
    singlefontfileenumerator_Release,
    singlefontfileenumerator_MoveNext,
    singlefontfileenumerator_GetCurrentFontFile
};

static HRESULT create_enumerator(IDWriteFontFile *font_file, IDWriteFontFileEnumerator **ret)
{
    struct test_fontenumerator *enumerator;

    enumerator = heap_alloc(sizeof(struct test_fontenumerator));
    if (!enumerator)
        return E_OUTOFMEMORY;

    enumerator->IDWriteFontFileEnumerator_iface.lpVtbl = &singlefontfileenumeratorvtbl;
    enumerator->ref = 1;
    enumerator->index = 0;
    enumerator->font_file = font_file;
    IDWriteFontFile_AddRef(font_file);

    *ret = &enumerator->IDWriteFontFileEnumerator_iface;
    return S_OK;
}

struct test_fontcollectionloader
{
    IDWriteFontCollectionLoader IDWriteFontFileCollectionLoader_iface;
    IDWriteFontFileLoader *loader;
};

static inline struct test_fontcollectionloader *impl_from_IDWriteFontFileCollectionLoader(IDWriteFontCollectionLoader* iface)
{
    return CONTAINING_RECORD(iface, struct test_fontcollectionloader, IDWriteFontFileCollectionLoader_iface);
}

static HRESULT WINAPI resourcecollectionloader_QueryInterface(IDWriteFontCollectionLoader *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontCollectionLoader))
    {
        *obj = iface;
        IDWriteFontCollectionLoader_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI resourcecollectionloader_AddRef(IDWriteFontCollectionLoader *iface)
{
    return 2;
}

static ULONG WINAPI resourcecollectionloader_Release(IDWriteFontCollectionLoader *iface)
{
    return 1;
}

static HRESULT WINAPI resourcecollectionloader_CreateEnumeratorFromKey(IDWriteFontCollectionLoader *iface, IDWriteFactory *factory,
    const void * collectionKey, UINT32  collectionKeySize, IDWriteFontFileEnumerator ** fontFileEnumerator)
{
    struct test_fontcollectionloader *This = impl_from_IDWriteFontFileCollectionLoader(iface);
    IDWriteFontFile *font_file;
    HRESULT hr;

    IDWriteFactory_CreateCustomFontFileReference(factory, collectionKey, collectionKeySize, This->loader, &font_file);

    hr = create_enumerator(font_file, fontFileEnumerator);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteFontFile_Release(font_file);
    return hr;
}

static const struct IDWriteFontCollectionLoaderVtbl resourcecollectionloadervtbl = {
    resourcecollectionloader_QueryInterface,
    resourcecollectionloader_AddRef,
    resourcecollectionloader_Release,
    resourcecollectionloader_CreateEnumeratorFromKey
};

/* Here is a functional custom font set of interfaces */
struct test_fontdatastream
{
    IDWriteFontFileStream IDWriteFontFileStream_iface;
    LONG ref;

    LPVOID data;
    DWORD size;
};

static inline struct test_fontdatastream *impl_from_IDWriteFontFileStream(IDWriteFontFileStream* iface)
{
    return CONTAINING_RECORD(iface, struct test_fontdatastream, IDWriteFontFileStream_iface);
}

static HRESULT WINAPI fontdatastream_QueryInterface(IDWriteFontFileStream *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileStream))
    {
        *obj = iface;
        IDWriteFontFileStream_AddRef(iface);
        return S_OK;
    }
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI fontdatastream_AddRef(IDWriteFontFileStream *iface)
{
    struct test_fontdatastream *This = impl_from_IDWriteFontFileStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI fontdatastream_Release(IDWriteFontFileStream *iface)
{
    struct test_fontdatastream *This = impl_from_IDWriteFontFileStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI fontdatastream_ReadFileFragment(IDWriteFontFileStream *iface, void const **fragment_start, UINT64 offset, UINT64 fragment_size, void **fragment_context)
{
    struct test_fontdatastream *This = impl_from_IDWriteFontFileStream(iface);
    *fragment_context = NULL;
    if (offset+fragment_size > This->size)
    {
        *fragment_start = NULL;
        return E_FAIL;
    }
    else
    {
        *fragment_start = (BYTE*)This->data + offset;
        return S_OK;
    }
}

static void WINAPI fontdatastream_ReleaseFileFragment(IDWriteFontFileStream *iface, void *fragment_context)
{
    /* Do Nothing */
}

static HRESULT WINAPI fontdatastream_GetFileSize(IDWriteFontFileStream *iface, UINT64 *size)
{
    struct test_fontdatastream *This = impl_from_IDWriteFontFileStream(iface);
    *size = This->size;
    return S_OK;
}

static HRESULT WINAPI fontdatastream_GetLastWriteTime(IDWriteFontFileStream *iface, UINT64 *last_writetime)
{
    return E_NOTIMPL;
}

static const IDWriteFontFileStreamVtbl fontdatastreamvtbl =
{
    fontdatastream_QueryInterface,
    fontdatastream_AddRef,
    fontdatastream_Release,
    fontdatastream_ReadFileFragment,
    fontdatastream_ReleaseFileFragment,
    fontdatastream_GetFileSize,
    fontdatastream_GetLastWriteTime
};

static HRESULT create_fontdatastream(LPVOID data, UINT size, IDWriteFontFileStream** iface)
{
    struct test_fontdatastream *This = HeapAlloc(GetProcessHeap(), 0, sizeof(struct test_fontdatastream));
    if (!This)
        return E_OUTOFMEMORY;

    This->data = data;
    This->size = size;
    This->ref = 1;
    This->IDWriteFontFileStream_iface.lpVtbl = &fontdatastreamvtbl;

    *iface = &This->IDWriteFontFileStream_iface;
    return S_OK;
}

static HRESULT WINAPI resourcefontfileloader_QueryInterface(IDWriteFontFileLoader *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader))
    {
        *obj = iface;
        return S_OK;
    }
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI resourcefontfileloader_AddRef(IDWriteFontFileLoader *iface)
{
    return 2;
}

static ULONG WINAPI resourcefontfileloader_Release(IDWriteFontFileLoader *iface)
{
    return 1;
}

static HRESULT WINAPI resourcefontfileloader_CreateStreamFromKey(IDWriteFontFileLoader *iface, const void *fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream **fontFileStream)
{
    LPVOID data;
    DWORD size;
    HGLOBAL mem;

    mem = LoadResource(GetModuleHandleA(NULL), *(HRSRC*)fontFileReferenceKey);
    ok(mem != NULL, "Failed to lock font resource\n");
    if (mem)
    {
        size = SizeofResource(GetModuleHandleA(NULL), *(HRSRC*)fontFileReferenceKey);
        data = LockResource(mem);
        return create_fontdatastream(data, size, fontFileStream);
    }
    return E_FAIL;
}

static const struct IDWriteFontFileLoaderVtbl resourcefontfileloadervtbl = {
    resourcefontfileloader_QueryInterface,
    resourcefontfileloader_AddRef,
    resourcefontfileloader_Release,
    resourcefontfileloader_CreateStreamFromKey
};

static IDWriteFontFileLoader rloader = { &resourcefontfileloadervtbl };

static D2D1_POINT_2F g_startpoints[2];
static int g_startpoint_count;

static HRESULT WINAPI test_geometrysink_QueryInterface(ID2D1SimplifiedGeometrySink *iface, REFIID riid, void **ret)
{
    if (IsEqualIID(riid, &IID_ID2D1SimplifiedGeometrySink) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ret = iface;
        ID2D1SimplifiedGeometrySink_AddRef(iface);
        return S_OK;
    }

    *ret = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_geometrysink_AddRef(ID2D1SimplifiedGeometrySink *iface)
{
    return 2;
}

static ULONG WINAPI test_geometrysink_Release(ID2D1SimplifiedGeometrySink *iface)
{
    return 1;
}

static void WINAPI test_geometrysink_SetFillMode(ID2D1SimplifiedGeometrySink *iface, D2D1_FILL_MODE mode)
{
    ok(mode == D2D1_FILL_MODE_WINDING, "fill mode %d\n", mode);
}

static void WINAPI test_geometrysink_SetSegmentFlags(ID2D1SimplifiedGeometrySink *iface, D2D1_PATH_SEGMENT flags)
{
    ok(0, "unexpected SetSegmentFlags() - flags %d\n", flags);
}

static void WINAPI test_geometrysink_BeginFigure(ID2D1SimplifiedGeometrySink *iface,
    D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin)
{
    ok(figureBegin == D2D1_FIGURE_BEGIN_FILLED, "begin figure %d\n", figureBegin);
    g_startpoints[g_startpoint_count++] = startPoint;
}

static void WINAPI test_geometrysink_AddLines(ID2D1SimplifiedGeometrySink *iface,
    const D2D1_POINT_2F *points, UINT32 count)
{
}

static void WINAPI test_geometrysink_AddBeziers(ID2D1SimplifiedGeometrySink *iface,
    const D2D1_BEZIER_SEGMENT *beziers, UINT32 count)
{
}

static void WINAPI test_geometrysink_EndFigure(ID2D1SimplifiedGeometrySink *iface, D2D1_FIGURE_END figureEnd)
{
    ok(figureEnd == D2D1_FIGURE_END_CLOSED, "end figure %d\n", figureEnd);
}

static HRESULT WINAPI test_geometrysink_Close(ID2D1SimplifiedGeometrySink *iface)
{
    ok(0, "unexpected Close()\n");
    return E_NOTIMPL;
}

static const ID2D1SimplifiedGeometrySinkVtbl test_geometrysink_vtbl = {
    test_geometrysink_QueryInterface,
    test_geometrysink_AddRef,
    test_geometrysink_Release,
    test_geometrysink_SetFillMode,
    test_geometrysink_SetSegmentFlags,
    test_geometrysink_BeginFigure,
    test_geometrysink_AddLines,
    test_geometrysink_AddBeziers,
    test_geometrysink_EndFigure,
    test_geometrysink_Close
};

static ID2D1SimplifiedGeometrySink test_geomsink = { &test_geometrysink_vtbl };

static void test_CreateFontFromLOGFONT(void)
{
    static const WCHAR tahomaspW[] = {'T','a','h','o','m','a',' ',0};
    IDWriteGdiInterop *interop;
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;
    IDWriteFont *font;
    LOGFONTW logfont;
    LONG weights[][2] = {
        {FW_NORMAL, DWRITE_FONT_WEIGHT_NORMAL},
        {FW_BOLD, DWRITE_FONT_WEIGHT_BOLD},
        {  0, DWRITE_FONT_WEIGHT_NORMAL},
        { 50, DWRITE_FONT_WEIGHT_NORMAL},
        {150, DWRITE_FONT_WEIGHT_NORMAL},
        {250, DWRITE_FONT_WEIGHT_NORMAL},
        {350, DWRITE_FONT_WEIGHT_NORMAL},
        {450, DWRITE_FONT_WEIGHT_NORMAL},
        {650, DWRITE_FONT_WEIGHT_BOLD},
        {750, DWRITE_FONT_WEIGHT_BOLD},
        {850, DWRITE_FONT_WEIGHT_BOLD},
        {950, DWRITE_FONT_WEIGHT_BOLD},
        {960, DWRITE_FONT_WEIGHT_BOLD},
    };
    OUTLINETEXTMETRICW otm;
    IDWriteFactory *factory;
    HRESULT hr;
    BOOL ret;
    HDC hdc;
    HFONT hfont;
    BOOL exists;
    int i;
    UINT r;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

if (0)
    /* null out parameter crashes this call */
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, NULL, NULL);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, NULL, &font);
    EXPECT_HR(hr, E_INVALIDARG);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

    hfont = CreateFontIndirectW(&logfont);
    hdc = CreateCompatibleDC(0);
    SelectObject(hdc, hfont);

    otm.otmSize = sizeof(otm);
    r = GetOutlineTextMetricsW(hdc, otm.otmSize, &otm);
    ok(r, "got %d\n", r);
    DeleteDC(hdc);
    DeleteObject(hfont);

    exists = TRUE;
    hr = IDWriteFont_HasCharacter(font, 0xd800, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == FALSE, "got %d\n", exists);

    exists = FALSE;
    hr = IDWriteFont_HasCharacter(font, 0x20, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == TRUE, "got %d\n", exists);

    /* now check properties */
    weight = IDWriteFont_GetWeight(font);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL, "got %d\n", weight);

    style = IDWriteFont_GetStyle(font);
    ok(style == DWRITE_FONT_STYLE_OBLIQUE, "got %d\n", style);
todo_wine
    ok(otm.otmfsSelection == 1, "got 0x%08x\n", otm.otmfsSelection);

    ret = IDWriteFont_IsSymbolFont(font);
    ok(!ret, "got %d\n", ret);

    IDWriteFont_Release(font);

    /* weight values */
    for (i = 0; i < sizeof(weights)/(2*sizeof(LONG)); i++)
    {
        memset(&logfont, 0, sizeof(logfont));
        logfont.lfHeight = 12;
        logfont.lfWidth  = 12;
        logfont.lfWeight = weights[i][0];
        lstrcpyW(logfont.lfFaceName, tahomaW);

        hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
        EXPECT_HR(hr, S_OK);

        weight = IDWriteFont_GetWeight(font);
        ok(weight == weights[i][1],
            "%d: got %d, expected %d\n", i, weight, weights[i][1]);

        IDWriteFont_Release(font);
    }

    /* weight not from enum */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = 550;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    font = NULL;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    weight = IDWriteFont_GetWeight(font);
    ok(weight == DWRITE_FONT_WEIGHT_NORMAL || broken(weight == DWRITE_FONT_WEIGHT_BOLD) /* win7 w/o SP */,
        "got %d\n", weight);
    IDWriteFont_Release(font);

    /* empty or nonexistent face name */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    lstrcpyW(logfont.lfFaceName, blahW);

    font = (void*)0xdeadbeef;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == DWRITE_E_NOFONT, "got 0x%08x\n", hr);
    ok(font == NULL, "got %p\n", font);

    /* Try with name 'Tahoma ' */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    lstrcpyW(logfont.lfFaceName, tahomaspW);

    font = (void*)0xdeadbeef;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == DWRITE_E_NOFONT, "got 0x%08x\n", hr);
    ok(font == NULL, "got %p\n", font);

    /* empty string as a facename */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;

    font = (void*)0xdeadbeef;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == DWRITE_E_NOFONT, "got 0x%08x\n", hr);
    ok(font == NULL, "got %p\n", font);

    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_CreateBitmapRenderTarget(void)
{
    IDWriteBitmapRenderTarget *target, *target2;
    IDWriteBitmapRenderTarget1 *target1;
    IDWriteGdiInterop *interop;
    IDWriteFactory *factory;
    HBITMAP hbm, hbm2;
    DWRITE_MATRIX m;
    DIBSECTION ds;
    COLORREF c;
    HRESULT hr;
    FLOAT pdip;
    SIZE size;
    HDC hdc;
    int ret;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    target = NULL;
    hr = IDWriteGdiInterop_CreateBitmapRenderTarget(interop, NULL, 0, 0, &target);
    EXPECT_HR(hr, S_OK);

if (0) /* crashes on native */
    hr = IDWriteBitmapRenderTarget_GetSize(target, NULL);

    size.cx = size.cy = -1;
    hr = IDWriteBitmapRenderTarget_GetSize(target, &size);
    EXPECT_HR(hr, S_OK);
    ok(size.cx == 0, "got %d\n", size.cx);
    ok(size.cy == 0, "got %d\n", size.cy);

    target2 = NULL;
    hr = IDWriteGdiInterop_CreateBitmapRenderTarget(interop, NULL, 0, 0, &target2);
    EXPECT_HR(hr, S_OK);
    ok(target != target2, "got %p, %p\n", target2, target);
    IDWriteBitmapRenderTarget_Release(target2);

    hdc = IDWriteBitmapRenderTarget_GetMemoryDC(target);
    ok(hdc != NULL, "got %p\n", hdc);

    hbm = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm != NULL, "got %p\n", hbm);

    /* check DIB properties */
    ret = GetObjectW(hbm, sizeof(ds), &ds);
    ok(ret == sizeof(BITMAP), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 1, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 1, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 1, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(!ds.dsBm.bmBits, "got %p\n", ds.dsBm.bmBits);

    IDWriteBitmapRenderTarget_Release(target);

    hbm = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(!hbm, "got %p\n", hbm);

    target = NULL;
    hr = IDWriteGdiInterop_CreateBitmapRenderTarget(interop, NULL, 10, 5, &target);
    EXPECT_HR(hr, S_OK);

    hdc = IDWriteBitmapRenderTarget_GetMemoryDC(target);
    ok(hdc != NULL, "got %p\n", hdc);

    /* test context settings */
    c = GetTextColor(hdc);
    ok(c == RGB(0, 0, 0), "got 0x%08x\n", c);
    ret = GetBkMode(hdc);
    ok(ret == OPAQUE, "got %d\n", ret);
    c = GetBkColor(hdc);
    ok(c == RGB(255, 255, 255), "got 0x%08x\n", c);

    hbm = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm != NULL, "got %p\n", hbm);

    /* check DIB properties */
    ret = GetObjectW(hbm, sizeof(ds), &ds);
    ok(ret == sizeof(ds), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 10, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 5, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 32, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(ds.dsBm.bmBits != NULL, "got %p\n", ds.dsBm.bmBits);

    size.cx = size.cy = -1;
    hr = IDWriteBitmapRenderTarget_GetSize(target, &size);
    EXPECT_HR(hr, S_OK);
    ok(size.cx == 10, "got %d\n", size.cx);
    ok(size.cy == 5, "got %d\n", size.cy);

    /* resize to same size */
    hr = IDWriteBitmapRenderTarget_Resize(target, 10, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 == hbm, "got %p, %p\n", hbm2, hbm);

    /* shrink */
    hr = IDWriteBitmapRenderTarget_Resize(target, 5, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    hr = IDWriteBitmapRenderTarget_Resize(target, 20, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    hr = IDWriteBitmapRenderTarget_Resize(target, 1, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    ret = GetObjectW(hbm2, sizeof(ds), &ds);
    ok(ret == sizeof(ds), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 1, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 5, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 32, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(ds.dsBm.bmBits != NULL, "got %p\n", ds.dsBm.bmBits);

    /* empty rectangle */
    hr = IDWriteBitmapRenderTarget_Resize(target, 0, 5);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hbm2 = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hbm2 != hbm, "got %p, %p\n", hbm2, hbm);

    ret = GetObjectW(hbm2, sizeof(ds), &ds);
    ok(ret == sizeof(BITMAP), "got %d\n", ret);
    ok(ds.dsBm.bmWidth == 1, "got %d\n", ds.dsBm.bmWidth);
    ok(ds.dsBm.bmHeight == 1, "got %d\n", ds.dsBm.bmHeight);
    ok(ds.dsBm.bmPlanes == 1, "got %d\n", ds.dsBm.bmPlanes);
    ok(ds.dsBm.bmBitsPixel == 1, "got %d\n", ds.dsBm.bmBitsPixel);
    ok(!ds.dsBm.bmBits, "got %p\n", ds.dsBm.bmBits);

    /* transform tests */
if (0) /* crashes on native */
    hr = IDWriteBitmapRenderTarget_GetCurrentTransform(target, NULL);

    memset(&m, 0xcc, sizeof(m));
    hr = IDWriteBitmapRenderTarget_GetCurrentTransform(target, &m);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(m.m11 == 1.0 && m.m22 == 1.0 && m.m12 == 0.0 && m.m21 == 0.0, "got %.1f,%.1f,%.1f,%.1f\n", m.m11, m.m22, m.m12, m.m21);
    ok(m.dx == 0.0 && m.dy == 0.0, "got %.1f,%.1f\n", m.dx, m.dy);

    memset(&m, 0, sizeof(m));
    hr = IDWriteBitmapRenderTarget_SetCurrentTransform(target, &m);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&m, 0xcc, sizeof(m));
    hr = IDWriteBitmapRenderTarget_GetCurrentTransform(target, &m);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(m.m11 == 0.0 && m.m22 == 0.0 && m.m12 == 0.0 && m.m21 == 0.0, "got %.1f,%.1f,%.1f,%.1f\n", m.m11, m.m22, m.m12, m.m21);
    ok(m.dx == 0.0 && m.dy == 0.0, "got %.1f,%.1f\n", m.dx, m.dy);

    hr = IDWriteBitmapRenderTarget_SetCurrentTransform(target, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&m, 0xcc, sizeof(m));
    hr = IDWriteBitmapRenderTarget_GetCurrentTransform(target, &m);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(m.m11 == 1.0 && m.m22 == 1.0 && m.m12 == 0.0 && m.m21 == 0.0, "got %.1f,%.1f,%.1f,%.1f\n", m.m11, m.m22, m.m12, m.m21);
    ok(m.dx == 0.0 && m.dy == 0.0, "got %.1f,%.1f\n", m.dx, m.dy);

    /* pixels per dip */
    pdip = IDWriteBitmapRenderTarget_GetPixelsPerDip(target);
    ok(pdip == 1.0, "got %.2f\n", pdip);

    hr = IDWriteBitmapRenderTarget_SetPixelsPerDip(target, 2.0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteBitmapRenderTarget_SetPixelsPerDip(target, -1.0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteBitmapRenderTarget_SetPixelsPerDip(target, 0.0);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    pdip = IDWriteBitmapRenderTarget_GetPixelsPerDip(target);
    ok(pdip == 2.0, "got %.2f\n", pdip);

    hr = IDWriteBitmapRenderTarget_QueryInterface(target, &IID_IDWriteBitmapRenderTarget1, (void**)&target1);
    if (hr == S_OK) {
        DWRITE_TEXT_ANTIALIAS_MODE mode;

        mode = IDWriteBitmapRenderTarget1_GetTextAntialiasMode(target1);
        ok(mode == DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE, "got %d\n", mode);

        hr = IDWriteBitmapRenderTarget1_SetTextAntialiasMode(target1, DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE+1);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

        mode = IDWriteBitmapRenderTarget1_GetTextAntialiasMode(target1);
        ok(mode == DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE, "got %d\n", mode);

        hr = IDWriteBitmapRenderTarget1_SetTextAntialiasMode(target1, DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        mode = IDWriteBitmapRenderTarget1_GetTextAntialiasMode(target1);
        ok(mode == DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE, "got %d\n", mode);

        IDWriteBitmapRenderTarget1_Release(target1);
    }
    else
        win_skip("IDWriteBitmapRenderTarget1 is not supported.\n");

    IDWriteBitmapRenderTarget_Release(target);
    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_GetFontFamily(void)
{
    IDWriteFontCollection *collection, *collection2;
    IDWriteFontCollection *syscoll;
    IDWriteFontFamily *family, *family2;
    IDWriteGdiInterop *interop;
    IDWriteFont *font, *font2;
    IDWriteFactory *factory;
    LOGFONTW logfont;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    hr = IDWriteFactory_GetSystemFontCollection(factory, &syscoll, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(font2 != font, "got %p, %p\n", font2, font);

if (0) /* crashes on native */
    hr = IDWriteFont_GetFontFamily(font, NULL);

    EXPECT_REF(font, 1);
    hr = IDWriteFont_GetFontFamily(font, &family);
    EXPECT_HR(hr, S_OK);
    EXPECT_REF(font, 1);
    EXPECT_REF(family, 2);

    hr = IDWriteFont_GetFontFamily(font, &family2);
    EXPECT_HR(hr, S_OK);
    ok(family2 == family, "got %p, previous %p\n", family2, family);
    EXPECT_REF(font, 1);
    EXPECT_REF(family, 3);
    IDWriteFontFamily_Release(family2);

    hr = IDWriteFont_QueryInterface(font, &IID_IDWriteFontFamily, (void**)&family2);
    EXPECT_HR(hr, E_NOINTERFACE);
    ok(family2 == NULL, "got %p\n", family2);

    hr = IDWriteFont_GetFontFamily(font2, &family2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(family2 != family, "got %p, %p\n", family2, family);

    collection = NULL;
    hr = IDWriteFontFamily_GetFontCollection(family, &collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    collection2 = NULL;
    hr = IDWriteFontFamily_GetFontCollection(family2, &collection2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(collection == collection2, "got %p, %p\n", collection, collection2);
    ok(collection == syscoll, "got %p, %p\n", collection, syscoll);

    IDWriteFontCollection_Release(syscoll);
    IDWriteFontCollection_Release(collection2);
    IDWriteFontCollection_Release(collection);
    IDWriteFontFamily_Release(family2);
    IDWriteFontFamily_Release(family);
    IDWriteFont_Release(font);
    IDWriteFont_Release(font2);
    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_GetFamilyNames(void)
{
    IDWriteFontFamily *family;
    IDWriteLocalizedStrings *names, *names2;
    IDWriteGdiInterop *interop;
    IDWriteFactory *factory;
    IDWriteFont *font;
    LOGFONTW logfont;
    WCHAR buffer[100];
    HRESULT hr;
    UINT32 len;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    EXPECT_HR(hr, S_OK);

    hr = IDWriteFont_GetFontFamily(font, &family);
    EXPECT_HR(hr, S_OK);

if (0) /* crashes on native */
    hr = IDWriteFontFamily_GetFamilyNames(family, NULL);

    hr = IDWriteFontFamily_GetFamilyNames(family, &names);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(names, 1);

    hr = IDWriteFontFamily_GetFamilyNames(family, &names2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(names2, 1);
    ok(names != names2, "got %p, was %p\n", names2, names);

    IDWriteLocalizedStrings_Release(names2);

    /* GetStringLength */
if (0) /* crashes on native */
    hr = IDWriteLocalizedStrings_GetStringLength(names, 0, NULL);

    len = 100;
    hr = IDWriteLocalizedStrings_GetStringLength(names, 10, &len);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(len == (UINT32)-1, "got %u\n", len);

    len = 0;
    hr = IDWriteLocalizedStrings_GetStringLength(names, 0, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(len > 0, "got %u\n", len);

    /* GetString */
    hr = IDWriteLocalizedStrings_GetString(names, 0, NULL, 0);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);

    hr = IDWriteLocalizedStrings_GetString(names, 10, NULL, 0);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

if (0)
    hr = IDWriteLocalizedStrings_GetString(names, 0, NULL, 100);

    buffer[0] = 1;
    hr = IDWriteLocalizedStrings_GetString(names, 10, buffer, 100);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(buffer[0] == 0, "got %x\n", buffer[0]);

    buffer[0] = 1;
    hr = IDWriteLocalizedStrings_GetString(names, 0, buffer, len-1);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(buffer[0] == 0, "got %x\n", buffer[0]);

    buffer[0] = 1;
    hr = IDWriteLocalizedStrings_GetString(names, 0, buffer, len);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(buffer[0] == 0, "got %x\n", buffer[0]);

    buffer[0] = 0;
    hr = IDWriteLocalizedStrings_GetString(names, 0, buffer, len+1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(buffer[0] != 0, "got %x\n", buffer[0]);

    IDWriteLocalizedStrings_Release(names);

    IDWriteFontFamily_Release(family);
    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_CreateFontFace(void)
{
    IDWriteFontFace *fontface, *fontface2;
    IDWriteFontCollection *collection;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteGdiInterop *interop;
    IDWriteFont *font, *font2;
    IDWriteFontFamily *family;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    LOGFONTW logfont;
    BOOL supported;
    UINT32 count;
    WCHAR *path;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    font = NULL;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    font2 = NULL;
    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(font != font2, "got %p, %p\n", font, font2);

    hr = IDWriteFont_QueryInterface(font, &IID_IDWriteFontFace, (void**)&fontface);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

if (0) /* crashes on native */
    hr = IDWriteFont_CreateFontFace(font, NULL);

    fontface = NULL;
    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    fontface2 = NULL;
    hr = IDWriteFont_CreateFontFace(font, &fontface2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fontface == fontface2, "got %p, was %p\n", fontface2, fontface);
    IDWriteFontFace_Release(fontface2);

    fontface2 = NULL;
    hr = IDWriteFont_CreateFontFace(font2, &fontface2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fontface == fontface2, "got %p, was %p\n", fontface2, fontface);
    IDWriteFontFace_Release(fontface2);

    IDWriteFont_Release(font2);
    IDWriteFont_Release(font);

    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFont, (void**)&font);
    ok(hr == E_NOINTERFACE || broken(hr == E_NOTIMPL), "got 0x%08x\n", hr);

    IDWriteFontFace_Release(fontface);
    IDWriteGdiInterop_Release(interop);

    /* Create from system collection */
    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontCollection_GetFontFamily(collection, 0, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    font = NULL;
    hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    font2 = NULL;
    hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(font != font2, "got %p, %p\n", font, font2);

    fontface = NULL;
    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    fontface2 = NULL;
    hr = IDWriteFont_CreateFontFace(font2, &fontface2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fontface == fontface2, "got %p, was %p\n", fontface2, fontface);

    IDWriteFontFace_Release(fontface);
    IDWriteFontFace_Release(fontface2);
    IDWriteFont_Release(font2);
    IDWriteFont_Release(font);
    IDWriteFontFamily_Release(family);
    IDWriteFontCollection_Release(collection);

    /* IDWriteFactory::CreateFontFace() */
    path = create_testfontfile(test_fontfile);
    factory = create_factory();

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    supported = FALSE;
    file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    face_type = DWRITE_FONT_FACE_TYPE_CFF;
    count = 0;
    hr = IDWriteFontFile_Analyze(file, &supported, &file_type, &face_type, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(supported == TRUE, "got %i\n", supported);
    ok(file_type == DWRITE_FONT_FILE_TYPE_TRUETYPE, "got %i\n", file_type);
    ok(face_type == DWRITE_FONT_FACE_TYPE_TRUETYPE, "got %i\n", face_type);
    ok(count == 1, "got %i\n", count);

    /* try mismatching face type, the one that's not supported */
    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_CFF, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
todo_wine
    ok(hr == DWRITE_E_FILEFORMAT, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION, 1, &file, 0,
        DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_RAW_CFF, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
todo_wine
    ok(hr == DWRITE_E_UNSUPPORTEDOPERATION || broken(hr == E_INVALIDARG) /* older versions */, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TYPE1, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_VECTOR, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_BITMAP, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_UNKNOWN, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    IDWriteFontFile_Release(file);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_GetMetrics(void)
{
    IDWriteGdiInterop *interop;
    DWRITE_FONT_METRICS metrics;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    OUTLINETEXTMETRICW otm;
    IDWriteFont1 *font1;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;
    HDC hdc;
    HFONT hfont;
    int ret;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    EXPECT_HR(hr, S_OK);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hfont = CreateFontIndirectW(&logfont);
    hdc = CreateCompatibleDC(0);
    SelectObject(hdc, hfont);

    otm.otmSize = sizeof(otm);
    ret = GetOutlineTextMetricsW(hdc, otm.otmSize, &otm);
    ok(ret, "got %d\n", ret);
    DeleteDC(hdc);
    DeleteObject(hfont);

if (0) /* crashes on native */
    IDWriteFont_GetMetrics(font, NULL);

    memset(&metrics, 0, sizeof(metrics));
    IDWriteFont_GetMetrics(font, &metrics);

    ok(metrics.designUnitsPerEm != 0, "designUnitsPerEm %u\n", metrics.designUnitsPerEm);
    ok(metrics.ascent != 0, "ascent %u\n", metrics.ascent);
    ok(metrics.descent != 0, "descent %u\n", metrics.descent);
    ok(metrics.lineGap == 0, "lineGap %d\n", metrics.lineGap);
    ok(metrics.capHeight, "capHeight %u\n", metrics.capHeight);
    ok(metrics.xHeight != 0, "xHeight %u\n", metrics.xHeight);
    ok(metrics.underlinePosition < 0, "underlinePosition %d\n", metrics.underlinePosition);
    ok(metrics.underlineThickness != 0, "underlineThickness %u\n", metrics.underlineThickness);
    ok(metrics.strikethroughPosition > 0, "strikethroughPosition %d\n", metrics.strikethroughPosition);
    ok(metrics.strikethroughThickness != 0, "strikethroughThickness %u\n", metrics.strikethroughThickness);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&metrics, 0, sizeof(metrics));
    IDWriteFontFace_GetMetrics(fontface, &metrics);

    ok(metrics.designUnitsPerEm != 0, "designUnitsPerEm %u\n", metrics.designUnitsPerEm);
    ok(metrics.ascent != 0, "ascent %u\n", metrics.ascent);
    ok(metrics.descent != 0, "descent %u\n", metrics.descent);
    ok(metrics.lineGap == 0, "lineGap %d\n", metrics.lineGap);
    ok(metrics.capHeight, "capHeight %u\n", metrics.capHeight);
    ok(metrics.xHeight != 0, "xHeight %u\n", metrics.xHeight);
    ok(metrics.underlinePosition < 0, "underlinePosition %d\n", metrics.underlinePosition);
    ok(metrics.underlineThickness != 0, "underlineThickness %u\n", metrics.underlineThickness);
    ok(metrics.strikethroughPosition > 0, "strikethroughPosition %d\n", metrics.strikethroughPosition);
    ok(metrics.strikethroughThickness != 0, "strikethroughThickness %u\n", metrics.strikethroughThickness);

    hr = IDWriteFont_QueryInterface(font, &IID_IDWriteFont1, (void**)&font1);
    if (hr == S_OK) {
        DWRITE_FONT_METRICS1 metrics1;
        IDWriteFontFace1 *fontface1;

        memset(&metrics1, 0, sizeof(metrics1));
        IDWriteFont1_GetMetrics(font1, &metrics1);

        ok(metrics1.designUnitsPerEm != 0, "designUnitsPerEm %u\n", metrics1.designUnitsPerEm);
        ok(metrics1.ascent != 0, "ascent %u\n", metrics1.ascent);
        ok(metrics1.descent != 0, "descent %u\n", metrics1.descent);
        ok(metrics1.lineGap == 0, "lineGap %d\n", metrics1.lineGap);
        ok(metrics1.capHeight, "capHeight %u\n", metrics1.capHeight);
        ok(metrics1.xHeight != 0, "xHeight %u\n", metrics1.xHeight);
        ok(metrics1.underlinePosition < 0, "underlinePosition %d\n", metrics1.underlinePosition);
        ok(metrics1.underlineThickness != 0, "underlineThickness %u\n", metrics1.underlineThickness);
        ok(metrics1.strikethroughPosition > 0, "strikethroughPosition %d\n", metrics1.strikethroughPosition);
        ok(metrics1.strikethroughThickness != 0, "strikethroughThickness %u\n", metrics1.strikethroughThickness);
        ok(metrics1.glyphBoxLeft < 0, "glyphBoxLeft %d\n", metrics1.glyphBoxLeft);
        ok(metrics1.glyphBoxTop > 0, "glyphBoxTop %d\n", metrics1.glyphBoxTop);
        ok(metrics1.glyphBoxRight > 0, "glyphBoxRight %d\n", metrics1.glyphBoxRight);
        ok(metrics1.glyphBoxBottom < 0, "glyphBoxBottom %d\n", metrics1.glyphBoxBottom);
        ok(metrics1.subscriptPositionY < 0, "subscriptPositionY %d\n", metrics1.subscriptPositionY);
        ok(metrics1.subscriptSizeX > 0, "subscriptSizeX %d\n", metrics1.subscriptSizeX);
        ok(metrics1.subscriptSizeY > 0, "subscriptSizeY %d\n", metrics1.subscriptSizeY);
        ok(metrics1.superscriptPositionY > 0, "superscriptPositionY %d\n", metrics1.superscriptPositionY);
        ok(metrics1.superscriptSizeX > 0, "superscriptSizeX %d\n", metrics1.superscriptSizeX);
        ok(metrics1.superscriptSizeY > 0, "superscriptSizeY %d\n", metrics1.superscriptSizeY);
        ok(!metrics1.hasTypographicMetrics, "hasTypographicMetrics %d\n", metrics1.hasTypographicMetrics);

        hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        memset(&metrics1, 0, sizeof(metrics1));
        IDWriteFontFace1_GetMetrics(fontface1, &metrics1);

        ok(metrics1.designUnitsPerEm != 0, "designUnitsPerEm %u\n", metrics1.designUnitsPerEm);
        ok(metrics1.ascent != 0, "ascent %u\n", metrics1.ascent);
        ok(metrics1.descent != 0, "descent %u\n", metrics1.descent);
        ok(metrics1.lineGap == 0, "lineGap %d\n", metrics1.lineGap);
        ok(metrics1.capHeight, "capHeight %u\n", metrics1.capHeight);
        ok(metrics1.xHeight != 0, "xHeight %u\n", metrics1.xHeight);
        ok(metrics1.underlinePosition < 0, "underlinePosition %d\n", metrics1.underlinePosition);
        ok(metrics1.underlineThickness != 0, "underlineThickness %u\n", metrics1.underlineThickness);
        ok(metrics1.strikethroughPosition > 0, "strikethroughPosition %d\n", metrics1.strikethroughPosition);
        ok(metrics1.strikethroughThickness != 0, "strikethroughThickness %u\n", metrics1.strikethroughThickness);
        ok(metrics1.glyphBoxLeft < 0, "glyphBoxLeft %d\n", metrics1.glyphBoxLeft);
        ok(metrics1.glyphBoxTop > 0, "glyphBoxTop %d\n", metrics1.glyphBoxTop);
        ok(metrics1.glyphBoxRight > 0, "glyphBoxRight %d\n", metrics1.glyphBoxRight);
        ok(metrics1.glyphBoxBottom < 0, "glyphBoxBottom %d\n", metrics1.glyphBoxBottom);
        ok(metrics1.subscriptPositionY < 0, "subscriptPositionY %d\n", metrics1.subscriptPositionY);
        ok(metrics1.subscriptSizeX > 0, "subscriptSizeX %d\n", metrics1.subscriptSizeX);
        ok(metrics1.subscriptSizeY > 0, "subscriptSizeY %d\n", metrics1.subscriptSizeY);
        ok(metrics1.superscriptPositionY > 0, "superscriptPositionY %d\n", metrics1.superscriptPositionY);
        ok(metrics1.superscriptSizeX > 0, "superscriptSizeX %d\n", metrics1.superscriptSizeX);
        ok(metrics1.superscriptSizeY > 0, "superscriptSizeY %d\n", metrics1.superscriptSizeY);
        ok(!metrics1.hasTypographicMetrics, "hasTypographicMetrics %d\n", metrics1.hasTypographicMetrics);

        IDWriteFontFace1_Release(fontface1);
        IDWriteFont1_Release(font1);
    }
    else
        win_skip("DWRITE_FONT_METRICS1 is not supported.\n");

    IDWriteFontFace_Release(fontface);

    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_system_fontcollection(void)
{
    IDWriteFontCollection *collection, *coll2;
    IDWriteLocalFontFileLoader *localloader;
    IDWriteFactory *factory, *factory2;
    IDWriteFontFileLoader *loader;
    IDWriteFontFamily *family;
    IDWriteFontFace *fontface;
    IDWriteFontFile *file;
    IDWriteFont *font;
    HRESULT hr;
    UINT32 i;
    BOOL ret;

    factory = create_factory();

    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_GetSystemFontCollection(factory, &coll2, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(coll2 == collection, "got %p, was %p\n", coll2, collection);
    IDWriteFontCollection_Release(coll2);

    hr = IDWriteFactory_GetSystemFontCollection(factory, &coll2, TRUE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(coll2 == collection, "got %p, was %p\n", coll2, collection);
    IDWriteFontCollection_Release(coll2);

    factory2 = create_factory();
    hr = IDWriteFactory_GetSystemFontCollection(factory2, &coll2, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(coll2 != collection, "got %p, was %p\n", coll2, collection);
    IDWriteFontCollection_Release(coll2);
    IDWriteFactory_Release(factory2);

    i = IDWriteFontCollection_GetFontFamilyCount(collection);
    ok(i, "got %u\n", i);

    /* invalid index */
    family = (void*)0xdeadbeef;
    hr = IDWriteFontCollection_GetFontFamily(collection, i, &family);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(family == NULL, "got %p\n", family);

    ret = FALSE;
    i = (UINT32)-1;
    hr = IDWriteFontCollection_FindFamilyName(collection, tahomaW, &i, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret, "got %d\n", ret);
    ok(i != (UINT32)-1, "got %u\n", i);

    ret = FALSE;
    i = (UINT32)-1;
    hr = IDWriteFontCollection_FindFamilyName(collection, tahomaUppercaseW, &i, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret, "got %d\n", ret);
    ok(i != (UINT32)-1, "got %u\n", i);

    ret = FALSE;
    i = (UINT32)-1;
    hr = IDWriteFontCollection_FindFamilyName(collection, tahomaStrangecaseW, &i, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret, "got %d\n", ret);
    ok(i != (UINT32)-1, "got %u\n", i);

    /* get back local file loader */
    hr = IDWriteFontCollection_GetFontFamily(collection, i, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFamily_Release(family);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFont_Release(font);

    i = 1;
    file = NULL;
    hr = IDWriteFontFace_GetFiles(fontface, &i, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(file != NULL, "got %p\n", file);

    hr = IDWriteFontFile_GetLoader(file, &loader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    hr = IDWriteFontFileLoader_QueryInterface(loader, &IID_IDWriteLocalFontFileLoader, (void**)&localloader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteLocalFontFileLoader_Release(localloader);

    /* local loader is not registered by default */
    hr = IDWriteFactory_RegisterFontFileLoader(factory, loader);
    ok(hr == S_OK || broken(hr == DWRITE_E_ALREADYREGISTERED), "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontFileLoader(factory, loader);
    ok(hr == S_OK || broken(hr == E_INVALIDARG), "got 0x%08x\n", hr);

    /* try with a different factory */
    factory2 = create_factory();
    hr = IDWriteFactory_RegisterFontFileLoader(factory2, loader);
    ok(hr == S_OK || broken(hr == DWRITE_E_ALREADYREGISTERED), "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontFileLoader(factory2, loader);
    ok(hr == DWRITE_E_ALREADYREGISTERED, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontFileLoader(factory2, loader);
    ok(hr == S_OK || broken(hr == E_INVALIDARG), "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontFileLoader(factory2, loader);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    IDWriteFactory_Release(factory2);

    IDWriteFontFileLoader_Release(loader);

    ret = TRUE;
    i = 0;
    hr = IDWriteFontCollection_FindFamilyName(collection, blahW, &i, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!ret, "got %d\n", ret);
    ok(i == (UINT32)-1, "got %u\n", i);

    IDWriteFontCollection_Release(collection);
    IDWriteFactory_Release(factory);
}

static void test_ConvertFontFaceToLOGFONT(void)
{
    DWRITE_FONT_SIMULATIONS sim;
    IDWriteGdiInterop *interop;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfEscapement = 100;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    logfont.lfUnderline = 1;
    logfont.lfStrikeOut = 1;

    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    sim = IDWriteFont_GetSimulations(font);
    ok(sim == DWRITE_FONT_SIMULATIONS_OBLIQUE, "sim %d\n", sim);

    sim = IDWriteFontFace_GetSimulations(fontface);
    ok(sim == DWRITE_FONT_SIMULATIONS_OBLIQUE, "sim %d\n", sim);

    IDWriteFont_Release(font);

if (0) /* crashes on native */
{
    hr = IDWriteGdiInterop_ConvertFontFaceToLOGFONT(interop, NULL, NULL);
    hr = IDWriteGdiInterop_ConvertFontFaceToLOGFONT(interop, fontface, NULL);
}

    memset(&logfont, 0xa, sizeof(logfont));
    logfont.lfFaceName[0] = 0;

    hr = IDWriteGdiInterop_ConvertFontFaceToLOGFONT(interop, fontface, &logfont);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(logfont.lfHeight == 0, "got %d\n", logfont.lfHeight);
    ok(logfont.lfWidth == 0, "got %d\n", logfont.lfWidth);
    ok(logfont.lfWeight == FW_NORMAL, "got %d\n", logfont.lfWeight);
    ok(logfont.lfEscapement == 0, "got %d\n", logfont.lfEscapement);
    ok(logfont.lfItalic == 1, "got %d\n", logfont.lfItalic);
    ok(logfont.lfUnderline == 0, "got %d\n", logfont.lfUnderline);
    ok(logfont.lfStrikeOut == 0, "got %d\n", logfont.lfStrikeOut);
    ok(!lstrcmpW(logfont.lfFaceName, tahomaW), "got %s\n", wine_dbgstr_w(logfont.lfFaceName));

    IDWriteGdiInterop_Release(interop);
    IDWriteFontFace_Release(fontface);
    IDWriteFactory_Release(factory);
}

static HRESULT WINAPI fontfileenumerator_QueryInterface(IDWriteFontFileEnumerator *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileEnumerator))
    {
        *obj = iface;
        IDWriteFontFileEnumerator_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI fontfileenumerator_AddRef(IDWriteFontFileEnumerator *iface)
{
    return 2;
}

static ULONG WINAPI fontfileenumerator_Release(IDWriteFontFileEnumerator *iface)
{
    return 1;
}

static HRESULT WINAPI fontfileenumerator_GetCurrentFontFile(IDWriteFontFileEnumerator *iface, IDWriteFontFile **file)
{
    *file = NULL;
    return E_FAIL;
}

static HRESULT WINAPI fontfileenumerator_MoveNext(IDWriteFontFileEnumerator *iface, BOOL *current)
{
    *current = FALSE;
    return S_OK;
}

static const struct IDWriteFontFileEnumeratorVtbl dwritefontfileenumeratorvtbl =
{
    fontfileenumerator_QueryInterface,
    fontfileenumerator_AddRef,
    fontfileenumerator_Release,
    fontfileenumerator_MoveNext,
    fontfileenumerator_GetCurrentFontFile,
};

static HRESULT WINAPI fontcollectionloader_QueryInterface(IDWriteFontCollectionLoader *iface, REFIID riid, void **obj)
{
    *obj = iface;
    return S_OK;
}

static ULONG WINAPI fontcollectionloader_AddRef(IDWriteFontCollectionLoader *iface)
{
    return 2;
}

static ULONG WINAPI fontcollectionloader_Release(IDWriteFontCollectionLoader *iface)
{
    return 1;
}

static HRESULT WINAPI fontcollectionloader_CreateEnumeratorFromKey(IDWriteFontCollectionLoader *iface, IDWriteFactory *factory, const void *key,
    UINT32 key_size, IDWriteFontFileEnumerator **ret)
{
    static IDWriteFontFileEnumerator enumerator = { &dwritefontfileenumeratorvtbl };
    *ret = &enumerator;
    return S_OK;
}

static const struct IDWriteFontCollectionLoaderVtbl dwritefontcollectionloadervtbl = {
    fontcollectionloader_QueryInterface,
    fontcollectionloader_AddRef,
    fontcollectionloader_Release,
    fontcollectionloader_CreateEnumeratorFromKey
};

static void test_CustomFontCollection(void)
{
    static const WCHAR fontnameW[] = {'w','i','n','e','_','t','e','s','t',0};
    IDWriteFontCollectionLoader collection = { &dwritefontcollectionloadervtbl };
    IDWriteFontCollectionLoader collection2 = { &dwritefontcollectionloadervtbl };
    IDWriteFontCollectionLoader collection3 = { &dwritefontcollectionloadervtbl };
    IDWriteFontCollection *font_collection = NULL;
    static IDWriteFontFileLoader rloader = { &resourcefontfileloadervtbl };
    struct test_fontcollectionloader resource_collection = { { &resourcecollectionloadervtbl }, &rloader };
    IDWriteFontFamily *family, *family2, *family3;
    IDWriteFontFace *idfontface, *idfontface2;
    IDWriteFontFile *fontfile, *fontfile2;
    IDWriteLocalizedStrings *string;
    IDWriteFont *idfont, *idfont2;
    IDWriteFactory *factory;
    UINT32 index, count;
    BOOL exists;
    HRESULT hr;
    HRSRC font;

    factory = create_factory();

    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, &collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, &collection2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, &collection);
    ok(hr == DWRITE_E_ALREADYREGISTERED, "got 0x%08x\n", hr);

    hr = IDWriteFactory_RegisterFontFileLoader(factory, &rloader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontCollectionLoader(factory, &resource_collection.IDWriteFontFileCollectionLoader_iface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateCustomFontCollection(factory, &collection3, "Billy", 6, &font_collection);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateCustomFontCollection(factory, &collection, "Billy", 6, &font_collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontCollection_Release(font_collection);

    hr = IDWriteFactory_CreateCustomFontCollection(factory, &collection2, "Billy", 6, &font_collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontCollection_Release(font_collection);

    hr = IDWriteFactory_CreateCustomFontCollection(factory, (IDWriteFontCollectionLoader*)0xdeadbeef, "Billy", 6, &font_collection);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    font = FindResourceA(GetModuleHandleA(NULL), (LPCSTR)MAKEINTRESOURCE(1), (LPCSTR)RT_RCDATA);
    ok(font != NULL, "Failed to find font resource\n");

    hr = IDWriteFactory_CreateCustomFontCollection(factory, &resource_collection.IDWriteFontFileCollectionLoader_iface,
        &font, sizeof(HRSRC), &font_collection);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    index = 1;
    exists = FALSE;
    hr = IDWriteFontCollection_FindFamilyName(font_collection, fontnameW, &index, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(index == 0, "got index %i\n", index);
    ok(exists, "got exists %i\n", exists);

    count = IDWriteFontCollection_GetFontFamilyCount(font_collection);
    ok(count == 1, "got %u\n", count);

    family = NULL;
    hr = IDWriteFontCollection_GetFontFamily(font_collection, 0, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(family, 1);

    family2 = NULL;
    hr = IDWriteFontCollection_GetFontFamily(font_collection, 0, &family2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(family2, 1);
    ok(family != family2, "got %p, %p\n", family, family2);

    hr = IDWriteFontFamily_GetFont(family, 0, &idfont);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(idfont, 1);
    EXPECT_REF(family, 2);
    hr = IDWriteFontFamily_GetFont(family, 0, &idfont2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(idfont2, 1);
    EXPECT_REF(family, 3);
    ok(idfont != idfont2, "got %p, %p\n", idfont, idfont2);
    IDWriteFont_Release(idfont2);

    hr = IDWriteFont_GetInformationalStrings(idfont, DWRITE_INFORMATIONAL_STRING_COPYRIGHT_NOTICE, &string, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists, "got %d\n", exists);
    EXPECT_REF(string, 1);

    family3 = NULL;
    hr = IDWriteFont_GetFontFamily(idfont, &family3);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(family, 3);
    ok(family == family3, "got %p, %p\n", family, family3);
    IDWriteFontFamily_Release(family3);

    idfontface = NULL;
    hr = IDWriteFont_CreateFontFace(idfont, &idfontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(idfont, 1);

    idfont2 = NULL;
    hr = IDWriteFontFamily_GetFont(family2, 0, &idfont2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(idfont2, 1);
    EXPECT_REF(idfont, 1);
    ok(idfont2 != idfont, "Font instances should not match\n");

    idfontface2 = NULL;
    hr = IDWriteFont_CreateFontFace(idfont2, &idfontface2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(idfontface2 == idfontface, "fontfaces should match\n");

    index = 1;
    fontfile = NULL;
    hr = IDWriteFontFace_GetFiles(idfontface, &index, &fontfile);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    index = 1;
    fontfile2 = NULL;
    hr = IDWriteFontFace_GetFiles(idfontface2, &index, &fontfile2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fontfile == fontfile2, "fontfiles should match\n");

    IDWriteFont_Release(idfont);
    IDWriteFont_Release(idfont2);
    IDWriteFontFile_Release(fontfile);
    IDWriteFontFile_Release(fontfile2);
    IDWriteFontFace_Release(idfontface);
    IDWriteFontFace_Release(idfontface2);
    IDWriteFontFamily_Release(family2);
    IDWriteFontFamily_Release(family);
    IDWriteFontCollection_Release(font_collection);

    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, &collection);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, &collection);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, &collection2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontCollectionLoader(factory, &resource_collection.IDWriteFontFileCollectionLoader_iface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontFileLoader(factory, &rloader);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteFactory_Release(factory);
}

static HRESULT WINAPI fontfileloader_QueryInterface(IDWriteFontFileLoader *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader))
    {
        *obj = iface;
        IDWriteFontFileLoader_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI fontfileloader_AddRef(IDWriteFontFileLoader *iface)
{
    return 2;
}

static ULONG WINAPI fontfileloader_Release(IDWriteFontFileLoader *iface)
{
    return 1;
}

static HRESULT WINAPI fontfileloader_CreateStreamFromKey(IDWriteFontFileLoader *iface, const void *fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream **fontFileStream)
{
    return 0x8faecafe;
}

static const struct IDWriteFontFileLoaderVtbl dwritefontfileloadervtbl = {
    fontfileloader_QueryInterface,
    fontfileloader_AddRef,
    fontfileloader_Release,
    fontfileloader_CreateStreamFromKey
};

static void test_CreateCustomFontFileReference(void)
{
    IDWriteFontFileLoader floader = { &dwritefontfileloadervtbl };
    IDWriteFontFileLoader floader2 = { &dwritefontfileloadervtbl };
    IDWriteFontFileLoader floader3 = { &dwritefontfileloadervtbl };
    IDWriteFactory *factory, *factory2;
    IDWriteFontFile *file, *file2;
    BOOL support;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    UINT32 count;
    IDWriteFontFace *face, *face2;
    HRESULT hr;
    HRSRC fontrsrc;
    UINT32 codePoints[1] = {0xa8};
    UINT16 indices[2];

    factory = create_factory();
    factory2 = create_factory();

    hr = IDWriteFactory_RegisterFontFileLoader(factory, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontFileLoader(factory, &floader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontFileLoader(factory, &floader2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontFileLoader(factory, &floader);
    ok(hr == DWRITE_E_ALREADYREGISTERED, "got 0x%08x\n", hr);
    hr = IDWriteFactory_RegisterFontFileLoader(factory, &rloader);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    file = NULL;
    hr = IDWriteFactory_CreateCustomFontFileReference(factory, "test", 4, &floader, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    hr = IDWriteFactory_CreateCustomFontFileReference(factory, "test", 4, &floader3, &file);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateCustomFontFileReference(factory, "test", 4, NULL, &file);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    file = NULL;
    hr = IDWriteFactory_CreateCustomFontFileReference(factory, "test", 4, &floader, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    file_type = DWRITE_FONT_FILE_TYPE_TRUETYPE;
    face_type = DWRITE_FONT_FACE_TYPE_TRUETYPE;
    support = TRUE;
    count = 1;
    hr = IDWriteFontFile_Analyze(file, &support, &file_type, &face_type, &count);
    ok(hr == 0x8faecafe, "got 0x%08x\n", hr);
    ok(support == FALSE, "got %i\n", support);
    ok(file_type == DWRITE_FONT_FILE_TYPE_UNKNOWN, "got %i\n", file_type);
    ok(face_type == DWRITE_FONT_FACE_TYPE_UNKNOWN, "got %i\n", face_type);
    ok(count == 0, "got %i\n", count);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_CFF, 1, &file, 0, 0, &face);
    ok(hr == 0x8faecafe, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    fontrsrc = FindResourceA(GetModuleHandleA(NULL), (LPCSTR)MAKEINTRESOURCE(1), (LPCSTR)RT_RCDATA);
    ok(fontrsrc != NULL, "Failed to find font resource\n");

    hr = IDWriteFactory_CreateCustomFontFileReference(factory, &fontrsrc, sizeof(HRSRC), &rloader, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    support = FALSE;
    count = 0;
    hr = IDWriteFontFile_Analyze(file, &support, &file_type, &face_type, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(support == TRUE, "got %i\n", support);
    ok(file_type == DWRITE_FONT_FILE_TYPE_TRUETYPE, "got %i\n", file_type);
    ok(face_type == DWRITE_FONT_FACE_TYPE_TRUETYPE, "got %i\n", face_type);
    ok(count == 1, "got %i\n", count);

    /* invalid index */
    hr = IDWriteFactory_CreateFontFace(factory, face_type, 1, &file, 1, DWRITE_FONT_SIMULATIONS_NONE, &face);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, face_type, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &face);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, face_type, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &face2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    /* fontface instances are reused starting with win7 */
    ok(face == face2 || broken(face != face2), "got %p, %p\n", face, face2);
    IDWriteFontFace_Release(face2);

    /* file was created with different factory */
    face2 = NULL;
    hr = IDWriteFactory_CreateFontFace(factory2, face_type, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &face2);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);
if (face2)
    IDWriteFontFace_Release(face2);

    file2 = NULL;
    hr = IDWriteFactory_CreateCustomFontFileReference(factory, &fontrsrc, sizeof(HRSRC), &rloader, &file2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(file != file2, "got %p, %p\n", file, file2);

    hr = IDWriteFactory_CreateFontFace(factory, face_type, 1, &file2, 0, DWRITE_FONT_SIMULATIONS_NONE, &face2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    /* fontface instances are reused starting with win7 */
    ok(face == face2 || broken(face != face2), "got %p, %p\n", face, face2);
    IDWriteFontFace_Release(face2);
    IDWriteFontFile_Release(file2);

    hr = IDWriteFontFace_GetGlyphIndices(face, NULL, 0, NULL);
    ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "got 0x%08x\n", hr);

    hr = IDWriteFontFace_GetGlyphIndices(face, codePoints, 0, NULL);
    ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "got 0x%08x\n", hr);

    hr = IDWriteFontFace_GetGlyphIndices(face, codePoints, 0, indices);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontFace_GetGlyphIndices(face, NULL, 0, indices);
    ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "got 0x%08x\n", hr);

    indices[0] = indices[1] = 11;
    hr = IDWriteFontFace_GetGlyphIndices(face, NULL, 1, indices);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(indices[0] == 0, "got index %i\n", indices[0]);
    ok(indices[1] == 11, "got index %i\n", indices[1]);

if (0) /* crashes on native */
    hr = IDWriteFontFace_GetGlyphIndices(face, NULL, 1, NULL);

    hr = IDWriteFontFace_GetGlyphIndices(face, codePoints, 1, indices);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(indices[0] == 6, "got index %i\n", indices[0]);
    IDWriteFontFace_Release(face);
    IDWriteFontFile_Release(file);

    hr = IDWriteFactory_UnregisterFontFileLoader(factory, &floader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontFileLoader(factory, &floader);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontFileLoader(factory, &floader2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory_UnregisterFontFileLoader(factory, &rloader);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteFactory_Release(factory2);
    IDWriteFactory_Release(factory);
}

static void test_CreateFontFileReference(void)
{
    HRESULT hr;
    IDWriteFontFile *ffile = NULL;
    BOOL support;
    DWRITE_FONT_FILE_TYPE type;
    DWRITE_FONT_FACE_TYPE face;
    UINT32 count;
    IDWriteFontFace *fface = NULL;
    IDWriteFactory *factory;
    WCHAR *path;

    path = create_testfontfile(test_fontfile);
    factory = create_factory();

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &ffile);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    support = FALSE;
    type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    face = DWRITE_FONT_FACE_TYPE_CFF;
    count = 0;
    hr = IDWriteFontFile_Analyze(ffile, &support, &type, &face, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(support == TRUE, "got %i\n", support);
    ok(type == DWRITE_FONT_FILE_TYPE_TRUETYPE, "got %i\n", type);
    ok(face == DWRITE_FONT_FACE_TYPE_TRUETYPE, "got %i\n", face);
    ok(count == 1, "got %i\n", count);

    hr = IDWriteFactory_CreateFontFace(factory, face, 1, &ffile, 0, DWRITE_FONT_SIMULATIONS_NONE, &fface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteFontFace_Release(fface);
    IDWriteFontFile_Release(ffile);
    IDWriteFactory_Release(factory);

    DELETE_FONTFILE(path);
}

static void test_shared_isolated(void)
{
    IDWriteFactory *isolated, *isolated2;
    IDWriteFactory *shared, *shared2;
    HRESULT hr;

    /* invalid type */
    shared = NULL;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED+1, &IID_IDWriteFactory, (IUnknown**)&shared);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(shared != NULL, "got %p\n", shared);
    IDWriteFactory_Release(shared);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&shared);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&shared2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(shared == shared2, "got %p, and %p\n", shared, shared2);

    IDWriteFactory_Release(shared);
    IDWriteFactory_Release(shared2);

    /* we got 2 references, released 2 - still same pointer is returned */
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&shared2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(shared == shared2, "got %p, and %p\n", shared, shared2);
    IDWriteFactory_Release(shared2);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&isolated);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&isolated2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(isolated != isolated2, "got %p, and %p\n", isolated, isolated2);
    IDWriteFactory_Release(isolated2);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED+1, &IID_IDWriteFactory, (IUnknown**)&isolated2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(shared != isolated2, "got %p, and %p\n", shared, isolated2);

    IDWriteFactory_Release(isolated);
    IDWriteFactory_Release(isolated2);
}

static void test_GetUnicodeRanges(void)
{
    DWRITE_UNICODE_RANGE *ranges, r;
    IDWriteFontFile *ffile = NULL;
    IDWriteFontFace1 *fontface1;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    UINT32 count;
    HRESULT hr;
    HRSRC font;

    factory = create_factory();

    hr = IDWriteFactory_RegisterFontFileLoader(factory, &rloader);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    font = FindResourceA(GetModuleHandleA(NULL), (LPCSTR)MAKEINTRESOURCE(1), (LPCSTR)RT_RCDATA);
    ok(font != NULL, "Failed to find font resource\n");

    hr = IDWriteFactory_CreateCustomFontFileReference(factory, &font, sizeof(HRSRC), &rloader, &ffile);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &ffile, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(ffile);

    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
    IDWriteFontFace_Release(fontface);
    if (hr != S_OK) {
        win_skip("GetUnicodeRanges() is not supported.\n");
        IDWriteFactory_Release(factory);
        return;
    }

    count = 0;
    hr = IDWriteFontFace1_GetUnicodeRanges(fontface1, 0, NULL, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(count > 0, "got %u\n", count);

    count = 1;
    hr = IDWriteFontFace1_GetUnicodeRanges(fontface1, 1, NULL, &count);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(count == 0, "got %u\n", count);

    count = 0;
    hr = IDWriteFontFace1_GetUnicodeRanges(fontface1, 1, &r, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(count > 1, "got %u\n", count);

    ranges = heap_alloc(count*sizeof(DWRITE_UNICODE_RANGE));
    hr = IDWriteFontFace1_GetUnicodeRanges(fontface1, count, ranges, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ranges[0].first = ranges[0].last = 0;
    hr = IDWriteFontFace1_GetUnicodeRanges(fontface1, 1, ranges, &count);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got 0x%08x\n", hr);
    ok(ranges[0].first != 0 && ranges[0].last != 0, "got 0x%x-0x%0x\n", ranges[0].first, ranges[0].last);

    heap_free(ranges);

    hr = IDWriteFactory_UnregisterFontFileLoader(factory, &rloader);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteFontFace1_Release(fontface1);
    IDWriteFactory_Release(factory);
}

static void test_GetFontFromFontFace(void)
{
    IDWriteFontFace *fontface, *fontface2;
    IDWriteFontCollection *collection;
    IDWriteFont *font, *font2, *font3;
    IDWriteFontFamily *family;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    WCHAR *path;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontCollection_GetFontFamily(collection, 0, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    font2 = NULL;
    hr = IDWriteFontCollection_GetFontFromFontFace(collection, fontface, &font2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(font2 != font, "got %p, %p\n", font2, font);

    font3 = NULL;
    hr = IDWriteFontCollection_GetFontFromFontFace(collection, fontface, &font3);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(font3 != font && font3 != font2, "got %p, %p, %p\n", font3, font2, font);

    hr = IDWriteFont_CreateFontFace(font2, &fontface2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fontface2 == fontface, "got %p, %p\n", fontface2, fontface);
    IDWriteFontFace_Release(fontface2);

    hr = IDWriteFont_CreateFontFace(font3, &fontface2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(fontface2 == fontface, "got %p, %p\n", fontface2, fontface);
    IDWriteFontFace_Release(fontface2);
    IDWriteFontFace_Release(fontface);
    IDWriteFont_Release(font3);
    IDWriteFactory_Release(factory);

    /* fontface that wasn't created from this collection */
    factory = create_factory();
    path = create_testfontfile(test_fontfile);

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    hr = IDWriteFontCollection_GetFontFromFontFace(collection, fontface, &font3);
    ok(hr == DWRITE_E_NOFONT, "got 0x%08x\n", hr);
    ok(font3 == NULL, "got %p\n", font3);
    IDWriteFontFace_Release(fontface);

    IDWriteFont_Release(font);
    IDWriteFont_Release(font2);
    IDWriteFontFamily_Release(family);
    IDWriteFontCollection_Release(collection);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static IDWriteFont *get_tahoma_instance(IDWriteFactory *factory, DWRITE_FONT_STYLE style)
{
    IDWriteFontCollection *collection;
    IDWriteFontFamily *family;
    IDWriteFont *font;
    UINT32 index;
    BOOL exists;
    HRESULT hr;

    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    index = ~0;
    exists = FALSE;
    hr = IDWriteFontCollection_FindFamilyName(collection, tahomaW, &index, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists, "got %d\n", exists);

    hr = IDWriteFontCollection_GetFontFamily(collection, index, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, style, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDWriteFontFamily_Release(family);
    IDWriteFontCollection_Release(collection);
    return font;
}

static void test_GetFirstMatchingFont(void)
{
    DWRITE_FONT_SIMULATIONS simulations;
    IDWriteFont *font, *font2;
    IDWriteFactory *factory;

    factory = create_factory();

    font = get_tahoma_instance(factory, DWRITE_FONT_STYLE_NORMAL);
    font2 = get_tahoma_instance(factory, DWRITE_FONT_STYLE_NORMAL);
    ok(font != font2, "got %p, %p\n", font, font2);
    IDWriteFont_Release(font);
    IDWriteFont_Release(font2);

    font = get_tahoma_instance(factory, DWRITE_FONT_STYLE_ITALIC);
    simulations = IDWriteFont_GetSimulations(font);
    ok(simulations == DWRITE_FONT_SIMULATIONS_OBLIQUE, "%d\n", simulations);
    IDWriteFont_Release(font);

    IDWriteFactory_Release(factory);
}

static void test_GetInformationalStrings(void)
{
    IDWriteLocalizedStrings *strings, *strings2;
    IDWriteFontCollection *collection;
    IDWriteFontFamily *family;
    IDWriteFactory *factory;
    IDWriteFont *font;
    BOOL exists;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontCollection_GetFontFamily(collection, 0, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    exists = TRUE;
    strings = (void*)0xdeadbeef;
    hr = IDWriteFont_GetInformationalStrings(font, DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME+1, &strings, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == FALSE, "got %d\n", exists);
    ok(strings == NULL, "got %p\n", strings);

    exists = TRUE;
    strings = NULL;
    hr = IDWriteFont_GetInformationalStrings(font, DWRITE_INFORMATIONAL_STRING_NONE, &strings, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == FALSE, "got %d\n", exists);

    exists = FALSE;
    strings = NULL;
    hr = IDWriteFont_GetInformationalStrings(font, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &strings, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == TRUE, "got %d\n", exists);

    /* strings instance is not reused */
    strings2 = NULL;
    hr = IDWriteFont_GetInformationalStrings(font, DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &strings2, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(strings2 != strings, "got %p, %p\n", strings2, strings);

    IDWriteLocalizedStrings_Release(strings);
    IDWriteLocalizedStrings_Release(strings2);
    IDWriteFont_Release(font);
    IDWriteFontFamily_Release(family);
    IDWriteFontCollection_Release(collection);
    IDWriteFactory_Release(factory);
}

static void test_GetGdiInterop(void)
{
    IDWriteGdiInterop *interop, *interop2;
    IDWriteFactory *factory, *factory2;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;

    factory = create_factory();

    interop = NULL;
    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    interop2 = NULL;
    hr = IDWriteFactory_GetGdiInterop(factory, &interop2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(interop == interop2, "got %p, %p\n", interop, interop2);
    IDWriteGdiInterop_Release(interop2);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&factory2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* each factory gets its own interop */
    interop2 = NULL;
    hr = IDWriteFactory_GetGdiInterop(factory2, &interop2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(interop != interop2, "got %p, %p\n", interop, interop2);

    /* release factory - interop still works */
    IDWriteFactory_Release(factory2);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop2, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFont_Release(font);

    IDWriteGdiInterop_Release(interop2);
    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_CreateFontFaceFromHdc(void)
{
    IDWriteGdiInterop *interop;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    HFONT hfont, oldhfont;
    LOGFONTW logfont;
    HRESULT hr;
    HDC hdc;

    factory = create_factory();

    interop = NULL;
    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteGdiInterop_CreateFontFaceFromHdc(interop, NULL, &fontface);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hfont = CreateFontIndirectW(&logfont);
    hdc = CreateCompatibleDC(0);
    oldhfont = SelectObject(hdc, hfont);

    fontface = NULL;
    hr = IDWriteGdiInterop_CreateFontFaceFromHdc(interop, hdc, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFace_Release(fontface);
    DeleteObject(SelectObject(hdc, oldhfont));
    DeleteDC(hdc);

    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_GetSimulations(void)
{
    DWRITE_FONT_SIMULATIONS simulations;
    IDWriteGdiInterop *interop;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    IDWriteFont *font;
    LOGFONTW logfont;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    simulations = IDWriteFont_GetSimulations(font);
    ok(simulations == DWRITE_FONT_SIMULATIONS_OBLIQUE, "got %d\n", simulations);
    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    simulations = IDWriteFontFace_GetSimulations(fontface);
    ok(simulations == DWRITE_FONT_SIMULATIONS_OBLIQUE, "got %d\n", simulations);
    IDWriteFontFace_Release(fontface);
    IDWriteFont_Release(font);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 0;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    simulations = IDWriteFont_GetSimulations(font);
    ok(simulations == DWRITE_FONT_SIMULATIONS_NONE, "got %d\n", simulations);
    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    simulations = IDWriteFontFace_GetSimulations(fontface);
    ok(simulations == DWRITE_FONT_SIMULATIONS_NONE, "got %d\n", simulations);
    IDWriteFontFace_Release(fontface);
    IDWriteFont_Release(font);

    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_GetFaceNames(void)
{
    static const WCHAR obliqueW[] = {'O','b','l','i','q','u','e',0};
    static const WCHAR enus2W[] = {'e','n','-','U','s',0};
    static const WCHAR enusW[] = {'e','n','-','u','s',0};
    IDWriteLocalizedStrings *strings, *strings2;
    IDWriteGdiInterop *interop;
    IDWriteFactory *factory;
    UINT32 count, index;
    IDWriteFont *font;
    LOGFONTW logfont;
    WCHAR buffW[255];
    BOOL exists;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWidth  = 12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 1;
    lstrcpyW(logfont.lfFaceName, tahomaW);

    hr = IDWriteGdiInterop_CreateFontFromLOGFONT(interop, &logfont, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_GetFaceNames(font, &strings);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFont_GetFaceNames(font, &strings2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(strings != strings2, "got %p, %p\n", strings2, strings);
    IDWriteLocalizedStrings_Release(strings2);

    count = IDWriteLocalizedStrings_GetCount(strings);
    ok(count == 1, "got %d\n", count);

    index = 1;
    exists = FALSE;
    hr = IDWriteLocalizedStrings_FindLocaleName(strings, enus2W, &index, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(index == 0 && exists, "got %d, %d\n", index, exists);

    count = 0;
    hr = IDWriteLocalizedStrings_GetLocaleNameLength(strings, 1, &count);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(count == ~0, "got %d\n", count);

    /* for simulated faces names are also simulated */
    buffW[0] = 0;
    hr = IDWriteLocalizedStrings_GetLocaleName(strings, 0, buffW, sizeof(buffW)/sizeof(WCHAR));
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, enusW), "got %s\n", wine_dbgstr_w(buffW));

    buffW[0] = 0;
    hr = IDWriteLocalizedStrings_GetString(strings, 0, buffW, sizeof(buffW)/sizeof(WCHAR));
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, obliqueW), "got %s\n", wine_dbgstr_w(buffW));
    IDWriteLocalizedStrings_Release(strings);

    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

struct local_refkey
{
    FILETIME writetime;
    WCHAR name[1];
};

static void test_TryGetFontTable(void)
{
    IDWriteLocalFontFileLoader *localloader;
    WIN32_FILE_ATTRIBUTE_DATA info;
    const struct local_refkey *key;
    IDWriteFontFileLoader *loader;
    const void *table, *table2;
    IDWriteFontFace *fontface;
    void *context, *context2;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    WCHAR buffW[MAX_PATH];
    BOOL exists, ret;
    UINT32 size, len;
    WCHAR *path;
    HRESULT hr;

    path = create_testfontfile(test_fontfile);

    factory = create_factory();

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    key = NULL;
    size = 0;
    hr = IDWriteFontFile_GetReferenceKey(file, (const void**)&key, &size);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(size != 0, "got %u\n", size);

    ret = GetFileAttributesExW(path, GetFileExInfoStandard, &info);
    ok(ret, "got %d\n", ret);
    ok(!memcmp(&info.ftLastWriteTime, &key->writetime, sizeof(key->writetime)), "got wrong write time\n");

    hr = IDWriteFontFile_GetLoader(file, &loader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFileLoader_QueryInterface(loader, &IID_IDWriteLocalFontFileLoader, (void**)&localloader);
    IDWriteFontFileLoader_Release(loader);

    hr = IDWriteLocalFontFileLoader_GetFilePathLengthFromKey(localloader, key, size, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(lstrlenW(key->name) == len, "path length %d\n", len);

    hr = IDWriteLocalFontFileLoader_GetFilePathFromKey(localloader, key, size, buffW, sizeof(buffW)/sizeof(WCHAR));
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(buffW, key->name), "got %s, expected %s\n", wine_dbgstr_w(buffW), wine_dbgstr_w(key->name));
    IDWriteLocalFontFileLoader_Release(localloader);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file, 0, 0, &fontface);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    exists = FALSE;
    context = (void*)0xdeadbeef;
    table = NULL;
    hr = IDWriteFontFace_TryGetFontTable(fontface, MS_CMAP_TAG, &table, &size, &context, &exists);
    ok(hr == S_OK, "got 0x%08x\n",hr);
    ok(exists == TRUE, "got %d\n", exists);
    ok(context == NULL && table != NULL, "cmap: context %p, table %p\n", context, table);

    exists = FALSE;
    context2 = (void*)0xdeadbeef;
    table2 = NULL;
    hr = IDWriteFontFace_TryGetFontTable(fontface, MS_CMAP_TAG, &table2, &size, &context2, &exists);
    ok(hr == S_OK, "got 0x%08x\n",hr);
    ok(exists == TRUE, "got %d\n", exists);
    ok(context2 == context && table2 == table, "cmap: context2 %p, table2 %p\n", context2, table2);

    IDWriteFontFace_ReleaseFontTable(fontface, context2);
    IDWriteFontFace_ReleaseFontTable(fontface, context);

    IDWriteFontFace_Release(fontface);
    IDWriteFontFile_Release(file);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_ConvertFontToLOGFONT(void)
{
    IDWriteFactory *factory, *factory2;
    IDWriteFontCollection *collection;
    IDWriteGdiInterop *interop;
    IDWriteFontFamily *family;
    IDWriteFont *font;
    LOGFONTW logfont;
    BOOL system;
    HRESULT hr;

    factory = create_factory();
    factory2 = create_factory();

    interop = NULL;
    hr = IDWriteFactory_GetGdiInterop(factory, &interop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_GetSystemFontCollection(factory2, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontCollection_GetFontFamily(collection, 0, &family);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if (0) { /* crashes on native */
    IDWriteGdiInterop_ConvertFontToLOGFONT(interop, NULL, NULL, NULL);
    IDWriteGdiInterop_ConvertFontToLOGFONT(interop, NULL, &logfont, NULL);
    IDWriteGdiInterop_ConvertFontToLOGFONT(interop, font, NULL, &system);
}
    system = TRUE;
    hr = IDWriteGdiInterop_ConvertFontToLOGFONT(interop, NULL, &logfont, &system);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(!system, "got %d\n", system);

    system = FALSE;
    memset(&logfont, 0, sizeof(logfont));
    hr = IDWriteGdiInterop_ConvertFontToLOGFONT(interop, font, &logfont, &system);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(system, "got %d\n", system);
    ok(logfont.lfFaceName[0] != 0, "got face name %s\n", wine_dbgstr_w(logfont.lfFaceName));

    IDWriteFactory_Release(factory2);

    IDWriteFontCollection_Release(collection);
    IDWriteFontFamily_Release(family);
    IDWriteFont_Release(font);
    IDWriteGdiInterop_Release(interop);
    IDWriteFactory_Release(factory);
}

static void test_CreateStreamFromKey(void)
{
    IDWriteLocalFontFileLoader *localloader;
    IDWriteFontFileStream *stream, *stream2;
    IDWriteFontFileLoader *loader;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    UINT64 writetime;
    WCHAR *path;
    void *key;
    UINT32 size;
    HRESULT hr;

    factory = create_factory();

    path = create_testfontfile(test_fontfile);

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    key = NULL;
    size = 0;
    hr = IDWriteFontFile_GetReferenceKey(file, (const void**)&key, &size);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(size != 0, "got %u\n", size);

    hr = IDWriteFontFile_GetLoader(file, &loader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFileLoader_QueryInterface(loader, &IID_IDWriteLocalFontFileLoader, (void**)&localloader);
    IDWriteFontFileLoader_Release(loader);

    hr = IDWriteLocalFontFileLoader_CreateStreamFromKey(localloader, key, size, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(stream, 1);

    hr = IDWriteLocalFontFileLoader_CreateStreamFromKey(localloader, key, size, &stream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(stream == stream2 || broken(stream != stream2) /* Win7 SP0 */, "got %p, %p\n", stream, stream2);
    if (stream == stream2)
        EXPECT_REF(stream, 2);
    IDWriteFontFileStream_Release(stream);
    IDWriteFontFileStream_Release(stream2);

    hr = IDWriteLocalFontFileLoader_CreateStreamFromKey(localloader, key, size, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(stream, 1);

    writetime = 0;
    hr = IDWriteFontFileStream_GetLastWriteTime(stream, &writetime);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(writetime != 0, "got %08x%08x\n", (UINT)(writetime >> 32), (UINT)writetime);

    IDWriteFontFileStream_Release(stream);

    IDWriteLocalFontFileLoader_Release(localloader);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_ReadFileFragment(void)
{
    IDWriteLocalFontFileLoader *localloader;
    IDWriteFontFileStream *stream;
    IDWriteFontFileLoader *loader;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    const void *fragment, *fragment2;
    void *key, *context, *context2;
    UINT64 filesize;
    UINT32 size;
    WCHAR *path;
    HRESULT hr;

    factory = create_factory();

    path = create_testfontfile(test_fontfile);

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    key = NULL;
    size = 0;
    hr = IDWriteFontFile_GetReferenceKey(file, (const void**)&key, &size);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(size != 0, "got %u\n", size);

    hr = IDWriteFontFile_GetLoader(file, &loader);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFileLoader_QueryInterface(loader, &IID_IDWriteLocalFontFileLoader, (void**)&localloader);
    IDWriteFontFileLoader_Release(loader);

    hr = IDWriteLocalFontFileLoader_CreateStreamFromKey(localloader, key, size, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFontFileStream_GetFileSize(stream, &filesize);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* reading past the end of the stream */
    fragment = (void*)0xdeadbeef;
    context = (void*)0xdeadbeef;
    hr = IDWriteFontFileStream_ReadFileFragment(stream, &fragment, 0, filesize+1, &context);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);
    ok(context == NULL, "got %p\n", context);
    ok(fragment == NULL, "got %p\n", fragment);

    fragment = (void*)0xdeadbeef;
    context = (void*)0xdeadbeef;
    hr = IDWriteFontFileStream_ReadFileFragment(stream, &fragment, 0, filesize, &context);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(context == NULL, "got %p\n", context);
    ok(fragment != NULL, "got %p\n", fragment);

    fragment2 = (void*)0xdeadbeef;
    context2 = (void*)0xdeadbeef;
    hr = IDWriteFontFileStream_ReadFileFragment(stream, &fragment2, 0, filesize, &context2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(context2 == NULL, "got %p\n", context2);
    ok(fragment == fragment2, "got %p, %p\n", fragment, fragment2);

    IDWriteFontFileStream_ReleaseFileFragment(stream, context);
    IDWriteFontFileStream_ReleaseFileFragment(stream, context2);

    /* fragment is released, try again */
    fragment = (void*)0xdeadbeef;
    context = (void*)0xdeadbeef;
    hr = IDWriteFontFileStream_ReadFileFragment(stream, &fragment, 0, filesize, &context);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(context == NULL, "got %p\n", context);
    ok(fragment == fragment2, "got %p, %p\n", fragment, fragment2);
    IDWriteFontFileStream_ReleaseFileFragment(stream, context);

    IDWriteFontFileStream_Release(stream);
    IDWriteLocalFontFileLoader_Release(localloader);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_GetDesignGlyphMetrics(void)
{
    DWRITE_GLYPH_METRICS metrics[2];
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    UINT16 indices[2];
    UINT32 codepoint;
    WCHAR *path;
    HRESULT hr;

    factory = create_factory();

    path = create_testfontfile(test_fontfile);

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file,
        0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == S_OK, "got 0x%08x\n",hr);
    IDWriteFontFile_Release(file);

    codepoint = 'A';
    indices[0] = 0;
    hr = IDWriteFontFace_GetGlyphIndices(fontface, &codepoint, 1, &indices[0]);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(indices[0] > 0, "got %u\n", indices[0]);

    hr = IDWriteFontFace_GetDesignGlyphMetrics(fontface, NULL, 0, metrics, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%08x\n",hr);

    hr = IDWriteFontFace_GetDesignGlyphMetrics(fontface, NULL, 1, metrics, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%08x\n",hr);

    hr = IDWriteFontFace_GetDesignGlyphMetrics(fontface, indices, 0, metrics, FALSE);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    /* missing glyphs are ignored */
    indices[1] = 1;
    memset(metrics, 0xcc, sizeof(metrics));
    hr = IDWriteFontFace_GetDesignGlyphMetrics(fontface, indices, 2, metrics, FALSE);
    ok(hr == S_OK, "got 0x%08x\n",hr);
    ok(metrics[0].advanceWidth == 1000, "got %d\n", metrics[0].advanceWidth);
    ok(metrics[1].advanceWidth == 0, "got %d\n", metrics[1].advanceWidth);

    IDWriteFontFace_Release(fontface);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_IsMonospacedFont(void)
{
    static const WCHAR courierW[] = {'C','o','u','r','i','e','r',' ','N','e','w',0};
    IDWriteFontCollection *collection;
    IDWriteFactory *factory;
    UINT32 index;
    BOOL exists;
    HRESULT hr;

    factory = create_factory();
    hr = IDWriteFactory_GetSystemFontCollection(factory, &collection, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    exists = FALSE;
    hr = IDWriteFontCollection_FindFamilyName(collection, courierW, &index, &exists);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if (exists) {
        IDWriteFontFamily *family;
        IDWriteFont1 *font1;
        IDWriteFont *font;

        hr = IDWriteFontCollection_GetFontFamily(collection, index, &family);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = IDWriteFontFamily_GetFirstMatchingFont(family, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        IDWriteFontFamily_Release(family);

        hr = IDWriteFont_QueryInterface(font, &IID_IDWriteFont1, (void**)&font1);
        if (hr == S_OK) {
            IDWriteFontFace1 *fontface1;
            IDWriteFontFace *fontface;
            BOOL is_monospaced;

            is_monospaced = IDWriteFont1_IsMonospacedFont(font1);
            ok(is_monospaced, "got %d\n", is_monospaced);

            hr = IDWriteFont1_CreateFontFace(font1, &fontface);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            is_monospaced = IDWriteFontFace1_IsMonospacedFont(fontface1);
            ok(is_monospaced, "got %d\n", is_monospaced);
            IDWriteFontFace1_Release(fontface1);

            IDWriteFontFace_Release(fontface);
            IDWriteFont1_Release(font1);
        }
        else
            win_skip("IsMonospacedFont() is not supported.\n");
    }
    else
        skip("Courier New font not found.\n");

    IDWriteFontCollection_Release(collection);
}

static void test_GetDesignGlyphAdvances(void)
{
    IDWriteFontFace1 *fontface1;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    WCHAR *path;
    HRESULT hr;

    factory = create_factory();

    path = create_testfontfile(test_fontfile);

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file,
        0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
    if (hr == S_OK) {
        UINT32 codepoint;
        UINT16 index;
        INT32 advance;

        codepoint = 'A';
        index = 0;
        hr = IDWriteFontFace1_GetGlyphIndices(fontface1, &codepoint, 1, &index);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(index > 0, "got %u\n", index);

        advance = 0;
        hr = IDWriteFontFace1_GetDesignGlyphAdvances(fontface1, 1, &index, &advance, FALSE);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(advance == 1000, "got %i\n", advance);

        advance = 0;
        hr = IDWriteFontFace1_GetDesignGlyphAdvances(fontface1, 1, &index, &advance, TRUE);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(advance == 2048, "got %i\n", advance);

        IDWriteFontFace1_Release(fontface1);
    }
    else
        win_skip("GetDesignGlyphAdvances() is not supported.\n");

    IDWriteFontFace_Release(fontface);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_GetGlyphRunOutline(void)
{
    DWRITE_GLYPH_OFFSET offsets[2];
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    IDWriteFontFace *face;
    UINT32 codepoint;
    FLOAT advances[2];
    UINT16 glyphs[2];
    WCHAR *path;
    HRESULT hr;

    path = create_testfontfile(test_fontfile);
    factory = create_factory();

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n",hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &face);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    codepoint = 'A';
    glyphs[0] = 0;
    hr = IDWriteFontFace_GetGlyphIndices(face, &codepoint, 1, glyphs);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(glyphs[0] > 0, "got %u\n", glyphs[0]);
    glyphs[1] = glyphs[0];

    hr = IDWriteFontFace_GetGlyphRunOutline(face, 2048.0, glyphs, advances, offsets, 1, FALSE, FALSE, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IDWriteFontFace_GetGlyphRunOutline(face, 2048.0, NULL, NULL, offsets, 1, FALSE, FALSE, &test_geomsink);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    advances[0] = 1.0;
    advances[1] = 0.0;

    offsets[0].advanceOffset = 1.0;
    offsets[0].ascenderOffset = 1.0;
    offsets[1].advanceOffset = 0.0;
    offsets[1].ascenderOffset = 0.0;

    /* default advances, no offsets */
    memset(g_startpoints, 0, sizeof(g_startpoints));
    g_startpoint_count = 0;
    hr = IDWriteFontFace_GetGlyphRunOutline(face, 1024.0, glyphs, NULL, NULL, 2, FALSE, FALSE, &test_geomsink);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(g_startpoint_count == 2, "got %d\n", g_startpoint_count);
    if (g_startpoint_count == 2) {
        /* glyph advance of 500 is applied */
        ok(g_startpoints[0].x == 229.5 && g_startpoints[0].y == -629.0, "0: got (%.2f,%.2f)\n", g_startpoints[0].x, g_startpoints[0].y);
        ok(g_startpoints[1].x == 729.5 && g_startpoints[1].y == -629.0, "1: got (%.2f,%.2f)\n", g_startpoints[1].x, g_startpoints[1].y);
    }

    /* default advances, no offsets, RTL */
    memset(g_startpoints, 0, sizeof(g_startpoints));
    g_startpoint_count = 0;
    hr = IDWriteFontFace_GetGlyphRunOutline(face, 1024.0, glyphs, NULL, NULL, 2, FALSE, TRUE, &test_geomsink);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(g_startpoint_count == 2, "got %d\n", g_startpoint_count);
    if (g_startpoint_count == 2) {
        /* advance is -500 now */
        ok(g_startpoints[0].x == -270.5 && g_startpoints[0].y == -629.0, "0: got (%.2f,%.2f)\n", g_startpoints[0].x, g_startpoints[0].y);
        ok(g_startpoints[1].x == -770.5 && g_startpoints[1].y == -629.0, "1: got (%.2f,%.2f)\n", g_startpoints[1].x, g_startpoints[1].y);
    }

    /* default advances, additional offsets */
    memset(g_startpoints, 0, sizeof(g_startpoints));
    g_startpoint_count = 0;
    hr = IDWriteFontFace_GetGlyphRunOutline(face, 1024.0, glyphs, NULL, offsets, 2, FALSE, FALSE, &test_geomsink);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(g_startpoint_count == 2, "got %d\n", g_startpoint_count);
    if (g_startpoint_count == 2) {
        /* offsets applied to first contour */
        ok(g_startpoints[0].x == 230.5 && g_startpoints[0].y == -630.0, "0: got (%.2f,%.2f)\n", g_startpoints[0].x, g_startpoints[0].y);
        ok(g_startpoints[1].x == 729.5 && g_startpoints[1].y == -629.0, "1: got (%.2f,%.2f)\n", g_startpoints[1].x, g_startpoints[1].y);
    }

    /* default advances, additional offsets, RTL */
    memset(g_startpoints, 0, sizeof(g_startpoints));
    g_startpoint_count = 0;
    hr = IDWriteFontFace_GetGlyphRunOutline(face, 1024.0, glyphs, NULL, offsets, 2, FALSE, TRUE, &test_geomsink);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(g_startpoint_count == 2, "got %d\n", g_startpoint_count);
    if (g_startpoint_count == 2) {
        ok(g_startpoints[0].x == -271.5 && g_startpoints[0].y == -630.0, "0: got (%.2f,%.2f)\n", g_startpoints[0].x, g_startpoints[0].y);
        ok(g_startpoints[1].x == -770.5 && g_startpoints[1].y == -629.0, "1: got (%.2f,%.2f)\n", g_startpoints[1].x, g_startpoints[1].y);
    }

    /* custom advances and offsets, offset turns total advance value to zero */
    memset(g_startpoints, 0, sizeof(g_startpoints));
    g_startpoint_count = 0;
    hr = IDWriteFontFace_GetGlyphRunOutline(face, 1024.0, glyphs, advances, offsets, 2, FALSE, FALSE, &test_geomsink);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(g_startpoint_count == 2, "got %d\n", g_startpoint_count);
    if (g_startpoint_count == 2) {
        ok(g_startpoints[0].x == 230.5 && g_startpoints[0].y == -630.0, "0: got (%.2f,%.2f)\n", g_startpoints[0].x, g_startpoints[0].y);
        ok(g_startpoints[1].x == 230.5 && g_startpoints[1].y == -629.0, "1: got (%.2f,%.2f)\n", g_startpoints[1].x, g_startpoints[1].y);
    }

    IDWriteFontFace_Release(face);
    IDWriteFactory_Release(factory);

    DELETE_FONTFILE(path);
}

static void test_GetEudcFontCollection(void)
{
    IDWriteFontCollection *coll, *coll2;
    IDWriteFactory1 *factory1;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_QueryInterface(factory, &IID_IDWriteFactory1, (void**)&factory1);
    IDWriteFactory_Release(factory);
    if (hr != S_OK) {
        win_skip("GetEudcFontCollection() is not supported.\n");
        return;
    }

    hr = IDWriteFactory1_GetEudcFontCollection(factory1, &coll, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IDWriteFactory1_GetEudcFontCollection(factory1, &coll2, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(coll == coll2, "got %p, %p\n", coll, coll2);
    IDWriteFontCollection_Release(coll);
    IDWriteFontCollection_Release(coll2);

    IDWriteFactory1_Release(factory1);
}

static void test_GetCaretMetrics(void)
{
    DWRITE_FONT_METRICS1 metrics;
    IDWriteFontFace1 *fontface1;
    DWRITE_CARET_METRICS caret;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    IDWriteFont *font;
    WCHAR *path;
    HRESULT hr;

    path = create_testfontfile(test_fontfile);
    factory = create_factory();

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
    IDWriteFontFace_Release(fontface);
    if (hr != S_OK) {
        win_skip("GetCaretMetrics() is not supported.\n");
        IDWriteFactory_Release(factory);
        DELETE_FONTFILE(path);
        return;
    }

    memset(&caret, 0xcc, sizeof(caret));
    IDWriteFontFace1_GetCaretMetrics(fontface1, &caret);
    ok(caret.slopeRise == 1, "got %d\n", caret.slopeRise);
    ok(caret.slopeRun == 0, "got %d\n", caret.slopeRun);
    ok(caret.offset == 0, "got %d\n", caret.offset);
    IDWriteFontFace1_Release(fontface1);
    IDWriteFactory_Release(factory);

    /* now with Tahoma Normal */
    factory = create_factory();
    font = get_tahoma_instance(factory, DWRITE_FONT_STYLE_NORMAL);
    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFont_Release(font);
    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFace_Release(fontface);

    memset(&caret, 0xcc, sizeof(caret));
    IDWriteFontFace1_GetCaretMetrics(fontface1, &caret);
    ok(caret.slopeRise == 1, "got %d\n", caret.slopeRise);
    ok(caret.slopeRun == 0, "got %d\n", caret.slopeRun);
    ok(caret.offset == 0, "got %d\n", caret.offset);
    IDWriteFontFace1_Release(fontface1);

    /* simulated italic */
    font = get_tahoma_instance(factory, DWRITE_FONT_STYLE_ITALIC);
    hr = IDWriteFont_CreateFontFace(font, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFont_Release(font);
    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFace_Release(fontface);

    IDWriteFontFace1_GetMetrics(fontface1, &metrics);

    memset(&caret, 0xcc, sizeof(caret));
    IDWriteFontFace1_GetCaretMetrics(fontface1, &caret);
    ok(caret.slopeRise == metrics.designUnitsPerEm, "got %d\n", caret.slopeRise);
    ok(caret.slopeRun > 0, "got %d\n", caret.slopeRun);
    ok(caret.offset == 0, "got %d\n", caret.offset);
    IDWriteFontFace1_Release(fontface1);

    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_GetGlyphCount(void)
{
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    UINT16 count;
    WCHAR *path;
    HRESULT hr;

    path = create_testfontfile(test_fontfile);
    factory = create_factory();

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    count = IDWriteFontFace_GetGlyphCount(fontface);
    ok(count == 7, "got %u\n", count);

    IDWriteFontFace_Release(fontface);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_GetKerningPairAdjustments(void)
{
    IDWriteFontFace1 *fontface1;
    IDWriteFontFace *fontface;
    IDWriteFactory *factory;
    IDWriteFontFile *file;
    WCHAR *path;
    HRESULT hr;

    path = create_testfontfile(test_fontfile);
    factory = create_factory();

    hr = IDWriteFactory_CreateFontFileReference(factory, path, NULL, &file);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &file, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontface);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IDWriteFontFile_Release(file);

    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
    if (hr == S_OK) {
        INT32 adjustments[1];

        hr = IDWriteFontFace1_GetKerningPairAdjustments(fontface1, 0, NULL, NULL);
        ok(hr == E_INVALIDARG || broken(hr == S_OK) /* win8 */, "got 0x%08x\n", hr);

    if (0) /* crashes on native */
        hr = IDWriteFontFace1_GetKerningPairAdjustments(fontface1, 1, NULL, NULL);

        adjustments[0] = 1;
        hr = IDWriteFontFace1_GetKerningPairAdjustments(fontface1, 1, NULL, adjustments);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
        ok(adjustments[0] == 0, "got %d\n", adjustments[0]);

        IDWriteFontFace1_Release(fontface1);
    }
    else
        win_skip("GetKerningPairAdjustments() is not supported.\n");

    IDWriteFontFace_Release(fontface);
    IDWriteFactory_Release(factory);
    DELETE_FONTFILE(path);
}

static void test_CreateRenderingParams(void)
{
    IDWriteRenderingParams2 *params2;
    IDWriteRenderingParams1 *params1;
    IDWriteRenderingParams *params;
    IDWriteFactory *factory;
    HRESULT hr;

    factory = create_factory();

    hr = IDWriteFactory_CreateCustomRenderingParams(factory, 1.0, 0.0, 0.0, DWRITE_PIXEL_GEOMETRY_FLAT,
        DWRITE_RENDERING_MODE_DEFAULT, &params);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDWriteRenderingParams_QueryInterface(params, &IID_IDWriteRenderingParams1, (void**)&params1);
    if (hr == S_OK) {
        FLOAT enhcontrast;

        /* test what enhanced contrast setting set by default to */
        enhcontrast = IDWriteRenderingParams1_GetGrayscaleEnhancedContrast(params1);
        ok(enhcontrast == 1.0, "got %.2f\n", enhcontrast);
        IDWriteRenderingParams1_Release(params1);

        hr = IDWriteRenderingParams_QueryInterface(params, &IID_IDWriteRenderingParams2, (void**)&params2);
        if (hr == S_OK) {
            DWRITE_GRID_FIT_MODE gridfit;

            /* default gridfit mode */
            gridfit = IDWriteRenderingParams2_GetGridFitMode(params2);
            ok(gridfit == DWRITE_GRID_FIT_MODE_DEFAULT, "got %d\n", gridfit);

            IDWriteRenderingParams2_Release(params2);
        }
        else
            win_skip("IDWriteRenderingParams2 not supported.\n");
    }
    else
        win_skip("IDWriteRenderingParams1 not supported.\n");

    IDWriteRenderingParams_Release(params);
    IDWriteFactory_Release(factory);
}

START_TEST(font)
{
    IDWriteFactory *factory;

    if (!(factory = create_factory())) {
        win_skip("failed to create factory\n");
        return;
    }

    test_CreateFontFromLOGFONT();
    test_CreateBitmapRenderTarget();
    test_GetFontFamily();
    test_GetFamilyNames();
    test_CreateFontFace();
    test_GetMetrics();
    test_system_fontcollection();
    test_ConvertFontFaceToLOGFONT();
    test_CustomFontCollection();
    test_CreateCustomFontFileReference();
    test_CreateFontFileReference();
    test_shared_isolated();
    test_GetUnicodeRanges();
    test_GetFontFromFontFace();
    test_GetFirstMatchingFont();
    test_GetInformationalStrings();
    test_GetGdiInterop();
    test_CreateFontFaceFromHdc();
    test_GetSimulations();
    test_GetFaceNames();
    test_TryGetFontTable();
    test_ConvertFontToLOGFONT();
    test_CreateStreamFromKey();
    test_ReadFileFragment();
    test_GetDesignGlyphMetrics();
    test_GetDesignGlyphAdvances();
    test_IsMonospacedFont();
    test_GetGlyphRunOutline();
    test_GetEudcFontCollection();
    test_GetCaretMetrics();
    test_GetGlyphCount();
    test_GetKerningPairAdjustments();
    test_CreateRenderingParams();

    IDWriteFactory_Release(factory);
}
