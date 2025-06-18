/*
 * Copyright 2015 Jacek Caban for CodeWeavers
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

#include "windows.h"
#include "initguid.h"
#include "dispex.h"
#include "ole2.h"
#include "olectl.h"
#include "objsafe.h"
#include "activscp.h"
#include "rpcproxy.h"
#include "msscript.h"
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(msscript);

#ifdef _WIN64

#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText

#else

#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText

#endif

struct ScriptControl;
typedef struct ConnectionPoint ConnectionPoint;
typedef struct ScriptProcedureCollection ScriptProcedureCollection;
typedef struct ScriptHost ScriptHost;

struct ConnectionPoint {
    IConnectionPoint IConnectionPoint_iface;
    ScriptControl *control;
    const IID *riid;
    ConnectionPoint *next;
};

struct named_item {
    struct list entry;
    BSTR name;
    IDispatch *disp;
};

struct module_enum {
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;

    UINT pos;
    ScriptHost *host;
    ScriptControl *control;
};

typedef struct {
    IScriptModule IScriptModule_iface;
    LONG ref;

    BSTR name;
    ScriptHost *host;
    IDispatch *script_dispatch;
    ITypeInfo *script_typeinfo;
    ITypeComp *script_typecomp;

    ScriptProcedureCollection *procedures;
} ScriptModule;

typedef struct {
    IScriptProcedure IScriptProcedure_iface;
    LONG ref;

    ULONG hash;
    struct list entry;

    BSTR name;
    USHORT num_args;
    VARTYPE ret_type;
} ScriptProcedure;

struct ScriptProcedureCollection {
    IScriptProcedureCollection IScriptProcedureCollection_iface;
    LONG ref;

    LONG count;
    ScriptModule *module;
    struct list hash_table[43];
};

struct procedure_enum {
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;

    WORD pos;
    WORD count;
    ScriptProcedureCollection *procedures;
};

typedef struct {
    IScriptError IScriptError_iface;
    IActiveScriptError *object;
    LONG ref;

    HRESULT number;
    BSTR text;
    BSTR source;
    BSTR desc;
    BSTR help_file;
    DWORD help_context;
    ULONG line;
    LONG column;

    BOOLEAN info_filled;
    BOOLEAN text_filled;
    BOOLEAN pos_filled;
} ScriptError;

struct ScriptHost {
    IActiveScriptSite IActiveScriptSite_iface;
    IActiveScriptSiteWindow IActiveScriptSiteWindow_iface;
    IServiceProvider IServiceProvider_iface;
    LONG ref;

    IActiveScript *script;
    IActiveScriptParse *parse;
    ScriptError *error;
    HWND site_hwnd;
    SCRIPTSTATE script_state;
    CLSID clsid;

    unsigned int module_count;
    struct list named_items;
};

struct ScriptControl {
    IScriptControl IScriptControl_iface;
    IPersistStreamInit IPersistStreamInit_iface;
    IOleObject IOleObject_iface;
    IOleControl IOleControl_iface;
    IQuickActivate IQuickActivate_iface;
    IViewObjectEx IViewObjectEx_iface;
    IPointerInactive IPointerInactive_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG ref;
    IOleClientSite *site;
    HWND site_hwnd;
    SIZEL extent;
    LONG timeout;
    VARIANT_BOOL allow_ui;
    VARIANT_BOOL use_safe_subset;

    /* connection points */
    ConnectionPoint *cp_list;
    ConnectionPoint cp_scsource;
    ConnectionPoint cp_propnotif;

    /* IViewObject sink */
    IAdviseSink *view_sink;
    DWORD view_sink_flags;

    /* modules */
    ScriptModule **modules;
    IScriptModuleCollection IScriptModuleCollection_iface;

    ScriptHost *host;
    ScriptError *error;
};

typedef enum tid_t {
    IScriptControl_tid,
    IScriptError_tid,
    IScriptModuleCollection_tid,
    IScriptModule_tid,
    IScriptProcedureCollection_tid,
    IScriptProcedure_tid,
    LAST_tid
} tid_t;

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
    &IID_IScriptControl,
    &IID_IScriptError,
    &IID_IScriptModuleCollection,
    &IID_IScriptModule,
    &IID_IScriptProcedureCollection,
    &IID_IScriptProcedure
};

static HRESULT load_typelib(void)
{
    HRESULT hres;
    ITypeLib *tl;

    hres = LoadRegTypeLib(&LIBID_MSScriptControl, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if(FAILED(hres)) {
        ERR("LoadRegTypeLib failed: %08lx\n", hres);
        return hres;
    }

    if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hres;
}

static HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (!typelib)
        hres = load_typelib();
    if (!typelib)
        return hres;

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08lx\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    ITypeInfo_AddRef(typeinfos[tid]);
    return S_OK;
}

static void release_typelib(void)
{
    unsigned i;

    if(!typelib)
        return;

    for(i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static inline BOOL is_power_of_2(unsigned x)
{
    return !(x & (x - 1));
}

static void clear_named_items(ScriptHost *host)
{
    struct named_item *item, *item1;
    LIST_FOR_EACH_ENTRY_SAFE(item, item1, &host->named_items, struct named_item, entry) {
       list_remove(&item->entry);
       SysFreeString(item->name);
       IDispatch_Release(item->disp);
       free(item);
    }
}

static struct named_item *host_get_named_item(ScriptHost *host, const WCHAR *nameW)
{
    struct named_item *item;

    LIST_FOR_EACH_ENTRY(item, &host->named_items, struct named_item, entry) {
        if (!wcscmp(item->name, nameW))
            return item;
    }

    return NULL;
}

static HRESULT get_script_dispatch(ScriptModule *module, IDispatch **disp)
{
    if (!module->script_dispatch)
    {
        HRESULT hr = IActiveScript_GetScriptDispatch(module->host->script,
                                                     module->name, &module->script_dispatch);
        if (FAILED(hr)) return hr;
    }
    *disp = module->script_dispatch;
    return S_OK;
}

static HRESULT get_script_typeinfo(ScriptModule *module, ITypeInfo **typeinfo)
{
    IDispatch *disp;
    HRESULT hr;

    if (!module->script_typeinfo)
    {
        hr = get_script_dispatch(module, &disp);
        if (FAILED(hr)) return hr;

        hr = IDispatch_GetTypeInfo(disp, 0, LOCALE_USER_DEFAULT, &module->script_typeinfo);
        if (FAILED(hr)) return hr;
    }
    *typeinfo = module->script_typeinfo;
    return S_OK;
}

static HRESULT get_script_typecomp(ScriptModule *module, ITypeInfo *typeinfo, ITypeComp **typecomp)
{
    HRESULT hr;

    if (!module->script_typecomp)
    {
        hr = ITypeInfo_QueryInterface(typeinfo, &IID_ITypeComp, (void**)&module->script_typecomp);
        if (FAILED(hr)) return hr;
    }
    *typecomp = module->script_typecomp;
    return S_OK;
}

static void uncache_module_objects(ScriptModule *module)
{
    if (module->script_dispatch)
    {
        IDispatch_Release(module->script_dispatch);
        module->script_dispatch = NULL;
    }
    if (module->script_typeinfo)
    {
        ITypeInfo_Release(module->script_typeinfo);
        module->script_typeinfo = NULL;
    }
    if (module->script_typecomp)
    {
        ITypeComp_Release(module->script_typecomp);
        module->script_typecomp = NULL;
    }
}

static HRESULT set_script_state(ScriptHost *host, SCRIPTSTATE state)
{
    HRESULT hr;

    hr = IActiveScript_SetScriptState(host->script, state);
    if (SUCCEEDED(hr))
        host->script_state = state;
    return hr;
}

static HRESULT start_script(ScriptModule *module)
{
    HRESULT hr = S_OK;

    if (module->host->script_state != SCRIPTSTATE_STARTED)
    {
        hr = set_script_state(module->host, SCRIPTSTATE_STARTED);
        if (SUCCEEDED(hr)) uncache_module_objects(module);
    }

    return hr;
}

static HRESULT add_script_object(ScriptHost *host, BSTR name, IDispatch *object, DWORD flags)
{
    struct named_item *item;
    HRESULT hr;

    if (host_get_named_item(host, name))
        return E_INVALIDARG;

    item = malloc(sizeof(*item));
    if (!item)
        return E_OUTOFMEMORY;

    item->name = SysAllocString(name);
    if (!item->name)
    {
        free(item);
        return E_OUTOFMEMORY;
    }
    IDispatch_AddRef(item->disp = object);
    list_add_tail(&host->named_items, &item->entry);

    hr = IActiveScript_AddNamedItem(host->script, name, flags);
    if (FAILED(hr))
    {
        list_remove(&item->entry);
        IDispatch_Release(item->disp);
        SysFreeString(item->name);
        free(item);
        return hr;
    }

    return hr;
}

static HRESULT parse_script_text(ScriptModule *module, BSTR script_text, DWORD flag, VARIANT *res)
{
    EXCEPINFO excepinfo;
    HRESULT hr;

    hr = start_script(module);
    if (FAILED(hr)) return hr;

    uncache_module_objects(module);
    if (module->procedures)
        module->procedures->count = -1;

    hr = IActiveScriptParse_ParseScriptText(module->host->parse, script_text, module->name,
                                            NULL, NULL, 0, 1, flag, res, &excepinfo);
    /* FIXME: more error handling */
    return hr;
}

static HRESULT WINAPI sp_caller_QueryInterface(IServiceProvider *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IServiceProvider, riid))
        *obj = iface;
    else
    {
        FIXME("(%p)->(%s)\n", iface, debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI sp_caller_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI sp_caller_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI sp_caller_QueryService(IServiceProvider *iface, REFGUID service, REFIID riid, void **obj)
{
    FIXME("(%p)->(%s %s %p): semi-stub\n", iface, debugstr_guid(service), debugstr_guid(riid), obj);

    *obj = NULL;
    if (IsEqualGUID(&SID_GetCaller, service))
        return S_OK;

    return E_NOINTERFACE;
}

static const IServiceProviderVtbl sp_caller_vtbl = {
    sp_caller_QueryInterface,
    sp_caller_AddRef,
    sp_caller_Release,
    sp_caller_QueryService
};

static IServiceProvider sp_caller = { &sp_caller_vtbl };

static HRESULT run_procedure(ScriptModule *module, BSTR procedure_name, SAFEARRAY *args, VARIANT *res)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    DISPPARAMS dp;
    DISPID dispid;
    HRESULT hr;
    UINT i;

    hr = start_script(module);
    if (FAILED(hr)) return hr;

    hr = get_script_dispatch(module, &disp);
    if (FAILED(hr)) return hr;

    hr = IDispatch_GetIDsOfNames(disp, &IID_NULL, &procedure_name, 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr)) return hr;

    dp.cArgs = args->rgsabound[0].cElements;
    dp.rgdispidNamedArgs = NULL;
    dp.cNamedArgs = 0;
    dp.rgvarg = malloc(dp.cArgs * sizeof(*dp.rgvarg));
    if (!dp.rgvarg) return E_OUTOFMEMORY;

    hr = SafeArrayLock(args);
    if (SUCCEEDED(hr))
    {
        /* The DISPPARAMS are stored in reverse order */
        for (i = 0; i < dp.cArgs; i++)
            dp.rgvarg[i] = *(VARIANT*)((char*)(args->pvData) + (dp.cArgs - i - 1) * args->cbElements);
        SafeArrayUnlock(args);

        hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        if (FAILED(hr))
        {
            hr = IDispatch_Invoke(disp, dispid, &IID_NULL, LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD, &dp, res, NULL, NULL);
        }
        else
        {
            hr = IDispatchEx_InvokeEx(dispex, dispid, LOCALE_USER_DEFAULT,
                                      DISPATCH_METHOD, &dp, res, NULL, &sp_caller);
            IDispatchEx_Release(dispex);
        }
    }
    free(dp.rgvarg);

    return hr;
}

static inline ScriptControl *impl_from_IScriptControl(IScriptControl *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IScriptControl_iface);
}

