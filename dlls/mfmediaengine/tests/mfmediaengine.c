/*
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"

#include "mfapi.h"
#include "mfmediaengine.h"

#include "wine/heap.h"
#include "wine/test.h"

static void test_factory(void)
{
    IMFMediaEngineClassFactory *factory, *factory2;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IMFMediaEngineClassFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /* pre-win8 */,
       "Failed to create class factory, hr %#x.\n", hr);

    hr = CoCreateInstance(&CLSID_MFMediaEngineClassFactory, (IUnknown *)factory, CLSCTX_INPROC_SERVER,
                          &IID_IMFMediaEngineClassFactory, (void **)&factory2);
    ok(hr == CLASS_E_NOAGGREGATION || broken(hr == REGDB_E_CLASSNOTREG) /* pre-win8 */,
       "Unexpected hr %#x.\n", hr);

    if (factory)
        IMFMediaEngineClassFactory_Release(factory);

    CoUninitialize();
}

START_TEST(mfmediaengine)
{
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "MFStartup failed: %#x.\n", hr);

    test_factory();

    MFShutdown();
}
