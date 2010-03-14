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

static void test_EnumChildContainerNames(void)
{
    HRESULT hr;
    WCHAR container[256];
    DWORD maxcount, index;
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const WCHAR zerotestW[] = {0,'e','s','t',0};

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Test various combinations of invalid parameters. */
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, NULL, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, NULL, sizeof(container)/sizeof(WCHAR));
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);

    /* Test the conditions in which the output buffer can be modified. */
    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, container, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(container, testW, sizeof(testW)),
       "Expected the container buffer to be untouched, got %s\n", wine_dbgstr_w(container));

    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, ~0, container, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(container, testW, sizeof(testW)),
       "Expected the container buffer to be untouched, got %s\n", wine_dbgstr_w(container));

    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, ~0, container, sizeof(container)/sizeof(WCHAR));
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(container, zerotestW, sizeof(zerotestW)),
       "Expected the container buffer string to be empty, got %s\n", wine_dbgstr_w(container));

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &maxcount);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    trace("Starting child container enumeration of the root container:\n");

    /* We should be able to enumerate as many child containers as the value
     * that IDxDiagContainer::GetNumberOfChildContainers returns. */
    for (index = 0; index <= maxcount; index++)
    {
        /* A buffer size of 1 is unlikely to be valid, as only a null terminator
         * could be stored, and it is unlikely that a container name could be empty. */
        DWORD buffersize = 1;
        memcpy(container, testW, sizeof(testW));
        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, buffersize);
        if (hr == E_INVALIDARG)
        {
            /* We should get here when index is one more than the maximum index value. */
            ok(maxcount == index,
               "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG "
               "on the last index %d, got 0x%08x\n", index, hr);
            ok(container[0] == '\0',
               "Expected the container buffer string to be empty, got %s\n", wine_dbgstr_w(container));
            break;
        }
        else if (hr == DXDIAG_E_INSUFFICIENT_BUFFER)
        {
            WCHAR temp[256];

            ok(container[0] == '\0',
               "Expected the container buffer string to be empty, got %s\n", wine_dbgstr_w(container));

            /* Get the container name to compare against. */
            hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, temp, sizeof(temp)/sizeof(WCHAR));
            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);

            /* Show that the DirectX SDK's stipulation that the buffer be at
             * least 256 characters long is a mere suggestion, and smaller sizes
             * can be acceptable also. IDxDiagContainer::EnumChildContainerNames
             * doesn't provide a way of getting the exact size required, so the
             * buffersize value will be iterated to at most 256 characters. */
            for (buffersize = 2; buffersize <= 256; buffersize++)
            {
                memcpy(container, testW, sizeof(testW));
                hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, buffersize);
                if (hr != DXDIAG_E_INSUFFICIENT_BUFFER)
                    break;

                ok(!memcmp(temp, container, sizeof(WCHAR)*(buffersize - 1)),
                   "Expected truncated container name string, got %s\n", wine_dbgstr_w(container));
            }

            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, "
               "got hr = 0x%08x, buffersize = %d\n", hr, buffersize);
            if (hr == S_OK)
                trace("pddc[%d] = %s, length = %d\n", index, wine_dbgstr_w(container), buffersize);
        }
        else
        {
            ok(0, "IDxDiagContainer::EnumChildContainerNames unexpectedly returned 0x%08x\n", hr);
            break;
        }
    }

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_GetChildContainer(void)
{
    HRESULT hr;
    WCHAR container[256] = {0};
    IDxDiagContainer *child;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Test various combinations of invalid parameters. */
    hr = IDxDiagContainer_GetChildContainer(pddc, NULL, NULL);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, NULL, &child);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(child == (void*)0xdeadbeef, "Expected output pointer to be unchanged, got %p\n", child);

    hr = IDxDiagContainer_GetChildContainer(pddc, container, NULL);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(child == NULL, "Expected output pointer to be NULL, got %p\n", child);

    /* Get the name of a suitable child container. */
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, container, sizeof(container)/sizeof(WCHAR));
    ok(hr == S_OK,
       "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::EnumChildContainerNames failed\n");
        goto cleanup;
    }

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
    ok(hr == S_OK,
       "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);
    ok(child != NULL && child != (void*)0xdeadbeef, "Expected a valid output pointer, got %p\n", child);

    if (SUCCEEDED(hr))
    {
        IDxDiagContainer *ptr;

        /* Show that IDxDiagContainer::GetChildContainer returns a different pointer
         * for multiple calls for the same container name. */
        hr = IDxDiagContainer_GetChildContainer(pddc, container, &ptr);
        ok(hr == S_OK,
           "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);
        if (SUCCEEDED(hr))
            todo_wine ok(ptr != child, "Expected the two pointers (%p vs. %p) to be unequal", child, ptr);

        IDxDiagContainer_Release(ptr);
        IDxDiagContainer_Release(child);
    }

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

START_TEST(container)
{
    CoInitialize(NULL);
    test_GetNumberOfChildContainers();
    test_GetNumberOfProps();
    test_EnumChildContainerNames();
    test_GetChildContainer();
    CoUninitialize();
}
