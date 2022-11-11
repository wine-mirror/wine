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

static const struct UiaCondition UiaFalseCondition = { ConditionType_False };

static BOOL uia_array_reserve(void **elements, SIZE_T *capacity, SIZE_T count, SIZE_T size)
{
    SIZE_T max_capacity, new_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(1, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = count;

    if (!*elements)
        new_elements = heap_alloc_zero(new_capacity * size);
    else
        new_elements = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *elements, new_capacity * size);
    if (!new_elements)
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

struct uia_node_array {
    HUIANODE *nodes;
    int node_count;
    SIZE_T node_arr_size;
};

static void clear_node_array(struct uia_node_array *nodes)
{
    if (nodes->nodes)
    {
        int i;

        for (i = 0; i < nodes->node_count; i++)
            UiaNodeRelease(nodes->nodes[i]);

        heap_free(nodes->nodes);
    }

    memset(nodes, 0, sizeof(*nodes));
}

static HRESULT add_node_to_node_array(struct uia_node_array *out_nodes, HUIANODE node)
{
    if (!uia_array_reserve((void **)&out_nodes->nodes, &out_nodes->node_arr_size, out_nodes->node_count + 1,
                sizeof(node)))
        return E_OUTOFMEMORY;

    IWineUiaNode_AddRef((IWineUiaNode *)node);
    out_nodes->nodes[out_nodes->node_count] = node;
    out_nodes->node_count++;

    return S_OK;
}

static HRESULT get_safearray_dim_bounds(SAFEARRAY *sa, UINT dim, LONG *lbound, LONG *elems)
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

static HRESULT get_safearray_bounds(SAFEARRAY *sa, LONG *lbound, LONG *elems)
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
        FIXME("Array type %#x value comparsion currently unimplemented.\n", prop_type);
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

int get_node_provider_type_at_idx(struct uia_node *node, int idx)
{
    int i, prov_idx;

    for (i = prov_idx = 0; i < PROV_TYPE_COUNT; i++)
    {
        if (node->prov[i])
        {
            if (prov_idx == idx)
                return i;
            else
                prov_idx++;
        }
    }

    ERR("Node %p has no provider at idx %d\n", node, idx);
    return 0;
}

static HRESULT get_prov_opts_from_node_provider(IWineUiaNode *node, int idx, int *out_opts)
{
    IWineUiaProvider *prov;
    HRESULT hr;

    *out_opts = 0;
    hr = IWineUiaNode_get_provider(node, idx, &prov);
    if (FAILED(hr))
        return hr;

    hr = IWineUiaProvider_get_prov_opts(prov, out_opts);
    IWineUiaProvider_Release(prov);

    return hr;
}

static HRESULT get_has_parent_from_node_provider(IWineUiaNode *node, int idx, BOOL *out_val)
{
    IWineUiaProvider *prov;
    HRESULT hr;

    *out_val = FALSE;
    hr = IWineUiaNode_get_provider(node, idx, &prov);
    if (FAILED(hr))
        return hr;

    hr = IWineUiaProvider_has_parent(prov, out_val);
    IWineUiaProvider_Release(prov);

    return hr;
}

static HRESULT get_navigate_from_node_provider(IWineUiaNode *node, int idx, int nav_dir, VARIANT *ret_val)
{
    IWineUiaProvider *prov;
    HRESULT hr;

    VariantInit(ret_val);
    hr = IWineUiaNode_get_provider(node, idx, &prov);
    if (FAILED(hr))
        return hr;

    hr = IWineUiaProvider_navigate(prov, nav_dir, ret_val);
    IWineUiaProvider_Release(prov);

    return hr;
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
        int i;

        for (i = 0; i < PROV_TYPE_COUNT; i++)
        {
            if (node->git_cookie[i])
            {
                IGlobalInterfaceTable *git;
                HRESULT hr;

                hr = get_global_interface_table(&git);
                if (SUCCEEDED(hr))
                {
                    hr = IGlobalInterfaceTable_RevokeInterfaceFromGlobal(git, node->git_cookie[i]);
                    if (FAILED(hr))
                        WARN("Failed to get revoke provider interface from Global Interface Table, hr %#lx\n", hr);
                }
            }

            if (node->prov[i])
                IWineUiaProvider_Release(node->prov[i]);
        }

        if (!list_empty(&node->prov_thread_list_entry))
            uia_provider_thread_remove_node((HUIANODE)iface);
        if (node->nested_node)
            uia_stop_provider_thread();

        heap_free(node);
    }

    return ref;
}

