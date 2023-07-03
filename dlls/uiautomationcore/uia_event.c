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
#include "wine/rbtree.h"
#include "assert.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static SAFEARRAY *uia_desktop_node_rt_id;
static BOOL WINAPI uia_init_desktop_rt_id(INIT_ONCE *once, void *param, void **ctx)
{
    SAFEARRAY *sa;

    if ((sa = SafeArrayCreateVector(VT_I4, 0, 2)))
    {
        if (SUCCEEDED(write_runtime_id_base(sa, GetDesktopWindow())))
            uia_desktop_node_rt_id = sa;
        else
            SafeArrayDestroy(sa);
    }

    return !!uia_desktop_node_rt_id;
}

static SAFEARRAY *uia_get_desktop_rt_id(void)
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;

    if (!uia_desktop_node_rt_id)
        InitOnceExecuteOnce(&once, uia_init_desktop_rt_id, NULL, NULL);

    return uia_desktop_node_rt_id;
}

/*
 * UI Automation event map.
 */
static struct uia_event_map
{
    struct rb_tree event_map;
    LONG event_count;

    /* rb_tree for serverside events, sorted by PID/event cookie. */
    struct rb_tree serverside_event_map;
    LONG serverside_event_count;
} uia_event_map;

struct uia_event_map_entry
{
    struct rb_entry entry;
    LONG refs;

    int event_id;

    /*
     * List of registered events for this event ID. Events are only removed
     * from the list when the event map entry reference count hits 0 and the
     * entry is destroyed. This avoids dealing with mid-list removal while
     * iterating over the list when an event is raised. Rather than remove
     * an event from the list, we mark an event as being defunct so it is
     * ignored.
     */
    struct list events_list;
    struct list serverside_events_list;
};

struct uia_event_identifier {
    LONG event_cookie;
    LONG proc_id;
};

static int uia_serverside_event_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_event *event = RB_ENTRY_VALUE(entry, struct uia_event, u.serverside.serverside_event_entry);
    struct uia_event_identifier *event_id = (struct uia_event_identifier *)key;

    if (event_id->proc_id != event->u.serverside.proc_id)
        return (event_id->proc_id > event->u.serverside.proc_id) - (event_id->proc_id < event->u.serverside.proc_id);
    else
        return (event_id->event_cookie > event->event_cookie) - (event_id->event_cookie < event->event_cookie);
}

static CRITICAL_SECTION event_map_cs;
static CRITICAL_SECTION_DEBUG event_map_cs_debug =
{
    0, 0, &event_map_cs,
    { &event_map_cs_debug.ProcessLocksList, &event_map_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": event_map_cs") }
};
static CRITICAL_SECTION event_map_cs = { &event_map_cs_debug, -1, 0, 0, 0, 0 };

static int uia_event_map_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_event_map_entry *event_entry = RB_ENTRY_VALUE(entry, struct uia_event_map_entry, entry);
    int event_id = *((int *)key);

    return (event_entry->event_id > event_id) - (event_entry->event_id < event_id);
}

static struct uia_event_map_entry *uia_get_event_map_entry_for_event(int event_id)
{
    struct uia_event_map_entry *map_entry = NULL;
    struct rb_entry *rb_entry;

    if (uia_event_map.event_count && (rb_entry = rb_get(&uia_event_map.event_map, &event_id)))
        map_entry = RB_ENTRY_VALUE(rb_entry, struct uia_event_map_entry, entry);

    return map_entry;
}

static HRESULT uia_event_map_add_event(struct uia_event *event)
{
    const int subtree_scope = TreeScope_Element | TreeScope_Descendants;
    struct uia_event_map_entry *event_entry;

    if (((event->scope & subtree_scope) == subtree_scope) && event->runtime_id &&
            !uia_compare_safearrays(uia_get_desktop_rt_id(), event->runtime_id, UIAutomationType_IntArray))
        event->desktop_subtree_event = TRUE;

    EnterCriticalSection(&event_map_cs);

    if (!(event_entry = uia_get_event_map_entry_for_event(event->event_id)))
    {
        if (!(event_entry = heap_alloc_zero(sizeof(*event_entry))))
        {
            LeaveCriticalSection(&event_map_cs);
            return E_OUTOFMEMORY;
        }

        event_entry->event_id = event->event_id;
        list_init(&event_entry->events_list);
        list_init(&event_entry->serverside_events_list);

        if (!uia_event_map.event_count)
            rb_init(&uia_event_map.event_map, uia_event_map_id_compare);
        rb_put(&uia_event_map.event_map, &event->event_id, &event_entry->entry);
        uia_event_map.event_count++;
    }

    IWineUiaEvent_AddRef(&event->IWineUiaEvent_iface);
    if (event->event_type == EVENT_TYPE_SERVERSIDE)
        list_add_head(&event_entry->serverside_events_list, &event->event_list_entry);
    else
        list_add_head(&event_entry->events_list, &event->event_list_entry);
    InterlockedIncrement(&event_entry->refs);

    event->event_map_entry = event_entry;
    LeaveCriticalSection(&event_map_cs);

    return S_OK;
}