static inline ScriptControl *impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IOleObject_iface);
}

static inline ScriptControl *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IPersistStreamInit_iface);
}

static inline ScriptControl *impl_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IOleControl_iface);
}

static inline ScriptControl *impl_from_IQuickActivate(IQuickActivate *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IQuickActivate_iface);
}

static inline ScriptControl *impl_from_IViewObjectEx(IViewObjectEx *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IViewObjectEx_iface);
}

static inline ScriptControl *impl_from_IPointerInactive(IPointerInactive *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IPointerInactive_iface);
}

static inline ScriptControl *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IConnectionPointContainer_iface);
}

static inline ScriptProcedure *impl_from_IScriptProcedure(IScriptProcedure *iface)
{
    return CONTAINING_RECORD(iface, ScriptProcedure, IScriptProcedure_iface);
}

static inline ScriptProcedureCollection *impl_from_IScriptProcedureCollection(IScriptProcedureCollection *iface)
{
    return CONTAINING_RECORD(iface, ScriptProcedureCollection, IScriptProcedureCollection_iface);
}

static inline ScriptControl *impl_from_IScriptModuleCollection(IScriptModuleCollection *iface)
{
    return CONTAINING_RECORD(iface, ScriptControl, IScriptModuleCollection_iface);
}

static inline ScriptModule *impl_from_IScriptModule(IScriptModule *iface)
{
    return CONTAINING_RECORD(iface, ScriptModule, IScriptModule_iface);
}

static inline ScriptError *impl_from_IScriptError(IScriptError *iface)
{
    return CONTAINING_RECORD(iface, ScriptError, IScriptError_iface);
}

static inline ConnectionPoint *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return CONTAINING_RECORD(iface, ConnectionPoint, IConnectionPoint_iface);
}

static inline ScriptHost *impl_from_IActiveScriptSite(IActiveScriptSite *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSite_iface);
}

static inline ScriptHost *impl_from_IActiveScriptSiteWindow(IActiveScriptSiteWindow *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteWindow_iface);
}

static inline ScriptHost *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IServiceProvider_iface);
}

static inline struct module_enum *module_enum_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, struct module_enum, IEnumVARIANT_iface);
}

static inline struct procedure_enum *procedure_enum_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, struct procedure_enum, IEnumVARIANT_iface);
}

/* IActiveScriptSite */
static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSite %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteWindow, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteWindow %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteWindow_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        clear_named_items(This);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *lcid)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p, %p)\n", This, lcid);

    *lcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR name, DWORD mask,
    IUnknown **unk, ITypeInfo **ti)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    struct named_item *item;

    TRACE("(%p, %s, %#lx, %p, %p)\n", This, debugstr_w(name), mask, unk, ti);

    item = host_get_named_item(This, name);
    if (!item)
        return TYPE_E_ELEMENTNOTFOUND;

    if (mask != SCRIPTINFO_IUNKNOWN) {
        FIXME("mask %#lx is not supported\n", mask);
        return E_NOTIMPL;
    }

    *unk = (IUnknown*)item->disp;
    IUnknown_AddRef(*unk);

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *version)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    FIXME("(%p, %p)\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface, const VARIANT *result,
    const EXCEPINFO *ei)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    FIXME("(%p, %s, %p)\n", This, debugstr_variant(result), ei);

    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE state)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    FIXME("(%p, %d)\n", This, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *script_error)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p, %p)\n", This, script_error);

    if (This->error)
    {
        IScriptError_Clear(&This->error->IScriptError_iface);
        IActiveScriptError_AddRef(script_error);
        This->error->object = script_error;
    }
    return S_FALSE;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
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

/* IActiveScriptSiteWindow */
static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface, REFIID riid, void **obj)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, obj);
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

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *hwnd)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);

    TRACE("(%p, %p)\n", This, hwnd);

    if (!hwnd) return E_POINTER;
    if (This->site_hwnd == ((HWND)-1)) return E_FAIL;

    *hwnd = This->site_hwnd;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL enable)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);

    FIXME("(%p, %d): stub\n", This, enable);

    return S_OK;
}

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

/* IServiceProvider */
static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **obj)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, obj);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID service,
    REFIID riid, void **obj)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(service), debugstr_guid(riid), obj);

    return E_NOTIMPL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static HRESULT WINAPI ScriptProcedure_QueryInterface(IScriptProcedure *iface, REFIID riid, void **ppv)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);

    if (IsEqualGUID(&IID_IDispatch, riid) || IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IScriptProcedure, riid))
    {
        *ppv = &This->IScriptProcedure_iface;
    }
    else
    {
        WARN("unsupported interface: (%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptProcedure_AddRef(IScriptProcedure *iface)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ScriptProcedure_Release(IScriptProcedure *iface)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        list_remove(&This->entry);
        SysFreeString(This->name);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI ScriptProcedure_GetTypeInfoCount(IScriptProcedure *iface, UINT *pctinfo)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptProcedure_GetTypeInfo(IScriptProcedure *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);

    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IScriptProcedure_tid, ppTInfo);
}

static HRESULT WINAPI ScriptProcedure_GetIDsOfNames(IScriptProcedure *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IScriptProcedure_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptProcedure_Invoke(IScriptProcedure *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IScriptProcedure_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                              pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptProcedure_get_Name(IScriptProcedure *iface, BSTR *pbstrName)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);

    TRACE("(%p)->(%p)\n", This, pbstrName);

    if (!pbstrName) return E_POINTER;

    *pbstrName = SysAllocString(This->name);
    return *pbstrName ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI ScriptProcedure_get_NumArgs(IScriptProcedure *iface, LONG *pcArgs)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);

    TRACE("(%p)->(%p)\n", This, pcArgs);

    if (!pcArgs) return E_POINTER;

    *pcArgs = This->num_args;
    return S_OK;
}

static HRESULT WINAPI ScriptProcedure_get_HasReturnValue(IScriptProcedure *iface, VARIANT_BOOL *pfHasReturnValue)
{
    ScriptProcedure *This = impl_from_IScriptProcedure(iface);

    TRACE("(%p)->(%p)\n", This, pfHasReturnValue);

    if (!pfHasReturnValue) return E_POINTER;

    *pfHasReturnValue = (This->ret_type == VT_VOID) ? VARIANT_FALSE : VARIANT_TRUE;
    return S_OK;
}

static const IScriptProcedureVtbl ScriptProcedureVtbl = {
    ScriptProcedure_QueryInterface,
    ScriptProcedure_AddRef,
    ScriptProcedure_Release,
    ScriptProcedure_GetTypeInfoCount,
    ScriptProcedure_GetTypeInfo,
    ScriptProcedure_GetIDsOfNames,
    ScriptProcedure_Invoke,
    ScriptProcedure_get_Name,
    ScriptProcedure_get_NumArgs,
    ScriptProcedure_get_HasReturnValue
};

/* This function always releases the FUNCDESC passed in */
static HRESULT get_script_procedure(ScriptProcedureCollection *procedures, ITypeInfo *typeinfo,
        FUNCDESC *desc, IScriptProcedure **procedure)
{
    struct list *proc_list;
    ScriptProcedure *proc;
    ULONG hash;
    HRESULT hr;
    BSTR str;
    UINT len;

    hr = ITypeInfo_GetNames(typeinfo, desc->memid, &str, 1, &len);
    if (FAILED(hr)) goto done;

    len = SysStringLen(str);
    hash = LHashValOfNameSys(sizeof(void*) == 8 ? SYS_WIN64 : SYS_WIN32, LOCALE_USER_DEFAULT, str);
    proc_list = &procedures->hash_table[hash % ARRAY_SIZE(procedures->hash_table)];

    /* Try to find it in the hash table */
    LIST_FOR_EACH_ENTRY(proc, proc_list, ScriptProcedure, entry)
    {
        if (proc->hash == hash && SysStringLen(proc->name) == len &&
            !memcmp(proc->name, str, len * sizeof(*str)))
        {
            SysFreeString(str);
            IScriptProcedure_AddRef(&proc->IScriptProcedure_iface);
            *procedure = &proc->IScriptProcedure_iface;
            goto done;
        }
    }

    if (!(proc = malloc(sizeof(*proc))))
    {
        hr = E_OUTOFMEMORY;
        SysFreeString(str);
        goto done;
    }

    proc->IScriptProcedure_iface.lpVtbl = &ScriptProcedureVtbl;
    proc->ref = 1;
    proc->hash = hash;
    proc->name = str;
    proc->num_args = desc->cParams + desc->cParamsOpt;
    proc->ret_type = desc->elemdescFunc.tdesc.vt;
    list_add_tail(proc_list, &proc->entry);

    *procedure = &proc->IScriptProcedure_iface;

done:
    ITypeInfo_ReleaseFuncDesc(typeinfo, desc);
    return hr;
}

