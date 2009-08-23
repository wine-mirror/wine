/*
 * Copyright 2009 Tony Wasserka
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

#include "wine/test.h"

#define COBJMACROS
#include "wincodec.h"

static void test_StreamOnMemory(void)
{
    IWICImagingFactory *pFactory;
    IWICStream *pStream;
    const BYTE CmpMem[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    };
    BYTE Memory[64], MemBuf[64];
    LARGE_INTEGER LargeNull, LargeInt;
    ULARGE_INTEGER uNewPos;
    ULONG uBytesRead, uBytesWritten;
    HRESULT hr;

    LargeNull.QuadPart = 0;

    memcpy(Memory, CmpMem, sizeof(CmpMem));

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&pFactory);
    if(FAILED(hr)) {
        skip("CoCreateInstance returned with %#x, expected %#x\n", hr, S_OK);
        return;
    }

    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream returned with %#x, expected %#x\n", hr, S_OK);
    if(FAILED(hr)) {
        skip("Failed to create stream\n");
        return;
    }

    /* InitializeFromMemory */
    hr = IWICStream_InitializeFromMemory(pStream, NULL, sizeof(Memory));   /* memory = NULL */
    ok(hr == E_INVALIDARG, "InitializeFromMemory returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, 0);   /* size = 0 */
    ok(hr == S_OK, "InitializeFromMemory returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));   /* stream already initialized */
    ok(hr == WINCODEC_ERR_WRONGSTATE, "InitializeFromMemory returned with %#x, expected %#x\n", hr, WINCODEC_ERR_WRONGSTATE);

    /* recreate stream */
    IWICStream_Release(pStream);
    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream failed with %#x\n", hr);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));
    ok(hr == S_OK, "InitializeFromMemory returned with %#x, expected %#x\n", hr, S_OK);


    /* Seek */
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, 0);

    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);

    LargeInt.HighPart = 1;
    LargeInt.LowPart = 0;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "Seek returned with %#x, expected %#x\n", hr, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, 0);

    LargeInt.QuadPart = sizeof(Memory) + 10;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == E_INVALIDARG, "Seek returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, 0);

    LargeInt.QuadPart = 1;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == E_INVALIDARG, "Seek returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, 0);

    LargeInt.QuadPart = -1;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == sizeof(Memory) - 1, "bSeek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, sizeof(Memory) - 1);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, &uNewPos); /* reset seek pointer */
    LargeInt.QuadPart = -sizeof(Memory) - 5;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == E_INVALIDARG, "Seek returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == 0, "bSeek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, 0); /* remains unchanged */
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Read */
    hr = IWICStream_Read(pStream, MemBuf, 12, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 12, "Read %u bytes, expected %u\n", uBytesRead, 3);
        ok(memcmp(MemBuf, CmpMem, 12) == 0, "Read returned invalid data!\n");

        /* check whether the seek pointer has moved correctly */
        IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
        ok(uNewPos.HighPart == 0 && uNewPos.LowPart == uBytesRead, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, uBytesRead);
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, 10, &uBytesRead);   /* source = dest */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 10, "Read %u bytes, expected %u\n", uBytesRead, 10);
        ok(memcmp(Memory, CmpMem, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, sizeof(Memory) + 10, &uBytesRead);   /* request too many bytes */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(Memory), "Read %u bytes, expected %u\n", uBytesRead, sizeof(Memory));
        ok(memcmp(Memory, CmpMem, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    hr = IWICStream_Read(pStream, NULL, 1, &uBytesRead);    /* destination buffer = NULL */
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, MemBuf, 0, &uBytesRead);    /* read 0 bytes */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_Read(pStream, NULL, 0, &uBytesRead);
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, MemBuf, 1, NULL);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);
    ZeroMemory(MemBuf, sizeof(MemBuf));
    hr = IWICStream_Read(pStream, MemBuf, sizeof(Memory) + 10, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(Memory), "Read %u bytes, expected %u\n", uBytesRead, sizeof(Memory));
        ok(memcmp(Memory, CmpMem, 64) == 0, "Read returned invalid data!\n");
    }
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Write */
    MemBuf[0] = CmpMem[0] + 1;
    MemBuf[1] = CmpMem[1] + 1;
    MemBuf[2] = CmpMem[2] + 1;
    hr = IWICStream_Write(pStream, MemBuf, 3, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesWritten == 3, "Wrote %u bytes, expected %u\n", uBytesWritten, 3);
        ok(memcmp(MemBuf, Memory, 3) == 0, "Wrote returned invalid data!\n"); /* make sure we're writing directly */

        /* check whether the seek pointer has moved correctly */
        IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
        ok(uNewPos.HighPart == 0 && uNewPos.LowPart == uBytesWritten, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, uBytesWritten);
    }
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(pStream, MemBuf, 0, &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_Write(pStream, NULL, 3, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0, "Wrote %u bytes, expected %u\n", uBytesWritten, 0);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, 0);

    hr = IWICStream_Write(pStream, NULL, 0, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0, "Wrote %u bytes, expected %u\n", uBytesWritten, 0);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, 0);

    hr = IWICStream_Write(pStream, CmpMem, sizeof(Memory) + 10, &uBytesWritten);
    ok(hr == STG_E_MEDIUMFULL, "Write returned with %#x, expected %#x\n", hr, STG_E_MEDIUMFULL);
    ok(uBytesWritten == 0, "Wrote %u bytes, expected %u\n", uBytesWritten, 0);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.HighPart == 0 && uNewPos.LowPart == uBytesWritten, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.HighPart, uNewPos.LowPart, 0, uBytesWritten);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    IWICStream_Release(pStream);
    IWICStream_Release(pFactory);
    CoUninitialize();
}

START_TEST(stream)
{
    test_StreamOnMemory();
}
