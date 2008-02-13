/*
 * Unit tests for Media Detector
 *
 * Copyright (C) 2008 Google (Lei Zhang)
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

#include <assert.h>

#define COBJMACROS

#include "wine/test.h"
#include "qedit.h"

static void test_mediadet(void)
{
    HRESULT hr;
    IMediaDet *pM = NULL;

    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (LPVOID*)&pM);
    todo_wine {
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pM != NULL, "pM is NULL\n");
    }
}

START_TEST(mediadet)
{
    CoInitialize(NULL);
    test_mediadet();
}
