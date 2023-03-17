/*
 * Copyright 2023 Connor McAdams for CodeWeavers
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

#include "uia_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

/*
 * Global interface table helper functions.
 */
static HRESULT get_global_interface_table(IGlobalInterfaceTable **git)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_StdGlobalInterfaceTable, NULL,
            CLSCTX_INPROC_SERVER, &IID_IGlobalInterfaceTable, (void **)git);
    if (FAILED(hr))
        WARN("Failed to get GlobalInterfaceTable, hr %#lx\n", hr);

    return hr;
}

HRESULT register_interface_in_git(IUnknown *iface, REFIID riid, DWORD *ret_cookie)
{
    IGlobalInterfaceTable *git;
    DWORD git_cookie;
    HRESULT hr;

    *ret_cookie = 0;
    hr = get_global_interface_table(&git);
    if (FAILED(hr))
        return hr;

    hr = IGlobalInterfaceTable_RegisterInterfaceInGlobal(git, iface, riid, &git_cookie);
    if (FAILED(hr))
    {
        WARN("Failed to register interface in GlobalInterfaceTable, hr %#lx\n", hr);
        return hr;
    }

    *ret_cookie = git_cookie;

    return S_OK;
}

HRESULT unregister_interface_in_git(DWORD git_cookie)
{
    IGlobalInterfaceTable *git;
    HRESULT hr;

    hr = get_global_interface_table(&git);
    if (FAILED(hr))
        return hr;

    hr = IGlobalInterfaceTable_RevokeInterfaceFromGlobal(git, git_cookie);
    if (FAILED(hr))
        WARN("Failed to revoke interface from GlobalInterfaceTable, hr %#lx\n", hr);

    return hr;
}

HRESULT get_interface_in_git(REFIID riid, DWORD git_cookie, IUnknown **ret_iface)
{
    IGlobalInterfaceTable *git;
    IUnknown *iface;
    HRESULT hr;

    hr = get_global_interface_table(&git);
    if (FAILED(hr))
        return hr;

    hr = IGlobalInterfaceTable_GetInterfaceFromGlobal(git, git_cookie, riid, (void **)&iface);
    if (FAILED(hr))
    {
        ERR("Failed to get interface from Global Interface Table, hr %#lx\n", hr);
        return hr;
    }

    *ret_iface = iface;

    return S_OK;
}

HRESULT get_safearray_dim_bounds(SAFEARRAY *sa, UINT dim, LONG *lbound, LONG *elems)
{
    LONG ubound;
    HRESULT hr;

    *lbound = *elems = 0;
    hr = SafeArrayGetLBound(sa, dim, lbound);
    if (FAILED(hr))
        return hr;

    hr = SafeArrayGetUBound(sa, dim, &ubound);
    if (FAILED(hr))
        return hr;

    *elems = (ubound - (*lbound)) + 1;
    return S_OK;
}

HRESULT get_safearray_bounds(SAFEARRAY *sa, LONG *lbound, LONG *elems)
{
    UINT dims;

    *lbound = *elems = 0;
    dims = SafeArrayGetDim(sa);
    if (dims != 1)
    {
        WARN("Invalid dimensions %d for safearray.\n", dims);
        return E_FAIL;
    }

    return get_safearray_dim_bounds(sa, 1, lbound, elems);
}

int uia_compare_safearrays(SAFEARRAY *sa1, SAFEARRAY *sa2, int prop_type)
{
    LONG i, idx, lbound[2], elems[2];
    int val[2];
    HRESULT hr;

    hr = get_safearray_bounds(sa1, &lbound[0], &elems[0]);
    if (FAILED(hr))
    {
        ERR("Failed to get safearray bounds from sa1 with hr %#lx\n", hr);
        return -1;
    }

    hr = get_safearray_bounds(sa2, &lbound[1], &elems[1]);
    if (FAILED(hr))
    {
        ERR("Failed to get safearray bounds from sa2 with hr %#lx\n", hr);
        return -1;
    }

    if (elems[0] != elems[1])
        return (elems[0] > elems[1]) - (elems[0] < elems[1]);

    if (prop_type != UIAutomationType_IntArray)
    {
        FIXME("Array type %#x value comparison currently unimplemented.\n", prop_type);
        return -1;
    }

    for (i = 0; i < elems[0]; i++)
    {
        idx = lbound[0] + i;
        hr = SafeArrayGetElement(sa1, &idx, &val[0]);
        if (FAILED(hr))
        {
            ERR("Failed to get element from sa1 with hr %#lx\n", hr);
            return -1;
        }

        idx = lbound[1] + i;
        hr = SafeArrayGetElement(sa2, &idx, &val[1]);
        if (FAILED(hr))
        {
            ERR("Failed to get element from sa2 with hr %#lx\n", hr);
            return -1;
        }

        if (val[0] != val[1])
            return (val[0] > val[1]) - (val[0] < val[1]);
    }

    return 0;
}