static void uia_event_map_entry_release(struct uia_event_map_entry *entry)
{
    ULONG ref = InterlockedDecrement(&entry->refs);

    if (!ref)
    {
        struct list *cursor, *cursor2;

        EnterCriticalSection(&event_map_cs);

        /*
         * Someone grabbed this while we were waiting to enter the CS, abort
         * destruction.
         */
        if (InterlockedCompareExchange(&entry->refs, 0, 0) != 0)
        {
            LeaveCriticalSection(&event_map_cs);
            return;
        }

        rb_remove(&uia_event_map.event_map, &entry->entry);
        uia_event_map.event_count--;
        LeaveCriticalSection(&event_map_cs);

        /* Release all events in the list. */
        LIST_FOR_EACH_SAFE(cursor, cursor2, &entry->events_list)
        {
            struct uia_event *event = LIST_ENTRY(cursor, struct uia_event, event_list_entry);

            IWineUiaEvent_Release(&event->IWineUiaEvent_iface);
        }

        LIST_FOR_EACH_SAFE(cursor, cursor2, &entry->serverside_events_list)
        {
            struct uia_event *event = LIST_ENTRY(cursor, struct uia_event, event_list_entry);

            IWineUiaEvent_Release(&event->IWineUiaEvent_iface);
        }

        heap_free(entry);
    }
}

/*
 * Functions for struct uia_event_args, a reference counted structure
 * used to store event arguments. This is necessary for serverside events
 * as they're raised on a background thread after the event raising
 * function has returned.
 */
struct uia_event_args
{
    struct UiaEventArgs simple_args;
    LONG ref;
};

static struct uia_event_args *create_uia_event_args(const struct uia_event_info *event_info)
{
    struct uia_event_args *args = heap_alloc_zero(sizeof(*args));

    if (!args)
        return NULL;

    args->simple_args.Type = event_info->event_arg_type;
    args->simple_args.EventId = event_info->event_id;
    args->ref = 1;

    return args;
}

static void uia_event_args_release(struct uia_event_args *args)
{
    if (!InterlockedDecrement(&args->ref))
        heap_free(args);
}

/*
 * UI Automation event thread.
 */
struct uia_event_thread
{
    HANDLE hthread;
    HWND hwnd;
    LONG ref;

    struct list *event_queue;
};

#define WM_UIA_EVENT_THREAD_STOP (WM_USER + 1)
#define WM_UIA_EVENT_THREAD_PROCESS_QUEUE (WM_USER + 2)
static struct uia_event_thread event_thread;
static CRITICAL_SECTION event_thread_cs;
static CRITICAL_SECTION_DEBUG event_thread_cs_debug =
{
    0, 0, &event_thread_cs,
    { &event_thread_cs_debug.ProcessLocksList, &event_thread_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": event_thread_cs") }
};
static CRITICAL_SECTION event_thread_cs = { &event_thread_cs_debug, -1, 0, 0, 0, 0 };

enum uia_queue_event_type {
    QUEUE_EVENT_TYPE_SERVERSIDE,
    QUEUE_EVENT_TYPE_CLIENTSIDE,
};

struct uia_queue_event
{
    struct list event_queue_entry;
    int queue_event_type;

    struct uia_event_args *args;
    struct uia_event *event;
    union {
        struct {
            HUIANODE node;
            HUIANODE nav_start_node;
        } serverside;
        struct {
            LRESULT node;
            LRESULT nav_start_node;
        } clientside;
     } u;
};

static void uia_event_queue_push(struct uia_queue_event *event)
{
    EnterCriticalSection(&event_thread_cs);

    assert(event_thread.event_queue);
    list_add_tail(event_thread.event_queue, &event->event_queue_entry);
    PostMessageW(event_thread.hwnd, WM_UIA_EVENT_THREAD_PROCESS_QUEUE, 0, 0);

    LeaveCriticalSection(&event_thread_cs);
}

static struct uia_queue_event *uia_event_queue_pop(struct list *event_queue)
{
    struct uia_queue_event *queue_event = NULL;

    EnterCriticalSection(&event_thread_cs);

    if (!list_empty(event_queue))
    {
        queue_event = LIST_ENTRY(list_head(event_queue), struct uia_queue_event, event_queue_entry);
        list_remove(list_head(event_queue));
    }

    LeaveCriticalSection(&event_thread_cs);
    return queue_event;
}

static void uia_node_lresult_release(LRESULT lr)
{
    IWineUiaNode *node;

    if (lr && SUCCEEDED(ObjectFromLresult(lr, &IID_IWineUiaNode, 0, (void **)&node)))
        IWineUiaNode_Release(node);
}

static HRESULT uia_event_invoke(HUIANODE node, HUIANODE nav_start_node, struct uia_event_args *args,
        struct uia_event *event);
