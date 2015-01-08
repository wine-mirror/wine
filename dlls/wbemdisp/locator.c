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
#include "initguid.h"
#include "objbase.h"
#include "wbemcli.h"
#include "wbemdisp.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wbemdisp_private.h"
#include "wbemdisp_classes.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemdisp);

enum type_id
{
    ISWbemLocator_tid,
    ISWbemObject_tid,
    ISWbemObjectSet_tid,
    ISWbemServices_tid,
    last_tid
};

static ITypeLib *wbemdisp_typelib;
static ITypeInfo *wbemdisp_typeinfo[last_tid];

static REFIID wbemdisp_tid_id[] =
{
    &IID_ISWbemLocator,
    &IID_ISWbemObject,
    &IID_ISWbemObjectSet,
    &IID_ISWbemServices
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
    ITypeInfo_AddRef( *ret );
    return S_OK;
}

struct object
{
    ISWbemObject ISWbemObject_iface;
    LONG refs;
    IWbemClassObject *object;
};

static inline struct object *impl_from_ISWbemObject(
    ISWbemObject *iface )
{
    return CONTAINING_RECORD( iface, struct object, ISWbemObject_iface );
}

static ULONG WINAPI object_AddRef(
    ISWbemObject *iface )
{
    struct object *object = impl_from_ISWbemObject( iface );
    return InterlockedIncrement( &object->refs );
}

static ULONG WINAPI object_Release(
    ISWbemObject *iface )
{
    struct object *object = impl_from_ISWbemObject( iface );
    LONG refs = InterlockedDecrement( &object->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", object );
        IWbemClassObject_Release( object->object );
        heap_free( object );
    }
    return refs;
}

