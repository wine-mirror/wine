/* WinRT Windows.ApplicationModel.Core.CoreApplication implementation
 *
 * Copyright 2024 Onni Kukkonen
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

#include "private.h"
#include <assert.h>
#include <winuser.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <pathcch.h>
#include <shlwapi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(coreapp);

struct coreapp_impl
{
    IActivationFactory IActivationFactory_iface;
    ICoreApplication ICoreApplication_iface;
    ICoreApplicationView ICoreApplicationView_iface;
    IFrameworkView *current_view;
    xmlDocPtr appx_manifest;
    char *display_name;
    char *identity_name;
    LONG ref;
};

static inline struct coreapp_impl *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct coreapp_impl, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct coreapp_impl *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ICoreApplication))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ICoreApplication_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct coreapp_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct coreapp_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI factory_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};


static inline struct coreapp_impl *impl_from_ICoreApplication( ICoreApplication *iface )
{
    return CONTAINING_RECORD( iface, struct coreapp_impl, ICoreApplication_iface );
}

static HRESULT WINAPI coreapp_impl_QueryInterface( ICoreApplication *iface, REFIID iid, void **out )
{
    struct coreapp_impl *impl = impl_from_ICoreApplication( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICoreApplication ))
    {
        *out = &impl->ICoreApplication_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI coreapp_impl_AddRef( ICoreApplication *iface )
{
    struct coreapp_impl *impl = impl_from_ICoreApplication( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI coreapp_impl_Release( ICoreApplication *iface )
{
    struct coreapp_impl *impl = impl_from_ICoreApplication( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI coreapp_impl_GetIids( ICoreApplication *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_impl_GetRuntimeClassName( ICoreApplication *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_impl_GetTrustLevel( ICoreApplication *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_impl_get_Id( ICoreApplication *iface, HSTRING *value)
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_impl_add_Suspending( ICoreApplication *iface, IEventHandler_SuspendingEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub.\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI coreapp_impl_remove_Suspending( ICoreApplication *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub.\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_impl_add_Resuming( ICoreApplication *iface, IEventHandler_IInspectable *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub.\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI coreapp_impl_remove_Resuming( ICoreApplication *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub.\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_impl_get_Properties( ICoreApplication *iface, IPropertySet **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI coreapp_impl_GetCurrentView( ICoreApplication *iface, ICoreApplicationView **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

static xmlNodePtr find_xml_child(xmlNodePtr node, const char* node_name) {
    xmlNodePtr cur_node;

    if (node == NULL) {
        return NULL;
    }

    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (xmlStrcmp(cur_node->name, (const xmlChar *)node_name) == 0) {
            return cur_node;
        }
    }
    
    return NULL;
}

static BOOL try_parse_appxmanifest( struct coreapp_impl *impl ) 
{
    WCHAR wpath[MAX_PATH];
    char *path;
    HRESULT ret;
    DWORD gmfl_ret;
    xmlNodePtr rootElem;
    xmlNodePtr subElem;

    impl->appx_manifest = NULL;

    gmfl_ret = GetModuleFileNameW(NULL, wpath, MAX_PATH);
    if (gmfl_ret == 0) {
        ERR("GetModuleFileNameW failed\n");
        return FALSE;
    }

    ret = PathCchRemoveFileSpec(wpath, MAX_PATH);
    if (!SUCCEEDED(ret)) {
        ERR("PathCchRemoveFileSpec failed\n");
        return FALSE;
    }

    // This shouldn't be here, but it's fine.
    SetCurrentDirectoryW(wpath);

    ret = PathCchAppend(wpath, MAX_PATH, L"appxmanifest.xml");
    if (!SUCCEEDED(ret)) {
        ERR("PathCchAppend failed\n");
        return FALSE;
    }

    if (PathFileExistsW(wpath)) {
        path = malloc(wcstombs(NULL, wpath, MAX_PATH)+1);
        if (wcstombs(path, wpath, MAX_PATH) == -1) {
            ERR("wcstombs failed for %s\n", debugstr_w(wpath));
        } else {
            impl->appx_manifest = xmlReadFile(path, NULL, 0);
            if (impl->appx_manifest == NULL) {
                ERR("xmlReadFile failed for %s\n", debugstr_a(path));
            }
        }

        free(path);
    } else {
        ERR("AppxManifest.xml: Not found at %s\n", debugstr_w(wpath));
    }

    if (impl->appx_manifest == NULL) {
        return FALSE;
    }

    rootElem = xmlDocGetRootElement(impl->appx_manifest);
    if (rootElem == NULL) {
        return FALSE;
    }

    if (strcmp((char*)rootElem->name, "Package") != 0) {
        ERR("AppxManifest.xml: Invalid root key '%s', expected 'Package'\n", rootElem->name);
        return FALSE;
    }

    subElem = find_xml_child(rootElem->children, "Properties");
    if (subElem == NULL) {
        ERR("AppxManifest.xml: No Properties defined!\n");
        return FALSE;
    }

    subElem = find_xml_child(subElem->children, "DisplayName");
    if (subElem == NULL) {
        ERR("AppxManifest.xml: No DisplayName defined!\n");
        return FALSE;
    }

    impl->display_name = (char*)subElem->children->content;
    TRACE("Got display name %s\n", impl->display_name);

    subElem = find_xml_child(rootElem->children, "Identity");
    if (subElem == NULL) {
        ERR("AppxManifest.xml: No Identity defined!\n");
        return FALSE;
    }

    impl->identity_name = (char*)xmlGetProp(subElem, (const xmlChar *)"Name");
    TRACE("Got identity name %s\n", impl->identity_name);

    return TRUE;
}

DWORD corewindow_tls;

static HRESULT WINAPI coreapp_impl_Run( ICoreApplication *iface, IFrameworkViewSource *view_source)
{
    HRESULT ret;
    HSTRING handle;
    IFrameworkView* view;
    struct corewindow_impl *corewindow_impl;
    struct coreapp_impl *impl = impl_from_ICoreApplication( iface );
    
    FIXME("iface %p, view_source %p semi-stub.\n", iface, view_source);

    corewindow_tls = TlsAlloc();
    if (corewindow_tls == TLS_OUT_OF_INDEXES) {
        ERR("Failed to allocate TLS!\n");
        return E_OUTOFMEMORY;
    }

    if (!try_parse_appxmanifest(impl)) {
        ERR("Failed to parse AppxManifest.xml\n");
        return APPX_E_INVALID_MANIFEST;
    }

    ret = view_source->lpVtbl->CreateView(view_source, &view);
    if (ret != S_OK) {
        ERR("CreateView fail\n");
        return ret;
    }

    // Initialize, Load, Run
    view->lpVtbl->Initialize(view, &impl->ICoreApplicationView_iface);

    corewindow_impl = create_corewindow(view, impl->identity_name, impl->display_name);
    if (!corewindow_impl) {
        ERR("create_corewindow fail\n");
        return E_FAIL;
    }

    ret = WindowsCreateString(NULL, 0, &handle);
    if (ret != S_OK) {
        ERR("WindowsCreateString fail\n");
        return ret;
    }
    
    view->lpVtbl->SetWindow(view, &corewindow_impl->ICoreWindow_iface);
    view->lpVtbl->Load(view, handle);

    //TODO: Run Activated handler
    view->lpVtbl->Run(view);
    return S_OK;
}

static HRESULT WINAPI coreapp_impl_RunWithActivationFactories( ICoreApplication *iface, IGetActivationFactory *factory)
{
    FIXME("iface %p, factory %p stub.\n", iface, factory);
    return E_NOTIMPL;
}

static const struct ICoreApplicationVtbl coreapp_impl_vtbl =
{
    coreapp_impl_QueryInterface,
    coreapp_impl_AddRef,
    coreapp_impl_Release,
    /* IInspectable methods */
    coreapp_impl_GetIids,
    coreapp_impl_GetRuntimeClassName,
    coreapp_impl_GetTrustLevel,
    /* ICoreApplication methods */
    coreapp_impl_get_Id,
    coreapp_impl_add_Suspending,
    coreapp_impl_remove_Suspending,
    coreapp_impl_add_Resuming,
    coreapp_impl_remove_Resuming,
    coreapp_impl_get_Properties,
    coreapp_impl_GetCurrentView,
    coreapp_impl_Run,
    coreapp_impl_RunWithActivationFactories
};