static HRESULT uia_raise_clientside_event(struct uia_queue_event *event)
{
    HUIANODE node, nav_start_node;
    HRESULT hr;

    node = nav_start_node = NULL;
    hr = uia_node_from_lresult(event->u.clientside.node, &node);
    if (FAILED(hr))
    {
        WARN("Failed to create node from lresult, hr %#lx\n", hr);
        uia_node_lresult_release(event->u.clientside.nav_start_node);
        return hr;
    }

    if (event->u.clientside.nav_start_node)
    {
        hr = uia_node_from_lresult(event->u.clientside.nav_start_node, &nav_start_node);
        if (FAILED(hr))
        {
            WARN("Failed to create nav_start_node from lresult, hr %#lx\n", hr);
            UiaNodeRelease(node);
            return hr;
        }
    }

    hr = uia_event_invoke(node, nav_start_node, event->args, event->event);
    UiaNodeRelease(node);
    UiaNodeRelease(nav_start_node);

    return hr;
}

static HRESULT uia_raise_serverside_event(struct uia_queue_event *event)
{
    HRESULT hr = S_OK;
    LRESULT lr, lr2;
    VARIANT v, v2;

    /*
     * uia_lresult_from_node is expected to release the node here upon
     * failure.
     */
    lr = lr2 = 0;
    if (!(lr = uia_lresult_from_node(event->u.serverside.node)))
    {
        UiaNodeRelease(event->u.serverside.nav_start_node);
        return E_FAIL;
    }

    if (event->u.serverside.nav_start_node && !(lr2 = uia_lresult_from_node(event->u.serverside.nav_start_node)))
    {
        uia_node_lresult_release(lr);
        return E_FAIL;
    }

    VariantInit(&v2);
    variant_init_i4(&v, lr);
    if (lr2)
        variant_init_i4(&v2, lr2);

    hr = IWineUiaEvent_raise_event(event->event->u.serverside.event_iface, v, v2);
    if (FAILED(hr))
    {
        uia_node_lresult_release(lr);
        uia_node_lresult_release(lr2);
    }

    return hr;
}

static void uia_event_thread_process_queue(struct list *event_queue)
{
    while (1)
    {
        struct uia_queue_event *event;
        HRESULT hr;

        if (!(event = uia_event_queue_pop(event_queue)))
            break;

        if (event->queue_event_type == QUEUE_EVENT_TYPE_SERVERSIDE)
            hr = uia_raise_serverside_event(event);
        else
            hr = uia_raise_clientside_event(event);

        if (FAILED(hr))
            WARN("Failed to raise event type %d with hr %#lx\n", event->queue_event_type, hr);

        uia_event_args_release(event->args);
        IWineUiaEvent_Release(&event->event->IWineUiaEvent_iface);
        heap_free(event);
    }
}

static DWORD WINAPI uia_event_thread_proc(void *arg)
{
    HANDLE initialized_event = arg;
    struct list event_queue;
    HWND hwnd;
    MSG msg;

    list_init(&event_queue);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hwnd = CreateWindowW(L"Message", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!hwnd)
    {
        WARN("CreateWindow failed: %ld\n", GetLastError());
        CoUninitialize();
        FreeLibraryAndExitThread(huia_module, 1);
    }

    event_thread.hwnd = hwnd;
    event_thread.event_queue = &event_queue;

    /* Initialization complete, thread can now process window messages. */
    SetEvent(initialized_event);
    TRACE("Event thread started.\n");
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if ((msg.hwnd == hwnd) && ((msg.message == WM_UIA_EVENT_THREAD_STOP) ||
                    (msg.message == WM_UIA_EVENT_THREAD_PROCESS_QUEUE)))
        {
            uia_event_thread_process_queue(&event_queue);
            if (msg.message == WM_UIA_EVENT_THREAD_STOP)
                break;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    TRACE("Shutting down UI Automation event thread.\n");

    DestroyWindow(hwnd);
    CoUninitialize();
    FreeLibraryAndExitThread(huia_module, 0);
}

static BOOL uia_start_event_thread(void)
{
    BOOL started = TRUE;

    EnterCriticalSection(&event_thread_cs);
    if (++event_thread.ref == 1)
    {
        HANDLE ready_event = NULL;
        HANDLE events[2];
        HMODULE hmodule;
        DWORD wait_obj;

        /* Increment DLL reference count. */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (const WCHAR *)uia_start_event_thread, &hmodule);

        events[0] = ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!(event_thread.hthread = CreateThread(NULL, 0, uia_event_thread_proc,
                ready_event, 0, NULL)))
        {
            FreeLibrary(hmodule);
            started = FALSE;
            goto exit;
        }

        events[1] = event_thread.hthread;
        wait_obj = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (wait_obj != WAIT_OBJECT_0)
        {
            CloseHandle(event_thread.hthread);
            started = FALSE;
        }

exit:
        if (ready_event)
            CloseHandle(ready_event);
        if (!started)
            memset(&event_thread, 0, sizeof(event_thread));
    }

    LeaveCriticalSection(&event_thread_cs);
    return started;
}