static HRESULT WINAPI procedure_enum_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    struct procedure_enum *This = procedure_enum_from_IEnumVARIANT(iface);

    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IEnumVARIANT, riid))
    {
        *ppv = &This->IEnumVARIANT_iface;
    }
    else
    {
        WARN("unsupported interface: (%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI procedure_enum_AddRef(IEnumVARIANT *iface)
{
    struct procedure_enum *This = procedure_enum_from_IEnumVARIANT(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI procedure_enum_Release(IEnumVARIANT *iface)
{
    struct procedure_enum *This = procedure_enum_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        IScriptProcedureCollection_Release(&This->procedures->IScriptProcedureCollection_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI procedure_enum_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    struct procedure_enum *This = procedure_enum_from_IEnumVARIANT(iface);
    FUNCDESC *desc;
    ITypeInfo *ti;
    UINT i, num;
    HRESULT hr;

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgVar, pCeltFetched);

    if (!rgVar) return E_POINTER;
    if (!This->procedures->module->host) return E_FAIL;

    hr = start_script(This->procedures->module);
    if (FAILED(hr)) return hr;

    hr = get_script_typeinfo(This->procedures->module, &ti);
    if (FAILED(hr)) return hr;

    num = min(celt, This->count - This->pos);
    for (i = 0; i < num; i++)
    {
        hr = ITypeInfo_GetFuncDesc(ti, This->pos + i, &desc);
        if (FAILED(hr)) break;

        hr = get_script_procedure(This->procedures, ti, desc, (IScriptProcedure**)&V_DISPATCH(rgVar + i));
        if (FAILED(hr)) break;

        V_VT(rgVar + i) = VT_DISPATCH;
    }

    if (FAILED(hr))
    {
        while (i--)
            VariantClear(rgVar + i);
        if (pCeltFetched) *pCeltFetched = 0;
        return hr;
    }

    This->pos += i;

    if (pCeltFetched) *pCeltFetched = i;
    return i == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI procedure_enum_Skip(IEnumVARIANT *iface, ULONG celt)
{
    struct procedure_enum *This = procedure_enum_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%lu)\n", This, celt);

    if (This->count - This->pos < celt)
    {
        This->pos = This->count;
        return S_FALSE;
    }
    This->pos += celt;
    return S_OK;
}

static HRESULT WINAPI procedure_enum_Reset(IEnumVARIANT *iface)
{
    struct procedure_enum *This = procedure_enum_from_IEnumVARIANT(iface);

    TRACE("(%p)\n", This);

    This->pos = 0;
    return S_OK;
}

static HRESULT WINAPI procedure_enum_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    struct procedure_enum *This = procedure_enum_from_IEnumVARIANT(iface);
    struct procedure_enum *clone;

    TRACE("(%p)->(%p)\n", This, ppEnum);

    if (!ppEnum) return E_POINTER;

    if (!(clone = malloc(sizeof(*clone))))
        return E_OUTOFMEMORY;

    *clone = *This;
    clone->ref = 1;
    IScriptProcedureCollection_AddRef(&This->procedures->IScriptProcedureCollection_iface);

    *ppEnum = &clone->IEnumVARIANT_iface;
    return S_OK;
}

static const IEnumVARIANTVtbl procedure_enum_vtbl = {
    procedure_enum_QueryInterface,
    procedure_enum_AddRef,
    procedure_enum_Release,
    procedure_enum_Next,
    procedure_enum_Skip,
    procedure_enum_Reset,
    procedure_enum_Clone
};

static HRESULT WINAPI ScriptProcedureCollection_QueryInterface(IScriptProcedureCollection *iface, REFIID riid, void **ppv)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);

    if (IsEqualGUID(&IID_IDispatch, riid) || IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IScriptProcedureCollection, riid))
    {
        *ppv = &This->IScriptProcedureCollection_iface;
    }
    else
    {
        WARN("unsupported interface: (%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptProcedureCollection_AddRef(IScriptProcedureCollection *iface)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ScriptProcedureCollection_Release(IScriptProcedureCollection *iface)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    UINT i;

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        /* Unlink any dangling items from the hash table */
        for (i = 0; i < ARRAY_SIZE(This->hash_table); i++)
            list_remove(&This->hash_table[i]);

        This->module->procedures = NULL;
        IScriptModule_Release(&This->module->IScriptModule_iface);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI ScriptProcedureCollection_GetTypeInfoCount(IScriptProcedureCollection *iface, UINT *pctinfo)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptProcedureCollection_GetTypeInfo(IScriptProcedureCollection *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);

    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IScriptProcedureCollection_tid, ppTInfo);
}

static HRESULT WINAPI ScriptProcedureCollection_GetIDsOfNames(IScriptProcedureCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IScriptProcedureCollection_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptProcedureCollection_Invoke(IScriptProcedureCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IScriptProcedureCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                              pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptProcedureCollection_get__NewEnum(IScriptProcedureCollection *iface, IUnknown **ppenumProcedures)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);
    struct procedure_enum *proc_enum;
    TYPEATTR *attr;
    ITypeInfo *ti;
    UINT count;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ppenumProcedures);

    if (!ppenumProcedures) return E_POINTER;
    if (!This->module->host) return E_FAIL;

    hr = start_script(This->module);
    if (FAILED(hr)) return hr;

    hr = get_script_typeinfo(This->module, &ti);
    if (FAILED(hr)) return hr;

    hr = ITypeInfo_GetTypeAttr(ti, &attr);
    if (FAILED(hr)) return hr;

    count = attr->cFuncs;
    ITypeInfo_ReleaseTypeAttr(ti, attr);

    if (!(proc_enum = malloc(sizeof(*proc_enum))))
        return E_OUTOFMEMORY;

    proc_enum->IEnumVARIANT_iface.lpVtbl = &procedure_enum_vtbl;
    proc_enum->ref = 1;
    proc_enum->pos = 0;
    proc_enum->count = count;
    proc_enum->procedures = This;
    IScriptProcedureCollection_AddRef(&This->IScriptProcedureCollection_iface);

    *ppenumProcedures = (IUnknown*)&proc_enum->IEnumVARIANT_iface;
    return S_OK;
}

static HRESULT WINAPI ScriptProcedureCollection_get_Item(IScriptProcedureCollection *iface, VARIANT index,
        IScriptProcedure **ppdispProcedure)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);
    ITypeInfo *typeinfo;
    FUNCDESC *desc;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, wine_dbgstr_variant(&index), ppdispProcedure);

    if (!ppdispProcedure) return E_POINTER;
    if (!This->module->host) return E_FAIL;

    hr = start_script(This->module);
    if (FAILED(hr)) return hr;

    hr = get_script_typeinfo(This->module, &typeinfo);
    if (FAILED(hr)) return hr;

    if (V_VT(&index) == VT_BSTR)
    {
        struct list *proc_list;
        ScriptProcedure *proc;
        ITypeComp *comp;
        BINDPTR bindptr;
        DESCKIND kind;
        ULONG hash;
        UINT len;

        len = SysStringLen(V_BSTR(&index));
        hash = LHashValOfNameSys(sizeof(void*) == 8 ? SYS_WIN64 : SYS_WIN32, LOCALE_USER_DEFAULT, V_BSTR(&index));
        proc_list = &This->hash_table[hash % ARRAY_SIZE(This->hash_table)];

        /* Try to find it in the hash table */
        LIST_FOR_EACH_ENTRY(proc, proc_list, ScriptProcedure, entry)
        {
            if (proc->hash == hash && SysStringLen(proc->name) == len &&
                !memcmp(proc->name, V_BSTR(&index), len * sizeof(WCHAR)))
            {
                IScriptProcedure_AddRef(&proc->IScriptProcedure_iface);
                *ppdispProcedure = &proc->IScriptProcedure_iface;
                return S_OK;
            }
        }

        hr = get_script_typecomp(This->module, typeinfo, &comp);
        if (FAILED(hr)) return hr;

        hr = ITypeComp_Bind(comp, V_BSTR(&index), hash, INVOKE_FUNC, &typeinfo, &kind, &bindptr);
        if (FAILED(hr)) return hr;

        switch (kind)
        {
        case DESCKIND_FUNCDESC:
            hr = get_script_procedure(This, typeinfo, bindptr.lpfuncdesc, ppdispProcedure);
            ITypeInfo_Release(typeinfo);
            return hr;
        case DESCKIND_IMPLICITAPPOBJ:
        case DESCKIND_VARDESC:
            ITypeInfo_ReleaseVarDesc(typeinfo, bindptr.lpvardesc);
            ITypeInfo_Release(typeinfo);
            break;
        case DESCKIND_TYPECOMP:
            ITypeComp_Release(bindptr.lptcomp);
            break;
        default:
            break;
        }
        return CTL_E_ILLEGALFUNCTIONCALL;
    }

    hr = VariantChangeType(&index, &index, 0, VT_INT);
    if (FAILED(hr)) return hr;
    if (V_INT(&index) <= 0) return 0x800a0009;

    hr = ITypeInfo_GetFuncDesc(typeinfo, V_INT(&index) - 1, &desc);
    if (FAILED(hr)) return hr;

    return get_script_procedure(This, typeinfo, desc, ppdispProcedure);
}

static HRESULT WINAPI ScriptProcedureCollection_get_Count(IScriptProcedureCollection *iface, LONG *plCount)
{
    ScriptProcedureCollection *This = impl_from_IScriptProcedureCollection(iface);
    TYPEATTR *attr;
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, plCount);

    if (!plCount) return E_POINTER;
    if (!This->module->host) return E_FAIL;

    if (This->count == -1)
    {
        hr = start_script(This->module);
        if (FAILED(hr)) return hr;

        hr = get_script_typeinfo(This->module, &ti);
        if (FAILED(hr)) return hr;

        hr = ITypeInfo_GetTypeAttr(ti, &attr);
        if (FAILED(hr)) return hr;

        This->count = attr->cFuncs;
        ITypeInfo_ReleaseTypeAttr(ti, attr);
    }

    *plCount = This->count;
    return S_OK;
}

static const IScriptProcedureCollectionVtbl ScriptProcedureCollectionVtbl = {
    ScriptProcedureCollection_QueryInterface,
    ScriptProcedureCollection_AddRef,
    ScriptProcedureCollection_Release,
    ScriptProcedureCollection_GetTypeInfoCount,
    ScriptProcedureCollection_GetTypeInfo,
    ScriptProcedureCollection_GetIDsOfNames,
    ScriptProcedureCollection_Invoke,
    ScriptProcedureCollection_get__NewEnum,
    ScriptProcedureCollection_get_Item,
    ScriptProcedureCollection_get_Count
};

static void detach_script_host(ScriptHost *host)
{
    if (--host->module_count)
        return;

    if (host->script) {
        IActiveScript_Close(host->script);
        IActiveScript_Release(host->script);
    }

    if (host->parse)
        IActiveScriptParse_Release(host->parse);

    if (host->error)
        IScriptError_Release(&host->error->IScriptError_iface);

    host->parse = NULL;
    host->error = NULL;
    host->script = NULL;
}

static void detach_module(ScriptModule *module)
{
    ScriptHost *host = module->host;

    if (host) {
        module->host = NULL;
        detach_script_host(host);
        IActiveScriptSite_Release(&host->IActiveScriptSite_iface);
    }
}

