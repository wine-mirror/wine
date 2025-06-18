/*
 * Copyright 2017 Nikolay Sivov
 * Copyright 2019 Jacek Caban for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "olectl.h"
#include "rpcproxy.h"
#include "activscp.h"
#include "dispex.h"
#include "mshtmhst.h"

#include "initguid.h"
#include "scrobj.h"
#include "xmllite.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrobj);

#ifdef _WIN64

#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText

#else

#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText

#endif

static HINSTANCE scrobj_instance;

enum member_type
{
    MEMBER_METHOD,
    MEMBER_PROPERTY
};

#define PROP_GETTER   0x01
#define PROP_SETTER   0x02

struct scriptlet_member
{
    struct list entry;
    enum member_type type;
    WCHAR *name;
    union
    {
        struct list parameters;
        unsigned int flags;
    } u;
};

struct method_parameter
{
    struct list entry;
    WCHAR *name;
};

struct scriptlet_script
{
    struct list entry;
    WCHAR *language;
    WCHAR *body;
};

struct scriptlet_global
{
    IDispatchEx IDispatchEx_iface;
    LONG ref;
};

struct scriptlet_factory
{
    IClassFactory IClassFactory_iface;

    LONG ref;

    IXmlReader *xml_reader;
    IMoniker *moniker;

    BOOL have_registration;
    BOOL have_public;
    WCHAR *description;
    WCHAR *progid;
    WCHAR *versioned_progid;
    WCHAR *version;
    WCHAR *classid_str;
    CLSID classid;

    struct list hosts;
    struct list members;
    struct list scripts;
};

struct script_reference
{
    struct script_host *host;
    DISPID id;
};

struct object_member
{
    enum member_type type;
    BSTR name;
    union
    {
        struct script_reference method;
        struct
        {
            struct script_reference get;
            struct script_reference put;
        } property;
    } u;
};

struct scriptlet_instance
{
    IDispatchEx IDispatchEx_iface;
    LONG ref;
    struct list hosts;
    struct scriptlet_global *global;
    unsigned member_cnt;
    struct object_member *members;
};

struct script_host
{
    IActiveScriptSite IActiveScriptSite_iface;
    IActiveScriptSiteWindow IActiveScriptSiteWindow_iface;
    IServiceProvider IServiceProvider_iface;

    LONG ref;
    struct list entry;

    WCHAR *language;

    IActiveScript *active_script;
    IActiveScriptParse *parser;
    IDispatchEx *script_dispatch;
    struct scriptlet_instance *object;
    SCRIPTSTATE state;
    BOOL cloned;
};

typedef enum tid_t
{
    NULL_tid,
    IGenScriptletTLib_tid,
    LAST_tid
} tid_t;

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
    &IID_NULL,
    &IID_IGenScriptletTLib,
};

static HRESULT load_typelib(void)
{
    HRESULT hres;
    ITypeLib *tl;

    if (typelib)
        return S_OK;

    hres = LoadRegTypeLib(&LIBID_Scriptlet, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if (FAILED(hres))
    {
        ERR("LoadRegTypeLib failed: %08lx\n", hres);
        return hres;
    }

    if (InterlockedCompareExchangePointer((void **)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hres;
}

static HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (FAILED(hres = load_typelib()))
        return hres;

    if (!typeinfos[tid])
    {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if (FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08lx\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if (InterlockedCompareExchangePointer((void **)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    ITypeInfo_AddRef(typeinfos[tid]);
    return S_OK;
}

static void release_typelib(void)
{
    unsigned i;

    if (!typelib)
        return;

    for (i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static inline struct scriptlet_global *global_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, struct scriptlet_global, IDispatchEx_iface);
}

static HRESULT WINAPI global_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }
    else if (IsEqualGUID(&IID_IDispatch, riid))
    {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }
    else if (IsEqualGUID(&IID_IDispatchEx, riid))
    {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }
    else
    {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI global_AddRef(IDispatchEx *iface)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI global_Release(IDispatchEx *iface)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) free(This);

    return ref;
}

static HRESULT WINAPI global_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    UINT i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    for(i=0; i < cNames; i++)
    {
        hres = IDispatchEx_GetDispID(&This->IDispatchEx_iface, rgszNames[i], 0, rgDispId + i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI global_Invoke(IDispatchEx *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return IDispatchEx_InvokeEx(&This->IDispatchEx_iface, dispIdMember, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI global_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%s %lx %p)\n", This, debugstr_w(bstrName), grfdex, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%s %lx)\n", This, debugstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx %lx %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx %p)\n", This, id, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx %lx %p)\n", This, grfdex, id, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI global_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    struct scriptlet_global *This = global_from_IDispatchEx(iface);
    FIXME("(%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

static IDispatchExVtbl global_vtbl = {
    global_QueryInterface,
    global_AddRef,
    global_Release,
    global_GetTypeInfoCount,
    global_GetTypeInfo,
    global_GetIDsOfNames,
    global_Invoke,
    global_GetDispID,
    global_InvokeEx,
    global_DeleteMemberByName,
    global_DeleteMemberByDispID,
    global_GetMemberProperties,
    global_GetMemberName,
    global_GetNextDispID,
    global_GetNameSpaceParent
};

static HRESULT set_script_state(struct script_host *host, SCRIPTSTATE state)
{
    HRESULT hres;
    if (state == host->state) return S_OK;
    hres = IActiveScript_SetScriptState(host->active_script, state);
    if (FAILED(hres)) return hres;
    host->state = state;
    return S_OK;
}

static inline struct script_host *impl_from_IActiveScriptSite(IActiveScriptSite *iface)
{
    return CONTAINING_RECORD(iface, struct script_host, IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }
    else if (IsEqualGUID(&IID_IActiveScriptSite, riid))
    {
        TRACE("(%p)->(IID_IActiveScriptSite %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }
    else if (IsEqualGUID(&IID_IActiveScriptSiteWindow, riid))
    {
        TRACE("(%p)->(IID_IActiveScriptSiteWindow %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteWindow_iface;
    }
    else if(IsEqualGUID(&IID_IServiceProvider, riid))
    {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }
    else
    {
        WARN("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        free(This->language);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *lcid)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p, %p)\n", This, lcid);

    *lcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR name, DWORD mask,
    IUnknown **unk, ITypeInfo **ti)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p, %s, %#lx, %p, %p)\n", This, debugstr_w(name), mask, unk, ti);

    if (mask != SCRIPTINFO_IUNKNOWN)
    {
        FIXME("mask %lx not supported\n", mask);
        return E_NOTIMPL;
    }

    if (wcscmp(name, L"scriptlet") && wcscmp(name, L"globals"))
    {
        FIXME("%s not supported\n", debugstr_w(name));
        return E_FAIL;
    }

    if (!This->object) return E_UNEXPECTED;

    *unk = (IUnknown *)&This->object->global->IDispatchEx_iface;
    IUnknown_AddRef(*unk);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *version)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p, %p)\n", This, version);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface, const VARIANT *result,
    const EXCEPINFO *ei)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p, %s, %p)\n", This, debugstr_variant(result), ei);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE state)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p, %d)\n", This, state);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *script_error)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p, %p)\n", This, script_error);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    struct script_host *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p)\n", This);
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

static inline struct script_host *impl_from_IActiveScriptSiteWindow(IActiveScriptSiteWindow *iface)
{
    return CONTAINING_RECORD(iface, struct script_host, IActiveScriptSiteWindow_iface);
}

static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface, REFIID riid, void **obj)
{
    struct script_host *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, obj);
}

static ULONG WINAPI ActiveScriptSiteWindow_AddRef(IActiveScriptSiteWindow *iface)
{
    struct script_host *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteWindow_Release(IActiveScriptSiteWindow *iface)
{
    struct script_host *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *hwnd)
{
    struct script_host *This = impl_from_IActiveScriptSiteWindow(iface);
    FIXME("(%p, %p)\n", This, hwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL enable)
{
    struct script_host *This = impl_from_IActiveScriptSiteWindow(iface);
    FIXME("(%p, %d)\n", This, enable);
    return E_NOTIMPL;
}

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

static inline struct script_host *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, struct script_host, IServiceProvider_iface);
}

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **obj)
{
    struct script_host *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, obj);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    struct script_host *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    struct script_host *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID service,
    REFIID riid, void **obj)
{
    struct script_host *This = impl_from_IServiceProvider(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(service), debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static struct script_host *find_script_host(struct list *hosts, const WCHAR *language)
{
    struct script_host *host;
    LIST_FOR_EACH_ENTRY(host, hosts, struct script_host, entry)
    {
        if (!wcscmp(host->language, language)) return host;
    }
    return NULL;
}

static HRESULT parse_scripts(struct scriptlet_factory *factory, struct list *hosts, BOOL start)
{
    DWORD parse_flags = SCRIPTTEXT_ISVISIBLE;
    struct scriptlet_script *script;
    struct script_host *host;
    HRESULT hres;

    if (!start) parse_flags |= SCRIPTITEM_ISPERSISTENT;

    LIST_FOR_EACH_ENTRY(script, &factory->scripts, struct scriptlet_script, entry)
    {
        host = find_script_host(hosts, script->language);

        if (start && host->state != SCRIPTSTATE_STARTED)
        {
            hres = set_script_state(host, SCRIPTSTATE_STARTED);
            if (FAILED(hres)) return hres;
        }

        if (host->cloned) continue;

        hres = IActiveScriptParse_ParseScriptText(host->parser, script->body, NULL, NULL, NULL, 0, 0 /* FIXME */,
                                                  parse_flags, NULL, NULL);
        if (FAILED(hres))
        {
            WARN("ParseScriptText failed: %08lx\n", hres);
            return hres;
        }
    }
    if (!start)
    {
        LIST_FOR_EACH_ENTRY(host, hosts, struct script_host, entry)
        {
            if (host->state != SCRIPTSTATE_UNINITIALIZED)
                set_script_state(host, SCRIPTSTATE_UNINITIALIZED);
        }
    }
    return S_OK;
}