static void uia_stop_event_thread(void)
{
    EnterCriticalSection(&event_thread_cs);
    if (!--event_thread.ref)
    {
        PostMessageW(event_thread.hwnd, WM_UIA_EVENT_THREAD_STOP, 0, 0);
        CloseHandle(event_thread.hthread);
        memset(&event_thread, 0, sizeof(event_thread));
    }
    LeaveCriticalSection(&event_thread_cs);
}

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

        /*
         * If this event has an event_map_entry, it should've been released
         * before hitting a reference count of 0.
         */
        assert(!event->event_map_entry);

        SafeArrayDestroy(event->runtime_id);
        if (event->event_type == EVENT_TYPE_CLIENTSIDE)
        {
            uia_cache_request_destroy(&event->u.clientside.cache_req);
            if (event->u.clientside.git_cookie)
                uia_stop_event_thread();
        }
        else
        {
            EnterCriticalSection(&event_map_cs);
            rb_remove(&uia_event_map.serverside_event_map, &event->u.serverside.serverside_event_entry);
            uia_event_map.serverside_event_count--;
            LeaveCriticalSection(&event_map_cs);
            if (event->u.serverside.event_iface)
                IWineUiaEvent_Release(event->u.serverside.event_iface);
            uia_stop_event_thread();
        }

        for (i = 0; i < event->event_advisers_count; i++)
            IWineUiaEventAdviser_Release(event->event_advisers[i]);
        heap_free(event->event_advisers);
        heap_free(event);
    }

    return ref;
}

static HRESULT WINAPI uia_event_advise_events(IWineUiaEvent *iface, BOOL advise_added, LONG adviser_start_idx)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    HRESULT hr;
    int i;

    TRACE("%p, %d, %ld\n", event, advise_added, adviser_start_idx);

    for (i = adviser_start_idx; i < event->event_advisers_count; i++)
    {
        hr = IWineUiaEventAdviser_advise(event->event_advisers[i], advise_added, (UINT_PTR)event);
        if (FAILED(hr))
            return hr;
    }

    /*
     * First call to advise events on a serverside provider, add it to the
     * events list so it can be raised.
     */
    if (!adviser_start_idx && advise_added && event->event_type == EVENT_TYPE_SERVERSIDE)
    {
        hr = uia_event_map_add_event(event);
        if (FAILED(hr))
            WARN("Failed to add event to event map, hr %#lx\n", hr);
    }

    /*
     * Once we've advised of removal, no need to keep the advisers around.
     * We can also release our reference to the event map.
     */
    if (!advise_added)
    {
        InterlockedIncrement(&event->event_defunct);
        uia_event_map_entry_release(event->event_map_entry);
        event->event_map_entry = NULL;

        for (i = 0; i < event->event_advisers_count; i++)
            IWineUiaEventAdviser_Release(event->event_advisers[i]);
        heap_free(event->event_advisers);
        event->event_advisers_count = event->event_advisers_arr_size = 0;
    }

    return S_OK;
}

static HRESULT WINAPI uia_event_set_event_data(IWineUiaEvent *iface, const GUID *event_guid, LONG scope,
        VARIANT runtime_id, IWineUiaEvent *event_iface)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);

    TRACE("%p, %s, %ld, %s, %p\n", event, debugstr_guid(event_guid), scope, debugstr_variant(&runtime_id), event_iface);

    assert(event->event_type == EVENT_TYPE_SERVERSIDE);

    event->event_id = UiaLookupId(AutomationIdentifierType_Event, event_guid);
    event->scope = scope;
    if (V_VT(&runtime_id) == (VT_I4 | VT_ARRAY))
    {
        HRESULT hr;

        hr = SafeArrayCopy(V_ARRAY(&runtime_id), &event->runtime_id);
        if (FAILED(hr))
        {
            WARN("Failed to copy runtime id, hr %#lx\n", hr);
            return hr;
        }
    }
    event->u.serverside.event_iface = event_iface;
    IWineUiaEvent_AddRef(event_iface);

    return S_OK;
}

static HRESULT WINAPI uia_event_raise_event(IWineUiaEvent *iface, VARIANT in_node, VARIANT in_nav_start_node)
{
    struct uia_event *event = impl_from_IWineUiaEvent(iface);
    struct uia_queue_event *queue_event;
    struct uia_event_args *args;

    TRACE("%p, %s, %s\n", iface, debugstr_variant(&in_node), debugstr_variant(&in_nav_start_node));

    assert(event->event_type != EVENT_TYPE_SERVERSIDE);

    if (!(queue_event = heap_alloc_zero(sizeof(*queue_event))))
        return E_OUTOFMEMORY;

    if (!(args = create_uia_event_args(uia_event_info_from_id(event->event_id))))
    {
        heap_free(queue_event);
        return E_OUTOFMEMORY;
    }

    queue_event->queue_event_type = QUEUE_EVENT_TYPE_CLIENTSIDE;
    queue_event->args = args;
    queue_event->event = event;
    queue_event->u.clientside.node = V_I4(&in_node);
    if (V_VT(&in_nav_start_node) == VT_I4)
        queue_event->u.clientside.nav_start_node = V_I4(&in_nav_start_node);

    IWineUiaEvent_AddRef(&event->IWineUiaEvent_iface);
    uia_event_queue_push(queue_event);

    return S_OK;
}