static HRESULT WINAPI uia_node_get_provider(IWineUiaNode *iface, int idx, IWineUiaProvider **out_prov)
{
    struct uia_node *node = impl_from_IWineUiaNode(iface);
    int prov_type;

    TRACE("(%p, %d, %p)\n", iface, idx, out_prov);

    *out_prov = NULL;
    if (node->disconnected)
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (idx >= node->prov_count)
        return E_INVALIDARG;

    prov_type = get_node_provider_type_at_idx(node, idx);
    if (node->git_cookie[prov_type])
    {
        IGlobalInterfaceTable *git;
        IWineUiaProvider *prov;
        HRESULT hr;

        hr = get_global_interface_table(&git);
        if (FAILED(hr))
            return hr;

        hr = IGlobalInterfaceTable_GetInterfaceFromGlobal(git, node->git_cookie[prov_type],
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
        *out_prov = node->prov[prov_type];
        IWineUiaProvider_AddRef(node->prov[prov_type]);
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
    int prov_type;

    TRACE("%p\n", node);

    if (node->disconnected)
    {
        ERR("Attempted to disconnect node which was already disconnected.\n");
        return E_FAIL;
    }

    /* Nested nodes can only have one provider. */
    prov_type = get_node_provider_type_at_idx(node, 0);
    if (node->git_cookie[prov_type])
    {
        IGlobalInterfaceTable *git;
        HRESULT hr;

        hr = get_global_interface_table(&git);
        if (SUCCEEDED(hr))
        {
            hr = IGlobalInterfaceTable_RevokeInterfaceFromGlobal(git, node->git_cookie[prov_type]);
            if (FAILED(hr))
                WARN("Failed to get revoke provider interface from Global Interface Table, hr %#lx\n", hr);
        }
        node->git_cookie[prov_type] = 0;
    }

    IWineUiaProvider_Release(node->prov[prov_type]);
    node->prov[prov_type] = NULL;

    node->disconnected = TRUE;
    node->prov_count = 0;

    return S_OK;
}

static HRESULT WINAPI uia_node_get_hwnd(IWineUiaNode *iface, ULONG *out_hwnd)
{
    struct uia_node *node = impl_from_IWineUiaNode(iface);

    TRACE("%p, %p\n", node, out_hwnd);

    *out_hwnd = HandleToUlong(node->hwnd);

    return S_OK;
}

static const IWineUiaNodeVtbl uia_node_vtbl = {
    uia_node_QueryInterface,
    uia_node_AddRef,
    uia_node_Release,
    uia_node_get_provider,
    uia_node_get_prop_val,
    uia_node_disconnect,
    uia_node_get_hwnd,
};

static struct uia_node *unsafe_impl_from_IWineUiaNode(IWineUiaNode *iface)
{
    if (!iface || (iface->lpVtbl != &uia_node_vtbl))
        return NULL;

    return CONTAINING_RECORD(iface, struct uia_node, IWineUiaNode_iface);
}

static BOOL is_nested_node_provider(IWineUiaProvider *iface);
static HRESULT prepare_uia_node(struct uia_node *node)
{
    int i, prov_idx;
    HRESULT hr;

    /* Get the provider index for the provider that created the node. */
    for (i = prov_idx = 0; i < PROV_TYPE_COUNT; i++)
    {
        if (i == node->creator_prov_type)
        {
            node->creator_prov_idx = prov_idx;
            break;
        }
        else if (node->prov[i])
            prov_idx++;
    }

    /*
     * HUIANODEs can only have one 'parent link' provider, which handles
     * parent and sibling navigation for the entire HUIANODE. Each provider is
     * queried for a parent in descending order, starting with the override
     * provider. The first provider to have a valid parent is made parent
     * link. If no providers return a valid parent, the provider at index 0
     * is made parent link by default.
     */
    for (i = prov_idx = 0; i < PROV_TYPE_COUNT; i++)
    {
        BOOL has_parent;

        if (!node->prov[i])
            continue;

        hr = get_has_parent_from_node_provider(&node->IWineUiaNode_iface, prov_idx, &has_parent);
        if (SUCCEEDED(hr) && has_parent)
        {
            node->parent_link_idx = prov_idx;
            break;
        }

        prov_idx++;
    }

    for (i = 0; i < PROV_TYPE_COUNT; i++)
    {
        enum ProviderOptions prov_opts;
        IGlobalInterfaceTable *git;
        struct uia_provider *prov;
        HRESULT hr;

        /* Only regular providers need to be queried for UseComThreading. */
        if (!node->prov[i] || is_nested_node_provider(node->prov[i]))
            continue;

        prov = impl_from_IWineUiaProvider(node->prov[i]);
        hr = IRawElementProviderSimple_get_ProviderOptions(prov->elprov, &prov_opts);
        if (FAILED(hr))
            continue;

        /*
        * If the UseComThreading ProviderOption is specified, all calls to the
        * provided IRawElementProviderSimple need to respect the apartment type
        * of the thread that creates the HUIANODE. i.e, if it's created in an
        * STA, and the HUIANODE is used in an MTA, we need to provide a proxy.
        */
        if (prov_opts & ProviderOptions_UseComThreading)
        {
            hr = get_global_interface_table(&git);
            if (FAILED(hr))
                return hr;

            hr = IGlobalInterfaceTable_RegisterInterfaceInGlobal(git, (IUnknown *)&prov->IWineUiaProvider_iface,
                    &IID_IWineUiaProvider, &node->git_cookie[i]);
            if (FAILED(hr))
                return hr;
        }
    }

    return S_OK;
}

static BOOL node_creator_is_parent_link(struct uia_node *node)
{
    if (node->creator_prov_idx == node->parent_link_idx)
        return TRUE;
    else
        return FALSE;
}

static HRESULT get_sibling_from_node_provider(struct uia_node *node, int prov_idx, int nav_dir,
        VARIANT *out_node)
{
    HUIANODE tmp_node;
    HRESULT hr;
    VARIANT v;

    hr = get_navigate_from_node_provider(&node->IWineUiaNode_iface, prov_idx, nav_dir, &v);
    if (FAILED(hr))
        return hr;

    hr = UiaHUiaNodeFromVariant(&v, &tmp_node);
    if (FAILED(hr))
        goto exit;

    while (1)
    {
        struct uia_node *node_data = impl_from_IWineUiaNode((IWineUiaNode *)tmp_node);

        /*
         * If our sibling provider is the parent link of it's HUIANODE, then
         * it is a valid sibling of this node.
         */
        if (node_creator_is_parent_link(node_data))
            break;

        /*
         * If our sibling provider is not the parent link of it's HUIANODE, we
         * need to try the next sibling.
         */
        hr = get_navigate_from_node_provider((IWineUiaNode *)tmp_node, node_data->creator_prov_idx, nav_dir, &v);
        UiaNodeRelease(tmp_node);
        if (FAILED(hr))
            return hr;

        tmp_node = NULL;
        hr = UiaHUiaNodeFromVariant(&v, &tmp_node);
        if (FAILED(hr))
            break;
    }

exit:
    if (tmp_node)
        *out_node = v;

    return S_OK;
}

static HRESULT get_child_for_node(struct uia_node *node, int start_prov_idx, int nav_dir, VARIANT *out_node)
{
    int prov_idx = start_prov_idx;
    HUIANODE tmp_node = NULL;
    HRESULT hr;
    VARIANT v;

    while ((prov_idx >= 0) && (prov_idx < node->prov_count))
    {
        struct uia_node *node_data;

        hr = get_navigate_from_node_provider(&node->IWineUiaNode_iface, prov_idx, nav_dir, &v);
        if (FAILED(hr))
            return hr;

        if (nav_dir == NavigateDirection_FirstChild)
            prov_idx--;
        else
            prov_idx++;

        /* If we failed to get a child, try the next provider. */
        hr = UiaHUiaNodeFromVariant(&v, &tmp_node);
        if (FAILED(hr))
            continue;

        node_data = impl_from_IWineUiaNode((IWineUiaNode *)tmp_node);

        /* This child is the parent link of its node, so we can return it. */
        if (node_creator_is_parent_link(node_data))
            break;

        /*
         * If the first child provider isn't the parent link of it's HUIANODE,
         * we need to check the child provider for any valid siblings.
         */
        if (nav_dir == NavigateDirection_FirstChild)
            hr = get_sibling_from_node_provider(node_data, node_data->creator_prov_idx,
                    NavigateDirection_NextSibling, &v);
        else
            hr = get_sibling_from_node_provider(node_data, node_data->creator_prov_idx,
                    NavigateDirection_PreviousSibling, &v);

        UiaNodeRelease(tmp_node);
        if (FAILED(hr))
            return hr;

        /* If we got a valid sibling from the child provider, return it. */
        hr = UiaHUiaNodeFromVariant(&v, &tmp_node);
        if (SUCCEEDED(hr))
            break;
    }

    if (tmp_node)
        *out_node = v;

    return S_OK;
}

static HRESULT navigate_uia_node(struct uia_node *node, int nav_dir, HUIANODE *out_node)
{
    HRESULT hr;
    VARIANT v;

    *out_node = NULL;

    VariantInit(&v);
    switch (nav_dir)
    {
    case NavigateDirection_FirstChild:
    case NavigateDirection_LastChild:
        /* First child always comes from last provider index. */
        if (nav_dir == NavigateDirection_FirstChild)
            hr = get_child_for_node(node, node->prov_count - 1, nav_dir, &v);
        else
            hr = get_child_for_node(node, 0, nav_dir, &v);
        if (FAILED(hr))
            WARN("Child navigation failed with hr %#lx\n", hr);
        break;

    case NavigateDirection_NextSibling:
    case NavigateDirection_PreviousSibling:
    {
        struct uia_node *node_data;
        HUIANODE parent;
        VARIANT tmp;

        hr = get_sibling_from_node_provider(node, node->parent_link_idx, nav_dir, &v);
        if (FAILED(hr))
        {
            WARN("Sibling navigation failed with hr %#lx\n", hr);
            break;
        }

        if (V_VT(&v) != VT_EMPTY)
            break;

        hr = get_navigate_from_node_provider(&node->IWineUiaNode_iface, node->parent_link_idx,
                NavigateDirection_Parent, &tmp);
        if (FAILED(hr))
        {
            WARN("Parent navigation failed with hr %#lx\n", hr);
            break;
        }

        hr = UiaHUiaNodeFromVariant(&tmp, &parent);
        if (FAILED(hr))
            break;

        /*
         * If the parent node has multiple providers, attempt to get a sibling
         * from one of them.
         */
        node_data = impl_from_IWineUiaNode((IWineUiaNode *)parent);
        if (node_data->prov_count > 1)
        {
            if (nav_dir == NavigateDirection_NextSibling)
                hr = get_child_for_node(node_data, node_data->creator_prov_idx - 1, NavigateDirection_FirstChild, &v);
            else
                hr = get_child_for_node(node_data, node_data->creator_prov_idx + 1, NavigateDirection_LastChild, &v);
        }

        UiaNodeRelease(parent);
        break;
    }

    case NavigateDirection_Parent:
        hr = get_navigate_from_node_provider(&node->IWineUiaNode_iface, node->parent_link_idx, nav_dir, &v);
        if (FAILED(hr))
            WARN("Parent navigation failed with hr %#lx\n", hr);
        break;

    default:
        WARN("Invalid NavigateDirection %d\n", nav_dir);
        return E_INVALIDARG;
    }

    if (V_VT(&v) != VT_EMPTY)
    {
        hr = UiaHUiaNodeFromVariant(&v, (HUIANODE *)out_node);
        if (FAILED(hr))
            WARN("UiaHUiaNodeFromVariant failed with hr %#lx\n", hr);
    }

    return S_OK;
}

static HRESULT uia_condition_check(HUIANODE node, struct UiaCondition *condition);
static BOOL uia_condition_matched(HRESULT hr);

/*
 * Assuming we have a tree that looks like this:
 *                  +-------+
 *                  |       |
 *                  |   1   |
 *                  |       |
 *                  +---+---+
 *                      |
 *          +-----------+-----------+
 *          |           |           |
 *      +---+---+   +---+---+   +---+---+
 *      |       |   |       |   |       |
 *      |   2   +---|   3   +---+   4   |
 *      |       |   |       |   |       |
 *      +---+---+   +-------+   +-------+
 *          |
 *      +---+---+
 *      |       |
 *      |   5   |
 *      |       |
 *      +-------+
 * If we start navigation of the tree from node 1, our visit order for a
 * depth first search would be 1 -> 2 -> 5 -> 3 -> 4.
 *
 * However, if we start from the middle of the sequence at node 5 without the
 * prior context of navigating from nodes 1 and 2, we need to use the function
 * below to reach node 3, so we can visit the nodes within the tree in the
 * same order of 5 -> 3 -> 4.
 */
static HRESULT traverse_uia_node_tree_siblings(HUIANODE huianode, struct UiaCondition *ascending_stop_cond,
        int dir, BOOL at_root_level, HUIANODE *out_node)
{
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);
    HUIANODE node2 = NULL;
    HRESULT hr;

    *out_node = NULL;

    IWineUiaNode_AddRef(&node->IWineUiaNode_iface);
    while (1)
    {
        hr = navigate_uia_node(node, dir, &node2);
        if (FAILED(hr) || node2 || !at_root_level)
            break;

        hr = navigate_uia_node(node, NavigateDirection_Parent, &node2);
        if (FAILED(hr) || !node2)
            break;

        hr = uia_condition_check(node2, ascending_stop_cond);
        if (FAILED(hr) || uia_condition_matched(hr))
        {
            UiaNodeRelease(node2);
            node2 = NULL;
            break;
        }

        IWineUiaNode_Release(&node->IWineUiaNode_iface);
        node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)node2);
    }

    IWineUiaNode_Release(&node->IWineUiaNode_iface);
    *out_node = node2;

    return hr;
}