static HRESULT lookup_script_properties(struct scriptlet_instance *object, BSTR name, struct script_reference *ret)
{
    struct script_host *host;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(host, &object->hosts, struct script_host, entry)
    {
        hres = IDispatchEx_GetDispID(host->script_dispatch, name, 0, &ret->id);
        if (FAILED(hres)) continue;
        ret->host = host;
        return S_OK;
    }

    FIXME("Could not find %s in scripts\n", debugstr_w(name));
    return E_FAIL;
}

static HRESULT lookup_object_properties(struct scriptlet_instance *object, struct scriptlet_factory *factory)
{
    struct scriptlet_member *member;
    unsigned i = 0, member_cnt = 0;
    size_t len;
    BSTR name;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(member, &factory->members, struct scriptlet_member, entry)
        member_cnt++;

    if (!member_cnt) return S_OK;

    if (!(object->members = calloc(member_cnt, sizeof(*object->members))))
        return E_OUTOFMEMORY;
    object->member_cnt = member_cnt;

    LIST_FOR_EACH_ENTRY(member, &factory->members, struct scriptlet_member, entry)
    {
        object->members[i].type = member->type;
        if (!(object->members[i].name = SysAllocString(member->name))) return E_OUTOFMEMORY;
        switch (member->type)
        {
        case MEMBER_METHOD:
            hres = lookup_script_properties(object, object->members[i].name, &object->members[i].u.method);
            if (FAILED(hres)) return hres;
            break;
        case MEMBER_PROPERTY:
            len = wcslen(member->name);
            if (!(name = SysAllocStringLen(NULL, len + 4))) return E_OUTOFMEMORY;
            wcscpy(name, L"get_");
            wcscat(name, member->name);
            hres = lookup_script_properties(object, name, &object->members[i].u.property.get);
            if (SUCCEEDED(hres))
            {
                memcpy(name, L"put", 3 * sizeof(WCHAR));
                hres = lookup_script_properties(object, name, &object->members[i].u.property.put);
            }
            SysFreeString(name);
            if (FAILED(hres)) return hres;
            break;
        }
        i++;
    }

    return S_OK;
}

static HRESULT init_script_host(struct script_host *host, IActiveScript *clone)
{
    HRESULT hres;

    if (!clone)
    {
        IClassFactoryEx *factory_ex;
        IClassFactory *factory;
        IUnknown *unk;
        CLSID clsid;

        if (FAILED(hres = CLSIDFromProgID(host->language, &clsid)))
        {
            WARN("Could not find script engine for %s\n", debugstr_w(host->language));
            return hres;
        }

        hres = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER, NULL,
                                &IID_IClassFactory, (void**)&factory);
        if (FAILED(hres)) return hres;

        hres = IClassFactory_QueryInterface(factory, &IID_IClassFactoryEx, (void**)&factory_ex);
        if (SUCCEEDED(hres))
        {
            FIXME("Use IClassFactoryEx\n");
            IClassFactoryEx_Release(factory_ex);
        }

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IUnknown, (void**)&unk);
        IClassFactory_Release(factory);
        if (FAILED(hres)) return hres;

        hres = IUnknown_QueryInterface(unk, &IID_IActiveScript, (void**)&host->active_script);
        IUnknown_Release(unk);
        if (FAILED(hres)) return hres;
    }
    else
    {
        IActiveScript_AddRef(clone);
        host->active_script = clone;
        host->cloned = TRUE;
    }

    hres = IActiveScript_QueryInterface(host->active_script, &IID_IActiveScriptParse, (void**)&host->parser);
    if (FAILED(hres)) return hres;

    if (!clone)
    {
        hres = IActiveScriptParse_InitNew(host->parser);
        if (FAILED(hres))
        {
            IActiveScriptParse_Release(host->parser);
            host->parser = NULL;
            return hres;
        }
    }

    return IActiveScript_SetScriptSite(host->active_script, &host->IActiveScriptSite_iface);
}

