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

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "ole2.h"
#include "exdisp.h"
#include "htiframe.h"
#include "mshtmhst.h"
#include "mshtmcid.h"
#include "mshtml.h"
#include "idispids.h"
#include "olectl.h"
#include "mshtmdid.h"
#include "shobjidl.h"
#include "shdeprecated.h"
#include "shlguid.h"
#include "exdispid.h"
#include "mimeinfo.h"
#include "hlink.h"
#include "docobjectservice.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_OLEGUID(CGID_DocHostCmdPriv, 0x000214D4L, 0, 0);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(Site_GetWindow);
DEFINE_EXPECT(ShowObject);
DEFINE_EXPECT(CanInPlaceActivate);
DEFINE_EXPECT(OnInPlaceActivate);
DEFINE_EXPECT(OnUIActivate);
DEFINE_EXPECT(GetWindowContext);
DEFINE_EXPECT(Frame_GetWindow);
DEFINE_EXPECT(Frame_SetActiveObject);
DEFINE_EXPECT(UIWindow_SetActiveObject);
DEFINE_EXPECT(SetMenu);
DEFINE_EXPECT(Invoke_AMBIENT_USERMODE);
DEFINE_EXPECT(Invoke_AMBIENT_DLCONTROL);
DEFINE_EXPECT(Invoke_AMBIENT_USERAGENT);
DEFINE_EXPECT(Invoke_AMBIENT_PALETTE);
DEFINE_EXPECT(Invoke_AMBIENT_SILENT);
DEFINE_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
DEFINE_EXPECT(Invoke_STATUSTEXTCHANGE);
DEFINE_EXPECT(Invoke_PROPERTYCHANGE);
DEFINE_EXPECT(Invoke_DOWNLOADBEGIN);
DEFINE_EXPECT(Invoke_BEFORENAVIGATE2);
DEFINE_EXPECT(Invoke_SETSECURELOCKICON);
DEFINE_EXPECT(Invoke_FILEDOWNLOAD);
DEFINE_EXPECT(Invoke_COMMANDSTATECHANGE);
DEFINE_EXPECT(Invoke_DOWNLOADCOMPLETE);
DEFINE_EXPECT(Invoke_ONMENUBAR);
DEFINE_EXPECT(Invoke_ONADDRESSBAR);
DEFINE_EXPECT(Invoke_ONSTATUSBAR);
DEFINE_EXPECT(Invoke_ONTOOLBAR);
DEFINE_EXPECT(Invoke_ONFULLSCREEN);
DEFINE_EXPECT(Invoke_ONTHEATERMODE);
DEFINE_EXPECT(Invoke_WINDOWSETRESIZABLE);
DEFINE_EXPECT(Invoke_TITLECHANGE);
DEFINE_EXPECT(Invoke_NAVIGATECOMPLETE2);
DEFINE_EXPECT(Invoke_PROGRESSCHANGE);
DEFINE_EXPECT(Invoke_DOCUMENTCOMPLETE);
DEFINE_EXPECT(Invoke_WINDOWCLOSING);
DEFINE_EXPECT(Invoke_282);
DEFINE_EXPECT(EnableModeless_TRUE);
DEFINE_EXPECT(EnableModeless_FALSE);
DEFINE_EXPECT(GetHostInfo);
DEFINE_EXPECT(GetOptionKeyPath);
DEFINE_EXPECT(GetOverridesKeyPath);
DEFINE_EXPECT(SetStatusText);
DEFINE_EXPECT(UpdateUI);
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_0);
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_1);
DEFINE_EXPECT(Exec_SETPROGRESSMAX);
DEFINE_EXPECT(Exec_SETPROGRESSPOS);
DEFINE_EXPECT(Exec_UPDATECOMMANDS);
DEFINE_EXPECT(Exec_SETTITLE);
DEFINE_EXPECT(QueryStatus_SETPROGRESSTEXT);
DEFINE_EXPECT(Exec_STOP);
DEFINE_EXPECT(Exec_IDM_STOP);
DEFINE_EXPECT(Exec_DocHostCommandHandler_2300);
DEFINE_EXPECT(QueryStatus_STOP);
DEFINE_EXPECT(QueryStatus_IDM_STOP);
DEFINE_EXPECT(DocHost_EnableModeless_TRUE);
DEFINE_EXPECT(DocHost_EnableModeless_FALSE);
DEFINE_EXPECT(DocHost_TranslateAccelerator);
DEFINE_EXPECT(GetDropTarget);
DEFINE_EXPECT(TranslateUrl);
DEFINE_EXPECT(ShowUI);
DEFINE_EXPECT(HideUI);
DEFINE_EXPECT(OnUIDeactivate);
DEFINE_EXPECT(OnInPlaceDeactivate);
DEFINE_EXPECT(RequestUIActivate);
DEFINE_EXPECT(ControlSite_TranslateAccelerator);
DEFINE_EXPECT(OnFocus);
DEFINE_EXPECT(GetExternal);

static const WCHAR wszItem[] = {'i','t','e','m',0};
static const WCHAR emptyW[] = {0};

static VARIANT_BOOL exvb;

static IWebBrowser2 *wb;

static HWND container_hwnd, shell_embedding_hwnd;
static BOOL is_downloading, is_first_load, use_container_olecmd, test_close, is_http, use_container_dochostui;
static HRESULT hr_dochost_TranslateAccelerator = E_NOTIMPL;
static HRESULT hr_site_TranslateAccelerator = E_NOTIMPL;
static const char *current_url;
static int wb_version;

#define DWL_EXPECT_BEFORE_NAVIGATE  0x01
#define DWL_FROM_PUT_HREF           0x02
#define DWL_FROM_GOBACK             0x04
#define DWL_FROM_GOFORWARD          0x08
#define DWL_HTTP                    0x10
#define DWL_REFRESH                 0x20

static DWORD dwl_flags;


/* Returns true if the user interface is in English. Note that this does not
 * presume of the formatting of dates, numbers, etc.
 */
static BOOL is_lang_english(void)
{
    static HMODULE hkernel32 = NULL;
    static LANGID (WINAPI *pGetThreadUILanguage)(void) = NULL;
    static LANGID (WINAPI *pGetUserDefaultUILanguage)(void) = NULL;

    if (!hkernel32)
    {
        hkernel32 = GetModuleHandleA("kernel32.dll");
        pGetThreadUILanguage = (void*)GetProcAddress(hkernel32, "GetThreadUILanguage");
        pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");
    }
    if (pGetThreadUILanguage)
        return PRIMARYLANGID(pGetThreadUILanguage()) == LANG_ENGLISH;
    if (pGetUserDefaultUILanguage)
        return PRIMARYLANGID(pGetUserDefaultUILanguage()) == LANG_ENGLISH;

    return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
}

static BOOL iface_cmp(IUnknown *iface1, IUnknown *iface2)
{
    IUnknown *unk1, *unk2;

    if(iface1 == iface2)
        return TRUE;

    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_Release(unk1);
    IUnknown_QueryInterface(iface2, &IID_IUnknown, (void**)&unk2);
    IUnknown_Release(unk2);

    return unk1 == unk2;
}

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

#define create_webbrowser() _create_webbrowser(__LINE__)
static IWebBrowser2 *_create_webbrowser(unsigned line)
{
    IWebBrowser2 *ret;
    HRESULT hres;

    wb_version = 2;

    hres = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IWebBrowser2, (void**)&ret);
    ok_(__FILE__,line)(hres == S_OK, "Creating WebBrowser object failed: %08x\n", hres);
    return ret;
}

#define test_LocationURL(a,b) _test_LocationURL(__LINE__,a,b)
static void _test_LocationURL(unsigned line, IWebBrowser2 *wb, const char *exurl)
{
    BSTR url = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IWebBrowser2_get_LocationURL(wb, &url);
    ok_(__FILE__,line) (hres == (*exurl ? S_OK : S_FALSE), "get_LocationURL failed: %08x\n", hres);
    if (hres == S_OK)
    {
        ok_(__FILE__,line) (!strcmp_wa(url, exurl), "unexpected URL: %s\n", wine_dbgstr_w(url));
        SysFreeString(url);
    }
}

#define test_ready_state(ex) _test_ready_state(__LINE__,ex);
static void _test_ready_state(unsigned line, READYSTATE exstate)
{
    READYSTATE state;
    HRESULT hres;

    hres = IWebBrowser2_get_ReadyState(wb, &state);
    ok_(__FILE__,line)(hres == S_OK, "get_ReadyState failed: %08x\n", hres);
    ok_(__FILE__,line)(state == exstate, "ReadyState = %d, expected %d\n", state, exstate);
}

#define get_document(u) _get_document(__LINE__,u)
static IHTMLDocument2 *_get_document(unsigned line, IWebBrowser2 *wb)
{
    IHTMLDocument2 *html_doc;
    IDispatch *disp;
    HRESULT hres;

    disp = NULL;
    hres = IWebBrowser2_get_Document(wb, &disp);
    ok_(__FILE__,line)(hres == S_OK, "get_Document failed: %08x\n", hres);
    ok_(__FILE__,line)(disp != NULL, "doc_disp == NULL\n");

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLDocument2, (void**)&html_doc);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IHTMLDocument iface: %08x\n", hres);
    ok(disp == (IDispatch*)html_doc, "disp != html_doc\n");
    IDispatch_Release(disp);

    return html_doc;
}

#define get_dochost(u) _get_dochost(__LINE__,u)
static IOleClientSite *_get_dochost(unsigned line, IWebBrowser2 *unk)
{
    IOleClientSite *client_site;
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    HRESULT hres;

    doc = _get_document(line, unk);
    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    IHTMLDocument2_Release(doc);
    ok_(__FILE__,line)(hres == S_OK, "Got 0x%08x\n", hres);

    hres = IOleObject_GetClientSite(oleobj, &client_site);
    IOleObject_Release(oleobj);
    ok_(__FILE__,line)(hres == S_OK, "Got 0x%08x\n", hres);

    return client_site;
}

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    return 2;
}

