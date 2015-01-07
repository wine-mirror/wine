/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wbemdisp.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wbemdisp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemdisp);

struct services
{
    ISWbemServices ISWbemServices_iface;
    LONG refs;
};

static inline struct services *impl_from_ISWbemServices(
    ISWbemServices *iface )
{
    return CONTAINING_RECORD( iface, struct services, ISWbemServices_iface );
}

static ULONG WINAPI services_AddRef(
    ISWbemServices *iface )
{
    struct services *services = impl_from_ISWbemServices( iface );
    return InterlockedIncrement( &services->refs );
}

static ULONG WINAPI services_Release(
    ISWbemServices *iface )
{
    struct services *services = impl_from_ISWbemServices( iface );
    LONG refs = InterlockedDecrement( &services->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", services );
        heap_free( services );
    }
    return refs;
}

static HRESULT WINAPI services_QueryInterface(
    ISWbemServices *iface,
    REFIID riid,
    void **ppvObject )
{
    struct services *services = impl_from_ISWbemServices( iface );

    TRACE( "%p %s %p\n", services, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemServices ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = services;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemServices_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI services_GetTypeInfoCount(
    ISWbemServices *iface,
    UINT *count )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_GetTypeInfo(
    ISWbemServices *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_GetIDsOfNames(
    ISWbemServices *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_Invoke(
    ISWbemServices *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_Get(
    ISWbemServices *iface,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObject **objWbemObject )
{
    FIXME( "%p, %s, %d, %p, %p\n", iface, debugstr_w(strObjectPath), iFlags, objWbemNamedValueSet,
           objWbemObject );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_GetAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_Delete(
    ISWbemServices *iface,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_DeleteAsync(
    ISWbemServices* This,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_InstancesOf(
    ISWbemServices *iface,
    BSTR strClass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_InstancesOfAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strClass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_SubclassesOf(
    ISWbemServices *iface,
    BSTR strSuperclass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_SubclassesOfAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strSuperclass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecQuery(
    ISWbemServices *iface,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecQueryAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG lFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_AssociatorsOf(
    ISWbemServices *iface,
    BSTR strObjectPath,
    BSTR strAssocClass,
    BSTR strResultClass,
    BSTR strResultRole,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredAssocQualifier,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_AssociatorsOfAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    BSTR strAssocClass,
    BSTR strResultClass,
    BSTR strResultRole,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredAssocQualifier,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ReferencesTo(
    ISWbemServices *iface,
    BSTR strObjectPath,
    BSTR strResultClass,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ReferencesToAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    BSTR strResultClass,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecNotificationQuery(
    ISWbemServices *iface,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemEventSource **objWbemEventSource )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecNotificationQueryAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecMethod(
    ISWbemServices *iface,
    BSTR strObjectPath,
    BSTR strMethodName,
    IDispatch *objWbemInParameters,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObject **objWbemOutParameters )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecMethodAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    BSTR strMethodName,
    IDispatch *objWbemInParameters,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_get_Security_(
        ISWbemServices *iface,
        ISWbemSecurity **objWbemSecurity )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static const ISWbemServicesVtbl services_vtbl =
{
    services_QueryInterface,
    services_AddRef,
    services_Release,
    services_GetTypeInfoCount,
    services_GetTypeInfo,
    services_GetIDsOfNames,
    services_Invoke,
    services_Get,
    services_GetAsync,
    services_Delete,
    services_DeleteAsync,
    services_InstancesOf,
    services_InstancesOfAsync,
    services_SubclassesOf,
    services_SubclassesOfAsync,
    services_ExecQuery,
    services_ExecQueryAsync,
    services_AssociatorsOf,
    services_AssociatorsOfAsync,
    services_ReferencesTo,
    services_ReferencesToAsync,
    services_ExecNotificationQuery,
    services_ExecNotificationQueryAsync,
    services_ExecMethod,
    services_ExecMethodAsync,
    services_get_Security_
};

static HRESULT SWbemServices_create( ISWbemServices **obj )
{
    struct services *services;

    TRACE( "%p\n", obj );

    if (!(services = heap_alloc( sizeof(*services) ))) return E_OUTOFMEMORY;
    services->ISWbemServices_iface.lpVtbl = &services_vtbl;
    services->refs = 1;

    *obj = &services->ISWbemServices_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct locator
{
    ISWbemLocator ISWbemLocator_iface;
    LONG refs;
};

static inline struct locator *impl_from_ISWbemLocator( ISWbemLocator *iface )
{
    return CONTAINING_RECORD( iface, struct locator, ISWbemLocator_iface );
}

static ULONG WINAPI locator_AddRef(
    ISWbemLocator *iface )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    return InterlockedIncrement( &locator->refs );
}

static ULONG WINAPI locator_Release(
    ISWbemLocator *iface )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    LONG refs = InterlockedDecrement( &locator->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", locator );
        heap_free( locator );
    }
    return refs;
}

static HRESULT WINAPI locator_QueryInterface(
    ISWbemLocator *iface,
    REFIID riid,
    void **ppvObject )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );

    TRACE( "%p, %s, %p\n", locator, debugstr_guid( riid ), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemLocator ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemLocator_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI locator_GetTypeInfoCount(
    ISWbemLocator *iface,
    UINT *count )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );

    TRACE( "%p, %p\n", locator, count );
    *count = 1;
    return S_OK;
}

enum type_id
{
    ISWbemLocator_tid,
    last_tid
};

static ITypeLib *wbemdisp_typelib;
static ITypeInfo *wbemdisp_typeinfo[last_tid];

static REFIID wbemdisp_tid_id[] =
{
    &IID_ISWbemLocator
};

static HRESULT get_typeinfo( enum type_id tid, ITypeInfo **ret )
{
    HRESULT hr;

    if (!wbemdisp_typelib)
    {
        ITypeLib *typelib;

        hr = LoadRegTypeLib( &LIBID_WbemScripting, 1, 2, LOCALE_SYSTEM_DEFAULT, &typelib );
        if (FAILED( hr ))
        {
            ERR( "LoadRegTypeLib failed: %08x\n", hr );
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)&wbemdisp_typelib, typelib, NULL ))
            ITypeLib_Release( typelib );
    }
    if (!wbemdisp_typeinfo[tid])
    {
        ITypeInfo *typeinfo;

        hr = ITypeLib_GetTypeInfoOfGuid( wbemdisp_typelib, wbemdisp_tid_id[tid], &typeinfo );
        if (FAILED( hr ))
        {
            ERR( "GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(wbemdisp_tid_id[tid]), hr );
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)(wbemdisp_typeinfo + tid), typeinfo, NULL ))
            ITypeInfo_Release( typeinfo );
    }
    *ret = wbemdisp_typeinfo[tid];
    return S_OK;
}

static HRESULT WINAPI locator_GetTypeInfo(
    ISWbemLocator *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    TRACE( "%p, %u, %u, %p\n", locator, index, lcid, info );

    return get_typeinfo( ISWbemLocator_tid, info );
}

static HRESULT WINAPI locator_GetIDsOfNames(
    ISWbemLocator *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %u, %p\n", locator, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemLocator_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI locator_Invoke(
    ISWbemLocator *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", locator, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemLocator_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &locator->ISWbemLocator_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI locator_ConnectServer(
    ISWbemLocator *iface,
    BSTR strServer,
    BSTR strNamespace,
    BSTR strUser,
    BSTR strPassword,
    BSTR strLocale,
    BSTR strAuthority,
    LONG iSecurityFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemServices **objWbemServices )
{
    FIXME( "%p, %s, %s, %s, %p, %s, %s, 0x%08x, %p, %p\n", iface, debugstr_w(strServer),
           debugstr_w(strNamespace), debugstr_w(strUser), strPassword, debugstr_w(strLocale),
           debugstr_w(strAuthority), iSecurityFlags, objWbemNamedValueSet, objWbemServices );
    return SWbemServices_create( objWbemServices );
}

static HRESULT WINAPI locator_get_Security_(
    ISWbemLocator *iface,
    ISWbemSecurity **objWbemSecurity )
{
    FIXME( "%p, %p\n", iface, objWbemSecurity );
    return E_NOTIMPL;
}

static const ISWbemLocatorVtbl locator_vtbl =
{
    locator_QueryInterface,
    locator_AddRef,
    locator_Release,
    locator_GetTypeInfoCount,
    locator_GetTypeInfo,
    locator_GetIDsOfNames,
    locator_Invoke,
    locator_ConnectServer,
    locator_get_Security_
};

HRESULT SWbemLocator_create( void **obj )
{
    struct locator *locator;

    TRACE( "%p\n", obj );

    if (!(locator = heap_alloc( sizeof(*locator) ))) return E_OUTOFMEMORY;
    locator->ISWbemLocator_iface.lpVtbl = &locator_vtbl;
    locator->refs = 1;

    *obj = &locator->ISWbemLocator_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
