/*
 * Some unit tests for d3dxof
 *
 * Copyright (C) 2008 Christian Costa
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

#include <assert.h>
#include "wine/test.h"
#include "dxfile.h"

char template[] =
"xof 0302txt 0064\n"
"template Header\n"
"{\n"
"<3D82AB43-62DA-11CF-AB390020AF71E433>\n"
"WORD major ;\n"
"WORD minor ;\n"
"DWORD flags ;\n"
"}\n";

static unsigned long getRefcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void test_d3dxof(void)
{
    HRESULT hr;
    unsigned long ref;
    LPDIRECTXFILE lpDirectXFile = NULL;
    hr = DirectXFileCreate(&lpDirectXFile);
    ok(hr == DXFILE_OK, "DirectXFileCreate: %x\n", hr);
    if(!lpDirectXFile)
    {
        trace("Couldn't create DirectXFile interface, skipping tests\n");
        return;
    }

    ref = getRefcount( (IUnknown *) lpDirectXFile);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    ref = IDirectXFile_AddRef(lpDirectXFile);
    ok(ref == 2, "Got refcount %ld, expected 1\n", ref);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    /* RegisterTemplates does not support txt format yet */
    hr = IDirectXFile_RegisterTemplates(lpDirectXFile, template, strlen(template));
    ok(hr == DXFILE_OK, "IDirectXFileImpl_RegisterTemplates: %x\n", hr);

    ref = IDirectXFile_Release(lpDirectXFile);
    ok(ref == 0, "Got refcount %ld, expected 1\n", ref);
}

START_TEST(d3dxof)
{
    test_d3dxof();
}