static HRESULT WINAPI ScriptModule_QueryInterface(IScriptModule *iface, REFIID riid, void **ppv)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    if (IsEqualGUID(&IID_IDispatch, riid) || IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IScriptModule, riid))
    {
        *ppv = &This->IScriptModule_iface;
    }
    else
    {
        WARN("unsupported interface: (%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptModule_AddRef(IScriptModule *iface)
{
    ScriptModule *This = impl_from_IScriptModule(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ScriptModule_Release(IScriptModule *iface)
{
    ScriptModule *This = impl_from_IScriptModule(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        detach_module(This);
        SysFreeString(This->name);
        uncache_module_objects(This);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ScriptModule_GetTypeInfoCount(IScriptModule *iface, UINT *pctinfo)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptModule_GetTypeInfo(IScriptModule *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IScriptModule_tid, ppTInfo);
}

static HRESULT WINAPI ScriptModule_GetIDsOfNames(IScriptModule *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ScriptModule *This = impl_from_IScriptModule(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IScriptModule_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptModule_Invoke(IScriptModule *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptModule *This = impl_from_IScriptModule(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IScriptModule_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                              pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptModule_get_Name(IScriptModule *iface, BSTR *pbstrName)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    TRACE("(%p)->(%p)\n", This, pbstrName);

    if (!pbstrName) return E_POINTER;
    if (!This->host) return E_FAIL;

    *pbstrName = SysAllocString(This->name ? This->name : L"Global");
    return *pbstrName ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI ScriptModule_get_CodeObject(IScriptModule *iface, IDispatch **ppdispObject)
{
    ScriptModule *This = impl_from_IScriptModule(iface);
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ppdispObject);

    if (!This->host) return E_FAIL;

    hr = start_script(This);
    if (FAILED(hr)) return hr;

    hr = get_script_dispatch(This, ppdispObject);
    if (FAILED(hr)) return hr;

    IDispatch_AddRef(*ppdispObject);
    return hr;
}

static HRESULT WINAPI ScriptModule_get_Procedures(IScriptModule *iface, IScriptProcedureCollection **ppdispProcedures)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    TRACE("(%p)->(%p)\n", This, ppdispProcedures);

    if (!This->host)
        return E_FAIL;

    if (This->procedures)
        IScriptProcedureCollection_AddRef(&This->procedures->IScriptProcedureCollection_iface);
    else
    {
        ScriptProcedureCollection *procs;
        UINT i;

        if (!(procs = malloc(sizeof(*procs))))
            return E_OUTOFMEMORY;

        procs->IScriptProcedureCollection_iface.lpVtbl = &ScriptProcedureCollectionVtbl;
        procs->ref = 1;
        procs->count = -1;
        procs->module = This;
        for (i = 0; i < ARRAY_SIZE(procs->hash_table); i++)
            list_init(&procs->hash_table[i]);

        This->procedures = procs;
        IScriptModule_AddRef(&This->IScriptModule_iface);
    }

    *ppdispProcedures = &This->procedures->IScriptProcedureCollection_iface;
    return S_OK;
}

static HRESULT WINAPI ScriptModule_AddCode(IScriptModule *iface, BSTR code)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(code));

    if (!This->host)
        return E_FAIL;

    return parse_script_text(This, code, SCRIPTTEXT_ISVISIBLE, NULL);
}

static HRESULT WINAPI ScriptModule_Eval(IScriptModule *iface, BSTR expression, VARIANT *res)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(expression), res);

    if (!res)
        return E_POINTER;
    V_VT(res) = VT_EMPTY;
    if (!This->host)
        return E_FAIL;

    return parse_script_text(This, expression, SCRIPTTEXT_ISEXPRESSION, res);
}

static HRESULT WINAPI ScriptModule_ExecuteStatement(IScriptModule *iface, BSTR statement)
{
    ScriptModule *This = impl_from_IScriptModule(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(statement));

    if (!This->host)
        return E_FAIL;

    return parse_script_text(This, statement, 0, NULL);
}

static HRESULT WINAPI ScriptModule_Run(IScriptModule *iface, BSTR procedure_name, SAFEARRAY **parameters, VARIANT *res)
{
    ScriptModule *This = impl_from_IScriptModule(iface);
    SAFEARRAY *sa;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(procedure_name), parameters, res);

    if (!parameters || !res) return E_POINTER;
    if (!(sa = *parameters)) return E_POINTER;

    V_VT(res) = VT_EMPTY;
    if (sa->cDims == 0) return DISP_E_BADINDEX;
    if (!(sa->fFeatures & FADF_VARIANT)) return DISP_E_BADVARTYPE;
    if (!This->host) return E_FAIL;

    return run_procedure(This, procedure_name, sa, res);
}

static const IScriptModuleVtbl ScriptModuleVtbl = {
    ScriptModule_QueryInterface,
    ScriptModule_AddRef,
    ScriptModule_Release,
    ScriptModule_GetTypeInfoCount,
    ScriptModule_GetTypeInfo,
    ScriptModule_GetIDsOfNames,
    ScriptModule_Invoke,
    ScriptModule_get_Name,
    ScriptModule_get_CodeObject,
    ScriptModule_get_Procedures,
    ScriptModule_AddCode,
    ScriptModule_Eval,
    ScriptModule_ExecuteStatement,
    ScriptModule_Run
};

static HRESULT WINAPI module_enum_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    struct module_enum *This = module_enum_from_IEnumVARIANT(iface);

    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IEnumVARIANT, riid))
    {
        *ppv = &This->IEnumVARIANT_iface;
    }
    else
    {
        WARN("unsupported interface: (%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI module_enum_AddRef(IEnumVARIANT *iface)
{
    struct module_enum *This = module_enum_from_IEnumVARIANT(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI module_enum_Release(IEnumVARIANT *iface)
{
    struct module_enum *This = module_enum_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        IActiveScriptSite_Release(&This->host->IActiveScriptSite_iface);
        IScriptControl_Release(&This->control->IScriptControl_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI module_enum_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    struct module_enum *This = module_enum_from_IEnumVARIANT(iface);
    unsigned int i, num;

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgVar, pCeltFetched);

    if (!rgVar) return E_POINTER;
    if (This->host != This->control->host) return E_FAIL;

    num = min(celt, This->host->module_count - This->pos);
    for (i = 0; i < num; i++)
    {
        V_VT(rgVar + i) = VT_DISPATCH;
        V_DISPATCH(rgVar + i) = (IDispatch*)(&This->control->modules[This->pos++]->IScriptModule_iface);
        IDispatch_AddRef(V_DISPATCH(rgVar + i));
    }

    if (pCeltFetched) *pCeltFetched = i;
    return i == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI module_enum_Skip(IEnumVARIANT *iface, ULONG celt)
{
    struct module_enum *This = module_enum_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%lu)\n", This, celt);

    if (This->host != This->control->host) return E_FAIL;

    if (This->host->module_count - This->pos < celt)
    {
        This->pos = This->host->module_count;
        return S_FALSE;
    }
    This->pos += celt;
    return S_OK;
}

static HRESULT WINAPI module_enum_Reset(IEnumVARIANT *iface)
{
    struct module_enum *This = module_enum_from_IEnumVARIANT(iface);

    TRACE("(%p)\n", This);

    if (This->host != This->control->host) return E_FAIL;

    This->pos = 0;
    return S_OK;
}

static HRESULT WINAPI module_enum_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    struct module_enum *This = module_enum_from_IEnumVARIANT(iface);
    struct module_enum *clone;

    TRACE("(%p)->(%p)\n", This, ppEnum);

    if (!ppEnum) return E_POINTER;
    if (This->host != This->control->host) return E_FAIL;

    if (!(clone = malloc(sizeof(*clone))))
        return E_OUTOFMEMORY;

    *clone = *This;
    clone->ref = 1;
    IActiveScriptSite_AddRef(&This->host->IActiveScriptSite_iface);
    IScriptControl_AddRef(&This->control->IScriptControl_iface);

    *ppEnum = &clone->IEnumVARIANT_iface;
    return S_OK;
}

static const IEnumVARIANTVtbl module_enum_vtbl = {
    module_enum_QueryInterface,
    module_enum_AddRef,
    module_enum_Release,
    module_enum_Next,
    module_enum_Skip,
    module_enum_Reset,
    module_enum_Clone
};

static ScriptModule *create_module(ScriptHost *host, BSTR name)
{
    ScriptModule *module;

    if (!(module = calloc(1, sizeof(*module)))) return NULL;

    module->IScriptModule_iface.lpVtbl = &ScriptModuleVtbl;
    module->ref = 1;
    if (name && !(module->name = SysAllocString(name)))
    {
        free(module);
        return NULL;
    }
    module->host = host;
    IActiveScriptSite_AddRef(&host->IActiveScriptSite_iface);
    return module;
}

static void release_modules(ScriptControl *control, BOOL force_detach)
{
    unsigned int i, module_count = control->host->module_count;

    for (i = 0; i < module_count; i++) {
        if (force_detach) detach_module(control->modules[i]);
        IScriptModule_Release(&control->modules[i]->IScriptModule_iface);
    }

    free(control->modules);
}

static ScriptModule *find_module(ScriptControl *control, BSTR name)
{
    unsigned int i;

    if (!wcsicmp(name, L"Global"))
        return control->modules[0];

    for (i = 1; i < control->host->module_count; i++)
    {
        if (!wcsicmp(name, control->modules[i]->name))
            return control->modules[i];
    }
    return NULL;
}

static HRESULT WINAPI ScriptModuleCollection_QueryInterface(IScriptModuleCollection *iface, REFIID riid, void **ppv)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);

    if (IsEqualGUID(&IID_IDispatch, riid) || IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IScriptModuleCollection, riid))
    {
        *ppv = &This->IScriptModuleCollection_iface;
    }
    else
    {
        WARN("unsupported interface: (%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptModuleCollection_AddRef(IScriptModuleCollection *iface)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI ScriptModuleCollection_Release(IScriptModuleCollection *iface)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI ScriptModuleCollection_GetTypeInfoCount(IScriptModuleCollection *iface, UINT *pctinfo)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptModuleCollection_GetTypeInfo(IScriptModuleCollection *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);

    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IScriptModuleCollection_tid, ppTInfo);
}

static HRESULT WINAPI ScriptModuleCollection_GetIDsOfNames(IScriptModuleCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IScriptModuleCollection_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptModuleCollection_Invoke(IScriptModuleCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IScriptModuleCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                              pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptModuleCollection_get__NewEnum(IScriptModuleCollection *iface, IUnknown **ppenumContexts)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);
    struct module_enum *module_enum;

    TRACE("(%p)->(%p)\n", This, ppenumContexts);

    if (!ppenumContexts) return E_POINTER;
    if (!This->host) return E_FAIL;

    if (!(module_enum = malloc(sizeof(*module_enum))))
        return E_OUTOFMEMORY;

    module_enum->IEnumVARIANT_iface.lpVtbl = &module_enum_vtbl;
    module_enum->ref = 1;
    module_enum->pos = 0;
    module_enum->host = This->host;
    module_enum->control = This;
    IActiveScriptSite_AddRef(&This->host->IActiveScriptSite_iface);
    IScriptControl_AddRef(&This->IScriptControl_iface);

    *ppenumContexts = (IUnknown*)&module_enum->IEnumVARIANT_iface;
    return S_OK;
}

static HRESULT WINAPI ScriptModuleCollection_get_Item(IScriptModuleCollection *iface, VARIANT index,
        IScriptModule **ppmod)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);
    ScriptModule *module;
    unsigned int i;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, wine_dbgstr_variant(&index), ppmod);

    if (!ppmod) return E_POINTER;
    if (!This->host) return E_FAIL;

    if (V_VT(&index) == VT_BSTR)
    {
        module = find_module(This, V_BSTR(&index));
        if (!module) return CTL_E_ILLEGALFUNCTIONCALL;
    }
    else
    {
        hr = VariantChangeType(&index, &index, 0, VT_INT);
        if (FAILED(hr)) return hr;

        i = V_INT(&index) - 1;
        if (i >= This->host->module_count) return 0x800a0009;

        module = This->modules[i];
    }

    *ppmod = &module->IScriptModule_iface;
    IScriptModule_AddRef(*ppmod);
    return S_OK;
}

