/*
 * Unit tests for IDxDiagContainer
 *
 * Copyright 2010 Andrew Nguyen
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

#include "dxdiag.h"
#include "wine/test.h"

static IDxDiagProvider *pddp;
static IDxDiagContainer *pddc;

static BOOL create_root_IDxDiagContainer(void)
{
    HRESULT hr;
    DXDIAG_INIT_PARAMS params;

    hr = CoCreateInstance(&CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDxDiagProvider, (LPVOID*)&pddp);
    if (SUCCEEDED(hr))
    {
        params.dwSize = sizeof(params);
        params.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
        params.bAllowWHQLChecks = FALSE;
        params.pReserved = NULL;
        hr = IDxDiagProvider_Initialize(pddp, &params);
        if (SUCCEEDED(hr))
        {
            hr = IDxDiagProvider_GetRootContainer(pddp, &pddc);
            if (SUCCEEDED(hr))
                return TRUE;
        }
        IDxDiagProvider_Release(pddp);
    }
    return FALSE;
}

static void test_GetNumberOfChildContainers(void)
{
    HRESULT hr;
    DWORD count;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, NULL);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetNumberOfChildContainers to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK,
       "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (hr == S_OK)
        ok(count != 0, "Expected the number of child containers for the root container to be non-zero\n");

    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_GetNumberOfProps(void)
{
    HRESULT hr;
    DWORD count;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetNumberOfProps(pddc, NULL);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetNumberOfProps to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = IDxDiagContainer_GetNumberOfProps(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);
    if (hr == S_OK)
        ok(count == 0, "Expected the number of properties for the root container to be zero\n");

    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

START_TEST(container)
{
    CoInitialize(NULL);
    test_GetNumberOfChildContainers();
    test_GetNumberOfProps();
    CoUninitialize();
}
