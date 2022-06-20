/*
 * Copyright 2022 Connor McAdams for CodeWeavers
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
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static void clear_uia_node_ptr_safearray(SAFEARRAY *sa, LONG elems)
{
    HUIANODE node;
    HRESULT hr;
    LONG i;

    for (i = 0; i < elems; i++)
    {
        hr = SafeArrayGetElement(sa, &i, &node);
        if (FAILED(hr))
            break;
        UiaNodeRelease(node);
    }
}

static void create_uia_node_safearray(VARIANT *in, VARIANT *out)
{
    LONG i, idx, lbound, ubound, elems;
    HUIANODE node;
    SAFEARRAY *sa;
    HRESULT hr;
    UINT dims;

    dims = SafeArrayGetDim(V_ARRAY(in));
    if (!dims || (dims > 1))
    {
        WARN("Invalid dimensions %d for element safearray.\n", dims);
        return;
    }

    hr = SafeArrayGetLBound(V_ARRAY(in), 1, &lbound);
    if (FAILED(hr))
        return;

    hr = SafeArrayGetUBound(V_ARRAY(in), 1, &ubound);
    if (FAILED(hr))
        return;

    elems = (ubound - lbound) + 1;
    if (!(sa = SafeArrayCreateVector(VT_UINT_PTR, 0, elems)))
        return;

    for (i = 0; i < elems; i++)
    {
        IRawElementProviderSimple *elprov;
        IUnknown *unk;

        idx = lbound + i;
        hr = SafeArrayGetElement(V_ARRAY(in), &idx, &unk);
        if (FAILED(hr))
            break;

        hr = IUnknown_QueryInterface(unk, &IID_IRawElementProviderSimple, (void **)&elprov);
        IUnknown_Release(unk);
        if (FAILED(hr))
            break;

        hr = UiaNodeFromProvider(elprov, &node);
        if (FAILED(hr))
            break;

        IRawElementProviderSimple_Release(elprov);
        hr = SafeArrayPutElement(sa, &i, &node);
        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
    {
        clear_uia_node_ptr_safearray(sa, elems);
        SafeArrayDestroy(sa);
        return;
    }

    V_VT(out) = VT_UINT_PTR | VT_ARRAY;
    V_ARRAY(out) = sa;
}

/* Convert a VT_UINT_PTR SAFEARRAY to VT_UNKNOWN. */
static void uia_node_ptr_to_unk_safearray(VARIANT *in)
{
    SAFEARRAY *sa = NULL;
    LONG ubound, i;
    HUIANODE node;
    HRESULT hr;

    hr = SafeArrayGetUBound(V_ARRAY(in), 1, &ubound);
    if (FAILED(hr))
        goto exit;

    if (!(sa = SafeArrayCreateVector(VT_UNKNOWN, 0, ubound + 1)))
    {
        hr = E_FAIL;
        goto exit;
    }

    for (i = 0; i < (ubound + 1); i++)
    {
        hr = SafeArrayGetElement(V_ARRAY(in), &i, &node);
        if (FAILED(hr))
            break;

        hr = SafeArrayPutElement(sa, &i, node);
        if (FAILED(hr))
            break;

        UiaNodeRelease(node);
    }

exit:
    if (FAILED(hr))
    {
        clear_uia_node_ptr_safearray(V_ARRAY(in), ubound + 1);
        if (sa)
            SafeArrayDestroy(sa);
    }

    VariantClear(in);
    if (SUCCEEDED(hr))
    {
        V_VT(in) = VT_UNKNOWN | VT_ARRAY;
        V_ARRAY(in) = sa;
    }
}

static HRESULT get_global_interface_table(IGlobalInterfaceTable **git)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_StdGlobalInterfaceTable, NULL,
            CLSCTX_INPROC_SERVER, &IID_IGlobalInterfaceTable, (void **)git);
    if (FAILED(hr))
        WARN("Failed to get GlobalInterfaceTable, hr %#lx\n", hr);

    return hr;
}

/*
 * IWineUiaNode interface.
 */
struct uia_node {
    IWineUiaNode IWineUiaNode_iface;
    LONG ref;

    IWineUiaProvider *prov;
    DWORD git_cookie;
};

