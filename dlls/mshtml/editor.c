/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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
#include "mshtmcid.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NSCMD_FONTCOLOR    "cmd_fontColor"
#define NSCMD_FONTFACE     "cmd_fontFace"

#define NSSTATE_ATTRIBUTE "state_attribute"

#define DOM_VK_LEFT  VK_LEFT
#define DOM_VK_UP    VK_UP
#define DOM_VK_RIGHT VK_RIGHT
#define DOM_VK_DOWN  VK_DOWN

static const WCHAR wszFont[] = {'f','o','n','t',0};
static const WCHAR wszSize[] = {'s','i','z','e',0};

static void do_ns_command(NSContainer *This, const char *cmd, nsICommandParams *nsparam)
{
    nsICommandManager *cmdmgr;
    nsIInterfaceRequestor *iface_req;
    nsresult nsres;

    TRACE("(%p)\n", This);

    nsres = nsIWebBrowser_QueryInterface(This->webbrowser,
            &IID_nsIInterfaceRequestor, (void**)&iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIInterfaceRequestor: %08x\n", nsres);
        return;
    }

    nsres = nsIInterfaceRequestor_GetInterface(iface_req, &IID_nsICommandManager,
                                               (void**)&cmdmgr);
    nsIInterfaceRequestor_Release(iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsICommandManager: %08x\n", nsres);
        return;
    }

    nsres = nsICommandManager_DoCommand(cmdmgr, cmd, nsparam, NULL);
    if(NS_FAILED(nsres))
        ERR("DoCommand(%s) failed: %08x\n", debugstr_a(cmd), nsres);

    nsICommandManager_Release(cmdmgr);
}

static nsresult get_ns_command_state(NSContainer *This, const char *cmd, nsICommandParams *nsparam)
{
    nsICommandManager *cmdmgr;
    nsIInterfaceRequestor *iface_req;
    nsresult nsres;

    nsres = nsIWebBrowser_QueryInterface(This->webbrowser,
            &IID_nsIInterfaceRequestor, (void**)&iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIInterfaceRequestor: %08x\n", nsres);
        return nsres;
    }

    nsres = nsIInterfaceRequestor_GetInterface(iface_req, &IID_nsICommandManager,
                                               (void**)&cmdmgr);
    nsIInterfaceRequestor_Release(iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsICommandManager: %08x\n", nsres);
        return nsres;
    }

    nsres = nsICommandManager_GetCommandState(cmdmgr, cmd, NULL, nsparam);
    if(NS_FAILED(nsres))
        ERR("GetCommandState(%s) failed: %08x\n", debugstr_a(cmd), nsres);

    nsICommandManager_Release(cmdmgr);
    return nsres;
}

static nsISelection *get_ns_selection(HTMLDocument *This)
{
    nsIDOMWindow *dom_window;
    nsISelection *nsselection = NULL;
    nsresult nsres;

    if(!This->nscontainer)
        return NULL;

    nsres = nsIWebBrowser_GetContentDOMWindow(This->nscontainer->webbrowser, &dom_window);
    if(NS_FAILED(nsres))
        return NULL;

    nsIDOMWindow_GetSelection(dom_window, &nsselection);
    nsIDOMWindow_Release(dom_window);

    return nsselection;

}

static void remove_child_attr(nsIDOMElement *elem, LPCWSTR tag, nsAString *attr_str)
{
    PRBool has_children;
    PRUint32 child_cnt, i;
    nsIDOMNode *child_node;
    nsIDOMNodeList *node_list;
    PRUint16 node_type;

    nsIDOMElement_HasChildNodes(elem, &has_children);
    if(!has_children)
        return;

    nsIDOMElement_GetChildNodes(elem, &node_list);
    nsIDOMNodeList_GetLength(node_list, &child_cnt);

    for(i=0; i<child_cnt; i++) {
        nsIDOMNodeList_Item(node_list, i, &child_node);

        nsIDOMNode_GetNodeType(child_node, &node_type);
        if(node_type == ELEMENT_NODE) {
            nsIDOMElement *child_elem;
            nsAString tag_str;
            const PRUnichar *ctag;

            nsIDOMNode_QueryInterface(child_node, &IID_nsIDOMElement, (void**)&child_elem);

            nsAString_Init(&tag_str, NULL);
            nsIDOMElement_GetTagName(child_elem, &tag_str);
            nsAString_GetData(&tag_str, &ctag, NULL);

            if(!strcmpiW(ctag, tag))
                /* FIXME: remove node if there are no more attributes */
                nsIDOMElement_RemoveAttribute(child_elem, attr_str);

            nsAString_Finish(&tag_str);

            remove_child_attr(child_elem, tag, attr_str);

            nsIDOMNode_Release(child_elem);
        }

        nsIDOMNode_Release(child_node);
    }

    nsIDOMNodeList_Release(node_list);
}

