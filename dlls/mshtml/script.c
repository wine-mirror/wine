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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "activscp.h"
#include "activdbg.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptSiteDebugVtbl IActiveScriptSiteDebug64Vtbl

#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure_Release IActiveScriptParseProcedure64_Release
#define IActiveScriptParseProcedure_ParseProcedureText IActiveScriptParseProcedure64_ParseProcedureText

#else

#define CTXARG_T DWORD
#define IActiveScriptSiteDebugVtbl IActiveScriptSiteDebug32Vtbl

#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure_Release IActiveScriptParseProcedure32_Release
#define IActiveScriptParseProcedure_ParseProcedureText IActiveScriptParseProcedure32_ParseProcedureText

#endif

static const WCHAR windowW[] = {'w','i','n','d','o','w',0};
static const WCHAR emptyW[] = {0};

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};
static const CLSID CLSID_VBScript =
    {0xb54f3741,0x5b07,0x11cf,{0xa4,0xb0,0x00,0xaa,0x00,0x4a,0x55,0xe8}};

struct ScriptHost {
    IActiveScriptSite              IActiveScriptSite_iface;
    IActiveScriptSiteInterruptPoll IActiveScriptSiteInterruptPoll_iface;
    IActiveScriptSiteWindow        IActiveScriptSiteWindow_iface;
    IActiveScriptSiteDebug         IActiveScriptSiteDebug_iface;
    IServiceProvider               IServiceProvider_iface;

    LONG ref;

    IActiveScript *script;
    IActiveScriptParse *parse;
    IActiveScriptParseProcedure *parse_proc;

    SCRIPTSTATE script_state;

    HTMLInnerWindow *window;

    GUID guid;
    struct list entry;
};

static void set_script_prop(ScriptHost *script_host, DWORD property, VARIANT *val)
{
    IActiveScriptProperty *script_prop;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IActiveScriptProperty,
            (void**)&script_prop);
    if(FAILED(hres)) {
        WARN("Could not get IActiveScriptProperty iface: %08x\n", hres);
        return;
    }

    hres = IActiveScriptProperty_SetProperty(script_prop, property, NULL, val);
    IActiveScriptProperty_Release(script_prop);
    if(FAILED(hres))
        WARN("SetProperty(%x) failed: %08x\n", property, hres);
}

static BOOL init_script_engine(ScriptHost *script_host)
{
    IObjectSafety *safety;
    SCRIPTSTATE state;
    DWORD supported_opts=0, enabled_opts=0;
    VARIANT var;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IActiveScriptParse, (void**)&script_host->parse);
    if(FAILED(hres)) {
        WARN("Could not get IActiveScriptHost: %08x\n", hres);
        return FALSE;
    }

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IObjectSafety, (void**)&safety);
    if(FAILED(hres)) {
        FIXME("Could not get IObjectSafety: %08x\n", hres);
        return FALSE;
    }

    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported_opts, &enabled_opts);
    if(FAILED(hres)) {
        FIXME("GetInterfaceSafetyOptions failed: %08x\n", hres);
    }else if(!(supported_opts & INTERFACE_USES_DISPEX)) {
        FIXME("INTERFACE_USES_DISPEX is not supported\n");
    }else {
        hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
                INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER,
                INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
        if(FAILED(hres))
            FIXME("SetInterfaceSafetyOptions failed: %08x\n", hres);
    }

    IObjectSafety_Release(safety);
    if(FAILED(hres))
        return FALSE;

    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    set_script_prop(script_host, SCRIPTPROP_INVOKEVERSIONING, &var);

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VARIANT_TRUE;
    set_script_prop(script_host, SCRIPTPROP_HACK_TRIDENTEVENTSINK, &var);

    hres = IActiveScriptParse_InitNew(script_host->parse);
    if(FAILED(hres)) {
        WARN("InitNew failed: %08x\n", hres);
        return FALSE;
    }

    hres = IActiveScript_SetScriptSite(script_host->script, &script_host->IActiveScriptSite_iface);
    if(FAILED(hres)) {
        WARN("SetScriptSite failed: %08x\n", hres);
        IActiveScript_Close(script_host->script);
        return FALSE;
    }

    hres = IActiveScript_GetScriptState(script_host->script, &state);
    if(FAILED(hres))
        WARN("GetScriptState failed: %08x\n", hres);
    else if(state != SCRIPTSTATE_INITIALIZED)
        FIXME("state = %x\n", state);

    hres = IActiveScript_SetScriptState(script_host->script, SCRIPTSTATE_STARTED);
    if(FAILED(hres)) {
        WARN("Starting script failed: %08x\n", hres);
        return FALSE;
    }

    hres = IActiveScript_AddNamedItem(script_host->script, windowW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    if(SUCCEEDED(hres)) {
        V_VT(&var) = VT_BOOL;
        V_BOOL(&var) = VARIANT_TRUE;
        set_script_prop(script_host, SCRIPTPROP_ABBREVIATE_GLOBALNAME_RESOLUTION, &var);
    }else {
       WARN("AddNamedItem failed: %08x\n", hres);
    }

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IActiveScriptParseProcedure2,
                                        (void**)&script_host->parse_proc);
    if(FAILED(hres)) {
        /* FIXME: QI for IActiveScriptParseProcedure */
        WARN("Could not get IActiveScriptParseProcedure iface: %08x\n", hres);
    }

    return TRUE;
}

