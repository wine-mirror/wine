/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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
#include "shlguid.h"
#include "mshtmdid.h"
#include "idispids.h"
#include "mshtmcid.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NSCMD_BOLD "cmd_bold"
#define NSCMD_ITALIC "cmd_italic"
#define NSCMD_UNDERLINE "cmd_underline"
#define NSCMD_ALIGN "cmd_align"
#define NSCMD_INDENT "cmd_indent"
#define NSCMD_OUTDENT "cmd_outdent"
#define NSCMD_INSERTHR "cmd_insertHR"
#define NSCMD_UL "cmd_ul"
#define NSCMD_OL "cmd_ol"

#define NSSTATE_ATTRIBUTE "state_attribute"

#define NSALIGN_CENTER "center"
#define NSALIGN_LEFT   "left"
#define NSALIGN_RIGHT  "right"

/**********************************************************
 * IOleCommandTarget implementation
 */

#define CMDTARGET_THIS(iface) DEFINE_THIS(HTMLDocument, OleCommandTarget, iface)

static HRESULT exec_open(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_new(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save_as(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save_copy_as(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static nsresult set_head_text(nsIPrintSettings *settings, LPCWSTR template, BOOL head, int pos)
{
    if(head) {
        switch(pos) {
        case 0:
            return nsIPrintSettings_SetHeaderStrLeft(settings, template);
        case 1:
            return nsIPrintSettings_SetHeaderStrRight(settings, template);
        case 2:
            return nsIPrintSettings_SetHeaderStrCenter(settings, template);
        }
    }else {
        switch(pos) {
        case 0:
            return nsIPrintSettings_SetFooterStrLeft(settings, template);
        case 1:
            return nsIPrintSettings_SetFooterStrRight(settings, template);
        case 2:
            return nsIPrintSettings_SetFooterStrCenter(settings, template);
        }
    }

    return NS_OK;
}

static void set_print_template(nsIPrintSettings *settings, LPCWSTR template, BOOL head)
{
    PRUnichar nstemplate[200]; /* FIXME: Use dynamic allocation */
    PRUnichar *p = nstemplate;
    LPCWSTR ptr=template;
    int pos=0;

    while(*ptr) {
        if(*ptr != '&') {
            *p++ = *ptr++;
            continue;
        }

        switch(*++ptr) {
        case '&':
            *p++ = '&';
            *p++ = '&';
            ptr++;
            break;
        case 'b': /* change align */
            ptr++;
            *p = 0;
            set_head_text(settings, nstemplate, head, pos);
            p = nstemplate;
            pos++;
            break;
        case 'd': { /* short date */
            SYSTEMTIME systime;
            GetLocalTime(&systime);
            GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &systime, NULL, p,
                    sizeof(nstemplate)-(p-nstemplate)*sizeof(WCHAR));
            p += strlenW(p);
            ptr++;
            break;
        }
        case 'p': /* page number */
            *p++ = '&';
            *p++ = 'P';
            ptr++;
            break;
        case 'P': /* page count */
            *p++ = '?'; /* FIXME */
            ptr++;
            break;
        case 'u':
            *p++ = '&';
            *p++ = 'U';
            ptr++;
            break;
        case 'w':
            /* FIXME: set window title */
            ptr++;
            break;
        default:
            *p++ = '&';
            *p++ = *ptr++;
        }
    }

    *p = 0;
    set_head_text(settings, nstemplate, head, pos);

    while(++pos < 3)
        set_head_text(settings, p, head, pos);
}

static void set_default_templates(nsIPrintSettings *settings)
{
    WCHAR buf[64];

    static const PRUnichar empty[] = {0};

    nsIPrintSettings_SetHeaderStrLeft(settings, empty);
    nsIPrintSettings_SetHeaderStrRight(settings, empty);
    nsIPrintSettings_SetHeaderStrCenter(settings, empty);
    nsIPrintSettings_SetFooterStrLeft(settings, empty);
    nsIPrintSettings_SetFooterStrRight(settings, empty);
    nsIPrintSettings_SetFooterStrCenter(settings, empty);

    if(LoadStringW(get_shdoclc(), IDS_PRINT_HEADER_TEMPLATE, buf,
                   sizeof(buf)/sizeof(WCHAR)))
        set_print_template(settings, buf, TRUE);


    if(LoadStringW(get_shdoclc(), IDS_PRINT_FOOTER_TEMPLATE, buf,
                   sizeof(buf)/sizeof(WCHAR)))
        set_print_template(settings, buf, FALSE);

}

static HRESULT exec_print(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    nsIInterfaceRequestor *iface_req;
    nsIWebBrowserPrint *nsprint;
    nsIPrintSettings *settings;
    nsresult nsres;

    TRACE("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);

    if(pvaOut)
        FIXME("unsupported pvaOut\n");

    if(!This->nscontainer)
        return S_OK;

    nsres = nsIWebBrowser_QueryInterface(This->nscontainer->webbrowser,
            &IID_nsIInterfaceRequestor, (void**)&iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIInterfaceRequestor: %08x\n", nsres);
        return S_OK;
    }

    nsres = nsIInterfaceRequestor_GetInterface(iface_req, &IID_nsIWebBrowserPrint,
            (void**)&nsprint);
    nsIInterfaceRequestor_Release(iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIWebBrowserPrint: %08x\n", nsres);
        return S_OK;
    }

    nsres = nsIWebBrowserPrint_GetGlobalPrintSettings(nsprint, &settings);
    if(NS_FAILED(nsres))
        ERR("GetCurrentPrintSettings failed: %08x\n", nsres);

    set_default_templates(settings);

    if(pvaIn) {
        switch(V_VT(pvaIn)) {
        case VT_BYREF|VT_ARRAY: {
            VARIANT *opts;
            DWORD opts_cnt;

            if(V_ARRAY(pvaIn)->cDims != 1)
                WARN("cDims = %d\n", V_ARRAY(pvaIn)->cDims);

            SafeArrayAccessData(V_ARRAY(pvaIn), (void**)&opts);
            opts_cnt = V_ARRAY(pvaIn)->rgsabound[0].cElements;

            if(opts_cnt >= 1) {
                switch(V_VT(opts)) {
                case VT_BSTR:
                    TRACE("setting footer %s\n", debugstr_w(V_BSTR(opts)));
                    set_print_template(settings, V_BSTR(opts), TRUE);
                    break;
                case VT_NULL:
                    break;
                default:
                    WARN("V_VT(opts) = %d\n", V_VT(opts));
                }
            }

            if(opts_cnt >= 2) {
                switch(V_VT(opts+1)) {
                case VT_BSTR:
                    TRACE("setting footer %s\n", debugstr_w(V_BSTR(opts+1)));
                    set_print_template(settings, V_BSTR(opts+1), FALSE);
                    break;
                case VT_NULL:
                    break;
                default:
                    WARN("V_VT(opts) = %d\n", V_VT(opts+1));
                }
            }

            if(opts_cnt >= 3)
                FIXME("Unsupported opts_cnt %d\n", opts_cnt);

            SafeArrayUnaccessData(V_ARRAY(pvaIn));
            break;
        }
        default:
            FIXME("unsupported vt %x\n", V_VT(pvaIn));
        }
    }

    nsres = nsIWebBrowserPrint_Print(nsprint, settings, NULL);
    if(NS_FAILED(nsres))
        ERR("Print failed: %08x\n", nsres);

    nsIWebBrowserPrint_Release(nsprint);

    return S_OK;
}

static HRESULT exec_print_preview(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_page_setup(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_spell(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_properties(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_cut(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_copy(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_paste(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_paste_special(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_undo(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_rendo(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_select_all(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_clear_selection(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_zoom(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_get_zoom_range(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_refresh(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_stop(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_stop_download(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_delete(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_enable_interaction(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_on_unload(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    TRACE("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);

    /* Tests show that we have nothing more to do here */

    if(pvaOut) {
        V_VT(pvaOut) = VT_BOOL;
        V_BOOL(pvaOut) = VARIANT_TRUE;
    }

    return S_OK;
}

static HRESULT exec_show_page_setup(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_show_print(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_close(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_set_print_template(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_get_print_template(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%d %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT query_mshtml_copy(HTMLDocument *This, OLECMD *cmd)
{
    FIXME("(%p)\n", This);
    cmd->cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
    return S_OK;
}

static HRESULT exec_mshtml_copy(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    FIXME("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);
    return E_NOTIMPL;
}

static HRESULT query_mshtml_cut(HTMLDocument *This, OLECMD *cmd)
{
    FIXME("(%p)\n", This);
    cmd->cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
    return S_OK;
}

static HRESULT exec_mshtml_cut(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    FIXME("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);
    return E_NOTIMPL;
}

static HRESULT query_mshtml_paste(HTMLDocument *This, OLECMD *cmd)
{
    FIXME("(%p)\n", This);
    cmd->cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
    return S_OK;
}

static HRESULT exec_mshtml_paste(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    FIXME("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);
    return E_NOTIMPL;
}

static HRESULT exec_browsemode(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    WARN("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);

    if(in || out)
        FIXME("unsupported args\n");

    This->usermode = BROWSEMODE;

    return S_OK;
}

static void setup_ns_editing(NSContainer *This)
{
    nsIInterfaceRequestor *iface_req;
    nsIEditingSession *editing_session = NULL;
    nsIURIContentListener *listener = NULL;
    nsIDOMWindow *dom_window = NULL;
    nsresult nsres;

    nsres = nsIWebBrowser_QueryInterface(This->webbrowser,
            &IID_nsIInterfaceRequestor, (void**)&iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIInterfaceRequestor: %08x\n", nsres);
        return;
    }

    nsres = nsIInterfaceRequestor_GetInterface(iface_req, &IID_nsIEditingSession,
                                               (void**)&editing_session);
    nsIInterfaceRequestor_Release(iface_req);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIEditingSession: %08x\n", nsres);
        return;
    }

    nsres = nsIWebBrowser_GetContentDOMWindow(This->webbrowser, &dom_window);
    if(NS_FAILED(nsres)) {
        ERR("Could not get content DOM window: %08x\n", nsres);
        nsIEditingSession_Release(editing_session);
        return;
    }

    nsres = nsIEditingSession_MakeWindowEditable(editing_session, dom_window, NULL, FALSE);
    nsIEditingSession_Release(editing_session);
    nsIDOMWindow_Release(dom_window);
    if(NS_FAILED(nsres)) {
        ERR("MakeWindowEditable failed: %08x\n", nsres);
        return;
    }

    /* MakeWindowEditable changes WebBrowser's parent URI content listener.
     * It seams to be a bug in Gecko. To workaround it we set our content
     * listener again and Gecko's one as its parent.
     */
    nsIWebBrowser_GetParentURIContentListener(This->webbrowser, &listener);
    nsIURIContentListener_SetParentContentListener(NSURICL(This), listener);
    nsIURIContentListener_Release(listener);
    nsIWebBrowser_SetParentURIContentListener(This->webbrowser, NSURICL(This));
}

static HRESULT exec_editmode(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    IMoniker *mon;
    HRESULT hres;

    static const WCHAR wszAboutBlank[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

    TRACE("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);

    if(in || out)
        FIXME("unsupported args\n");

    This->usermode = EDITMODE;

    if(This->frame)
        IOleInPlaceFrame_SetStatusText(This->frame, NULL);

    if(This->hostui) {
        DOCHOSTUIINFO hostinfo;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(This->hostui, &hostinfo);
        if(SUCCEEDED(hres))
            /* FIXME: use hostinfo */
            TRACE("hostinfo = {%u %08x %08x %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
    }

    if(This->nscontainer)
        setup_ns_editing(This->nscontainer);

    hres = CreateURLMoniker(NULL, wszAboutBlank, &mon);
    if(FAILED(hres)) {
        FIXME("CreateURLMoniker failed: %08x\n", hres);
        return hres;
    }

    return IPersistMoniker_Load(PERSISTMON(This), TRUE, mon, NULL, 0);
}

static HRESULT exec_htmleditmode(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    FIXME("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);
    return S_OK;
}

static HRESULT exec_setdirty(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    FIXME("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);
    return E_NOTIMPL;
}

static HRESULT exec_baselinefont3(HTMLDocument *This, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    FIXME("(%p)->(%08x %p %p)\n", This, cmdexecopt, in, out);
    return S_OK;
}

static const struct {
    OLECMDF cmdf;
    HRESULT (*func)(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
} exec_table[OLECMDID_GETPRINTTEMPLATE+1] = {
    {0},
    { OLECMDF_SUPPORTED,                  exec_open                 }, /* OLECMDID_OPEN */
    { OLECMDF_SUPPORTED,                  exec_new                  }, /* OLECMDID_NEW */
    { OLECMDF_SUPPORTED,                  exec_save                 }, /* OLECMDID_SAVE */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_save_as              }, /* OLECMDID_SAVEAS */
    { OLECMDF_SUPPORTED,                  exec_save_copy_as         }, /* OLECMDID_SAVECOPYAS */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_print                }, /* OLECMDID_PRINT */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_print_preview        }, /* OLECMDID_PRINTPREVIEW */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_page_setup           }, /* OLECMDID_PAGESETUP */
    { OLECMDF_SUPPORTED,                  exec_spell                }, /* OLECMDID_SPELL */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_properties           }, /* OLECMDID_PROPERTIES */
    { OLECMDF_SUPPORTED,                  exec_cut                  }, /* OLECMDID_CUT */
    { OLECMDF_SUPPORTED,                  exec_copy                 }, /* OLECMDID_COPY */
    { OLECMDF_SUPPORTED,                  exec_paste                }, /* OLECMDID_PASTE */
    { OLECMDF_SUPPORTED,                  exec_paste_special        }, /* OLECMDID_PASTESPECIAL */
    { OLECMDF_SUPPORTED,                  exec_undo                 }, /* OLECMDID_UNDO */
    { OLECMDF_SUPPORTED,                  exec_rendo                }, /* OLECMDID_REDO */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_select_all           }, /* OLECMDID_SELECTALL */
    { OLECMDF_SUPPORTED,                  exec_clear_selection      }, /* OLECMDID_CLEARSELECTION */
    { OLECMDF_SUPPORTED,                  exec_zoom                 }, /* OLECMDID_ZOOM */
    { OLECMDF_SUPPORTED,                  exec_get_zoom_range       }, /* OLECMDID_GETZOOMRANGE */
    {0},
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_refresh              }, /* OLECMDID_REFRESH */
    { OLECMDF_SUPPORTED|OLECMDF_ENABLED,  exec_stop                 }, /* OLECMDID_STOP */
    {0},{0},{0},{0},{0},{0},
    { OLECMDF_SUPPORTED,                  exec_stop_download        }, /* OLECMDID_STOPDOWNLOAD */
    {0},{0},
    { OLECMDF_SUPPORTED,                  exec_delete               }, /* OLECMDID_DELETE */
    {0},{0},
    { OLECMDF_SUPPORTED,                  exec_enable_interaction   }, /* OLECMDID_ENABLE_INTERACTION */
    { OLECMDF_SUPPORTED,                  exec_on_unload            }, /* OLECMDID_ONUNLOAD */
    {0},{0},{0},{0},{0},
    { OLECMDF_SUPPORTED,                  exec_show_page_setup      }, /* OLECMDID_SHOWPAGESETUP */
    { OLECMDF_SUPPORTED,                  exec_show_print           }, /* OLECMDID_SHOWPRINT */
    {0},{0},
    { OLECMDF_SUPPORTED,                  exec_close                }, /* OLECMDID_CLOSE */
    {0},{0},{0},
    { OLECMDF_SUPPORTED,                  exec_set_print_template   }, /* OLECMDID_SETPRINTTEMPLATE */
    { OLECMDF_SUPPORTED,                  exec_get_print_template   }  /* OLECMDID_GETPRINTTEMPLATE */
};

static const cmdtable_t base_cmds[] = {
    {IDM_COPY,             query_mshtml_copy,     exec_mshtml_copy},
    {IDM_PASTE,            query_mshtml_paste,    exec_mshtml_paste},
    {IDM_CUT,              query_mshtml_cut,      exec_mshtml_cut},
    {IDM_BROWSEMODE,       NULL,           exec_browsemode},
    {IDM_EDITMODE,         NULL,           exec_editmode},
    {IDM_PRINT,            NULL,           exec_print},
    {IDM_SETDIRTY,         NULL,           exec_setdirty},
    {IDM_HTMLEDITMODE,     NULL,           exec_htmleditmode},
    {IDM_BASELINEFONT3,    NULL,           exec_baselinefont3},
    {0,NULL,NULL}
};

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleCommandTarget_Release(IOleCommandTarget *iface)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT query_from_table(HTMLDocument *This, const cmdtable_t *cmdtable, OLECMD *cmd)
{
    const cmdtable_t *iter = cmdtable;

    cmd->cmdf = 0;

    while(iter->id && iter->id != cmd->cmdID)
        iter++;

    if(!iter->id || !iter->query)
        return OLECMDERR_E_NOTSUPPORTED;

    return iter->query(This, cmd);
}

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    HRESULT hres = S_OK, hr;

    TRACE("(%p)->(%s %d %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);

    if(!pguidCmdGroup) {
        ULONG i;

        for(i=0; i<cCmds; i++) {
            if(prgCmds[i].cmdID<OLECMDID_OPEN || prgCmds[i].cmdID>OLECMDID_GETPRINTTEMPLATE) {
                WARN("Unsupported cmdID = %d\n", prgCmds[i].cmdID);
                prgCmds[i].cmdf = 0;
                hres = OLECMDERR_E_NOTSUPPORTED;
            }else {
                if(prgCmds[i].cmdID == OLECMDID_OPEN || prgCmds[i].cmdID == OLECMDID_NEW) {
                    IOleCommandTarget *cmdtrg = NULL;
                    OLECMD olecmd;

                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    if(This->client) {
                        hr = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget,
                                (void**)&cmdtrg);
                        if(SUCCEEDED(hr)) {
                            olecmd.cmdID = prgCmds[i].cmdID;
                            olecmd.cmdf = 0;

                            hr = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 1, &olecmd, NULL);
                            if(SUCCEEDED(hr) && olecmd.cmdf)
                                prgCmds[i].cmdf = olecmd.cmdf;
                        }
                    }else {
                        ERR("This->client == NULL, native would crash\n");
                    }
                }else {
                    prgCmds[i].cmdf = exec_table[prgCmds[i].cmdID].cmdf;
                    TRACE("cmdID = %d  returning %x\n", prgCmds[i].cmdID, prgCmds[i].cmdf);
                }
                hres = S_OK;
            }
        }

        if(pCmdText)
            FIXME("Set pCmdText\n");
    }else if(IsEqualGUID(&CGID_MSHTML, pguidCmdGroup)) {
        ULONG i;

        for(i=0; i<cCmds; i++) {
            HRESULT hres = query_from_table(This, base_cmds, prgCmds+i);
            if(hres == OLECMDERR_E_NOTSUPPORTED)
                hres = query_from_table(This, editmode_cmds, prgCmds+i);
            if(hres != OLECMDERR_E_NOTSUPPORTED)
                continue;

            switch(prgCmds[i].cmdID) {
            case IDM_PRINT:
                FIXME("CGID_MSHTML: IDM_PRINT\n");
                prgCmds[i].cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
                break;
            case IDM_BLOCKDIRLTR:
                FIXME("CGID_MSHTML: IDM_BLOCKDIRLTR\n");
                prgCmds[i].cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
                break;
            case IDM_BLOCKDIRRTL:
                FIXME("CGID_MSHTML: IDM_BLOCKDIRRTL\n");
                prgCmds[i].cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
                break;
            default:
                FIXME("CGID_MSHTML: unsupported cmdID %d\n", prgCmds[i].cmdID);
                prgCmds[i].cmdf = 0;
            }
        }

        hres = prgCmds[i-1].cmdf ? S_OK : OLECMDERR_E_NOTSUPPORTED;

        if(pCmdText)
            FIXME("Set pCmdText\n");
    }else {
        FIXME("Unsupported pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
        hres = OLECMDERR_E_UNKNOWNGROUP;
    }

    return hres;
}

static HRESULT exec_from_table(HTMLDocument *This, const cmdtable_t *cmdtable, DWORD cmdid,
                               DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    const cmdtable_t *iter = cmdtable;

    while(iter->id && iter->id != cmdid)
        iter++;

    if(!iter->id || !iter->exec)
        return OLECMDERR_E_NOTSUPPORTED;

    return iter->exec(This, cmdexecopt, in, out);
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);

    if(!pguidCmdGroup) {
        if(nCmdID<OLECMDID_OPEN || nCmdID>OLECMDID_GETPRINTTEMPLATE || !exec_table[nCmdID].func) {
            WARN("Unsupported cmdID = %d\n", nCmdID);
            return OLECMDERR_E_NOTSUPPORTED;
        }

        return exec_table[nCmdID].func(This, nCmdexecopt, pvaIn, pvaOut);
    }else if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        FIXME("unsupported nCmdID %d of CGID_Explorer group\n", nCmdID);
        TRACE("%p %p\n", pvaIn, pvaOut);
        return OLECMDERR_E_NOTSUPPORTED;
    }else if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        FIXME("unsupported nCmdID %d of CGID_ShellDocView group\n", nCmdID);
        return OLECMDERR_E_NOTSUPPORTED;
    }else if(IsEqualGUID(&CGID_MSHTML, pguidCmdGroup)) {
        HRESULT hres = exec_from_table(This, base_cmds, nCmdID, nCmdexecopt, pvaIn, pvaOut);
        if(hres == OLECMDERR_E_NOTSUPPORTED)
            hres = exec_from_table(This, editmode_cmds, nCmdID,
                                   nCmdexecopt, pvaIn, pvaOut);
        if(hres == OLECMDERR_E_NOTSUPPORTED)
            FIXME("unsupported nCmdID %d of CGID_MSHTML group\n", nCmdID);

        return hres;
    }

    FIXME("Unsupported pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
    return OLECMDERR_E_UNKNOWNGROUP;
}

#undef CMDTARGET_THIS

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

void HTMLDocument_OleCmd_Init(HTMLDocument *This)
{
    This->lpOleCommandTargetVtbl = &OleCommandTargetVtbl;
}