static ULONG WINAPI OleCommandTarget_Release(IOleCommandTarget *iface)
{
    return 1;
}

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ok(!pguidCmdGroup, "pguidCmdGroup != MULL\n");
    ok(cCmds == 1, "cCmds=%d, expected 1\n", cCmds);
    ok(!pCmdText, "pCmdText != NULL\n");

    switch(prgCmds[0].cmdID) {
    case OLECMDID_STOP:
        CHECK_EXPECT2(QueryStatus_STOP);
        prgCmds[0].cmdf = OLECMDF_SUPPORTED;
        return S_OK;
    case OLECMDID_SETPROGRESSTEXT:
        CHECK_EXPECT(QueryStatus_SETPROGRESSTEXT);
        prgCmds[0].cmdf = OLECMDF_ENABLED;
        return S_OK;
    case IDM_STOP:
        /* Note:
         *  IDM_STOP is a command specific to CGID_MSHTML (not an OLECMDID command).
         *  This command appears here for the ExecWB and QueryStatusWB tests in order
         *  to help demonstrate that these routines use a NULL pguidCmdGroup.
         */
        CHECK_EXPECT(QueryStatus_IDM_STOP);
        prgCmds[0].cmdf = 0;
        return S_OK;
    default:
        ok(0, "unexpected command %d\n", prgCmds[0].cmdID);
    }

    return E_FAIL;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if(!pguidCmdGroup) {
        switch(nCmdID) {
        case OLECMDID_SETPROGRESSMAX:
            CHECK_EXPECT2(Exec_SETPROGRESSMAX);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn)
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETPROGRESSPOS:
            CHECK_EXPECT2(Exec_SETPROGRESSPOS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn)
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETDOWNLOADSTATE:
            if(is_downloading)
                ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER || !nCmdexecopt,
                   "nCmdexecopts=%08x\n", nCmdexecopt);
            else
                ok(!nCmdexecopt, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d\n", V_VT(pvaIn));
            switch(V_I4(pvaIn)) {
            case 0:
                CHECK_EXPECT2(Exec_SETDOWNLOADSTATE_0);
                break;
            case 1:
                CHECK_EXPECT2(Exec_SETDOWNLOADSTATE_1);
                break;
            default:
                ok(0, "unexpevted V_I4(pvaIn)=%d\n", V_I4(pvaIn));
            }
            return S_OK;
        case OLECMDID_UPDATECOMMANDS:
            CHECK_EXPECT2(Exec_UPDATECOMMANDS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(!pvaIn, "pvaIn != NULL\n");
            ok(!pvaOut, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETTITLE:
            CHECK_EXPECT(Exec_SETTITLE);
            /* TODO: test args */
            return S_OK;
        case OLECMDID_STOP:
            CHECK_EXPECT(Exec_STOP);
            return S_OK;
        case IDM_STOP:
            /* Note:
             *  IDM_STOP is a command specific to CGID_MSHTML (not an OLECMDID command).
             *  This command appears here for the ExecWB and QueryStatusWB tests in order
             *  to help demonstrate that these routines use a NULL pguidCmdGroup.
             */
            CHECK_EXPECT(Exec_IDM_STOP);
            return OLECMDERR_E_NOTSUPPORTED;
        case OLECMDID_UPDATETRAVELENTRY_DATARECOVERY:
        case OLECMDID_PAGEAVAILABLE: /* TODO (IE11) */
            return E_NOTIMPL;
        case 6058: /* TODO */
            return E_NOTIMPL;
        default:
            ok(0, "unexpected nCmdID %d\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        switch(nCmdID) {
        case 20: /* TODO */
        case 24: /* TODO */
        case 25: /* IE5 */
        case 37: /* TODO */
        case 39: /* TODO */
        case 66: /* TODO */
        case 67: /* TODO */
        case 69: /* TODO */
        case 101: /* TODO (IE8) */
        case 109: /* TODO (IE9) */
        case 113: /* TODO (IE10) */
        case 119: /* IE11 */
            return E_FAIL;
        default:
            ok(0, "unexpected nCmdID %d\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        switch(nCmdID) {
        case 105: /* TODO */
        case 133: /* IE11 */
        case 134: /* TODO (IE10) */
        case 135: /* IE11 */
        case 136: /* TODO (IE10) */
        case 138: /* TODO */
        case 140: /* TODO (Win7) */
        case 144: /* TODO */
        case 178: /* IE11 */
            return E_FAIL;
        default:
            ok(0, "unexpected nCmdID %d\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_DocHostCmdPriv, pguidCmdGroup)) {
        switch(nCmdID) {
        case 11: /* TODO */
            break;
        default:
            ok(0, "unexpected nCmdID %d of CGID_DocHostCmdPriv\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_DocHostCommandHandler, pguidCmdGroup)) {
        switch(nCmdID) {
        case 6041: /* TODO */
            break;
        case 2300:
            CHECK_EXPECT(Exec_DocHostCommandHandler_2300);
            return E_NOTIMPL;
        default:
            ok(0, "unexpected nCmdID %d of CGID_DocHostCommandHandler\n", nCmdID);
        }
    }else {
        ok(0, "unexpected pguidCmdGroup %s\n", wine_dbgstr_guid(pguidCmdGroup));
    }

    return E_FAIL;
}

static IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

static IOleCommandTarget OleCommandTarget = { &OleCommandTargetVtbl };

static HRESULT WINAPI OleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_ITargetContainer, riid))
        return E_NOINTERFACE; /* TODO */

    if(IsEqualGUID(&IID_IOleCommandTarget, riid)) {
        if(use_container_olecmd)
        {
            *ppv = &OleCommandTarget;
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI OleContainer_AddRef(IOleContainer *iface)
{
    return 2;
}

static ULONG WINAPI OleContainer_Release(IOleContainer *iface)
{
    return 1;
}

static HRESULT WINAPI OleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc,
        LPOLESTR pszDiaplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags,
        IEnumUnknown **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleContainerVtbl OleContainerVtbl = {
    OleContainer_QueryInterface,
    OleContainer_AddRef,
    OleContainer_Release,
    OleContainer_ParseDisplayName,
    OleContainer_EnumObjects,
    OleContainer_LockContainer
};

static IOleContainer OleContainer = { &OleContainerVtbl };

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(V_VT(pVarResult) == VT_EMPTY, "V_VT(pVarResult)=%d\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_PROPERTYGET, "wFlags=%08x, expected DISPATCH_PROPERTYGET\n", wFlags);
    ok(pDispParams->rgvarg == NULL, "pDispParams->rgvarg = %p\n", pDispParams->rgvarg);
    ok(pDispParams->rgdispidNamedArgs == NULL,
       "pDispParams->rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
    ok(pDispParams->cArgs == 0, "pDispParams->cArgs = %d\n", pDispParams->cArgs);
    ok(pDispParams->cNamedArgs == 0, "pDispParams->cNamedArgs = %d\n", pDispParams->cNamedArgs);

    switch(dispIdMember) {
    case DISPID_AMBIENT_USERMODE:
        CHECK_EXPECT2(Invoke_AMBIENT_USERMODE);
        return E_FAIL;
    case DISPID_AMBIENT_DLCONTROL:
        CHECK_EXPECT(Invoke_AMBIENT_DLCONTROL);
        ok(puArgErr != NULL, "puArgErr=%p\n", puArgErr);
        return E_FAIL;
    case DISPID_AMBIENT_USERAGENT:
       CHECK_EXPECT(Invoke_AMBIENT_USERAGENT);
        ok(puArgErr != NULL, "puArgErr=%p\n", puArgErr);
        return E_FAIL;
    case DISPID_AMBIENT_PALETTE:
        CHECK_EXPECT(Invoke_AMBIENT_PALETTE);
        ok(puArgErr != NULL, "puArgErr=%p\n", puArgErr);
        return E_FAIL;
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        CHECK_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        ok(puArgErr == NULL, "puArgErr=%p\n", puArgErr);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        return S_OK;
    case DISPID_AMBIENT_SILENT:
        CHECK_EXPECT(Invoke_AMBIENT_SILENT);
        ok(puArgErr == NULL, "puArgErr=%p\n", puArgErr);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        return S_OK;
    }

    ok(0, "unexpected dispIdMember %d\n", dispIdMember);
    return E_NOTIMPL;
}

static IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static HRESULT WINAPI WebBrowserEvents2_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&DIID_DWebBrowserEvents2, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

#define test_invoke_bool(p,s) _test_invoke_bool(__LINE__,p,s)
static void _test_invoke_bool(unsigned line, const DISPPARAMS *params, BOOL strict)
{
    ok_(__FILE__,line) (params->rgvarg != NULL, "rgvarg == NULL\n");
    ok_(__FILE__,line) (params->cArgs == 1, "cArgs=%d, expected 1\n", params->cArgs);
    ok_(__FILE__,line) (V_VT(params->rgvarg) == VT_BOOL, "V_VT(arg)=%d\n", V_VT(params->rgvarg));
    if(strict)
        ok_(__FILE__,line) (V_BOOL(params->rgvarg) == exvb, "V_VT(arg)=%x, expected %x\n",
                            V_BOOL(params->rgvarg), exvb);
    else
        ok_(__FILE__,line) (!V_BOOL(params->rgvarg) == !exvb, "V_VT(arg)=%x, expected %x\n",
                            V_BOOL(params->rgvarg), exvb);
}

static void test_OnBeforeNavigate(const VARIANT *disp, const VARIANT *url, const VARIANT *flags,
        const VARIANT *frame, const VARIANT *post_data, const VARIANT *headers, const VARIANT *cancel)
{
    ok(V_VT(disp) == VT_DISPATCH, "V_VT(disp)=%d, expected VT_DISPATCH\n", V_VT(disp));
    ok(V_DISPATCH(disp) != NULL, "V_DISPATCH(disp) == NULL\n");
    ok(V_DISPATCH(disp) == (IDispatch*)wb, "V_DISPATCH(disp)=%p, wb=%p\n", V_DISPATCH(disp), wb);

    ok(V_VT(url) == (VT_BYREF|VT_VARIANT), "V_VT(url)=%x, expected VT_BYREF|VT_VARIANT\n", V_VT(url));
    ok(V_VARIANTREF(url) != NULL, "V_VARIANTREF(url) == NULL)\n");
    if(V_VARIANTREF(url)) {
        ok(V_VT(V_VARIANTREF(url)) == VT_BSTR, "V_VT(V_VARIANTREF(url))=%d, expected VT_BSTR\n",
           V_VT(V_VARIANTREF(url)));
        ok(V_BSTR(V_VARIANTREF(url)) != NULL, "V_BSTR(V_VARIANTREF(url)) == NULL\n");
        ok(!strcmp_wa(V_BSTR(V_VARIANTREF(url)), current_url), "unexpected url %s, expected %s\n",
           wine_dbgstr_w(V_BSTR(V_VARIANTREF(url))), current_url);
    }

    ok(V_VT(flags) == (VT_BYREF|VT_VARIANT), "V_VT(flags)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(flags));
    ok(V_VT(flags) == (VT_BYREF|VT_VARIANT), "V_VT(flags)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(flags));
    ok(V_VARIANTREF(flags) != NULL, "V_VARIANTREF(flags) == NULL)\n");
    if(V_VARIANTREF(flags)) {
        ok(V_VT(V_VARIANTREF(flags)) == VT_I4, "V_VT(V_VARIANTREF(flags))=%d, expected VT_I4\n",
           V_VT(V_VARIANTREF(flags)));
        if(is_first_load) {
            ok(V_I4(V_VARIANTREF(flags)) == 0, "V_I4(V_VARIANTREF(flags)) = %x, expected 0\n",
               V_I4(V_VARIANTREF(flags)));
        }else {
            ok((V_I4(V_VARIANTREF(flags)) & ~0x40) == 0, "V_I4(V_VARIANTREF(flags)) = %x, expected 0x40 or 0\n",
               V_I4(V_VARIANTREF(flags)));
        }
    }

    ok(V_VT(frame) == (VT_BYREF|VT_VARIANT), "V_VT(frame)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(frame));
    ok(V_VT(frame) == (VT_BYREF|VT_VARIANT), "V_VT(frame)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(frame));
    ok(V_VARIANTREF(frame) != NULL, "V_VARIANTREF(frame) == NULL)\n");
    if(V_VARIANTREF(frame)) {
        ok(V_VT(V_VARIANTREF(frame)) == VT_BSTR, "V_VT(V_VARIANTREF(frame))=%d, expected VT_BSTR\n",
           V_VT(V_VARIANTREF(frame)));
        ok(V_BSTR(V_VARIANTREF(frame)) == NULL, "V_BSTR(V_VARIANTREF(frame)) = %p, expected NULL\n",
           V_BSTR(V_VARIANTREF(frame)));
    }

    ok(V_VT(post_data) == (VT_BYREF|VT_VARIANT), "V_VT(post_data)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(post_data));
    ok(V_VT(post_data) == (VT_BYREF|VT_VARIANT), "V_VT(post_data)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(post_data));
    ok(V_VARIANTREF(post_data) != NULL, "V_VARIANTREF(post_data) == NULL)\n");
    if(V_VARIANTREF(post_data)) {
        ok(V_VT(V_VARIANTREF(post_data)) == (VT_VARIANT|VT_BYREF),
           "V_VT(V_VARIANTREF(post_data))=%d, expected VT_VARIANT|VT_BYREF\n",
           V_VT(V_VARIANTREF(post_data)));
        ok(V_VARIANTREF(V_VARIANTREF(post_data)) != NULL,
           "V_VARIANTREF(V_VARIANTREF(post_data)) == NULL\n");
        if(V_VARIANTREF(V_VARIANTREF(post_data))) {
            ok(V_VT(V_VARIANTREF(V_VARIANTREF(post_data))) == VT_EMPTY,
               "V_VT(V_VARIANTREF(V_VARIANTREF(post_data))) = %d, expected VT_EMPTY\n",
               V_VT(V_VARIANTREF(V_VARIANTREF(post_data))));

            if(V_VT(V_VARIANTREF(V_VARIANTREF(post_data))) == (VT_UI1|VT_ARRAY)) {
                const SAFEARRAY *sa = V_ARRAY(V_VARIANTREF(V_VARIANTREF(post_data)));

                ok(sa->cDims == 1, "sa->cDims = %d, expected 1\n", sa->cDims);
                ok(sa->fFeatures == 0, "sa->fFeatures = %d, expected 0\n", sa->fFeatures);
            }
        }
    }

    ok(V_VT(headers) == (VT_BYREF|VT_VARIANT), "V_VT(headers)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(headers));
    ok(V_VARIANTREF(headers) != NULL, "V_VARIANTREF(headers) == NULL)\n");
    if(V_VARIANTREF(headers)) {
        ok(V_VT(V_VARIANTREF(headers)) == VT_BSTR, "V_VT(V_VARIANTREF(headers))=%d, expected VT_BSTR\n",
           V_VT(V_VARIANTREF(headers)));
        ok(V_BSTR(V_VARIANTREF(headers)) == NULL, "V_BSTR(V_VARIANTREF(headers)) = %p, expected NULL\n",
           V_BSTR(V_VARIANTREF(headers)));
    }

    ok(V_VT(cancel) == (VT_BYREF|VT_BOOL), "V_VT(cancel)=%x, expected VT_BYREF|VT_BOOL\n",
       V_VT(cancel));
    ok(V_BOOLREF(cancel) != NULL, "V_BOOLREF(pDispParams->rgvarg[0] == NULL)\n");
    if(V_BOOLREF(cancel))
        ok(*V_BOOLREF(cancel) == VARIANT_FALSE, "*V_BOOLREF(cancel) = %x, expected VARIANT_FALSE\n",
           *V_BOOLREF(cancel));
}

static void test_navigatecomplete2(DISPPARAMS *dp)
{
    VARIANT *v;

    CHECK_EXPECT(Invoke_NAVIGATECOMPLETE2);

    ok(dp->rgvarg != NULL, "rgvarg == NULL\n");
    ok(dp->cArgs == 2, "cArgs=%d, expected 2\n", dp->cArgs);

    ok(V_VT(dp->rgvarg) == (VT_BYREF|VT_VARIANT), "V_VT(dp->rgvarg) = %d\n", V_VT(dp->rgvarg));
    v = V_VARIANTREF(dp->rgvarg);
    ok(V_VT(v) == VT_BSTR, "V_VT(url) = %d\n", V_VT(v));
    ok(!strcmp_wa(V_BSTR(v), current_url), "url=%s, expected %s\n", wine_dbgstr_w(V_BSTR(v)), current_url);

    ok(V_VT(dp->rgvarg+1) == VT_DISPATCH, "V_VT(dp->rgvarg+1) = %d\n", V_VT(dp->rgvarg+1));
    ok(V_DISPATCH(dp->rgvarg+1) == (IDispatch*)wb, "V_DISPATCH=%p, wb=%p\n", V_DISPATCH(dp->rgvarg+1), wb);

    test_ready_state((dwl_flags & (DWL_FROM_PUT_HREF|DWL_FROM_GOBACK|DWL_FROM_GOFORWARD))
                     ? READYSTATE_COMPLETE : READYSTATE_LOADING);
}

static void test_documentcomplete(DISPPARAMS *dp)
{
    VARIANT *v;

    CHECK_EXPECT(Invoke_DOCUMENTCOMPLETE);

    ok(dp->rgvarg != NULL, "rgvarg == NULL\n");
    ok(dp->cArgs == 2, "cArgs=%d, expected 2\n", dp->cArgs);

    ok(V_VT(dp->rgvarg) == (VT_BYREF|VT_VARIANT), "V_VT(dp->rgvarg) = %d\n", V_VT(dp->rgvarg));
    v = V_VARIANTREF(dp->rgvarg);
    ok(V_VT(v) == VT_BSTR, "V_VT(url) = %d\n", V_VT(v));
    ok(!strcmp_wa(V_BSTR(v), current_url), "url=%s, expected %s\n", wine_dbgstr_w(V_BSTR(v)), current_url);

    ok(V_VT(dp->rgvarg+1) == VT_DISPATCH, "V_VT(dp->rgvarg+1) = %d\n", V_VT(dp->rgvarg+1));
    ok(V_DISPATCH(dp->rgvarg+1) == (IDispatch*)wb, "V_DISPATCH=%p, wb=%p\n", V_DISPATCH(dp->rgvarg+1), wb);

    test_ready_state(READYSTATE_COMPLETE);
}

static HRESULT WINAPI WebBrowserEvents2_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(pVarResult == NULL, "pVarResult=%p\n", pVarResult);
    ok(wFlags == DISPATCH_METHOD, "wFlags=%08x, expected DISPATCH_METHOD\n", wFlags);
    ok(pDispParams->rgdispidNamedArgs == NULL,
       "pDispParams->rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
    ok(pDispParams->cNamedArgs == 0, "pDispParams->cNamedArgs = %d\n", pDispParams->cNamedArgs);

    switch(dispIdMember) {
    case DISPID_STATUSTEXTCHANGE:
        CHECK_EXPECT2(Invoke_STATUSTEXTCHANGE);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 1, "cArgs=%d, expected 1\n", pDispParams->cArgs);
        ok(V_VT(pDispParams->rgvarg) == VT_BSTR, "V_VT(pDispParams->rgvarg)=%d, expected VT_BSTR\n",
           V_VT(pDispParams->rgvarg));
        /* TODO: Check text */
        break;

    case DISPID_PROPERTYCHANGE:
        CHECK_EXPECT2(Invoke_PROPERTYCHANGE);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 1, "cArgs=%d, expected 1\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_DOWNLOADBEGIN:
        CHECK_EXPECT(Invoke_DOWNLOADBEGIN);

        ok(pDispParams->rgvarg == NULL, "rgvarg=%p, expected NULL\n", pDispParams->rgvarg);
        ok(pDispParams->cArgs == 0, "cArgs=%d, expected 0\n", pDispParams->cArgs);
        test_ready_state(READYSTATE_LOADING);
        break;

    case DISPID_BEFORENAVIGATE2:
        CHECK_EXPECT(Invoke_BEFORENAVIGATE2);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 7, "cArgs=%d, expected 7\n", pDispParams->cArgs);
        test_OnBeforeNavigate(pDispParams->rgvarg+6, pDispParams->rgvarg+5, pDispParams->rgvarg+4,
                              pDispParams->rgvarg+3, pDispParams->rgvarg+2, pDispParams->rgvarg+1,
                              pDispParams->rgvarg);
        test_ready_state((dwl_flags & (DWL_FROM_PUT_HREF|DWL_FROM_GOFORWARD)) ? READYSTATE_COMPLETE : READYSTATE_LOADING);
        break;

    case DISPID_SETSECURELOCKICON:
        CHECK_EXPECT2(Invoke_SETSECURELOCKICON);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 1, "cArgs=%d, expected 1\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_FILEDOWNLOAD:
        CHECK_EXPECT(Invoke_FILEDOWNLOAD);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 2, "cArgs=%d, expected 2\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_COMMANDSTATECHANGE:
        CHECK_EXPECT2(Invoke_COMMANDSTATECHANGE);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 2, "cArgs=%d, expected 2\n", pDispParams->cArgs);

        ok(V_VT(pDispParams->rgvarg) == VT_BOOL, "V_VT(pDispParams->rgvarg) = %d\n", V_VT(pDispParams->rgvarg));
        ok(V_VT(pDispParams->rgvarg+1) == VT_I4, "V_VT(pDispParams->rgvarg+1) = %d\n", V_VT(pDispParams->rgvarg+1));

        /* TODO: Check args */
        break;

    case DISPID_DOWNLOADCOMPLETE:
        CHECK_EXPECT(Invoke_DOWNLOADCOMPLETE);

        ok(pDispParams->rgvarg == NULL, "rgvarg=%p, expected NULL\n", pDispParams->rgvarg);
        ok(pDispParams->cArgs == 0, "cArgs=%d, expected 0\n", pDispParams->cArgs);
        test_ready_state(READYSTATE_LOADING);
        break;

    case DISPID_ONMENUBAR:
        CHECK_EXPECT(Invoke_ONMENUBAR);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONADDRESSBAR:
        CHECK_EXPECT(Invoke_ONADDRESSBAR);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONSTATUSBAR:
        CHECK_EXPECT(Invoke_ONSTATUSBAR);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONTOOLBAR:
        CHECK_EXPECT(Invoke_ONTOOLBAR);
        test_invoke_bool(pDispParams, FALSE);
        break;

    case DISPID_ONFULLSCREEN:
        CHECK_EXPECT(Invoke_ONFULLSCREEN);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONTHEATERMODE:
        CHECK_EXPECT(Invoke_ONTHEATERMODE);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_WINDOWSETRESIZABLE:
        CHECK_EXPECT(Invoke_WINDOWSETRESIZABLE);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_TITLECHANGE:
        CHECK_EXPECT2(Invoke_TITLECHANGE);
        /* FIXME */
        break;

    case DISPID_NAVIGATECOMPLETE2:
        test_navigatecomplete2(pDispParams);
        break;

    case DISPID_PROGRESSCHANGE:
        CHECK_EXPECT2(Invoke_PROGRESSCHANGE);
        /* FIXME */
        break;

    case DISPID_DOCUMENTCOMPLETE:
        test_documentcomplete(pDispParams);
        break;

    case DISPID_WINDOWCLOSING: {
        VARIANT *is_child = pDispParams->rgvarg+1, *cancel = pDispParams->rgvarg;

        CHECK_EXPECT(Invoke_WINDOWCLOSING);

        ok(pDispParams->cArgs == 2, "pdp->cArgs = %d\n", pDispParams->cArgs);
        ok(V_VT(is_child) == VT_BOOL, "V_VT(is_child) = %d\n", V_VT(is_child));
        ok(!V_BOOL(is_child), "V_BOOL(is_child) = %x\n", V_BOOL(is_child));
        ok(V_VT(cancel) == (VT_BYREF|VT_BOOL), "V_VT(cancel) = %d\n", V_VT(cancel));
        ok(!*V_BOOLREF(cancel), "*V_BOOLREF(cancel) = %x\n", *V_BOOLREF(cancel));

        *V_BOOLREF(cancel) = VARIANT_TRUE;
        return S_OK;
    }

    case 282: /* FIXME */
        CHECK_EXPECT2(Invoke_282);
        break;

    case 290: /* FIXME: IE10 */
        break;

    default:
        ok(0, "unexpected dispIdMember %d\n", dispIdMember);
    }

    return S_OK;
}

static IDispatchVtbl WebBrowserEvents2Vtbl = {
    WebBrowserEvents2_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    WebBrowserEvents2_Invoke
};

static IDispatch WebBrowserEvents2 = { &WebBrowserEvents2Vtbl };

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    CHECK_EXPECT(GetContainer);

    ok(ppContainer != NULL, "ppContainer == NULL\n");
    if(ppContainer)
        *ppContainer = &OleContainer;

    return S_OK;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    CHECK_EXPECT(ShowObject);
    return S_OK;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI IOleControlSite_fnQueryInterface(IOleControlSite *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IOleControlSite_fnAddRef(IOleControlSite *iface)
{
    return 2;
}

static ULONG WINAPI IOleControlSite_fnRelease(IOleControlSite *iface)
{
    return 1;
}

static HRESULT WINAPI IOleControlSite_fnOnControlInfoChanged(IOleControlSite* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnLockInPlaceActive(IOleControlSite* This,
                                                          BOOL fLock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnGetExtendedControl(IOleControlSite* This,
                                                           IDispatch **ppDisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnTransformCoords(IOleControlSite* This,
                                                        POINTL *pPtlHimetric,
                                                        POINTF *pPtfContainer,
                                                        DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnTranslateAccelerator(IOleControlSite* This, MSG *pMsg,
                                                             DWORD grfModifiers)
{
    CHECK_EXPECT(ControlSite_TranslateAccelerator);
    return hr_site_TranslateAccelerator;
}

static HRESULT WINAPI IOleControlSite_fnOnFocus(IOleControlSite* This, BOOL fGotFocus)
{
    CHECK_EXPECT2(OnFocus);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnShowPropertyFrame(IOleControlSite* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IOleControlSiteVtbl ControlSiteVtbl = {
    IOleControlSite_fnQueryInterface,
    IOleControlSite_fnAddRef,
    IOleControlSite_fnRelease,
    IOleControlSite_fnOnControlInfoChanged,
    IOleControlSite_fnLockInPlaceActive,
    IOleControlSite_fnGetExtendedControl,
    IOleControlSite_fnTransformCoords,
    IOleControlSite_fnTranslateAccelerator,
    IOleControlSite_fnOnFocus,
    IOleControlSite_fnShowPropertyFrame
};

static IOleControlSite ControlSite = { &ControlSiteVtbl };

static HRESULT WINAPI InPlaceUIWindow_QueryInterface(IOleInPlaceFrame *iface,
                                                     REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceUIWindow_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceUIWindow_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceUIWindow_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    CHECK_EXPECT(Frame_GetWindow);
    ok(phwnd != NULL, "phwnd == NULL\n");
    if(phwnd)
        *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceUIWindow_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    CHECK_EXPECT(UIWindow_SetActiveObject);
    if(!test_close) {
        ok(pActiveObject != NULL, "pActiveObject = NULL\n");
        ok(!lstrcmpW(pszObjName, wszItem), "unexpected pszObjName\n");
    } else {
        ok(!pActiveObject, "pActiveObject != NULL\n");
        ok(!pszObjName, "pszObjName != NULL\n");
    }
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    CHECK_EXPECT(Frame_SetActiveObject);
    if(!test_close) {
        ok(pActiveObject != NULL, "pActiveObject = NULL\n");
        ok(!lstrcmpW(pszObjName, wszItem), "unexpected pszObjName\n");
    } else {
        ok(!pActiveObject, "pActiveObject != NULL\n");
        ok(!pszObjName, "pszObjName != NULL\n");
    }
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    CHECK_EXPECT(SetMenu);
    ok(hmenuShared == NULL, "hmenuShared=%p\n", hmenuShared);
    ok(holemenu == NULL, "holemenu=%p\n", holemenu);
    ok(hwndActiveObject == shell_embedding_hwnd, "hwndActiveObject=%p, expected %p\n",
       hwndActiveObject, shell_embedding_hwnd);
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    CHECK_EXPECT2(SetStatusText);
    /* FIXME: Check pszStatusText */
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    if(fEnable)
        CHECK_EXPECT2(EnableModeless_TRUE);
    else
        CHECK_EXPECT2(EnableModeless_FALSE);
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceUIWindowVtbl = {
    InPlaceUIWindow_QueryInterface,
    InPlaceUIWindow_AddRef,
    InPlaceUIWindow_Release,
    InPlaceUIWindow_GetWindow,
    InPlaceUIWindow_ContextSensitiveHelp,
    InPlaceUIWindow_GetBorder,
    InPlaceUIWindow_RequestBorderSpace,
    InPlaceUIWindow_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

static IOleInPlaceUIWindow InPlaceUIWindow = { (IOleInPlaceUIWindowVtbl*)&InPlaceUIWindowVtbl };

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceUIWindow_QueryInterface,
    InPlaceUIWindow_AddRef,
    InPlaceUIWindow_Release,
    InPlaceFrame_GetWindow,
    InPlaceUIWindow_ContextSensitiveHelp,
    InPlaceUIWindow_GetBorder,
    InPlaceUIWindow_RequestBorderSpace,
    InPlaceUIWindow_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSiteEx *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSiteEx *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSiteEx *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSiteEx *iface, HWND *phwnd)
{
    CHECK_EXPECT(Site_GetWindow);
    ok(phwnd != NULL, "phwnd == NULL\n");
    if(phwnd)
        *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSiteEx *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(CanInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnUIActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSiteEx *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT pos_rect = {2,1,1002,901};
    static const RECT clip_rect = {10,10,990,890};

    CHECK_EXPECT(GetWindowContext);

    ok(ppFrame != NULL, "ppFrame = NULL\n");
    if(ppFrame)
        *ppFrame = &InPlaceFrame;

    ok(ppDoc != NULL, "ppDoc = NULL\n");
    if(ppDoc)
        *ppDoc = &InPlaceUIWindow;

    ok(lprcPosRect != NULL, "lprcPosRect = NULL\n");
    if(lprcPosRect)
        memcpy(lprcPosRect, &pos_rect, sizeof(RECT));

    ok(lprcClipRect != NULL, "lprcClipRect = NULL\n");
    if(lprcClipRect)
        memcpy(lprcClipRect, &clip_rect, sizeof(RECT));

    ok(lpFrameInfo != NULL, "lpFrameInfo = NULL\n");
    if(lpFrameInfo) {
        ok(lpFrameInfo->cb == sizeof(*lpFrameInfo), "lpFrameInfo->cb = %u, expected %u\n", lpFrameInfo->cb, (unsigned)sizeof(*lpFrameInfo));
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = container_hwnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
    }

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSiteEx *iface, SIZE scrollExtant)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSiteEx *iface, BOOL fUndoable)
{
    CHECK_EXPECT(OnUIDeactivate);
    ok(!fUndoable, "fUndoable should be FALSE\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnInPlaceDeactivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSiteEx *iface, LPCRECT lprcPosRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivateEx(IOleInPlaceSiteEx *iface,
                                                      BOOL *pNoRedraw, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivateEx(IOleInPlaceSiteEx *iface,
                                                        BOOL fNoRedraw)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_RequestUIActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT2(RequestUIActivate);
    return S_OK;
}

static const IOleInPlaceSiteExVtbl InPlaceSiteExVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange,
    InPlaceSite_OnInPlaceActivateEx,
    InPlaceSite_OnInPlaceDeactivateEx,
    InPlaceSite_RequestUIActivate
};

static IOleInPlaceSiteEx InPlaceSite = { &InPlaceSiteExVtbl };

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    return 2;
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    return 1;
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface, DWORD dwID, POINT *ppt,
        IUnknown *pcmdtReserved, IDispatch *pdicpReserved)
{
    ok(0, "unexpected call %d %p %p %p\n", dwID, ppt, pcmdtReserved, pdicpReserved);
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    CHECK_EXPECT2(GetHostInfo);
    ok(pInfo != NULL, "pInfo=NULL\n");
    if(pInfo) {
        ok(pInfo->cbSize == sizeof(DOCHOSTUIINFO) || broken(!pInfo->cbSize), "pInfo->cbSize=%u\n", pInfo->cbSize);
        ok(!pInfo->dwFlags, "pInfo->dwFlags=%08x, expected 0\n", pInfo->dwFlags);
        ok(!pInfo->dwDoubleClick, "pInfo->dwDoubleClick=%08x, expected 0\n", pInfo->dwDoubleClick);
        ok(!pInfo->pchHostCss, "pInfo->pchHostCss=%p, expected NULL\n", pInfo->pchHostCss);
        ok(!pInfo->pchHostNS, "pInfo->pchhostNS=%p, expected NULL\n", pInfo->pchHostNS);
    }
    return E_FAIL;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    CHECK_EXPECT(ShowUI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(HideUI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT2(UpdateUI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface, BOOL fEnable)
{
    if(fEnable)
        CHECK_EXPECT(DocHost_EnableModeless_TRUE);
    else
        CHECK_EXPECT(DocHost_EnableModeless_FALSE);

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface, LPCRECT prcBorder,
        IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface, LPMSG lpMsg,
        const GUID *pguidCmdGroup, DWORD nCmdID)
{
    CHECK_EXPECT(DocHost_TranslateAccelerator);
    ok(pguidCmdGroup != NULL, "Got NULL pguidCmdGroup.\n");
    if(pguidCmdGroup)
        ok(IsEqualGUID(pguidCmdGroup, &CGID_MSHTML), "Unexpected pguidCmdGroup\n");
    ok(lpMsg != NULL, "Got NULL lpMsg.\n");
    return hr_dochost_TranslateAccelerator;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOptionKeyPath);
    ok(pchKey != NULL, "pchKey==NULL\n");
    if(pchKey)
        ok(*pchKey == NULL, "*pchKey=%p\n", *pchKey);
    ok(!dw, "dw=%x\n", dw);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    CHECK_EXPECT(GetDropTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface, IDispatch **ppDispatch)
{
    CHECK_EXPECT(GetExternal);
    *ppDispatch = NULL;
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    if(is_downloading && !(dwl_flags & DWL_EXPECT_BEFORE_NAVIGATE))
        todo_wine CHECK_EXPECT(TranslateUrl);
    else
        CHECK_EXPECT(TranslateUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface, IDataObject *pDO,
        IDataObject **ppPORet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOverridesKeyPath);
    ok(pchKey != NULL, "pchKey==NULL\n");
    if(pchKey)
        ok(*pchKey == NULL, "*pchKey=%p\n", *pchKey);
    ok(!dw, "dw=%x\n", dw);
    return E_NOTIMPL;
}

static const IDocHostUIHandler2Vtbl DocHostUIHandlerVtbl = {
    DocHostUIHandler_QueryInterface,
    DocHostUIHandler_AddRef,
    DocHostUIHandler_Release,
    DocHostUIHandler_ShowContextMenu,
    DocHostUIHandler_GetHostInfo,
    DocHostUIHandler_ShowUI,
    DocHostUIHandler_HideUI,
    DocHostUIHandler_UpdateUI,
    DocHostUIHandler_EnableModeless,
    DocHostUIHandler_OnDocWindowActivate,
    DocHostUIHandler_OnFrameWindowActivate,
    DocHostUIHandler_ResizeBorder,
    DocHostUIHandler_TranslateAccelerator,
    DocHostUIHandler_GetOptionKeyPath,
    DocHostUIHandler_GetDropTarget,
    DocHostUIHandler_GetExternal,
    DocHostUIHandler_TranslateUrl,
    DocHostUIHandler_FilterDataObject,
    DocHostUIHandler_GetOverrideKeyPath
};

static IDocHostUIHandler2 DocHostUIHandler = { &DocHostUIHandlerVtbl };


static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
                                    REFGUID guidService, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(!winetest_interactive)
        return E_NOINTERFACE;

    if (IsEqualGUID(&SID_STopLevelBrowser, guidService))
        trace("Service SID_STopLevelBrowser\n");
    else if (IsEqualGUID(&SID_SEditCommandTarget, guidService))
        trace("Service SID_SEditCommandTarget\n");
    else if (IsEqualGUID(&IID_ITargetFrame2, guidService))
        trace("Service IID_ITargetFrame2\n");
    else if (IsEqualGUID(&SID_SInternetSecurityManager, guidService))
        trace("Service SID_SInternetSecurityManager\n");
    else if (IsEqualGUID(&SID_SOleUndoManager, guidService))
        trace("Service SID_SOleUndoManager\n");
    else if (IsEqualGUID(&SID_IMimeInfo, guidService))
        trace("Service SID_IMimeInfo\n");
    else if (IsEqualGUID(&SID_STopWindow, guidService))
        trace("Service SID_STopWindow\n");

    /* 30D02401-6A81-11D0-8274-00C04FD5AE38 Explorer Bar: Search */
    /* D1E7AFEC-6A2E-11D0-8C78-00C04FD918B4 no info */
    /* A9227C3C-7F8E-11D0-8CB0-00A0C92DBFE8 no info */
    /* 371EA634-DC5C-11D1-BA57-00C04FC2040E one reference to IVersionHost */
    /* 3050F429-98B5-11CF-BB82-00AA00BDCE0B IID_IElementBehaviorFactory */
    /* 6D12FE80-7911-11CF-9534-0000C05BAE0B SID_DefView */
    /* AD7F6C62-F6BD-11D2-959B-006097C553C8 no info */
    /* 53A2D5B1-D2FC-11D0-84E0-006097C9987D no info */
    /* 3050F312-98B5-11CF-BB82-00AA00BDCE0B HTMLFrameBaseClass */
    /* 639447BD-B2D3-44B9-9FB0-510F23CB45E4 no info */
    /* 20C46561-8491-11CF-960C-0080C7F4EE85 no info */

    else
        trace("Service %s not supported\n", wine_dbgstr_guid(guidService));

    return E_NOINTERFACE;
}


static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };


static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleWindow, riid)
            || IsEqualGUID(&IID_IOleInPlaceSite, riid)
            || IsEqualGUID(&IID_IOleInPlaceSiteEx, riid))
        *ppv = &InPlaceSite;
    else if(IsEqualGUID(&IID_IDocHostUIHandler, riid)
            || IsEqualGUID(&IID_IDocHostUIHandler2, riid))
        *ppv = use_container_dochostui ? &DocHostUIHandler : NULL;
    else if(IsEqualGUID(&IID_IDispatch, riid))
        *ppv = &Dispatch;
    else if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &ServiceProvider;
    else if(IsEqualGUID(&IID_IDocHostShowUI, riid))
        trace("interface IID_IDocHostShowUI\n");
    else if(IsEqualGUID(&IID_IOleControlSite, riid))
        *ppv = &ControlSite;
    else if(IsEqualGUID(&IID_IOleCommandTarget, riid))
        trace("interface IID_IOleCommandTarget\n");

    /* B6EA2050-048A-11D1-82B9-00C04FB9942E IAxWinHostWindow */

    else
    {
        /* are there more interfaces, that a host can support? */
        trace("%s: interface not supported\n", wine_dbgstr_guid(riid));
    }

    return (*ppv) ? S_OK : E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND create_container_window(void)
{
    static const WCHAR wszWebBrowserContainer[] =
        {'W','e','b','B','r','o','w','s','e','r','C','o','n','t','a','i','n','e','r',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszWebBrowserContainer,
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(wszWebBrowserContainer, wszWebBrowserContainer,
            WS_OVERLAPPEDWINDOW, 10, 10, 600, 600, NULL, NULL, NULL, NULL);
}

static void test_DoVerb(IWebBrowser2 *unk)
{
    IOleObject *oleobj;
    RECT rect = {0,0,1000,1000};
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    SET_EXPECT(OnUIActivate);
    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(SetMenu);
    SET_EXPECT(OnFocus);

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                             0, (HWND)0xdeadbeef, &rect);
    ok(hres == S_OK, "DoVerb failed: %08x\n", hres);

    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);
    CHECK_CALLED(OnUIActivate);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(SetMenu);
    todo_wine CHECK_CALLED(OnFocus);

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                           0, (HWND)0xdeadbeef, &rect);
    ok(hres == S_OK, "DoVerb failed: %08x\n", hres);

    IOleObject_Release(oleobj);
}

static void call_DoVerb(IWebBrowser2 *unk, LONG verb)
{
    IOleObject *oleobj;
    RECT rect = {60,60,600,600};
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleObject_DoVerb(oleobj, verb, NULL, &ClientSite,
                             -1, container_hwnd, &rect);
    ok(hres == S_OK, "DoVerb failed: %08x\n", hres);

    IOleObject_Release(oleobj);
}

static HWND get_hwnd(IWebBrowser2 *unk)
{
    IOleInPlaceObject *inplace;
    HWND hwnd;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceObject, (void**)&inplace);
    ok(hres == S_OK, "QueryInterface(IID_OleInPlaceObject) failed: %08x\n", hres);

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);

    IOleInPlaceObject_Release(inplace);
    return hwnd;
}

static void test_GetMiscStatus(IOleObject *oleobj)
{
    DWORD st, i;
    HRESULT hres;

    for(i=0; i<10; i++) {
        st = 0xdeadbeef;
        hres = IOleObject_GetMiscStatus(oleobj, i, &st);
        ok(hres == S_OK, "GetMiscStatus failed: %08x\n", hres);
        ok(st == (OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
                  |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE),
           "st=%08x, expected OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|"
           "OLEMISC_INSIDEOUT|OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE)\n", st);
    }
}

static void test_SetHostNames(IOleObject *oleobj)
{
    HRESULT hres;

    static const WCHAR test_appW[] =  {'t','e','s','t',' ','a','p','p',0};

    hres = IOleObject_SetHostNames(oleobj, test_appW, (void*)0xdeadbeef);
    ok(hres == S_OK, "SetHostNames failed: %08x\n", hres);
}

static void test_ClientSite(IWebBrowser2 *unk, IOleClientSite *client, BOOL stop_download)
{
    IOleObject *oleobj;
    IOleInPlaceObject *inplace;
    HWND hwnd;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    test_GetMiscStatus(oleobj);
    test_SetHostNames(oleobj);

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceObject, (void**)&inplace);
    ok(hres == S_OK, "QueryInterface(IID_OleInPlaceObject) failed: %08x\n", hres);
    if(FAILED(hres)) {
        IOleObject_Release(oleobj);
        return;
    }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
    ok((hwnd == NULL) ^ (client == NULL), "unexpected hwnd %p\n", hwnd);

    if(client) {
        SET_EXPECT(GetContainer);
        SET_EXPECT(Site_GetWindow);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
    }else if(stop_download) {
        SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
        if (use_container_olecmd) SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
        SET_EXPECT(Invoke_COMMANDSTATECHANGE);
    }else {
        SET_EXPECT(Invoke_STATUSTEXTCHANGE);
        SET_EXPECT(Invoke_PROGRESSCHANGE);
    }

    hres = IOleObject_SetClientSite(oleobj, client);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

    if(client) {
        CHECK_CALLED(GetContainer);
        CHECK_CALLED(Site_GetWindow);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
    }else if(stop_download) {
        todo_wine CHECK_CALLED(Invoke_DOWNLOADCOMPLETE);
        if (use_container_olecmd) todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
        todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
    }else {
        CLEAR_CALLED(Invoke_STATUSTEXTCHANGE); /* Called by IE9 */
        CLEAR_CALLED(Invoke_PROGRESSCHANGE);
    }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
    ok((hwnd == NULL) == (client == NULL), "unexpected hwnd %p\n", hwnd);

    shell_embedding_hwnd = hwnd;

    test_SetHostNames(oleobj);

    IOleInPlaceObject_Release(inplace);
    IOleObject_Release(oleobj);
}

static void test_ClassInfo(IWebBrowser2 *unk)
{
    IProvideClassInfo2 *class_info;
    TYPEATTR *type_attr;
    ITypeInfo *typeinfo;
    GUID guid;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "QueryInterface(IID_IProvideClassInfo)  failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08x\n", hres);
    ok(IsEqualGUID(wb_version > 1 ? &DIID_DWebBrowserEvents2 : &DIID_DWebBrowserEvents, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 0, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08x, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 2, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08x, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08x, expected E_POINTER\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, 0, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08x, expected E_POINTER\n", hres);

    typeinfo = NULL;
    hres = IProvideClassInfo2_GetClassInfo(class_info, &typeinfo);
    ok(hres == S_OK, "GetClassInfo failed: %08x\n", hres);
    ok(typeinfo != NULL, "typeinfo == NULL\n");

    hres = ITypeInfo_GetTypeAttr(typeinfo, &type_attr);
    ok(hres == S_OK, "GetTypeAtr failed: %08x\n", hres);

    ok(IsEqualGUID(&type_attr->guid, wb_version > 1 ? &CLSID_WebBrowser : &CLSID_WebBrowser_V1),
       "guid = %s\n", wine_dbgstr_guid(&type_attr->guid));

    ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
    ITypeInfo_Release(typeinfo);

    IProvideClassInfo2_Release(class_info);
}

static void test_EnumVerbs(IWebBrowser2 *wb)
{
    IEnumOLEVERB *enum_verbs;
    IOleObject *oleobj;
    OLEVERB verbs[20];
    ULONG fetched;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08x\n", hres);

    hres = IOleObject_EnumVerbs(oleobj, &enum_verbs);
    IOleObject_Release(oleobj);
    ok(hres == S_OK, "EnumVerbs failed: %08x\n", hres);
    ok(enum_verbs != NULL, "enum_verbs == NULL\n");

    fetched = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, sizeof(verbs)/sizeof(*verbs), verbs, &fetched);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(!fetched, "fetched = %d\n", fetched);

    fetched = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, 1, verbs, &fetched);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(!fetched, "fetched = %d\n", fetched);

    hres = IEnumOLEVERB_Reset(enum_verbs);
    ok(hres == S_OK, "Reset failed: %08x\n", hres);

    fetched = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, 1, verbs, &fetched);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(!fetched, "fetched = %d\n", fetched);

    hres = IEnumOLEVERB_Next(enum_verbs, 1, verbs, NULL);
    ok(hres == S_OK, "Next failed: %08x\n", hres);

    hres = IEnumOLEVERB_Skip(enum_verbs, 20);
    ok(hres == S_OK, "Reset failed: %08x\n", hres);

    fetched = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, 1, verbs, &fetched);
    ok(hres == S_OK, "Next failed: %08x\n", hres);
    ok(!fetched, "fetched = %d\n", fetched);

    IEnumOLEVERB_Release(enum_verbs);
}

static void test_ie_funcs(IWebBrowser2 *wb)
{
    IDispatch *disp;
    VARIANT_BOOL b;
    int i;
    SHANDLE_PTR hwnd;
    HRESULT hres;
    BSTR sName;

    /* HWND */

    hwnd = 0xdeadbeef;
    hres = IWebBrowser2_get_HWND(wb, &hwnd);
    ok(hres == E_FAIL, "get_HWND failed: %08x, expected E_FAIL\n", hres);
    ok(hwnd == 0, "unexpected hwnd %p\n", (PVOID)hwnd);

    /* MenuBar */

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONMENUBAR);
    hres = IWebBrowser2_put_MenuBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_MenuBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONMENUBAR);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONMENUBAR);
    hres = IWebBrowser2_put_MenuBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_MenuBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONMENUBAR);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* AddressBar */

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_AddressBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_AddressBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_MenuBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    /* StatusBar */

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_StatusBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_StatusBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_StatusBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* ToolBar */

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08x\n", hres);
    ok(i, "i=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_ToolBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08x\n", hres);
    ok(i == VARIANT_FALSE, "b=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_ToolBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08x\n", hres);
    ok(i, "i=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_ToolBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    /* FullScreen */

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_FullScreen failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = 100));
    ok(hres == S_OK, "put_FullScreen failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_FullScreen failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    /* TheaterMode */

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_TheaterMode failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = 100));
    ok(hres == S_OK, "put_TheaterMode failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_TheaterMode failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    /* Resizable */

    b = 0x100;
    hres = IWebBrowser2_get_Resizable(wb, &b);
    ok(hres == E_NOTIMPL, "get_Resizable failed: %08x\n", hres);
    ok(b == 0x100, "b=%x\n", b);

    SET_EXPECT(Invoke_WINDOWSETRESIZABLE);
    hres = IWebBrowser2_put_Resizable(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_Resizable failed: %08x\n", hres);
    CHECK_CALLED(Invoke_WINDOWSETRESIZABLE);

    SET_EXPECT(Invoke_WINDOWSETRESIZABLE);
    hres = IWebBrowser2_put_Resizable(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_Resizable failed: %08x\n", hres);
    CHECK_CALLED(Invoke_WINDOWSETRESIZABLE);

    hres = IWebBrowser2_get_Resizable(wb, &b);
    ok(hres == E_NOTIMPL, "get_Resizable failed: %08x\n", hres);
    ok(b == 0x100, "b=%x\n", b);

    /* Application */

    disp = NULL;
    hres = IWebBrowser2_get_Application(wb, &disp);
    ok(hres == S_OK, "get_Application failed: %08x\n", hres);
    ok(disp == (void*)wb, "disp=%p, expected %p\n", disp, wb);
    if(disp)
        IDispatch_Release(disp);

    hres = IWebBrowser2_get_Application(wb, NULL);
    ok(hres == E_POINTER, "get_Application failed: %08x, expected E_POINTER\n", hres);

    /* Name */
    hres = IWebBrowser2_get_Name(wb, &sName);
    ok(hres == S_OK, "getName failed: %08x, expected S_OK\n", hres);
    if (is_lang_english())
        ok(!strcmp_wa(sName, "Microsoft Web Browser Control"), "got '%s', expected 'Microsoft Web Browser Control'\n", wine_dbgstr_w(sName));
    else /* Non-English cannot be blank. */
        ok(sName!=NULL, "get_Name return a NULL string.\n");
    SysFreeString(sName);

    /* Quit */

    hres = IWebBrowser2_Quit(wb);
    ok(hres == E_FAIL, "Quit failed: %08x, expected E_FAIL\n", hres);
}

static void test_Silent(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, VARIANT_TRUE);
    ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, 100);
    ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, VARIANT_FALSE);
    ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    if(is_clientsite) {
        hres = IWebBrowser2_put_Silent(wb, VARIANT_TRUE);
        ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

        SET_EXPECT(Invoke_AMBIENT_SILENT);
    }

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_SILENT);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08x\n", hres);

    if(is_clientsite)
        CHECK_CALLED(Invoke_AMBIENT_SILENT);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);
}

static void test_Offline(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, VARIANT_TRUE);
    ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, 100);
    ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, VARIANT_FALSE);
    ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    if(is_clientsite) {
        hres = IWebBrowser2_put_Offline(wb, VARIANT_TRUE);
        ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    }

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08x\n", hres);

    if(is_clientsite)
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);
}

