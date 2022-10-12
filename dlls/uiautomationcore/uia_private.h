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

extern HMODULE huia_module DECLSPEC_HIDDEN;

enum uia_prop_type {
    PROP_TYPE_UNKNOWN,
    PROP_TYPE_ELEM_PROP,
    PROP_TYPE_SPECIAL,
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

    HWND hwnd;
    BOOL nested_node;
    BOOL disconnected;
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
};

static inline struct uia_provider *impl_from_IWineUiaProvider(IWineUiaProvider *iface)
{
    return CONTAINING_RECORD(iface, struct uia_provider, IWineUiaProvider_iface);
}

/* uia_client.c */
int uia_compare_runtime_ids(SAFEARRAY *sa1, SAFEARRAY *sa2) DECLSPEC_HIDDEN;
int get_node_provider_type_at_idx(struct uia_node *node, int idx) DECLSPEC_HIDDEN;
HRESULT create_uia_node_from_elprov(IRawElementProviderSimple *elprov, HUIANODE *out_node,
        BOOL get_hwnd_providers) DECLSPEC_HIDDEN;

/* uia_ids.c */
const struct uia_prop_info *uia_prop_info_from_id(PROPERTYID prop_id) DECLSPEC_HIDDEN;

/* uia_provider.c */
void uia_stop_provider_thread(void) DECLSPEC_HIDDEN;
void uia_provider_thread_remove_node(HUIANODE node) DECLSPEC_HIDDEN;
LRESULT uia_lresult_from_node(HUIANODE huianode) DECLSPEC_HIDDEN;
