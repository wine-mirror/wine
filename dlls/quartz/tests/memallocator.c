/*
 * Unit tests for Direct Show functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>

#define COBJMACROS

#include "wine/test.h"
#include "uuids.h"
#include "dshow.h"
#include "control.h"

static void CommitDecommitTest(void)
{
    IMemAllocator* pMemAllocator;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (LPVOID*)&pMemAllocator);
    ok(hr==S_OK, "Unable to create memory allocator %lx\n", hr);

    if (hr == S_OK)
    {
	ALLOCATOR_PROPERTIES RequestedProps;
	ALLOCATOR_PROPERTIES ActualProps;

	RequestedProps.cBuffers = 1;
	RequestedProps.cbBuffer = 65536;
	RequestedProps.cbAlign = 1;
	RequestedProps.cbPrefix = 0;

	hr = IMemAllocator_SetProperties(pMemAllocator, &RequestedProps, &ActualProps);
	ok(hr==S_OK, "SetProperties returned: %lx\n", hr);

	hr = IMemAllocator_Commit(pMemAllocator);
	ok(hr==S_OK, "Commit returned: %lx\n", hr);
	hr = IMemAllocator_Commit(pMemAllocator);
	ok(hr==S_OK, "Commit returned: %lx\n", hr);

	hr = IMemAllocator_Decommit(pMemAllocator);
	ok(hr==S_OK, "Decommit returned: %lx\n", hr);
	hr = IMemAllocator_Decommit(pMemAllocator);
	ok(hr==S_OK, "Cecommit returned: %lx\n", hr);

	IMemAllocator_Release(pMemAllocator);
    }
}

START_TEST(memallocator)
{
    CoInitialize(NULL);

    CommitDecommitTest();
}
