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

static HRESULT get_safearray_bounds(SAFEARRAY *sa, LONG *lbound, LONG *elems)
{
    LONG ubound;
    HRESULT hr;
    UINT dims;

    *lbound = *elems = 0;
    dims = SafeArrayGetDim(sa);
    if (dims != 1)
    {
        WARN("Invalid dimensions %d for safearray.\n", dims);
        return E_FAIL;
    }

    hr = SafeArrayGetLBound(sa, 1, lbound);
    if (FAILED(hr))
        return hr;

    hr = SafeArrayGetUBound(sa, 1, &ubound);
    if (FAILED(hr))
        return hr;

    *elems = (ubound - (*lbound)) + 1;
    return S_OK;
}

int uia_compare_runtime_ids(SAFEARRAY *sa1, SAFEARRAY *sa2)
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
    LONG i, idx, lbound, elems;
    HUIANODE node;
    SAFEARRAY *sa;
    HRESULT hr;

    if (FAILED(get_safearray_bounds(V_ARRAY(in), &lbound, &elems)))
        return;

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

static HWND get_hwnd_from_provider(IRawElementProviderSimple *elprov)
{
    IRawElementProviderSimple *host_prov;
    HRESULT hr;
    VARIANT v;
    HWND hwnd;

    hwnd = NULL;
    VariantInit(&v);
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &host_prov);
    if (SUCCEEDED(hr) && host_prov)
    {
        hr = IRawElementProviderSimple_GetPropertyValue(host_prov, UIA_NativeWindowHandlePropertyId, &v);
        if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
            hwnd = UlongToHandle(V_I4(&v));

        VariantClear(&v);
        IRawElementProviderSimple_Release(host_prov);
    }

    if (!IsWindow(hwnd))
    {
        hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_NativeWindowHandlePropertyId, &v);
        if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
            hwnd = UlongToHandle(V_I4(&v));
        VariantClear(&v);
    }

    return hwnd;
}

static IRawElementProviderSimple *get_provider_hwnd_fragment_root(IRawElementProviderSimple *elprov, HWND *hwnd)
{
    IRawElementProviderFragmentRoot *elroot, *elroot2;
    IRawElementProviderSimple *elprov2, *ret;
    IRawElementProviderFragment *elfrag;
    HRESULT hr;
    int depth;

    *hwnd = NULL;

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    if (FAILED(hr) || !elfrag)
        return NULL;

    depth = 0;
    ret = NULL;
    elroot = elroot2 = NULL;

    /*
     * Recursively walk up the fragment root chain until:
     * We get a fragment root that has an HWND associated with it.
     * We get a NULL fragment root.
     * We get the same fragment root as the current fragment root.
     * We've gone up the chain ten times.
     */
    while (depth < 10)
    {
        hr = IRawElementProviderFragment_get_FragmentRoot(elfrag, &elroot);
        IRawElementProviderFragment_Release(elfrag);
        if (FAILED(hr) || !elroot || (elroot == elroot2))
            break;

        hr = IRawElementProviderFragmentRoot_QueryInterface(elroot, &IID_IRawElementProviderSimple, (void **)&elprov2);
        if (FAILED(hr) || !elprov2)
            break;

        *hwnd = get_hwnd_from_provider(elprov2);
        if (IsWindow(*hwnd))
        {
            ret = elprov2;
            break;
        }

        hr = IRawElementProviderSimple_QueryInterface(elprov2, &IID_IRawElementProviderFragment, (void **)&elfrag);
        IRawElementProviderSimple_Release(elprov2);
        if (FAILED(hr) || !elfrag)
            break;

        if (elroot2)
            IRawElementProviderFragmentRoot_Release(elroot2);
        elroot2 = elroot;
        elroot = NULL;
        depth++;
    }

    if (elroot)
        IRawElementProviderFragmentRoot_Release(elroot);
    if (elroot2)
        IRawElementProviderFragmentRoot_Release(elroot2);

    return ret;
}

/*
 * IWineUiaNode interface.
 */
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

        if (node->prov)
            IWineUiaProvider_Release(node->prov);
        if (!list_empty(&node->prov_thread_list_entry))
            uia_provider_thread_remove_node((HUIANODE)iface);
        if (node->nested_node)
            uia_stop_provider_thread();

        heap_free(node);
    }

    return ref;
}

