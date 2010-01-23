/*
 * XMLLite IXmlReader tests
 *
 * Copyright 2010 (C) Nikolay Sivov
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "ole2.h"
#include "xmllite.h"
#include "wine/test.h"

DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);

HRESULT WINAPI (*pCreateXmlReader)(REFIID riid, void **ppvObject, IMalloc *pMalloc);
HRESULT WINAPI (*pCreateXmlReaderInputWithEncodingName)(IUnknown *stream,
                                                        IMalloc *pMalloc,
                                                        LPCWSTR encoding,
                                                        BOOL hint,
                                                        LPCWSTR base_uri,
                                                        IXmlReaderInput **ppInput);
static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

typedef struct input_iids_t {
    IID iids[10];
    int count;
} input_iids_t;

static const IID *setinput_full[] = {
    &IID_IXmlReaderInput,
    &IID_IStream,
    &IID_ISequentialStream,
    NULL
};

/* this applies to early xmllite versions */
static const IID *setinput_full_old[] = {
    &IID_IXmlReaderInput,
    &IID_ISequentialStream,
    &IID_IStream,
    NULL
};

/* after ::SetInput(IXmlReaderInput*) */
static const IID *setinput_readerinput[] = {
    &IID_IStream,
    &IID_ISequentialStream,
    NULL
};

static const IID *empty_seq[] = {
    NULL
};

static input_iids_t input_iids;

static void ok_iids_(const input_iids_t *iids, const IID **expected, const IID **exp_broken, int todo, int line)
{
    int i = 0, size = 0;

    while (expected[i++]) size++;

    if (todo) {
        todo_wine
            ok_(__FILE__, line)(iids->count == size, "Sequence size mismatch (%d), got (%d)\n", size, iids->count);
    }
    else
       ok_(__FILE__, line)(iids->count == size, "Sequence size mismatch (%d), got (%d)\n", size, iids->count);

    if (iids->count != size) return;

    for (i = 0; i < size; i++) {
        ok_(__FILE__, line)(IsEqualGUID(&iids->iids[i], expected[i]) ||
            (exp_broken ? broken(IsEqualGUID(&iids->iids[i], exp_broken[i])) : FALSE),
            "Wrong IID(%d), got (%s)\n", i, debugstr_guid(&iids->iids[i]));
    }
}
#define ok_iids(got, exp, brk, todo) ok_iids_(got, exp, brk, todo, __LINE__)

typedef struct _testinput
{
    const IUnknownVtbl *lpVtbl;
    LONG ref;
} testinput;

static inline testinput *impl_from_IUnknown(IUnknown *iface)
{
    return (testinput *)((char*)iface - FIELD_OFFSET(testinput, lpVtbl));
}