void get_font_size(HTMLDocument *This, WCHAR *ret)
{
    nsISelection *nsselection = get_ns_selection(This);
    nsIDOMElement *elem = NULL;
    nsIDOMNode *node = NULL, *tmp_node;
    nsAString tag_str;
    LPCWSTR tag;
    PRUint16 node_type;
    nsresult nsres;

    *ret = 0;

    if(!nsselection)
        return;

    nsISelection_GetFocusNode(nsselection, &node);
    nsISelection_Release(nsselection);

    while(node) {
        nsres = nsIDOMNode_GetNodeType(node, &node_type);
        if(NS_FAILED(nsres) || node_type == DOCUMENT_NODE)
            break;

        if(node_type == ELEMENT_NODE) {
            nsIDOMNode_QueryInterface(node, &IID_nsIDOMElement, (void**)&elem);

            nsAString_Init(&tag_str, NULL);
            nsIDOMElement_GetTagName(elem, &tag_str);
            nsAString_GetData(&tag_str, &tag, NULL);

            if(!strcmpiW(tag, wszFont)) {
                nsAString size_str, val_str;
                LPCWSTR val;

                TRACE("found font tag %p\n", elem);

                nsAString_Init(&size_str, wszSize);
                nsAString_Init(&val_str, NULL);

                nsIDOMElement_GetAttribute(elem, &size_str, &val_str);
                nsAString_GetData(&val_str, &val, NULL);

                if(*val) {
                    TRACE("found size %s\n", debugstr_w(val));
                    strcpyW(ret, val);
                }

                nsAString_Finish(&size_str);
                nsAString_Finish(&val_str);
            }

            nsAString_Finish(&tag_str);

            nsIDOMElement_Release(elem);
        }

        if(*ret)
            break;

        tmp_node = node;
        nsIDOMNode_GetParentNode(node, &node);
        nsIDOMNode_Release(tmp_node);
    }

    if(node)
        nsIDOMNode_Release(node);
}

void set_font_size(HTMLDocument *This, LPCWSTR size)
{
    nsISelection *nsselection;
    PRBool collapsed;
    nsIDOMDocument *nsdoc;
    nsIDOMElement *elem;
    nsIDOMRange *range;
    PRInt32 range_cnt = 0;
    nsAString font_str;
    nsAString size_str;
    nsAString val_str;
    nsresult nsres;

    nsselection = get_ns_selection(This);

    if(!nsselection)
        return;

    nsres = nsIWebNavigation_GetDocument(This->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres))
        return;

    nsAString_Init(&font_str, wszFont);
    nsAString_Init(&size_str, wszSize);
    nsAString_Init(&val_str, size);

    nsISelection_GetRangeCount(nsselection, &range_cnt);
    if(range_cnt != 1)
        FIXME("range_cnt %d not supprted\n", range_cnt);

    nsIDOMDocument_CreateElement(nsdoc, &font_str, &elem);
    nsIDOMElement_SetAttribute(elem, &size_str, &val_str);        

    nsISelection_GetRangeAt(nsselection, 0, &range);
    nsISelection_GetIsCollapsed(nsselection, &collapsed);
    nsISelection_RemoveAllRanges(nsselection);

    nsIDOMRange_SurroundContents(range, (nsIDOMNode*)elem);

    if(collapsed) {
        nsISelection_Collapse(nsselection, (nsIDOMNode*)elem, 0);
    }else {
        /* Remove all size attrbutes from the range */
        remove_child_attr(elem, wszFont, &size_str);
        nsISelection_SelectAllChildren(nsselection, (nsIDOMNode*)elem);
    }

    nsIDOMRange_Release(range);
    nsIDOMElement_Release(elem);

    nsAString_Finish(&font_str);
    nsAString_Finish(&size_str);
    nsAString_Finish(&val_str);

    nsISelection_Release(nsselection);
    nsIDOMDocument_Release(nsdoc);
}

static BOOL is_visible_text_node(nsIDOMNode *node)
{
    nsIDOMCharacterData *char_data;
    nsAString data_str;
    LPCWSTR data, ptr;
    PRUint32 len;

    nsIDOMNode_QueryInterface(node, &IID_nsIDOMCharacterData, (void**)&char_data);

    nsIDOMCharacterData_GetLength(char_data, &len);

    nsAString_Init(&data_str, NULL);
    nsIDOMCharacterData_GetData(char_data, &data_str);
    nsAString_GetData(&data_str, &data, NULL);

    if(*data == '\n') {
        len--;
        for(ptr=data+1; ptr && isspaceW(*ptr); ptr++)
            len--;
    }

    nsAString_Finish(&data_str);

    nsIDOMCharacterData_Release(char_data);

    return len != 0;
}