/*
 * This function is used to traverse the 'raw' tree of HUIANODEs using a
 * depth-first search. As each node in the tree is visited, it is checked
 * against two conditions.
 *
 * The first is the view condition, which is a filter for what is considered to
 * be a valid node in the current tree 'view'. The view condition essentially
 * creates a sort of 'virtual' tree. UI Automation provides three default tree
 * views:
 * -The 'raw' view has the view condition as ConditionType_True, so the tree
 *  is completely unfiltered. All nodes are valid.
 * -The 'control' view contains only nodes that do not return VARIANT_FALSE
 *  for UIA_IsControlElementPropertyId.
 * -The 'content' view contains only nodes that do not return VARIANT_FALSE
 *  for both UIA_IsControlElementPropertyId and UIA_IsContentElementPropertyId.
 *
 * If the currently visited node is considered to be valid within our view
 * condition, it is then checked against the search condition to determine if
 * it should be returned.
 */
static HRESULT traverse_uia_node_tree(HUIANODE huianode, struct UiaCondition *view_cond,
        struct UiaCondition *search_cond, struct UiaCondition *pre_sibling_nav_stop_cond,
        struct UiaCondition *ascending_stop_cond, int traversal_opts, BOOL at_root_level, BOOL find_first,
        BOOL *root_found, int max_depth, int *cur_depth, struct uia_node_array *out_nodes)
{
    HUIANODE node = huianode;
    HRESULT hr;

    while (1)
    {
        struct uia_node *node_data = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)node);
        BOOL incr_depth = FALSE;
        HUIANODE node2 = NULL;

        hr = uia_condition_check(node, view_cond);
        if (FAILED(hr))
            break;

        if (uia_condition_matched(hr))
        {
            /*
             * If this is a valid node within our treeview, we need to increment
             * our current depth within the tree.
             */
            incr_depth = TRUE;

            if (*root_found)
            {
                hr = uia_condition_check(node, search_cond);
                if (FAILED(hr))
                    break;

                if (uia_condition_matched(hr))
                {
                    hr = add_node_to_node_array(out_nodes, node);
                    if (FAILED(hr))
                        break;

                    if (find_first)
                    {
                        hr = S_FALSE;
                        break;
                    }
                }
            }

            *root_found = TRUE;
        }

        if (incr_depth)
            (*cur_depth)++;

        /* If we haven't exceeded our maximum traversal depth, visit children of this node. */
        if (max_depth < 0 || (*cur_depth) <= max_depth)
        {
            if (traversal_opts & TreeTraversalOptions_LastToFirstOrder)
                hr = navigate_uia_node(node_data, NavigateDirection_LastChild, &node2);
            else
                hr = navigate_uia_node(node_data, NavigateDirection_FirstChild, &node2);

            if (SUCCEEDED(hr) && node2)
                hr = traverse_uia_node_tree(node2, view_cond, search_cond, pre_sibling_nav_stop_cond, ascending_stop_cond,
                        traversal_opts, FALSE, find_first, root_found, max_depth, cur_depth, out_nodes);

            if (FAILED(hr) || hr == S_FALSE)
                break;
        }

        if (incr_depth)
            (*cur_depth)--;

        /*
         * Before attempting to visit any siblings of huianode, make sure
         * huianode doesn't match our stop condition.
         */
        hr = uia_condition_check(node, pre_sibling_nav_stop_cond);
        if (FAILED(hr) || uia_condition_matched(hr))
            break;

        /* Now, check for any siblings to visit. */
        if (traversal_opts & TreeTraversalOptions_LastToFirstOrder)
            hr = traverse_uia_node_tree_siblings(node, ascending_stop_cond, NavigateDirection_PreviousSibling,
                    at_root_level, &node2);
        else
            hr = traverse_uia_node_tree_siblings(node, ascending_stop_cond, NavigateDirection_NextSibling,
                    at_root_level, &node2);

        if (FAILED(hr) || !node2)
            break;

        UiaNodeRelease(node);
        node = node2;
    }

    UiaNodeRelease(node);

    return hr;
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