static void test_ambient_unknown(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    HRESULT hres;

    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_USERMODE);
    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_USERAGENT);
    SET_EXPECT(Invoke_AMBIENT_PALETTE);

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_UNKNOWN);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08x\n", hres);

    CHECK_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_EXPECT(Invoke_AMBIENT_SILENT);
    CHECK_EXPECT(Invoke_AMBIENT_USERMODE);
    CHECK_EXPECT(Invoke_AMBIENT_DLCONTROL);
    CHECK_EXPECT(Invoke_AMBIENT_USERAGENT);
    CHECK_EXPECT(Invoke_AMBIENT_PALETTE);

    CLEAR_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CLEAR_CALLED(Invoke_AMBIENT_SILENT);
    CLEAR_CALLED(Invoke_AMBIENT_USERMODE);
    CLEAR_CALLED(Invoke_AMBIENT_DLCONTROL);
    CLEAR_CALLED(Invoke_AMBIENT_USERAGENT);
    CLEAR_CALLED(Invoke_AMBIENT_PALETTE);
}

static void test_wb_funcs(IWebBrowser2 *wb, BOOL is_clientsite)
{
    IOleControl *control;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "Could not get IOleControl interface: %08x\n", hres);

    test_Silent(wb, control, is_clientsite);
    test_Offline(wb, control, is_clientsite);
    test_ambient_unknown(wb, control, is_clientsite);

    IOleControl_Release(control);
}