static void release_script_engine(ScriptHost *This)
{
    if(!This->script)
        return;

    switch(This->script_state) {
    case SCRIPTSTATE_CONNECTED:
        IActiveScript_SetScriptState(This->script, SCRIPTSTATE_DISCONNECTED);

    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_DISCONNECTED:
    case SCRIPTSTATE_INITIALIZED:
        IActiveScript_Close(This->script);

    default:
        if(This->parse_proc) {
            IActiveScriptParseProcedure_Release(This->parse_proc);
            This->parse_proc = NULL;
        }

        if(This->parse) {
            IActiveScriptParse_Release(This->parse);
            This->parse = NULL;
        }
    }

    IActiveScript_Release(This->script);
    This->script = NULL;
    This->script_state = SCRIPTSTATE_UNINITIALIZED;
}

void connect_scripts(HTMLInnerWindow *window)
{
    ScriptHost *iter;

    LIST_FOR_EACH_ENTRY(iter, &window->script_hosts, ScriptHost, entry) {
        if(iter->script_state == SCRIPTSTATE_STARTED)
            IActiveScript_SetScriptState(iter->script, SCRIPTSTATE_CONNECTED);
    }
}

static inline ScriptHost *impl_from_IActiveScriptSite(IActiveScriptSite *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSite %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteInterruptPoll, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteInterruprtPoll %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteInterruptPoll_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteWindow, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteWindow %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteWindow_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteDebug, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteDebug %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteDebug_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_ICanHandleException, riid)) {
        TRACE("(%p)->(IID_ICanHandleException not supported %p)\n", This, ppv);
        return E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_script_engine(This);
        if(This->window)
            list_remove(&This->entry);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%p)\n", This, plcid);

    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%s %x %p %p)\n", This, debugstr_w(pstrName), dwReturnMask, ppiunkItem, ppti);

    if(dwReturnMask != SCRIPTINFO_IUNKNOWN) {
        FIXME("Unsupported mask %x\n", dwReturnMask);
        return E_NOTIMPL;
    }

    *ppiunkItem = NULL;

    if(strcmpW(pstrName, windowW))
        return DISP_E_MEMBERNOTFOUND;

    if(!This->window)
        return E_FAIL;

    /* FIXME: Return proxy object */
    *ppiunkItem = (IUnknown*)&This->window->base.IHTMLWindow2_iface;
    IUnknown_AddRef(*ppiunkItem);

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p)->(%p)\n", This, pbstrVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p)->(%p %p)\n", This, pvarResult, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%x)\n", This, ssScriptState);

    This->script_state = ssScriptState;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p)->(%p)\n", This, pscripterror);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->()\n", This);

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->()\n", This);

    return S_OK;
}

static const IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static inline ScriptHost *impl_from_IActiveScriptSiteInterruptPoll(IActiveScriptSiteInterruptPoll *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteInterruptPoll_iface);
}