static HRESULT WINAPI ScriptModuleCollection_get_Count(IScriptModuleCollection *iface, LONG *plCount)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);

    TRACE("(%p)->(%p)\n", This, plCount);

    if (!plCount) return E_POINTER;
    if (!This->host) return E_FAIL;

    *plCount = This->host->module_count;
    return S_OK;
}

static HRESULT WINAPI ScriptModuleCollection_Add(IScriptModuleCollection *iface, BSTR name,
        VARIANT *object, IScriptModule **ppmod)
{
    ScriptControl *This = impl_from_IScriptModuleCollection(iface);
    ScriptModule *module, **modules;
    ScriptHost *host = This->host;
    HRESULT hr;

    TRACE("(%p)->(%s %s %p)\n", This, wine_dbgstr_w(name), wine_dbgstr_variant(object), ppmod);

    if (!ppmod) return E_POINTER;
    if (!name || V_VT(object) != VT_DISPATCH) return E_INVALIDARG;
    if (!host) return E_FAIL;
    if (find_module(This, name)) return E_INVALIDARG;

    /* See if we need to grow the array */
    if (is_power_of_2(host->module_count))
    {
        modules = realloc(This->modules, host->module_count * 2 * sizeof(*This->modules));
        if (!modules) return E_OUTOFMEMORY;
        This->modules = modules;
    }

    if (!(module = create_module(host, name)))
        return E_OUTOFMEMORY;

    /* If no object, Windows only calls AddNamedItem without adding a NULL object */
    if (V_DISPATCH(object))
        hr = add_script_object(host, name, V_DISPATCH(object), 0);
    else
        hr = IActiveScript_AddNamedItem(host->script, name, SCRIPTITEM_CODEONLY);

    if (FAILED(hr))
    {
        IScriptModule_Release(&module->IScriptModule_iface);
        return hr;
    }
    This->modules[host->module_count++] = module;

    *ppmod = &module->IScriptModule_iface;
    IScriptModule_AddRef(*ppmod);
    return S_OK;
}

static const IScriptModuleCollectionVtbl ScriptModuleCollectionVtbl = {
    ScriptModuleCollection_QueryInterface,
    ScriptModuleCollection_AddRef,
    ScriptModuleCollection_Release,
    ScriptModuleCollection_GetTypeInfoCount,
    ScriptModuleCollection_GetTypeInfo,
    ScriptModuleCollection_GetIDsOfNames,
    ScriptModuleCollection_Invoke,
    ScriptModuleCollection_get__NewEnum,
    ScriptModuleCollection_get_Item,
    ScriptModuleCollection_get_Count,
    ScriptModuleCollection_Add
};

static void fill_error_info(ScriptError *error)
{
    EXCEPINFO info;

    if (error->info_filled) return;
    error->info_filled = TRUE;

    if (!error->object)
        return;
    if (FAILED(IActiveScriptError_GetExceptionInfo(error->object, &info)))
        return;
    if (info.pfnDeferredFillIn)
        info.pfnDeferredFillIn(&info);

    error->number = info.scode;
    error->source = info.bstrSource;
    error->desc = info.bstrDescription;
    error->help_file = info.bstrHelpFile;
    error->help_context = info.dwHelpContext;
}

static void fill_error_text(ScriptError *error)
{
    if (error->text_filled) return;
    error->text_filled = TRUE;

    if (error->object)
        IActiveScriptError_GetSourceLineText(error->object, &error->text);
}

static void fill_error_pos(ScriptError *error)
{
    DWORD context;
    LONG column;
    ULONG line;

    if (error->pos_filled) return;
    error->pos_filled = TRUE;

    if (!error->object)
        return;
    if (FAILED(IActiveScriptError_GetSourcePosition(error->object, &context, &line, &column)))
        return;

    error->line = line;
    error->column = column;
}

static HRESULT WINAPI ScriptError_QueryInterface(IScriptError *iface, REFIID riid, void **ppv)
{
    ScriptError *This = impl_from_IScriptError(iface);

    if (IsEqualGUID(&IID_IDispatch, riid) || IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IScriptError, riid))
    {
        *ppv = &This->IScriptError_iface;
    }
    else
    {
        WARN("unsupported interface: (%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptError_AddRef(IScriptError *iface)
{
    ScriptError *This = impl_from_IScriptError(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ScriptError_Release(IScriptError *iface)
{
    ScriptError *This = impl_from_IScriptError(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        IScriptError_Clear(&This->IScriptError_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ScriptError_GetTypeInfoCount(IScriptError *iface, UINT *pctinfo)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptError_GetTypeInfo(IScriptError *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IScriptError_tid, ppTInfo);
}

static HRESULT WINAPI ScriptError_GetIDsOfNames(IScriptError *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ScriptError *This = impl_from_IScriptError(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IScriptError_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptError_Invoke(IScriptError *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptError *This = impl_from_IScriptError(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IScriptError_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                              pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ScriptError_get_Number(IScriptError *iface, LONG *plNumber)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, plNumber);

    fill_error_info(This);
    *plNumber = This->number;
    return S_OK;
}

static HRESULT WINAPI ScriptError_get_Source(IScriptError *iface, BSTR *pbstrSource)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, pbstrSource);

    fill_error_info(This);
    *pbstrSource = SysAllocString(This->source);
    return S_OK;
}

static HRESULT WINAPI ScriptError_get_Description(IScriptError *iface, BSTR *pbstrDescription)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, pbstrDescription);

    fill_error_info(This);
    *pbstrDescription = SysAllocString(This->desc);
    return S_OK;
}

static HRESULT WINAPI ScriptError_get_HelpFile(IScriptError *iface, BSTR *pbstrHelpFile)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, pbstrHelpFile);

    fill_error_info(This);
    *pbstrHelpFile = SysAllocString(This->help_file);
    return S_OK;
}

static HRESULT WINAPI ScriptError_get_HelpContext(IScriptError *iface, LONG *plHelpContext)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, plHelpContext);

    fill_error_info(This);
    *plHelpContext = This->help_context;
    return S_OK;
}

static HRESULT WINAPI ScriptError_get_Text(IScriptError *iface, BSTR *pbstrText)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, pbstrText);

    fill_error_text(This);
    *pbstrText = SysAllocString(This->text);
    return S_OK;
}

static HRESULT WINAPI ScriptError_get_Line(IScriptError *iface, LONG *plLine)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, plLine);

    fill_error_pos(This);
    *plLine = This->line;
    return S_OK;
}

static HRESULT WINAPI ScriptError_get_Column(IScriptError *iface, LONG *plColumn)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->(%p)\n", This, plColumn);

    fill_error_pos(This);
    *plColumn = This->column;
    return S_OK;
}

static HRESULT WINAPI ScriptError_Clear(IScriptError *iface)
{
    ScriptError *This = impl_from_IScriptError(iface);

    TRACE("(%p)->()\n", This);

    if (This->object)
    {
        IActiveScriptError_Release(This->object);
        This->object = NULL;
    }
    SysFreeString(This->text);
    SysFreeString(This->source);
    SysFreeString(This->desc);
    SysFreeString(This->help_file);

    This->number = 0;
    This->text = NULL;
    This->source = NULL;
    This->desc = NULL;
    This->help_file = NULL;
    This->help_context = 0;
    This->line = 0;
    This->column = 0;

    This->info_filled = FALSE;
    This->text_filled = FALSE;
    This->pos_filled = FALSE;
    return S_OK;
}

static const IScriptErrorVtbl ScriptErrorVtbl = {
    ScriptError_QueryInterface,
    ScriptError_AddRef,
    ScriptError_Release,
    ScriptError_GetTypeInfoCount,
    ScriptError_GetTypeInfo,
    ScriptError_GetIDsOfNames,
    ScriptError_Invoke,
    ScriptError_get_Number,
    ScriptError_get_Source,
    ScriptError_get_Description,
    ScriptError_get_HelpFile,
    ScriptError_get_HelpContext,
    ScriptError_get_Text,
    ScriptError_get_Line,
    ScriptError_get_Column,
    ScriptError_Clear
};

static HRESULT set_safety_opts(IActiveScript *script, VARIANT_BOOL use_safe_subset)
{
    IObjectSafety *objsafety;
    HRESULT hr;

    hr = IActiveScript_QueryInterface(script, &IID_IObjectSafety, (void**)&objsafety);
    if (FAILED(hr)) {
        FIXME("Could not get IObjectSafety, %#lx\n", hr);
        return hr;
    }

    hr = IObjectSafety_SetInterfaceSafetyOptions(objsafety, &IID_IActiveScriptParse, INTERFACESAFE_FOR_UNTRUSTED_DATA,
                                                 use_safe_subset ? INTERFACESAFE_FOR_UNTRUSTED_DATA : 0);
    IObjectSafety_Release(objsafety);
    if (FAILED(hr)) {
        FIXME("SetInterfaceSafetyOptions failed, %#lx\n", hr);
        return hr;
    }

    return hr;
}