static void test_GetControlInfo(IWebBrowser2 *unk)
{
    IOleControl *control;
    CONTROLINFO info;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "Could not get IOleControl: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleControl_GetControlInfo(control, &info);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08x, expected E_NOTIMPL\n", hres);
    hres = IOleControl_GetControlInfo(control, NULL);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08x, expected E_NOTIMPL\n", hres);

    IOleControl_Release(control);
}

static void test_Extent(IWebBrowser2 *unk)
{
    IOleObject *oleobj;
    SIZE size, expected;
    HRESULT hres;
    DWORD dpi_x;
    DWORD dpi_y;
    HDC hdc;

    /* default aspect ratio is 96dpi / 96dpi */
    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);
    if (dpi_x != 96 || dpi_y != 96)
        trace("dpi: %d / %d\n", dpi_y, dpi_y);

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not get IOleObkect: %08x\n", hres);
    if(FAILED(hres))
        return;

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    /* Default size is 50x20 pixels, in himetric units */
    expected.cx = MulDiv( 50, 2540, dpi_x );
    expected.cy = MulDiv( 20, 2540, dpi_y );
    ok(size.cx == expected.cx && size.cy == expected.cy, "size = {%d %d} (expected %d %d)\n",
       size.cx, size.cy, expected.cx, expected.cy );

    size.cx = 800;
    size.cy = 700;
    hres = IOleObject_SetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "SetExtent failed: %08x\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 800 && size.cy == 700, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 800 && size.cy == 700, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = 900;
    size.cy = 800;
    hres = IOleObject_SetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "SetExtent failed: %08x\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 900 && size.cy == 800, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 900 && size.cy == 800, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = 1000;
    size.cy = 900;
    hres = IOleObject_SetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "SetExtent failed: %08x\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 1000 && size.cy == 900, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 1000 && size.cy == 900, "size = {%d %d}\n", size.cx, size.cy);

    IOleObject_Release(oleobj);
}