static inline struct uia_node *impl_from_IWineUiaNode(IWineUiaNode *iface)
{
    return CONTAINING_RECORD(iface, struct uia_node, IWineUiaNode_iface);
}

static HRESULT WINAPI uia_node_QueryInterface(IWineUiaNode *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaNode) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaNode_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_node_AddRef(IWineUiaNode *iface)
{
    struct uia_node *node = impl_from_IWineUiaNode(iface);
    ULONG ref = InterlockedIncrement(&node->ref);

    TRACE("%p, refcount %ld\n", node, ref);
    return ref;
}

static ULONG WINAPI uia_node_Release(IWineUiaNode *iface)
{
    struct uia_node *node = impl_from_IWineUiaNode(iface);
    ULONG ref = InterlockedDecrement(&node->ref);

    TRACE("%p, refcount %ld\n", node, ref);
    if (!ref)
    {
        if (node->git_cookie)
        {
            IGlobalInterfaceTable *git;
            HRESULT hr;

            hr = get_global_interface_table(&git);
            if (SUCCEEDED(hr))
            {
                hr = IGlobalInterfaceTable_RevokeInterfaceFromGlobal(git, node->git_cookie);
                if (FAILED(hr))
                    WARN("Failed to get revoke provider interface from Global Interface Table, hr %#lx\n", hr);
            }
        }

        IWineUiaProvider_Release(node->prov);
        heap_free(node);
    }

    return ref;
}

static HRESULT WINAPI uia_node_get_provider(IWineUiaNode *iface, IWineUiaProvider **out_prov)
{
    struct uia_node *node = impl_from_IWineUiaNode(iface);

    if (node->git_cookie)
    {
        IGlobalInterfaceTable *git;
        IWineUiaProvider *prov;
        HRESULT hr;

        hr = get_global_interface_table(&git);
        if (FAILED(hr))
            return hr;

        hr = IGlobalInterfaceTable_GetInterfaceFromGlobal(git, node->git_cookie,
                &IID_IWineUiaProvider, (void **)&prov);
        if (FAILED(hr))
        {
            ERR("Failed to get provider interface from GlobalInterfaceTable, hr %#lx\n", hr);
            return hr;
        }
        *out_prov = prov;
    }
    else
    {
        *out_prov = node->prov;
        IWineUiaProvider_AddRef(node->prov);
    }

    return S_OK;
}

static const IWineUiaNodeVtbl uia_node_vtbl = {
    uia_node_QueryInterface,
    uia_node_AddRef,
    uia_node_Release,
    uia_node_get_provider,
};

static struct uia_node *unsafe_impl_from_IWineUiaNode(IWineUiaNode *iface)
{
    if (!iface || (iface->lpVtbl != &uia_node_vtbl))
        return NULL;

    return CONTAINING_RECORD(iface, struct uia_node, IWineUiaNode_iface);
}

/*
 * IWineUiaProvider interface.
 */
struct uia_provider {
    IWineUiaProvider IWineUiaProvider_iface;
    LONG ref;

    IRawElementProviderSimple *elprov;
};

static inline struct uia_provider *impl_from_IWineUiaProvider(IWineUiaProvider *iface)
{
    return CONTAINING_RECORD(iface, struct uia_provider, IWineUiaProvider_iface);
}

static HRESULT WINAPI uia_provider_QueryInterface(IWineUiaProvider *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaProvider) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaProvider_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_provider_AddRef(IWineUiaProvider *iface)
{
    struct uia_provider *prov = impl_from_IWineUiaProvider(iface);
    ULONG ref = InterlockedIncrement(&prov->ref);

    TRACE("%p, refcount %ld\n", prov, ref);
    return ref;
}

static ULONG WINAPI uia_provider_Release(IWineUiaProvider *iface)
{
    struct uia_provider *prov = impl_from_IWineUiaProvider(iface);
    ULONG ref = InterlockedDecrement(&prov->ref);

    TRACE("%p, refcount %ld\n", prov, ref);
    if (!ref)
    {
        IRawElementProviderSimple_Release(prov->elprov);
        heap_free(prov);
    }

    return ref;
}