static const IWineUiaEventVtbl uia_event_vtbl = {
    uia_event_QueryInterface,
    uia_event_AddRef,
    uia_event_Release,
    uia_event_advise_events,
    uia_event_set_event_data,
    uia_event_raise_event,
};

static struct uia_event *unsafe_impl_from_IWineUiaEvent(IWineUiaEvent *iface)
{
    if (!iface || (iface->lpVtbl != &uia_event_vtbl))
        return NULL;

    return CONTAINING_RECORD(iface, struct uia_event, IWineUiaEvent_iface);
}

static HRESULT create_uia_event(struct uia_event **out_event, LONG event_cookie, int event_type)
{
    struct uia_event *event = heap_alloc_zero(sizeof(*event));

    *out_event = NULL;
    if (!event)
        return E_OUTOFMEMORY;

    event->IWineUiaEvent_iface.lpVtbl = &uia_event_vtbl;
    event->ref = 1;
    event->event_cookie = event_cookie;
    event->event_type = event_type;
    *out_event = event;

    return S_OK;
}

static HRESULT create_clientside_uia_event(struct uia_event **out_event, int event_id, int scope, UiaEventCallback *cback,
        SAFEARRAY *runtime_id)
{
    struct uia_event *event = NULL;
    static LONG next_event_cookie;
    HRESULT hr;

    *out_event = NULL;
    hr = create_uia_event(&event, InterlockedIncrement(&next_event_cookie), EVENT_TYPE_CLIENTSIDE);
    if (FAILED(hr))
        return hr;

    event->runtime_id = runtime_id;
    event->event_id = event_id;
    event->scope = scope;
    event->u.clientside.cback = cback;

    *out_event = event;
    return S_OK;
}

HRESULT create_serverside_uia_event(struct uia_event **out_event, LONG process_id, LONG event_cookie)
{
    struct uia_event_identifier event_identifier = { event_cookie, process_id };
    struct rb_entry *rb_entry;
    struct uia_event *event;
    HRESULT hr = S_OK;

    /*
     * Attempt to lookup an existing event for this PID/event_cookie. If there
     * is one, return S_FALSE.
     */
    *out_event = NULL;
    EnterCriticalSection(&event_map_cs);
    if (uia_event_map.serverside_event_count && (rb_entry = rb_get(&uia_event_map.serverside_event_map, &event_identifier)))
    {
        *out_event = RB_ENTRY_VALUE(rb_entry, struct uia_event, u.serverside.serverside_event_entry);
        hr = S_FALSE;
        goto exit;
    }

    hr = create_uia_event(&event, event_cookie, EVENT_TYPE_SERVERSIDE);
    if (FAILED(hr))
        goto exit;

    if (!uia_start_event_thread())
    {
        heap_free(event);
        hr = E_FAIL;
        goto exit;
    }

    event->u.serverside.proc_id = process_id;
    uia_event_map.serverside_event_count++;
    if (uia_event_map.serverside_event_count == 1)
        rb_init(&uia_event_map.serverside_event_map, uia_serverside_event_id_compare);
    rb_put(&uia_event_map.serverside_event_map, &event_identifier, &event->u.serverside.serverside_event_entry);
    *out_event = event;

exit:
    LeaveCriticalSection(&event_map_cs);
    return hr;
}

static HRESULT uia_event_add_event_adviser(IWineUiaEventAdviser *adviser, struct uia_event *event)
{
    if (!uia_array_reserve((void **)&event->event_advisers, &event->event_advisers_arr_size,
                event->event_advisers_count + 1, sizeof(*event->event_advisers)))
        return E_OUTOFMEMORY;

    event->event_advisers[event->event_advisers_count] = adviser;
    IWineUiaEventAdviser_AddRef(adviser);
    event->event_advisers_count++;

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

    hr = uia_event_add_event_adviser(&adv_events->IWineUiaEventAdviser_iface, event);
    IWineUiaEventAdviser_Release(&adv_events->IWineUiaEventAdviser_iface);

    return hr;
}

/*
 * IWineUiaEventAdviser interface for serverside events.
 */
struct uia_serverside_event_adviser {
    IWineUiaEventAdviser IWineUiaEventAdviser_iface;
    LONG ref;

    IWineUiaEvent *event_iface;
};

static inline struct uia_serverside_event_adviser *impl_from_serverside_IWineUiaEventAdviser(IWineUiaEventAdviser *iface)
{
    return CONTAINING_RECORD(iface, struct uia_serverside_event_adviser, IWineUiaEventAdviser_iface);
}