static HRESULT WINAPI uia_node_get_provider(IWineUiaNode *iface, IWineUiaProvider **out_prov)
{
    struct uia_node *node = impl_from_IWineUiaNode(iface);

    if (node->disconnected)
    {
        *out_prov = NULL;
        return UIA_E_ELEMENTNOTAVAILABLE;
    }

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

static HRESULT WINAPI uia_node_get_prop_val(IWineUiaNode *iface, const GUID *prop_guid,
        VARIANT *ret_val)
{
    int prop_id = UiaLookupId(AutomationIdentifierType_Property, prop_guid);
    struct uia_node *node = impl_from_IWineUiaNode(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %s, %p\n", iface, debugstr_guid(prop_guid), ret_val);

    if (node->disconnected)
    {
        VariantInit(ret_val);
        return UIA_E_ELEMENTNOTAVAILABLE;
    }

    hr = UiaGetPropertyValue((HUIANODE)iface, prop_id, &v);

    /* VT_UNKNOWN is UiaGetReservedNotSupported value, no need to marshal it. */
    if (V_VT(&v) == VT_UNKNOWN)
        V_VT(ret_val) = VT_EMPTY;
    else
        *ret_val = v;

    return hr;
}

static HRESULT WINAPI uia_node_disconnect(IWineUiaNode *iface)
{
    struct uia_node *node = impl_from_IWineUiaNode(iface);

    TRACE("%p\n", node);

    if (node->disconnected)
    {
        ERR("Attempted to disconnect node which was already disconnected.\n");
        return E_FAIL;
    }

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
        node->git_cookie = 0;
    }

    IWineUiaProvider_Release(node->prov);
    node->prov = NULL;
    node->disconnected = TRUE;

    return S_OK;
}

static const IWineUiaNodeVtbl uia_node_vtbl = {
    uia_node_QueryInterface,
    uia_node_AddRef,
    uia_node_Release,
    uia_node_get_provider,
    uia_node_get_prop_val,
    uia_node_disconnect,
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

static void uia_stop_client_thread(void);
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

static HRESULT uia_provider_get_elem_prop_val(struct uia_provider *prov,
        const struct uia_prop_info *prop_info, VARIANT *ret_val)
{
    HRESULT hr;
    VARIANT v;

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
        VariantClear(&v);
        if (FAILED(hr))
            goto exit;

        hr = UiaNodeFromProvider(elprov, &node);
        IRawElementProviderSimple_Release(elprov);
        if (SUCCEEDED(hr))
        {
            if (prov->return_nested_node)
            {
                LRESULT lr = uia_lresult_from_node(node);

                if (!lr)
                    return E_FAIL;

                V_VT(ret_val) = VT_I4;
                V_I4(ret_val) = lr;
            }
            else
                get_variant_for_node(node, ret_val);
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

static SAFEARRAY *append_uia_runtime_id(SAFEARRAY *sa, HWND hwnd, enum ProviderOptions root_opts);
static HRESULT uia_provider_get_special_prop_val(struct uia_provider *prov,
        const struct uia_prop_info *prop_info, VARIANT *ret_val)
{
    HRESULT hr;

    switch (prop_info->prop_id)
    {
    case UIA_RuntimeIdPropertyId:
    {
        IRawElementProviderFragment *elfrag;
        SAFEARRAY *sa;
        LONG lbound;
        int val;

        hr = IRawElementProviderSimple_QueryInterface(prov->elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
        if (FAILED(hr) || !elfrag)
            break;

        hr = IRawElementProviderFragment_GetRuntimeId(elfrag, &sa);
        IRawElementProviderFragment_Release(elfrag);
        if (FAILED(hr) || !sa)
            break;

        hr = SafeArrayGetLBound(sa, 1, &lbound);
        if (FAILED(hr))
        {
            SafeArrayDestroy(sa);
            break;
        }

        hr = SafeArrayGetElement(sa, &lbound, &val);
        if (FAILED(hr))
        {
            SafeArrayDestroy(sa);
            break;
        }

        if (val == UiaAppendRuntimeId)
        {
            enum ProviderOptions prov_opts = 0;
            IRawElementProviderSimple *elprov;
            HWND hwnd;

            elprov = get_provider_hwnd_fragment_root(prov->elprov, &hwnd);
            if (!elprov)
            {
                SafeArrayDestroy(sa);
                return E_FAIL;
            }

            hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opts);
            IRawElementProviderSimple_Release(elprov);
            if (FAILED(hr))
                WARN("get_ProviderOptions for root provider failed with %#lx\n", hr);

            if (!(sa = append_uia_runtime_id(sa, hwnd, prov_opts)))
                break;
        }

        V_VT(ret_val) = VT_I4 | VT_ARRAY;
        V_ARRAY(ret_val) = sa;
        break;
    }

    default:
        break;
    }

    return S_OK;
}

static HRESULT WINAPI uia_provider_get_prop_val(IWineUiaProvider *iface,
        const struct uia_prop_info *prop_info, VARIANT *ret_val)
{
    struct uia_provider *prov = impl_from_IWineUiaProvider(iface);

    TRACE("%p, %p, %p\n", iface, prop_info, ret_val);

    VariantInit(ret_val);
    switch (prop_info->prop_type)
    {
    case PROP_TYPE_ELEM_PROP:
        return uia_provider_get_elem_prop_val(prov, prop_info, ret_val);

    case PROP_TYPE_SPECIAL:
        return uia_provider_get_special_prop_val(prov, prop_info, ret_val);

    default:
        break;
    }

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
    node->hwnd = get_hwnd_from_provider(elprov);

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
    list_init(&node->prov_thread_list_entry);
    list_init(&node->node_map_list_entry);
    node->ref = 1;

    *huianode = (void *)&node->IWineUiaNode_iface;

    return hr;
}

/*
 * UI Automation client thread functions.
 */
struct uia_client_thread
{
    CO_MTA_USAGE_COOKIE mta_cookie;
    HANDLE hthread;
    HWND hwnd;
    LONG ref;
};

struct uia_get_node_prov_args {
    LRESULT lr;
    BOOL unwrap;
};

static struct uia_client_thread client_thread;
static CRITICAL_SECTION client_thread_cs;
static CRITICAL_SECTION_DEBUG client_thread_cs_debug =
{
    0, 0, &client_thread_cs,
    { &client_thread_cs_debug.ProcessLocksList, &client_thread_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": client_thread_cs") }
};
static CRITICAL_SECTION client_thread_cs = { &client_thread_cs_debug, -1, 0, 0, 0, 0 };

#define WM_UIA_CLIENT_GET_NODE_PROV (WM_USER + 1)
#define WM_UIA_CLIENT_THREAD_STOP (WM_USER + 2)
static HRESULT create_wine_uia_nested_node_provider(struct uia_node *node, LRESULT lr, BOOL unwrap);
static LRESULT CALLBACK uia_client_thread_msg_proc(HWND hwnd, UINT msg, WPARAM wparam,
        LPARAM lparam)
{
    switch (msg)
    {
    case WM_UIA_CLIENT_GET_NODE_PROV:
    {
        struct uia_get_node_prov_args *args = (struct uia_get_node_prov_args *)wparam;
        return create_wine_uia_nested_node_provider((struct uia_node *)lparam, args->lr, args->unwrap);
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static DWORD WINAPI uia_client_thread_proc(void *arg)
{
    HANDLE initialized_event = arg;
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowW(L"Message", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!hwnd)
    {
        WARN("CreateWindow failed: %ld\n", GetLastError());
        FreeLibraryAndExitThread(huia_module, 1);
    }

    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)uia_client_thread_msg_proc);
    client_thread.hwnd = hwnd;

    /* Initialization complete, thread can now process window messages. */
    SetEvent(initialized_event);
    TRACE("Client thread started.\n");
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_UIA_CLIENT_THREAD_STOP)
            break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    TRACE("Shutting down UI Automation client thread.\n");

    DestroyWindow(hwnd);
    FreeLibraryAndExitThread(huia_module, 0);
}

static BOOL uia_start_client_thread(void)
{
    BOOL started = TRUE;

    EnterCriticalSection(&client_thread_cs);
    if (++client_thread.ref == 1)
    {
        HANDLE ready_event = NULL;
        HANDLE events[2];
        HMODULE hmodule;
        DWORD wait_obj;
        HRESULT hr;

        /*
         * We use CoIncrementMTAUsage here instead of CoInitialize because it
         * allows us to exit the implicit MTA immediately when the thread
         * reference count hits 0, rather than waiting for the thread to
         * shutdown and call CoUninitialize like the provider thread.
         */
        hr = CoIncrementMTAUsage(&client_thread.mta_cookie);
        if (FAILED(hr))
        {
            started = FALSE;
            goto exit;
        }

        /* Increment DLL reference count. */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (const WCHAR *)uia_start_client_thread, &hmodule);

        events[0] = ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!(client_thread.hthread = CreateThread(NULL, 0, uia_client_thread_proc,
                ready_event, 0, NULL)))
        {
            FreeLibrary(hmodule);
            started = FALSE;
            goto exit;
        }

        events[1] = client_thread.hthread;
        wait_obj = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (wait_obj != WAIT_OBJECT_0)
        {
            CloseHandle(client_thread.hthread);
            started = FALSE;
        }

exit:
        if (ready_event)
            CloseHandle(ready_event);
        if (!started)
        {
            WARN("Failed to start client thread\n");
            if (client_thread.mta_cookie)
                CoDecrementMTAUsage(client_thread.mta_cookie);
            memset(&client_thread, 0, sizeof(client_thread));
        }
    }

    LeaveCriticalSection(&client_thread_cs);
    return started;
}

static void uia_stop_client_thread(void)
{
    EnterCriticalSection(&client_thread_cs);
    if (!--client_thread.ref)
    {
        PostMessageW(client_thread.hwnd, WM_UIA_CLIENT_THREAD_STOP, 0, 0);
        CoDecrementMTAUsage(client_thread.mta_cookie);
        CloseHandle(client_thread.hthread);
        memset(&client_thread, 0, sizeof(client_thread));
    }
    LeaveCriticalSection(&client_thread_cs);
}

/*
 * IWineUiaProvider interface for nested node providers.
 *
 * Nested node providers represent an HUIANODE that resides in another
 * thread or process. We retrieve values from this HUIANODE through the
 * IWineUiaNode interface.
 */
struct uia_nested_node_provider {
    IWineUiaProvider IWineUiaProvider_iface;
    LONG ref;

    IWineUiaNode *nested_node;
};

static inline struct uia_nested_node_provider *impl_from_nested_node_IWineUiaProvider(IWineUiaProvider *iface)
{
    return CONTAINING_RECORD(iface, struct uia_nested_node_provider, IWineUiaProvider_iface);
}

static HRESULT WINAPI uia_nested_node_provider_QueryInterface(IWineUiaProvider *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaProvider) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaProvider_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_nested_node_provider_AddRef(IWineUiaProvider *iface)
{
    struct uia_nested_node_provider *prov = impl_from_nested_node_IWineUiaProvider(iface);
    ULONG ref = InterlockedIncrement(&prov->ref);

    TRACE("%p, refcount %ld\n", prov, ref);
    return ref;
}

static ULONG WINAPI uia_nested_node_provider_Release(IWineUiaProvider *iface)
{
    struct uia_nested_node_provider *prov = impl_from_nested_node_IWineUiaProvider(iface);
    ULONG ref = InterlockedDecrement(&prov->ref);

    TRACE("%p, refcount %ld\n", prov, ref);
    if (!ref)
    {
        IWineUiaNode_Release(prov->nested_node);
        uia_stop_client_thread();
        heap_free(prov);
    }

    return ref;
}

static HRESULT uia_node_from_lresult(LRESULT lr, HUIANODE *huianode);
static HRESULT WINAPI uia_nested_node_provider_get_prop_val(IWineUiaProvider *iface,
        const struct uia_prop_info *prop_info, VARIANT *ret_val)
{
    struct uia_nested_node_provider *prov = impl_from_nested_node_IWineUiaProvider(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p, %p\n", iface, prop_info, ret_val);

    VariantInit(ret_val);
    if (prop_info->type == UIAutomationType_ElementArray)
    {
        FIXME("Element array property types currently unsupported for nested nodes.\n");
        return E_NOTIMPL;
    }

    hr = IWineUiaNode_get_prop_val(prov->nested_node, prop_info->guid, &v);
    if (FAILED(hr))
        return hr;

    switch (prop_info->type)
    {
    case UIAutomationType_Element:
    {
        HUIANODE node;

        hr = uia_node_from_lresult((LRESULT)V_I4(&v), &node);
        if (FAILED(hr))
            return hr;

        get_variant_for_node(node, ret_val);
        VariantClear(&v);
        break;
    }

    default:
        *ret_val = v;
        break;
    }

    return S_OK;
}

static const IWineUiaProviderVtbl uia_nested_node_provider_vtbl = {
    uia_nested_node_provider_QueryInterface,
    uia_nested_node_provider_AddRef,
    uia_nested_node_provider_Release,
    uia_nested_node_provider_get_prop_val,
};

static HRESULT create_wine_uia_nested_node_provider(struct uia_node *node, LRESULT lr,
        BOOL unwrap)
{
    IWineUiaProvider *provider_iface = NULL;
    struct uia_nested_node_provider *prov;
    IGlobalInterfaceTable *git;
    IWineUiaNode *nested_node;
    DWORD git_cookie;
    HRESULT hr;

    hr = ObjectFromLresult(lr, &IID_IWineUiaNode, 0, (void **)&nested_node);
    if (FAILED(hr))
    {
        uia_stop_client_thread();
        return hr;
    }

    /*
     * If we're retrieving a node from an HWND that belongs to the same thread
     * as the client making the request, return a normal provider instead of a
     * nested node provider.
     */
    if (unwrap)
    {
        struct uia_node *node_data = unsafe_impl_from_IWineUiaNode(nested_node);
        struct uia_provider *prov_data;

        if (!node_data)
        {
            ERR("Failed to get uia_node structure from nested node\n");
            uia_stop_client_thread();
            return E_FAIL;
        }

        IWineUiaProvider_AddRef(node_data->prov);
        provider_iface = node_data->prov;
        git_cookie = node_data->git_cookie;
        prov_data = impl_from_IWineUiaProvider(node_data->prov);
        prov_data->return_nested_node = FALSE;

        node_data->git_cookie = 0;
        IWineUiaNode_Release(&node_data->IWineUiaNode_iface);
        uia_stop_client_thread();
    }
    else
    {
        prov = heap_alloc_zero(sizeof(*prov));
        if (!prov)
            return E_OUTOFMEMORY;

        prov->IWineUiaProvider_iface.lpVtbl = &uia_nested_node_provider_vtbl;
        prov->nested_node = nested_node;
        prov->ref = 1;
        provider_iface = &prov->IWineUiaProvider_iface;

        /*
         * We need to use the GIT on all nested node providers so that our
         * IWineUiaNode proxy is used in the correct apartment.
         */
        hr = get_global_interface_table(&git);
        if (FAILED(hr))
        {
            IWineUiaProvider_Release(&prov->IWineUiaProvider_iface);
            return hr;
        }

        hr = IGlobalInterfaceTable_RegisterInterfaceInGlobal(git, (IUnknown *)&prov->IWineUiaProvider_iface,
                &IID_IWineUiaProvider, &git_cookie);
        if (FAILED(hr))
        {
            IWineUiaProvider_Release(&prov->IWineUiaProvider_iface);
            return hr;
        }
    }

    node->prov = provider_iface;
    node->git_cookie = git_cookie;

    return S_OK;
}

static HRESULT uia_node_from_lresult(LRESULT lr, HUIANODE *huianode)
{
    struct uia_node *node;
    HRESULT hr;

    *huianode = NULL;
    node = heap_alloc_zero(sizeof(*node));
    if (!node)
        return E_OUTOFMEMORY;

    node->IWineUiaNode_iface.lpVtbl = &uia_node_vtbl;
    list_init(&node->prov_thread_list_entry);
    list_init(&node->node_map_list_entry);
    node->ref = 1;

    uia_start_client_thread();
    hr = create_wine_uia_nested_node_provider(node, lr, FALSE);
    if (FAILED(hr))
    {
        heap_free(node);
        return hr;
    }

    *huianode = (void *)&node->IWineUiaNode_iface;

    return hr;
}

/*
 * UiaNodeFromHandle is expected to work even if the calling thread hasn't
 * initialized COM. We marshal our node on a separate thread that initializes
 * COM for this reason.
 */
static HRESULT uia_get_provider_from_hwnd(struct uia_node *node)
{
    struct uia_get_node_prov_args args;

    if (!uia_start_client_thread())
        return E_FAIL;

    SetLastError(NOERROR);
    args.lr = SendMessageW(node->hwnd, WM_GETOBJECT, 0, UiaRootObjectId);
    if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
    {
        uia_stop_client_thread();
        return UIA_E_ELEMENTNOTAVAILABLE;
    }

    if (!args.lr)
    {
        FIXME("No native UIA provider for hwnd %p, MSAA proxy currently unimplemented.\n", node->hwnd);
        uia_stop_client_thread();
        return E_NOTIMPL;
    }

    args.unwrap = GetCurrentThreadId() == GetWindowThreadProcessId(node->hwnd, NULL);
    return SendMessageW(client_thread.hwnd, WM_UIA_CLIENT_GET_NODE_PROV, (WPARAM)&args, (LPARAM)node);
}

/***********************************************************************
 *          UiaNodeFromHandle (uiautomationcore.@)
 */
HRESULT WINAPI UiaNodeFromHandle(HWND hwnd, HUIANODE *huianode)
{
    struct uia_node *node;
    HRESULT hr;

    TRACE("(%p, %p)\n", hwnd, huianode);

    if (!huianode)
        return E_INVALIDARG;

    *huianode = NULL;

    if (!IsWindow(hwnd))
        return UIA_E_ELEMENTNOTAVAILABLE;

    node = heap_alloc_zero(sizeof(*node));
    if (!node)
        return E_OUTOFMEMORY;

    node->hwnd = hwnd;
    node->IWineUiaNode_iface.lpVtbl = &uia_node_vtbl;
    list_init(&node->prov_thread_list_entry);
    list_init(&node->node_map_list_entry);
    node->ref = 1;

    hr = uia_get_provider_from_hwnd(node);
    if (FAILED(hr))
    {
        heap_free(node);
        return hr;
    }

    *huianode = (void *)&node->IWineUiaNode_iface;

    return S_OK;
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

static HRESULT get_prop_val_from_node_provider(struct uia_node *node,
        const struct uia_prop_info *prop_info, VARIANT *v)
{
    IWineUiaProvider *prov;
    HRESULT hr;

    hr = IWineUiaNode_get_provider(&node->IWineUiaNode_iface, &prov);
    if (FAILED(hr))
        return hr;

    VariantInit(v);
    hr = IWineUiaProvider_get_prop_val(prov, prop_info, v);
    IWineUiaProvider_Release(prov);

    return hr;
}

/***********************************************************************
 *          UiaGetPropertyValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetPropertyValue(HUIANODE huianode, PROPERTYID prop_id, VARIANT *out_val)
{
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);
    const struct uia_prop_info *prop_info;
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

    switch (prop_id)
    {
    case UIA_RuntimeIdPropertyId:
    {
        SAFEARRAY *sa;

        hr = UiaGetRuntimeId(huianode, &sa);
        if (SUCCEEDED(hr) && sa)
        {
            V_VT(out_val) = VT_I4 | VT_ARRAY;
            V_ARRAY(out_val) = sa;
        }
        return S_OK;
    }

    default:
        break;
    }

    hr = get_prop_val_from_node_provider(node, prop_info, &v);
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

    return hr;
}

#define UIA_RUNTIME_ID_PREFIX 42

enum fragment_root_prov_type_ids {
    FRAGMENT_ROOT_NONCLIENT_TYPE_ID = 0x03,
    FRAGMENT_ROOT_MAIN_TYPE_ID      = 0x04,
    FRAGMENT_ROOT_OVERRIDE_TYPE_ID  = 0x05,
};

static HRESULT write_runtime_id_base(SAFEARRAY *sa, HWND hwnd)
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

static SAFEARRAY *append_uia_runtime_id(SAFEARRAY *sa, HWND hwnd, enum ProviderOptions root_opts)
{
    LONG i, idx, lbound, elems;
    SAFEARRAY *sa2, *ret;
    HRESULT hr;
    int val;

    ret = sa2 = NULL;
    hr = get_safearray_bounds(sa, &lbound, &elems);
    if (FAILED(hr))
        goto exit;

    /* elems includes the UiaAppendRuntimeId value, so we only add 2. */
    if (!(sa2 = SafeArrayCreateVector(VT_I4, 0, elems + 2)))
        goto exit;

    hr = write_runtime_id_base(sa2, hwnd);
    if (FAILED(hr))
        goto exit;

    if (root_opts & ProviderOptions_NonClientAreaProvider)
        val = FRAGMENT_ROOT_NONCLIENT_TYPE_ID;
    else if (root_opts & ProviderOptions_OverrideProvider)
        val = FRAGMENT_ROOT_OVERRIDE_TYPE_ID;
    else
        val = FRAGMENT_ROOT_MAIN_TYPE_ID;

    idx = 2;
    hr = SafeArrayPutElement(sa2, &idx, &val);
    if (FAILED(hr))
        goto exit;

    for (i = 0; i < (elems - 1); i++)
    {
        idx = (lbound + 1) + i;
        hr = SafeArrayGetElement(sa, &idx, &val);
        if (FAILED(hr))
            goto exit;

        idx = (3 + i);
        hr = SafeArrayPutElement(sa2, &idx, &val);
        if (FAILED(hr))
            goto exit;
    }

    ret = sa2;

exit:

    if (!ret)
        SafeArrayDestroy(sa2);

    SafeArrayDestroy(sa);
    return ret;
}

/***********************************************************************
 *          UiaGetRuntimeId (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetRuntimeId(HUIANODE huianode, SAFEARRAY **runtime_id)
{
    const struct uia_prop_info *prop_info = uia_prop_info_from_id(UIA_RuntimeIdPropertyId);
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);
    HRESULT hr;

    TRACE("(%p, %p)\n", huianode, runtime_id);

    if (!node || !runtime_id)
        return E_INVALIDARG;

    *runtime_id = NULL;

    /* Provide an HWND based runtime ID if the node has an HWND. */
    if (node->hwnd)
    {
        SAFEARRAY *sa;

        if (!(sa = SafeArrayCreateVector(VT_I4, 0, 2)))
            return E_FAIL;

        hr = write_runtime_id_base(sa, node->hwnd);
        if (FAILED(hr))
        {
            SafeArrayDestroy(sa);
            return hr;
        }

        *runtime_id = sa;
        return S_OK;
    }
    else
    {
        VARIANT v;

        hr = get_prop_val_from_node_provider(node, prop_info, &v);
        if (FAILED(hr))
        {
            VariantClear(&v);
            return hr;
        }

        if (V_VT(&v) == (VT_I4 | VT_ARRAY))
            *runtime_id = V_ARRAY(&v);
    }

    return S_OK;
}

/***********************************************************************
 *          UiaHUiaNodeFromVariant (uiautomationcore.@)
 */
HRESULT WINAPI UiaHUiaNodeFromVariant(VARIANT *in_val, HUIANODE *huianode)
{
    const VARTYPE expected_vt = sizeof(void *) == 8 ? VT_I8 : VT_I4;

    TRACE("(%p, %p)\n", in_val, huianode);

    if (!in_val || !huianode)
        return E_INVALIDARG;

    *huianode = NULL;
    if ((V_VT(in_val) != expected_vt) && (V_VT(in_val) != VT_UNKNOWN))
    {
        WARN("Invalid vt %d\n", V_VT(in_val));
        return E_INVALIDARG;
    }

    if (V_VT(in_val) == VT_UNKNOWN)
    {
        if (V_UNKNOWN(in_val))
            IUnknown_AddRef(V_UNKNOWN(in_val));
        *huianode = (HUIANODE)V_UNKNOWN(in_val);
    }
    else
    {
#ifdef _WIN64
        *huianode = (HUIANODE)V_I8(in_val);
#else
        *huianode = (HUIANODE)V_I4(in_val);
#endif
    }

    return S_OK;
}
