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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "shlguid.h"
#include "wininet.h"
#include "winternl.h"

#include "mshtml_private.h"
#include "htmlscript.h"
#include "htmlevent.h"
#include "binding.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

const compat_mode_info_t compat_mode_info[] = {
    { 5, 7 },   /* DOCMODE_QUIRKS */
    { 5, 5 },   /* DOCMODE_IE5 */
    { 7, 7 },   /* DOCMODE_IE7 */
    { 8, 8 },   /* DOCMODE_IE8 */
    { 9, 9 },   /* DOCMODE_IE8 */
    { 10, 10 }, /* DOCMODE_IE10 */
    { 11, 11 }  /* DOCMODE_IE11 */
};

static const IID NS_ICONTENTUTILS_CID =
    {0x762C4AE7,0xB923,0x422F,{0xB9,0x7E,0xB9,0xBF,0xC1,0xEF,0x7B,0xF0}};

static nsIContentUtils *content_utils;

static BOOL is_iexplore(void)
{
    static volatile char cache = -1;
    BOOL ret = cache;
    if(ret == -1) {
        const WCHAR *p, *name = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
        if((p = wcsrchr(name, '/'))) name = p + 1;
        if((p = wcsrchr(name, '\\'))) name = p + 1;
        ret = !wcsicmp(name, L"iexplore.exe");
        cache = ret;
    }
    return ret;
}

static PRUnichar *handle_insert_comment(HTMLDocumentNode *doc, const PRUnichar *comment)
{
    unsigned majorv = 0, minorv = 0, compat_version;
    const PRUnichar *ptr, *end;
    PRUnichar *buf;
    DWORD len;

    enum {
        CMP_EQ,
        CMP_LT,
        CMP_LTE,
        CMP_GT,
        CMP_GTE
    } cmpt = CMP_EQ;

    static const PRUnichar endifW[] = {'<','!','[','e','n','d','i','f',']'};

    if(comment[0] != '[' || comment[1] != 'i' || comment[2] != 'f')
        return NULL;

    ptr = comment+3;
    while(iswspace(*ptr))
        ptr++;

    if(ptr[0] == 'l' && ptr[1] == 't') {
        ptr += 2;
        if(*ptr == 'e') {
            cmpt = CMP_LTE;
            ptr++;
        }else {
            cmpt = CMP_LT;
        }
    }else if(ptr[0] == 'g' && ptr[1] == 't') {
        ptr += 2;
        if(*ptr == 'e') {
            cmpt = CMP_GTE;
            ptr++;
        }else {
            cmpt = CMP_GT;
        }
    }

    if(!iswspace(*ptr++))
        return NULL;
    while(iswspace(*ptr))
        ptr++;

    if(ptr[0] != 'I' || ptr[1] != 'E')
        return NULL;

    ptr +=2;
    if(!iswspace(*ptr++))
        return NULL;
    while(iswspace(*ptr))
        ptr++;

    if(!is_digit(*ptr))
        return NULL;
    while(is_digit(*ptr))
        majorv = majorv*10 + (*ptr++ - '0');

    if(*ptr == '.') {
        ptr++;
        if(!is_digit(*ptr))
            return NULL;
        while(is_digit(*ptr))
            minorv = minorv*10 + (*ptr++ - '0');
    }

    while(iswspace(*ptr))
        ptr++;
    if(ptr[0] != ']' || ptr[1] != '>')
        return NULL;
    ptr += 2;

    len = lstrlenW(ptr);
    if(len < ARRAY_SIZE(endifW))
        return NULL;

    end = ptr + len - ARRAY_SIZE(endifW);
    if(memcmp(end, endifW, sizeof(endifW)))
        return NULL;

    compat_version = compat_mode_info[doc->document_mode].ie_version;

    switch(cmpt) {
    case CMP_EQ:
        if(compat_version == majorv && !minorv)
            break;
        return NULL;
    case CMP_LT:
        if(compat_version < majorv || (compat_version == majorv && minorv))
            break;
        return NULL;
    case CMP_LTE:
        if(compat_version <= majorv)
            break;
        return NULL;
    case CMP_GT:
        if(compat_version > majorv)
            break;
        return NULL;
    case CMP_GTE:
        if(compat_version >= majorv || (compat_version == majorv && !minorv))
            break;
        return NULL;
    }

    buf = heap_alloc((end-ptr+1)*sizeof(WCHAR));
    if(!buf)
        return NULL;

    memcpy(buf, ptr, (end-ptr)*sizeof(WCHAR));
    buf[end-ptr] = 0;

    return buf;
}

