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
 * IWineUiaEvent interface.
 */
static inline struct uia_event *impl_from_IWineUiaEvent(IWineUiaEvent *iface)
{
    return CONTAINING_RECORD(iface, struct uia_event, IWineUiaEvent_iface);
}

static HRESULT WINAPI uia_event_QueryInterface(IWineUiaEvent *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaEvent) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaEvent_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_event_AddRef(IWineUiaEvent *iface)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    ULONG ref = InterlockedIncrement(&event->ref);

    TRACE("%p, refcount %ld\n", event, ref);
    return ref;
}

static ULONG WINAPI uia_event_Release(IWineUiaEvent *iface)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    ULONG ref = InterlockedDecrement(&event->ref);

    TRACE("%p, refcount %ld\n", event, ref);
    if (!ref)
    {
        int i;

        SafeArrayDestroy(event->runtime_id);
        for (i = 0; i < event->event_advisers_count; i++)
            IWineUiaEventAdviser_Release(event->event_advisers[i]);
        heap_free(event->event_advisers);
        heap_free(event);
    }

    return ref;
}

static HRESULT WINAPI uia_event_advise_events(IWineUiaEvent *iface, BOOL advise_added)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    HRESULT hr;
    int i;

    TRACE("%p, %d\n", event, advise_added);

    for (i = 0; i < event->event_advisers_count; i++)
    {
        hr = IWineUiaEventAdviser_advise(event->event_advisers[i], advise_added, (UINT_PTR)event);
        if (FAILED(hr))
            return hr;
    }

    /* Once we've advised of removal, no need to keep the advisers around. */
    if (!advise_added)
    {
        for (i = 0; i < event->event_advisers_count; i++)
            IWineUiaEventAdviser_Release(event->event_advisers[i]);
        heap_free(event->event_advisers);
        event->event_advisers_count = event->event_advisers_arr_size = 0;
    }

    return S_OK;
}

static const IWineUiaEventVtbl uia_event_vtbl = {
    uia_event_QueryInterface,
    uia_event_AddRef,
    uia_event_Release,
    uia_event_advise_events,
};

static struct uia_event *unsafe_impl_from_IWineUiaEvent(IWineUiaEvent *iface)
{
    if (!iface || (iface->lpVtbl != &uia_event_vtbl))
        return NULL;

    return CONTAINING_RECORD(iface, struct uia_event, IWineUiaEvent_iface);
}

static HRESULT create_uia_event(struct uia_event **out_event, int event_id, int scope, UiaEventCallback *cback,
        SAFEARRAY *runtime_id)
{
    struct uia_event *event = heap_alloc_zero(sizeof(*event));

    *out_event = NULL;
    if (!event)
        return E_OUTOFMEMORY;

    event->IWineUiaEvent_iface.lpVtbl = &uia_event_vtbl;
    event->ref = 1;
    event->runtime_id = runtime_id;
    event->event_id = event_id;
    event->scope = scope;
    event->cback = cback;

    *out_event = event;
    return S_OK;
}

/*
 * IWineUiaEventAdviser interface.
 */
struct uia_event_adviser {
    IWineUiaEventAdviser IWineUiaEventAdviser_iface;
    LONG ref;

    IRawElementProviderAdviseEvents *advise_events;
    DWORD git_cookie;
};

static inline struct uia_event_adviser *impl_from_IWineUiaEventAdviser(IWineUiaEventAdviser *iface)
{
    return CONTAINING_RECORD(iface, struct uia_event_adviser, IWineUiaEventAdviser_iface);
}

static HRESULT WINAPI uia_event_adviser_QueryInterface(IWineUiaEventAdviser *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaEventAdviser) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaEventAdviser_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_event_adviser_AddRef(IWineUiaEventAdviser *iface)
{
    struct uia_event_adviser *adv_events = impl_from_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedIncrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    return ref;
}

static ULONG WINAPI uia_event_adviser_Release(IWineUiaEventAdviser *iface)
{
    struct uia_event_adviser *adv_events = impl_from_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedDecrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    if (!ref)
    {
        if (adv_events->git_cookie)
        {
            if (FAILED(unregister_interface_in_git(adv_events->git_cookie)))
                WARN("Failed to revoke advise events interface from GIT\n");
        }
        IRawElementProviderAdviseEvents_Release(adv_events->advise_events);
        heap_free(adv_events);
    }

    return ref;
}

static HRESULT WINAPI uia_event_adviser_advise(IWineUiaEventAdviser *iface, BOOL advise_added, LONG_PTR huiaevent)
{
    struct uia_event_adviser *adv_events = impl_from_IWineUiaEventAdviser(iface);
    struct uia_event *event_data = (struct uia_event *)huiaevent;
    IRawElementProviderAdviseEvents *advise_events;
    HRESULT hr;

    TRACE("%p, %d, %#Ix\n", adv_events, advise_added, huiaevent);

    if (adv_events->git_cookie)
    {
        hr = get_interface_in_git(&IID_IRawElementProviderAdviseEvents, adv_events->git_cookie,
                (IUnknown **)&advise_events);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        advise_events = adv_events->advise_events;
        IRawElementProviderAdviseEvents_AddRef(advise_events);
    }

    if (advise_added)
        hr = IRawElementProviderAdviseEvents_AdviseEventAdded(advise_events, event_data->event_id, NULL);
    else
        hr = IRawElementProviderAdviseEvents_AdviseEventRemoved(advise_events, event_data->event_id, NULL);

    IRawElementProviderAdviseEvents_Release(advise_events);
    return hr;
}