static HRESULT init_script_host(ScriptControl *control, const CLSID *clsid, ScriptHost **ret)
{
    ScriptHost *host;
    HRESULT hr;

    *ret = NULL;

    host = malloc(sizeof(*host));
    if (!host)
        return E_OUTOFMEMORY;

    host->IActiveScriptSite_iface.lpVtbl = &ActiveScriptSiteVtbl;
    host->IActiveScriptSiteWindow_iface.lpVtbl = &ActiveScriptSiteWindowVtbl;
    host->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    host->ref = 1;
    host->script = NULL;
    host->parse = NULL;
    host->clsid = *clsid;
    host->module_count = 1;
    list_init(&host->named_items);

    hr = CoCreateInstance(&host->clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&host->script);
    if (FAILED(hr)) {
        WARN("Failed to create an instance for %s, %#lx\n", debugstr_guid(clsid), hr);
        goto failed;
    }

    hr = set_safety_opts(host->script, control->use_safe_subset);
    if (FAILED(hr)) goto failed;

    hr = IActiveScript_SetScriptSite(host->script, &host->IActiveScriptSite_iface);
    if (FAILED(hr)) {
        WARN("SetScriptSite failed, %#lx\n", hr);
        goto failed;
    }

    hr = IActiveScript_QueryInterface(host->script, &IID_IActiveScriptParse, (void**)&host->parse);
    if (FAILED(hr)) {
        WARN("Failed to get IActiveScriptParse, %#lx\n", hr);
        goto failed;
    }

    hr = IActiveScriptParse_InitNew(host->parse);
    if (FAILED(hr)) {
        WARN("InitNew failed, %#lx\n", hr);
        goto failed;
    }
    host->script_state = SCRIPTSTATE_INITIALIZED;
    host->site_hwnd = control->allow_ui ? control->site_hwnd : ((HWND)-1);
    host->error = control->error;
    IScriptError_AddRef(&host->error->IScriptError_iface);

    *ret = host;
    return S_OK;

failed:
    detach_script_host(host);
    return hr;
}

static HRESULT WINAPI ScriptControl_QueryInterface(IScriptControl *iface, REFIID riid, void **ppv)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IScriptControl_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IScriptControl_iface;
    }else if(IsEqualGUID(&IID_IScriptControl, riid)) {
        TRACE("(%p)->(IID_IScriptControl %p)\n", This, ppv);
        *ppv = &This->IScriptControl_iface;
    }else if(IsEqualGUID(&IID_IOleObject, riid)) {
        TRACE("(%p)->(IID_IOleObject %p)\n", This, ppv);
        *ppv = &This->IOleObject_iface;
    }else if(IsEqualGUID(&IID_IPersistStreamInit, riid)) {
        TRACE("(%p)->(IID_IPersistStreamInit %p)\n", This, ppv);
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(&IID_IPersist, riid)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(&IID_IOleControl, riid)) {
        TRACE("(%p)->(IID_IOleControl %p)\n", This, ppv);
        *ppv = &This->IOleControl_iface;
    }else if(IsEqualGUID(&IID_IQuickActivate, riid)) {
        TRACE("(%p)->(IID_IQuickActivate %p)\n", This, ppv);
        *ppv = &This->IQuickActivate_iface;
    }else if(IsEqualGUID(&IID_IViewObject, riid)) {
        TRACE("(%p)->(IID_IViewObject %p)\n", This, ppv);
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IViewObject2, riid)) {
        TRACE("(%p)->(IID_IViewObject2 %p)\n", This, ppv);
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IViewObjectEx, riid)) {
        TRACE("(%p)->(IID_IViewObjectEx %p)\n", This, ppv);
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IPointerInactive, riid)) {
        TRACE("(%p)->(IID_IPointerInactive %p)\n", This, ppv);
        *ppv = &This->IPointerInactive_iface;
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = &This->IConnectionPointContainer_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptControl_AddRef(IScriptControl *iface)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ScriptControl_Release(IScriptControl *iface)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if (This->site)
            IOleClientSite_Release(This->site);
        if (This->host)
        {
            release_modules(This, FALSE);
            IActiveScriptSite_Release(&This->host->IActiveScriptSite_iface);
        }
        IScriptError_Release(&This->error->IScriptError_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ScriptControl_GetTypeInfoCount(IScriptControl *iface, UINT *pctinfo)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptControl_GetTypeInfo(IScriptControl *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IScriptControl_tid, ppTInfo);
}

static HRESULT WINAPI ScriptControl_GetIDsOfNames(IScriptControl *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hres = get_typeinfo(IScriptControl_tid, &typeinfo);
    if(SUCCEEDED(hres)) {
        hres = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hres;
}

static HRESULT WINAPI ScriptControl_Invoke(IScriptControl *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hres = get_typeinfo(IScriptControl_tid, &typeinfo);
    if(SUCCEEDED(hres)) {
        hres = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hres;
}

static HRESULT WINAPI ScriptControl_get_Language(IScriptControl *iface, BSTR *p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    LPOLESTR progidW;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_POINTER;

    *p = NULL;

    if (!This->host)
        return S_OK;

    hr = ProgIDFromCLSID(&This->host->clsid, &progidW);
    if (FAILED(hr))
        return hr;

    *p = SysAllocString(progidW);
    CoTaskMemFree(progidW);
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI ScriptControl_put_Language(IScriptControl *iface, BSTR language)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    CLSID clsid;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(language));

    if (language && FAILED(CLSIDFromProgID(language, &clsid)))
        return CTL_E_INVALIDPROPERTYVALUE;

    if (This->host) {
        release_modules(This, TRUE);
        IActiveScriptSite_Release(&This->host->IActiveScriptSite_iface);
        This->host = NULL;
    }

    if (!language)
        return S_OK;

    hres = init_script_host(This, &clsid, &This->host);
    if (FAILED(hres))
        return hres;

    /* Alloc global module */
    This->modules = calloc(1, sizeof(*This->modules));
    if (This->modules) {
        This->modules[0] = create_module(This->host, NULL);
        if (!This->modules[0]) {
            free(This->modules);
            This->modules = NULL;
            hres = E_OUTOFMEMORY;
        }
    }
    else
        hres = E_OUTOFMEMORY;

    if (FAILED(hres)) {
        detach_script_host(This->host);
        This->host = NULL;
    }
    return hres;
}

static HRESULT WINAPI ScriptControl_get_State(IScriptControl *iface, ScriptControlStates *p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    SCRIPTSTATE state;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(!This->host)
        return E_FAIL;

    hres = IActiveScript_GetScriptState(This->host->script, &state);
    if (FAILED(hres)) return hres;

    switch (state)
    {
    case SCRIPTSTATE_INITIALIZED:
    case SCRIPTSTATE_STARTED:
        *p = Initialized;
        break;
    case SCRIPTSTATE_CONNECTED:
        *p = Connected;
        break;
    default:
        WARN("unexpected state %d\n", state);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI ScriptControl_put_State(IScriptControl *iface, ScriptControlStates state)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    TRACE("(%p)->(%x)\n", This, state);

    if(!This->host)
        return E_FAIL;

    if(state != Initialized && state != Connected)
        return CTL_E_INVALIDPROPERTYVALUE;

    return IActiveScript_SetScriptState(This->host->script, state == Connected ? SCRIPTSTATE_CONNECTED : SCRIPTSTATE_STARTED);
}

static HRESULT WINAPI ScriptControl_put_SitehWnd(IScriptControl *iface, LONG hwnd)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%lx)\n", This, hwnd);

    if (hwnd && !IsWindow(LongToHandle(hwnd)))
        return CTL_E_INVALIDPROPERTYVALUE;

    This->site_hwnd = LongToHandle(hwnd);
    if (This->host)
        This->host->site_hwnd = This->allow_ui ? This->site_hwnd : ((HWND)-1);
    return S_OK;
}

static HRESULT WINAPI ScriptControl_get_SitehWnd(IScriptControl *iface, LONG *p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p) return E_POINTER;

    *p = HandleToLong(This->site_hwnd);
    return S_OK;
}

static HRESULT WINAPI ScriptControl_get_Timeout(IScriptControl *iface, LONG *p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_POINTER;

    *p = This->timeout;
    return S_OK;
}

static HRESULT WINAPI ScriptControl_put_Timeout(IScriptControl *iface, LONG timeout)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%ld)\n", This, timeout);

    if (timeout < -1)
        return CTL_E_INVALIDPROPERTYVALUE;

    if (timeout != -1)
        FIXME("execution timeout ignored\n");

    This->timeout = timeout;
    return S_OK;
}

static HRESULT WINAPI ScriptControl_get_AllowUI(IScriptControl *iface, VARIANT_BOOL *p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    *p = This->allow_ui;
    return S_OK;
}

static HRESULT WINAPI ScriptControl_put_AllowUI(IScriptControl *iface, VARIANT_BOOL allow_ui)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    TRACE("(%p)->(%x)\n", This, allow_ui);

    This->allow_ui = allow_ui;
    if (This->host)
        This->host->site_hwnd = allow_ui ? This->site_hwnd : ((HWND)-1);
    return S_OK;
}

static HRESULT WINAPI ScriptControl_get_UseSafeSubset(IScriptControl *iface, VARIANT_BOOL *p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    *p = This->use_safe_subset;
    return S_OK;
}

static HRESULT WINAPI ScriptControl_put_UseSafeSubset(IScriptControl *iface, VARIANT_BOOL use_safe_subset)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    TRACE("(%p)->(%x)\n", This, use_safe_subset);

    if (This->host && This->use_safe_subset != use_safe_subset)
        set_safety_opts(This->host->script, use_safe_subset);

    This->use_safe_subset = use_safe_subset;
    return S_OK;
}

static HRESULT WINAPI ScriptControl_get_Modules(IScriptControl *iface, IScriptModuleCollection **p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!This->host) return E_FAIL;

    *p = &This->IScriptModuleCollection_iface;
    IScriptControl_AddRef(iface);
    return S_OK;
}

static HRESULT WINAPI ScriptControl_get_Error(IScriptControl *iface, IScriptError **p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p) return E_POINTER;

    *p = &This->error->IScriptError_iface;
    IScriptError_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI ScriptControl_get_CodeObject(IScriptControl *iface, IDispatch **p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!This->host) return E_FAIL;

    return IScriptModule_get_CodeObject(&This->modules[0]->IScriptModule_iface, p);
}

static HRESULT WINAPI ScriptControl_get_Procedures(IScriptControl *iface, IScriptProcedureCollection **p)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!This->host) return E_FAIL;

    return IScriptModule_get_Procedures(&This->modules[0]->IScriptModule_iface, p);
}

static HRESULT WINAPI ScriptControl__AboutBox(IScriptControl *iface)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ScriptControl_AddObject(IScriptControl *iface, BSTR name, IDispatch *object, VARIANT_BOOL add_members)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    DWORD flags = SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE;

    TRACE("(%p)->(%s %p %x)\n", This, debugstr_w(name), object, add_members);

    if (!object)
        return E_INVALIDARG;

    if (!This->host)
        return E_FAIL;

    if (add_members)
        flags |= SCRIPTITEM_GLOBALMEMBERS;
    return add_script_object(This->host, name, object, flags);
}

static HRESULT WINAPI ScriptControl_Reset(IScriptControl *iface)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)\n", This);

    if (!This->host)
        return E_FAIL;

    clear_named_items(This->host);
    return set_script_state(This->host, SCRIPTSTATE_INITIALIZED);
}

