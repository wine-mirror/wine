/*
 * MimeOle tests
 *
 * Copyright 2007 Huw Davies
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
#include "ole2.h"
#include "ocidl.h"

#include "initguid.h"
#include "mimeole.h"

#include <stdio.h>
#include <assert.h>

#include "wine/test.h"

static char msg1[] =
    "MIME-Version: 1.0\r\n"
    "Content-Type: multipart/mixed;\r\n"
    " boundary=\"------------1.5.0.6\";\r\n"
    " stuff=\"du;nno\"\r\n"
    "foo: bar\r\n"
    "From: Huw Davies <huw@codeweavers.com>\r\n"
    "From: Me <xxx@codeweavers.com>\r\n"
    "To: wine-patches <wine-patches@winehq.org>\r\n"
    "Cc: Huw Davies <huw@codeweavers.com>,\r\n"
    "    \"Fred Bloggs\"   <fred@bloggs.com>\r\n"
    "foo: baz\r\n"
    "bar: fum\r\n"
    "\r\n"
    "This is a multi-part message in MIME format.\r\n"
    "--------------1.5.0.6\r\n"
    "Content-Type: text/plain; format=fixed; charset=UTF-8\r\n"
    "Content-Transfer-Encoding: 8bit\r\n"
    "\r\n"
    "Stuff\r\n"
    "--------------1.5.0.6\r\n"
    "Content-Type: text/plain; charset=\"us-ascii\"\r\n"
    "Content-Transfer-Encoding: 7bit\r\n"
    "\r\n"
    "More stuff\r\n"
    "--------------1.5.0.6--\r\n";

static void test_CreateVirtualStream(void)
{
    HRESULT hr;
    IStream *pstm;

    hr = MimeOleCreateVirtualStream(&pstm);
    ok(hr == S_OK, "ret %08x\n", hr);

    IStream_Release(pstm);
}

static void test_CreateSecurity(void)
{
    HRESULT hr;
    IMimeSecurity *sec;

    hr = MimeOleCreateSecurity(&sec);
    ok(hr == S_OK, "ret %08x\n", hr);

    IMimeSecurity_Release(sec);
}

static void test_CreateBody(void)
{
    HRESULT hr;
    IMimeBody *body;
    HBODY handle = (void *)0xdeadbeef;
    IStream *in;
    LARGE_INTEGER off;
    ULARGE_INTEGER pos;
    ENCODINGTYPE enc;

    hr = CoCreateInstance(&CLSID_IMimeBody, NULL, CLSCTX_INPROC_SERVER, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeBody_GetHandle(body, &handle);
    ok(hr == MIME_E_NO_DATA, "ret %08x\n", hr);
    ok(handle == NULL, "handle %p\n", handle);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &in);
    ok(hr == S_OK, "ret %08x\n", hr);
    IStream_Write(in, msg1, sizeof(msg1) - 1, NULL);
    off.QuadPart = 0;
    IStream_Seek(in, off, STREAM_SEEK_SET, NULL);

    /* Need to call InitNew before Load otherwise Load crashes with native inetcomm */
    hr = IMimeBody_InitNew(body);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeBody_GetCurrentEncoding(body, &enc);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(enc == IET_7BIT, "encoding %d\n", enc);

    hr = IMimeBody_Load(body, in);
    ok(hr == S_OK, "ret %08x\n", hr);
    off.QuadPart = 0;
    IStream_Seek(in, off, STREAM_SEEK_CUR, &pos);
    ok(pos.u.LowPart == 328, "pos %u\n", pos.u.LowPart);

    hr = IMimeBody_IsContentType(body, "multipart", "mixed");
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_IsContentType(body, "text", "plain");
    ok(hr == S_FALSE, "ret %08x\n", hr);
    hr = IMimeBody_IsContentType(body, NULL, "mixed");
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeBody_SetData(body, IET_8BIT, "text", "plain", &IID_IStream, in);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_IsContentType(body, "text", "plain");
    todo_wine
        ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_GetCurrentEncoding(body, &enc);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(enc == IET_8BIT, "encoding %d\n", enc);

    IMimeBody_Release(body);
}

static void test_Allocator(void)
{
    HRESULT hr;
    IMimeAllocator *alloc;

    hr = MimeOleGetAllocator(&alloc);
    ok(hr == S_OK, "ret %08x\n", hr);
    IMimeAllocator_Release(alloc);
}

START_TEST(mimeole)
{
    OleInitialize(NULL);
    test_CreateVirtualStream();
    test_CreateSecurity();
    test_CreateBody();
    test_Allocator();
    OleUninitialize();
}
