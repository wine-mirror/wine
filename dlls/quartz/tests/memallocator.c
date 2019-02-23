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

static void test_properties(void)
{
    ALLOCATOR_PROPERTIES req_props = {0}, ret_props;
    IMemAllocator *allocator = create_allocator();
    IMediaSample *sample;
    LONG size, ret_size;
    unsigned int i;
    HRESULT hr;

    static const ALLOCATOR_PROPERTIES tests[] =
    {
        {0, 0, 1, 0},
        {1, 0, 1, 0},
        {1, 1, 1, 0},
        {1, 1, 4, 0},
        {2, 1, 1, 0},
        {1, 2, 4, 0},
        {1, 1, 16, 0},
        {1, 16, 16, 0},
        {1, 17, 16, 0},
        {1, 32, 16, 0},
        {1, 1, 16, 1},
        {1, 15, 16, 1},
        {1, 16, 16, 1},
        {1, 17, 16, 1},
        {1, 0, 16, 16},
        {1, 1, 16, 16},
        {1, 16, 16, 16},
    };

    memset(&ret_props, 0xcc, sizeof(ret_props));
    hr = IMemAllocator_GetProperties(allocator, &ret_props);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!ret_props.cBuffers, "Got %d buffers.\n", ret_props.cBuffers);
    ok(!ret_props.cbBuffer, "Got size %d.\n", ret_props.cbBuffer);
    ok(!ret_props.cbAlign, "Got align %d.\n", ret_props.cbAlign);
    ok(!ret_props.cbPrefix, "Got prefix %d.\n", ret_props.cbPrefix);

    hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
    ok(hr == VFW_E_BADALIGN, "Got hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        req_props = tests[i];
        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        ok(hr == S_OK, "Test %u: Got hr %#x.\n", i, hr);
        ok(!memcmp(&req_props, &tests[i], sizeof(req_props)), "Test %u: Requested props should not be changed.\n", i);
        ok(ret_props.cBuffers == req_props.cBuffers, "Test %u: Got %d buffers.\n", i, ret_props.cBuffers);
        ok(ret_props.cbBuffer >= req_props.cbBuffer, "Test %u: Got size %d.\n", i, ret_props.cbBuffer);
        ok(ret_props.cbAlign == req_props.cbAlign, "Test %u: Got alignment %d.\n", i, ret_props.cbAlign);
        ok(ret_props.cbPrefix == req_props.cbPrefix, "Test %u: Got prefix %d.\n", i, ret_props.cbPrefix);
        ret_size = ret_props.cbBuffer;

        hr = IMemAllocator_GetProperties(allocator, &ret_props);
        ok(hr == S_OK, "Test %u: Got hr %#x.\n", i, hr);
        ok(ret_props.cBuffers == req_props.cBuffers, "Test %u: Got %d buffers.\n", i, ret_props.cBuffers);
        ok(ret_props.cbBuffer == ret_size, "Test %u: Got size %d.\n", i, ret_props.cbBuffer);
        ok(ret_props.cbAlign == req_props.cbAlign, "Test %u: Got alignment %d.\n", i, ret_props.cbAlign);
        ok(ret_props.cbPrefix == req_props.cbPrefix, "Test %u: Got prefix %d.\n", i, ret_props.cbPrefix);

        hr = IMemAllocator_Commit(allocator);
        if (!req_props.cbBuffer)
        {
            ok(hr == VFW_E_SIZENOTSET, "Test %u: Got hr %#x.\n", i, hr);
            continue;
        }
        ok(hr == S_OK, "Test %u: Got hr %#x.\n", i, hr);

        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        ok(hr == VFW_E_ALREADY_COMMITTED, "Test %u: Got hr %#x.\n", i, hr);

        hr = IMemAllocator_GetBuffer(allocator, &sample, NULL, NULL, 0);
        ok(hr == S_OK, "Test %u: Got hr %#x.\n", i, hr);

        size = IMediaSample_GetSize(sample);
        ok(size == ret_size, "Test %u: Got size %d.\n", i, size);

        hr = IMemAllocator_Decommit(allocator);
        ok(hr == S_OK, "Test %u: Got hr %#x.\n", i, hr);

        hr = IMemAllocator_SetProperties(allocator, &req_props, &ret_props);
        ok(hr == VFW_E_BUFFERS_OUTSTANDING, "Test %u: Got hr %#x.\n", i, hr);

        hr = IMediaSample_Release(sample);
        ok(hr == S_OK, "Test %u: Got hr %#x.\n", i, hr);
    }

    IMemAllocator_Release(allocator);
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

    test_properties();
    test_commit();

    CoUninitialize();
}