static void test_ConnectionPoint(IWebBrowser2 *unk, BOOL init)
{
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    HRESULT hres;

    static DWORD dw = 100;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &DIID_DWebBrowserEvents2, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    if(init) {
        hres = IConnectionPoint_Advise(point, (IUnknown*)&WebBrowserEvents2, &dw);
        ok(hres == S_OK, "Advise failed: %08x\n", hres);
        ok(dw == 1, "dw=%d, expected 1\n", dw);
    }else {
        hres = IConnectionPoint_Unadvise(point, dw);
        ok(hres == S_OK, "Unadvise failed: %08x\n", hres);
    }

    IConnectionPoint_Release(point);
}

static void test_Navigate2(IWebBrowser2 *webbrowser, const char *nav_url)
{
    VARIANT url;
    BOOL is_file;
    HRESULT hres;

    test_LocationURL(webbrowser, is_first_load ? "" : current_url);
    test_ready_state(is_first_load ? READYSTATE_UNINITIALIZED : READYSTATE_COMPLETE);

    is_http = !memcmp(nav_url, "http:", 5);
    V_VT(&url) = VT_BSTR;
    V_BSTR(&url) = a2bstr(current_url = nav_url);

    if((is_file = !strncasecmp(nav_url, "file://", 7)))
        current_url = nav_url + 7;

    if(is_first_load) {
        SET_EXPECT(Invoke_AMBIENT_USERMODE);
        SET_EXPECT(Invoke_PROPERTYCHANGE);
        SET_EXPECT(Invoke_BEFORENAVIGATE2);
        SET_EXPECT(Invoke_DOWNLOADBEGIN);
        if (use_container_olecmd) SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
        SET_EXPECT(EnableModeless_FALSE);
        SET_EXPECT(Invoke_STATUSTEXTCHANGE);
        SET_EXPECT(SetStatusText);
        if(use_container_dochostui)
            SET_EXPECT(GetHostInfo);
        SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
        SET_EXPECT(Invoke_AMBIENT_USERAGENT);
        SET_EXPECT(Invoke_AMBIENT_PALETTE);
        if(use_container_dochostui) {
            SET_EXPECT(GetOptionKeyPath);
            SET_EXPECT(GetOverridesKeyPath);
        }
        if (use_container_olecmd) SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
        if (use_container_olecmd) SET_EXPECT(Exec_SETPROGRESSMAX);
        if (use_container_olecmd) SET_EXPECT(Exec_SETPROGRESSPOS);
        SET_EXPECT(Invoke_SETSECURELOCKICON);
        SET_EXPECT(Invoke_FILEDOWNLOAD);
        if (use_container_olecmd) SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
        SET_EXPECT(Invoke_COMMANDSTATECHANGE);
        SET_EXPECT(EnableModeless_TRUE);
        if (!use_container_olecmd) SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
        if (is_file) SET_EXPECT(Invoke_PROGRESSCHANGE);
    }

    hres = IWebBrowser2_Navigate2(webbrowser, &url, NULL, NULL, NULL, NULL);
    ok(hres == S_OK, "Navigate2 failed: %08x\n", hres);

    if(is_first_load) {
        CHECK_CALLED(Invoke_AMBIENT_USERMODE);
        todo_wine CHECK_CALLED(Invoke_PROPERTYCHANGE);
        CHECK_CALLED(Invoke_BEFORENAVIGATE2);
        todo_wine CHECK_CALLED(Invoke_DOWNLOADBEGIN);
        if (use_container_olecmd) todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
        CHECK_CALLED(EnableModeless_FALSE);
        CHECK_CALLED(Invoke_STATUSTEXTCHANGE);
        CHECK_CALLED(SetStatusText);
        if(use_container_dochostui)
            CHECK_CALLED(GetHostInfo);
        CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
        CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
        CLEAR_CALLED(Invoke_AMBIENT_PALETTE); /* Not called by IE9 */
        if(use_container_dochostui) {
            CLEAR_CALLED(GetOptionKeyPath);
            CHECK_CALLED(GetOverridesKeyPath);
        }
        if (use_container_olecmd) todo_wine CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
        if (use_container_olecmd) todo_wine CHECK_CALLED(Exec_SETPROGRESSMAX);
        if (use_container_olecmd) todo_wine CHECK_CALLED(Exec_SETPROGRESSPOS);
        todo_wine CHECK_CALLED(Invoke_SETSECURELOCKICON);
        todo_wine CHECK_CALLED(Invoke_FILEDOWNLOAD);
        todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
        if (use_container_olecmd) todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
        CHECK_CALLED(EnableModeless_TRUE);
        if (is_file) todo_wine CHECK_CALLED(Invoke_PROGRESSCHANGE);
    }

    VariantClear(&url);

    test_ready_state(READYSTATE_LOADING);
}

