/*
 * Copyright (C) 2010 David Adam
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

#include "initguid.h"
#include "dmusici.h"
#include "wine/test.h"

static void test_release_object(void)
{
    HRESULT hr;
    IDirectMusicLoader8* loader = NULL;

    hr = CoInitialize(NULL);
    if ( FAILED(hr) )
    {
        skip("CoInitialize failed.\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, &IID_IDirectMusicLoader8, (void**)&loader);
    if ( FAILED(hr) )
    {
        skip("CoCreateInstance failed.\n");
        CoUninitialize();
        return;
    }

    hr = IDirectMusicLoader_ReleaseObject(loader, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, received %#x\n", hr);

    IDirectMusicLoader_Release(loader);
    CoUninitialize();
}

START_TEST(loader)
{
    test_release_object();
}