static HRESULT WINAPI ScriptControl_AddCode(IScriptControl *iface, BSTR code)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%s).\n", This, debugstr_w(code));

    if (!This->host)
        return E_FAIL;

    return parse_script_text(This->modules[0], code, SCRIPTTEXT_ISVISIBLE, NULL);
}

static HRESULT WINAPI ScriptControl_Eval(IScriptControl *iface, BSTR expression, VARIANT *res)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%s, %p).\n", This, debugstr_w(expression), res);

    if (!res)
        return E_POINTER;
    V_VT(res) = VT_EMPTY;
    if (!This->host)
        return E_FAIL;

    return parse_script_text(This->modules[0], expression, SCRIPTTEXT_ISEXPRESSION, res);
}

static HRESULT WINAPI ScriptControl_ExecuteStatement(IScriptControl *iface, BSTR statement)
{
    ScriptControl *This = impl_from_IScriptControl(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(statement));

    if (!This->host)
        return E_FAIL;

    return parse_script_text(This->modules[0], statement, 0, NULL);
}

static HRESULT WINAPI ScriptControl_Run(IScriptControl *iface, BSTR procedure_name, SAFEARRAY **parameters, VARIANT *res)
{
    ScriptControl *This = impl_from_IScriptControl(iface);
    SAFEARRAY *sa;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(procedure_name), parameters, res);

    if (!parameters || !res) return E_POINTER;
    if (!(sa = *parameters)) return E_POINTER;

    V_VT(res) = VT_EMPTY;
    if (sa->cDims == 0) return DISP_E_BADINDEX;
    if (!(sa->fFeatures & FADF_VARIANT)) return DISP_E_BADVARTYPE;
    if (!This->host) return E_FAIL;

    return run_procedure(This->modules[0], procedure_name, sa, res);
}

static const IScriptControlVtbl ScriptControlVtbl = {
    ScriptControl_QueryInterface,
    ScriptControl_AddRef,
    ScriptControl_Release,
    ScriptControl_GetTypeInfoCount,
    ScriptControl_GetTypeInfo,
    ScriptControl_GetIDsOfNames,
    ScriptControl_Invoke,
    ScriptControl_get_Language,
    ScriptControl_put_Language,
    ScriptControl_get_State,
    ScriptControl_put_State,
    ScriptControl_put_SitehWnd,
    ScriptControl_get_SitehWnd,
    ScriptControl_get_Timeout,
    ScriptControl_put_Timeout,
    ScriptControl_get_AllowUI,
    ScriptControl_put_AllowUI,
    ScriptControl_get_UseSafeSubset,
    ScriptControl_put_UseSafeSubset,
    ScriptControl_get_Modules,
    ScriptControl_get_Error,
    ScriptControl_get_CodeObject,
    ScriptControl_get_Procedures,
    ScriptControl__AboutBox,
    ScriptControl_AddObject,
    ScriptControl_Reset,
    ScriptControl_AddCode,
    ScriptControl_Eval,
    ScriptControl_ExecuteStatement,
    ScriptControl_Run
};

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **obj)
{
    ScriptControl *This = impl_from_IOleObject(iface);
    return IScriptControl_QueryInterface(&This->IScriptControl_iface, riid, obj);
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    ScriptControl *This = impl_from_IOleObject(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    ScriptControl *This = impl_from_IOleObject(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, IOleClientSite *site)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, site);

    if (This->site)
        IOleClientSite_Release(This->site);

    if ((This->site = site))
        IOleClientSite_AddRef(site);

    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, IOleClientSite **site)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, site);

    if (!site)
        return E_POINTER;

    if ((*site = This->site))
        IOleClientSite_AddRef(*site);

    return S_OK;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR containerapp, LPCOLESTR containerobj)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%s %s)\n", This, debugstr_w(containerapp), debugstr_w(containerobj));

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD save)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld)\n", This, save);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD which, IMoniker *moniker)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld %p)\n", This, which, moniker);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD assign, DWORD which, IMoniker **moniker)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld %ld %p)\n", This, assign, which, moniker);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *dataobj, BOOL creation,
    DWORD reserved)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%p %d %ld)\n", This, dataobj, creation, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD reserved, IDataObject **dataobj)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld %p)\n", This, reserved, dataobj);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG verb, LPMSG msg, IOleClientSite *active_site,
    LONG index, HWND hwndParent, LPCRECT rect)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld %p %p %ld %p %p)\n", This, verb, msg, active_site, index, hwndParent, rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **enumoleverb)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%p)\n", This, enumoleverb);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *clsid)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%p)\n", This, clsid);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD form_of_type, LPOLESTR *usertype)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld %p)\n", This, form_of_type, usertype);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD aspect, SIZEL *size)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld %p)\n", This, aspect, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD aspect, SIZEL *size)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, aspect, size);

    if (aspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    *size = This->extent;
    return S_OK;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *sink, DWORD *connection)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%p %p)\n", This, sink, connection);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD connection)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%ld)\n", This, connection);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **enumadvise)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%p)\n", This, enumadvise);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD aspect, DWORD *status)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, aspect, status);

    return OleRegGetMiscStatus(&CLSID_ScriptControl, aspect, status);
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *logpal)
{
    ScriptControl *This = impl_from_IOleObject(iface);

    FIXME("(%p)->(%p)\n", This, logpal);

    return E_NOTIMPL;
}

static const IOleObjectVtbl OleObjectVtbl = {
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface, REFIID riid, void **obj)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);
    return IScriptControl_QueryInterface(&This->IScriptControl_iface, riid, obj);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *clsid)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);

    FIXME("(%p)->(%p)\n", This, clsid);

    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, IStream *stream)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);

    FIXME("(%p)->(%p)\n", This, stream);

    return S_OK;
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, IStream *stream, BOOL clear_dirty)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);

    FIXME("(%p)->(%p %d)\n", This, stream, clear_dirty);

    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface, ULARGE_INTEGER *size)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);

    FIXME("(%p)->(%p)\n", This, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    ScriptControl *This = impl_from_IPersistStreamInit(iface);

    FIXME("(%p)\n", This);

    return S_OK;
}

static const IPersistStreamInitVtbl PersistStreamInitVtbl = {
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

static HRESULT WINAPI OleControl_QueryInterface(IOleControl *iface, REFIID riid, void **obj)
{
    ScriptControl *This = impl_from_IOleControl(iface);
    return IScriptControl_QueryInterface(&This->IScriptControl_iface, riid, obj);
}

static ULONG WINAPI OleControl_AddRef(IOleControl *iface)
{
    ScriptControl *This = impl_from_IOleControl(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI OleControl_Release(IOleControl *iface)
{
    ScriptControl *This = impl_from_IOleControl(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI OleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *info)
{
    ScriptControl *This = impl_from_IOleControl(iface);

    TRACE("(%p)->(%p)\n", This, info);

    if (!info)
        return E_POINTER;

    info->hAccel = NULL;
    info->cAccel = 0;

    return S_OK;
}

static HRESULT WINAPI OleControl_OnMnemonic(IOleControl *iface, MSG *msg)
{
    ScriptControl *This = impl_from_IOleControl(iface);

    FIXME("(%p)->(%p)\n", This, msg);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispid)
{
    ScriptControl *This = impl_from_IOleControl(iface);

    FIXME("(%p)->(%#lx)\n", This, dispid);

    return S_OK;
}

static HRESULT WINAPI OleControl_FreezeEvents(IOleControl *iface, BOOL freeze)
{
    ScriptControl *This = impl_from_IOleControl(iface);

    FIXME("(%p)->(%d)\n", This, freeze);

    return S_OK;
}

static const IOleControlVtbl OleControlVtbl = {
    OleControl_QueryInterface,
    OleControl_AddRef,
    OleControl_Release,
    OleControl_GetControlInfo,
    OleControl_OnMnemonic,
    OleControl_OnAmbientPropertyChange,
    OleControl_FreezeEvents
};

static HRESULT WINAPI QuickActivate_QueryInterface(IQuickActivate *iface, REFIID riid, void **obj)
{
    ScriptControl *This = impl_from_IQuickActivate(iface);
    return IScriptControl_QueryInterface(&This->IScriptControl_iface, riid, obj);
}

static ULONG WINAPI QuickActivate_AddRef(IQuickActivate *iface)
{
    ScriptControl *This = impl_from_IQuickActivate(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI QuickActivate_Release(IQuickActivate *iface)
{
    ScriptControl *This = impl_from_IQuickActivate(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI QuickActivate_QuickActivate(IQuickActivate *iface, QACONTAINER *container, QACONTROL *control)
{
    ScriptControl *This = impl_from_IQuickActivate(iface);

    FIXME("(%p)->(%p %p)\n", This, container, control);

    return E_NOTIMPL;
}

static HRESULT WINAPI QuickActivate_SetContentExtent(IQuickActivate *iface, SIZEL *size)
{
    ScriptControl *This = impl_from_IQuickActivate(iface);

    FIXME("(%p)->(%p)\n", This, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI QuickActivate_GetContentExtent(IQuickActivate *iface, SIZEL *size)
{
    ScriptControl *This = impl_from_IQuickActivate(iface);

    FIXME("(%p)->(%p)\n", This, size);

    return E_NOTIMPL;
}

static const IQuickActivateVtbl QuickActivateVtbl = {
    QuickActivate_QueryInterface,
    QuickActivate_AddRef,
    QuickActivate_Release,
    QuickActivate_QuickActivate,
    QuickActivate_SetContentExtent,
    QuickActivate_GetContentExtent
};

static HRESULT WINAPI ViewObject_QueryInterface(IViewObjectEx *iface, REFIID riid, void **obj)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);
    return IScriptControl_QueryInterface(&This->IScriptControl_iface, riid, obj);
}

static ULONG WINAPI ViewObject_AddRef(IViewObjectEx *iface)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI ViewObject_Release(IViewObjectEx *iface)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI ViewObject_Draw(IViewObjectEx *iface, DWORD drawaspect, LONG index, void *aspect,
    DVTARGETDEVICE *device, HDC target_dev, HDC hdc_draw, const RECTL *bounds, const RECTL *win_bounds,
    BOOL (STDMETHODCALLTYPE *fn_continue)(ULONG_PTR cont), ULONG_PTR cont)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p %p %p %p %p %p %Iu)\n", This, drawaspect, index, aspect, device, target_dev,
        hdc_draw, bounds, win_bounds, fn_continue, cont);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetColorSet(IViewObjectEx *iface, DWORD drawaspect, LONG index, void *aspect,
    DVTARGETDEVICE *device, HDC hic_target, LOGPALETTE **colorset)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p %p %p)\n", This, drawaspect, index, aspect, device, hic_target,
        colorset);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Freeze(IViewObjectEx *iface, DWORD drawaspect, LONG index, void *aspect,
    DWORD *freeze)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p)\n", This, drawaspect, index, aspect, freeze);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Unfreeze(IViewObjectEx *iface, DWORD freeze)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld)\n", This, freeze);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_SetAdvise(IViewObjectEx *iface, DWORD aspects, DWORD flags, IAdviseSink *sink)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    TRACE("(%p)->(%ld %#lx %p)\n", This, aspects, flags, sink);

    if (aspects != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    This->view_sink_flags = flags;
    if (This->view_sink)
        IAdviseSink_Release(This->view_sink);
    This->view_sink = sink;
    if (This->view_sink)
        IAdviseSink_AddRef(This->view_sink);

    return S_OK;
}

static HRESULT WINAPI ViewObject_GetAdvise(IViewObjectEx *iface, DWORD *aspects, DWORD *flags, IAdviseSink **sink)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    TRACE("(%p)->(%p %p %p)\n", This, aspects, flags, sink);

    if (aspects)
        *aspects = DVASPECT_CONTENT;
    if (flags)
        *flags = This->view_sink_flags;
    if (sink) {
        *sink = This->view_sink;
        if (*sink)
            IAdviseSink_AddRef(*sink);
    }

    return S_OK;
}

static HRESULT WINAPI ViewObject_GetExtent(IViewObjectEx *iface, DWORD draw_aspect, LONG index,
    DVTARGETDEVICE *device, SIZEL *size)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p)\n", This, draw_aspect, index, device, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetRect(IViewObjectEx *iface, DWORD aspect, RECTL *rect)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %p)\n", This, aspect, rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetViewStatus(IViewObjectEx *iface, DWORD *status)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    TRACE("(%p)->(%p)\n", This, status);

    *status = VIEWSTATUS_OPAQUE;
    return S_OK;
}