static HRESULT create_script_host(const WCHAR *language, IActiveScript *origin_script, struct list *hosts, struct script_host **ret)
{
    IActiveScript *clone = NULL;
    struct script_host *host;
    HRESULT hres;

    if (!(host = calloc(1, sizeof(*host))))
        return E_OUTOFMEMORY;

    host->IActiveScriptSite_iface.lpVtbl = &ActiveScriptSiteVtbl;
    host->IActiveScriptSiteWindow_iface.lpVtbl = &ActiveScriptSiteWindowVtbl;
    host->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    host->ref = 1;
    host->state = SCRIPTSTATE_CLOSED;

    if (!(host->language = wcsdup(language)))
    {
        IActiveScriptSite_Release(&host->IActiveScriptSite_iface);
        return E_OUTOFMEMORY;
    }

    if (origin_script)
    {
        hres = IActiveScript_Clone(origin_script, &clone);
        if (FAILED(hres)) clone = NULL;
    }

    list_add_tail(hosts, &host->entry);
    hres = init_script_host(host, clone);
    if (clone) IActiveScript_Release(clone);
    if (FAILED(hres)) return hres;
    if (ret) *ret = host;
    return S_OK;
}

static void detach_script_hosts(struct list *hosts)
{
    while (!list_empty(hosts))
    {
        struct script_host *host = LIST_ENTRY(list_head(hosts), struct script_host, entry);
        if (host->state != SCRIPTSTATE_UNINITIALIZED) set_script_state(host, SCRIPTSTATE_UNINITIALIZED);
        list_remove(&host->entry);
        host->object = NULL;
        if (host->script_dispatch)
        {
            IDispatchEx_Release(host->script_dispatch);
            host->script_dispatch = NULL;
        }
        if (host->parser)
        {
            IActiveScript_Close(host->active_script);
            IActiveScriptParse_Release(host->parser);
            host->parser = NULL;
        }
        if (host->active_script)
        {
            IActiveScript_Release(host->active_script);
            host->active_script = NULL;
        }
        IActiveScriptSite_Release(&host->IActiveScriptSite_iface);
    }
}

static HRESULT create_scriptlet_hosts(struct scriptlet_factory *factory, struct list *hosts)
{
    struct scriptlet_script *script;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(script, &factory->scripts, struct scriptlet_script, entry)
    {
        if (find_script_host(hosts, script->language)) continue;
        hres = create_script_host(script->language, NULL, hosts, NULL);
        if (FAILED(hres))
        {
            detach_script_hosts(hosts);
            return hres;
        }
    }

    return S_OK;
}

static inline struct scriptlet_instance *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, struct scriptlet_instance, IDispatchEx_iface);
}

