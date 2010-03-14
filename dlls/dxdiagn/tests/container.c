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

#include <stdio.h>
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

static void test_dot_parsing(void)
{
    HRESULT hr;
    WCHAR containerbufW[256] = {0}, childbufW[256] = {0};
    DWORD count, index;
    size_t i;
    static const struct
    {
        const char *format;
        const HRESULT expect;
    } test_strings[] = {
        { "%s.%s",   S_OK },
        { "%s.%s.",  S_OK },
        { ".%s.%s",  E_INVALIDARG },
        { "%s.%s..", E_INVALIDARG },
        { ".%s.%s.", E_INVALIDARG },
        { "..%s.%s", E_INVALIDARG },
    };

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Find a container with a child container of its own. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        IDxDiagContainer *child;

        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, containerbufW, sizeof(containerbufW)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, containerbufW, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_EnumChildContainerNames(child, 0, childbufW, sizeof(childbufW)/sizeof(WCHAR));
            ok(hr == S_OK || hr == E_INVALIDARG,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK or E_INVALIDARG, got 0x%08x\n", hr);
            IDxDiagContainer_Release(child);

            if (SUCCEEDED(hr))
                break;
        }
    }

    if (!*containerbufW || !*childbufW)
    {
        skip("Unable to find a suitable container\n");
        goto cleanup;
    }

    trace("Testing IDxDiagContainer::GetChildContainer dot parsing with container %s and child container %s.\n",
          wine_dbgstr_w(containerbufW), wine_dbgstr_w(childbufW));

    for (i = 0; i < sizeof(test_strings)/sizeof(test_strings[0]); i++)
    {
        IDxDiagContainer *child;
        char containerbufA[256];
        char childbufA[256];
        char dotbufferA[255 + 255 + 3 + 1];
        WCHAR dotbufferW[255 + 255 + 3 + 1]; /* containerbuf + childbuf + dots + null terminator */

        WideCharToMultiByte(CP_ACP, 0, containerbufW, -1, containerbufA, sizeof(containerbufA), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, childbufW, -1, childbufA, sizeof(childbufA), NULL, NULL);
        sprintf(dotbufferA, test_strings[i].format, containerbufA, childbufA);
        MultiByteToWideChar(CP_ACP, 0, dotbufferA, -1, dotbufferW, sizeof(dotbufferW)/sizeof(WCHAR));

        trace("Trying container name %s\n", wine_dbgstr_w(dotbufferW));
        hr = IDxDiagContainer_GetChildContainer(pddc, dotbufferW, &child);
        ok(hr == test_strings[i].expect,
           "Expected IDxDiagContainer::GetChildContainer to return 0x%08x for %s, got 0x%08x\n",
           test_strings[i].expect, wine_dbgstr_w(dotbufferW), hr);
        if (SUCCEEDED(hr))
            IDxDiagContainer_Release(child);
    }

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_EnumPropNames(void)
{
    HRESULT hr;
    WCHAR container[256], property[256];
    IDxDiagContainer *child = NULL;
    DWORD count, index, propcount;
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const WCHAR zerotestW[] = {0,'e','s','t',0};

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Find a container with a non-zero number of properties. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, sizeof(container)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_GetNumberOfProps(child, &propcount);
            ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);

            if (!propcount)
            {
                IDxDiagContainer_Release(child);
                child = NULL;
            }
            else
                break;
        }
    }

    if (!child)
    {
        skip("Unable to find a container with non-zero property count\n");
        goto cleanup;
    }

    hr = IDxDiagContainer_EnumPropNames(child, ~0, NULL, 0);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08x\n", hr);

    memcpy(property, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumPropNames(child, ~0, property, 0);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(property, testW, sizeof(testW)),
       "Expected the property buffer to be unchanged, got %s\n", wine_dbgstr_w(property));

    memcpy(property, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumPropNames(child, ~0, property, sizeof(property)/sizeof(WCHAR));
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(property, testW, sizeof(testW)),
       "Expected the property buffer to be unchanged, got %s\n", wine_dbgstr_w(property));

    IDxDiagContainer_Release(child);

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
    test_dot_parsing();
    test_EnumPropNames();
    CoUninitialize();
}