static nsresult run_insert_comment(HTMLDocumentNode *doc, nsISupports *comment_iface, nsISupports *arg2)
{
    const PRUnichar *comment;
    nsIDOMComment *nscomment;
    PRUnichar *replace_html;
    nsAString comment_str;
    nsresult nsres;

    nsres = nsISupports_QueryInterface(comment_iface, &IID_nsIDOMComment, (void**)&nscomment);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMComment iface:%08lx\n", nsres);
        return nsres;
    }

    nsAString_Init(&comment_str, NULL);
    nsres = nsIDOMComment_GetData(nscomment, &comment_str);
    if(NS_FAILED(nsres))
        return nsres;

    nsAString_GetData(&comment_str, &comment);
    replace_html = handle_insert_comment(doc, comment);
    nsAString_Finish(&comment_str);

    if(replace_html) {
        HRESULT hres;

        hres = replace_node_by_html(doc->nsdoc, (nsIDOMNode*)nscomment, replace_html);
        heap_free(replace_html);
        if(FAILED(hres))
            nsres = NS_ERROR_FAILURE;
    }


    nsIDOMComment_Release(nscomment);
    return nsres;
}

static nsresult run_bind_to_tree(HTMLDocumentNode *doc, nsISupports *nsiface, nsISupports *arg2)
{
    nsIDOMNode *nsnode;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", doc, nsiface);

    nsres = nsISupports_QueryInterface(nsiface, &IID_nsIDOMNode, (void**)&nsnode);
    if(NS_FAILED(nsres))
        return nsres;

    hres = get_node(nsnode, TRUE, &node);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres)) {
        ERR("Could not get node\n");
        return nsres;
    }

    if(node->vtbl->bind_to_tree)
        node->vtbl->bind_to_tree(node);

    node_release(node);
    return nsres;
}

/* Calls undocumented 69 cmd of CGID_Explorer */
static void call_explorer_69(HTMLDocumentObj *doc)
{
    IOleCommandTarget *olecmd;
    VARIANT var;
    HRESULT hres;

    if(!doc->client)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(FAILED(hres))
        return;

    VariantInit(&var);
    hres = IOleCommandTarget_Exec(olecmd, &CGID_Explorer, 69, 0, NULL, &var);
    IOleCommandTarget_Release(olecmd);
    if(SUCCEEDED(hres) && V_VT(&var) != VT_NULL)
        FIXME("handle result\n");
}

static void parse_complete(HTMLDocumentObj *doc)
{
    TRACE("(%p)\n", doc);

    if(doc->nscontainer->usermode == EDITMODE)
        init_editor(doc->basedoc.doc_node);

    call_explorer_69(doc);
    if(doc->view_sink)
        IAdviseSink_OnViewChange(doc->view_sink, DVASPECT_CONTENT, -1);
    call_property_onchanged(&doc->cp_container, 1005);
    call_explorer_69(doc);

    if(doc->webbrowser && doc->nscontainer->usermode != EDITMODE && !(doc->basedoc.window->load_flags & BINDING_REFRESH))
        IDocObjectService_FireNavigateComplete2(doc->doc_object_service, &doc->basedoc.window->base.IHTMLWindow2_iface, 0);

    /* FIXME: IE7 calls EnableModelless(TRUE), EnableModelless(FALSE) and sets interactive state here */
}

static nsresult run_end_load(HTMLDocumentNode *This, nsISupports *arg1, nsISupports *arg2)
{
    TRACE("(%p)\n", This);

    if(!This->basedoc.doc_obj)
        return NS_OK;

    if(This == This->basedoc.doc_obj->basedoc.doc_node) {
        /*
         * This should be done in the worker thread that parses HTML,
         * but we don't have such thread (Gecko parses HTML for us).
         */
        parse_complete(This->basedoc.doc_obj);
    }

    bind_event_scripts(This);
    set_ready_state(This->basedoc.window, READYSTATE_INTERACTIVE);
    return NS_OK;
}