static nsIDOMNode *get_child_text_node(nsIDOMNode *node, BOOL first)
{
    nsIDOMNode *iter, *iter2;

    if(first)
        nsIDOMNode_GetFirstChild(node, &iter);
    else
        nsIDOMNode_GetLastChild(node, &iter);

    while(iter) {
        PRUint16 node_type;

        nsIDOMNode_GetNodeType(iter, &node_type);
        switch(node_type) {
        case TEXT_NODE:
            if(is_visible_text_node(iter))
                return iter;
        case ELEMENT_NODE:
            iter2 = get_child_text_node(iter, first);
            if(iter2) {
                nsIDOMNode_Release(iter);
                return iter2;
            }
        }

        if(first)
            nsIDOMNode_GetNextSibling(iter, &iter2);
        else
            nsIDOMNode_GetPreviousSibling(iter, &iter2);

        nsIDOMNode_Release(iter);
        iter = iter2;
    }

    return NULL;
}

static nsIDOMNode *get_next_text_node(nsIDOMNode *node, BOOL next)
{
    nsIDOMNode *iter, *iter2 = NULL, *parent = NULL;
    PRUint16 node_type;

    iter = node;
    nsIDOMNode_AddRef(iter);

    while(1) {
        if(next)
            nsIDOMNode_GetNextSibling(iter, &iter2);
        else
            nsIDOMNode_GetPreviousSibling(iter, &iter2);

        while(!iter2) {
            nsIDOMNode_GetParentNode(iter, &parent);
            nsIDOMNode_Release(iter);
            if(!parent)
                return NULL;

            iter = parent;

            if(next)
                nsIDOMNode_GetNextSibling(iter, &iter2);
            else
                nsIDOMNode_GetPreviousSibling(iter, &iter2);
        }

        nsIDOMNode_Release(iter);
        iter = iter2;

        nsIDOMNode_GetNodeType(iter, &node_type);

        switch(node_type) {
        case TEXT_NODE:
            if(is_visible_text_node(iter))
                return iter;
        case ELEMENT_NODE:
            iter2 = get_child_text_node(iter, next);
            if(iter2) {
                nsIDOMNode_Release(iter);
                return iter2;
            }
        }
    }

    return NULL;
}

static void collapse_end_node(nsISelection *selection, nsIDOMNode *node)
{
    nsIDOMCharacterData *char_data;
    PRUint32 len;

    nsIDOMNode_QueryInterface(node, &IID_nsIDOMCharacterData, (void**)&char_data);
    nsIDOMCharacterData_GetLength(char_data, &len);
    nsIDOMCharacterData_Release(char_data);

    nsISelection_Collapse(selection, node, len);
}

static void collapse_next_char(HTMLDocument *doc, nsIDOMKeyEvent *event, BOOL next)
{
    nsISelection *selection = get_ns_selection(doc);
    nsIDOMNode *node;
    PRBool collapsed, b;
    PRUint16 node_type;
    nsIDOMNode *text_node;

    nsIDOMKeyEvent_GetCtrlKey(event, &b);
    if(b) return;

    nsIDOMKeyEvent_GetShiftKey(event, &b);
    if(b) return;

    nsISelection_GetIsCollapsed(selection, &collapsed);
    if(!collapsed)
        nsISelection_CollapseToEnd(selection);

    nsISelection_GetFocusNode(selection, &node);
    nsIDOMNode_GetNodeType(node, &node_type);

    if(node_type == TEXT_NODE) {
        nsIDOMCharacterData *char_data;
        PRInt32 offset;
        PRUint32 len;

        nsISelection_GetFocusOffset(selection, &offset);

        nsIDOMNode_QueryInterface(node, &IID_nsIDOMCharacterData, (void**)&char_data);
        nsIDOMCharacterData_GetLength(char_data, &len);
        nsIDOMCharacterData_Release(char_data);

        if(next ? offset != len : offset) {
            nsISelection_Collapse(selection, node, offset + (next?1:-1));
            return;
        }
    }

    text_node = get_next_text_node(node, next);
    if(text_node) {
        if(next)
            nsISelection_Collapse(selection, text_node, 1);
        else
            collapse_end_node(selection, text_node);
        nsIDOMNode_Release(text_node);
    }

    nsIDOMNode_Release(node);
    nsISelection_Release(selection);
}