static HRESULT WINAPI scriptlet_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }
    else if (IsEqualGUID(&IID_IDispatch, riid))
    {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }
    else if (IsEqualGUID(&IID_IDispatchEx, riid))
    {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }
    else
    {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI scriptlet_AddRef(IDispatchEx *iface)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI scriptlet_Release(IDispatchEx *iface)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        unsigned i;
        for (i = 0; i < This->member_cnt; i++)
            SysFreeString(This->members[i].name);
        free(This->members);
        detach_script_hosts(&This->hosts);
        if (This->global) IDispatchEx_Release(&This->global->IDispatchEx_iface);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI scriptlet_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    UINT i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    for(i=0; i < cNames; i++)
    {
        hres = IDispatchEx_GetDispID(&This->IDispatchEx_iface, rgszNames[i], 0, rgDispId + i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI scriptlet_Invoke(IDispatchEx *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return IDispatchEx_InvokeEx(&This->IDispatchEx_iface, dispIdMember, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI scriptlet_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    unsigned i;

    TRACE("(%p)->(%s %lx %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if (grfdex & ~(fdexNameCaseInsensitive|fdexNameCaseSensitive))
        FIXME("Unsupported grfdex %lx\n", grfdex);

    for (i = 0; i < This->member_cnt; i++)
    {
        if (grfdex & fdexNameCaseInsensitive)
        {
            if (wcsicmp(This->members[i].name, bstrName)) continue;
        }
        else if (wcscmp(This->members[i].name, bstrName)) continue;
        *pid = i + 1;
        return S_OK;
    }

    WARN("Unknown property %s\n", debugstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI scriptlet_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *pdp,
        VARIANT *res, EXCEPINFO *pei, IServiceProvider *caller)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    struct object_member *member;

    TRACE("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, flags, pdp, res, pei, caller);

    if (id < 1 || id > This->member_cnt)
    {
        WARN("Unknown id %lxu\n", id);
        return DISP_E_MEMBERNOTFOUND;
    }
    member = &This->members[id - 1];

    switch (member->type)
    {
    case MEMBER_METHOD:
        if ((flags & ~DISPATCH_PROPERTYGET) != DISPATCH_METHOD)
        {
            FIXME("unsupported method flags %x\n", flags);
            return DISP_E_MEMBERNOTFOUND;
        }
        return IDispatchEx_InvokeEx(member->u.method.host->script_dispatch, member->u.method.id, lcid,
                                    DISPATCH_METHOD, pdp, res, pei, caller);
    case MEMBER_PROPERTY:
        if (flags & DISPATCH_PROPERTYGET)
        {
            if (!member->u.property.get.host)
            {
                FIXME("No %s getter\n", debugstr_w(member->name));
                return DISP_E_MEMBERNOTFOUND;
            }
            return IDispatchEx_InvokeEx(member->u.property.get.host->script_dispatch, member->u.property.get.id, lcid,
                                        DISPATCH_METHOD, pdp, res, pei, caller);
        }
        if (flags & DISPATCH_PROPERTYPUT)
        {
            DISPPARAMS dp;

            if (!member->u.property.put.host)
            {
                FIXME("No %s setter\n", debugstr_w(member->name));
                return DISP_E_MEMBERNOTFOUND;
            }
            if (pdp->cNamedArgs != 1 || pdp->rgdispidNamedArgs[0] != DISPID_PROPERTYPUT)
            {
                FIXME("no propput argument\n");
                return E_FAIL;
            }
            dp = *pdp;
            dp.cNamedArgs = 0;
            return IDispatchEx_InvokeEx(member->u.property.put.host->script_dispatch, member->u.property.put.id, lcid,
                                        DISPATCH_METHOD, &dp, res, pei, caller);
        }

        FIXME("unsupported flags %x\n", flags);
    }

    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI scriptlet_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%s %lx)\n", This, debugstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx %lx %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx %p)\n", This, id, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%lx %lx %p)\n", This, grfdex, id, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    struct scriptlet_instance *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

static IDispatchExVtbl DispatchExVtbl = {
    scriptlet_QueryInterface,
    scriptlet_AddRef,
    scriptlet_Release,
    scriptlet_GetTypeInfoCount,
    scriptlet_GetTypeInfo,
    scriptlet_GetIDsOfNames,
    scriptlet_Invoke,
    scriptlet_GetDispID,
    scriptlet_InvokeEx,
    scriptlet_DeleteMemberByName,
    scriptlet_DeleteMemberByDispID,
    scriptlet_GetMemberProperties,
    scriptlet_GetMemberName,
    scriptlet_GetNextDispID,
    scriptlet_GetNameSpaceParent
};

static HRESULT create_scriptlet_instance(struct scriptlet_factory *factory, IDispatchEx **disp)
{
    struct script_host *factory_host, *host;
    struct scriptlet_instance *obj;
    IDispatch *script_dispatch;
    HRESULT hres = S_OK;

    if (!(obj = calloc(1, sizeof(*obj))))
        return E_OUTOFMEMORY;

    obj->IDispatchEx_iface.lpVtbl = &DispatchExVtbl;
    obj->ref = 1;
    list_init(&obj->hosts);

    if (!(obj->global = malloc(sizeof(*obj->global))))
    {
        IDispatchEx_Release(&obj->IDispatchEx_iface);
        return E_OUTOFMEMORY;
    }
    obj->global->IDispatchEx_iface.lpVtbl = &global_vtbl;
    obj->global->ref = 1;

    LIST_FOR_EACH_ENTRY(factory_host, &factory->hosts, struct script_host, entry)
    {
        hres = create_script_host(factory_host->language, factory_host->active_script, &obj->hosts, &host);
        if (FAILED(hres)) break;
        host->object = obj;

        hres = IActiveScript_AddNamedItem(host->active_script, L"scriptlet",
                                          SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS);
        if (FAILED(hres)) break;

        hres = IActiveScript_AddNamedItem(host->active_script, L"globals", SCRIPTITEM_ISVISIBLE);
        if (FAILED(hres)) break;

        hres = IActiveScript_GetScriptDispatch(host->active_script, NULL, &script_dispatch);
        if (FAILED(hres)) return hres;

        hres = IDispatch_QueryInterface(script_dispatch, &IID_IDispatchEx, (void**)&host->script_dispatch);
        IDispatch_Release(script_dispatch);
        if (FAILED(hres))
        {
            FIXME("IDispatchEx not supported by script engine\n");
            return hres;
        }
    }

    if (SUCCEEDED(hres)) hres = parse_scripts(factory, &obj->hosts, TRUE);
    if (SUCCEEDED(hres)) hres = lookup_object_properties(obj, factory);
    if (FAILED(hres))
    {
        IDispatchEx_Release(&obj->IDispatchEx_iface);
        return hres;
    }

    *disp = &obj->IDispatchEx_iface;
    return S_OK;
}

static const char *debugstr_xml_name(struct scriptlet_factory *factory)
{
    const WCHAR *str;
    UINT len;
    HRESULT hres;
    hres = IXmlReader_GetLocalName(factory->xml_reader, &str, &len);
    if (FAILED(hres)) return "#err";
    return debugstr_wn(str, len);
}

static HRESULT next_xml_node(struct scriptlet_factory *factory, XmlNodeType *node_type)
{
    HRESULT hres;
    for (;;)
    {
        hres = IXmlReader_Read(factory->xml_reader, node_type);
        if (FAILED(hres)) break;
        if (*node_type == XmlNodeType_Whitespace) continue;
        if (*node_type == XmlNodeType_ProcessingInstruction)
        {
            FIXME("Ignoring processing instruction\n");
            continue;
        }
        break;
    }
    return hres;
}

static BOOL is_xml_name(struct scriptlet_factory *factory, const WCHAR *name)
{
    const WCHAR *qname;
    UINT len;
    HRESULT hres;
    hres = IXmlReader_GetQualifiedName(factory->xml_reader, &qname, &len);
    return hres == S_OK && len == wcslen(name) && !memcmp(qname, name, len * sizeof(WCHAR));
}

static HRESULT expect_end_element(struct scriptlet_factory *factory)
{
    XmlNodeType node_type;
    HRESULT hres;
    hres = next_xml_node(factory, &node_type);
    if (hres != S_OK || node_type != XmlNodeType_EndElement)
    {
        FIXME("Unexpected node %u %s\n", node_type, debugstr_xml_name(factory));
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT expect_no_attributes(struct scriptlet_factory *factory)
{
    UINT count;
    HRESULT hres;
    hres = IXmlReader_GetAttributeCount(factory->xml_reader, &count);
    if (FAILED(hres)) return hres;
    if (!count) return S_OK;
    FIXME("Unexpected attributes\n");
    return E_FAIL;
}

static BOOL is_case_xml_name(struct scriptlet_factory *factory, const WCHAR *name)
{
    const WCHAR *qname;
    UINT len;
    HRESULT hres;
    hres = IXmlReader_GetQualifiedName(factory->xml_reader, &qname, &len);
    return hres == S_OK && len == wcslen(name) && !memicmp(qname, name, len * sizeof(WCHAR));
}

static HRESULT read_xml_value(struct scriptlet_factory *factory, WCHAR **ret)
{
    const WCHAR *str;
    UINT len;
    HRESULT hres;
    hres = IXmlReader_GetValue(factory->xml_reader, &str, &len);
    if (FAILED(hres)) return hres;
    if (!(*ret = malloc((len + 1) * sizeof(WCHAR)))) return E_OUTOFMEMORY;
    memcpy(*ret, str, len * sizeof(WCHAR));
    (*ret)[len] = 0;
    return S_OK;
}

static HRESULT parse_scriptlet_registration(struct scriptlet_factory *factory)
{
    HRESULT hres;

    TRACE("\n");

    if (factory->have_registration)
    {
        FIXME("duplicated registration element\n");
        return E_FAIL;
    }
    factory->have_registration = TRUE;

    for (;;)
    {
        hres = IXmlReader_MoveToNextAttribute(factory->xml_reader);
        if (hres != S_OK) break;

        if (is_xml_name(factory, L"description"))
        {
            if (factory->description)
            {
                FIXME("Duplicated description\n");
                return E_FAIL;
            }
            hres = read_xml_value(factory, &factory->description);
            if (FAILED(hres)) return hres;
        }
        else if (is_case_xml_name(factory, L"progid"))
        {
            if (factory->progid)
            {
                FIXME("Duplicated progid\n");
                return E_FAIL;
            }
            hres = read_xml_value(factory, &factory->progid);
            if (FAILED(hres)) return hres;
        }
        else if (is_xml_name(factory, L"version"))
        {
            if (factory->version)
            {
                FIXME("Duplicated version\n");
                return E_FAIL;
            }
            hres = read_xml_value(factory, &factory->version);
            if (FAILED(hres)) return hres;
        }
        else if (is_xml_name(factory, L"classid"))
        {
            if (factory->classid_str)
            {
                FIXME("Duplicated classid attribute\n");
                return E_FAIL;
            }
            hres = read_xml_value(factory, &factory->classid_str);
            if (FAILED(hres)) return hres;
            hres = IIDFromString(factory->classid_str, &factory->classid);
            if (FAILED(hres))
            {
                FIXME("Invalid classid %s\n", debugstr_w(factory->classid_str));

                return E_FAIL;
            }
        }
        else
        {
            FIXME("Unexpected attribute\n");
            return E_NOTIMPL;
        }
    }

    if (!factory->progid && !factory->classid_str)
    {
        FIXME("Incomplet registration element\n");
        return E_FAIL;
    }

    if (factory->progid)
    {
        size_t progid_len = wcslen(factory->progid);
        size_t version_len = wcslen(factory->version);

        if (!(factory->versioned_progid = malloc((progid_len + version_len + 2) * sizeof(WCHAR))))
            return E_OUTOFMEMORY;

        memcpy(factory->versioned_progid, factory->progid, (progid_len + 1) * sizeof(WCHAR));
        if (version_len)
        {
            factory->versioned_progid[progid_len++] = '.';
            wcscpy(factory->versioned_progid + progid_len, factory->version);
        }
        else factory->versioned_progid[progid_len] = 0;
    }

    return expect_end_element(factory);
}

static HRESULT parse_scriptlet_public(struct scriptlet_factory *factory)
{
    struct scriptlet_member *member, *member_iter;
    enum member_type member_type;
    XmlNodeType node_type;
    BOOL empty;
    HRESULT hres;

    TRACE("\n");

    if (factory->have_public)
    {
        FIXME("duplicated public element\n");
        return E_FAIL;
    }
    factory->have_public = TRUE;

    for (;;)
    {
        hres = next_xml_node(factory, &node_type);
        if (FAILED(hres)) return hres;
        if (node_type == XmlNodeType_EndElement) break;
        if (node_type != XmlNodeType_Element)
        {
            FIXME("Unexpected node type %u\n", node_type);
            return E_FAIL;
        }

        if (is_xml_name(factory, L"method"))
            member_type = MEMBER_METHOD;
        else if (is_xml_name(factory, L"property"))
            member_type = MEMBER_PROPERTY;
        else
        {
            FIXME("Unexpected element %s\n", debugstr_xml_name(factory));
            return E_NOTIMPL;
        }

        empty = IXmlReader_IsEmptyElement(factory->xml_reader);

        hres = IXmlReader_MoveToAttributeByName(factory->xml_reader, L"name", NULL);
        if (hres != S_OK)
        {
            FIXME("Missing method name\n");
            return E_FAIL;
        }

        if (!(member = malloc(sizeof(*member)))) return E_OUTOFMEMORY;
        member->type = member_type;

        hres = read_xml_value(factory, &member->name);
        if (FAILED(hres))
        {
            free(member);
            return hres;
        }

        LIST_FOR_EACH_ENTRY(member_iter, &factory->members, struct scriptlet_member, entry)
        {
            if (!wcsicmp(member_iter->name, member->name))
            {
                FIXME("Duplicated member %s\n", debugstr_w(member->name));
                free(member);
                return E_FAIL;
            }
        }
        list_add_tail(&factory->members, &member->entry);

        switch (member_type)
        {
        case MEMBER_METHOD:
            list_init(&member->u.parameters);

            if (empty) break;
            for (;;)
            {
                struct method_parameter *parameter;

                hres = next_xml_node(factory, &node_type);
                if (FAILED(hres)) return hres;
                if (node_type == XmlNodeType_EndElement) break;
                if (node_type != XmlNodeType_Element)
                {
                    FIXME("Unexpected node type %u\n", node_type);
                    return E_FAIL;
                }
                if (!is_case_xml_name(factory, L"parameter"))
                {
                    FIXME("Unexpected method element\n");
                    return E_FAIL;
                }

                empty = IXmlReader_IsEmptyElement(factory->xml_reader);

                hres = IXmlReader_MoveToAttributeByName(factory->xml_reader, L"name", NULL);
                if (hres != S_OK)
                {
                    FIXME("Missing parameter name\n");
                    return E_FAIL;
                }

                if (!(parameter = calloc(1, sizeof(*parameter)))) return E_OUTOFMEMORY;
                list_add_tail(&member->u.parameters, &parameter->entry);

                hres = read_xml_value(factory, &parameter->name);
                if (FAILED(hres)) return hres;
                if (!empty && FAILED(hres = expect_end_element(factory))) return hres;
            }
            break;

        case MEMBER_PROPERTY:
            member->u.flags = 0;

            if (empty) break;
            for (;;)
            {
                hres = next_xml_node(factory, &node_type);
                if (FAILED(hres)) return hres;
                if (node_type == XmlNodeType_EndElement) break;
                if (node_type != XmlNodeType_Element)
                {
                    FIXME("Unexpected node type %u\n", node_type);
                    return E_FAIL;
                }
                if (is_case_xml_name(factory, L"get"))
                {
                    if (member->u.flags & PROP_GETTER)
                    {
                        FIXME("Duplicated getter\n");
                        return E_FAIL;
                    }
                    member->u.flags |= PROP_GETTER;
                }
                else if (is_case_xml_name(factory, L"put"))
                {
                    if (member->u.flags & PROP_SETTER)
                    {
                        FIXME("Duplicated setter\n");
                        return E_FAIL;
                    }
                    member->u.flags |= PROP_SETTER;
                }
                else
                {
                    FIXME("Unexpected property element %s\n", debugstr_xml_name(factory));
                    return E_FAIL;
                }

                empty = IXmlReader_IsEmptyElement(factory->xml_reader);
                if (!empty && FAILED(hres = expect_end_element(factory))) return hres;
            }
            if (!member->u.flags)
            {
                FIXME("No getter or setter\n");
                return E_NOTIMPL;
            }
            break;
        }
        if (FAILED(hres)) return hres;
    }

    return S_OK;
}

static HRESULT parse_scriptlet_script(struct scriptlet_factory *factory, struct scriptlet_script *script)
{
    XmlNodeType node_type;
    size_t buf_size, size;
    WCHAR *new_body;
    UINT read;
    HRESULT hres;

    TRACE("\n");

    for (;;)
    {
        hres = IXmlReader_MoveToNextAttribute(factory->xml_reader);
        if (hres != S_OK) break;

        if (is_xml_name(factory, L"language"))
        {
            if (script->language)
            {
                FIXME("Duplicated language\n");
                return E_FAIL;
            }
            hres = read_xml_value(factory, &script->language);
            if (FAILED(hres)) return hres;
        }
        else
        {
            FIXME("Unexpected attribute\n");
            return E_NOTIMPL;
        }
    }

    if (!script->language)
    {
        FIXME("Language not specified\n");
        return E_FAIL;
    }

    if (FAILED(hres = next_xml_node(factory, &node_type))) return hres;

    if (node_type != XmlNodeType_Text && node_type != XmlNodeType_CDATA)
    {
        FIXME("Unexpected node type %u\n", node_type);
        return E_FAIL;
    }

    if (!(script->body = malloc((buf_size = 1024) * sizeof(WCHAR)))) return E_OUTOFMEMORY;
    size = 0;

    for (;;)
    {
        read = 0;
        hres = IXmlReader_ReadValueChunk(factory->xml_reader, script->body + size, buf_size - size - 1, &read);
        if (FAILED(hres)) return hres;
        size += read;
        if (hres == S_FALSE) break;
        if (size + 1 == buf_size)
        {
            if (!(new_body = realloc(script->body, (buf_size *= 2) * sizeof(WCHAR)))) return E_OUTOFMEMORY;
            script->body = new_body;
        }
    }
    script->body[size++] = 0;
    if (size != buf_size) script->body = realloc(script->body, size * sizeof(WCHAR));

    TRACE("body %s\n", debugstr_w(script->body));
    return expect_end_element(factory);
}

static HRESULT parse_scriptlet_file(struct scriptlet_factory *factory, const WCHAR *url)
{
    XmlNodeType node_type;
    IBindCtx *bind_ctx;
    IStream *stream;
    HRESULT hres;

    hres = CreateURLMoniker(NULL, url, &factory->moniker);
    if (FAILED(hres))
    {
        WARN("CreateURLMoniker failed: %08lx\n", hres);
        return hres;
    }

    hres = CreateBindCtx(0, &bind_ctx);
    if(FAILED(hres))
        return hres;

    hres = IMoniker_BindToStorage(factory->moniker, bind_ctx, NULL, &IID_IStream, (void**)&stream);
    IBindCtx_Release(bind_ctx);
    if (FAILED(hres))
        return hres;

    hres = CreateXmlReader(&IID_IXmlReader, (void**)&factory->xml_reader, NULL);
    if(SUCCEEDED(hres))
        hres = IXmlReader_SetInput(factory->xml_reader, (IUnknown*)stream);
    IStream_Release(stream);
    if (FAILED(hres))
        return hres;

    hres = next_xml_node(factory, &node_type);
    if (hres == S_OK && node_type == XmlNodeType_XmlDeclaration)
        hres = next_xml_node(factory, &node_type);
    if (FAILED(hres))
        return hres;

    if (node_type != XmlNodeType_Element || !is_xml_name(factory, L"component"))
    {
        FIXME("Unexpected %s element\n", debugstr_xml_name(factory));
        return E_FAIL;
    }

    hres = expect_no_attributes(factory);
    if (FAILED(hres)) return hres;

    for (;;)
    {
        hres = next_xml_node(factory, &node_type);
        if (FAILED(hres)) return hres;
        if (node_type == XmlNodeType_EndElement) break;
        if (node_type != XmlNodeType_Element)
        {
            FIXME("Unexpected node type %u\n", node_type);
            return E_FAIL;
        }

        if (is_xml_name(factory, L"registration"))
            hres = parse_scriptlet_registration(factory);
        else if (is_xml_name(factory, L"public"))
            hres = parse_scriptlet_public(factory);
        else if (is_xml_name(factory, L"script"))
        {
            struct scriptlet_script *script;
            if (!(script = calloc(1, sizeof(*script)))) return E_OUTOFMEMORY;
            list_add_tail(&factory->scripts, &script->entry);
            hres = parse_scriptlet_script(factory, script);
            if (FAILED(hres)) return hres;
        }
        else
        {
            FIXME("Unexpected element %s\n", debugstr_xml_name(factory));
            return E_NOTIMPL;
        }
        if (FAILED(hres)) return hres;
    }

    hres = next_xml_node(factory, &node_type);
    if (hres != S_FALSE)
    {
        FIXME("Unexpected node type %u\n", node_type);
        return E_FAIL;
    }
    return S_OK;
}

static inline struct scriptlet_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct scriptlet_factory, IClassFactory_iface);
}

static HRESULT WINAPI scriptlet_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    struct scriptlet_factory *This = impl_from_IClassFactory(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = &This->IClassFactory_iface;
    }
    else if (IsEqualGUID(&IID_IClassFactory, riid))
    {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = &This->IClassFactory_iface;
    }
    else
    {
        WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI scriptlet_factory_AddRef(IClassFactory *iface)
{
    struct scriptlet_factory *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI scriptlet_factory_Release(IClassFactory *iface)
{
    struct scriptlet_factory *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        if (This->moniker) IMoniker_Release(This->moniker);
        if (This->xml_reader) IXmlReader_Release(This->xml_reader);
        detach_script_hosts(&This->hosts);
        while (!list_empty(&This->members))
        {
            struct scriptlet_member *member = LIST_ENTRY(list_head(&This->members), struct scriptlet_member, entry);
            list_remove(&member->entry);
            free(member->name);
            if (member->type == MEMBER_METHOD)
            {
                while (!list_empty(&member->u.parameters))
                {
                    struct method_parameter *parameter = LIST_ENTRY(list_head(&member->u.parameters), struct method_parameter, entry);
                    list_remove(&parameter->entry);
                    free(parameter->name);
                    free(parameter);
                }
            }
            free(member);
        }
        while (!list_empty(&This->scripts))
        {
            struct scriptlet_script *script = LIST_ENTRY(list_head(&This->scripts), struct scriptlet_script, entry);
            list_remove(&script->entry);
            free(script->language);
            free(script->body);
            free(script);
        }
        free(This->classid_str);
        free(This->description);
        free(This->versioned_progid);
        free(This->progid);
        free(This->version);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI scriptlet_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct scriptlet_factory *This = impl_from_IClassFactory(iface);
    IDispatchEx *disp;
    HRESULT hres;

    TRACE("(%p)->(%p %s %p)\n", This, outer, debugstr_guid(riid), ppv);

    if (outer) FIXME("outer not supported\n");

    if (list_empty(&This->hosts))
    {
        hres = create_scriptlet_hosts(This, &This->hosts);
        if (FAILED(hres)) return hres;

        hres = parse_scripts(This, &This->hosts, FALSE);
        if (FAILED(hres))
        {
            detach_script_hosts(&This->hosts);
            return hres;
        }
    }

    hres = create_scriptlet_instance(This, &disp);
    if (FAILED(hres)) return hres;

    hres = IDispatchEx_QueryInterface(disp, riid, ppv);
    IDispatchEx_Release(disp);
    return hres;
}

static HRESULT WINAPI scriptlet_factory_LockServer(IClassFactory *iface, BOOL fLock)
{
    struct scriptlet_factory *This = impl_from_IClassFactory(iface);
    TRACE("(%p)->(%x)\n", This, fLock);
    return S_OK;
}

static const struct IClassFactoryVtbl scriptlet_factory_vtbl =
{
    scriptlet_factory_QueryInterface,
    scriptlet_factory_AddRef,
    scriptlet_factory_Release,
    scriptlet_factory_CreateInstance,
    scriptlet_factory_LockServer
};

static HRESULT create_scriptlet_factory(const WCHAR *url, struct scriptlet_factory **ret)
{
    struct scriptlet_factory *factory;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(url));

    if (!(factory = calloc(1, sizeof(*factory))))
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &scriptlet_factory_vtbl;
    factory->ref = 1;
    list_init(&factory->hosts);
    list_init(&factory->members);
    list_init(&factory->scripts);

    hres = parse_scriptlet_file(factory, url);
    if (FAILED(hres))
    {
        IClassFactory_Release(&factory->IClassFactory_iface);
        return hres;
    }

    *ret = factory;
    return S_OK;
}

static HRESULT register_scriptlet(struct scriptlet_factory *factory)
{
    HKEY key, clsid_key, obj_key;
    LSTATUS status;
    HRESULT hres;

    if (factory->classid_str)
    {
        WCHAR *url;

        status = RegCreateKeyW(HKEY_CLASSES_ROOT, L"CLSID", &clsid_key);
        if (status) return E_ACCESSDENIED;

        status = RegCreateKeyW(clsid_key, factory->classid_str, &obj_key);
        RegCloseKey(clsid_key);
        if (status) return E_ACCESSDENIED;

        hres = IMoniker_GetDisplayName(factory->moniker, NULL, NULL, &url);
        if (FAILED(hres))
        {
            RegCloseKey(obj_key);
            return hres;
        }
        status = RegCreateKeyW(obj_key, L"ScriptletURL", &key);
        if (!status)
        {
            status = RegSetValueExW(key, NULL, 0, REG_SZ, (BYTE*)url,
                                    (wcslen(url) + 1) * sizeof(WCHAR));
            RegCloseKey(key);
        }
        CoTaskMemFree(url);

        if (factory->description)
            status = RegSetValueExW(obj_key, NULL, 0, REG_SZ, (BYTE*)factory->description,
                                    (wcslen(factory->description) + 1) * sizeof(WCHAR));

        if (!status)
        {
            status = RegCreateKeyW(obj_key, L"InprocServer32", &key);
            if (!status)
            {
                WCHAR str[MAX_PATH];
                GetModuleFileNameW(scrobj_instance, str, MAX_PATH);
                status = RegSetValueExW(key, NULL, 0, REG_SZ, (BYTE*)str, (wcslen(str) + 1) * sizeof(WCHAR));
                RegCloseKey(key);
            }
        }

        if (!status && factory->versioned_progid)
        {
            status = RegCreateKeyW(obj_key, L"ProgID", &key);
            if (!status)
            {
                status = RegSetValueExW(key, NULL, 0, REG_SZ, (BYTE*)factory->versioned_progid,
                                        (wcslen(factory->versioned_progid) + 1) * sizeof(WCHAR));
                RegCloseKey(key);
            }
        }

        if (!status && factory->progid)
        {
            status = RegCreateKeyW(obj_key, L"VersionIndependentProgID", &key);
            if (!status)
            {
                status = RegSetValueExW(key, NULL, 0, REG_SZ, (BYTE*)factory->progid,
                                        (wcslen(factory->progid) + 1) * sizeof(WCHAR));
                RegCloseKey(key);
            }
        }

        RegCloseKey(obj_key);
        if (status) return E_ACCESSDENIED;
    }

    if (factory->progid)
    {
        status = RegCreateKeyW(HKEY_CLASSES_ROOT, factory->progid, &key);
        if (status) return E_ACCESSDENIED;

        if (factory->description)
            status = RegSetValueExW(key, NULL, 0, REG_SZ, (BYTE*)factory->description,
                                    (wcslen(factory->description) + 1) * sizeof(WCHAR));

        if (!status && factory->classid_str)
        {
            status = RegCreateKeyW(key, L"CLSID", &clsid_key);
            if (!status)
            {
                status = RegSetValueExW(clsid_key, NULL, 0, REG_SZ, (BYTE*)factory->classid_str,
                                        (wcslen(factory->classid_str) + 1) * sizeof(WCHAR));
                RegCloseKey(clsid_key);
            }
        }

        RegCloseKey(key);
        if (status) return E_ACCESSDENIED;
    }

    if (factory->versioned_progid)
    {
        status = RegCreateKeyW(HKEY_CLASSES_ROOT, factory->versioned_progid, &key);
        if (status) return E_ACCESSDENIED;

        if (factory->description)
            status = RegSetValueExW(key, NULL, 0, REG_SZ, (BYTE*)factory->description,
                                    (wcslen(factory->description) + 1) * sizeof(WCHAR));

        if (!status && factory->classid_str)
        {
            status = RegCreateKeyW(key, L"CLSID", &clsid_key);
            if (!status)
            {
                status = RegSetValueExW(clsid_key, NULL, 0, REG_SZ, (BYTE*)factory->classid_str,
                                        (wcslen(factory->classid_str) + 1) * sizeof(WCHAR));
                RegCloseKey(clsid_key);
            }
        }

        RegCloseKey(key);
        if (status) return E_ACCESSDENIED;
    }

    return S_OK;
}

static HRESULT unregister_scriptlet(struct scriptlet_factory *factory)
{
    LSTATUS status;

    if (factory->classid_str)
    {
        HKEY clsid_key;
        status = RegCreateKeyW(HKEY_CLASSES_ROOT, L"CLSID", &clsid_key);
        if (!status)
        {
            RegDeleteTreeW(clsid_key, factory->classid_str);
            RegCloseKey(clsid_key);
        }
    }

    if (factory->progid) RegDeleteTreeW(HKEY_CLASSES_ROOT, factory->progid);
    if (factory->versioned_progid) RegDeleteTreeW(HKEY_CLASSES_ROOT, factory->versioned_progid);
    return S_OK;
}

struct scriptlet_typelib
{
    IGenScriptletTLib IGenScriptletTLib_iface;
    LONG ref;

    BSTR guid;
};

static inline struct scriptlet_typelib *impl_from_IGenScriptletTLib(IGenScriptletTLib *iface)
{
    return CONTAINING_RECORD(iface, struct scriptlet_typelib, IGenScriptletTLib_iface);
}

static HRESULT WINAPI scriptlet_typelib_QueryInterface(IGenScriptletTLib *iface, REFIID riid, void **obj)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IGenScriptletTLib) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IGenScriptletTLib_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI scriptlet_typelib_AddRef(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI scriptlet_typelib_Release(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (!ref)
    {
        SysFreeString(This->guid);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI scriptlet_typelib_GetTypeInfoCount(IGenScriptletTLib *iface, UINT *count)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %p)\n", This, count);

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI scriptlet_typelib_GetTypeInfo(IGenScriptletTLib *iface, UINT index, LCID lcid,
    ITypeInfo **tinfo)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %u, %lx, %p)\n", This, index, lcid, tinfo);

    return get_typeinfo(IGenScriptletTLib_tid, tinfo);
}

static HRESULT WINAPI scriptlet_typelib_GetIDsOfNames(IGenScriptletTLib *iface, REFIID riid, LPOLESTR *names,
    UINT cNames, LCID lcid, DISPID *dispid)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p, %s, %p, %u, %lx, %p)\n", This, debugstr_guid(riid), names, cNames, lcid, dispid);

    hr = get_typeinfo(IGenScriptletTLib_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, cNames, dispid);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI scriptlet_typelib_Invoke(IGenScriptletTLib *iface, DISPID dispid, REFIID riid,
    LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *argerr)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p, %ld, %s, %lx, %x, %p, %p, %p, %p)\n", This, dispid, debugstr_guid(riid), lcid, flags,
        params, result, ei, argerr);

    hr = get_typeinfo(IGenScriptletTLib_tid, &typeinfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IGenScriptletTLib_iface, dispid, flags,
                params, result, ei, argerr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI scriptlet_typelib_AddURL(IGenScriptletTLib *iface, BSTR url)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(url));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_Path(IGenScriptletTLib *iface, BSTR path)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_Path(IGenScriptletTLib *iface, BSTR *path)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_Doc(IGenScriptletTLib *iface, BSTR doc)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(doc));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_Doc(IGenScriptletTLib *iface, BSTR *doc)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, doc);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_Name(IGenScriptletTLib *iface, BSTR name)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_Name(IGenScriptletTLib *iface, BSTR *name)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_MajorVersion(IGenScriptletTLib *iface, WORD version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %x): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_MajorVersion(IGenScriptletTLib *iface, WORD *version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_MinorVersion(IGenScriptletTLib *iface, WORD version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %x): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_MinorVersion(IGenScriptletTLib *iface, WORD *version)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %p): stub\n", This, version);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_Write(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_Reset(IGenScriptletTLib *iface)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_put_GUID(IGenScriptletTLib *iface, BSTR guid)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    FIXME("(%p, %s): stub\n", This, debugstr_w(guid));

    return E_NOTIMPL;
}