static HRESULT WINAPI ActiveScriptSiteInterruptPoll_QueryInterface(IActiveScriptSiteInterruptPoll *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteInterruptPoll_AddRef(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteInterruptPoll_Release(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteInterruptPoll_QueryContinue(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static const IActiveScriptSiteInterruptPollVtbl ActiveScriptSiteInterruptPollVtbl = {
    ActiveScriptSiteInterruptPoll_QueryInterface,
    ActiveScriptSiteInterruptPoll_AddRef,
    ActiveScriptSiteInterruptPoll_Release,
    ActiveScriptSiteInterruptPoll_QueryContinue
};

static inline ScriptHost *impl_from_IActiveScriptSiteWindow(IActiveScriptSiteWindow *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteWindow_iface);
}

static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteWindow_AddRef(IActiveScriptSiteWindow *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteWindow_Release(IActiveScriptSiteWindow *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *phwnd)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL fEnable)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

static inline ScriptHost *impl_from_IActiveScriptSiteDebug(IActiveScriptSiteDebug *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteDebug_iface);
}

static HRESULT WINAPI ActiveScriptSiteDebug_QueryInterface(IActiveScriptSiteDebug *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteDebug_AddRef(IActiveScriptSiteDebug *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteDebug_Release(IActiveScriptSiteDebug *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteDebug_GetDocumentContextFromPosition(IActiveScriptSiteDebug *iface,
            CTXARG_T dwSourceContext, ULONG uCharacterOffset, ULONG uNumChars, IDebugDocumentContext **ppsc)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%s %u %u %p)\n", This, wine_dbgstr_longlong(dwSourceContext), uCharacterOffset,
          uNumChars, ppsc);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug_GetApplication(IActiveScriptSiteDebug *iface, IDebugApplication **ppda)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%p)\n", This, ppda);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug_GetRootApplicationNode(IActiveScriptSiteDebug *iface,
            IDebugApplicationNode **ppdanRoot)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%p)\n", This, ppdanRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug_OnScriptErrorDebug(IActiveScriptSiteDebug *iface,
            IActiveScriptErrorDebug *pErrorDebug, BOOL *pfEnterDebugger, BOOL *pfCallOnScriptErrorWhenContinuing)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pErrorDebug, pfEnterDebugger, pfCallOnScriptErrorWhenContinuing);
    return E_NOTIMPL;
}

static const IActiveScriptSiteDebugVtbl ActiveScriptSiteDebugVtbl = {
    ActiveScriptSiteDebug_QueryInterface,
    ActiveScriptSiteDebug_AddRef,
    ActiveScriptSiteDebug_Release,
    ActiveScriptSiteDebug_GetDocumentContextFromPosition,
    ActiveScriptSiteDebug_GetApplication,
    ActiveScriptSiteDebug_GetRootApplicationNode,
    ActiveScriptSiteDebug_OnScriptErrorDebug
};

static inline ScriptHost *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IServiceProvider_iface);
}

static HRESULT WINAPI ASServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ASServiceProvider_AddRef(IServiceProvider *iface)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ASServiceProvider_Release(IServiceProvider *iface)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ASServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&SID_SInternetHostSecurityManager, guidService)) {
        TRACE("(%p)->(SID_SInternetHostSecurityManager)\n", This);

        if(!This->window || !This->window->doc)
            return E_NOINTERFACE;

        return IInternetHostSecurityManager_QueryInterface(&This->window->doc->IInternetHostSecurityManager_iface,
                riid, ppv);
    }

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ASServiceProviderVtbl = {
    ASServiceProvider_QueryInterface,
    ASServiceProvider_AddRef,
    ASServiceProvider_Release,
    ASServiceProvider_QueryService
};