static nsresult run_insert_script(HTMLDocumentNode *doc, nsISupports *script_iface, nsISupports *parser_iface)
{
    nsIDOMHTMLScriptElement *nsscript;
    HTMLScriptElement *script_elem;
    nsIParser *nsparser = NULL;
    script_queue_entry_t *iter;
    HTMLInnerWindow *window;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", doc, script_iface);

    window = doc->window;
    if(!window)
        return NS_OK;

    nsres = nsISupports_QueryInterface(script_iface, &IID_nsIDOMHTMLScriptElement, (void**)&nsscript);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMHTMLScriptElement: %08lx\n", nsres);
        return nsres;
    }

    if(parser_iface) {
        nsres = nsISupports_QueryInterface(parser_iface, &IID_nsIParser, (void**)&nsparser);
        if(NS_FAILED(nsres)) {
            ERR("Could not get nsIParser iface: %08lx\n", nsres);
            nsparser = NULL;
        }
    }

    hres = script_elem_from_nsscript(nsscript, &script_elem);
    nsIDOMHTMLScriptElement_Release(nsscript);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    if(nsparser) {
        nsIParser_BeginEvaluatingParserInsertedScript(nsparser);
        window->parser_callback_cnt++;
    }

    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    doc_insert_script(window, script_elem, TRUE);

    while(!list_empty(&window->script_queue)) {
        iter = LIST_ENTRY(list_head(&window->script_queue), script_queue_entry_t, entry);
        list_remove(&iter->entry);
        if(!iter->script->parsed)
            doc_insert_script(window, iter->script, TRUE);
        IHTMLScriptElement_Release(&iter->script->IHTMLScriptElement_iface);
        heap_free(iter);
    }

    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);

    if(nsparser) {
        window->parser_callback_cnt--;
        nsIParser_EndEvaluatingParserInsertedScript(nsparser);
        nsIParser_Release(nsparser);
    }

    IHTMLScriptElement_Release(&script_elem->IHTMLScriptElement_iface);

    return NS_OK;
}

/*
 * We may change document mode only in early stage of document lifetime.
 * Later attempts will not have an effect.
 */
compat_mode_t lock_document_mode(HTMLDocumentNode *doc)
{
    TRACE("%p: %d\n", doc, doc->document_mode);

    doc->document_mode_locked = TRUE;
    return doc->document_mode;
}

static void set_document_mode(HTMLDocumentNode *doc, compat_mode_t document_mode, BOOL lock)
{
    compat_mode_t max_compat_mode;

    if(doc->document_mode_locked) {
        WARN("attempting to set document mode %d on locked document %p\n", document_mode, doc);
        return;
    }

    TRACE("%p: %d\n", doc, document_mode);

    max_compat_mode = doc->window && doc->window->base.outer_window
        ? get_max_compat_mode(doc->window->base.outer_window->uri)
        : COMPAT_MODE_IE11;
    if(max_compat_mode < document_mode) {
        WARN("Tried to set compat mode %u higher than maximal configured %u\n",
             document_mode, max_compat_mode);
        document_mode = max_compat_mode;
    }

    doc->document_mode = document_mode;
    if(lock)
        lock_document_mode(doc);
}

static BOOL is_ua_compatible_delimiter(WCHAR c)
{
    return !c || c == ';' || c == ',' || iswspace(c);
}

const WCHAR *parse_compat_version(const WCHAR *version_string, compat_mode_t *r)
{
    DWORD version = 0;
    const WCHAR *p;

    for(p = version_string; '0' <= *p && *p <= '9'; p++)
        version = version * 10 + *p-'0';
    if(!is_ua_compatible_delimiter(*p) || p == version_string)
        return NULL;

    switch(version){
    case 5:
    case 6:
        *r = COMPAT_MODE_IE5;
        break;
    case 7:
        *r = COMPAT_MODE_IE7;
        break;
    case 8:
        *r = COMPAT_MODE_IE8;
        break;
    case 9:
        *r = COMPAT_MODE_IE9;
        break;
    case 10:
        *r = COMPAT_MODE_IE10;
        break;
    default:
        *r = version < 5 ? COMPAT_MODE_QUIRKS : COMPAT_MODE_IE11;
    }
    return p;
}