static void test_QueryStatusWB(IWebBrowser2 *webbrowser, BOOL use_custom_target, BOOL has_document)
{
    HRESULT hres, success_state;
    OLECMDF success_flags;
    enum OLECMDF status;

    /*
     * We can tell the difference between the custom container target and the built-in target
     * since the custom target returns OLECMDF_SUPPORTED instead of OLECMDF_ENABLED.
     */
    if (use_custom_target)
        success_flags = OLECMDF_SUPPORTED;
    else
        success_flags = OLECMDF_ENABLED;

    /* When no target is available (no document or custom target) an error is returned */
    if (has_document)
        success_state = S_OK;
    else
        success_state = E_UNEXPECTED;

    /*
     * Test a safe operation that exists as both a high-numbered MSHTML id and an OLECMDID.
     * These tests show that QueryStatusWB uses a NULL pguidCmdGroup, since OLECMDID_STOP
     * is enabled and IDM_STOP is not.
     */
    status = 0xdeadbeef;
    if (use_custom_target) SET_EXPECT(QueryStatus_STOP);
    hres = IWebBrowser2_QueryStatusWB(webbrowser, OLECMDID_STOP, &status);
    ok(hres == success_state, "QueryStatusWB failed: %08x %08x\n", hres, success_state);
    if (!use_custom_target && has_document)
        todo_wine ok((has_document && status == success_flags) || (!has_document && status == 0xdeadbeef),
                     "OLECMDID_STOP not enabled/supported: %08x %08x\n", status, success_flags);
    else
        ok((has_document && status == success_flags) || (!has_document && status == 0xdeadbeef),
           "OLECMDID_STOP not enabled/supported: %08x %08x\n", status, success_flags);
    status = 0xdeadbeef;
    if (use_custom_target) SET_EXPECT(QueryStatus_IDM_STOP);
    hres = IWebBrowser2_QueryStatusWB(webbrowser, IDM_STOP, &status);
    ok(hres == success_state, "QueryStatusWB failed: %08x %08x\n", hres, success_state);
    ok((has_document && status == 0) || (!has_document && status == 0xdeadbeef),
       "IDM_STOP is enabled/supported: %08x %d\n", status, has_document);
}

static void test_ExecWB(IWebBrowser2 *webbrowser, BOOL use_custom_target, BOOL has_document)
{
    HRESULT hres, olecmdid_state, idm_state;

    /* When no target is available (no document or custom target) an error is returned */
    if (has_document)
    {
        olecmdid_state = S_OK;
        idm_state = OLECMDERR_E_NOTSUPPORTED;
    }
    else
    {
        olecmdid_state = E_UNEXPECTED;
        idm_state = E_UNEXPECTED;
    }

    /*
     * Test a safe operation that exists as both a high-numbered MSHTML id and an OLECMDID.
     * These tests show that QueryStatusWB uses a NULL pguidCmdGroup, since OLECMDID_STOP
     * succeeds (S_OK) and IDM_STOP does not (OLECMDERR_E_NOTSUPPORTED).
     */
    if(use_custom_target) {
        SET_EXPECT(Exec_STOP);
    }else if(has_document) {
        SET_EXPECT(Invoke_STATUSTEXTCHANGE);
        SET_EXPECT(SetStatusText);
    }
    hres = IWebBrowser2_ExecWB(webbrowser, OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    if(!use_custom_target && has_document) {
        todo_wine ok(hres == olecmdid_state, "ExecWB failed: %08x %08x\n", hres, olecmdid_state);
        CLEAR_CALLED(Invoke_STATUSTEXTCHANGE); /* Called by IE9 */
        CLEAR_CALLED(SetStatusText); /* Called by IE9 */
    }else {
        ok(hres == olecmdid_state, "ExecWB failed: %08x %08x\n", hres, olecmdid_state);
    }
    if (use_custom_target)
        SET_EXPECT(Exec_IDM_STOP);
    hres = IWebBrowser2_ExecWB(webbrowser, IDM_STOP, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    ok(hres == idm_state, "ExecWB failed: %08x %08x\n", hres, idm_state);
}

static void test_download(DWORD flags)
{
    BOOL *b = (flags & DWL_REFRESH) ? &called_Exec_SETDOWNLOADSTATE_0 : &called_Invoke_DOCUMENTCOMPLETE;
    MSG msg;

    is_downloading = TRUE;
    dwl_flags = flags;

    test_ready_state((flags & (DWL_FROM_PUT_HREF|DWL_FROM_GOBACK|DWL_FROM_GOFORWARD|DWL_REFRESH))
                     ? READYSTATE_COMPLETE : READYSTATE_LOADING);

    if(flags & (DWL_EXPECT_BEFORE_NAVIGATE|(is_http ? DWL_FROM_PUT_HREF : 0)))
        SET_EXPECT(Invoke_PROPERTYCHANGE);

    if(flags & DWL_EXPECT_BEFORE_NAVIGATE) {
        SET_EXPECT(Invoke_BEFORENAVIGATE2);
        SET_EXPECT(TranslateUrl);
    }
    SET_EXPECT(Exec_SETPROGRESSMAX);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
    SET_EXPECT(DocHost_EnableModeless_FALSE);
    SET_EXPECT(DocHost_EnableModeless_TRUE);
    SET_EXPECT(Invoke_SETSECURELOCKICON);
    SET_EXPECT(Invoke_282);
    SET_EXPECT(EnableModeless_FALSE);
    SET_EXPECT(Invoke_COMMANDSTATECHANGE);
    SET_EXPECT(Invoke_STATUSTEXTCHANGE);
    SET_EXPECT(SetStatusText);
    SET_EXPECT(EnableModeless_TRUE);
    if(!is_first_load)
        SET_EXPECT(GetHostInfo);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
    SET_EXPECT(Invoke_TITLECHANGE);
    if(!(flags & DWL_REFRESH))
        SET_EXPECT(Invoke_NAVIGATECOMPLETE2);
    if(is_first_load)
        SET_EXPECT(GetDropTarget);
    SET_EXPECT(Invoke_PROGRESSCHANGE);
    if(!(flags & DWL_REFRESH))
        SET_EXPECT(Invoke_DOCUMENTCOMPLETE);

    if(flags & DWL_HTTP)
        SET_EXPECT(Exec_SETTITLE);
    SET_EXPECT(UpdateUI);
    SET_EXPECT(Exec_UPDATECOMMANDS);
    SET_EXPECT(QueryStatus_STOP);

    while(!*b && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if(flags & (DWL_EXPECT_BEFORE_NAVIGATE|(is_http ? DWL_FROM_PUT_HREF : 0)))
        todo_wine CHECK_CALLED(Invoke_PROPERTYCHANGE);

    if(flags & DWL_EXPECT_BEFORE_NAVIGATE) {
        CHECK_CALLED(Invoke_BEFORENAVIGATE2);
        CHECK_CALLED(TranslateUrl);
    }
    todo_wine CHECK_CALLED(Exec_SETPROGRESSMAX);
    todo_wine CHECK_CALLED(Exec_SETPROGRESSPOS);
    CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
    CLEAR_CALLED(DocHost_EnableModeless_FALSE); /* IE 7 */
    CLEAR_CALLED(DocHost_EnableModeless_TRUE); /* IE 7 */
    todo_wine CHECK_CALLED(Invoke_SETSECURELOCKICON);
    CLEAR_CALLED(Invoke_282); /* IE 7 */
    if(is_first_load)
        todo_wine CHECK_CALLED(EnableModeless_FALSE);
    else
        CLEAR_CALLED(EnableModeless_FALSE); /* IE 8 */
    CLEAR_CALLED(Invoke_COMMANDSTATECHANGE);
    todo_wine CHECK_CALLED(Invoke_STATUSTEXTCHANGE);
    todo_wine CHECK_CALLED(SetStatusText);
    if(is_first_load)
        todo_wine CHECK_CALLED(EnableModeless_TRUE);
    else
        CLEAR_CALLED(EnableModeless_FALSE); /* IE 8 */
    if(!is_first_load)
        todo_wine CHECK_CALLED(GetHostInfo);
    CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
    todo_wine CHECK_CALLED(Invoke_TITLECHANGE);
    if(!(flags & DWL_REFRESH))
        CHECK_CALLED(Invoke_NAVIGATECOMPLETE2);
    if(is_first_load)
        todo_wine CHECK_CALLED(GetDropTarget);
    if(!(flags & DWL_REFRESH))
        CHECK_CALLED(Invoke_DOCUMENTCOMPLETE);

    is_downloading = FALSE;

    test_ready_state(READYSTATE_COMPLETE);

    while(!called_Exec_UPDATECOMMANDS && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    todo_wine CHECK_CALLED(Invoke_PROGRESSCHANGE);
    if(flags & DWL_HTTP)
        CLEAR_CALLED(Exec_SETTITLE); /* FIXME: make it more strict */
    CHECK_CALLED(UpdateUI);
    CHECK_CALLED(Exec_UPDATECOMMANDS);
    CLEAR_CALLED(QueryStatus_STOP);
}

static void test_Refresh(IWebBrowser2 *webbrowser)
{
    HRESULT hres;

    trace("Refresh...\n");

    SET_EXPECT(Exec_DocHostCommandHandler_2300);
    hres = IWebBrowser2_Refresh(webbrowser);
    ok(hres == S_OK, "Refresh failed: %08x\n", hres);
    CHECK_CALLED(Exec_DocHostCommandHandler_2300);

    test_download(DWL_REFRESH);
}

static void test_olecmd(IWebBrowser2 *unk, BOOL loaded)
{
    IOleCommandTarget *cmdtrg;
    OLECMD cmds[3];
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "Could not get IOleCommandTarget iface: %08x\n", hres);
    if(FAILED(hres))
        return;

    cmds[0].cmdID = OLECMDID_SPELL;
    cmds[0].cmdf = 0xdeadbeef;
    cmds[1].cmdID = OLECMDID_REFRESH;
    cmds[1].cmdf = 0xdeadbeef;
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 2, cmds, NULL);
    if(loaded) {
        ok(hres == S_OK, "QueryStatus failed: %08x\n", hres);
        ok(cmds[0].cmdf == OLECMDF_SUPPORTED, "OLECMDID_SPELL cmdf = %x\n", cmds[0].cmdf);
        ok(cmds[1].cmdf == (OLECMDF_ENABLED|OLECMDF_SUPPORTED),
           "OLECMDID_REFRESH cmdf = %x\n", cmds[1].cmdf);
    }else {
        ok(hres == 0x80040104, "QueryStatus failed: %08x\n", hres);
        ok(cmds[0].cmdf == 0xdeadbeef, "OLECMDID_SPELL cmdf = %x\n", cmds[0].cmdf);
        ok(cmds[1].cmdf == 0xdeadbeef, "OLECMDID_REFRESH cmdf = %x\n", cmds[0].cmdf);
    }

    IOleCommandTarget_Release(cmdtrg);
}

static void test_IServiceProvider(IWebBrowser2 *unk)
{
    IServiceProvider *servprov = (void*)0xdeadbeef;
    IUnknown *iface;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IServiceProvider, (void**)&servprov);
    ok(hres == S_OK, "QueryInterface returned %08x, expected S_OK\n", hres);
    if(FAILED(hres))
        return;

    hres = IServiceProvider_QueryService(servprov, &SID_STopLevelBrowser, &IID_IBrowserService2, (LPVOID*)&iface);
    ok(hres == E_FAIL, "QueryService returned %08x, expected E_FAIL\n", hres);
    ok(!iface, "QueryService returned %p, expected NULL\n", iface);
    if(hres == S_OK)
        IUnknown_Release(iface);

    hres = IServiceProvider_QueryService(servprov, &SID_SHTMLWindow, &IID_IHTMLWindow2, (LPVOID*)&iface);
    ok(hres == S_OK, "QueryService returned %08x, expected S_OK\n", hres);
    ok(iface != NULL, "QueryService returned NULL\n");
    if(hres == S_OK)
        IUnknown_Release(iface);

    IServiceProvider_Release(servprov);
}

static void test_put_href(IWebBrowser2 *unk, const char *url)
{
    IHTMLLocation *location;
    IHTMLDocument2 *doc;
    BSTR str;
    HRESULT hres;

    trace("put_href(%s)...\n", url);

    doc = get_document(unk);

    location = NULL;
    hres = IHTMLDocument2_get_location(doc, &location);
    IHTMLDocument2_Release(doc);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);
    ok(location != NULL, "location == NULL\n");

    is_http = !memcmp(url, "http:", 5);

    SET_EXPECT(TranslateUrl);
    SET_EXPECT(Invoke_BEFORENAVIGATE2);
    if(!is_http)
        SET_EXPECT(Invoke_PROPERTYCHANGE);

    dwl_flags = DWL_FROM_PUT_HREF;

    str = a2bstr(current_url = url);
    is_first_load = FALSE;
    hres = IHTMLLocation_put_href(location, str);

    CHECK_CALLED(TranslateUrl);
    CHECK_CALLED(Invoke_BEFORENAVIGATE2);
    if(!is_http)
        todo_wine CHECK_CALLED(Invoke_PROPERTYCHANGE);

    IHTMLLocation_Release(location);
    SysFreeString(str);
    ok(hres == S_OK, "put_href failed: %08x\n", hres);

    test_ready_state(READYSTATE_COMPLETE);
}