static HRESULT WINAPI scriptlet_typelib_get_GUID(IGenScriptletTLib *iface, BSTR *ret)
{
    struct scriptlet_typelib *This = impl_from_IGenScriptletTLib(iface);

    TRACE("(%p, %p)\n", This, ret);

    *ret = NULL;

    if (!This->guid)
    {
        WCHAR guidW[39];
        GUID guid;
        HRESULT hr;

        hr = CoCreateGuid(&guid);
        if (FAILED(hr))
            return hr;

        hr = StringFromGUID2(&guid, guidW, ARRAY_SIZE(guidW));
        if (FAILED(hr))
            return hr;

        if (!(This->guid = SysAllocString(guidW)))
            return E_OUTOFMEMORY;
    }

    *ret = SysAllocString(This->guid);
    return *ret ? S_OK : E_OUTOFMEMORY;
}

static const IGenScriptletTLibVtbl scriptlet_typelib_vtbl =
{
    scriptlet_typelib_QueryInterface,
    scriptlet_typelib_AddRef,
    scriptlet_typelib_Release,
    scriptlet_typelib_GetTypeInfoCount,
    scriptlet_typelib_GetTypeInfo,
    scriptlet_typelib_GetIDsOfNames,
    scriptlet_typelib_Invoke,
    scriptlet_typelib_AddURL,
    scriptlet_typelib_put_Path,
    scriptlet_typelib_get_Path,
    scriptlet_typelib_put_Doc,
    scriptlet_typelib_get_Doc,
    scriptlet_typelib_put_Name,
    scriptlet_typelib_get_Name,
    scriptlet_typelib_put_MajorVersion,
    scriptlet_typelib_get_MajorVersion,
    scriptlet_typelib_put_MinorVersion,
    scriptlet_typelib_get_MinorVersion,
    scriptlet_typelib_Write,
    scriptlet_typelib_Reset,
    scriptlet_typelib_put_GUID,
    scriptlet_typelib_get_GUID
};

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("%p, %lu, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            scrobj_instance = hinst;
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            release_typelib();
            break;
    }
    return TRUE;
}