static BOOL parse_ua_compatible(const WCHAR *p, compat_mode_t *r)
{
    static const WCHAR ie_eqW[] = {'I','E','='};
    static const WCHAR edgeW[] = {'e','d','g','e'};
    compat_mode_t mode = COMPAT_MODE_INVALID;

    TRACE("%s\n", debugstr_w(p));

    if(wcsnicmp(ie_eqW, p, ARRAY_SIZE(ie_eqW)))
        return FALSE;
    p += 3;

    do {
        while(iswspace(*p)) p++;
        if(!wcsnicmp(p, edgeW, ARRAY_SIZE(edgeW))) {
            p += ARRAY_SIZE(edgeW);
            if(is_ua_compatible_delimiter(*p))
                mode = COMPAT_MODE_IE11;
            break;
        }else if(!(p = parse_compat_version(p, r)))
            break;
        if(mode < *r)
            mode = *r;
        while(iswspace(*p)) p++;
    } while(*p++ == ',');

    *r = mode;
    return mode != COMPAT_MODE_INVALID;
}

void process_document_response_headers(HTMLDocumentNode *doc, IBinding *binding)
{
    IWinInetHttpInfo *http_info;
    char buf[1024];
    DWORD size;
    HRESULT hres;

    hres = IBinding_QueryInterface(binding, &IID_IWinInetHttpInfo, (void**)&http_info);
    if(FAILED(hres)) {
        TRACE("No IWinInetHttpInfo\n");
        return;
    }

    size = sizeof(buf);
    strcpy(buf, "X-UA-Compatible");
    hres = IWinInetHttpInfo_QueryInfo(http_info, HTTP_QUERY_CUSTOM, buf, &size, NULL, NULL);
    if(hres == S_OK && size) {
        compat_mode_t document_mode;
        WCHAR *header;

        TRACE("size %lu\n", size);

        header = heap_strdupAtoW(buf);
        if(header && parse_ua_compatible(header, &document_mode)) {
            TRACE("setting document mode %d\n", document_mode);
            set_document_mode(doc, document_mode, FALSE);
        }
        heap_free(header);
    }

    IWinInetHttpInfo_Release(http_info);
}

static void process_meta_element(HTMLDocumentNode *doc, nsIDOMHTMLMetaElement *meta_element)
{
    nsAString http_equiv_str, content_str;
    nsresult nsres;

    nsAString_Init(&http_equiv_str, NULL);
    nsAString_Init(&content_str, NULL);
    nsres = nsIDOMHTMLMetaElement_GetHttpEquiv(meta_element, &http_equiv_str);
    if(NS_SUCCEEDED(nsres))
        nsres = nsIDOMHTMLMetaElement_GetContent(meta_element, &content_str);

    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *http_equiv, *content;

        nsAString_GetData(&http_equiv_str, &http_equiv);
        nsAString_GetData(&content_str, &content);

        TRACE("%s: %s\n", debugstr_w(http_equiv), debugstr_w(content));

        if(!wcsicmp(http_equiv, L"x-ua-compatible")) {
            compat_mode_t document_mode;
            if(parse_ua_compatible(content, &document_mode))
                set_document_mode(doc, document_mode, TRUE);
            else
                FIXME("Unsupported document mode %s\n", debugstr_w(content));
        }
    }

    nsAString_Finish(&http_equiv_str);
    nsAString_Finish(&content_str);
}

typedef struct nsRunnable nsRunnable;

typedef nsresult (*runnable_proc_t)(HTMLDocumentNode*,nsISupports*,nsISupports*);

struct nsRunnable {
    nsIRunnable  nsIRunnable_iface;

    LONG ref;

    runnable_proc_t proc;

    HTMLDocumentNode *doc;
    nsISupports *arg1;
    nsISupports *arg2;
};

static inline nsRunnable *impl_from_nsIRunnable(nsIRunnable *iface)
{
    return CONTAINING_RECORD(iface, nsRunnable, nsIRunnable_iface);
}

