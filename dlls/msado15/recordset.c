/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include "objbase.h"
#include "msado15_backcompat.h"

#include "wine/debug.h"
#include "wine/heap.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

struct recordset
{
    _Recordset  Recordset_iface;
    LONG        refs;
};

static inline struct recordset *impl_from_Recordset( _Recordset *iface )
{
    return CONTAINING_RECORD( iface, struct recordset, Recordset_iface );
}

static ULONG WINAPI recordset_AddRef( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    LONG refs = InterlockedIncrement( &recordset->refs );
    TRACE( "%p new refcount %d\n", recordset, refs );
    return refs;
}

static ULONG WINAPI recordset_Release( _Recordset *iface )
{
    struct recordset *recordset = impl_from_Recordset( iface );
    LONG refs = InterlockedDecrement( &recordset->refs );
    TRACE( "%p new refcount %d\n", recordset, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", recordset );
        heap_free( recordset );
    }
    return refs;
}

static HRESULT WINAPI recordset_QueryInterface( _Recordset *iface, REFIID riid, void **obj )
{
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID__Recordset ) || IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    recordset_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI recordset_GetTypeInfoCount( _Recordset *iface, UINT *count )
{
    FIXME( "%p, %p\n", iface, count );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_GetTypeInfo( _Recordset *iface, UINT index, LCID lcid, ITypeInfo **info )
{
    FIXME( "%p, %u, %u, %p\n", iface, index, lcid, info );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_GetIDsOfNames( _Recordset *iface, REFIID riid, LPOLESTR *names, UINT count,
                                                LCID lcid, DISPID *dispid )
{
    FIXME( "%p, %s, %p, %u, %u, %p\n", iface, debugstr_guid(riid), names, count, lcid, dispid );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Invoke( _Recordset *iface, DISPID member, REFIID riid, LCID lcid, WORD flags,
                                         DISPPARAMS *params, VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    FIXME( "%p, %d, %s, %d, %d, %p, %p, %p, %p\n", iface, member, debugstr_guid(riid), lcid, flags, params,
           result, excep_info, arg_err );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Properties( _Recordset *iface, Properties **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_AbsolutePosition( _Recordset *iface, PositionEnum_Param *pos )
{
    FIXME( "%p, %p\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_AbsolutePosition( _Recordset *iface, PositionEnum_Param pos )
{
    FIXME( "%p, %d\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_putref_ActiveConnection( _Recordset *iface, IDispatch *connection )
{
    FIXME( "%p, %p\n", iface, connection );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_ActiveConnection( _Recordset *iface, VARIANT connection )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&connection) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_ActiveConnection( _Recordset *iface, VARIANT *connection )
{
    FIXME( "%p, %p\n", iface, connection );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_BOF( _Recordset *iface, VARIANT_BOOL *bof )
{
    FIXME( "%p, %p\n", iface, bof );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Bookmark( _Recordset *iface, VARIANT *bookmark )
{
    FIXME( "%p, %p\n", iface, bookmark );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Bookmark( _Recordset *iface, VARIANT bookmark )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&bookmark) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_CacheSize( _Recordset *iface, LONG *size )
{
    FIXME( "%p, %p\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_CacheSize( _Recordset *iface, LONG size )
{
    FIXME( "%p, %d\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_CursorType( _Recordset *iface, CursorTypeEnum *cursor_type )
{
    FIXME( "%p, %p\n", iface, cursor_type );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_CursorType( _Recordset *iface, CursorTypeEnum cursor_type )
{
    FIXME( "%p, %d\n", iface, cursor_type );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_EOF( _Recordset *iface, VARIANT_BOOL *eof )
{
    FIXME( "%p, %p\n", iface, eof );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Fields( _Recordset *iface, Fields **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_LockType( _Recordset *iface, LockTypeEnum *lock_type )
{
    FIXME( "%p, %p\n", iface, lock_type );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_LockType( _Recordset *iface, LockTypeEnum lock_type )
{
    FIXME( "%p, %d\n", iface, lock_type );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_MaxRecords( _Recordset *iface, LONG *max_records )
{
    FIXME( "%p, %p\n", iface, max_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_MaxRecords( _Recordset *iface, LONG max_records )
{
    FIXME( "%p, %d\n", iface, max_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_RecordCount( _Recordset *iface, LONG *count )
{
    FIXME( "%p, %p\n", iface, count );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_putref_Source( _Recordset *iface, IDispatch *source )
{
    FIXME( "%p, %p\n", iface, source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Source( _Recordset *iface, BSTR source )
{
    FIXME( "%p, %s\n", iface, debugstr_w(source) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Source( _Recordset *iface, VARIANT *source )
{
    FIXME( "%p, %p\n", iface, source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_AddNew( _Recordset *iface, VARIANT field_list, VARIANT values )
{
    FIXME( "%p, %s, %s\n", iface, debugstr_variant(&field_list), debugstr_variant(&values) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_CancelUpdate( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Close( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Delete( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_GetRows( _Recordset *iface, LONG rows, VARIANT start, VARIANT fields, VARIANT *var )
{
    FIXME( "%p, %d, %s, %s, %p\n", iface, rows, debugstr_variant(&start), debugstr_variant(&fields), var );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Move( _Recordset *iface, LONG num_records, VARIANT start )
{
    FIXME( "%p, %d, %s\n", iface, num_records, debugstr_variant(&start) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_MoveNext( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_MovePrevious( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_MoveFirst( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_MoveLast( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Open( _Recordset *iface, VARIANT source, VARIANT active_connection,
                                      CursorTypeEnum cursor_type, LockTypeEnum lock_type, LONG options )
{
    FIXME( "%p, %s, %s, %d, %d, %d\n", iface, debugstr_variant(&source), debugstr_variant(&active_connection),
           cursor_type, lock_type, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Requery( _Recordset *iface, LONG options )
{
    FIXME( "%p, %d\n", iface, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset__xResync( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Update( _Recordset *iface, VARIANT fields, VARIANT values )
{
    FIXME( "%p, %s, %s\n", iface, debugstr_variant(&fields), debugstr_variant(&values) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_AbsolutePage( _Recordset *iface, PositionEnum_Param *pos )
{
    FIXME( "%p, %p\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_AbsolutePage( _Recordset *iface, PositionEnum_Param pos )
{
    FIXME( "%p, %d\n", iface, pos );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_EditMode( _Recordset *iface, EditModeEnum *mode )
{
    FIXME( "%p, %p\n", iface, mode );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Filter( _Recordset *iface, VARIANT *criteria )
{
    FIXME( "%p, %p\n", iface, criteria );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Filter( _Recordset *iface, VARIANT criteria )
{
    FIXME( "%p, %s\n", iface, debugstr_variant(&criteria) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_PageCount( _Recordset *iface, LONG *count )
{
    FIXME( "%p, %p\n", iface, count );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_PageSize( _Recordset *iface, LONG *size )
{
    FIXME( "%p, %p\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_PageSize( _Recordset *iface, LONG size )
{
    FIXME( "%p, %d\n", iface, size );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Sort( _Recordset *iface, BSTR *criteria )
{
    FIXME( "%p, %p\n", iface, criteria );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Sort( _Recordset *iface, BSTR criteria )
{
    FIXME( "%p, %s\n", iface, debugstr_w(criteria) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Status( _Recordset *iface, LONG *status )
{
    FIXME( "%p, %p\n", iface, status );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_State( _Recordset *iface, LONG *state )
{
    FIXME( "%p, %p\n", iface, state );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset__xClone( _Recordset *iface, _Recordset **obj )
{
    FIXME( "%p, %p\n", iface, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_UpdateBatch( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_CancelBatch( _Recordset *iface, AffectEnum affect_records )
{
    FIXME( "%p, %u\n", iface, affect_records );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_CursorLocation( _Recordset *iface, CursorLocationEnum *cursor_loc )
{
    FIXME( "%p, %p\n", iface, cursor_loc );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_CursorLocation( _Recordset *iface, CursorLocationEnum cursor_loc )
{
    FIXME( "%p, %u\n", iface, cursor_loc );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_NextRecordset( _Recordset *iface, VARIANT *records_affected, _Recordset **record_set )
{
    FIXME( "%p, %p, %p\n", iface, records_affected, record_set );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Supports( _Recordset *iface, CursorOptionEnum cursor_options, VARIANT_BOOL *ret )
{
    FIXME( "%p, %08x, %p\n", iface, cursor_options, ret );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Collect( _Recordset *iface, VARIANT index, VARIANT *var )
{
    FIXME( "%p, %s, %p\n", iface, debugstr_variant(&index), var );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Collect( _Recordset *iface, VARIANT index, VARIANT var )
{
    FIXME( "%p, %s, %s\n", iface, debugstr_variant(&index), debugstr_variant(&var) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_MarshalOptions( _Recordset *iface, MarshalOptionsEnum *options )
{
    FIXME( "%p, %p\n", iface, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_MarshalOptions( _Recordset *iface, MarshalOptionsEnum options )
{
    FIXME( "%p, %u\n", iface, options );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Find( _Recordset *iface, BSTR criteria, LONG skip_records,
                                      SearchDirectionEnum search_direction, VARIANT start )
{
    FIXME( "%p, %s, %d, %d, %s\n", iface, debugstr_w(criteria), skip_records, search_direction,
           debugstr_variant(&start) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Cancel( _Recordset *iface )
{
    FIXME( "%p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_DataSource( _Recordset *iface, IUnknown **data_source )
{
    FIXME( "%p, %p\n", iface, data_source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_putref_DataSource( _Recordset *iface, IUnknown *data_source )
{
    FIXME( "%p, %p\n", iface, data_source );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset__xSave( _Recordset *iface, BSTR filename, PersistFormatEnum persist_format )
{
    FIXME( "%p, %s, %u\n", iface, debugstr_w(filename), persist_format );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_ActiveCommand( _Recordset *iface, IDispatch **cmd )
{
    FIXME( "%p, %p\n", iface, cmd );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_StayInSync( _Recordset *iface, VARIANT_BOOL stay_in_sync )
{
    FIXME( "%p, %d\n", iface, stay_in_sync );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_StayInSync( _Recordset *iface, VARIANT_BOOL *stay_in_sync )
{
    FIXME( "%p, %p\n", iface, stay_in_sync );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_GetString( _Recordset *iface, StringFormatEnum string_format, LONG num_rows,
                                           BSTR column_delimeter, BSTR row_delimeter, BSTR null_expr,
                                           BSTR *ret_string )
{
    FIXME( "%p, %u, %d, %s, %s, %s, %p\n", iface, string_format, num_rows, debugstr_w(column_delimeter),
           debugstr_w(row_delimeter), debugstr_w(null_expr), ret_string );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_DataMember( _Recordset *iface, BSTR *data_member )
{
    FIXME( "%p, %p\n", iface, data_member );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_DataMember( _Recordset *iface, BSTR data_member )
{
    FIXME( "%p, %s\n", iface, debugstr_w(data_member) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_CompareBookmarks( _Recordset *iface, VARIANT bookmark1, VARIANT bookmark2, CompareEnum *compare )
{
    FIXME( "%p, %s, %s, %p\n", iface, debugstr_variant(&bookmark1), debugstr_variant(&bookmark2), compare );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Clone( _Recordset *iface, LockTypeEnum lock_type, _Recordset **obj )
{
    FIXME( "%p, %d, %p\n", iface, lock_type, obj );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Resync( _Recordset *iface, AffectEnum affect_records, ResyncEnum resync_values )
{
    FIXME( "%p, %u, %u\n", iface, affect_records, resync_values );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Seek( _Recordset *iface, VARIANT key_values, SeekEnum seek_option )
{
    FIXME( "%p, %s, %u\n", iface, debugstr_variant(&key_values), seek_option );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_put_Index( _Recordset *iface, BSTR index )
{
    FIXME( "%p, %s\n", iface, debugstr_w(index) );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_get_Index( _Recordset *iface, BSTR *index )
{
    FIXME( "%p, %p\n", iface, index );
    return E_NOTIMPL;
}

static HRESULT WINAPI recordset_Save( _Recordset *iface, VARIANT destination, PersistFormatEnum persist_format )
{
    FIXME( "%p, %s, %u\n", iface, debugstr_variant(&destination), persist_format );
    return E_NOTIMPL;
}

static const struct _RecordsetVtbl recordset_vtbl =
{
    recordset_QueryInterface,
    recordset_AddRef,
    recordset_Release,
    recordset_GetTypeInfoCount,
    recordset_GetTypeInfo,
    recordset_GetIDsOfNames,
    recordset_Invoke,
    recordset_get_Properties,
    recordset_get_AbsolutePosition,
    recordset_put_AbsolutePosition,
    recordset_putref_ActiveConnection,
    recordset_put_ActiveConnection,
    recordset_get_ActiveConnection,
    recordset_get_BOF,
    recordset_get_Bookmark,
    recordset_put_Bookmark,
    recordset_get_CacheSize,
    recordset_put_CacheSize,
    recordset_get_CursorType,
    recordset_put_CursorType,
    recordset_get_EOF,
    recordset_get_Fields,
    recordset_get_LockType,
    recordset_put_LockType,
    recordset_get_MaxRecords,
    recordset_put_MaxRecords,
    recordset_get_RecordCount,
    recordset_putref_Source,
    recordset_put_Source,
    recordset_get_Source,
    recordset_AddNew,
    recordset_CancelUpdate,
    recordset_Close,
    recordset_Delete,
    recordset_GetRows,
    recordset_Move,
    recordset_MoveNext,
    recordset_MovePrevious,
    recordset_MoveFirst,
    recordset_MoveLast,
    recordset_Open,
    recordset_Requery,
    recordset__xResync,
    recordset_Update,
    recordset_get_AbsolutePage,
    recordset_put_AbsolutePage,
    recordset_get_EditMode,
    recordset_get_Filter,
    recordset_put_Filter,
    recordset_get_PageCount,
    recordset_get_PageSize,
    recordset_put_PageSize,
    recordset_get_Sort,
    recordset_put_Sort,
    recordset_get_Status,
    recordset_get_State,
    recordset__xClone,
    recordset_UpdateBatch,
    recordset_CancelBatch,
    recordset_get_CursorLocation,
    recordset_put_CursorLocation,
    recordset_NextRecordset,
    recordset_Supports,
    recordset_get_Collect,
    recordset_put_Collect,
    recordset_get_MarshalOptions,
    recordset_put_MarshalOptions,
    recordset_Find,
    recordset_Cancel,
    recordset_get_DataSource,
    recordset_putref_DataSource,
    recordset__xSave,
    recordset_get_ActiveCommand,
    recordset_put_StayInSync,
    recordset_get_StayInSync,
    recordset_GetString,
    recordset_get_DataMember,
    recordset_put_DataMember,
    recordset_CompareBookmarks,
    recordset_Clone,
    recordset_Resync,
    recordset_Seek,
    recordset_put_Index,
    recordset_get_Index,
    recordset_Save
};

HRESULT Recordset_create( void **obj )
{
    struct recordset *recordset;

    if (!(recordset = heap_alloc_zero( sizeof(*recordset) ))) return E_OUTOFMEMORY;
    recordset->Recordset_iface.lpVtbl = &recordset_vtbl;
    recordset->refs = 1;

    *obj = &recordset->Recordset_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}