static HRESULT WINAPI object_QueryInterface(
    ISWbemObject *iface,
    REFIID riid,
    void **ppvObject )
{
    struct object *object = impl_from_ISWbemObject( iface );

    TRACE( "%p %s %p\n", object, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemObject ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = object;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemObject_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI object_GetTypeInfoCount(
    ISWbemObject *iface,
    UINT *count )
{
    struct object *object = impl_from_ISWbemObject( iface );

    TRACE( "%p, %p\n", object, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI object_GetTypeInfo(
    ISWbemObject *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct object *object = impl_from_ISWbemObject( iface );
    TRACE( "%p, %u, %u, %p\n", object, index, lcid, info );

    return get_typeinfo( ISWbemObject_tid, info );
}

static HRESULT WINAPI object_GetIDsOfNames(
    ISWbemObject *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct object *object = impl_from_ISWbemObject( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %u, %p\n", object, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemObject_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI object_Invoke(
    ISWbemObject *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct object *object = impl_from_ISWbemObject( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", object, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemObject_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &object->ISWbemObject_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI object_Put_(
    ISWbemObject *iface,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectPath **objWbemObjectPath )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static const ISWbemObjectVtbl object_vtbl =
{
    object_QueryInterface,
    object_AddRef,
    object_Release,
    object_GetTypeInfoCount,
    object_GetTypeInfo,
    object_GetIDsOfNames,
    object_Invoke,
    object_Put_
};

static HRESULT SWbemObject_create( IWbemClassObject *wbem_object, ISWbemObject **obj )
{
    struct object *object;

    TRACE( "%p, %p\n", obj, wbem_object );

    if (!(object = heap_alloc( sizeof(*object) ))) return E_OUTOFMEMORY;
    object->ISWbemObject_iface.lpVtbl = &object_vtbl;
    object->refs = 1;
    object->object = wbem_object;
    IWbemClassObject_AddRef( object->object );

    *obj = &object->ISWbemObject_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct objectset
{
    ISWbemObjectSet ISWbemObjectSet_iface;
    LONG refs;
    IEnumWbemClassObject *objectenum;
};

static inline struct objectset *impl_from_ISWbemObjectSet(
    ISWbemObjectSet *iface )
{
    return CONTAINING_RECORD( iface, struct objectset, ISWbemObjectSet_iface );
}

static ULONG WINAPI objectset_AddRef(
    ISWbemObjectSet *iface )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    return InterlockedIncrement( &objectset->refs );
}

static ULONG WINAPI objectset_Release(
    ISWbemObjectSet *iface )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    LONG refs = InterlockedDecrement( &objectset->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", objectset );
        IEnumWbemClassObject_Release( objectset->objectenum );
        heap_free( objectset );
    }
    return refs;
}

static HRESULT WINAPI objectset_QueryInterface(
    ISWbemObjectSet *iface,
    REFIID riid,
    void **ppvObject )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );

    TRACE( "%p %s %p\n", objectset, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemObjectSet ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = objectset;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemObjectSet_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI objectset_GetTypeInfoCount(
    ISWbemObjectSet *iface,
    UINT *count )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    TRACE( "%p, %p\n", objectset, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI objectset_GetTypeInfo(
    ISWbemObjectSet *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    TRACE( "%p, %u, %u, %p\n", objectset, index, lcid, info );

    return get_typeinfo( ISWbemObjectSet_tid, info );
}

static HRESULT WINAPI objectset_GetIDsOfNames(
    ISWbemObjectSet *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %u, %p\n", objectset, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemObjectSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI objectset_Invoke(
    ISWbemObjectSet *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", objectset, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemObjectSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &objectset->ISWbemObjectSet_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI objectset_get__NewEnum(
    ISWbemObjectSet *iface,
    IUnknown **pUnk )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI objectset_Item(
    ISWbemObjectSet *iface,
    BSTR strObjectPath,
    LONG iFlags,
    ISWbemObject **objWbemObject )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI objectset_get_Count(
    ISWbemObjectSet *iface,
    LONG *iCount )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );

    TRACE( "%p, %p\n", objectset, iCount );

    *iCount = 0;
    IEnumWbemClassObject_Reset( objectset->objectenum );
    while (!IEnumWbemClassObject_Skip( objectset->objectenum, WBEM_INFINITE, 1 )) (*iCount)++;
    return S_OK;
}

static HRESULT WINAPI objectset_get_Security_(
    ISWbemObjectSet *iface,
    ISWbemSecurity **objWbemSecurity )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI objectset_ItemIndex(
    ISWbemObjectSet *iface,
    LONG lIndex,
    ISWbemObject **objWbemObject )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static const ISWbemObjectSetVtbl objectset_vtbl =
{
    objectset_QueryInterface,
    objectset_AddRef,
    objectset_Release,
    objectset_GetTypeInfoCount,
    objectset_GetTypeInfo,
    objectset_GetIDsOfNames,
    objectset_Invoke,
    objectset_get__NewEnum,
    objectset_Item,
    objectset_get_Count,
    objectset_get_Security_,
    objectset_ItemIndex
};

static HRESULT SWbemObjectSet_create( IEnumWbemClassObject *wbem_objectenum, ISWbemObjectSet **obj )
{
    struct objectset *objectset;

    TRACE( "%p, %p\n", obj, wbem_objectenum );

    if (!(objectset = heap_alloc( sizeof(*objectset) ))) return E_OUTOFMEMORY;
    objectset->ISWbemObjectSet_iface.lpVtbl = &objectset_vtbl;
    objectset->refs = 1;
    objectset->objectenum = wbem_objectenum;
    IEnumWbemClassObject_AddRef( objectset->objectenum );

    *obj = &objectset->ISWbemObjectSet_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct services
{
    ISWbemServices ISWbemServices_iface;
    LONG refs;
    IWbemServices *services;
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
        IWbemServices_Release( services->services );
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
    struct services *services = impl_from_ISWbemServices( iface );
    TRACE( "%p, %p\n", services, count );

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI services_GetTypeInfo(
    ISWbemServices *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct services *services = impl_from_ISWbemServices( iface );
    TRACE( "%p, %u, %u, %p\n", services, index, lcid, info );

    return get_typeinfo( ISWbemServices_tid, info );
}

static HRESULT WINAPI services_GetIDsOfNames(
    ISWbemServices *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct services *services = impl_from_ISWbemServices( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %u, %p\n", services, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemServices_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
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
    struct services *services = impl_from_ISWbemServices( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", services, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemServices_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &services->ISWbemServices_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI services_Get(
    ISWbemServices *iface,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObject **objWbemObject )
{
    struct services *services = impl_from_ISWbemServices( iface );
    IWbemClassObject *obj;
    HRESULT hr;

    TRACE( "%p, %s, %d, %p, %p\n", iface, debugstr_w(strObjectPath), iFlags, objWbemNamedValueSet,
           objWbemObject );

    if (objWbemNamedValueSet) FIXME( "ignoring context\n" );

    hr = IWbemServices_GetObject( services->services, strObjectPath, iFlags, NULL, &obj, NULL );
    if (hr != S_OK) return hr;

    hr = SWbemObject_create( obj, objWbemObject );
    IWbemClassObject_Release( obj );
    return hr;
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
    struct services *services = impl_from_ISWbemServices( iface );
    IEnumWbemClassObject *iter;
    HRESULT hr;

    TRACE( "%p, %s, %s, %x, %p, %p\n", iface, debugstr_w(strQuery), debugstr_w(strQueryLanguage),
           iFlags, objWbemNamedValueSet, objWbemObjectSet );

    if (objWbemNamedValueSet) FIXME( "ignoring context\n" );

    hr = IWbemServices_ExecQuery( services->services, strQueryLanguage, strQuery, iFlags, NULL, &iter );
    if (hr != S_OK) return hr;

    hr = SWbemObjectSet_create( iter, objWbemObjectSet );
    IEnumWbemClassObject_Release( iter );
    return hr;
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

static HRESULT SWbemServices_create( IWbemServices *wbem_services, ISWbemServices **obj )
{
    struct services *services;

    TRACE( "%p, %p\n", obj, wbem_services );

    if (!(services = heap_alloc( sizeof(*services) ))) return E_OUTOFMEMORY;
    services->ISWbemServices_iface.lpVtbl = &services_vtbl;
    services->refs = 1;
    services->services = wbem_services;
    IWbemServices_AddRef( services->services );

    *obj = &services->ISWbemServices_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct locator
{
    ISWbemLocator ISWbemLocator_iface;
    LONG refs;
    IWbemLocator *locator;
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
        IWbemLocator_Release( locator->locator );
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

static BSTR build_resource_string( BSTR server, BSTR namespace )
{
    static const WCHAR defaultW[] = {'r','o','o','t','\\','d','e','f','a','u','l','t',0};
    ULONG len, len_server = 0, len_namespace = 0;
    BSTR ret;

    if (server && *server) len_server = strlenW( server );
    else len_server = 1;
    if (namespace && *namespace) len_namespace = strlenW( namespace );
    else len_namespace = sizeof(defaultW) / sizeof(defaultW[0]) - 1;

    if (!(ret = SysAllocStringLen( NULL, 2 + len_server + 1 + len_namespace ))) return NULL;

    ret[0] = ret[1] = '\\';
    if (server && *server) strcpyW( ret + 2, server );
    else ret[2] = '.';

    len = len_server + 2;
    ret[len++] = '\\';

    if (namespace && *namespace) strcpyW( ret + len, namespace );
    else strcpyW( ret + len, defaultW );
    return ret;
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
    struct locator *locator = impl_from_ISWbemLocator( iface );
    IWbemServices *services;
    BSTR resource;
    HRESULT hr;

    TRACE( "%p, %s, %s, %s, %p, %s, %s, 0x%08x, %p, %p\n", iface, debugstr_w(strServer),
           debugstr_w(strNamespace), debugstr_w(strUser), strPassword, debugstr_w(strLocale),
           debugstr_w(strAuthority), iSecurityFlags, objWbemNamedValueSet, objWbemServices );

    if (objWbemNamedValueSet) FIXME( "context not supported\n" );

    if (!locator->locator)
    {
        hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
                               (void **)&locator->locator );
        if (hr != S_OK) return hr;
    }

    if (!(resource = build_resource_string( strServer, strNamespace ))) return E_OUTOFMEMORY;
    hr = IWbemLocator_ConnectServer( locator->locator, resource, strUser, strPassword, strLocale,
                                     iSecurityFlags, strAuthority, NULL, &services );
    SysFreeString( resource );
    if (hr != S_OK) return hr;

    hr = SWbemServices_create( services, objWbemServices );
    IWbemServices_Release( services );
    return hr;
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
    locator->locator = NULL;

    *obj = &locator->ISWbemLocator_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