static nsresult NSAPI nsRunnable_QueryInterface(nsIRunnable *iface,
        nsIIDRef riid, void **result)
{
    nsRunnable *This = impl_from_nsIRunnable(iface);

    if(IsEqualGUID(riid, &IID_nsISupports)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = &This->nsIRunnable_iface;
    }else if(IsEqualGUID(riid, &IID_nsIRunnable)) {
        TRACE("(%p)->(IID_nsIRunnable %p)\n", This, result);
        *result = &This->nsIRunnable_iface;
    }else {
        *result = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
        return NS_NOINTERFACE;
    }

    nsISupports_AddRef((nsISupports*)*result);
    return NS_OK;
}

static nsrefcnt NSAPI nsRunnable_AddRef(nsIRunnable *iface)
{
    nsRunnable *This = impl_from_nsIRunnable(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsRunnable_Release(nsIRunnable *iface)
{
    nsRunnable *This = impl_from_nsIRunnable(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        htmldoc_release(&This->doc->basedoc);
        if(This->arg1)
            nsISupports_Release(This->arg1);
        if(This->arg2)
            nsISupports_Release(This->arg2);
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsRunnable_Run(nsIRunnable *iface)
{
    nsRunnable *This = impl_from_nsIRunnable(iface);

    return This->proc(This->doc, This->arg1, This->arg2);
}

static const nsIRunnableVtbl nsRunnableVtbl = {
    nsRunnable_QueryInterface,
    nsRunnable_AddRef,
    nsRunnable_Release,
    nsRunnable_Run
};

static void add_script_runner(HTMLDocumentNode *This, runnable_proc_t proc, nsISupports *arg1, nsISupports *arg2)
{
    nsRunnable *runnable;

    runnable = heap_alloc_zero(sizeof(*runnable));
    if(!runnable)
        return;

    runnable->nsIRunnable_iface.lpVtbl = &nsRunnableVtbl;
    runnable->ref = 1;

    htmldoc_addref(&This->basedoc);
    runnable->doc = This;
    runnable->proc = proc;

    if(arg1)
        nsISupports_AddRef(arg1);
    runnable->arg1 = arg1;

    if(arg2)
        nsISupports_AddRef(arg2);
    runnable->arg2 = arg2;

    nsIContentUtils_AddScriptRunner(content_utils, &runnable->nsIRunnable_iface);

    nsIRunnable_Release(&runnable->nsIRunnable_iface);
}

static inline HTMLDocumentNode *impl_from_nsIDocumentObserver(nsIDocumentObserver *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, nsIDocumentObserver_iface);
}

static nsresult NSAPI nsDocumentObserver_QueryInterface(nsIDocumentObserver *iface,
        nsIIDRef riid, void **result)
{
    HTMLDocumentNode *This = impl_from_nsIDocumentObserver(iface);

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports, %p)\n", This, result);
        *result = &This->nsIDocumentObserver_iface;
    }else if(IsEqualGUID(&IID_nsIMutationObserver, riid)) {
        TRACE("(%p)->(IID_nsIMutationObserver %p)\n", This, result);
        *result = &This->nsIDocumentObserver_iface;
    }else if(IsEqualGUID(&IID_nsIDocumentObserver, riid)) {
        TRACE("(%p)->(IID_nsIDocumentObserver %p)\n", This, result);
        *result = &This->nsIDocumentObserver_iface;
    }else {
        *result = NULL;
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
        return NS_NOINTERFACE;
    }

    htmldoc_addref(&This->basedoc);
    return NS_OK;
}

static nsrefcnt NSAPI nsDocumentObserver_AddRef(nsIDocumentObserver *iface)
{
    HTMLDocumentNode *This = impl_from_nsIDocumentObserver(iface);
    return htmldoc_addref(&This->basedoc);
}

static nsrefcnt NSAPI nsDocumentObserver_Release(nsIDocumentObserver *iface)
{
    HTMLDocumentNode *This = impl_from_nsIDocumentObserver(iface);
    return htmldoc_release(&This->basedoc);
}

static void NSAPI nsDocumentObserver_CharacterDataWillChange(nsIDocumentObserver *iface,
        nsIDocument *aDocument, nsIContent *aContent, void /*CharacterDataChangeInfo*/ *aInfo)
{
}

static void NSAPI nsDocumentObserver_CharacterDataChanged(nsIDocumentObserver *iface,
        nsIDocument *aDocument, nsIContent *aContent, void /*CharacterDataChangeInfo*/ *aInfo)
{
}

static void NSAPI nsDocumentObserver_AttributeWillChange(nsIDocumentObserver *iface, nsIDocument *aDocument,
        void *aElement, LONG aNameSpaceID, nsIAtom *aAttribute, LONG aModType, const nsAttrValue *aNewValue)
{
}

static void NSAPI nsDocumentObserver_AttributeChanged(nsIDocumentObserver *iface, nsIDocument *aDocument,
        void *aElement, LONG aNameSpaceID, nsIAtom *aAttribute, LONG aModType, const nsAttrValue *aOldValue)
{
}

static void NSAPI nsDocumentObserver_NativeAnonymousChildListChange(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent, cpp_bool aIsRemove)
{
}

static void NSAPI nsDocumentObserver_AttributeSetToCurrentValue(nsIDocumentObserver *iface, nsIDocument *aDocument,
        void *aElement, LONG aNameSpaceID, nsIAtom *aAttribute)
{
}

static void NSAPI nsDocumentObserver_ContentAppended(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContainer, nsIContent *aFirstNewContent, LONG aNewIndexInContainer)
{
}

static void NSAPI nsDocumentObserver_ContentInserted(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContainer, nsIContent *aChild, LONG aIndexInContainer)
{
}

static void NSAPI nsDocumentObserver_ContentRemoved(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContainer, nsIContent *aChild, LONG aIndexInContainer,
        nsIContent *aProviousSibling)
{
}

static void NSAPI nsDocumentObserver_NodeWillBeDestroyed(nsIDocumentObserver *iface, const nsINode *aNode)
{
}

static void NSAPI nsDocumentObserver_ParentChainChanged(nsIDocumentObserver *iface, nsIContent *aContent)
{
}

static void NSAPI nsDocumentObserver_BeginUpdate(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsUpdateType aUpdateType)
{
}

static void NSAPI nsDocumentObserver_EndUpdate(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsUpdateType aUpdateType)
{
}

static void NSAPI nsDocumentObserver_BeginLoad(nsIDocumentObserver *iface, nsIDocument *aDocument)
{
}

static void NSAPI nsDocumentObserver_EndLoad(nsIDocumentObserver *iface, nsIDocument *aDocument)
{
    HTMLDocumentNode *This = impl_from_nsIDocumentObserver(iface);

    TRACE("(%p)\n", This);

    if(This->skip_mutation_notif)
        return;

    This->content_ready = TRUE;
    add_script_runner(This, run_end_load, NULL, NULL);
}

static void NSAPI nsDocumentObserver_ContentStatesChanged(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent, EventStates aStateMask)
{
}

static void NSAPI nsDocumentObserver_DocumentStatesChanged(nsIDocumentObserver *iface, nsIDocument *aDocument,
        EventStates aStateMask)
{
}

static void NSAPI nsDocumentObserver_StyleSheetAdded(nsIDocumentObserver *iface, mozilla_StyleSheetHandle aStyleSheet,
        cpp_bool aDocumentSheet)
{
}

static void NSAPI nsDocumentObserver_StyleSheetRemoved(nsIDocumentObserver *iface, mozilla_StyleSheetHandle aStyleSheet,
        cpp_bool aDocumentSheet)
{
}

static void NSAPI nsDocumentObserver_StyleSheetApplicableStateChanged(nsIDocumentObserver *iface,
        mozilla_StyleSheetHandle aStyleSheet)
{
}

static void NSAPI nsDocumentObserver_StyleRuleChanged(nsIDocumentObserver *iface, mozilla_StyleSheetHandle aStyleSheet)
{
}

static void NSAPI nsDocumentObserver_StyleRuleAdded(nsIDocumentObserver *iface, mozilla_StyleSheetHandle aStyleSheet)
{
}

static void NSAPI nsDocumentObserver_StyleRuleRemoved(nsIDocumentObserver *iface, mozilla_StyleSheetHandle aStyleSheet)
{
}

static void NSAPI nsDocumentObserver_BindToDocument(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent)
{
    HTMLDocumentNode *This = impl_from_nsIDocumentObserver(iface);
    nsIDOMHTMLIFrameElement *nsiframe;
    nsIDOMHTMLFrameElement *nsframe;
    nsIDOMHTMLScriptElement *nsscript;
    nsIDOMHTMLMetaElement *nsmeta;
    nsIDOMElement *nselem;
    nsIDOMComment *nscomment;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, aDocument, aContent);

    if(This->document_mode < COMPAT_MODE_IE10) {
        nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMComment, (void**)&nscomment);
        if(NS_SUCCEEDED(nsres)) {
            TRACE("comment node\n");

            add_script_runner(This, run_insert_comment, (nsISupports*)nscomment, NULL);
            nsIDOMComment_Release(nscomment);
            return;
        }
    }

    if(This->document_mode == COMPAT_MODE_QUIRKS) {
        nsIDOMDocumentType *nsdoctype;

        nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMDocumentType, (void**)&nsdoctype);
        if(NS_SUCCEEDED(nsres)) {
            compat_mode_t mode = COMPAT_MODE_IE7;

            TRACE("doctype node\n");

            /* Native mshtml hardcodes special behavior for iexplore.exe here. The feature control registry
               keys under HKLM or HKCU\Software\Microsoft\Internet Explorer\Main\FeatureControl are not used
               in this case (neither in Wow6432Node), although FEATURE_BROWSER_EMULATION does override this,
               but it is not set by default on native, and the behavior is still different. This was tested
               by removing all iexplore.exe values from any FeatureControl subkeys, and renaming the test
               executable to iexplore.exe, which changed its default compat mode in such cases. */
            if(This->window && This->window->base.outer_window && is_iexplore()) {
                HTMLOuterWindow *window = This->window->base.outer_window;
                DWORD zone;
                HRESULT hres;

                /* Internet URL zone is treated differently and defaults to the latest supported mode. */
                hres = IInternetSecurityManager_MapUrlToZone(get_security_manager(), window->url, &zone, 0);
                if(SUCCEEDED(hres) && zone == URLZONE_INTERNET)
                    mode = COMPAT_MODE_IE11;
            }

            set_document_mode(This, mode, FALSE);
            nsIDOMDocumentType_Release(nsdoctype);
        }
    }

    nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMElement, (void**)&nselem);
    if(NS_FAILED(nsres))
        return;

    check_event_attr(This, nselem);
    nsIDOMElement_Release(nselem);

    nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMHTMLIFrameElement, (void**)&nsiframe);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("iframe node\n");

        add_script_runner(This, run_bind_to_tree, (nsISupports*)nsiframe, NULL);
        nsIDOMHTMLIFrameElement_Release(nsiframe);
        return;
    }

    nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMHTMLFrameElement, (void**)&nsframe);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("frame node\n");

        add_script_runner(This, run_bind_to_tree, (nsISupports*)nsframe, NULL);
        nsIDOMHTMLFrameElement_Release(nsframe);
        return;
    }

    nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMHTMLScriptElement, (void**)&nsscript);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("script element\n");

        add_script_runner(This, run_bind_to_tree, (nsISupports*)nsscript, NULL);
        nsIDOMHTMLScriptElement_Release(nsscript);
        return;
    }

    nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMHTMLMetaElement, (void**)&nsmeta);
    if(NS_SUCCEEDED(nsres)) {
        process_meta_element(This, nsmeta);
        nsIDOMHTMLMetaElement_Release(nsmeta);
    }
}