static HRESULT WINAPI uia_serverside_event_adviser_QueryInterface(IWineUiaEventAdviser *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IWineUiaEventAdviser) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IWineUiaEventAdviser_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_serverside_event_adviser_AddRef(IWineUiaEventAdviser *iface)
{
    struct uia_serverside_event_adviser *adv_events = impl_from_serverside_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedIncrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    return ref;
}

static ULONG WINAPI uia_serverside_event_adviser_Release(IWineUiaEventAdviser *iface)
{
    struct uia_serverside_event_adviser *adv_events = impl_from_serverside_IWineUiaEventAdviser(iface);
    ULONG ref = InterlockedDecrement(&adv_events->ref);

    TRACE("%p, refcount %ld\n", adv_events, ref);
    if (!ref)
    {
        IWineUiaEvent_Release(adv_events->event_iface);
        heap_free(adv_events);
    }
    return ref;
}

static HRESULT WINAPI uia_serverside_event_adviser_advise(IWineUiaEventAdviser *iface, BOOL advise_added, LONG_PTR huiaevent)
{
    struct uia_serverside_event_adviser *adv_events = impl_from_serverside_IWineUiaEventAdviser(iface);
    struct uia_event *event_data = (struct uia_event *)huiaevent;
    HRESULT hr;

    TRACE("%p, %d, %#Ix\n", adv_events, advise_added, huiaevent);

    if (advise_added)
    {
        const struct uia_event_info *event_info = uia_event_info_from_id(event_data->event_id);
        VARIANT v;

        VariantInit(&v);
        if (event_data->runtime_id)
        {
            V_VT(&v) = VT_I4 | VT_ARRAY;
            V_ARRAY(&v) = event_data->runtime_id;
        }

        hr = IWineUiaEvent_set_event_data(adv_events->event_iface, event_info->guid, event_data->scope, v,
                &event_data->IWineUiaEvent_iface);
        if (FAILED(hr))
        {
            WARN("Failed to set event data on serverside event, hr %#lx\n", hr);
            return hr;
        }
    }

    return IWineUiaEvent_advise_events(adv_events->event_iface, advise_added, 0);
}

static const IWineUiaEventAdviserVtbl uia_serverside_event_adviser_vtbl = {
    uia_serverside_event_adviser_QueryInterface,
    uia_serverside_event_adviser_AddRef,
    uia_serverside_event_adviser_Release,
    uia_serverside_event_adviser_advise,
};

HRESULT uia_event_add_serverside_event_adviser(IWineUiaEvent *serverside_event, struct uia_event *event)
{
    struct uia_serverside_event_adviser *adv_events;
    HRESULT hr;

    /*
     * Need to create a proxy IWineUiaEvent for our clientside event to use
     * this serverside IWineUiaEvent proxy from the appropriate apartment.
     */
    if (!event->u.clientside.git_cookie)
    {
        if (!uia_start_event_thread())
            return E_FAIL;

        hr = register_interface_in_git((IUnknown *)&event->IWineUiaEvent_iface, &IID_IWineUiaEvent,
                &event->u.clientside.git_cookie);
        if (FAILED(hr))
        {
            uia_stop_event_thread();
            return hr;
        }
    }

    if (!(adv_events = heap_alloc_zero(sizeof(*adv_events))))
        return E_OUTOFMEMORY;

    adv_events->IWineUiaEventAdviser_iface.lpVtbl = &uia_serverside_event_adviser_vtbl;
    adv_events->ref = 1;
    adv_events->event_iface = serverside_event;
    IWineUiaEvent_AddRef(serverside_event);

    hr = uia_event_add_event_adviser(&adv_events->IWineUiaEventAdviser_iface, event);
    IWineUiaEventAdviser_Release(&adv_events->IWineUiaEventAdviser_iface);

    return hr;
}

static HRESULT uia_event_advise(struct uia_event *event, BOOL advise_added, LONG start_idx)
{
    IWineUiaEvent *event_iface;
    HRESULT hr;

    if (event->u.clientside.git_cookie)
    {
        hr = get_interface_in_git(&IID_IWineUiaEvent, event->u.clientside.git_cookie,
                (IUnknown **)&event_iface);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        event_iface = &event->IWineUiaEvent_iface;
        IWineUiaEvent_AddRef(event_iface);
    }

    hr = IWineUiaEvent_advise_events(event_iface, advise_added, start_idx);
    IWineUiaEvent_Release(event_iface);

    return hr;
}

/***********************************************************************
 *          UiaEventAddWindow (uiautomationcore.@)
 */
