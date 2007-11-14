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

    hr = CoCreateInstance(&CLSID_IMimeBody, NULL, CLSCTX_INPROC_SERVER, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeBody_GetHandle(body, &handle);
    ok(hr == MIME_E_NO_DATA, "ret %08x\n", hr);
    ok(handle == NULL, "handle %p\n", handle);

    IMimeBody_Release(body);
}

START_TEST(mimeole)
{
    OleInitialize(NULL);
    test_CreateVirtualStream();
    test_CreateSecurity();
    test_CreateBody();
    OleUninitialize();
}