static ScriptHost *create_script_host(HTMLInnerWindow *window, const GUID *guid)
{
    ScriptHost *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(*ret));
    ret->IActiveScriptSite_iface.lpVtbl = &ActiveScriptSiteVtbl;
    ret->IActiveScriptSiteInterruptPoll_iface.lpVtbl = &ActiveScriptSiteInterruptPollVtbl;
    ret->IActiveScriptSiteWindow_iface.lpVtbl = &ActiveScriptSiteWindowVtbl;
    ret->IActiveScriptSiteDebug_iface.lpVtbl = &ActiveScriptSiteDebugVtbl;
    ret->IServiceProvider_iface.lpVtbl = &ASServiceProviderVtbl;
    ret->ref = 1;
    ret->window = window;
    ret->script_state = SCRIPTSTATE_UNINITIALIZED;

    ret->guid = *guid;
    list_add_tail(&window->script_hosts, &ret->entry);

    hres = CoCreateInstance(&ret->guid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&ret->script);
    if(FAILED(hres))
        WARN("Could not load script engine: %08x\n", hres);
    else if(!init_script_engine(ret))
        release_script_engine(ret);

    return ret;
}

static void parse_text(ScriptHost *script_host, LPCWSTR text)
{
    EXCEPINFO excepinfo;
    VARIANT var;
    HRESULT hres;

    static const WCHAR script_endW[] = {'<','/','S','C','R','I','P','T','>',0};

    TRACE("%s\n", debugstr_w(text));

    VariantInit(&var);
    memset(&excepinfo, 0, sizeof(excepinfo));
    TRACE(">>>\n");
    hres = IActiveScriptParse_ParseScriptText(script_host->parse, text, windowW, NULL, script_endW,
                                              0, 0, SCRIPTTEXT_ISVISIBLE|SCRIPTTEXT_HOSTMANAGESSOURCE,
                                              &var, &excepinfo);
    if(SUCCEEDED(hres))
        TRACE("<<<\n");
    else
        WARN("<<< %08x\n", hres);

}

static void parse_extern_script(ScriptHost *script_host, LPCWSTR src)
{
    IMoniker *mon;
    WCHAR *text;
    HRESULT hres;

    static const WCHAR wine_schemaW[] = {'w','i','n','e',':'};

    if(strlenW(src) > sizeof(wine_schemaW)/sizeof(WCHAR) && !memcmp(src, wine_schemaW, sizeof(wine_schemaW)))
        src += sizeof(wine_schemaW)/sizeof(WCHAR);

    hres = CreateURLMoniker(NULL, src, &mon);
    if(FAILED(hres))
        return;

    hres = bind_mon_to_wstr(script_host->window, mon, &text);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return;

    parse_text(script_host, text);

    heap_free(text);
}

static void parse_inline_script(ScriptHost *script_host, nsIDOMHTMLScriptElement *nsscript)
{
    const PRUnichar *text;
    nsAString text_str;
    nsresult nsres;

    nsAString_Init(&text_str, NULL);

    nsres = nsIDOMHTMLScriptElement_GetText(nsscript, &text_str);

    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&text_str, &text);
        parse_text(script_host, text);
    }else {
        ERR("GetText failed: %08x\n", nsres);
    }

    nsAString_Finish(&text_str);
}

static void parse_script_elem(ScriptHost *script_host, nsIDOMHTMLScriptElement *nsscript)
{
    const PRUnichar *src;
    nsAString src_str;
    nsresult nsres;

    nsAString_Init(&src_str, NULL);

    nsres = nsIDOMHTMLScriptElement_GetSrc(nsscript, &src_str);
    nsAString_GetData(&src_str, &src);

    if(NS_FAILED(nsres))
        ERR("GetSrc failed: %08x\n", nsres);
    else if(*src)
        parse_extern_script(script_host, src);
    else
        parse_inline_script(script_host, nsscript);

    nsAString_Finish(&src_str);
}

static BOOL get_guid_from_type(LPCWSTR type, GUID *guid)
{
    const WCHAR text_javascriptW[] =
        {'t','e','x','t','/','j','a','v','a','s','c','r','i','p','t',0};
    const WCHAR text_vbscriptW[] =
        {'t','e','x','t','/','v','b','s','c','r','i','p','t',0};

    /* FIXME: Handle more types */
    if(!strcmpiW(type, text_javascriptW)) {
        *guid = CLSID_JScript;
    }else if(!strcmpiW(type, text_vbscriptW)) {
        *guid = CLSID_VBScript;
    }else {
        FIXME("Unknown type %s\n", debugstr_w(type));
        return FALSE;
    }

    return TRUE;
}