static void get_variant_for_node(HUIANODE node, VARIANT *v)
{
#ifdef _WIN64
            V_VT(v) = VT_I8;
            V_I8(v) = (UINT64)node;
#else
            V_VT(v) = VT_I4;
            V_I4(v) = (UINT32)node;
#endif
}

static HRESULT WINAPI uia_provider_get_prop_val(IWineUiaProvider *iface,
        const struct uia_prop_info *prop_info, VARIANT *ret_val)
{
    struct uia_provider *prov = impl_from_IWineUiaProvider(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p, %p\n", iface, prop_info, ret_val);

    VariantInit(&v);
    hr = IRawElementProviderSimple_GetPropertyValue(prov->elprov, prop_info->prop_id, &v);
    if (FAILED(hr))
        goto exit;

    switch (prop_info->type)
    {
    case UIAutomationType_Int:
        if (V_VT(&v) != VT_I4)
        {
            WARN("Invalid vt %d for UIAutomationType_Int\n", V_VT(&v));
            goto exit;
        }
        *ret_val = v;
        break;

    case UIAutomationType_IntArray:
        if (V_VT(&v) != (VT_I4 | VT_ARRAY))
        {
            WARN("Invalid vt %d for UIAutomationType_IntArray\n", V_VT(&v));
            goto exit;
        }
        *ret_val = v;
        break;

    case UIAutomationType_Double:
        if (V_VT(&v) != VT_R8)
        {
            WARN("Invalid vt %d for UIAutomationType_Double\n", V_VT(&v));
            goto exit;
        }
        *ret_val = v;
        break;

    case UIAutomationType_DoubleArray:
        if (V_VT(&v) != (VT_R8 | VT_ARRAY))
        {
            WARN("Invalid vt %d for UIAutomationType_DoubleArray\n", V_VT(&v));
            goto exit;
        }
        *ret_val = v;
        break;

    case UIAutomationType_Bool:
        if (V_VT(&v) != VT_BOOL)
        {
            WARN("Invalid vt %d for UIAutomationType_Bool\n", V_VT(&v));
            goto exit;
        }
        *ret_val = v;
        break;

    case UIAutomationType_String:
        if (V_VT(&v) != VT_BSTR)
        {
            WARN("Invalid vt %d for UIAutomationType_String\n", V_VT(&v));
            goto exit;
        }
        *ret_val = v;
        break;

    case UIAutomationType_Element:
    {
        IRawElementProviderSimple *elprov;
        HUIANODE node;

        if (V_VT(&v) != VT_UNKNOWN)
        {
            WARN("Invalid vt %d for UIAutomationType_Element\n", V_VT(&v));
            goto exit;
        }

        hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IRawElementProviderSimple,
                (void **)&elprov);
        if (FAILED(hr))
            goto exit;

        hr = UiaNodeFromProvider(elprov, &node);
        if (SUCCEEDED(hr))
        {
            get_variant_for_node(node, ret_val);
            VariantClear(&v);
            IRawElementProviderSimple_Release(elprov);
        }
        break;
    }

    case UIAutomationType_ElementArray:
        if (V_VT(&v) != (VT_UNKNOWN | VT_ARRAY))
        {
            WARN("Invalid vt %d for UIAutomationType_ElementArray\n", V_VT(&v));
            goto exit;
        }
        create_uia_node_safearray(&v, ret_val);
        if (V_VT(ret_val) == (VT_UINT_PTR | VT_ARRAY))
            VariantClear(&v);
        break;

    default:
        break;
    }

exit:
    if (V_VT(ret_val) == VT_EMPTY)
        VariantClear(&v);

    return S_OK;
}

static const IWineUiaProviderVtbl uia_provider_vtbl = {
    uia_provider_QueryInterface,
    uia_provider_AddRef,
    uia_provider_Release,
    uia_provider_get_prop_val,
};