static HRESULT WINAPI ViewObject_QueryHitPoint(IViewObjectEx *iface, DWORD aspect, const RECT *bounds,
    POINT pt, LONG close_hint, DWORD *hit_result)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %s %s %ld %p)\n", This, aspect, wine_dbgstr_rect(bounds), wine_dbgstr_point(&pt), close_hint, hit_result);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_QueryHitRect(IViewObjectEx *iface, DWORD aspect, const RECT *bounds,
    const RECT *loc, LONG close_hint, DWORD *hit_result)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %s %s %ld %p)\n", This, aspect, wine_dbgstr_rect(bounds), wine_dbgstr_rect(loc), close_hint, hit_result);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetNaturalExtent(IViewObjectEx *iface, DWORD aspect, LONG index,
    DVTARGETDEVICE *device, HDC target_hdc, DVEXTENTINFO *extent_info, SIZEL *size)
{
    ScriptControl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p %p %p)\n", This, aspect, index, device, target_hdc, extent_info, size);

    return E_NOTIMPL;
}

static const IViewObjectExVtbl ViewObjectExVtbl = {
    ViewObject_QueryInterface,
    ViewObject_AddRef,
    ViewObject_Release,
    ViewObject_Draw,
    ViewObject_GetColorSet,
    ViewObject_Freeze,
    ViewObject_Unfreeze,
    ViewObject_SetAdvise,
    ViewObject_GetAdvise,
    ViewObject_GetExtent,
    ViewObject_GetRect,
    ViewObject_GetViewStatus,
    ViewObject_QueryHitPoint,
    ViewObject_QueryHitRect,
    ViewObject_GetNaturalExtent
};

static HRESULT WINAPI PointerInactive_QueryInterface(IPointerInactive *iface, REFIID riid, void **obj)
{
    ScriptControl *This = impl_from_IPointerInactive(iface);
    return IScriptControl_QueryInterface(&This->IScriptControl_iface, riid, obj);
}

static ULONG WINAPI PointerInactive_AddRef(IPointerInactive *iface)
{
    ScriptControl *This = impl_from_IPointerInactive(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI PointerInactive_Release(IPointerInactive *iface)
{
    ScriptControl *This = impl_from_IPointerInactive(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI PointerInactive_GetActivationPolicy(IPointerInactive *iface, DWORD *policy)
{
    ScriptControl *This = impl_from_IPointerInactive(iface);

    TRACE("(%p)->(%p)\n", This, policy);

    if (!policy)
        return E_POINTER;

    *policy = 0;
    return S_OK;
}

static HRESULT WINAPI PointerInactive_OnInactiveMouseMove(IPointerInactive *iface, const RECT *bounds,
    LONG x, LONG y, DWORD key_state)
{
    ScriptControl *This = impl_from_IPointerInactive(iface);

    FIXME("(%p)->(%s %ld %ld %#lx)\n", This, wine_dbgstr_rect(bounds), x, y, key_state);

    return E_NOTIMPL;
}

static HRESULT WINAPI PointerInactive_OnInactiveSetCursor(IPointerInactive *iface, const RECT *bounds,
    LONG x, LONG y, DWORD msg, BOOL set_always)
{
    ScriptControl *This = impl_from_IPointerInactive(iface);

    FIXME("(%p)->(%s %ld %ld %#lx %d)\n", This, wine_dbgstr_rect(bounds), x, y, msg, set_always);

    return E_NOTIMPL;
}

static const IPointerInactiveVtbl PointerInactiveVtbl = {
    PointerInactive_QueryInterface,
    PointerInactive_AddRef,
    PointerInactive_Release,
    PointerInactive_GetActivationPolicy,
    PointerInactive_OnInactiveMouseMove,
    PointerInactive_OnInactiveSetCursor
};

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface, REFIID riid, void **obj)
{
    ScriptControl *This = impl_from_IConnectionPointContainer(iface);
    return IScriptControl_QueryInterface(&This->IScriptControl_iface, riid, obj);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    ScriptControl *This = impl_from_IConnectionPointContainer(iface);
    return IScriptControl_AddRef(&This->IScriptControl_iface);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    ScriptControl *This = impl_from_IConnectionPointContainer(iface);
    return IScriptControl_Release(&This->IScriptControl_iface);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface, IEnumConnectionPoints **enum_points)
{
    ScriptControl *This = impl_from_IConnectionPointContainer(iface);

    FIXME("(%p)->(%p)\n", This, enum_points);

    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface, REFIID riid, IConnectionPoint **cp)
{
    ScriptControl *This = impl_from_IConnectionPointContainer(iface);
    ConnectionPoint *iter;

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), cp);

    *cp = NULL;

    for (iter = This->cp_list; iter; iter = iter->next) {
        if (IsEqualIID(iter->riid, riid))
            *cp = &iter->IConnectionPoint_iface;
    }

    if (*cp) {
        IConnectionPoint_AddRef(*cp);
        return S_OK;
    }

    FIXME("unsupported connection point %s\n", debugstr_guid(riid));
    return CONNECT_E_NOCONNECTION;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl = {
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface,
        REFIID riid, void **ppv)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IConnectionPoint_iface;
    }else if(IsEqualGUID(&IID_IConnectionPoint, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IConnectionPoint_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_AddRef(&This->control->IConnectionPointContainer_iface);
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_Release(&This->control->IConnectionPointContainer_iface);
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *iid)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, iid);

    *iid = *This->riid;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **container)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, container);

    if (!container)
        return E_POINTER;

    *container = &This->control->IConnectionPointContainer_iface;
    IConnectionPointContainer_AddRef(*container);

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *unk_sink,
        DWORD *cookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    FIXME("(%p)->(%p %p)\n", This, unk_sink, cookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD cookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    FIXME("(%p)->(%ld)\n", This, cookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
        IEnumConnections **ppEnum)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    FIXME("(%p)->(%p): stub\n", This, ppEnum);

    return E_NOTIMPL;
}

static const IConnectionPointVtbl ConnectionPointVtbl =
{
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

static void ConnectionPoint_Init(ConnectionPoint *cp, ScriptControl *sc, REFIID riid)
{
    cp->IConnectionPoint_iface.lpVtbl = &ConnectionPointVtbl;
    cp->control = sc;
    cp->riid = riid;

    cp->next = sc->cp_list;
    sc->cp_list = cp;
}

static HRESULT WINAPI ScriptControl_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    ScriptControl *script_control;
    DWORD dpi_x, dpi_y;
    HRESULT hres;
    HDC hdc;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    script_control = calloc(1, sizeof(*script_control));
    if(!script_control)
        return E_OUTOFMEMORY;

    script_control->error = calloc(1, sizeof(*script_control->error));
    if(!script_control->error)
    {
        free(script_control);
        return E_OUTOFMEMORY;
    }

    script_control->IScriptControl_iface.lpVtbl = &ScriptControlVtbl;
    script_control->IPersistStreamInit_iface.lpVtbl = &PersistStreamInitVtbl;
    script_control->IOleObject_iface.lpVtbl = &OleObjectVtbl;
    script_control->IOleControl_iface.lpVtbl = &OleControlVtbl;
    script_control->IQuickActivate_iface.lpVtbl = &QuickActivateVtbl;
    script_control->IViewObjectEx_iface.lpVtbl = &ViewObjectExVtbl;
    script_control->IPointerInactive_iface.lpVtbl = &PointerInactiveVtbl;
    script_control->IConnectionPointContainer_iface.lpVtbl = &ConnectionPointContainerVtbl;
    script_control->IScriptModuleCollection_iface.lpVtbl = &ScriptModuleCollectionVtbl;
    script_control->ref = 1;
    script_control->timeout = 10000;
    script_control->allow_ui = VARIANT_TRUE;
    script_control->use_safe_subset = VARIANT_FALSE;

    script_control->error->IScriptError_iface.lpVtbl = &ScriptErrorVtbl;
    script_control->error->ref = 1;

    ConnectionPoint_Init(&script_control->cp_scsource, script_control, &DIID_DScriptControlSource);
    ConnectionPoint_Init(&script_control->cp_propnotif, script_control, &IID_IPropertyNotifySink);

    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    script_control->extent.cx = MulDiv(38, 2540, dpi_x);
    script_control->extent.cy = MulDiv(38, 2540, dpi_y);

    hres = IScriptControl_QueryInterface(&script_control->IScriptControl_iface, riid, ppv);
    IScriptControl_Release(&script_control->IScriptControl_iface);
    return hres;
}

/******************************************************************
 *              DllMain (msscript.ocx.@)
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p %ld %p)\n", instance, reason, reserved);

    switch(reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        break;
    case DLL_PROCESS_DETACH:
        if(!reserved)
            release_typelib();
        break;
    }

    return TRUE;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl ScriptControlFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ScriptControl_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ScriptControlFactory = { &ScriptControlFactoryVtbl };

/***********************************************************************
 *		DllGetClassObject	(msscript.ocx.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_ScriptControl, rclsid)) {
        TRACE("(CLSID_ScriptControl %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&ScriptControlFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
