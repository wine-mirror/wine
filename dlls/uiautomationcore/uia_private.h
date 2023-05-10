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

#define COBJMACROS

#include "uiautomation.h"
#include "uia_classes.h"
#include "wine/list.h"
#include "wine/heap.h"

extern HMODULE huia_module DECLSPEC_HIDDEN;

enum uia_prop_type {
    PROP_TYPE_UNKNOWN,
    PROP_TYPE_ELEM_PROP,
    PROP_TYPE_SPECIAL,
    PROP_TYPE_PATTERN_PROP,
};

/*
 * HUIANODEs that have an associated HWND are able to pull data from up to 4
 * different providers:
 *
 * - Override providers are used to override values from all other providers.
 * - Main providers are the base provider for an HUIANODE.
 * - Nonclient providers are used to represent the nonclient area of the HWND.
 * - HWND providers are used to represent data from the HWND as a whole, such
 *   as the bounding box.
 *
 * When a property is requested from the node, each provider is queried in
 * descending order starting with the override provider until either one
 * returns a property or there are no more providers to query.
 */
enum uia_node_prov_type {
    PROV_TYPE_OVERRIDE,
    PROV_TYPE_MAIN,
    PROV_TYPE_NONCLIENT,
    PROV_TYPE_HWND,
    PROV_TYPE_COUNT,
};

struct uia_node {
    IWineUiaNode IWineUiaNode_iface;
    LONG ref;

    IWineUiaProvider *prov[PROV_TYPE_COUNT];
    DWORD git_cookie[PROV_TYPE_COUNT];
    int prov_count;
    int parent_link_idx;
    int creator_prov_idx;

    HWND hwnd;
    BOOL nested_node;
    BOOL disconnected;
    int creator_prov_type;
    struct list prov_thread_list_entry;
    struct list node_map_list_entry;
    struct uia_provider_thread_map_entry *map;
};

static inline struct uia_node *impl_from_IWineUiaNode(IWineUiaNode *iface)
{
    return CONTAINING_RECORD(iface, struct uia_node, IWineUiaNode_iface);
}

struct uia_provider {
    IWineUiaProvider IWineUiaProvider_iface;
    LONG ref;

    IRawElementProviderSimple *elprov;
    BOOL return_nested_node;
    BOOL parent_check_ran;
    BOOL has_parent;
};

static inline struct uia_provider *impl_from_IWineUiaProvider(IWineUiaProvider *iface)
{
    return CONTAINING_RECORD(iface, struct uia_provider, IWineUiaProvider_iface);
}

struct uia_event
{
    IWineUiaEvent IWineUiaEvent_iface;
    LONG ref;

    SAFEARRAY *runtime_id;
    int event_id;
    int scope;

    IWineUiaEventAdviser **event_advisers;
    int event_advisers_count;
    SIZE_T event_advisers_arr_size;

    UiaEventCallback *cback;
};

static inline void variant_init_bool(VARIANT *v, BOOL val)
{
    V_VT(v) = VT_BOOL;
    V_BOOL(v) = val ? VARIANT_TRUE : VARIANT_FALSE;
}

static inline void variant_init_i4(VARIANT *v, int val)
{
    V_VT(v) = VT_I4;
    V_I4(v) = val;
}

static inline BOOL uia_array_reserve(void **elements, SIZE_T *capacity, SIZE_T count, SIZE_T size)
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

/* uia_client.c */
int get_node_provider_type_at_idx(struct uia_node *node, int idx) DECLSPEC_HIDDEN;
HRESULT attach_event_to_uia_node(HUIANODE node, struct uia_event *event) DECLSPEC_HIDDEN;
HRESULT create_uia_node_from_elprov(IRawElementProviderSimple *elprov, HUIANODE *out_node,
        BOOL get_hwnd_providers) DECLSPEC_HIDDEN;

/* uia_com_client.c */
HRESULT create_uia_iface(IUnknown **iface, BOOL is_cui8) DECLSPEC_HIDDEN;

/* uia_event.c */
HRESULT uia_event_add_provider_event_adviser(IRawElementProviderAdviseEvents *advise_events,
        struct uia_event *event) DECLSPEC_HIDDEN;

/* uia_ids.c */
const struct uia_prop_info *uia_prop_info_from_id(PROPERTYID prop_id) DECLSPEC_HIDDEN;
const struct uia_event_info *uia_event_info_from_id(EVENTID event_id) DECLSPEC_HIDDEN;
const struct uia_pattern_info *uia_pattern_info_from_id(PATTERNID pattern_id) DECLSPEC_HIDDEN;
const struct uia_control_type_info *uia_control_type_info_from_id(CONTROLTYPEID control_type_id) DECLSPEC_HIDDEN;

/* uia_provider.c */
HRESULT create_base_hwnd_provider(HWND hwnd, IRawElementProviderSimple **elprov) DECLSPEC_HIDDEN;
void uia_stop_provider_thread(void) DECLSPEC_HIDDEN;
void uia_provider_thread_remove_node(HUIANODE node) DECLSPEC_HIDDEN;
LRESULT uia_lresult_from_node(HUIANODE huianode) DECLSPEC_HIDDEN;
HRESULT create_msaa_provider(IAccessible *acc, long child_id, HWND hwnd, BOOL known_root_acc,
        IRawElementProviderSimple **elprov) DECLSPEC_HIDDEN;

/* uia_utils.c */
HRESULT register_interface_in_git(IUnknown *iface, REFIID riid, DWORD *ret_cookie) DECLSPEC_HIDDEN;
HRESULT unregister_interface_in_git(DWORD git_cookie) DECLSPEC_HIDDEN;
HRESULT get_interface_in_git(REFIID riid, DWORD git_cookie, IUnknown **ret_iface) DECLSPEC_HIDDEN;
HRESULT get_safearray_dim_bounds(SAFEARRAY *sa, UINT dim, LONG *lbound, LONG *elems) DECLSPEC_HIDDEN;
HRESULT get_safearray_bounds(SAFEARRAY *sa, LONG *lbound, LONG *elems) DECLSPEC_HIDDEN;
int uia_compare_safearrays(SAFEARRAY *sa1, SAFEARRAY *sa2, int prop_type) DECLSPEC_HIDDEN;