static void NSAPI nsDocumentObserver_AttemptToExecuteScript(nsIDocumentObserver *iface, nsIContent *aContent,
        nsIParser *aParser, cpp_bool *aBlock)
{
    HTMLDocumentNode *This = impl_from_nsIDocumentObserver(iface);
    nsIDOMHTMLScriptElement *nsscript;
    nsresult nsres;

    TRACE("(%p)->(%p %p %p)\n", This, aContent, aParser, aBlock);

    nsres = nsIContent_QueryInterface(aContent, &IID_nsIDOMHTMLScriptElement, (void**)&nsscript);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("script node\n");

        lock_document_mode(This);
        add_script_runner(This, run_insert_script, (nsISupports*)nsscript, (nsISupports*)aParser);
        nsIDOMHTMLScriptElement_Release(nsscript);
    }
}

static const nsIDocumentObserverVtbl nsDocumentObserverVtbl = {
    nsDocumentObserver_QueryInterface,
    nsDocumentObserver_AddRef,
    nsDocumentObserver_Release,
    nsDocumentObserver_CharacterDataWillChange,
    nsDocumentObserver_CharacterDataChanged,
    nsDocumentObserver_AttributeWillChange,
    nsDocumentObserver_AttributeChanged,
    nsDocumentObserver_NativeAnonymousChildListChange,
    nsDocumentObserver_AttributeSetToCurrentValue,
    nsDocumentObserver_ContentAppended,
    nsDocumentObserver_ContentInserted,
    nsDocumentObserver_ContentRemoved,
    nsDocumentObserver_NodeWillBeDestroyed,
    nsDocumentObserver_ParentChainChanged,
    nsDocumentObserver_BeginUpdate,
    nsDocumentObserver_EndUpdate,
    nsDocumentObserver_BeginLoad,
    nsDocumentObserver_EndLoad,
    nsDocumentObserver_ContentStatesChanged,
    nsDocumentObserver_DocumentStatesChanged,
    nsDocumentObserver_StyleSheetAdded,
    nsDocumentObserver_StyleSheetRemoved,
    nsDocumentObserver_StyleSheetApplicableStateChanged,
    nsDocumentObserver_StyleRuleChanged,
    nsDocumentObserver_StyleRuleAdded,
    nsDocumentObserver_StyleRuleRemoved,
    nsDocumentObserver_BindToDocument,
    nsDocumentObserver_AttemptToExecuteScript
};

