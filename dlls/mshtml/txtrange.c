/*
 * Copyright 2006-2007 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const WCHAR brW[] = {'b','r',0};

typedef struct {
    const IHTMLTxtRangeVtbl *lpHTMLTxtRangeVtbl;

    LONG ref;

    nsIDOMRange *nsrange;
    HTMLDocument *doc;

    struct list entry;
} HTMLTxtRange;

#define HTMLTXTRANGE(x)  ((IHTMLTxtRange*)  &(x)->lpHTMLTxtRangeVtbl)

typedef struct {
    WCHAR *buf;
    DWORD len;
    DWORD size;
} wstrbuf_t;

typedef struct {
    PRUint16 type;
    nsIDOMNode *node;
    PRUint32 off;
    nsAString str;
    const PRUnichar *p;
} dompos_t;

typedef enum {
    RU_UNKNOWN,
    RU_CHAR,
    RU_WORD,
    RU_SENTENCE,
    RU_TEXTEDIT
} range_unit_t;

static HTMLTxtRange *get_range_object(HTMLDocument *doc, IHTMLTxtRange *iface)
{
    HTMLTxtRange *iter;

    LIST_FOR_EACH_ENTRY(iter, &doc->range_list, HTMLTxtRange, entry) {
        if(HTMLTXTRANGE(iter) == iface)
            return iter;
    }

    ERR("Could not find range in document\n");
    return NULL;
}

static range_unit_t string_to_unit(LPCWSTR str)
{
    static const WCHAR characterW[] =
        {'c','h','a','r','a','c','t','e','r',0};
    static const WCHAR wordW[] =
        {'w','o','r','d',0};
    static const WCHAR sentenceW[] =
        {'s','e','n','t','e','n','c','e',0};
    static const WCHAR texteditW[] =
        {'t','e','x','t','e','d','i','t',0};

    if(!strcmpiW(str, characterW))  return RU_CHAR;
    if(!strcmpiW(str, wordW))       return RU_WORD;
    if(!strcmpiW(str, sentenceW))   return RU_SENTENCE;
    if(!strcmpiW(str, texteditW))   return RU_TEXTEDIT;

    return RU_UNKNOWN;
}

static int string_to_nscmptype(LPCWSTR str)
{
    static const WCHAR seW[] = {'S','t','a','r','t','T','o','E','n','d',0};
    static const WCHAR ssW[] = {'S','t','a','r','t','T','o','S','t','a','r','t',0};
    static const WCHAR esW[] = {'E','n','d','T','o','S','t','a','r','t',0};
    static const WCHAR eeW[] = {'E','n','d','T','o','E','n','d',0};

    if(!strcmpiW(str, seW))  return NS_START_TO_END;
    if(!strcmpiW(str, ssW))  return NS_START_TO_START;
    if(!strcmpiW(str, esW))  return NS_END_TO_START;
    if(!strcmpiW(str, eeW))  return NS_END_TO_END;

    return -1;
}

static PRUint16 get_node_type(nsIDOMNode *node)
{
    PRUint16 type = 0xfff;

    if(node)
        nsIDOMNode_GetNodeType(node, &type);

    return type;
}

static BOOL is_br_node(nsIDOMNode *node)
{
    nsIDOMElement *elem;
    nsAString tag_str;
    const PRUnichar *tag;
    BOOL ret = FALSE;
    nsresult nsres;

    nsres = nsIDOMNode_QueryInterface(node, &IID_nsIDOMElement, (void**)&elem);
    if(NS_FAILED(nsres))
        return FALSE;

    nsAString_Init(&tag_str, NULL);
    nsIDOMElement_GetTagName(elem, &tag_str);
    nsIDOMElement_Release(elem);
    nsAString_GetData(&tag_str, &tag, 0);

    if(!strcmpiW(tag, brW))
        ret = TRUE;

    nsAString_Finish(&tag_str);

    return ret;
}

static inline void wstrbuf_init(wstrbuf_t *buf)
{
    buf->len = 0;
    buf->size = 16;
    buf->buf = mshtml_alloc(buf->size * sizeof(WCHAR));
    *buf->buf = 0;
}

static inline void wstrbuf_finish(wstrbuf_t *buf)
{
    mshtml_free(buf->buf);
}

static void wstrbuf_append_len(wstrbuf_t *buf, LPCWSTR str, int len)
{
    if(buf->len+len >= buf->size) {
        buf->size = 2*buf->len+len;
        buf->buf = mshtml_realloc(buf->buf, buf->size * sizeof(WCHAR));
    }

    memcpy(buf->buf+buf->len, str, len*sizeof(WCHAR));
    buf->len += len;
    buf->buf[buf->len] = 0;
}

static inline void wstrbuf_append(wstrbuf_t *buf, LPCWSTR str)
{
    wstrbuf_append_len(buf, str, strlenW(str));
}

static void wstrbuf_append_node(wstrbuf_t *buf, nsIDOMNode *node)
{

    switch(get_node_type(node)) {
    case TEXT_NODE: {
        nsIDOMText *nstext;
        nsAString data_str;
        const PRUnichar *data;

        nsIDOMNode_QueryInterface(node, &IID_nsIDOMText, (void**)&nstext);

        nsAString_Init(&data_str, NULL);
        nsIDOMText_GetData(nstext, &data_str);
        nsAString_GetData(&data_str, &data, NULL);
        wstrbuf_append(buf, data);
        nsAString_Finish(&data_str);

       nsIDOMText_Release(nstext);

        break;
    }
    case ELEMENT_NODE:
        if(is_br_node(node)) {
            static const WCHAR endlW[] = {'\r','\n'};
            wstrbuf_append_len(buf, endlW, 2);
        }
    }
}

static BOOL fill_nodestr(dompos_t *pos)
{
    nsIDOMText *text;
    nsresult nsres;

    if(pos->type != TEXT_NODE)
        return FALSE;

    nsres = nsIDOMNode_QueryInterface(pos->node, &IID_nsIDOMText, (void**)&text);
    if(NS_FAILED(nsres))
        return FALSE;

    nsAString_Init(&pos->str, NULL);
    nsIDOMText_GetData(text, &pos->str);
    nsIDOMText_Release(text);
    nsAString_GetData(&pos->str, &pos->p, NULL);

    return TRUE;
}

static nsIDOMNode *next_node(nsIDOMNode *iter)
{
    nsIDOMNode *ret, *tmp;
    nsresult nsres;

    if(!iter)
        return NULL;

    nsres = nsIDOMNode_GetFirstChild(iter, &ret);
    if(NS_SUCCEEDED(nsres) && ret)
        return ret;

    nsIDOMNode_AddRef(iter);

    do {
        nsres = nsIDOMNode_GetNextSibling(iter, &ret);
        if(NS_SUCCEEDED(nsres) && ret) {
            nsIDOMNode_Release(iter);
            return ret;
        }

        nsres = nsIDOMNode_GetParentNode(iter, &tmp);
        nsIDOMNode_Release(iter);
        iter = tmp;
    }while(NS_SUCCEEDED(nsres) && iter);

    return NULL;
}

static nsIDOMNode *prev_node(HTMLTxtRange *This, nsIDOMNode *iter)
{
    nsIDOMNode *ret, *tmp;
    nsresult nsres;

    if(!iter) {
        nsIDOMHTMLDocument *nshtmldoc;
        nsIDOMHTMLElement *nselem;
        nsIDOMDocument *nsdoc;

        nsIWebNavigation_GetDocument(This->doc->nscontainer->navigation, &nsdoc);
        nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMHTMLDocument, (void**)&nshtmldoc);
        nsIDOMDocument_Release(nsdoc);
        nsIDOMHTMLDocument_GetBody(nshtmldoc, &nselem);
        nsIDOMHTMLDocument_Release(nshtmldoc);

        nsIDOMElement_GetLastChild(nselem, &tmp);
        if(!tmp)
            return (nsIDOMNode*)nselem;

        while(tmp) {
            ret = tmp;
            nsIDOMNode_GetLastChild(ret, &tmp);
        }

        nsIDOMElement_Release(nselem);

        return ret;
    }

    nsres = nsIDOMNode_GetLastChild(iter, &ret);
    if(NS_SUCCEEDED(nsres) && ret)
        return ret;

    nsIDOMNode_AddRef(iter);

    do {
        nsres = nsIDOMNode_GetPreviousSibling(iter, &ret);
        if(NS_SUCCEEDED(nsres) && ret) {
            nsIDOMNode_Release(iter);
            return ret;
        }

        nsres = nsIDOMNode_GetParentNode(iter, &tmp);
        nsIDOMNode_Release(iter);
        iter = tmp;
    }while(NS_SUCCEEDED(nsres) && iter);

    return NULL;
}

static nsIDOMNode *get_child_node(nsIDOMNode *node, PRUint32 off)
{
    nsIDOMNodeList *node_list;
    nsIDOMNode *ret = NULL;

    nsIDOMNode_GetChildNodes(node, &node_list);
    nsIDOMNodeList_Item(node_list, off, &ret);
    nsIDOMNodeList_Release(node_list);

    return ret;
}

static void get_cur_pos(HTMLTxtRange *This, BOOL start, dompos_t *pos)
{
    nsIDOMNode *node;
    PRInt32 off;

    pos->p = NULL;

    if(!start) {
        PRBool collapsed;
        nsIDOMRange_GetCollapsed(This->nsrange, &collapsed);
        start = collapsed;
    }

    if(start) {
        nsIDOMRange_GetStartContainer(This->nsrange, &node);
        nsIDOMRange_GetStartOffset(This->nsrange, &off);
    }else {
        nsIDOMRange_GetEndContainer(This->nsrange, &node);
        nsIDOMRange_GetEndOffset(This->nsrange, &off);
    }

    pos->type = get_node_type(node);
    if(pos->type == ELEMENT_NODE) {
        if(start) {
            pos->node = get_child_node(node, off);
            pos->off = 0;
        }else {
            pos->node = off ? get_child_node(node, off-1) : prev_node(This, node);
            pos->off = -1;
        }

        pos->type = get_node_type(pos->node);
        nsIDOMNode_Release(node);
    }else if(start) {
        pos->node = node;
        pos->off = off;
    }else if(off) {
        pos->node = node;
        pos->off = off-1;
    }else {
        pos->node = prev_node(This, node);
        pos->off = -1;
        nsIDOMNode_Release(node);
    }

    if(pos->type == TEXT_NODE)
        fill_nodestr(pos);
}

static void set_range_pos(HTMLTxtRange *This, BOOL start, dompos_t *pos)
{
    nsresult nsres;

    if(start) {
        if(pos->type == TEXT_NODE)
            nsres = nsIDOMRange_SetStart(This->nsrange, pos->node, pos->off);
        else
            nsres = nsIDOMRange_SetStartBefore(This->nsrange, pos->node);
    }else {
        if(pos->type == TEXT_NODE)
            nsres = nsIDOMRange_SetEnd(This->nsrange, pos->node, pos->off+1);
        else
            nsres = nsIDOMRange_SetEndAfter(This->nsrange, pos->node);
    }

    if(NS_FAILED(nsres))
        ERR("failed: %p %08x\n", pos->node, nsres);
}

static inline void dompos_release(dompos_t *pos)
{
    if(pos->node)
        nsIDOMNode_Release(pos->node);

    if(pos->p)
        nsAString_Finish(&pos->str);
}

static inline void dompos_addref(dompos_t *pos)
{
    if(pos->node)
        nsIDOMNode_AddRef(pos->node);

    if(pos->type == TEXT_NODE)
        fill_nodestr(pos);
}

static void range_to_string(HTMLTxtRange *This, wstrbuf_t *buf)
{
    nsIDOMNode *iter, *tmp;
    dompos_t start_pos, end_pos;
    PRBool collapsed;

    nsIDOMRange_GetCollapsed(This->nsrange, &collapsed);
    if(collapsed) {
        wstrbuf_finish(buf);
        buf->buf = NULL;
        buf->size = 0;
        return;
    }

    get_cur_pos(This, FALSE, &end_pos);
    get_cur_pos(This, TRUE, &start_pos);

    if(start_pos.type == TEXT_NODE) {
        if(start_pos.node == end_pos.node) {
            wstrbuf_append_len(buf, start_pos.p+start_pos.off, end_pos.off-start_pos.off+1);
            iter = start_pos.node;
            nsIDOMNode_AddRef(iter);
        }else {
            wstrbuf_append(buf, start_pos.p+start_pos.off);
            iter = next_node(start_pos.node);
        }
    }else {
        iter = start_pos.node;
        nsIDOMNode_AddRef(iter);
    }

    while(iter != end_pos.node) {
        wstrbuf_append_node(buf, iter);
        tmp = next_node(iter);
        nsIDOMNode_Release(iter);
        iter = tmp;
    }

    nsIDOMNode_AddRef(end_pos.node);

    if(start_pos.node != end_pos.node && !is_br_node(end_pos.node))
        wstrbuf_append_len(buf, end_pos.p, end_pos.off+1);

    nsIDOMNode_Release(iter);
    dompos_release(&start_pos);
    dompos_release(&end_pos);
}

static WCHAR get_pos_char(const dompos_t *pos)
{
    switch(pos->type) {
    case TEXT_NODE:
        return pos->p[pos->off];
    case ELEMENT_NODE:
        if(is_br_node(pos->node))
            return '\n';
    }

    return 0;
}

static WCHAR next_char(const dompos_t *pos, dompos_t *new_pos)
{
    nsIDOMNode *iter, *tmp;

    if(pos->type == TEXT_NODE && pos->off != -1 && pos->p[pos->off+1]) {
        *new_pos = *pos;
        new_pos->off++;
        dompos_addref(new_pos);
        return new_pos->p[new_pos->off];
    }

    iter = next_node(pos->node);
    if(!iter)
        return 0;

    while(1) {
        switch(get_node_type(iter)) {
        case TEXT_NODE:
            new_pos->node = iter;
            new_pos->type = TEXT_NODE;
            new_pos->off = 0;
            fill_nodestr(new_pos);
            return *new_pos->p;

        case ELEMENT_NODE:
            if(!is_br_node(iter))
                break;

            new_pos->node = iter;
            new_pos->type = ELEMENT_NODE;
            new_pos->off = 0;
            new_pos->p = NULL;
            return '\n';
        }

        tmp = iter;
        iter = next_node(iter);
        nsIDOMNode_Release(tmp);

        if(!iter)
            break;
    }

    return 0;
}

static WCHAR prev_char(HTMLTxtRange *This, const dompos_t *pos, dompos_t *new_pos)
{
    nsIDOMNode *iter, *tmp;

    if(pos->type == TEXT_NODE && pos->off > 0) {
        *new_pos = *pos;
        new_pos->off--;
        dompos_addref(new_pos);
        return new_pos->p[new_pos->off];
    }

    iter = prev_node(This, pos->node);
    if(!iter)
        return 0;

    while(1) {
        switch(get_node_type(iter)) {
        case TEXT_NODE:
            new_pos->node = iter;
            new_pos->type = TEXT_NODE;
            fill_nodestr(new_pos);
            new_pos->off = strlenW(new_pos->p)-1;
            return new_pos->p[new_pos->off];

        case ELEMENT_NODE:
            if(!is_br_node(iter))
                break;

            new_pos->node = iter;
            new_pos->type = ELEMENT_NODE;
            new_pos->off = 0;
            new_pos->p = NULL;
            return '\n';
        }

        tmp = iter;
        iter = prev_node(This, iter);
        nsIDOMNode_Release(tmp);

        if(!iter)
            break;
    }

    *new_pos = *pos;
    dompos_addref(new_pos);
    return 0;
}

static long move_next_chars(long cnt, const dompos_t *pos, BOOL col, dompos_t *new_pos)
{
    dompos_t iter, tmp;
    long ret = 0;
    WCHAR c;

    if(col)
        ret++;

    if(ret >= cnt) {
        *new_pos = *pos;
        dompos_addref(new_pos);
        return ret;
    }

    c = next_char(pos, &iter);
    ret++;

    while(ret < cnt) {
        tmp = iter;
        c = next_char(&tmp, &iter);
        if(!c) {
            iter = tmp;
            break;
        }
        ret++;
        dompos_release(&tmp);
    }

    *new_pos = iter;
    return ret;
}

static long move_prev_chars(HTMLTxtRange *This, long cnt, const dompos_t *pos, BOOL end, dompos_t *new_pos)
{
    dompos_t iter, tmp;
    long ret = 0;
    WCHAR c;

    c = prev_char(This, pos, &iter);
    if(c)
        ret++;

    while(c && ret < cnt) {
        tmp = iter;
        c = prev_char(This, &tmp, &iter);
        if(!c) {
            iter = tmp;
            if(end)
                ret++;
            break;
        }
        ret++;
        dompos_release(&tmp);
    }

    *new_pos = iter;
    return ret;
}

static BOOL find_next_space(const dompos_t *pos, BOOL first_space, dompos_t *ret)
{
    dompos_t iter, tmp;
    WCHAR c;

    if(first_space) {
        c = get_pos_char(pos);
        if(c && isspaceW(c)) {
            *ret = *pos;
            dompos_addref(ret);
            return FALSE;
        }
    }

    c = next_char(pos, &iter);
    if(!c) {
        *ret = *pos;
        dompos_addref(ret);
        return FALSE;
    }

    while(!isspaceW(c)) {
        tmp = iter;
        c = next_char(&tmp, &iter);
        if(!c) {
            iter = tmp;
            break;
        }
        dompos_release(&tmp);
    }

    *ret = iter;
    return TRUE;
}

static long find_prev_space(HTMLTxtRange *This, const dompos_t *pos, BOOL first_space, dompos_t *ret)
{
    dompos_t iter, tmp;
    WCHAR c;

    c = prev_char(This, pos, &iter);
    if(!c || (first_space && isspaceW(c))) {
        *ret = *pos;
        dompos_addref(ret);
        return FALSE;
    }

    while(1) {
        tmp = iter;
        c = prev_char(This, &tmp, &iter);
        if(!c || isspaceW(c)) {
            dompos_release(&iter);
            break;
        }
        dompos_release(&tmp);
    }

    *ret = tmp;
    return TRUE;
}

static long move_next_words(long cnt, const dompos_t *pos, dompos_t *new_pos)
{
    dompos_t iter, tmp;
    long ret = 0;

    iter = *pos;
    dompos_addref(&iter);

    while(ret < cnt) {
        if(!find_next_space(&iter, FALSE, &tmp))
            break;

        ret++;
        dompos_release(&iter);
        iter = tmp;
    }

    *new_pos = iter;
    return ret;
}

static long move_prev_words(HTMLTxtRange *This, long cnt, const dompos_t *pos, dompos_t *new_pos)
{
    dompos_t iter, tmp;
    long ret = 0;

    iter = *pos;
    dompos_addref(&iter);

    while(ret < cnt) {
        if(!find_prev_space(This, &iter, FALSE, &tmp))
            break;

        dompos_release(&iter);
        iter = tmp;
        ret++;
    }

    *new_pos = iter;
    return ret;
}

#define HTMLTXTRANGE_THIS(iface) DEFINE_THIS(HTMLTxtRange, HTMLTxtRange, iface)

static HRESULT WINAPI HTMLTxtRange_QueryInterface(IHTMLTxtRange *iface, REFIID riid, void **ppv)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLTXTRANGE(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLTXTRANGE(This);
    }else if(IsEqualGUID(&IID_IHTMLTxtRange, riid)) {
        TRACE("(%p)->(IID_IHTMLTxtRange %p)\n", This, ppv);
        *ppv = HTMLTXTRANGE(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLTxtRange_AddRef(IHTMLTxtRange *iface)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLTxtRange_Release(IHTMLTxtRange *iface)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nsrange)
            nsISelection_Release(This->nsrange);
        if(This->doc)
            list_remove(&This->entry);
        mshtml_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLTxtRange_GetTypeInfoCount(IHTMLTxtRange *iface, UINT *pctinfo)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_GetTypeInfo(IHTMLTxtRange *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_GetIDsOfNames(IHTMLTxtRange *iface, REFIID riid,
                                                 LPOLESTR *rgszNames, UINT cNames,
                                                 LCID lcid, DISPID *rgDispId)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_Invoke(IHTMLTxtRange *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_get_htmlText(IHTMLTxtRange *iface, BSTR *p)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(This->nsrange) {
        nsIDOMDocumentFragment *fragment;
        nsresult nsres;

        nsres = nsIDOMRange_CloneContents(This->nsrange, &fragment);
        if(NS_SUCCEEDED(nsres)) {
            const PRUnichar *nstext;
            nsAString nsstr;

            nsAString_Init(&nsstr, NULL);
            nsnode_to_nsstring((nsIDOMNode*)fragment, &nsstr);
            nsIDOMDocumentFragment_Release(fragment);

            nsAString_GetData(&nsstr, &nstext, NULL);
            *p = SysAllocString(nstext);

            nsAString_Finish(&nsstr);
        }
    }

    if(!*p) {
        const WCHAR emptyW[] = {0};
        *p = SysAllocString(emptyW);
    }

    TRACE("return %s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_put_text(IHTMLTxtRange *iface, BSTR v)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    nsIDOMDocument *nsdoc;
    nsIDOMText *text_node;
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->doc)
        return MSHTML_E_NODOC;

    nsres = nsIWebNavigation_GetDocument(This->doc->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("GetDocument failed: %08x\n", nsres);
        return S_OK;
    }

    nsAString_Init(&text_str, v);
    nsres = nsIDOMDocument_CreateTextNode(nsdoc, &text_str, &text_node);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return S_OK;
    }
    nsres = nsIDOMRange_DeleteContents(This->nsrange);
    if(NS_FAILED(nsres))
        ERR("DeleteContents failed: %08x\n", nsres);

    nsres = nsIDOMRange_InsertNode(This->nsrange, (nsIDOMNode*)text_node);
    if(NS_FAILED(nsres))
        ERR("InsertNode failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_get_text(IHTMLTxtRange *iface, BSTR *p)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    wstrbuf_t buf;

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    if(!This->nsrange)
        return S_OK;

    wstrbuf_init(&buf);
    range_to_string(This, &buf);
    if(buf.buf)
        *p = SysAllocString(buf.buf);
    wstrbuf_finish(&buf);

    TRACE("ret %s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_parentElement(IHTMLTxtRange *iface, IHTMLElement **parent)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    nsIDOMNode *nsnode, *tmp;
    HTMLDOMNode *node;

    TRACE("(%p)->(%p)\n", This, parent);

    nsIDOMRange_GetCommonAncestorContainer(This->nsrange, &nsnode);
    while(nsnode && get_node_type(nsnode) != ELEMENT_NODE) {
        nsIDOMNode_GetParentNode(nsnode, &tmp);
        nsIDOMNode_Release(nsnode);
        nsnode = tmp;
    }

    if(!nsnode) {
        *parent = NULL;
        return S_OK;
    }

    node = get_node(This->doc, nsnode);
    nsIDOMNode_Release(nsnode);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(node), &IID_IHTMLElement, (void**)parent);
}

static HRESULT WINAPI HTMLTxtRange_duplicate(IHTMLTxtRange *iface, IHTMLTxtRange **Duplicate)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    nsIDOMRange *nsrange = NULL;

    TRACE("(%p)->(%p)\n", This, Duplicate);

    nsIDOMRange_CloneRange(This->nsrange, &nsrange);
    *Duplicate = HTMLTxtRange_Create(This->doc, nsrange);
    nsIDOMRange_Release(nsrange);

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_inRange(IHTMLTxtRange *iface, IHTMLTxtRange *Range,
        VARIANT_BOOL *InRange)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    HTMLTxtRange *src_range;
    PRInt16 nsret = 0;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, Range, InRange);

    *InRange = VARIANT_FALSE;

    src_range = get_range_object(This->doc, Range);
    if(!src_range)
        return E_FAIL;

    nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_START_TO_START,
            src_range->nsrange, &nsret);
    if(NS_SUCCEEDED(nsres) && nsret <= 0) {
        nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_END_TO_END,
                src_range->nsrange, &nsret);
        if(NS_SUCCEEDED(nsres) && nsret >= 0)
            *InRange = VARIANT_TRUE;
    }

    if(NS_FAILED(nsres))
        ERR("CompareBoundaryPoints failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_isEqual(IHTMLTxtRange *iface, IHTMLTxtRange *Range,
        VARIANT_BOOL *IsEqual)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    HTMLTxtRange *src_range;
    PRInt16 nsret = 0;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, Range, IsEqual);

    *IsEqual = VARIANT_FALSE;

    src_range = get_range_object(This->doc, Range);
    if(!src_range)
        return E_FAIL;

    nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_START_TO_START,
            src_range->nsrange, &nsret);
    if(NS_SUCCEEDED(nsres) && !nsret) {
        nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_END_TO_END,
                src_range->nsrange, &nsret);
        if(NS_SUCCEEDED(nsres) && !nsret)
            *IsEqual = VARIANT_TRUE;
    }

    if(NS_FAILED(nsres))
        ERR("CompareBoundaryPoints failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_scrollIntoView(IHTMLTxtRange *iface, VARIANT_BOOL fStart)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fStart);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_collapse(IHTMLTxtRange *iface, VARIANT_BOOL Start)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);

    TRACE("(%p)->(%x)\n", This, Start);

    nsIDOMRange_Collapse(This->nsrange, Start != VARIANT_FALSE);
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_expand(IHTMLTxtRange *iface, BSTR Unit, VARIANT_BOOL *Success)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    range_unit_t unit;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(Unit), Success);

    unit = string_to_unit(Unit);
    if(unit == RU_UNKNOWN)
        return E_INVALIDARG;

    switch(unit) {
    case RU_WORD: {
        dompos_t end_pos, start_pos, new_pos;

        *Success = VARIANT_FALSE;

        get_cur_pos(This, TRUE, &start_pos);
        get_cur_pos(This, FALSE, &end_pos);
        if(find_next_space(&end_pos, TRUE, &new_pos)) {
            set_range_pos(This, FALSE, &new_pos);
            *Success = VARIANT_TRUE;
        }
        dompos_release(&new_pos);

        if(find_prev_space(This, &start_pos, TRUE, &new_pos)) {
            set_range_pos(This, TRUE, &new_pos);
            *Success = VARIANT_TRUE;
        }

        dompos_release(&new_pos);
        dompos_release(&end_pos);

        break;
    }
    default:
        FIXME("Unimplemented unit %s\n", debugstr_w(Unit));
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_move(IHTMLTxtRange *iface, BSTR Unit,
        long Count, long *ActualCount)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    range_unit_t unit;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(Unit), Count, ActualCount);

    unit = string_to_unit(Unit);
    if(unit == RU_UNKNOWN)
        return E_INVALIDARG;

    IHTMLTxtRange_collapse(HTMLTXTRANGE(This), TRUE);

    if(!Count) {
        *ActualCount = 0;
        return S_OK;
    }

    switch(unit) {
    case RU_CHAR: {
        dompos_t cur_pos, new_pos;

        get_cur_pos(This, TRUE, &cur_pos);

        if(Count > 0) {
            *ActualCount = move_next_chars(Count, &cur_pos, TRUE, &new_pos);
            set_range_pos(This, FALSE, &new_pos);
            IHTMLTxtRange_collapse(HTMLTXTRANGE(This), FALSE);
            dompos_release(&new_pos);
        }else {
            *ActualCount = -move_prev_chars(This, -Count, &cur_pos, FALSE, &new_pos);
            set_range_pos(This, TRUE, &new_pos);
            IHTMLTxtRange_collapse(HTMLTXTRANGE(This), TRUE);
            dompos_release(&new_pos);
        }

        dompos_release(&cur_pos);
        break;
    }

    case RU_WORD: {
        dompos_t cur_pos, new_pos;

        get_cur_pos(This, TRUE, &cur_pos);

        if(Count > 0) {
            *ActualCount = move_next_words(Count, &cur_pos, &new_pos);
            set_range_pos(This, FALSE, &new_pos);
            IHTMLTxtRange_collapse(HTMLTXTRANGE(This), FALSE);
            dompos_release(&new_pos);
        }else {
            *ActualCount = -move_prev_words(This, -Count, &cur_pos, &new_pos);
            set_range_pos(This, TRUE, &new_pos);
            IHTMLTxtRange_collapse(HTMLTXTRANGE(This), TRUE);
            dompos_release(&new_pos);
        }

        dompos_release(&cur_pos);
        break;
    }

    default:
        FIXME("unimplemented unit %s\n", debugstr_w(Unit));
    }

    TRACE("ret %ld\n", *ActualCount);
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_moveStart(IHTMLTxtRange *iface, BSTR Unit,
        long Count, long *ActualCount)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %ld %p)\n", This, debugstr_w(Unit), Count, ActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_moveEnd(IHTMLTxtRange *iface, BSTR Unit,
        long Count, long *ActualCount)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    range_unit_t unit;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(Unit), Count, ActualCount);

    unit = string_to_unit(Unit);
    if(unit == RU_UNKNOWN)
        return E_INVALIDARG;

    if(!Count) {
        *ActualCount = 0;
        return S_OK;
    }

    switch(unit) {
    case RU_CHAR: {
        dompos_t cur_pos, new_pos;
        PRBool collapsed;

        get_cur_pos(This, FALSE, &cur_pos);
        nsIDOMRange_GetCollapsed(This->nsrange, &collapsed);

        if(Count > 0) {
            *ActualCount = move_next_chars(Count, &cur_pos, collapsed, &new_pos);
            set_range_pos(This, FALSE, &new_pos);
        }else {
            *ActualCount = -move_prev_chars(This, -Count, &cur_pos, TRUE,  &new_pos);
            if(*ActualCount == Count)
                set_range_pos(This, FALSE, &new_pos);
            else
                IHTMLTxtRange_collapse(HTMLTXTRANGE(This), TRUE);
        }

        dompos_release(&cur_pos);
        dompos_release(&new_pos);
        break;
    }

    default:
        FIXME("unimplemented unit %s\n", debugstr_w(Unit));
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_select(IHTMLTxtRange *iface)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);

    TRACE("(%p)\n", This);

    if(This->doc->nscontainer) {
        nsIDOMWindow *dom_window = NULL;
        nsISelection *nsselection;

        nsIWebBrowser_GetContentDOMWindow(This->doc->nscontainer->webbrowser, &dom_window);
        nsIDOMWindow_GetSelection(dom_window, &nsselection);
        nsIDOMWindow_Release(dom_window);

        nsISelection_RemoveAllRanges(nsselection);
        nsISelection_AddRange(nsselection, This->nsrange);

        nsISelection_Release(nsselection);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_pasteHTML(IHTMLTxtRange *iface, BSTR html)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(html));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_moveToElementText(IHTMLTxtRange *iface, IHTMLElement *element)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, element);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_setEndPoint(IHTMLTxtRange *iface, BSTR how,
        IHTMLTxtRange *SourceRange)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(how), SourceRange);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_compareEndPoints(IHTMLTxtRange *iface, BSTR how,
        IHTMLTxtRange *SourceRange, long *ret)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    HTMLTxtRange *src_range;
    PRInt16 nsret = 0;
    int nscmpt;
    nsresult nsres;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(how), SourceRange, ret);

    nscmpt = string_to_nscmptype(how);
    if(nscmpt == -1)
        return E_INVALIDARG;

    src_range = get_range_object(This->doc, SourceRange);
    if(!src_range)
        return E_FAIL;

    nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, nscmpt, src_range->nsrange, &nsret);
    if(NS_FAILED(nsres))
        ERR("CompareBoundaryPoints failed: %08x\n", nsres);

    *ret = nsret;
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_findText(IHTMLTxtRange *iface, BSTR String,
        long count, long Flags, VARIANT_BOOL *Success)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %ld %08lx %p)\n", This, debugstr_w(String), count, Flags, Success);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_moveToPoint(IHTMLTxtRange *iface, long x, long y)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_getBookmark(IHTMLTxtRange *iface, BSTR *Bookmark)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Bookmark);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_moveToBookmark(IHTMLTxtRange *iface, BSTR Bookmark,
        VARIANT_BOOL *Success)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(Bookmark), Success);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandSupported(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandEnabled(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandState(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandIndeterm(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandText(IHTMLTxtRange *iface, BSTR cmdID,
        BSTR *pcmdText)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pcmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandValue(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT *pcmdValue)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pcmdValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_execCommand(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %x v %p)\n", This, debugstr_w(cmdID), showUI, pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_execCommandShowHelp(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = HTMLTXTRANGE_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

#undef HTMLTXTRANGE_THIS

static const IHTMLTxtRangeVtbl HTMLTxtRangeVtbl = {
    HTMLTxtRange_QueryInterface,
    HTMLTxtRange_AddRef,
    HTMLTxtRange_Release,
    HTMLTxtRange_GetTypeInfoCount,
    HTMLTxtRange_GetTypeInfo,
    HTMLTxtRange_GetIDsOfNames,
    HTMLTxtRange_Invoke,
    HTMLTxtRange_get_htmlText,
    HTMLTxtRange_put_text,
    HTMLTxtRange_get_text,
    HTMLTxtRange_parentElement,
    HTMLTxtRange_duplicate,
    HTMLTxtRange_inRange,
    HTMLTxtRange_isEqual,
    HTMLTxtRange_scrollIntoView,
    HTMLTxtRange_collapse,
    HTMLTxtRange_expand,
    HTMLTxtRange_move,
    HTMLTxtRange_moveStart,
    HTMLTxtRange_moveEnd,
    HTMLTxtRange_select,
    HTMLTxtRange_pasteHTML,
    HTMLTxtRange_moveToElementText,
    HTMLTxtRange_setEndPoint,
    HTMLTxtRange_compareEndPoints,
    HTMLTxtRange_findText,
    HTMLTxtRange_moveToPoint,
    HTMLTxtRange_getBookmark,
    HTMLTxtRange_moveToBookmark,
    HTMLTxtRange_queryCommandSupported,
    HTMLTxtRange_queryCommandEnabled,
    HTMLTxtRange_queryCommandState,
    HTMLTxtRange_queryCommandIndeterm,
    HTMLTxtRange_queryCommandText,
    HTMLTxtRange_queryCommandValue,
    HTMLTxtRange_execCommand,
    HTMLTxtRange_execCommandShowHelp
};

IHTMLTxtRange *HTMLTxtRange_Create(HTMLDocument *doc, nsIDOMRange *nsrange)
{
    HTMLTxtRange *ret = mshtml_alloc(sizeof(HTMLTxtRange));

    ret->lpHTMLTxtRangeVtbl = &HTMLTxtRangeVtbl;
    ret->ref = 1;

    if(nsrange)
        nsIDOMRange_AddRef(nsrange);
    ret->nsrange = nsrange;

    ret->doc = doc;
    list_add_head(&doc->range_list, &ret->entry);

    return HTMLTXTRANGE(ret);
}

void detach_ranges(HTMLDocument *This)
{
    HTMLTxtRange *iter;

    LIST_FOR_EACH_ENTRY(iter, &This->range_list, HTMLTxtRange, entry) {
        iter->doc = NULL;
    }
}
