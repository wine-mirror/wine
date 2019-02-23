/*
 * Memory allocator unit tests
 *
 * Copyright (C) 2005 Christian Costa
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
#include "dshow.h"
#include "wine/test.h"

static IMemAllocator *create_allocator(void)
{
    IMemAllocator *allocator = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMemAllocator, (void **)&allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return allocator;
}

static void test_commit(void)
{
    ALLOCATOR_PROPERTIES req_props = {2, 65536, 1, 0}, ret_props;
    IMemAllocator *allocator = create_allocator();
    IMediaSample *sample, *sample2;
    HRESULT hr;
    BYTE *data;

    hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#x.\n", hr);

    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IMediaSample_Release(sample);

    hr = IMemAllocator_Decommit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IMemAllocator_Decommit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    /* Extant samples remain valid even after Decommit() is called. */
    hr = IMemAllocator_Commit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMediaSample_GetPointer(sample, &data);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IMemAllocator_Decommit(allocator);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    memset(data, 0xcc, 65536);

    hr = IMemAllocator_GetBuffer(allocator, &sample2, NULL, NULL, 0);
    ok(hr == VFW_E_NOT_COMMITTED, "Got hr %#x.\n", hr);

    IMediaSample_Release(sample);
    IMemAllocator_Release(allocator);
}

START_TEST(memallocator)
{
    CoInitialize(NULL);

    test_commit();

    CoUninitialize();
}