static void test_go_back(IWebBrowser2 *wb, const char *back_url)
{
    HRESULT hres;

    current_url = back_url;

    SET_EXPECT(Invoke_BEFORENAVIGATE2);
    SET_EXPECT(Invoke_COMMANDSTATECHANGE);
    hres = IWebBrowser2_GoBack(wb);
    ok(hres == S_OK, "GoBack failed: %08x\n", hres);
    CHECK_CALLED(Invoke_BEFORENAVIGATE2);
    todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
}

static void test_go_forward(IWebBrowser2 *wb, const char *forward_url)
{
    HRESULT hres;

    current_url = forward_url;
    dwl_flags |= DWL_FROM_GOFORWARD;

    SET_EXPECT(Invoke_BEFORENAVIGATE2);
    SET_EXPECT(Invoke_COMMANDSTATECHANGE);
    hres = IWebBrowser2_GoForward(wb);
    ok(hres == S_OK, "GoForward failed: %08x\n", hres);
    CHECK_CALLED(Invoke_BEFORENAVIGATE2);
    todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
}

static void test_QueryInterface(IWebBrowser2 *wb)
{
    IQuickActivate *qa = (IQuickActivate*)0xdeadbeef;
    IRunnableObject *runnable = (IRunnableObject*)0xdeadbeef;
    IPerPropertyBrowsing *propbrowse = (void*)0xdeadbeef;
    IOleInPlaceSite *inplace = (void*)0xdeadbeef;
    IOleCache *cache = (void*)0xdeadbeef;
    IObjectWithSite *site = (void*)0xdeadbeef;
    IViewObjectEx *viewex = (void*)0xdeadbeef;
    IOleLink *link = (void*)0xdeadbeef;
    IMarshal *marshal = (void*)0xdeadbeef;
    IUnknown *unk = (IUnknown*)wb;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IQuickActivate, (void**)&qa);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qa == NULL, "qa=%p, expected NULL\n", qa);

    hres = IUnknown_QueryInterface(unk, &IID_IRunnableObject, (void**)&runnable);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(runnable == NULL, "runnable=%p, expected NULL\n", runnable);

    hres = IUnknown_QueryInterface(unk, &IID_IPerPropertyBrowsing, (void**)&propbrowse);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(propbrowse == NULL, "propbrowse=%p, expected NULL\n", propbrowse);

    hres = IUnknown_QueryInterface(unk, &IID_IOleCache, (void**)&cache);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(cache == NULL, "cache=%p, expected NULL\n", cache);

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceSite, (void**)&inplace);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(inplace == NULL, "inplace=%p, expected NULL\n", inplace);

    hres = IUnknown_QueryInterface(unk, &IID_IObjectWithSite, (void**)&site);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(site == NULL, "site=%p, expected NULL\n", site);

    hres = IUnknown_QueryInterface(unk, &IID_IViewObjectEx, (void**)&viewex);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(viewex == NULL, "viewex=%p, expected NULL\n", viewex);

    hres = IUnknown_QueryInterface(unk, &IID_IOleLink, (void**)&link);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(link == NULL, "link=%p, expected NULL\n", link);

    hres = IUnknown_QueryInterface(unk, &IID_IMarshal, (void**)&marshal);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(marshal == NULL, "marshal=%p, expected NULL\n", marshal);

}

static void test_UIActivate(IWebBrowser2 *unk, BOOL activate)
{
    IOleDocumentView *docview;
    IHTMLDocument2 *doc;
    HRESULT hres;

    doc = get_document(unk);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleDocumentView, (void**)&docview);
    ok(hres == S_OK, "Got 0x%08x\n", hres);
    if(SUCCEEDED(hres)) {
        if(activate) {
            SET_EXPECT(RequestUIActivate);
            SET_EXPECT(ShowUI);
            SET_EXPECT(HideUI);
            SET_EXPECT(OnFocus);
        }

        hres = IOleDocumentView_UIActivate(docview, activate);
        if(activate)
            todo_wine ok(hres == S_OK, "Got 0x%08x\n", hres);
        else
            ok(hres == S_OK, "Got 0x%08x\n", hres);

        if(activate) {
            todo_wine {
                CHECK_CALLED(RequestUIActivate);
                CHECK_CALLED(ShowUI);
                CHECK_CALLED(HideUI);
                CHECK_CALLED(OnFocus);
            }
        }

        IOleDocumentView_Release(docview);
    }

    IHTMLDocument2_Release(doc);
}

static void test_external(IWebBrowser2 *unk)
{
    IDocHostUIHandler2 *dochost;
    IOleClientSite *client;
    IDispatch *disp;
    HRESULT hres;

    client = get_dochost(unk);

    hres = IOleClientSite_QueryInterface(client, &IID_IDocHostUIHandler2, (void**)&dochost);
    ok(hres == S_OK, "Could not get IDocHostUIHandler2 iface: %08x\n", hres);
    IOleClientSite_Release(client);

    if(use_container_dochostui)
        SET_EXPECT(GetExternal);
    disp = (void*)0xdeadbeef;
    hres = IDocHostUIHandler2_GetExternal(dochost, &disp);
    if(use_container_dochostui) {
        CHECK_CALLED(GetExternal);
        ok(hres == S_FALSE, "GetExternal failed: %08x\n", hres);
        ok(!disp, "disp = %p\n", disp);
    }else {
        IShellUIHelper *uihelper;

        ok(hres == S_OK, "GetExternal failed: %08x\n", hres);
        ok(disp != NULL, "disp == NULL\n");

        hres = IDispatch_QueryInterface(disp, &IID_IShellUIHelper, (void**)&uihelper);
        ok(hres == S_OK, "Could not get IShellUIHelper iface: %08x\n", hres);
        IShellUIHelper_Release(uihelper);
    }

    IDocHostUIHandler2_Release(dochost);
}

static void test_htmlwindow_close(IWebBrowser2 *wb)
{
    IHTMLWindow2 *window;
    IHTMLDocument2 *doc;
    HRESULT hres;

    doc = get_document(wb);

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);
    IHTMLDocument2_Release(doc);

    SET_EXPECT(Invoke_WINDOWCLOSING);

    hres = IHTMLWindow2_close(window);
    ok(hres == S_OK, "close failed: %08x\n", hres);

    CHECK_CALLED(Invoke_WINDOWCLOSING);

    IHTMLWindow2_Release(window);
}

static void test_TranslateAccelerator(IWebBrowser2 *unk)
{
    IOleClientSite *doc_clientsite;
    IOleInPlaceActiveObject *pao;
    HRESULT hres;
    DWORD keycode;
    MSG msg_a = {
        container_hwnd,
        0, 0, 0,
        GetTickCount(),
        {5, 5}
    };

    test_Navigate2(unk, "about:blank");

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceActiveObject, (void**)&pao);
    ok(hres == S_OK, "Got 0x%08x\n", hres);
    if(SUCCEEDED(hres)) {
        /* One accelerator that should be handled by mshtml */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F1;
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        ok(hres == S_FALSE, "Got 0x%08x (%04x::%02lx)\n", hres, msg_a.message, msg_a.wParam);

        /* And one that should not */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F5;
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        ok(hres == S_FALSE, "Got 0x%08x (%04x::%02lx)\n", hres, msg_a.message, msg_a.wParam);

        IOleInPlaceActiveObject_Release(pao);
    }

    test_UIActivate(unk, TRUE);

    /* Test again after UIActivate */
    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceActiveObject, (void**)&pao);
    ok(hres == S_OK, "Got 0x%08x\n", hres);
    if(SUCCEEDED(hres)) {
        /* One accelerator that should be handled by mshtml */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F1;
        SET_EXPECT(DocHost_TranslateAccelerator);
        SET_EXPECT(ControlSite_TranslateAccelerator);
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        ok(hres == S_FALSE, "Got 0x%08x (%04x::%02lx)\n", hres, msg_a.message, msg_a.wParam);
        todo_wine CHECK_CALLED(DocHost_TranslateAccelerator);
        todo_wine CHECK_CALLED(ControlSite_TranslateAccelerator);

        /* And one that should not */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F5;
        SET_EXPECT(DocHost_TranslateAccelerator);
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        todo_wine ok(hres == S_OK, "Got 0x%08x (%04x::%02lx)\n", hres, msg_a.message, msg_a.wParam);
        todo_wine CHECK_CALLED(DocHost_TranslateAccelerator);

        IOleInPlaceActiveObject_Release(pao);
    }

    doc_clientsite = get_dochost(unk);
    if(doc_clientsite) {
        IDocHostUIHandler2 *dochost;
        IOleControlSite *doc_controlsite;
        IUnknown *unk_test;

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IOleInPlaceFrame, (void**)&unk_test);
        ok(hres == E_NOINTERFACE, "Got 0x%08x\n", hres);
        if(SUCCEEDED(hres)) IUnknown_Release(unk_test);

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IDocHostShowUI, (void**)&unk_test);
        todo_wine ok(hres == S_OK, "Got 0x%08x\n", hres);
        if(SUCCEEDED(hres)) IUnknown_Release(unk_test);

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IDocHostUIHandler, (void**)&unk_test);
        ok(hres == S_OK, "Got 0x%08x\n", hres);
        if(SUCCEEDED(hres)) IUnknown_Release(unk_test);

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IDocHostUIHandler2, (void**)&dochost);
        ok(hres == S_OK, "Got 0x%08x\n", hres);
        if(SUCCEEDED(hres)) {
            msg_a.message = WM_KEYDOWN;
            hr_dochost_TranslateAccelerator = 0xdeadbeef;
            for(keycode = 0; keycode <= 0x100; keycode++) {
                msg_a.wParam = keycode;
                SET_EXPECT(DocHost_TranslateAccelerator);
                hres = IDocHostUIHandler2_TranslateAccelerator(dochost, &msg_a, &CGID_MSHTML, 1234);
                ok(hres == 0xdeadbeef, "Got 0x%08x\n", hres);
                CHECK_CALLED(DocHost_TranslateAccelerator);
            }
            hr_dochost_TranslateAccelerator = E_NOTIMPL;

            SET_EXPECT(DocHost_TranslateAccelerator);
            hres = IDocHostUIHandler2_TranslateAccelerator(dochost, &msg_a, &CGID_MSHTML, 1234);
            ok(hres == E_NOTIMPL, "Got 0x%08x\n", hres);
            CHECK_CALLED(DocHost_TranslateAccelerator);

            IDocHostUIHandler2_Release(dochost);
        }

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IOleControlSite, (void**)&doc_controlsite);
        ok(hres == S_OK, "Got 0x%08x\n", hres);
        if(SUCCEEDED(hres)) {
            msg_a.message = WM_KEYDOWN;
            hr_site_TranslateAccelerator = 0xdeadbeef;
            for(keycode = 0; keycode < 0x100; keycode++) {
                msg_a.wParam = keycode;
                SET_EXPECT(ControlSite_TranslateAccelerator);
                hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
                if(keycode == 0x9 || keycode == 0x75)
                    todo_wine ok(hres == S_OK, "Got 0x%08x (keycode: %04x)\n", hres, keycode);
                else
                    ok(hres == S_FALSE, "Got 0x%08x (keycode: %04x)\n", hres, keycode);

                CHECK_CALLED(ControlSite_TranslateAccelerator);
            }
            msg_a.wParam = VK_LEFT;
            SET_EXPECT(ControlSite_TranslateAccelerator);
            hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
            ok(hres == S_FALSE, "Got 0x%08x (keycode: %04x)\n", hres, keycode);
            CHECK_CALLED(ControlSite_TranslateAccelerator);

            hr_site_TranslateAccelerator = S_OK;
            SET_EXPECT(ControlSite_TranslateAccelerator);
            hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
            ok(hres == S_OK, "Got 0x%08x (keycode: %04x)\n", hres, keycode);
            CHECK_CALLED(ControlSite_TranslateAccelerator);

            hr_site_TranslateAccelerator = E_NOTIMPL;
            SET_EXPECT(ControlSite_TranslateAccelerator);
            hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
            ok(hres == S_FALSE, "Got 0x%08x (keycode: %04x)\n", hres, keycode);
            CHECK_CALLED(ControlSite_TranslateAccelerator);

            IOleControlSite_Release(doc_controlsite);
        }

        IOleClientSite_Release(doc_clientsite);
    }

    test_UIActivate(unk, FALSE);
}

