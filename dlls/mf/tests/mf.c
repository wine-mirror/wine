/*
 * Unit tests for mf.dll.
 *
 * Copyright 2017 Nikolay Sivov
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

#include "mfidl.h"

#include "wine/test.h"

static void test_topology(void)
{
    IMFTopology *topology;
    HRESULT hr;

    hr = MFCreateTopology(NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    hr = MFCreateTopology(&topology);
    ok(hr == S_OK, "got %#x\n", hr);
    IMFTopology_Release(topology);
}

START_TEST(mf)
{
    test_topology();
}