static BOOL get_guid_from_language(LPCWSTR type, GUID *guid)
{
    HRESULT hres;

    hres = CLSIDFromProgID(type, guid);
    if(FAILED(hres))
        return FALSE;

    /* FIXME: Check CATID_ActiveScriptParse */

    return TRUE;
}

static BOOL get_script_guid(nsIDOMHTMLScriptElement *nsscript, GUID *guid)
{
    nsAString attr_str, val_str;
    BOOL ret = FALSE;
    nsresult nsres;

    static const PRUnichar languageW[] = {'l','a','n','g','u','a','g','e',0};

    nsAString_Init(&val_str, NULL);

    nsres = nsIDOMHTMLScriptElement_GetType(nsscript, &val_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *type;

        nsAString_GetData(&val_str, &type);
        if(*type) {
            ret = get_guid_from_type(type, guid);
            nsAString_Finish(&val_str);
            return ret;
        }
    }else {
        ERR("GetType failed: %08x\n", nsres);
    }

    nsAString_InitDepend(&attr_str, languageW);
    nsres = nsIDOMHTMLScriptElement_GetAttribute(nsscript, &attr_str, &val_str);
    nsAString_Finish(&attr_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *language;

        nsAString_GetData(&val_str, &language);

        if(*language) {
            ret = get_guid_from_language(language, guid);
        }else {
            *guid = CLSID_JScript;
            ret = TRUE;
        }
    }else {
        ERR("GetAttribute(language) failed: %08x\n", nsres);
    }

    nsAString_Finish(&val_str);

    return ret;
}

static ScriptHost *get_script_host(HTMLInnerWindow *window, const GUID *guid)
{
    ScriptHost *iter;

    LIST_FOR_EACH_ENTRY(iter, &window->script_hosts, ScriptHost, entry) {
        if(IsEqualGUID(guid, &iter->guid))
            return iter;
    }

    return create_script_host(window, guid);
}

void doc_insert_script(HTMLInnerWindow *window, nsIDOMHTMLScriptElement *nsscript)
{
    ScriptHost *script_host;
    GUID guid;

    if(!get_script_guid(nsscript, &guid)) {
        WARN("Could not find script GUID\n");
        return;
    }

    if(IsEqualGUID(&CLSID_JScript, &guid)
       && (!window->base.outer_window || window->base.outer_window->scriptmode != SCRIPTMODE_ACTIVESCRIPT)) {
        TRACE("Ignoring JScript\n");
        return;
    }

    script_host = get_script_host(window, &guid);
    if(!script_host)
        return;

    if(script_host->parse)
        parse_script_elem(script_host, nsscript);
}