static const IWineUiaEventAdviserVtbl uia_event_adviser_vtbl = {
    uia_event_adviser_QueryInterface,
    uia_event_adviser_AddRef,
    uia_event_adviser_Release,
    uia_event_adviser_advise,
};

HRESULT uia_event_add_provider_event_adviser(IRawElementProviderAdviseEvents *advise_events, struct uia_event *event)
{
    struct uia_event_adviser *adv_events;
    IRawElementProviderSimple *elprov;
    enum ProviderOptions prov_opts;
    HRESULT hr;

    hr = IRawElementProviderAdviseEvents_QueryInterface(advise_events, &IID_IRawElementProviderSimple,
            (void **)&elprov);
    if (FAILED(hr))
    {
        ERR("Failed to get IRawElementProviderSimple from advise events\n");
        return E_FAIL;
    }

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opts);
    IRawElementProviderSimple_Release(elprov);
    if (FAILED(hr))
        return hr;

    if (!(adv_events = heap_alloc_zero(sizeof(*adv_events))))
        return E_OUTOFMEMORY;

    if (prov_opts & ProviderOptions_UseComThreading)
    {
        hr = register_interface_in_git((IUnknown *)advise_events, &IID_IRawElementProviderAdviseEvents,
                &adv_events->git_cookie);
        if (FAILED(hr))
        {
            heap_free(adv_events);
            return hr;
        }
    }

    adv_events->IWineUiaEventAdviser_iface.lpVtbl = &uia_event_adviser_vtbl;
    adv_events->ref = 1;
    adv_events->advise_events = advise_events;
    IRawElementProviderAdviseEvents_AddRef(advise_events);

    if (!uia_array_reserve((void **)&event->event_advisers, &event->event_advisers_arr_size,
                event->event_advisers_count + 1, sizeof(*event->event_advisers)))
    {
        IWineUiaEventAdviser_Release(&adv_events->IWineUiaEventAdviser_iface);
        return E_OUTOFMEMORY;
    }

    event->event_advisers[event->event_advisers_count] = &adv_events->IWineUiaEventAdviser_iface;
    event->event_advisers_count++;

    return S_OK;
}

/***********************************************************************
 *          UiaAddEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaAddEvent(HUIANODE huianode, EVENTID event_id, UiaEventCallback *callback, enum TreeScope scope,
        PROPERTYID *prop_ids, int prop_ids_count, struct UiaCacheRequest *cache_req, HUIAEVENT *huiaevent)
{
    const struct uia_event_info *event_info = uia_event_info_from_id(event_id);
    struct uia_event *event;
    SAFEARRAY *sa;
    HRESULT hr;

    TRACE("(%p, %d, %p, %#x, %p, %d, %p, %p)\n", huianode, event_id, callback, scope, prop_ids, prop_ids_count,
            cache_req, huiaevent);

    if (!huianode || !callback || !cache_req || !huiaevent)
        return E_INVALIDARG;

    if (!event_info)
        WARN("No event information for event ID %d\n", event_id);

    *huiaevent = NULL;
    if (event_info && (event_info->event_arg_type == EventArgsType_PropertyChanged))
    {
        FIXME("Property changed event registration currently unimplemented\n");
        return E_NOTIMPL;
    }

    hr = UiaGetRuntimeId(huianode, &sa);
    if (FAILED(hr))
        return hr;

    hr = create_uia_event(&event, event_id, scope, callback, sa);
    if (FAILED(hr))
    {
        SafeArrayDestroy(sa);
        return hr;
    }

    hr = attach_event_to_uia_node(huianode, event);
    if (FAILED(hr))
        goto exit;

    hr = IWineUiaEvent_advise_events(&event->IWineUiaEvent_iface, TRUE);
    if (FAILED(hr))
        goto exit;

    *huiaevent = (HUIAEVENT)event;

exit:
    if (FAILED(hr))
        IWineUiaEvent_Release(&event->IWineUiaEvent_iface);

    return hr;
}

/***********************************************************************
 *          UiaRemoveEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRemoveEvent(HUIAEVENT huiaevent)
{
    struct uia_event *event = unsafe_impl_from_IWineUiaEvent((IWineUiaEvent *)huiaevent);
    HRESULT hr;

    TRACE("(%p)\n", event);

    if (!event)
        return E_INVALIDARG;

    hr = IWineUiaEvent_advise_events(&event->IWineUiaEvent_iface, FALSE);
    IWineUiaEvent_Release(&event->IWineUiaEvent_iface);
    if (FAILED(hr))
        WARN("advise_events failed with hr %#lx\n", hr);

    return S_OK;
}