static HRESULT create_wine_uia_provider(struct uia_node *node, IRawElementProviderSimple *elprov)
{
    static const int supported_prov_opts = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    enum ProviderOptions prov_opts;
    struct uia_provider *prov;
    HRESULT hr;

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opts);
    if (FAILED(hr))
        return hr;

    if (prov_opts & ~supported_prov_opts)
        FIXME("Ignoring unsupported ProviderOption(s) %#x\n", prov_opts & ~supported_prov_opts);

    prov = heap_alloc_zero(sizeof(*prov));
    if (!prov)
        return E_OUTOFMEMORY;

    prov->IWineUiaProvider_iface.lpVtbl = &uia_provider_vtbl;
    prov->elprov = elprov;
    prov->ref = 1;
    node->prov = &prov->IWineUiaProvider_iface;

    /*
     * If the UseComThreading ProviderOption is specified, all calls to the
     * provided IRawElementProviderSimple need to respect the apartment type
     * of the thread that creates the HUIANODE. i.e, if it's created in an
     * STA, and the HUIANODE is used in an MTA, we need to provide a proxy.
     */
    if (prov_opts & ProviderOptions_UseComThreading)
    {
        IGlobalInterfaceTable *git;

        hr = get_global_interface_table(&git);
        if (FAILED(hr))
        {
            heap_free(prov);
            return hr;
        }

        hr = IGlobalInterfaceTable_RegisterInterfaceInGlobal(git, (IUnknown *)&prov->IWineUiaProvider_iface,
                &IID_IWineUiaProvider, &node->git_cookie);
        if (FAILED(hr))
        {
            heap_free(prov);
            return hr;
        }
    }

    IRawElementProviderSimple_AddRef(elprov);
    return S_OK;
}

/***********************************************************************
 *          UiaNodeFromProvider (uiautomationcore.@)
 */
HRESULT WINAPI UiaNodeFromProvider(IRawElementProviderSimple *elprov, HUIANODE *huianode)
{
    struct uia_node *node;
    HRESULT hr;

    TRACE("(%p, %p)\n", elprov, huianode);

    if (!elprov || !huianode)
        return E_INVALIDARG;

    *huianode = NULL;

    node = heap_alloc_zero(sizeof(*node));
    if (!node)
        return E_OUTOFMEMORY;

    hr = create_wine_uia_provider(node, elprov);
    if (FAILED(hr))
    {
        heap_free(node);
        return hr;
    }

    node->IWineUiaNode_iface.lpVtbl = &uia_node_vtbl;
    node->ref = 1;

    *huianode = (void *)&node->IWineUiaNode_iface;

    return hr;
}

/***********************************************************************
 *          UiaNodeRelease (uiautomationcore.@)
 */
BOOL WINAPI UiaNodeRelease(HUIANODE huianode)
{
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);

    TRACE("(%p)\n", huianode);

    if (!node)
        return FALSE;

    IWineUiaNode_Release(&node->IWineUiaNode_iface);
    return TRUE;
}

/***********************************************************************
 *          UiaGetPropertyValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetPropertyValue(HUIANODE huianode, PROPERTYID prop_id, VARIANT *out_val)
{
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);
    const struct uia_prop_info *prop_info;
    IWineUiaProvider *prov;
    HRESULT hr;
    VARIANT v;

    TRACE("(%p, %d, %p)\n", huianode, prop_id, out_val);

    if (!node || !out_val)
        return E_INVALIDARG;

    V_VT(out_val) = VT_UNKNOWN;
    UiaGetReservedNotSupportedValue(&V_UNKNOWN(out_val));

    prop_info = uia_prop_info_from_id(prop_id);
    if (!prop_info)
        return E_INVALIDARG;

    if (!prop_info->type)
    {
        FIXME("No type info for prop_id %d\n", prop_id);
        return E_NOTIMPL;
    }

    hr = IWineUiaNode_get_provider(&node->IWineUiaNode_iface, &prov);
    if (FAILED(hr))
        return hr;

    VariantInit(&v);
    hr = IWineUiaProvider_get_prop_val(prov, prop_info, &v);
    if (SUCCEEDED(hr) && V_VT(&v) != VT_EMPTY)
    {
        /*
         * ElementArray types come back as an array of pointers to prevent the
         * HUIANODEs from getting marshaled. We need to convert them to
         * VT_UNKNOWN here.
         */
        if (prop_info->type == UIAutomationType_ElementArray)
        {
            uia_node_ptr_to_unk_safearray(&v);
            if (V_VT(&v) != VT_EMPTY)
                *out_val = v;
        }
        else
            *out_val = v;
    }

    IWineUiaProvider_Release(prov);

    return S_OK;
}