HRESULT WINAPI UiaEventAddWindow(HUIAEVENT huiaevent, HWND hwnd)
{
    struct uia_event *event = unsafe_impl_from_IWineUiaEvent((IWineUiaEvent *)huiaevent);
    int old_event_advisers_count;
    HUIANODE node;
    HRESULT hr;

    TRACE("(%p, %p)\n", huiaevent, hwnd);

    if (!event)
        return E_INVALIDARG;

    assert(event->event_type == EVENT_TYPE_CLIENTSIDE);

    hr = UiaNodeFromHandle(hwnd, &node);
    if (FAILED(hr))
        return hr;

    old_event_advisers_count = event->event_advisers_count;
    hr = attach_event_to_uia_node(node, event);
    if (FAILED(hr))
        goto exit;

    if (event->event_advisers_count != old_event_advisers_count)
        hr = uia_event_advise(event, TRUE, old_event_advisers_count);

exit:
    UiaNodeRelease(node);

    return hr;
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

    hr = create_clientside_uia_event(&event, event_id, scope, callback, sa);
    if (FAILED(hr))
    {
        SafeArrayDestroy(sa);
        return hr;
    }

    hr = uia_cache_request_clone(&event->u.clientside.cache_req, cache_req);
    if (FAILED(hr))
        goto exit;

    hr = attach_event_to_uia_node(huianode, event);
    if (FAILED(hr))
        goto exit;

    hr = uia_event_advise(event, TRUE, 0);
    if (FAILED(hr))
        goto exit;

    hr = uia_event_map_add_event(event);
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

    assert(event->event_type == EVENT_TYPE_CLIENTSIDE);
    hr = uia_event_advise(event, FALSE, 0);
    if (FAILED(hr))
        return hr;

    if (event->u.clientside.git_cookie)
    {
        hr = unregister_interface_in_git(event->u.clientside.git_cookie);
        if (FAILED(hr))
            return hr;
    }

    IWineUiaEvent_Release(&event->IWineUiaEvent_iface);
    return S_OK;
}

static HRESULT uia_event_check_match(HUIANODE node, HUIANODE nav_start_node, SAFEARRAY *rt_id,
        struct uia_event_args *args, struct uia_event *event);
static HRESULT uia_event_invoke(HUIANODE node, HUIANODE nav_start_node, struct uia_event_args *args, struct uia_event *event)
{
    HRESULT hr = S_OK;

    if (event->event_type == EVENT_TYPE_CLIENTSIDE)
    {
        SAFEARRAY *out_req;
        BSTR tree_struct;

        if (nav_start_node)
            return uia_event_check_match(node, nav_start_node, NULL, args, event);

        hr = UiaGetUpdatedCache(node, &event->u.clientside.cache_req, NormalizeState_View, NULL, &out_req,
                &tree_struct);
        if (SUCCEEDED(hr))
        {
            event->u.clientside.cback(&args->simple_args, out_req, tree_struct);
            SafeArrayDestroy(out_req);
            SysFreeString(tree_struct);
        }
    }
    else
    {
        struct uia_queue_event *queue_event;
        HUIANODE node2, nav_start_node2;

        if (!(queue_event = heap_alloc_zero(sizeof(*queue_event))))
            return E_OUTOFMEMORY;

        node2 = nav_start_node2 = NULL;
        hr = clone_uia_node(node, &node2);
        if (FAILED(hr))
        {
            heap_free(queue_event);
            return hr;
        }

        if (nav_start_node)
        {
            hr = clone_uia_node(nav_start_node, &nav_start_node2);
            if (FAILED(hr))
            {
                heap_free(queue_event);
                UiaNodeRelease(node2);
                return hr;
            }
        }

        queue_event->queue_event_type = QUEUE_EVENT_TYPE_SERVERSIDE;
        queue_event->args = args;
        queue_event->event = event;
        queue_event->u.serverside.node = node2;
        queue_event->u.serverside.nav_start_node = nav_start_node2;

        InterlockedIncrement(&args->ref);
        IWineUiaEvent_AddRef(&event->IWineUiaEvent_iface);
        uia_event_queue_push(queue_event);
    }

    return hr;
}

static void set_refuse_hwnd_providers(struct uia_node *node, BOOL refuse_hwnd_providers)
{
    struct uia_provider *prov_data = impl_from_IWineUiaProvider(node->prov[get_node_provider_type_at_idx(node, 0)]);

    prov_data->refuse_hwnd_node_providers = refuse_hwnd_providers;
}

/*
 * Check if the provider that raised the event matches this particular event.
 */
static HRESULT uia_event_check_match(HUIANODE node, HUIANODE nav_start_node, SAFEARRAY *rt_id,
        struct uia_event_args *args, struct uia_event *event)
{
    struct UiaPropertyCondition prop_cond = { ConditionType_Property, UIA_RuntimeIdPropertyId };
    struct uia_node *node_data = impl_from_IWineUiaNode((IWineUiaNode *)nav_start_node);
    HRESULT hr = S_OK;

    /* Event is no longer valid. */
    if (InterlockedCompareExchange(&event->event_defunct, 0, 0) != 0)
        return S_OK;

    /* Can't match an event that doesn't have a runtime ID, early out. */
    if (!event->runtime_id)
        return S_OK;

    if (event->desktop_subtree_event)
        return uia_event_invoke(node, NULL, args, event);

