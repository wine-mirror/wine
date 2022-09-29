/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

typedef enum {
    EVENTID_DOMCONTENTLOADED,
    EVENTID_ABORT,
    EVENTID_ANIMATIONEND,
    EVENTID_ANIMATIONSTART,
    EVENTID_BEFOREACTIVATE,
    EVENTID_BEFOREUNLOAD,
    EVENTID_BLUR,
    EVENTID_CHANGE,
    EVENTID_CLICK,
    EVENTID_CONTEXTMENU,
    EVENTID_DATAAVAILABLE,
    EVENTID_DBLCLICK,
    EVENTID_DRAG,
    EVENTID_DRAGSTART,
    EVENTID_ERROR,
    EVENTID_FOCUS,
    EVENTID_FOCUSIN,
    EVENTID_FOCUSOUT,
    EVENTID_HELP,
    EVENTID_INPUT,
    EVENTID_KEYDOWN,
    EVENTID_KEYPRESS,
    EVENTID_KEYUP,
    EVENTID_LOAD,
    EVENTID_LOADEND,
    EVENTID_LOADSTART,
    EVENTID_MESSAGE,
    EVENTID_MOUSEDOWN,
    EVENTID_MOUSEMOVE,
    EVENTID_MOUSEOUT,
    EVENTID_MOUSEOVER,
    EVENTID_MOUSEUP,
    EVENTID_MOUSEWHEEL,
    EVENTID_MSTHUMBNAILCLICK,
    EVENTID_PASTE,
    EVENTID_PROGRESS,
    EVENTID_READYSTATECHANGE,
    EVENTID_RESIZE,
    EVENTID_SCROLL,
    EVENTID_SELECTIONCHANGE,
    EVENTID_SELECTSTART,
    EVENTID_STORAGE,
    EVENTID_STORAGECOMMIT,
    EVENTID_SUBMIT,
    EVENTID_TIMEOUT,
    EVENTID_UNLOAD,
    EVENTID_LAST
} eventid_t;

typedef struct DOMEvent {
    DispatchEx dispex;
    IDOMEvent IDOMEvent_iface;

    LONG ref;
    void *(*query_interface)(struct DOMEvent*,REFIID);
    void (*destroy)(struct DOMEvent*);

    nsIDOMEvent *nsevent;

    eventid_t event_id;
    WCHAR *type;
    EventTarget *target;
    EventTarget *current_target;
    ULONGLONG time_stamp;
    BOOL bubbles;
    BOOL cancelable;
    BOOL prevent_default;
    BOOL stop_propagation;
    BOOL stop_immediate_propagation;
    BOOL trusted;
    DOM_EVENT_PHASE phase;

    IHTMLEventObj *event_obj;
    BOOL no_event_obj;
} DOMEvent;

const WCHAR *get_event_name(eventid_t) DECLSPEC_HIDDEN;
void check_event_attr(HTMLDocumentNode*,nsIDOMElement*) DECLSPEC_HIDDEN;
void release_event_target(EventTarget*) DECLSPEC_HIDDEN;
HRESULT set_event_handler(EventTarget*,eventid_t,VARIANT*) DECLSPEC_HIDDEN;
HRESULT get_event_handler(EventTarget*,eventid_t,VARIANT*) DECLSPEC_HIDDEN;
HRESULT attach_event(EventTarget*,BSTR,IDispatch*,VARIANT_BOOL*) DECLSPEC_HIDDEN;
HRESULT detach_event(EventTarget*,BSTR,IDispatch*) DECLSPEC_HIDDEN;
HRESULT fire_event(HTMLDOMNode*,const WCHAR*,VARIANT*,VARIANT_BOOL*) DECLSPEC_HIDDEN;
void update_doc_cp_events(HTMLDocumentNode*,cp_static_data_t*) DECLSPEC_HIDDEN;
HRESULT doc_init_events(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void detach_events(HTMLDocumentNode *doc) DECLSPEC_HIDDEN;
HRESULT create_event_obj(compat_mode_t,IHTMLEventObj**) DECLSPEC_HIDDEN;
void bind_target_event(HTMLDocumentNode*,EventTarget*,const WCHAR*,IDispatch*) DECLSPEC_HIDDEN;
HRESULT ensure_doc_nsevent_handler(HTMLDocumentNode*,nsIDOMNode*,eventid_t) DECLSPEC_HIDDEN;

void dispatch_event(EventTarget*,DOMEvent*) DECLSPEC_HIDDEN;

HRESULT create_document_event(HTMLDocumentNode*,eventid_t,DOMEvent**) DECLSPEC_HIDDEN;
HRESULT create_document_event_str(HTMLDocumentNode*,const WCHAR*,IDOMEvent**) DECLSPEC_HIDDEN;
HRESULT create_event_from_nsevent(nsIDOMEvent*,compat_mode_t,DOMEvent**) DECLSPEC_HIDDEN;
HRESULT create_message_event(HTMLDocumentNode*,VARIANT*,DOMEvent**) DECLSPEC_HIDDEN;
HRESULT create_storage_event(HTMLDocumentNode*,BSTR,BSTR,BSTR,const WCHAR*,BOOL,DOMEvent**) DECLSPEC_HIDDEN;

void init_nsevents(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void release_nsevents(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void add_nsevent_listener(HTMLDocumentNode*,nsIDOMNode*,LPCWSTR) DECLSPEC_HIDDEN;
void detach_nsevent(HTMLDocumentNode*,const WCHAR*) DECLSPEC_HIDDEN;

/* We extend dispex vtbl for EventTarget functions to avoid separated vtbl. */
typedef struct {
    dispex_static_data_vtbl_t dispex_vtbl;
    nsISupports *(*get_gecko_target)(DispatchEx*);
    void (*bind_event)(DispatchEx*,eventid_t);
    EventTarget *(*get_parent_event_target)(DispatchEx*);
    HRESULT (*handle_event_default)(DispatchEx*,eventid_t,nsIDOMEvent*,BOOL*);
    ConnectionPointContainer *(*get_cp_container)(DispatchEx*);
    IHTMLEventObj *(*set_current_event)(DispatchEx*,IHTMLEventObj*);
} event_target_vtbl_t;

IHTMLEventObj *default_set_current_event(HTMLInnerWindow*,IHTMLEventObj*) DECLSPEC_HIDDEN;

static inline EventTarget *get_node_event_prop_target(HTMLDOMNode *node, eventid_t eid)
{
    return node->vtbl->get_event_prop_target ? node->vtbl->get_event_prop_target(node, eid) : &node->event_target;
}

static inline HRESULT set_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return set_event_handler(get_node_event_prop_target(node, eid), eid, var);
}

static inline HRESULT get_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return get_event_handler(get_node_event_prop_target(node, eid), eid, var);
}

static inline HRESULT set_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return set_event_handler(&doc->doc_node->node.event_target, eid, var);
}

static inline HRESULT get_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return get_event_handler(&doc->doc_node->node.event_target, eid, var);
}