static HRESULT WINAPI testinput_QueryInterface(IUnknown *iface, REFIID riid, void** ppvObj)
{
    if (IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    input_iids.iids[input_iids.count++] = *riid;

    *ppvObj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI testinput_AddRef(IUnknown *iface)
{
    testinput *This = impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI testinput_Release(IUnknown *iface)
{
    testinput *This = impl_from_IUnknown(iface);
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static const struct IUnknownVtbl testinput_vtbl =
{
    testinput_QueryInterface,
    testinput_AddRef,
    testinput_Release
};

static HRESULT testinput_createinstance(void **ppObj)
{
    testinput *input;

    input = HeapAlloc(GetProcessHeap(), 0, sizeof (*input));
    if(!input) return E_OUTOFMEMORY;

    input->lpVtbl = &testinput_vtbl;
    input->ref = 1;

    *ppObj = &input->lpVtbl;

    return S_OK;
}

static BOOL init_pointers(void)
{
    /* don't free module here, it's to be unloaded on exit */
    HMODULE mod = LoadLibraryA("xmllite.dll");

    if (!mod)
    {
        win_skip("xmllite library not available\n");
        return FALSE;
    }

#define MAKEFUNC(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return FALSE;
    MAKEFUNC(CreateXmlReader);
    MAKEFUNC(CreateXmlReaderInputWithEncodingName);
#undef MAKEFUNC

    return TRUE;
}

static void test_reader_create(void)
{
    HRESULT hr;
    IXmlReader *reader;
    IUnknown *input;

    /* crashes native */
    if (0)
    {
        hr = pCreateXmlReader(&IID_IXmlReader, NULL, NULL);
        hr = pCreateXmlReader(NULL, (LPVOID*)&reader, NULL);
    }

    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* Null input pointer, releases previous input */
    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* test input interface selection sequence */
    hr = testinput_createinstance((void**)&input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    input_iids.count = 0;
    hr = IXmlReader_SetInput(reader, input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_full, setinput_full_old, FALSE);

    IUnknown_Release(input);

    IXmlReader_Release(reader);
}

static void test_readerinput(void)
{
    IXmlReaderInput *reader_input;
    IXmlReader *reader, *reader2;
    IUnknown *obj, *input;
    IStream *stream;
    HRESULT hr;
    LONG ref;

    hr = pCreateXmlReaderInputWithEncodingName(NULL, NULL, NULL, FALSE, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    hr = pCreateXmlReaderInputWithEncodingName(NULL, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ref = IStream_AddRef(stream);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IStream_Release(stream);
    hr = pCreateXmlReaderInputWithEncodingName((IUnknown*)stream, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* IXmlReaderInput grabs a stream reference */
    ref = IStream_AddRef(stream);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IStream_Release(stream);

    /* try ::SetInput() with valid IXmlReaderInput */
    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);

    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    /* IXmlReader grabs a IXmlReaderInput reference */
    ref = IUnknown_AddRef(reader_input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(reader_input);

    ref = IStream_AddRef(stream);
    ok(ref == 4, "Expected 4, got %d\n", ref);
    IStream_Release(stream);

    IXmlReader_Release(reader);

    ref = IStream_AddRef(stream);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IStream_Release(stream);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);

    /* IID_IXmlReaderInput */
    /* it returns a kind of private undocumented vtable incompatible with IUnknown,
       so it's not a COM interface actually.
       Such query will be used only to check if input is really IXmlReaderInput */
    obj = (IUnknown*)0xdeadbeef;
    hr = IUnknown_QueryInterface(reader_input, &IID_IXmlReaderInput, (void**)&obj);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ref = IUnknown_AddRef(reader_input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(reader_input);

    IUnknown_Release(reader_input);
    IStream_Release(stream);

    /* test input interface selection sequence */
    hr = testinput_createinstance((void**)&input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    input_iids.count = 0;
    ref = IUnknown_AddRef(input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(input);
    hr = pCreateXmlReaderInputWithEncodingName(input, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok_iids(&input_iids, empty_seq, NULL, FALSE);
    /* IXmlReaderInput stores stream interface as IUnknown */
    ref = IUnknown_AddRef(input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(input);

    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    input_iids.count = 0;
    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);
    ref = IUnknown_AddRef(input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(input);
    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_readerinput, NULL, FALSE);

    ref = IUnknown_AddRef(input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(input);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);
    /* repeat another time, no check or caching here */
    input_iids.count = 0;
    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_readerinput, NULL, FALSE);

    /* another reader */
    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader2, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* resolving from IXmlReaderInput to IStream/ISequentialStream is done at
       ::SetInput() level, each time it's called */
    input_iids.count = 0;
    hr = IXmlReader_SetInput(reader2, reader_input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_readerinput, NULL, FALSE);

    IXmlReader_Release(reader2);
    IXmlReader_Release(reader);

    IUnknown_Release(reader_input);
    IUnknown_Release(input);
}

START_TEST(reader)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    if (!init_pointers())
    {
       CoUninitialize();
       return;
    }

    test_reader_create();
    test_readerinput();

    CoUninitialize();
}