    if (rt_id && !uia_compare_safearrays(rt_id, event->runtime_id, UIAutomationType_IntArray))
    {
        if (event->scope & TreeScope_Element)
            hr = uia_event_invoke(node, NULL, args, event);
        return hr;
    }

    if (!(event->scope & (TreeScope_Descendants | TreeScope_Children)))
        return S_OK;

    V_VT(&prop_cond.Value) = VT_I4 | VT_ARRAY;
    V_ARRAY(&prop_cond.Value) = event->runtime_id;

    IWineUiaNode_AddRef(&node_data->IWineUiaNode_iface);
    while (1)
    {
        HUIANODE node2 = NULL;

        /*
         * When trying to match serverside events through navigation, we
         * don't want any clientside providers added in the server process.
         * Once we encounter a provider with an HWND, we pass it off to the
         * client for any further navigation.
         */
        if (event->event_type == EVENT_TYPE_SERVERSIDE)
        {
            if (node_data->hwnd)
            {
                hr = uia_event_invoke(node, (HUIANODE)&node_data->IWineUiaNode_iface, args, event);
                break;
            }
            set_refuse_hwnd_providers(node_data, TRUE);
        }

        hr = navigate_uia_node(node_data, NavigateDirection_Parent, &node2);
        IWineUiaNode_Release(&node_data->IWineUiaNode_iface);
        if (FAILED(hr) || !node2)
            return hr;

        node_data = impl_from_IWineUiaNode((IWineUiaNode *)node2);
        hr = uia_condition_check(node2, (struct UiaCondition *)&prop_cond);
        if (FAILED(hr))
            break;

        if (uia_condition_matched(hr))
        {
            hr = uia_event_invoke(node, NULL, args, event);
            break;
        }

        if (!(event->scope & TreeScope_Descendants))
            break;
    }
    IWineUiaNode_Release(&node_data->IWineUiaNode_iface);

    return hr;
}

static HRESULT uia_raise_event(IRawElementProviderSimple *elprov, struct uia_event_args *args)
{
    struct uia_event_map_entry *event_entry;
    enum ProviderOptions prov_opts = 0;
    struct list *cursor, *cursor2;
    HUIANODE node;
    SAFEARRAY *sa;
    HRESULT hr;

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opts);
    if (FAILED(hr))
        return hr;

    EnterCriticalSection(&event_map_cs);
    if ((event_entry = uia_get_event_map_entry_for_event(args->simple_args.EventId)))
        InterlockedIncrement(&event_entry->refs);
    LeaveCriticalSection(&event_map_cs);

    if (!event_entry)
        return S_OK;

    /*
     * For events raised on server-side providers, we don't want to add any
     * clientside HWND providers.
     */
    if (prov_opts & ProviderOptions_ServerSideProvider)
        hr = create_uia_node_from_elprov(elprov, &node, FALSE);
    else
        hr = create_uia_node_from_elprov(elprov, &node, TRUE);
    if (FAILED(hr))
    {
        uia_event_map_entry_release(event_entry);
        return hr;
    }

    hr = UiaGetRuntimeId(node, &sa);
    if (FAILED(hr))
    {
        uia_event_map_entry_release(event_entry);
        UiaNodeRelease(node);
        return hr;
    }

    LIST_FOR_EACH_SAFE(cursor, cursor2, &event_entry->events_list)
    {
        struct uia_event *event = LIST_ENTRY(cursor, struct uia_event, event_list_entry);

        hr = uia_event_check_match(node, node, sa, args, event);
        if (FAILED(hr))
            break;
    }

    if (prov_opts & ProviderOptions_ServerSideProvider)
    {
        LIST_FOR_EACH_SAFE(cursor, cursor2, &event_entry->serverside_events_list)
        {
            struct uia_event *event = LIST_ENTRY(cursor, struct uia_event, event_list_entry);

            hr = uia_event_check_match(node, node, sa, args, event);
            if (FAILED(hr))
                break;
        }
    }

    uia_event_map_entry_release(event_entry);
    SafeArrayDestroy(sa);
    UiaNodeRelease(node);

    return hr;
}

/***********************************************************************
 *          UiaRaiseAutomationEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAutomationEvent(IRawElementProviderSimple *elprov, EVENTID id)
{
    const struct uia_event_info *event_info = uia_event_info_from_id(id);
    struct uia_event_args *args;
    HRESULT hr;

    TRACE("(%p, %d)\n", elprov, id);

    if (!elprov)
        return E_INVALIDARG;

    if (!event_info || event_info->event_arg_type != EventArgsType_Simple)
    {
        if (!event_info)
            FIXME("No event info structure for event id %d\n", id);
        else
            WARN("Wrong event raising function for event args type %d\n", event_info->event_arg_type);

        return S_OK;
    }

    args = create_uia_event_args(event_info);
    if (!args)
        return E_OUTOFMEMORY;

    hr = uia_raise_event(elprov, args);
    uia_event_args_release(args);
    if (FAILED(hr))
        return hr;

    return S_OK;
}
