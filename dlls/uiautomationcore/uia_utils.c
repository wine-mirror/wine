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

#define UIA_RUNTIME_ID_PREFIX 42
HRESULT write_runtime_id_base(SAFEARRAY *sa, HWND hwnd)
{
    const int rt_id[2] = { UIA_RUNTIME_ID_PREFIX, HandleToUlong(hwnd) };
    HRESULT hr;
    LONG idx;

    for (idx = 0; idx < ARRAY_SIZE(rt_id); idx++)
    {
        hr = SafeArrayPutElement(sa, &idx, (void *)&rt_id[idx]);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

/*
 * UiaCondition cloning functions.
 */
static void uia_condition_destroy(struct UiaCondition *cond)
{
    if (!cond)
        return;

    switch (cond->ConditionType)
    {
    case ConditionType_Property:
    {
        struct UiaPropertyCondition *prop_cond = (struct UiaPropertyCondition *)cond;

        VariantClear(&prop_cond->Value);
        break;
    }

    case ConditionType_Not:
    {
        struct UiaNotCondition *not_cond = (struct UiaNotCondition *)cond;

        uia_condition_destroy(not_cond->pConditions);
        break;
    }

    case ConditionType_And:
    case ConditionType_Or:
    {
        struct UiaAndOrCondition *and_or_cond = (struct UiaAndOrCondition *)cond;
        int i;

        for (i = 0; i < and_or_cond->cConditions; i++)
            uia_condition_destroy(and_or_cond->ppConditions[i]);
        heap_free(and_or_cond->ppConditions);
        break;
    }

    default:
        break;
    }

    heap_free(cond);
}

static HRESULT uia_condition_clone(struct UiaCondition **dst, struct UiaCondition *src)
{
    HRESULT hr = S_OK;

    *dst = NULL;
    switch (src->ConditionType)
    {
    case ConditionType_True:
    case ConditionType_False:
        if (!(*dst = heap_alloc_zero(sizeof(**dst))))
            return E_OUTOFMEMORY;

        (*dst)->ConditionType = src->ConditionType;
        break;

    case ConditionType_Property:
    {
        struct UiaPropertyCondition *prop_cond = heap_alloc_zero(sizeof(*prop_cond));
        struct UiaPropertyCondition *src_cond = (struct UiaPropertyCondition *)src;

        if (!prop_cond)
            return E_OUTOFMEMORY;

        *dst = (struct UiaCondition *)prop_cond;
        prop_cond->ConditionType = ConditionType_Property;
        prop_cond->PropertyId = src_cond->PropertyId;
        prop_cond->Flags = src_cond->Flags;
        VariantInit(&prop_cond->Value);
        hr = VariantCopy(&prop_cond->Value, &src_cond->Value);
        break;
    }

    case ConditionType_Not:
    {
        struct UiaNotCondition *not_cond = heap_alloc_zero(sizeof(*not_cond));
        struct UiaNotCondition *src_cond = (struct UiaNotCondition *)src;

        if (!not_cond)
            return E_OUTOFMEMORY;

        *dst = (struct UiaCondition *)not_cond;
        not_cond->ConditionType = ConditionType_Not;
        hr = uia_condition_clone(&not_cond->pConditions, src_cond->pConditions);
        break;
    }

    case ConditionType_And:
    case ConditionType_Or:
    {
        struct UiaAndOrCondition *and_or_cond = heap_alloc_zero(sizeof(*and_or_cond));
        struct UiaAndOrCondition *src_cond = (struct UiaAndOrCondition *)src;
        int i;

        if (!and_or_cond)
            return E_OUTOFMEMORY;

        *dst = (struct UiaCondition *)and_or_cond;
        and_or_cond->ConditionType = src_cond->ConditionType;
        and_or_cond->ppConditions = heap_alloc_zero(sizeof(*and_or_cond->ppConditions) * src_cond->cConditions);
        if (!and_or_cond->ppConditions)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        and_or_cond->cConditions = src_cond->cConditions;
        for (i = 0; i < src_cond->cConditions; i++)
        {
            hr = uia_condition_clone(&and_or_cond->ppConditions[i], src_cond->ppConditions[i]);
            if (FAILED(hr))
                goto exit;
        }

        break;
    }

    default:
        WARN("Tried to clone condition with invalid type %d\n", src->ConditionType);
        return E_INVALIDARG;
    }

exit:
    if (FAILED(hr))
    {
        uia_condition_destroy(*dst);
        *dst = NULL;
    }

    return hr;
}

/*
 * UiaCacheRequest cloning functions.
 */
void uia_cache_request_destroy(struct UiaCacheRequest *cache_req)
{
    uia_condition_destroy(cache_req->pViewCondition);
    heap_free(cache_req->pProperties);
    heap_free(cache_req->pPatterns);
}

HRESULT uia_cache_request_clone(struct UiaCacheRequest *dst, struct UiaCacheRequest *src)
{
    HRESULT hr;

    hr = uia_condition_clone(&dst->pViewCondition, src->pViewCondition);
    if (FAILED(hr))
        return hr;

    dst->Scope = src->Scope;
    dst->automationElementMode = src->automationElementMode;
    if (src->cProperties)
    {
        if (!(dst->pProperties = heap_alloc_zero(sizeof(*dst->pProperties) * src->cProperties)))
        {
            uia_cache_request_destroy(dst);
            return E_OUTOFMEMORY;
        }

        dst->cProperties = src->cProperties;
        memcpy(dst->pProperties, src->pProperties, sizeof(*dst->pProperties) * dst->cProperties);
    }

    if (src->cPatterns)
    {
        if (!(dst->pPatterns = heap_alloc_zero(sizeof(*dst->pPatterns) * src->cPatterns)))
        {
            uia_cache_request_destroy(dst);
            return E_OUTOFMEMORY;
        }

        dst->cPatterns = src->cPatterns;
        memcpy(dst->pPatterns, src->pPatterns, sizeof(*dst->pPatterns) * dst->cPatterns);
    }

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