void handle_edit_event(HTMLDocument *This, nsIDOMEvent *event)
{
    nsIDOMKeyEvent *key_event;
    PRUint32 code;

    nsIDOMEvent_QueryInterface(event, &IID_nsIDOMKeyEvent, (void**)&key_event);

    nsIDOMKeyEvent_GetKeyCode(key_event, &code);

    switch(code) {
    case DOM_VK_LEFT:
        TRACE("left\n");
        collapse_next_char(This, key_event, FALSE);
        break;
    case DOM_VK_RIGHT:
        TRACE("right\n");
        collapse_next_char(This, key_event, TRUE);
    };

    nsIDOMKeyEvent_Release(key_event);
}

static HRESULT exec_fontname(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%p %p)\n", This, in, out);

    if(!This->nscontainer)
        return E_FAIL;

    if(in) {
        nsICommandParams *nsparam = create_nscommand_params();
        char *stra;
        DWORD len;

        if(V_VT(in) != VT_BSTR) {
            FIXME("Unsupported vt=%d\n", V_VT(out));
            return E_INVALIDARG;
        }

        len = WideCharToMultiByte(CP_ACP, 0, V_BSTR(in), -1, NULL, 0, NULL, NULL);
        stra = mshtml_alloc(len);
        WideCharToMultiByte(CP_ACP, 0, V_BSTR(in), -1, stra, -1, NULL, NULL);
        nsICommandParams_SetCStringValue(nsparam, NSSTATE_ATTRIBUTE, stra);
        mshtml_free(stra);

        do_ns_command(This->nscontainer, NSCMD_FONTFACE, nsparam);

        nsICommandParams_Release(nsparam);
    }

    if(out) {
        nsICommandParams *nsparam;
        LPWSTR strw;
        char *stra;
        DWORD len;
        nsresult nsres;

        if(V_VT(out) != VT_BSTR) {
            FIXME("Unsupported vt=%d\n", V_VT(out));
            return E_INVALIDARG;
        }

        nsparam = create_nscommand_params();

        nsres = get_ns_command_state(This->nscontainer, NSCMD_FONTFACE, nsparam);
        if(NS_FAILED(nsres))
            return S_OK;

        nsICommandParams_GetCStringValue(nsparam, NSSTATE_ATTRIBUTE, &stra);
        nsICommandParams_Release(nsparam);

        len = MultiByteToWideChar(CP_ACP, 0, stra, -1, NULL, 0);
        strw = mshtml_alloc(len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, stra, -1, strw, -1);
        nsfree(stra);

        V_BSTR(out) = SysAllocString(strw);
        mshtml_free(strw);
    }

    return S_OK;
}

static HRESULT exec_forecolor(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%p %p)\n", This, in, out);

    if(in) {
        if(V_VT(in) == VT_I4) {
            nsICommandParams *nsparam = create_nscommand_params();
            char color_str[10];

            sprintf(color_str, "#%02x%02x%02x",
                    V_I4(in)&0xff, (V_I4(in)>>8)&0xff, (V_I4(in)>>16)&0xff);

            nsICommandParams_SetCStringValue(nsparam, NSSTATE_ATTRIBUTE, color_str);
            do_ns_command(This->nscontainer, NSCMD_FONTCOLOR, nsparam);

            nsICommandParams_Release(nsparam);
        }else {
            FIXME("unsupported in vt=%d\n", V_VT(in));
        }
    }

    if(out) {
        FIXME("unsupported out\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT exec_fontsize(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%p %p)\n", This, in, out);

    if(out) {
        WCHAR val[10] = {0};

        switch(V_VT(out)) {
        case VT_I4:
            get_font_size(This, val);
            V_I4(out) = strtolW(val, NULL, 10);
            break;
        case VT_BSTR:
            get_font_size(This, val);
            V_BSTR(out) = SysAllocString(val);
            break;
        default:
            FIXME("unsupported vt %d\n", V_VT(out));
        }
    }

    if(in) {
        switch(V_VT(in)) {
        case VT_I4: {
            WCHAR size[10];
            static const WCHAR format[] = {'%','d',0};
            wsprintfW(size, format, V_I4(in));
            set_font_size(This, size);
            break;
        }
        case VT_BSTR:
            set_font_size(This, V_BSTR(in));
            break;
        default:
            FIXME("unsupported vt %d\n", V_VT(in));
        }
    }

    return S_OK;
}

const cmdtable_t editmode_cmds[] = {
    {IDM_FONTNAME,        NULL,         exec_fontname},
    {IDM_FONTSIZE,        NULL,         exec_fontsize},
    {IDM_FORECOLOR,       NULL,         exec_forecolor},
    {0,NULL,NULL}
};