static void test_dochost_qs(IWebBrowser2 *webbrowser)
{
    IOleClientSite *client_site;
    IServiceProvider *serv_prov;
    IUnknown *service;
    HRESULT hres;

    client_site = get_dochost(webbrowser);
    hres = IOleClientSite_QueryInterface(client_site, &IID_IServiceProvider, (void**)&serv_prov);
    IOleClientSite_Release(client_site);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08x\n", hres);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IHlinkFrame, &IID_IHlinkFrame, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08x\n", hres);
    ok(iface_cmp(service, (IUnknown*)webbrowser), "service != unk\n");
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IWebBrowserApp, &IID_IHlinkFrame, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08x\n", hres);
    ok(iface_cmp(service, (IUnknown*)webbrowser), "service != unk\n");
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IShellBrowser, &IID_IShellBrowser, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08x\n", hres);
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IShellBrowser, &IID_IBrowserService, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08x\n", hres);
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IShellBrowser, &IID_IDocObjectService, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08x\n", hres);
    IUnknown_Release(service);

    IServiceProvider_Release(serv_prov);
}

static void test_Close(IWebBrowser2 *wb, BOOL do_download)
{
    IOleObject *oo;
    IOleClientSite *ocs;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void**)&oo);
    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);
    if(hres != S_OK)
        return;

    test_close = TRUE;

    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(OnUIDeactivate);
    SET_EXPECT(OnFocus);
    SET_EXPECT(OnInPlaceDeactivate);
    SET_EXPECT(Invoke_STATUSTEXTCHANGE);
    if(!do_download) {
        SET_EXPECT(Invoke_COMMANDSTATECHANGE);
        SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
    }
    hres = IOleObject_Close(oo, OLECLOSE_NOSAVE);
    ok(hres == S_OK, "OleObject_Close failed: %x\n", hres);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(OnUIDeactivate);
    todo_wine CHECK_CALLED(OnFocus);
    CHECK_CALLED(OnInPlaceDeactivate);
    CLEAR_CALLED(Invoke_STATUSTEXTCHANGE); /* Called by IE9 */
    if(!do_download) {
        todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
        todo_wine CHECK_CALLED(Invoke_DOWNLOADCOMPLETE);
    }

    hres = IOleObject_GetClientSite(oo, &ocs);
    ok(hres == S_OK, "hres = %x\n", hres);
    ok(!ocs, "ocs != NULL\n");

    hres = IOleObject_Close(oo, OLECLOSE_NOSAVE);
    ok(hres == S_OK, "OleObject_Close failed: %x\n", hres);

    test_close = FALSE;
    IOleObject_Release(oo);
}

#define TEST_DOWNLOAD    0x0001
#define TEST_NOOLECMD    0x0002
#define TEST_NODOCHOST   0x0004

static void init_test(IWebBrowser2 *webbrowser, DWORD flags)
{
    wb = webbrowser;

    is_downloading = (flags & TEST_DOWNLOAD) != 0;
    is_first_load = TRUE;
    dwl_flags = 0;
    use_container_olecmd = !(flags & TEST_NOOLECMD);
    use_container_dochostui = !(flags & TEST_NODOCHOST);
}

static void test_WebBrowser(BOOL do_download, BOOL do_close)
{
    IWebBrowser2 *webbrowser;
    ULONG ref;

    webbrowser = create_webbrowser();
    if(!webbrowser)
        return;

    init_test(webbrowser, do_download ? TEST_DOWNLOAD : 0);

    test_QueryStatusWB(webbrowser, FALSE, FALSE);
    test_ExecWB(webbrowser, FALSE, FALSE);
    test_QueryInterface(webbrowser);
    test_ready_state(READYSTATE_UNINITIALIZED);
    test_ClassInfo(webbrowser);
    test_EnumVerbs(webbrowser);
    test_LocationURL(webbrowser, "");
    test_ConnectionPoint(webbrowser, TRUE);
    test_ClientSite(webbrowser, &ClientSite, !do_download);
    test_Extent(webbrowser);
    test_wb_funcs(webbrowser, TRUE);
    test_DoVerb(webbrowser);
    test_olecmd(webbrowser, FALSE);
    test_Navigate2(webbrowser, "about:blank");
    test_QueryStatusWB(webbrowser, TRUE, TRUE);
    test_ExecWB(webbrowser, TRUE, TRUE);

    if(do_download) {
        IHTMLDocument2 *doc, *doc2;

        test_download(0);
        test_olecmd(webbrowser, TRUE);
        doc = get_document(webbrowser);

        test_put_href(webbrowser, "about:test");
        test_download(DWL_FROM_PUT_HREF);
        doc2 = get_document(webbrowser);
        ok(doc == doc2, "doc != doc2\n");
        IHTMLDocument2_Release(doc2);

        trace("Navigate2 repeated...\n");
        test_Navigate2(webbrowser, "about:blank");
        test_download(DWL_EXPECT_BEFORE_NAVIGATE);
        doc2 = get_document(webbrowser);
        ok(doc == doc2, "doc != doc2\n");
        IHTMLDocument2_Release(doc2);
        IHTMLDocument2_Release(doc);

        if(!do_close) {
            trace("Navigate2 http URL...\n");
            test_ready_state(READYSTATE_COMPLETE);
            test_Navigate2(webbrowser, "http://test.winehq.org/tests/hello.html");
            test_download(DWL_EXPECT_BEFORE_NAVIGATE|DWL_HTTP);

            test_Refresh(webbrowser);

            trace("put_href http URL...\n");
            test_put_href(webbrowser, "http://test.winehq.org/tests/winehq_snapshot/");
            test_download(DWL_FROM_PUT_HREF|DWL_HTTP);

            trace("GoBack...\n");
            test_go_back(webbrowser, "http://test.winehq.org/tests/hello.html");
            test_download(DWL_FROM_GOBACK|DWL_HTTP);

            trace("GoForward...\n");
            test_go_forward(webbrowser, "http://test.winehq.org/tests/winehq_snapshot/");
            test_download(DWL_FROM_GOFORWARD|DWL_HTTP);
        }else {
            trace("Navigate2 repeated with the same URL...\n");
            test_Navigate2(webbrowser, "about:blank");
            test_download(DWL_EXPECT_BEFORE_NAVIGATE);
        }

        test_EnumVerbs(webbrowser);
        test_TranslateAccelerator(webbrowser);

        test_dochost_qs(webbrowser);
    }

    test_external(webbrowser);
    test_htmlwindow_close(webbrowser);

    if(do_close)
        test_Close(webbrowser, do_download);
    else
        test_ClientSite(webbrowser, NULL, !do_download);
    test_ie_funcs(webbrowser);
    test_GetControlInfo(webbrowser);
    test_wb_funcs(webbrowser, FALSE);
    test_ConnectionPoint(webbrowser, FALSE);
    test_IServiceProvider(webbrowser);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0 || broken(do_download && !do_close), "ref=%d, expected 0\n", ref);
}

static void test_WebBrowserV1(void)
{
    IWebBrowser2 *wb;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WebBrowser_V1, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get WebBrowserV1 instance: %08x\n", hres);

    init_test(wb, 0);
    wb_version = 1;

    test_QueryStatusWB(wb, FALSE, FALSE);
    test_ExecWB(wb, FALSE, FALSE);
    test_QueryInterface(wb);
    test_ready_state(READYSTATE_UNINITIALIZED);
    test_ClassInfo(wb);
    test_EnumVerbs(wb);

    ref = IWebBrowser2_Release(wb);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void test_WebBrowser_slim_container(void)
{
    IWebBrowser2 *webbrowser;
    ULONG ref;

    webbrowser = create_webbrowser();
    init_test(webbrowser, TEST_NOOLECMD|TEST_NODOCHOST);

    test_ConnectionPoint(webbrowser, TRUE);
    test_ClientSite(webbrowser, &ClientSite, TRUE);
    test_DoVerb(webbrowser);
    test_Navigate2(webbrowser, "about:blank");

    /* Tests of interest */
    test_QueryStatusWB(webbrowser, FALSE, TRUE);
    test_ExecWB(webbrowser, FALSE, TRUE);
    test_external(webbrowser);

    /* Cleanup stage */
    test_ClientSite(webbrowser, NULL, TRUE);
    test_ConnectionPoint(webbrowser, FALSE);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void test_WebBrowser_DoVerb(void)
{
    IWebBrowser2 *webbrowser;
    RECT rect;
    HWND hwnd;
    ULONG ref;
    BOOL res;

    webbrowser = create_webbrowser();
    init_test(webbrowser, 0);

    test_ClientSite(webbrowser, &ClientSite, FALSE);

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    call_DoVerb(webbrowser, OLEIVERB_INPLACEACTIVATE);
    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);

    hwnd = get_hwnd(webbrowser);

    memset(&rect, 0xa, sizeof(rect));
    res = GetWindowRect(hwnd, &rect);
    ok(res, "GetWindowRect failed: %u\n", GetLastError());

    SET_EXPECT(OnInPlaceDeactivate);
    call_DoVerb(webbrowser, OLEIVERB_HIDE);
    CHECK_CALLED(OnInPlaceDeactivate);

    test_ClientSite(webbrowser, NULL, FALSE);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}


/* Check if Internet Explorer is configured to run in "Enhanced Security Configuration" (aka hardened mode) */
/* Note: this code is duplicated in dlls/mshtml/tests/mshtml_test.h and dlls/urlmon/tests/sec_mgr.c */
static BOOL is_ie_hardened(void)
{
    HKEY zone_map;
    DWORD ie_harden, type, size;

    ie_harden = 0;
    if(RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap",
                    0, KEY_QUERY_VALUE, &zone_map) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueExA(zone_map, "IEHarden", NULL, &type, (LPBYTE) &ie_harden, &size) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            ie_harden = 0;
        }
        RegCloseKey(zone_map);
    }

    return ie_harden != 0;
}

static void test_FileProtocol(void)
{
    IWebBrowser2 *webbrowser;
    HANDLE file;
    ULONG ref;
    char file_path[MAX_PATH];
    char file_url[MAX_PATH] = "File://";

    static const char test_file[] = "wine_test.html";

    GetTempPathA(MAX_PATH, file_path);
    strcat(file_path, test_file);

    webbrowser = create_webbrowser();
    if(!webbrowser)
        return;

    init_test(webbrowser, 0);

    file = CreateFileA(file_path, GENERIC_WRITE, 0, NULL,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS){
        ok(0, "CreateFile failed\n");
        return;
    }
    CloseHandle(file);

    GetLongPathNameA(file_path, file_path, sizeof(file_path));
    strcat(file_url, file_path);

    test_ConnectionPoint(webbrowser, TRUE);
    test_ClientSite(webbrowser, &ClientSite, TRUE);
    test_DoVerb(webbrowser);
    test_Navigate2(webbrowser, file_url);
    test_ClientSite(webbrowser, NULL, TRUE);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0, "ref=%u, expected 0\n", ref);

    if(file != INVALID_HANDLE_VALUE)
        DeleteFileA(file_path);
}

START_TEST(webbrowser)
{
    OleInitialize(NULL);

    container_hwnd = create_container_window();

    trace("Testing WebBrowser (no download)...\n");
    test_WebBrowser(FALSE, FALSE);
    test_WebBrowser(FALSE, TRUE);

    if(!is_ie_hardened()) {
        trace("Testing WebBrowser...\n");
        test_WebBrowser(TRUE, FALSE);
    }else {
        win_skip("Skipping http tests in hardened mode\n");
    }

    trace("Testing WebBrowser DoVerb\n");
    test_WebBrowser_DoVerb();
    trace("Testing WebBrowser (with close)...\n");
    test_WebBrowser(TRUE, TRUE);
    trace("Testing WebBrowser w/o container-based olecmd...\n");
    test_WebBrowser_slim_container();
    trace("Testing WebBrowserV1...\n");
    test_WebBrowserV1();
    test_FileProtocol();

    OleUninitialize();
}