static inline struct coreapp_impl *impl_from_ICoreApplicationView( ICoreApplicationView *iface )
{
    return CONTAINING_RECORD( iface, struct coreapp_impl, ICoreApplicationView_iface );
}

static HRESULT WINAPI coreappview_impl_QueryInterface( ICoreApplicationView *iface, REFIID iid, void **out )
{
    struct coreapp_impl *impl = impl_from_ICoreApplicationView( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICoreApplicationView ))
    {
        *out = &impl->ICoreApplicationView_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI coreappview_impl_AddRef( ICoreApplicationView *iface )
{
    struct coreapp_impl *impl = impl_from_ICoreApplicationView( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI coreappview_impl_Release( ICoreApplicationView *iface )
{
    struct coreapp_impl *impl = impl_from_ICoreApplicationView( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI coreappview_impl_GetIids( ICoreApplicationView *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreappview_impl_GetRuntimeClassName( ICoreApplicationView *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreappview_impl_GetTrustLevel( ICoreApplicationView *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreappview_impl_get_CoreWindow( ICoreApplicationView *iface, __x_ABI_CWindows_CUI_CCore_CICoreWindow **value)
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI coreappview_impl_add_Activated( ICoreApplicationView *iface, ITypedEventHandler_CoreApplicationView_IActivatedEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub.\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI coreappview_impl_remove_Activated( ICoreApplicationView *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub.\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI coreappview_impl_get_IsMain( ICoreApplicationView *iface, boolean *value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI coreappview_impl_get_IsHosted( ICoreApplicationView *iface, boolean *value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);
    return E_NOTIMPL;
}

static const struct ICoreApplicationViewVtbl coreappview_impl_vtbl =
{
    coreappview_impl_QueryInterface,
    coreappview_impl_AddRef,
    coreappview_impl_Release,
    /* IInspectable methods */
    coreappview_impl_GetIids,
    coreappview_impl_GetRuntimeClassName,
    coreappview_impl_GetTrustLevel,
    /* ICoreApplicationView methods */
    coreappview_impl_get_CoreWindow,
    coreappview_impl_add_Activated,
    coreappview_impl_remove_Activated,
    coreappview_impl_get_IsMain,
    coreappview_impl_get_IsHosted
};



static struct coreapp_impl coreapp_impl_global =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .ICoreApplication_iface.lpVtbl = &coreapp_impl_vtbl,
    .ICoreApplicationView_iface.lpVtbl = &coreappview_impl_vtbl,
    .ref = 1,
};

IActivationFactory *coreapplication_factory = &coreapp_impl_global.IActivationFactory_iface;