void init_document_mutation(HTMLDocumentNode *doc)
{
    nsIDocument *nsdoc;
    nsresult nsres;

    doc->nsIDocumentObserver_iface.lpVtbl = &nsDocumentObserverVtbl;

    nsres = nsIDOMHTMLDocument_QueryInterface(doc->nsdoc, &IID_nsIDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDocument: %08lx\n", nsres);
        return;
    }

    nsIContentUtils_AddDocumentObserver(content_utils, nsdoc, &doc->nsIDocumentObserver_iface);
    nsIDocument_Release(nsdoc);
}

void release_document_mutation(HTMLDocumentNode *doc)
{
    nsIDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMHTMLDocument_QueryInterface(doc->nsdoc, &IID_nsIDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDocument: %08lx\n", nsres);
        return;
    }

    nsIContentUtils_RemoveDocumentObserver(content_utils, nsdoc, &doc->nsIDocumentObserver_iface);
    nsIDocument_Release(nsdoc);
}

JSContext *get_context_from_document(nsIDOMHTMLDocument *nsdoc)
{
    nsIDocument *doc;
    JSContext *ctx;
    nsresult nsres;

    nsres = nsIDOMHTMLDocument_QueryInterface(nsdoc, &IID_nsIDocument, (void**)&doc);
    assert(nsres == NS_OK);

    ctx = nsIContentUtils_GetContextFromDocument(content_utils, doc);
    nsIDocument_Release(doc);

    TRACE("ret %p\n", ctx);
    return ctx;
}

void init_mutation(nsIComponentManager *component_manager)
{
    nsIFactory *factory;
    nsresult nsres;

    if(!component_manager) {
        if(content_utils) {
            nsIContentUtils_Release(content_utils);
            content_utils = NULL;
        }
        return;
    }

    nsres = nsIComponentManager_GetClassObject(component_manager, &NS_ICONTENTUTILS_CID,
            &IID_nsIFactory, (void**)&factory);
    if(NS_FAILED(nsres)) {
        ERR("Could not create nsIContentUtils service: %08lx\n", nsres);
        return;
    }

    nsres = nsIFactory_CreateInstance(factory, NULL, &IID_nsIContentUtils, (void**)&content_utils);
    nsIFactory_Release(factory);
    if(NS_FAILED(nsres))
        ERR("Could not create nsIContentUtils instance: %08lx\n", nsres);
}