IDispatch *script_parse_event(HTMLInnerWindow *window, LPCWSTR text)
{
    ScriptHost *script_host;
    GUID guid = CLSID_JScript;
    const WCHAR *ptr;
    IDispatch *disp;
    HRESULT hres;

    static const WCHAR delimiterW[] = {'\"',0};

    for(ptr = text; isalnumW(*ptr); ptr++);
    if(*ptr == ':') {
        LPWSTR language;
        BOOL b;

        language = heap_alloc((ptr-text+1)*sizeof(WCHAR));
        memcpy(language, text, (ptr-text)*sizeof(WCHAR));
        language[ptr-text] = 0;

        b = get_guid_from_language(language, &guid);

        heap_free(language);

        if(!b) {
            WARN("Could not find language\n");
            return NULL;
        }

        ptr++;
    }else {
        ptr = text;
    }

    if(IsEqualGUID(&CLSID_JScript, &guid)
       && (!window->base.outer_window || window->base.outer_window->scriptmode != SCRIPTMODE_ACTIVESCRIPT)) {
        TRACE("Ignoring JScript\n");
        return NULL;
    }

    script_host = get_script_host(window, &guid);
    if(!script_host || !script_host->parse_proc)
        return NULL;

    hres = IActiveScriptParseProcedure_ParseProcedureText(script_host->parse_proc, ptr, NULL, emptyW,
            NULL, NULL, delimiterW, 0 /* FIXME */, 0,
            SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
    if(FAILED(hres)) {
        WARN("ParseProcedureText failed: %08x\n", hres);
        return NULL;
    }

    TRACE("ret %p\n", disp);
    return disp;
}

HRESULT exec_script(HTMLInnerWindow *window, const WCHAR *code, const WCHAR *lang, VARIANT *ret)
{
    ScriptHost *script_host;
    EXCEPINFO ei;
    GUID guid;
    HRESULT hres;

    static const WCHAR delimW[] = {'"',0};

    if(!get_guid_from_language(lang, &guid)) {
        WARN("Could not find script GUID\n");
        return CO_E_CLASSSTRING;
    }

    script_host = get_script_host(window, &guid);
    if(!script_host) {
        FIXME("No script host\n");
        return E_FAIL;
    }

    if(!script_host->parse) {
        FIXME("script_host->parse == NULL\n");
        return E_FAIL;
    }

    memset(&ei, 0, sizeof(ei));
    TRACE(">>>\n");
    hres = IActiveScriptParse_ParseScriptText(script_host->parse, code, NULL, NULL, delimW, 0, 0, SCRIPTTEXT_ISVISIBLE, ret, &ei);
    if(SUCCEEDED(hres))
        TRACE("<<<\n");
    else
        WARN("<<< %08x\n", hres);

    return hres;
}

IDispatch *get_script_disp(ScriptHost *script_host)
{
    IDispatch *disp;
    HRESULT hres;

    if(!script_host->script)
        return NULL;

    hres = IActiveScript_GetScriptDispatch(script_host->script, windowW, &disp);
    if(FAILED(hres))
        return NULL;

    return disp;
}

BOOL find_global_prop(HTMLInnerWindow *window, BSTR name, DWORD flags, ScriptHost **ret_host, DISPID *ret_id)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    ScriptHost *iter;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(iter, &window->script_hosts, ScriptHost, entry) {
        disp = get_script_disp(iter);
        if(!disp)
            continue;

        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_GetDispID(dispex, name, flags & (~fdexNameEnsure), ret_id);
            IDispatchEx_Release(dispex);
        }else {
            FIXME("No IDispatchEx\n");
            hres = E_NOTIMPL;
        }

        IDispatch_Release(disp);
        if(SUCCEEDED(hres)) {
            *ret_host = iter;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL is_jscript_available(void)
{
    static BOOL available, checked;

    if(!checked) {
        IUnknown *unk;
        HRESULT hres = CoGetClassObject(&CLSID_JScript, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);

        if(SUCCEEDED(hres)) {
            available = TRUE;
            IUnknown_Release(unk);
        }else {
            available = FALSE;
        }
        checked = TRUE;
    }

    return available;
}

void set_script_mode(HTMLOuterWindow *window, SCRIPTMODE mode)
{
    nsIWebBrowserSetup *setup;
    nsresult nsres;

    if(mode == SCRIPTMODE_ACTIVESCRIPT && !is_jscript_available()) {
        TRACE("jscript.dll not available\n");
        window->scriptmode = SCRIPTMODE_GECKO;
        return;
    }

    window->scriptmode = mode;

    if(!window->doc_obj->nscontainer || !window->doc_obj->nscontainer->webbrowser)
        return;

    nsres = nsIWebBrowser_QueryInterface(window->doc_obj->nscontainer->webbrowser,
            &IID_nsIWebBrowserSetup, (void**)&setup);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIWebBrowserSetup_SetProperty(setup, SETUP_ALLOW_JAVASCRIPT,
                window->scriptmode == SCRIPTMODE_GECKO);
        nsIWebBrowserSetup_Release(setup);
    }

    if(NS_FAILED(nsres))
        ERR("JavaScript setup failed: %08x\n", nsres);
}

void release_script_hosts(HTMLInnerWindow *window)
{
    ScriptHost *iter;

    while(!list_empty(&window->script_hosts)) {
        iter = LIST_ENTRY(list_head(&window->script_hosts), ScriptHost, entry);

        release_script_engine(iter);
        list_remove(&iter->entry);
        iter->window = NULL;
        IActiveScriptSite_Release(&iter->IActiveScriptSite_iface);
    }
}