static HRESULT get_variant_for_elprov_node(IRawElementProviderSimple *elprov, BOOL out_nested,
        VARIANT *v)
{
    HUIANODE node;
    HRESULT hr;

    VariantInit(v);
    hr = create_uia_node_from_elprov(elprov, &node, !out_nested);
    IRawElementProviderSimple_Release(elprov);
    if (SUCCEEDED(hr))
    {
        if (out_nested)
        {
            LRESULT lr = uia_lresult_from_node(node);

            if (!lr)
                return E_FAIL;

            V_VT(v) = VT_I4;
            V_I4(v) = lr;
        }
        else
            get_variant_for_node(node, v);
    }

    return S_OK;
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

        hr = get_variant_for_elprov_node(elprov, prov->return_nested_node, ret_val);
        if (FAILED(hr))
            return hr;

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

static HRESULT WINAPI uia_provider_get_prov_opts(IWineUiaProvider *iface, int *out_opts)
{
    struct uia_provider *prov = impl_from_IWineUiaProvider(iface);
    enum ProviderOptions prov_opts;
    HRESULT hr;

    TRACE("%p, %p\n", iface, out_opts);

    *out_opts = 0;
    hr = IRawElementProviderSimple_get_ProviderOptions(prov->elprov, &prov_opts);
    if (SUCCEEDED(hr))
        *out_opts = prov_opts;

    return S_OK;
}

static HRESULT WINAPI uia_provider_has_parent(IWineUiaProvider *iface, BOOL *out_val)
{
    struct uia_provider *prov = impl_from_IWineUiaProvider(iface);

    TRACE("%p, %p\n", iface, out_val);

    if (!prov->parent_check_ran)
    {
        IRawElementProviderFragment *elfrag, *elfrag2;
        HRESULT hr;

        prov->has_parent = FALSE;
        hr = IRawElementProviderSimple_QueryInterface(prov->elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
        if (SUCCEEDED(hr) && elfrag)
        {
            hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
            IRawElementProviderFragment_Release(elfrag);
            if (SUCCEEDED(hr) && elfrag2)
            {
                prov->has_parent = TRUE;
                IRawElementProviderFragment_Release(elfrag2);
            }
        }

        prov->parent_check_ran = TRUE;
    }

    *out_val = prov->has_parent;

    return S_OK;
}

static HRESULT WINAPI uia_provider_navigate(IWineUiaProvider *iface, int nav_dir, VARIANT *out_val)
{
    struct uia_provider *prov = impl_from_IWineUiaProvider(iface);
    IRawElementProviderFragment *elfrag, *elfrag2;
    HRESULT hr;

    TRACE("%p, %d, %p\n", iface, nav_dir, out_val);

    VariantInit(out_val);
    hr = IRawElementProviderSimple_QueryInterface(prov->elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    if (FAILED(hr) || !elfrag)
        return S_OK;

    hr = IRawElementProviderFragment_Navigate(elfrag, nav_dir, &elfrag2);
    IRawElementProviderFragment_Release(elfrag);
    if (SUCCEEDED(hr) && elfrag2)
    {
        IRawElementProviderSimple *elprov;

        hr = IRawElementProviderFragment_QueryInterface(elfrag2, &IID_IRawElementProviderSimple, (void **)&elprov);
        IRawElementProviderFragment_Release(elfrag2);
        if (FAILED(hr) || !elprov)
            return hr;

        hr = get_variant_for_elprov_node(elprov, prov->return_nested_node, out_val);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

static const IWineUiaProviderVtbl uia_provider_vtbl = {
    uia_provider_QueryInterface,
    uia_provider_AddRef,
    uia_provider_Release,
    uia_provider_get_prop_val,
    uia_provider_get_prov_opts,
    uia_provider_has_parent,
    uia_provider_navigate,
};

static HRESULT create_wine_uia_provider(struct uia_node *node, IRawElementProviderSimple *elprov,
        int prov_type)
{
    struct uia_provider *prov = heap_alloc_zero(sizeof(*prov));

    if (!prov)
        return E_OUTOFMEMORY;

    prov->IWineUiaProvider_iface.lpVtbl = &uia_provider_vtbl;
    prov->elprov = elprov;
    prov->ref = 1;
    node->prov[prov_type] = &prov->IWineUiaProvider_iface;
    if (!node->prov_count)
        node->creator_prov_type = prov_type;
    node->prov_count++;

    IRawElementProviderSimple_AddRef(elprov);
    return S_OK;
}

static HRESULT uia_get_providers_for_hwnd(struct uia_node *node);
HRESULT create_uia_node_from_elprov(IRawElementProviderSimple *elprov, HUIANODE *out_node,
        BOOL get_hwnd_providers)
{
    static const int unsupported_prov_opts = ProviderOptions_ProviderOwnsSetFocus | ProviderOptions_HasNativeIAccessible |
        ProviderOptions_UseClientCoordinates;
    enum ProviderOptions prov_opts;
    struct uia_node *node;
    int prov_type;
    HRESULT hr;

    *out_node = NULL;

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opts);
    if (FAILED(hr))
        return hr;

    if (prov_opts & unsupported_prov_opts)
        FIXME("Ignoring unsupported ProviderOption(s) %#x\n", prov_opts & unsupported_prov_opts);

    if (prov_opts & ProviderOptions_OverrideProvider)
        prov_type = PROV_TYPE_OVERRIDE;
    else if (prov_opts & ProviderOptions_NonClientAreaProvider)
        prov_type = PROV_TYPE_NONCLIENT;
    else if (prov_opts & ProviderOptions_ServerSideProvider)
        prov_type = PROV_TYPE_MAIN;
    else if (prov_opts & ProviderOptions_ClientSideProvider)
        prov_type = PROV_TYPE_HWND;
    else
        prov_type = PROV_TYPE_MAIN;

    node = heap_alloc_zero(sizeof(*node));
    if (!node)
        return E_OUTOFMEMORY;

    node->IWineUiaNode_iface.lpVtbl = &uia_node_vtbl;
    node->hwnd = get_hwnd_from_provider(elprov);
    list_init(&node->prov_thread_list_entry);
    list_init(&node->node_map_list_entry);
    node->ref = 1;

    hr = create_wine_uia_provider(node, elprov, prov_type);
    if (FAILED(hr))
    {
        heap_free(node);
        return hr;
    }

    if (node->hwnd && get_hwnd_providers)
    {
        hr = uia_get_providers_for_hwnd(node);
        if (FAILED(hr))
            WARN("uia_get_providers_for_hwnd failed with hr %#lx\n", hr);
    }

    hr = prepare_uia_node(node);
    if (FAILED(hr))
    {
        IWineUiaNode_Release(&node->IWineUiaNode_iface);
        return hr;
    }

    *out_node = (void *)&node->IWineUiaNode_iface;

    return S_OK;
}

/***********************************************************************
 *          UiaNodeFromProvider (uiautomationcore.@)
 */
HRESULT WINAPI UiaNodeFromProvider(IRawElementProviderSimple *elprov, HUIANODE *huianode)
{
    TRACE("(%p, %p)\n", elprov, huianode);

    if (!elprov || !huianode)
        return E_INVALIDARG;

    return create_uia_node_from_elprov(elprov, huianode, TRUE);
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

static HRESULT WINAPI uia_nested_node_provider_get_prov_opts(IWineUiaProvider *iface, int *out_opts)
{
    struct uia_nested_node_provider *prov = impl_from_nested_node_IWineUiaProvider(iface);

    TRACE("%p, %p\n", iface, out_opts);

    return get_prov_opts_from_node_provider(prov->nested_node, 0, out_opts);
}

static HRESULT WINAPI uia_nested_node_provider_has_parent(IWineUiaProvider *iface, BOOL *out_val)
{
    struct uia_nested_node_provider *prov = impl_from_nested_node_IWineUiaProvider(iface);

    TRACE("%p, %p\n", iface, out_val);

    return get_has_parent_from_node_provider(prov->nested_node, 0, out_val);
}

static HRESULT WINAPI uia_nested_node_provider_navigate(IWineUiaProvider *iface, int nav_dir, VARIANT *out_val)
{
    struct uia_nested_node_provider *prov = impl_from_nested_node_IWineUiaProvider(iface);
    HUIANODE node;
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %d, %p\n", iface, nav_dir, out_val);

    VariantInit(out_val);
    hr = get_navigate_from_node_provider(prov->nested_node, 0, nav_dir, &v);
    if (FAILED(hr) || V_VT(&v) == VT_EMPTY)
        return hr;

    hr = uia_node_from_lresult((LRESULT)V_I4(&v), &node);
    if (FAILED(hr))
        return hr;

    get_variant_for_node(node, out_val);
    VariantClear(&v);

    return S_OK;
}

static const IWineUiaProviderVtbl uia_nested_node_provider_vtbl = {
    uia_nested_node_provider_QueryInterface,
    uia_nested_node_provider_AddRef,
    uia_nested_node_provider_Release,
    uia_nested_node_provider_get_prop_val,
    uia_nested_node_provider_get_prov_opts,
    uia_nested_node_provider_has_parent,
    uia_nested_node_provider_navigate,
};

static BOOL is_nested_node_provider(IWineUiaProvider *iface)
{
    if (iface->lpVtbl == &uia_nested_node_provider_vtbl)
        return TRUE;

    return FALSE;
}

static HRESULT create_wine_uia_nested_node_provider(struct uia_node *node, LRESULT lr,
        BOOL unwrap)
{
    IWineUiaProvider *provider_iface = NULL;
    struct uia_nested_node_provider *prov;
    IGlobalInterfaceTable *git;
    IWineUiaNode *nested_node;
    int prov_opts, prov_type;
    DWORD git_cookie;
    HRESULT hr;

    hr = ObjectFromLresult(lr, &IID_IWineUiaNode, 0, (void **)&nested_node);
    if (FAILED(hr))
    {
        uia_stop_client_thread();
        return hr;
    }

    hr = get_prov_opts_from_node_provider(nested_node, 0, &prov_opts);
    if (FAILED(hr))
    {
        WARN("Failed to get provider options for node %p with hr %#lx\n", nested_node, hr);
        IWineUiaNode_Release(nested_node);
        uia_stop_client_thread();
        return hr;
    }

    /* Nested nodes can only serve as override or main providers. */
    if (prov_opts & ProviderOptions_OverrideProvider)
        prov_type = PROV_TYPE_OVERRIDE;
    else
        prov_type = PROV_TYPE_MAIN;

    if (node->prov[prov_type])
    {
        TRACE("Already have a provider of type %d for this node.\n", prov_type);
        IWineUiaNode_Release(nested_node);
        uia_stop_client_thread();
        return S_OK;
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

        provider_iface = node_data->prov[get_node_provider_type_at_idx(node_data, 0)];
        git_cookie = 0;

        IWineUiaProvider_AddRef(provider_iface);
        prov_data = impl_from_IWineUiaProvider(provider_iface);
        prov_data->return_nested_node = FALSE;
        prov_data->parent_check_ran = FALSE;

        IWineUiaNode_Release(nested_node);
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

        if (!node->hwnd)
        {
            ULONG hwnd;

            hr = IWineUiaNode_get_hwnd(nested_node, &hwnd);
            if (SUCCEEDED(hr))
                node->hwnd = UlongToHandle(hwnd);
        }
    }

    node->prov[prov_type] = provider_iface;
    node->git_cookie[prov_type] = git_cookie;
    if (!node->prov_count)
        node->creator_prov_type = prov_type;
    node->prov_count++;

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

    if (node->hwnd)
    {
        hr = uia_get_providers_for_hwnd(node);
        if (FAILED(hr))
            WARN("uia_get_providers_for_hwnd failed with hr %#lx\n", hr);
    }

    hr = prepare_uia_node(node);
    if (FAILED(hr))
    {
        IWineUiaNode_Release(&node->IWineUiaNode_iface);
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
        uia_stop_client_thread();
        return S_FALSE;
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

    hr = uia_get_providers_for_hwnd(node);
    if (FAILED(hr))
    {
        heap_free(node);
        return hr;
    }

    hr = prepare_uia_node(node);
    if (FAILED(hr))
    {
        IWineUiaNode_Release(&node->IWineUiaNode_iface);
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
    HRESULT hr = S_OK;
    int i;

    for (i = 0; i < node->prov_count; i++)
    {
        hr = IWineUiaNode_get_provider(&node->IWineUiaNode_iface, i, &prov);
        if (FAILED(hr))
            return hr;

        VariantInit(v);
        hr = IWineUiaProvider_get_prop_val(prov, prop_info, v);
        IWineUiaProvider_Release(prov);
        if (FAILED(hr) || V_VT(v) != VT_EMPTY)
            break;
    }

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

static SAFEARRAY WINAPI *default_uia_provider_callback(HWND hwnd, enum ProviderType prov_type)
{
    switch (prov_type)
    {
    case ProviderType_Proxy:
        FIXME("Default ProviderType_Proxy MSAA provider unimplemented.\n");
        break;

    case ProviderType_NonClientArea:
        FIXME("Default ProviderType_NonClientArea provider unimplemented.\n");
        break;

    case ProviderType_BaseHwnd:
        FIXME("Default ProviderType_BaseHwnd provider unimplemented.\n");
        break;

    default:
        break;
    }

    return NULL;
}

static UiaProviderCallback *uia_provider_callback = default_uia_provider_callback;

static HRESULT uia_get_clientside_provider(struct uia_node *node, int prov_type,
        int node_prov_type)
{
    IRawElementProviderSimple *elprov;
    LONG lbound, elems;
    SAFEARRAY *sa;
    IUnknown *unk;
    VARTYPE vt;
    HRESULT hr;

    if (!(sa = uia_provider_callback(node->hwnd, prov_type)))
        return S_OK;

    hr = SafeArrayGetVartype(sa, &vt);
    if (FAILED(hr) || (vt != VT_UNKNOWN))
        goto exit;

    hr = get_safearray_bounds(sa, &lbound, &elems);
    if (FAILED(hr))
        goto exit;

    /* Returned SAFEARRAY can only have 1 element. */
    if (elems != 1)
    {
        WARN("Invalid element count %ld for returned SAFEARRAY\n", elems);
        goto exit;
    }

    hr = SafeArrayGetElement(sa, &lbound, &unk);
    if (FAILED(hr))
        goto exit;

    hr = IUnknown_QueryInterface(unk, &IID_IRawElementProviderSimple, (void **)&elprov);
    IUnknown_Release(unk);
    if (FAILED(hr) || !elprov)
    {
        WARN("Failed to get IRawElementProviderSimple from returned SAFEARRAY.\n");
        hr = S_OK;
        goto exit;
    }

    hr = create_wine_uia_provider(node, elprov, node_prov_type);
    IRawElementProviderSimple_Release(elprov);

exit:
    if (FAILED(hr))
        WARN("Failed to get clientside provider, hr %#lx\n", hr);
    SafeArrayDestroy(sa);
    return hr;
}

static HRESULT uia_get_providers_for_hwnd(struct uia_node *node)
{
    HRESULT hr;

    hr = uia_get_provider_from_hwnd(node);
    if (FAILED(hr))
        return hr;

    if (!node->prov[PROV_TYPE_MAIN])
    {
        hr = uia_get_clientside_provider(node, ProviderType_Proxy, PROV_TYPE_MAIN);
        if (FAILED(hr))
            return hr;
    }

    if (!node->prov[PROV_TYPE_OVERRIDE])
        FIXME("Override provider callback currently unimplemented.\n");

    if (!node->prov[PROV_TYPE_NONCLIENT])
    {
        hr = uia_get_clientside_provider(node, ProviderType_NonClientArea, PROV_TYPE_NONCLIENT);
        if (FAILED(hr))
            return hr;
    }

    if (!node->prov[PROV_TYPE_HWND])
    {
        hr = uia_get_clientside_provider(node, ProviderType_BaseHwnd, PROV_TYPE_HWND);
        if (FAILED(hr))
            return hr;
    }

    if (!node->prov_count)
    {
        if (uia_provider_callback == default_uia_provider_callback)
            return E_NOTIMPL;
        else
            return E_FAIL;
    }

    return S_OK;
}

/***********************************************************************
 *          UiaRegisterProviderCallback (uiautomationcore.@)
 */
void WINAPI UiaRegisterProviderCallback(UiaProviderCallback *callback)
{
    TRACE("(%p)\n", callback);

    if (callback)
        uia_provider_callback = callback;
    else
        uia_provider_callback = default_uia_provider_callback;
}

static BOOL uia_condition_matched(HRESULT hr)
{
    if (hr == S_FALSE)
        return FALSE;
    else
        return TRUE;
}

static HRESULT uia_property_condition_check(HUIANODE node, struct UiaPropertyCondition *prop_cond)
{
    const struct uia_prop_info *prop_info = uia_prop_info_from_id(prop_cond->PropertyId);
    HRESULT hr;
    VARIANT v;

    if (!prop_info)
        return E_INVALIDARG;

    switch (prop_info->type)
    {
    case UIAutomationType_Bool:
    case UIAutomationType_IntArray:
        break;

    default:
        FIXME("PropertyCondition comparison unimplemented for type %#x\n", prop_info->type);
        return E_NOTIMPL;
    }

    hr = UiaGetPropertyValue(node, prop_info->prop_id, &v);
    if (FAILED(hr) || V_VT(&v) == VT_UNKNOWN)
        return S_FALSE;

    if (V_VT(&v) == V_VT(&prop_cond->Value))
    {
        switch (prop_info->type)
        {
        case UIAutomationType_Bool:
            if (V_BOOL(&v) == V_BOOL(&prop_cond->Value))
                hr = S_OK;
            else
                hr = S_FALSE;
            break;

        case UIAutomationType_IntArray:
            if (!uia_compare_safearrays(V_ARRAY(&v), V_ARRAY(&prop_cond->Value), prop_info->type))
                hr = S_OK;
            else
                hr = S_FALSE;
            break;

        default:
            break;
        }
    }
    else
        hr = S_FALSE;

    VariantClear(&v);
    return hr;
}

static HRESULT uia_condition_check(HUIANODE node, struct UiaCondition *condition)
{
    HRESULT hr;

    switch (condition->ConditionType)
    {
    case ConditionType_True:
        return S_OK;

    case ConditionType_False:
        return S_FALSE;

    case ConditionType_Not:
    {
        struct UiaNotCondition *not_cond = (struct UiaNotCondition *)condition;

        hr = uia_condition_check(node, not_cond->pConditions);
        if (FAILED(hr))
            return hr;

        if (uia_condition_matched(hr))
            return S_FALSE;
        else
            return S_OK;
    }

    case ConditionType_And:
    case ConditionType_Or:
    {
        struct UiaAndOrCondition *and_or_cond = (struct UiaAndOrCondition *)condition;
        int i;

        for (i = 0; i < and_or_cond->cConditions; i++)
        {
            hr = uia_condition_check(node, and_or_cond->ppConditions[i]);
            if (FAILED(hr))
                return hr;

            if (condition->ConditionType == ConditionType_And && !uia_condition_matched(hr))
                return S_FALSE;
            else if (condition->ConditionType == ConditionType_Or && uia_condition_matched(hr))
                return S_OK;
        }

        if (condition->ConditionType == ConditionType_Or)
            return S_FALSE;
        else
            return S_OK;
    }

    case ConditionType_Property:
        return uia_property_condition_check(node, (struct UiaPropertyCondition *)condition);

    default:
        WARN("Invalid condition type %d\n", condition->ConditionType);
        return E_INVALIDARG;
    }
}

/***********************************************************************
 *          UiaGetUpdatedCache (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetUpdatedCache(HUIANODE huianode, struct UiaCacheRequest *cache_req, enum NormalizeState normalize_state,
        struct UiaCondition *normalize_cond, SAFEARRAY **out_req, BSTR *tree_struct)
{
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);
    struct UiaCondition *cond;
    SAFEARRAYBOUND sabound[2];
    SAFEARRAY *sa;
    LONG idx[2];
    HRESULT hr;
    VARIANT v;

    TRACE("(%p, %p, %u, %p, %p, %p)\n", huianode, cache_req, normalize_state, normalize_cond, out_req, tree_struct);

    if (!node || !out_req || !tree_struct || !cache_req)
        return E_INVALIDARG;

    *tree_struct = NULL;
    *out_req = NULL;

    if (cache_req->Scope != TreeScope_Element)
    {
        FIXME("Unsupported cache request scope %#x\n", cache_req->Scope);
        return E_NOTIMPL;
    }

    switch (normalize_state)
    {
    case NormalizeState_None:
        cond = NULL;
        break;

    case NormalizeState_View:
        cond = cache_req->pViewCondition;
        break;

    case NormalizeState_Custom:
        cond = normalize_cond;
        break;

    default:
        WARN("Invalid normalize_state %d\n", normalize_state);
        return E_INVALIDARG;
    }

    if (cond)
    {
        hr = uia_condition_check(huianode, cond);
        if (FAILED(hr))
            return hr;

        if (!uia_condition_matched(hr))
        {
            *tree_struct = SysAllocString(L"");
            return S_OK;
        }
    }

    sabound[0].cElements = sabound[1].cElements = 1;
    sabound[0].lLbound = sabound[1].lLbound = 0;
    if (!(sa = SafeArrayCreate(VT_VARIANT, 2, sabound)))
    {
        WARN("Failed to create safearray\n");
        return E_FAIL;
    }

    get_variant_for_node(huianode, &v);
    idx[0] = idx[1] = 0;

    hr = SafeArrayPutElement(sa, idx, &v);
    if (FAILED(hr))
    {
        SafeArrayDestroy(sa);
        return hr;
    }

    /*
     * AddRef huianode since we're returning a reference to the same node we
     * passed in, rather than creating a new one.
     */
    IWineUiaNode_AddRef(&node->IWineUiaNode_iface);

    *out_req = sa;
    *tree_struct = SysAllocString(L"P)");

    return S_OK;
}

/***********************************************************************
 *          UiaNavigate (uiautomationcore.@)
 */
HRESULT WINAPI UiaNavigate(HUIANODE huianode, enum NavigateDirection dir, struct UiaCondition *nav_condition,
        struct UiaCacheRequest *cache_req, SAFEARRAY **out_req, BSTR *tree_struct)
{
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);
    HUIANODE node2;
    HRESULT hr;

    TRACE("(%p, %u, %p, %p, %p, %p)\n", huianode, dir, nav_condition, cache_req, out_req,
            tree_struct);

    if (!node || !nav_condition || !cache_req || !out_req || !tree_struct)
        return E_INVALIDARG;

    *out_req = NULL;
    *tree_struct = NULL;

    if (nav_condition->ConditionType != ConditionType_True)
    {
        FIXME("ConditionType %d based navigation is not implemented.\n", nav_condition->ConditionType);
        return E_NOTIMPL;
    }

    hr = navigate_uia_node(node, dir, &node2);
    if (FAILED(hr))
        return hr;

    if (node2)
    {
        hr = UiaGetUpdatedCache(node2, cache_req, NormalizeState_None, NULL, out_req, tree_struct);
        if (FAILED(hr))
            WARN("UiaGetUpdatedCache failed with hr %#lx\n", hr);
        UiaNodeRelease(node2);
    }

    return hr;
}

/* Combine multiple cache requests into a single SAFEARRAY. */
static HRESULT uia_cache_request_combine(SAFEARRAY **reqs, int reqs_count, SAFEARRAY *out_req)
{
    LONG idx[2], lbound[2], elems[2], cur_offset;
    int i, x, y;
    HRESULT hr;
    VARIANT v;

    for (i = cur_offset = 0; i < reqs_count; i++)
    {
        if (!reqs[i])
            continue;

        for (x = 0; x < 2; x++)
        {
            hr = get_safearray_dim_bounds(reqs[i], 1 + x, &lbound[x], &elems[x]);
            if (FAILED(hr))
                return hr;
        }

        for (x = 0; x < elems[0]; x++)
        {
            for (y = 0; y < elems[1]; y++)
            {
                idx[0] = x + lbound[0];
                idx[1] = y + lbound[1];
                hr = SafeArrayGetElement(reqs[i], idx, &v);
                if (FAILED(hr))
                    return hr;

                idx[0] = x + cur_offset;
                idx[1] = y;
                hr = SafeArrayPutElement(out_req, idx, &v);
                if (FAILED(hr))
                    return hr;
            }
        }

        cur_offset += elems[0];
    }

    return S_OK;
}

/***********************************************************************
 *          UiaFind (uiautomationcore.@)
 */
HRESULT WINAPI UiaFind(HUIANODE huianode, struct UiaFindParams *find_params, struct UiaCacheRequest *cache_req,
        SAFEARRAY **out_req, SAFEARRAY **out_offsets, SAFEARRAY **out_tree_structs)
{
    struct UiaPropertyCondition prop_cond = { ConditionType_Property, UIA_RuntimeIdPropertyId };
    struct uia_node *node = unsafe_impl_from_IWineUiaNode((IWineUiaNode *)huianode);
    SAFEARRAY *runtime_id, *req, *offsets, *tree_structs, **tmp_reqs;
    struct UiaCondition *sibling_stop_cond;
    struct uia_node_array nodes = { 0 };
    LONG idx, lbound, elems, cur_offset;
    SAFEARRAYBOUND sabound[2];
    int i, cur_depth = 0;
    BSTR tree_struct;
    BOOL root_found;
    HRESULT hr;

    TRACE("(%p, %p, %p, %p, %p, %p)\n", huianode, find_params, cache_req, out_req, out_offsets, out_tree_structs);

    if (!node || !find_params || !cache_req || !out_req || !out_offsets || !out_tree_structs)
        return E_INVALIDARG;

    *out_tree_structs = *out_offsets = *out_req = tree_structs = offsets = req = NULL;
    tmp_reqs = NULL;

    /*
     * If the initial node has a runtime ID, we'll use it as a stop
     * condition.
     */
    hr = UiaGetRuntimeId(huianode, &runtime_id);
    if (SUCCEEDED(hr) && runtime_id)
    {
        V_VT(&prop_cond.Value) = VT_I4 | VT_ARRAY;
        V_ARRAY(&prop_cond.Value) = runtime_id;
        sibling_stop_cond = (struct UiaCondition *)&prop_cond;
    }
    else
        sibling_stop_cond =  (struct UiaCondition *)&UiaFalseCondition;

    if (find_params->ExcludeRoot)
        root_found = FALSE;
    else
        root_found = TRUE;

    IWineUiaNode_AddRef(&node->IWineUiaNode_iface);
    hr = traverse_uia_node_tree(huianode, cache_req->pViewCondition, find_params->pFindCondition, sibling_stop_cond,
            cache_req->pViewCondition, TreeTraversalOptions_Default, TRUE, find_params->FindFirst, &root_found,
            find_params->MaxDepth, &cur_depth, &nodes);
    if (FAILED(hr) || !nodes.node_count)
        goto exit;

    if (!(offsets = SafeArrayCreateVector(VT_I4, 0, nodes.node_count)))
    {
        hr = E_FAIL;
        goto exit;
    }

    if (!(tree_structs = SafeArrayCreateVector(VT_BSTR, 0, nodes.node_count)))
    {
        hr = E_FAIL;
        goto exit;
    }

    if (!(tmp_reqs = heap_alloc_zero(sizeof(*tmp_reqs) * nodes.node_count)))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    /*
     * Get a count of how many total nodes we'll need to return, as well as
     * set the tree structure strings and cache request offsets for our final
     * combined SAFEARRAY.
     */
    for (i = cur_offset = 0; i < nodes.node_count; i++)
    {
        hr = UiaGetUpdatedCache(nodes.nodes[i], cache_req, NormalizeState_None, NULL, &tmp_reqs[i], &tree_struct);
        if (FAILED(hr))
            goto exit;

        idx = i;
        hr = SafeArrayPutElement(tree_structs, &idx, tree_struct);
        SysFreeString(tree_struct);
        if (FAILED(hr))
            goto exit;

        hr = SafeArrayPutElement(offsets, &idx, &cur_offset);
        if (FAILED(hr))
            goto exit;

        if (!tmp_reqs[i])
            continue;

        hr = get_safearray_dim_bounds(tmp_reqs[i], 1, &lbound, &elems);
        if (FAILED(hr))
            goto exit;

        cur_offset += elems;
    }

    if (nodes.node_count == 1)
    {
        req = tmp_reqs[0];
        heap_free(tmp_reqs);
        tmp_reqs = NULL;
    }
    else
    {
        sabound[0].lLbound = sabound[1].lLbound = 0;
        sabound[0].cElements = cur_offset;
        sabound[1].cElements = 1 + cache_req->cProperties + cache_req->cPatterns;
        if (!(req = SafeArrayCreate(VT_VARIANT, 2, sabound)))
        {
            hr = E_FAIL;
            goto exit;
        }

        hr = uia_cache_request_combine(tmp_reqs, nodes.node_count, req);
        if (FAILED(hr))
            goto exit;
    }

    *out_tree_structs = tree_structs;
    *out_offsets = offsets;
    *out_req = req;

exit:
    VariantClear(&prop_cond.Value);
    clear_node_array(&nodes);

    if (tmp_reqs)
    {
        for (i = 0; i < nodes.node_count; i++)
            SafeArrayDestroy(tmp_reqs[i]);
        heap_free(tmp_reqs);
    }

    if (FAILED(hr))
    {
        SafeArrayDestroy(tree_structs);
        SafeArrayDestroy(offsets);
        SafeArrayDestroy(req);
    }

    return hr;
}
