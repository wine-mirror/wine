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
#include "ole2.h"
#include "shlguid.h"
#include "mshtmdid.h"
#include "idispids.h"
#include "mshtmcid.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/**********************************************************
 * IOleCommandTarget implementation
 */

#define CMDTARGET_THIS(iface) DEFINE_THIS(HTMLDocument, OleCommandTarget, iface)

static HRESULT exec_open(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_new(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save_as(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_save_copy_as(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_print(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_print_preview(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_page_setup(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_spell(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_properties(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_cut(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_copy(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_paste(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_paste_special(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_undo(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_rendo(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_select_all(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_clear_selection(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_zoom(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_get_zoom_range(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_refresh(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_stop(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_stop_download(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_delete(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_enable_interaction(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_on_unload(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    TRACE("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);

    /* Tests show that we have nothing more to do here */

    if(pvaOut) {
        V_VT(pvaOut) = VT_BOOL;
        V_BOOL(pvaOut) = VARIANT_TRUE;
    }

    return S_OK;
}

static HRESULT exec_show_page_setup(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_show_print(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_close(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_set_print_template(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_get_print_template(HTMLDocument *This, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    FIXME("(%p)->(%ld %p %p)\n", This, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT exec_browsemode(HTMLDocument *This)
{
    WARN("(%p)\n", This);

    This->usermode = BROWSEMODE;

    return S_OK;
}

static HRESULT exec_baselinefont3(HTMLDocument *This)
{
    FIXME("(%p)\n", This);
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

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);
    HRESULT hres = S_OK, hr;

    TRACE("(%p)->(%s %ld %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);

    if(!pguidCmdGroup) {
        ULONG i;

        for(i=0; i<cCmds; i++) {
            if(prgCmds[i].cmdID<OLECMDID_OPEN || prgCmds[i].cmdID>OLECMDID_GETPRINTTEMPLATE) {
                WARN("Unsupported cmdID = %ld\n", prgCmds[i].cmdID);
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
                    TRACE("cmdID = %ld  returning %lx\n", prgCmds[i].cmdID, prgCmds[i].cmdf);
                }
                hres = S_OK;
            }
        }

        if(pCmdText)
            FIXME("Set pCmdText\n");
    }else {
        FIXME("Unsupported pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
        hres = OLECMDERR_E_UNKNOWNGROUP;
    }

    return hres;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HTMLDocument *This = CMDTARGET_THIS(iface);

    if(!pguidCmdGroup) {
        if(nCmdID<OLECMDID_OPEN || nCmdID>OLECMDID_GETPRINTTEMPLATE || !exec_table[nCmdID].func) {
            WARN("Unsupported cmdID = %ld\n", nCmdID);
            return OLECMDERR_E_NOTSUPPORTED;
        }

        return exec_table[nCmdID].func(This, nCmdexecopt, pvaIn, pvaOut);
    }else if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        FIXME("unsupported nCmdID %ld of CGID_Explorer group\n", nCmdID);
        TRACE("%p %p\n", pvaIn, pvaOut);
        return OLECMDERR_E_NOTSUPPORTED;
    }else if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        FIXME("unsupported nCmdID %ld of CGID_ShellDocView group\n", nCmdID);
        return OLECMDERR_E_NOTSUPPORTED;
    }else if(IsEqualGUID(&CGID_MSHTML, pguidCmdGroup)) {
        switch(nCmdID) {
        case IDM_BROWSEMODE:
            return exec_browsemode(This);
        case IDM_BASELINEFONT3:
            return exec_baselinefont3(This);
        default:
            FIXME("unsupported nCmdID %ld of CGID_MSHTML group\n", nCmdID);
            return OLECMDERR_E_NOTSUPPORTED;
        }
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
