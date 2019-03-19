/*
 * Copyright 2018 Alistair Leslie-Hughes
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#undef INITGUID
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"

#include "wine/test.h"

static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return;
    X(MFCreateMFByteStreamOnStream);
#undef X
}

static void test_MFCreateSourceReaderFromByteStream(void)
{
    static const WCHAR audio[] = {'A','u','d','i','o',0};
    IMFSourceReader *source = NULL;
    IMFAttributes *attributes;
    IMFByteStream *bytestream = NULL;
    IStream *stream = NULL;
    HRESULT hr;

    if(!pMFCreateMFByteStreamOnStream)
    {
        win_skip("MFCreateMFByteStreamOnStream() not found\n");
        return;
    }

    hr = MFCreateAttributes( &attributes, 3 );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr =  IMFAttributes_SetString(attributes, &MF_READWRITE_MMCSS_CLASS_AUDIO, audio);
    ok(hr == S_OK, "Failed to set string value, hr %#x.\n", hr);

    hr = IMFAttributes_SetUINT32(attributes, &MF_READWRITE_MMCSS_PRIORITY_AUDIO, 0);
    ok(hr == S_OK, "Failed to set attribute, hr %#x.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = MFCreateSourceReaderFromByteStream(bytestream, attributes, &source);
todo_wine
    ok(hr == S_OK || hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "got 0x%08x\n", hr);

    if(stream)
        IStream_Release(stream);
    if(bytestream)
        IMFByteStream_Release(bytestream);
    IMFAttributes_Release(attributes);
    if(source)
        IMFSourceReader_Release(source);
}

static void test_factory(void)
{
    IMFReadWriteClassFactory *factory, *factory2;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MFReadWriteClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFReadWriteClassFactory,
            (void **)&factory);
    ok(hr == S_OK, "Failed to create class factory, hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_MFReadWriteClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER, &IID_IMFReadWriteClassFactory,
            (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION, "Unexpected hr %#x.\n", hr);

    IMFReadWriteClassFactory_Release(factory);

    CoUninitialize();
}

START_TEST(mfplat)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    init_functions();

    test_MFCreateSourceReaderFromByteStream();
    test_factory();

    MFShutdown();
}