/***********************************************************************
 *      DllInstall (scrobj.@)
 */
HRESULT WINAPI DllInstall(BOOL install, const WCHAR *arg)
{
    struct scriptlet_factory *factory;
    HRESULT hres;

    if (install)
    {
        hres = DllRegisterServer();
        if (!arg || FAILED(hres)) return hres;
    }
    else if (!arg)
        return DllUnregisterServer();

    hres = create_scriptlet_factory(arg, &factory);
    if (SUCCEEDED(hres))
    {
        if (factory->have_registration)
        {
            /* validate scripts */
            hres = create_scriptlet_hosts(factory, &factory->hosts);
            if (SUCCEEDED(hres)) hres = parse_scripts(factory, &factory->hosts, FALSE);
        }
        else
        {
            FIXME("No registration info\n");
            hres = E_FAIL;
        }
        if (SUCCEEDED(hres))
        {
            if (install)
                hres = register_scriptlet(factory);
            else
                hres = unregister_scriptlet(factory);
        }
        IClassFactory_Release(&factory->IClassFactory_iface);
    }

    return hres;
}

static HRESULT WINAPI scriptlet_typelib_CreateInstance(IClassFactory *factory, IUnknown *outer, REFIID riid, void **obj)
{
    struct scriptlet_typelib *This;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p)\n", factory, outer, debugstr_guid(riid), obj);

    *obj = NULL;

    if (!(This = calloc(1, sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IGenScriptletTLib_iface.lpVtbl = &scriptlet_typelib_vtbl;
    This->ref = 1;

    hr = IGenScriptletTLib_QueryInterface(&This->IGenScriptletTLib_iface, riid, obj);
    IGenScriptletTLib_Release(&This->IGenScriptletTLib_iface);
    return hr;
}

static HRESULT WINAPI scrruncf_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }
    else if (IsEqualGUID(&IID_IClassFactory, riid))
    {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI scrruncf_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI scrruncf_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI scrruncf_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const struct IClassFactoryVtbl scriptlet_typelib_factory_vtbl =
{
    scrruncf_QueryInterface,
    scrruncf_AddRef,
    scrruncf_Release,
    scriptlet_typelib_CreateInstance,
    scrruncf_LockServer
};

static IClassFactory scriptlet_typelib_factory = { &scriptlet_typelib_factory_vtbl };

/***********************************************************************
 *      DllGetClassObject (scrobj.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    WCHAR key_name[128], *p;
    LONG size = 0;
    LSTATUS status;

    if (IsEqualGUID(&CLSID_TypeLib, rclsid))
    {
        TRACE("(Scriptlet.TypeLib %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&scriptlet_typelib_factory, riid, ppv);
    }

    wcscpy(key_name, L"CLSID\\");
    p = key_name + wcslen(key_name);
    p += StringFromGUID2(rclsid, p, ARRAY_SIZE(key_name) - (p - key_name)) - 1;
    wcscpy(p, L"\\ScriptletURL");
    status = RegQueryValueW(HKEY_CLASSES_ROOT, key_name, NULL, &size);
    if (!status)
    {
        struct scriptlet_factory *factory;
        WCHAR *url;
        HRESULT hres;

        if (!(url = malloc(size * sizeof(WCHAR)))) return E_OUTOFMEMORY;
        status = RegQueryValueW(HKEY_CLASSES_ROOT, key_name, url, &size);

        hres = create_scriptlet_factory(url, &factory);
        free(url);
        if (FAILED(hres)) return hres;

        hres = IClassFactory_QueryInterface(&factory->IClassFactory_iface, riid, ppv);
        IClassFactory_Release(&factory->IClassFactory_iface);
        return hres;
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
