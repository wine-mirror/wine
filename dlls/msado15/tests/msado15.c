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

#include <stdio.h>
#define COBJMACROS
#include <initguid.h>
#include <oledb.h>
#include <olectl.h>
#include <msado15_backcompat.h>
#include "wine/test.h"
#include "msdasql.h"

DEFINE_GUID(DBPROPSET_ROWSET,            0xc8b522be, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

#define MAKE_ADO_HRESULT( err ) MAKE_HRESULT( SEVERITY_ERROR, FACILITY_CONTROL, err )

static BOOL is_bof( _Recordset *recordset )
{
    VARIANT_BOOL bof = VARIANT_FALSE;
    _Recordset_get_BOF( recordset, &bof );
    return bof == VARIANT_TRUE;
}

static BOOL is_eof( _Recordset *recordset )
{
    VARIANT_BOOL eof = VARIANT_FALSE;
    _Recordset_get_EOF( recordset, &eof );
    return eof == VARIANT_TRUE;
}

static void test_Recordset(void)
{
    _Recordset *recordset;
    IRunnableObject *runtime;
    ISupportErrorInfo *errorinfo;
    Fields *fields, *fields2;
    Field *field;
    Properties *props;
    Property *prop;
    LONG count, state;
    ADO_LONGPTR rec_count, max_records;
    VARIANT missing, val, index;
    CursorLocationEnum location;
    CursorTypeEnum cursor;
    BSTR name;
    HRESULT hr;
    VARIANT bookmark, filter, active;
    EditModeEnum editmode;
    LONG cache_size;

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Recordset_QueryInterface( recordset, &IID_IRunnableObject, (void**)&runtime);
    ok(hr == E_NOINTERFACE, "Unexpected IRunnableObject interface\n");
    ok(runtime == NULL, "expected NULL\n");

    /* _Recordset object supports ISupportErrorInfo */
    errorinfo = NULL;
    hr = _Recordset_QueryInterface( recordset, &IID_ISupportErrorInfo, (void **)&errorinfo );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (errorinfo) ISupportErrorInfo_Release( errorinfo );

    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* calling get_Fields again returns the same object */
    hr = _Recordset_get_Fields( recordset, &fields2 );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( fields2 == fields, "expected same object\n" );
    Fields_Release( fields );

    count = -1;
    hr = Fields_get_Count( fields2, &count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !count, "got %ld\n", count );

    hr = _Recordset_Close( recordset );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    Fields_Release( fields2 );
    ok( !_Recordset_Release( recordset ), "_Recordset not released\n" );

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08lx\n", hr );

    state = -1;
    hr = _Recordset_get_State( recordset, &state );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( state == adStateClosed, "got %ld\n", state );

    location = -1;
    hr = _Recordset_get_CursorLocation( recordset, &location );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( location == adUseServer, "got %d\n", location );

    cursor = adOpenUnspecified;
    hr = _Recordset_get_CursorType( recordset, &cursor );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cursor == adOpenForwardOnly, "got %d\n", cursor );

    cache_size = 0;
    hr = _Recordset_get_CacheSize( recordset, &cache_size );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cache_size == 1, "got %ld\n", cache_size );

    hr = _Recordset_put_CacheSize( recordset, 5 );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Recordset_get_CacheSize( recordset, &cache_size );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cache_size == 5, "got %ld\n", cache_size );

    hr = _Recordset_put_CacheSize( recordset, 1 );
    ok( hr == S_OK, "got %08lx\n", hr );

    max_records = 0;
    hr = _Recordset_get_MaxRecords( recordset, &max_records );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( max_records == 0, "got %Id\n", max_records );

    hr = _Recordset_put_MaxRecords( recordset, 5 );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Recordset_get_MaxRecords( recordset, &max_records );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( max_records == 5, "got %Id\n", max_records );

    hr = _Recordset_put_MaxRecords( recordset, 0 );
    ok( hr == S_OK, "got %08lx\n", hr );

    editmode = -1;
    hr = _Recordset_get_EditMode( recordset, &editmode );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    VariantInit( &bookmark );
    hr = _Recordset_get_Bookmark( recordset, &bookmark );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    VariantInit( &bookmark );
    hr = _Recordset_put_Bookmark( recordset, bookmark );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    VariantInit( &filter );
    hr = _Recordset_put_Filter( recordset, filter );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidArgument ), "got %08lx\n", hr );

    V_VT(&filter) = VT_BSTR;
    V_BSTR(&filter) = SysAllocString( L"field1 = 1" );
    hr = _Recordset_put_Filter( recordset, filter );
    ok( hr == S_OK, "got %08lx\n", hr );
    VariantClear(&filter);

    V_VT(&filter) = VT_I4;
    V_I4(&filter) = 0;
    hr = _Recordset_put_Filter( recordset, filter );
    ok( hr == S_OK, "got %08lx\n", hr );

    V_VT(&filter) = VT_I2;
    V_I2(&filter) = 0;
    hr = _Recordset_put_Filter( recordset, filter );
    ok( hr == S_OK, "got %08lx\n", hr );

    VariantInit( &missing );
    hr = _Recordset_AddNew( recordset, missing, missing );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    V_VT( &missing ) = VT_ERROR;
    V_ERROR( &missing ) = DISP_E_PARAMNOTFOUND;
    hr = _Recordset_Open( recordset, missing, missing, adOpenStatic, adLockBatchOptimistic, adCmdUnspecified );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidConnection ), "got %08lx\n", hr );

    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08lx\n", hr );

    V_VT( &missing ) = VT_ERROR;
    V_ERROR( &missing ) = DISP_E_PARAMNOTFOUND;
    hr = _Recordset_Open( recordset, missing, missing, adOpenStatic, adLockBatchOptimistic, adCmdUnspecified );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidConnection ), "got %08lx\n", hr );

    name = SysAllocString( L"field" );
    hr = Fields__Append( fields, name, adInteger, 4, adFldUnspecified );
    ok( hr == S_OK, "got %08lx\n", hr );

    V_VT( &index ) = VT_I4;
    V_I4( &index ) = 1000;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == MAKE_ADO_HRESULT(adErrItemNotFound), "got %08lx\n", hr );

    V_VT( &index ) = VT_I4;
    V_I4( &index ) = 0;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08lx\n", hr );
    Field_Release(field);

    V_VT( &index ) = VT_I2;
    V_I4( &index ) = 0;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08lx\n", hr );
    Field_Release(field);

    V_VT( &index ) = VT_I1;
    V_I1( &index ) = 0;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08lx\n", hr );
    Field_Release(field);

    V_VT( &index ) = VT_R8;
    V_R8( &index ) = 0.1;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08lx\n", hr );
    Field_Release(field);

    V_VT( &index ) = VT_UNKNOWN;
    V_UNKNOWN( &index ) = NULL;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == MAKE_ADO_HRESULT(adErrItemNotFound), "got %08lx\n", hr );

    V_VT( &index ) = VT_BSTR;
    V_BSTR( &index ) = name;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08lx\n", hr );
    SysFreeString( name );

    hr = Field_QueryInterface(field, &IID_Properties, (void**)&props);
    ok( hr == E_NOINTERFACE, "got %08lx\n", hr );

    hr = Field_get_Properties(field, &props);
    ok( hr == S_OK, "got %08lx\n", hr );

    count = -1;
    hr = Properties_get_Count(props, &count);
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !count, "got %ld\n", count );

    V_VT( &index ) = VT_I4;
    V_I4( &index ) = 1000;
    hr = Properties_get_Item(props, index, &prop);
    ok( hr == MAKE_ADO_HRESULT(adErrItemNotFound), "got %08lx\n", hr );

    Properties_Release(props);

    hr = _Recordset_Open( recordset, missing, missing, adOpenStatic, adLockBatchOptimistic, adCmdUnspecified );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( is_eof( recordset ), "not eof\n" );
    ok( is_bof( recordset ), "not bof\n" );

if (0)
{   /* Causes a crash */
    hr = _Recordset_get_ActiveConnection( recordset, NULL );
}

    V_VT(&active) = VT_UNKNOWN;
    V_UNKNOWN(&active) = (IUnknown *)0xdeadbeef;
    hr = _Recordset_get_ActiveConnection( recordset, &active );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( V_VT(&active) == VT_DISPATCH, "got %d\n", V_VT(&active) );
    ok( V_DISPATCH(&active) == NULL, "got %p\n", V_DISPATCH(&active) );
    VariantClear(&active);

    editmode = -1;
    hr = _Recordset_get_EditMode( recordset, &editmode );
    ok( hr == MAKE_ADO_HRESULT( adErrNoCurrentRecord ), "got %08lx\n", hr );

    hr = _Recordset_Open( recordset, missing, missing, adOpenStatic, adLockBatchOptimistic, adCmdUnspecified );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectOpen ), "got %08lx\n", hr );

    state = -1;
    hr = _Recordset_get_State( recordset, &state );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( state == adStateOpen, "got %ld\n", state );

    VariantInit( &bookmark );
    hr = _Recordset_get_Bookmark( recordset, &bookmark );
    ok( hr == MAKE_ADO_HRESULT( adErrNoCurrentRecord ), "got %08lx\n", hr );

    VariantInit( &bookmark );
    hr = _Recordset_put_Bookmark( recordset, bookmark );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidArgument ), "got %08lx\n", hr );

    rec_count = -1;
    hr = _Recordset_get_RecordCount( recordset, &rec_count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !rec_count, "got %Id\n", rec_count );

    hr = _Recordset_AddNew( recordset, missing, missing );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    editmode = -1;
    hr = _Recordset_get_EditMode( recordset, &editmode );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( editmode == adEditAdd, "got %d\n", editmode );

    rec_count = -1;
    hr = _Recordset_get_RecordCount( recordset, &rec_count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( rec_count == 1, "got %Id\n", rec_count );

    /* get_Fields still returns the same object */
    hr = _Recordset_get_Fields( recordset, &fields2 );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( fields2 == fields, "expected same object\n" );
    Fields_Release( fields2 );

    V_VT(&filter) = VT_BSTR;
    V_BSTR(&filter) = SysAllocString( L"field1 = 1" );
    hr = _Recordset_put_Filter( recordset, filter );
    todo_wine ok( hr == MAKE_ADO_HRESULT( adErrItemNotFound ), "got %08lx\n", hr );
    VariantClear(&filter);

    V_VT(&filter) = VT_BSTR;
    V_BSTR(&filter) = SysAllocString( L"field = 1" );
    hr = _Recordset_put_Filter( recordset, filter );
    ok( hr == S_OK, "got %08lx\n", hr );
    VariantClear(&filter);

    V_VT(&filter) = VT_I4;
    V_I4(&filter) = 0;
    hr = _Recordset_put_Filter( recordset, filter );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = -1;
    hr = Fields_get_Count( fields2, &count );
    ok( count == 1, "got %ld\n", count );

    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( V_VT( &val ) == VT_EMPTY, "got %u\n", V_VT( &val  ) );

    V_VT( &val ) = VT_I4;
    V_I4( &val ) = -1;
    hr = Field_put_Value( field, val );
    ok( hr == S_OK, "got %08lx\n", hr );

    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( V_VT( &val ) == VT_I4, "got %u\n", V_VT( &val ) );
    ok( V_I4( &val ) == -1, "got %ld\n", V_I4( &val ) );

    /* Update/Cancel doesn't update EditMode when no active connection. */
    hr = _Recordset_Update( recordset, missing, missing );
    ok( hr == S_OK, "got %08lx\n", hr );

    editmode = -1;
    hr = _Recordset_get_EditMode( recordset, &editmode );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( editmode == adEditAdd, "got %d\n", editmode );

    hr = _Recordset_Cancel( recordset );
    ok( hr == S_OK, "got %08lx\n", hr );

    editmode = -1;
    hr = _Recordset_get_EditMode( recordset, &editmode );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( editmode == adEditAdd, "got %d\n", editmode );

    hr = _Recordset_AddNew( recordset, missing, missing );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* field object returns different value after AddNew */
    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( V_VT( &val ) == VT_EMPTY, "got %u\n", V_VT( &val ) );

    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );
    hr = _Recordset_MoveFirst( recordset );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( V_VT( &val ) == VT_I4, "got %u\n", V_VT( &val ) );
    ok( V_I4( &val ) == -1, "got %ld\n", V_I4( &val ) );

    hr = _Recordset_MoveNext( recordset );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "not bof\n" );

    hr = _Recordset_MoveNext( recordset );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( is_eof( recordset ), "not eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    hr = _Recordset_MoveFirst( recordset );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    hr = _Recordset_MovePrevious( recordset );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( is_bof( recordset ), "not bof\n" );

    /* try get value at BOF */
    VariantInit( &val );
    hr = Field_get_Value( field, &val );
    ok( hr == MAKE_ADO_HRESULT( adErrNoCurrentRecord ), "got %08lx\n", hr );

    hr = _Recordset_Close( recordset );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( !count, "got %ld\n", count );

    hr = Field_get_Name(field, &name);
    todo_wine ok( hr == MAKE_ADO_HRESULT( adErrObjectNotSet ), "got %08lx\n", hr );

    state = -1;
    hr = _Recordset_get_State( recordset, &state );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( state == adStateClosed, "got %ld\n", state );

    ok (!Field_Release( field ), "Field not released\n" );
    Fields_Release( fields );
    ok( !_Recordset_Release( recordset ), "_Recordset not released\n" );
}

/* This interface is queried for but is not documented anywhere. */
DEFINE_GUID(UKN_INTERFACE, 0x6f1e39e1, 0x05c6, 0x11d0, 0xa7, 0x8b, 0x00, 0xaa, 0x00, 0xa3, 0xf0, 0x0d);

struct test_rowset
{
    IRowset IRowset_iface;
    IRowsetInfo IRowsetInfo_iface;
    IColumnsInfo IColumnsInfo_iface;
    LONG refs;
};

static inline struct test_rowset *impl_from_IRowset( IRowset *iface )
{
    return CONTAINING_RECORD( iface, struct test_rowset, IRowset_iface );
}

static inline struct test_rowset *impl_from_IRowsetInfo( IRowsetInfo *iface )
{
    return CONTAINING_RECORD( iface, struct test_rowset, IRowsetInfo_iface );
}

static inline struct test_rowset *impl_from_IColumnsInfo( IColumnsInfo *iface )
{
    return CONTAINING_RECORD( iface, struct test_rowset, IColumnsInfo_iface );
}

static HRESULT WINAPI rowset_info_QueryInterface(IRowsetInfo *iface, REFIID riid, void **obj)
{
    struct test_rowset *rowset = impl_from_IRowsetInfo( iface );
    return IRowset_QueryInterface(&rowset->IRowset_iface, riid, obj);
}

static ULONG WINAPI rowset_info_AddRef(IRowsetInfo *iface)
{
    struct test_rowset *rowset = impl_from_IRowsetInfo( iface );
    return IRowset_AddRef(&rowset->IRowset_iface);
}

static ULONG WINAPI rowset_info_Release(IRowsetInfo *iface)
{
    struct test_rowset *rowset = impl_from_IRowsetInfo( iface );
    return IRowset_Release(&rowset->IRowset_iface);
}

static HRESULT WINAPI rowset_info_GetProperties(IRowsetInfo *iface, const ULONG count,
        const DBPROPIDSET propertyidsets[], ULONG *out_count, DBPROPSET **propertysets1)
{
    ok( count == 2, "got %ld\n", count );

    ok( IsEqualIID(&DBPROPSET_ROWSET, &propertyidsets[0].guidPropertySet), "got %s\n", wine_dbgstr_guid(&propertyidsets[0].guidPropertySet));
    ok( propertyidsets[0].cPropertyIDs == 17, "got %ld\n", propertyidsets[0].cPropertyIDs );

    ok( IsEqualIID(&DBPROPSET_PROVIDERROWSET, &propertyidsets[1].guidPropertySet), "got %s\n", wine_dbgstr_guid(&propertyidsets[1].guidPropertySet));
    ok( propertyidsets[1].cPropertyIDs == 1, "got %ld\n", propertyidsets[1].cPropertyIDs );

    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_info_GetReferencedRowset(IRowsetInfo *iface, DBORDINAL ordinal,
        REFIID riid, IUnknown **unk)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_info_GetSpecification(IRowsetInfo *iface, REFIID riid,
        IUnknown **specification)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static const struct IRowsetInfoVtbl rowset_info =
{
    rowset_info_QueryInterface,
    rowset_info_AddRef,
    rowset_info_Release,
    rowset_info_GetProperties,
    rowset_info_GetReferencedRowset,
    rowset_info_GetSpecification
};

static HRESULT WINAPI column_info_QueryInterface(IColumnsInfo *iface, REFIID riid, void **obj)
{
    struct test_rowset *rowset = impl_from_IColumnsInfo( iface );
    return IRowset_QueryInterface(&rowset->IRowset_iface, riid, obj);
}

static ULONG WINAPI column_info_AddRef(IColumnsInfo *iface)
{
    struct test_rowset *rowset = impl_from_IColumnsInfo( iface );
    return IRowset_AddRef(&rowset->IRowset_iface);
}

static ULONG WINAPI column_info_Release(IColumnsInfo *iface)
{
    struct test_rowset *rowset = impl_from_IColumnsInfo( iface );
    return IRowset_Release(&rowset->IRowset_iface);
}

static HRESULT WINAPI column_info_GetColumnInfo(IColumnsInfo *This, DBORDINAL *columns,
        DBCOLUMNINFO **colinfo, OLECHAR **stringsbuffer)
{
    DBCOLUMNINFO *dbcolumn;
    *columns = 1;

    *stringsbuffer = CoTaskMemAlloc(sizeof(L"Column1"));
    lstrcpyW(*stringsbuffer, L"Column1");

    dbcolumn = CoTaskMemAlloc(sizeof(DBCOLUMNINFO));

    dbcolumn->pwszName = *stringsbuffer;
    dbcolumn->pTypeInfo = NULL;
    dbcolumn->iOrdinal = 1;
    dbcolumn->dwFlags = DBCOLUMNFLAGS_MAYBENULL;
    dbcolumn->ulColumnSize = 5;
    dbcolumn->wType = DBTYPE_I4;
    dbcolumn->bPrecision = 1;
    dbcolumn->bScale = 1;
    dbcolumn->columnid.eKind = DBKIND_NAME;
    dbcolumn->columnid.uName.pwszName = *stringsbuffer;

    *colinfo = dbcolumn;

    return S_OK;
}

static HRESULT WINAPI column_info_MapColumnIDs(IColumnsInfo *This, DBORDINAL column_ids,
        const DBID *dbids, DBORDINAL *columns)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static const struct IColumnsInfoVtbl column_info =
{
    column_info_QueryInterface,
    column_info_AddRef,
    column_info_Release,
    column_info_GetColumnInfo,
    column_info_MapColumnIDs,
};

static HRESULT WINAPI rowset_QueryInterface(IRowset *iface, REFIID riid, void **obj)
{
    struct test_rowset *rowset = impl_from_IRowset( iface );

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IRowset) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        trace("Requested interface IID_IRowset\n");
        *obj = &rowset->IRowset_iface;
    }
    else if (IsEqualIID(riid, &IID_IRowsetInfo))
    {
        trace("Requested interface IID_IRowsetInfo\n");
        *obj = &rowset->IRowsetInfo_iface;
    }
    else if (IsEqualIID(riid, &IID_IColumnsInfo))
    {
        trace("Requested interface IID_IColumnsInfo\n");
        *obj = &rowset->IColumnsInfo_iface;
    }
    else if (IsEqualIID(riid, &IID_IRowsetLocate))
    {
        trace("Requested interface IID_IRowsetLocate\n");
        return E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, &IID_IDBAsynchStatus))
    {
        trace("Requested interface IID_IDBAsynchStatus\n");
        return E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, &IID_IAccessor))
    {
        trace("Requested interface IID_IAccessor\n");
        return E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, &UKN_INTERFACE))
    {
        trace("Unknown interface\n");
        return E_NOINTERFACE;
    }

    if(*obj) {
        IUnknown_AddRef((IUnknown*)*obj);
        return S_OK;
    }

    ok(0, "Unsupported interface %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI rowset_AddRef(IRowset *iface)
{
    struct test_rowset *rowset = impl_from_IRowset( iface );
    return InterlockedIncrement( &rowset->refs );
}

static ULONG WINAPI rowset_Release(IRowset *iface)
{
    struct test_rowset *rowset = impl_from_IRowset( iface );
    /* Object not allocated no need to destroy */
    return InterlockedDecrement( &rowset->refs );
}

static HRESULT WINAPI rowset_AddRefRows(IRowset *iface, DBCOUNTITEM cRows, const HROW rghRows[],
    DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetData(IRowset *iface, HROW hRow, HACCESSOR hAccessor, void *pData)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_GetNextRows(IRowset *iface, HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
    DBROWCOUNT cRows, DBCOUNTITEM *pcRowObtained, HROW **prghRows)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_ReleaseRows(IRowset *iface, DBCOUNTITEM cRows, const HROW rghRows[],
    DBROWOPTIONS rgRowOptions[], DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI rowset_RestartPosition(IRowset *iface, HCHAPTER hReserved)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static const struct IRowsetVtbl rowset_vtbl =
{
    rowset_QueryInterface,
    rowset_AddRef,
    rowset_Release,
    rowset_AddRefRows,
    rowset_GetData,
    rowset_GetNextRows,
    rowset_ReleaseRows,
    rowset_RestartPosition
};

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static void test_ADORecordsetConstruction(void)
{
    _Recordset *recordset;
    ADORecordsetConstruction *construct;
    Fields *fields = NULL;
    Field *field;
    struct test_rowset testrowset;
    IRowset *rowset;
    HRESULT hr;
    LONG ref, count;

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Recordset_QueryInterface( recordset, &IID_ADORecordsetConstruction, (void**)&construct );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( fields != NULL, "NULL value\n");

    testrowset.IRowset_iface.lpVtbl = &rowset_vtbl;
    testrowset.IRowsetInfo_iface.lpVtbl = &rowset_info;
    testrowset.IColumnsInfo_iface.lpVtbl = &column_info;
    testrowset.refs = 1;

    rowset = &testrowset.IRowset_iface;

    ref = get_refcount( rowset );
    ok( ref == 1, "got %ld\n", ref );
    hr = ADORecordsetConstruction_put_Rowset( construct, (IUnknown*)rowset );
    ok( hr == S_OK, "got %08lx\n", hr );

    ref = get_refcount( rowset );
    ok( ref == 2, "got %ld\n", ref );

    ref = get_refcount( rowset );
    ok( ref == 2, "got %ld\n", ref );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( count == 1, "got %ld\n", count );
    if (count > 0)
    {
        unsigned char prec, scale;
        VARIANT index;
        ADO_LONGPTR size;
        DataTypeEnum type;

        V_VT( &index ) = VT_BSTR;
        V_BSTR( &index ) = SysAllocString( L"Column1" );

        hr = Fields_get_Item( fields, index, &field );
        VariantClear(&index);
        ok( hr == S_OK, "got %08lx\n", hr );

        hr = Field_get_Type( field, &type );
        ok( hr == S_OK, "got %08lx\n", hr );
        ok( type == adInteger, "got %d\n", type );
        size = -1;
        hr = Field_get_DefinedSize( field, &size );
        ok( hr == S_OK, "got %08lx\n", hr );
        ok( size == 5, "got %Id\n", size );
        hr = Field_get_Precision( field, &prec );
        ok( hr == S_OK, "got %08lx\n", hr );
        ok( prec == 1, "got %u\n", prec );
        hr = Field_get_NumericScale( field, &scale );
        ok( hr == S_OK, "got %08lx\n", hr );
        ok( scale == 1, "got %u\n", scale );

        Field_Release(field);
    }

    ref = get_refcount(rowset);
    ok( ref == 2, "got %ld\n", ref );

    Fields_Release(fields);

    ADORecordsetConstruction_Release(construct);

    ok( !_Recordset_Release( recordset ), "_Recordset not released\n" );
}

static void test_Fields(void)
{
    _Recordset *recordset;
    ISupportErrorInfo *errorinfo;
    unsigned char prec, scale;
    Fields *fields;
    Field *field, *field2;
    VARIANT val, index;
    BSTR name;
    LONG count;
    ADO_LONGPTR size;
    DataTypeEnum type;
    LONG attrs;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Recordset_QueryInterface( recordset, &IID_Fields, (void **)&fields );
    ok( hr == E_NOINTERFACE, "got %08lx\n", hr );

    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Fields object supports ISupportErrorInfo */
    errorinfo = NULL;
    hr = Fields_QueryInterface( fields, &IID_ISupportErrorInfo, (void **)&errorinfo );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (errorinfo) ISupportErrorInfo_Release( errorinfo );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( !count, "got %ld\n", count );

    name = SysAllocString( L"field" );
    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Fields_Append( fields, name, adInteger, 4, adFldUnspecified, val );
    ok( hr == S_OK, "got %08lx\n", hr );
    SysFreeString( name );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( count == 1, "got %ld\n", count );

    name = SysAllocString( L"field2" );
    hr = Fields__Append( fields, name, adInteger, 4, adFldUnspecified );
    ok( hr == S_OK, "got %08lx\n", hr );
    SysFreeString( name );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( count == 2, "got %ld\n", count );

    /* handing out field object doesn't add reference to fields or recordset object */
    name = SysAllocString( L"field" );
    V_VT( &index ) = VT_BSTR;
    V_BSTR( &index ) = name;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* calling get_Item again returns the same object and adds reference */
    hr = Fields_get_Item( fields, index, &field2 );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( field2 == field, "expected same object\n" );
    Field_Release( field2 );
    SysFreeString( name );

    /* Field object supports ISupportErrorInfo */
    errorinfo = NULL;
    hr = Field_QueryInterface( field, &IID_ISupportErrorInfo, (void **)&errorinfo );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (errorinfo) ISupportErrorInfo_Release( errorinfo );

    /* verify values set with _Append */
    hr = Field_get_Name( field, &name );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !lstrcmpW( name, L"field" ), "got %s\n", wine_dbgstr_w(name) );
    SysFreeString( name );
    type = 0xdead;
    hr = Field_get_Type( field, &type );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( type == adInteger, "got %d\n", type );
    size = -1;
    hr = Field_get_DefinedSize( field, &size );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( size == 4, "got %Id\n", size );
    attrs = 0xdead;
    hr = Field_get_Attributes( field, &attrs );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !attrs, "got %ld\n", attrs );
    hr = Field_get_Precision( field, &prec );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !prec, "got %u\n", prec );
    hr = Field_get_NumericScale( field, &scale );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !scale, "got %u\n", scale );

    hr = Field_put_Precision( field, 7 );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = Field_get_Precision( field, &prec );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( prec == 7, "got %u\n", prec );

    hr = Field_put_NumericScale( field, 12 );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = Field_get_NumericScale( field, &scale );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( scale == 12, "got %u\n", scale );

    Field_Release( field );
    Fields_Release( fields );
    ok( !_Recordset_Release( recordset ), "_Recordset not released\n" );
}

static HRESULT str_to_byte_array( const char *data, VARIANT *ret )
{
    SAFEARRAY *vector;
    LONG i, len = strlen(data);
    HRESULT hr;

    if (!(vector = SafeArrayCreateVector( VT_UI1, 0, len ))) return E_OUTOFMEMORY;
    for (i = 0; i < len; i++)
    {
        if ((hr = SafeArrayPutElement( vector, &i, (void *)&data[i] )) != S_OK)
        {
            SafeArrayDestroy( vector );
            return hr;
        }
    }

    V_VT( ret ) = VT_ARRAY | VT_UI1;
    V_ARRAY( ret ) = vector;
    return S_OK;
}

static void test_Stream(void)
{
    _Stream *stream;
    VARIANT_BOOL eos;
    StreamTypeEnum type;
    LineSeparatorEnum sep;
    LONG refs;
    ADO_LONGPTR size, pos;
    ObjectStateEnum state;
    ConnectModeEnum mode;
    BSTR charset, str;
    VARIANT missing, val;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Stream, NULL, CLSCTX_INPROC_SERVER, &IID__Stream, (void **)&stream );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Stream_get_Size( stream, &size );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    hr = _Stream_get_EOS( stream, &eos );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    hr = _Stream_get_Position( stream, &pos );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    hr = _Stream_put_Position( stream, 0 );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    /* check default type */
    type = 0;
    hr = _Stream_get_Type( stream, &type );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( type == adTypeText, "got %u\n", type );

    hr = _Stream_put_Type( stream, adTypeBinary );
    ok( hr == S_OK, "got %08lx\n", hr );

    type = 0;
    hr = _Stream_get_Type( stream, &type );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( type == adTypeBinary, "got %u\n", type );

    /* revert */
    hr = _Stream_put_Type( stream, adTypeText );
    ok( hr == S_OK, "got %08lx\n", hr );

    sep = 0;
    hr = _Stream_get_LineSeparator( stream, &sep );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( sep == adCRLF, "got %d\n", sep );

    hr = _Stream_put_LineSeparator( stream, adLF );
    ok( hr == S_OK, "got %08lx\n", hr );

    state = 0xdeadbeef;
    hr = _Stream_get_State( stream, &state );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( state == adStateClosed, "got %u\n", state );

    mode = 0xdeadbeef;
    hr = _Stream_get_Mode( stream, &mode );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( mode == adModeUnknown, "got %u\n", mode );

    hr = _Stream_put_Mode( stream, adModeReadWrite );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Stream_get_Charset( stream, &charset );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !lstrcmpW( charset, L"Unicode" ), "got %s\n", wine_dbgstr_w(charset) );
    SysFreeString( charset );

    str = SysAllocString( L"Unicode" );
    hr = _Stream_put_Charset( stream, str );
    ok( hr == S_OK, "got %08lx\n", hr );
    SysFreeString( str );

    hr = _Stream_Read( stream, 2, &val );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    V_VT( &missing ) = VT_ERROR;
    V_ERROR( &missing ) = DISP_E_PARAMNOTFOUND;
    hr = _Stream_Open( stream, missing, adModeUnknown, adOpenStreamUnspecified, NULL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Stream_Open( stream, missing, adModeUnknown, adOpenStreamUnspecified, NULL, NULL );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectOpen ), "got %08lx\n", hr );

    state = 0xdeadbeef;
    hr = _Stream_get_State( stream, &state );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( state == adStateOpen, "got %u\n", state );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !size, "got %Id\n", size );

    eos = VARIANT_FALSE;
    hr = _Stream_get_EOS( stream, &eos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( eos == VARIANT_TRUE, "got %04x\n", eos );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !pos, "got %Id\n", pos );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !size, "got %Id\n", size );

    hr = _Stream_Read( stream, 2, &val );
    ok( hr == MAKE_ADO_HRESULT( adErrIllegalOperation ), "got %08lx\n", hr );

    hr = _Stream_ReadText( stream, 2, &str );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !str[0], "got %s\n", wine_dbgstr_w(str) );
    SysFreeString( str );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !pos, "got %Id\n", pos );

    str = SysAllocString( L"test" );
    hr = _Stream_WriteText( stream, str, adWriteChar );
    ok( hr == S_OK, "got %08lx\n", hr );
    SysFreeString( str );

    hr = _Stream_ReadText( stream, adReadAll, &str );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !str[0], "got %s\n", wine_dbgstr_w(str) );
    SysFreeString( str );

    hr = _Stream_put_Position( stream, 0 );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = _Stream_ReadText( stream, adReadAll, &str );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !lstrcmpW( str, L"test" ), "got %s\n", wine_dbgstr_w(str) );
    SysFreeString( str );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( pos == 10, "got %Id\n", pos );

    eos = VARIANT_FALSE;
    hr = _Stream_get_EOS( stream, &eos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( eos == VARIANT_TRUE, "got %04x\n", eos );

    hr = _Stream_put_Position( stream, 6 );
    ok( hr == S_OK, "got %08lx\n", hr );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( size == 10, "got %Id\n", size );

    hr = _Stream_put_Position( stream, 2 );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Stream_SetEOS( stream );
    ok( hr == S_OK, "got %08lx\n", hr );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( pos == 2, "got %Id\n", pos );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( size == 2, "got %Id\n", size );

    hr = _Stream_Close( stream );
    ok( hr == S_OK, "got %08lx\n", hr );

    state = 0xdeadbeef;
    hr = _Stream_get_State( stream, &state );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( state == adStateClosed, "got %u\n", state );

    hr = _Stream_Close( stream );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08lx\n", hr );

    refs = _Stream_Release( stream );
    ok( !refs, "got %ld\n", refs );

    /* binary type */
    hr = CoCreateInstance( &CLSID_Stream, NULL, CLSCTX_INPROC_SERVER, &IID__Stream, (void **)&stream );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Stream_put_Type( stream, adTypeBinary );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Stream_Open( stream, missing, adModeUnknown, adOpenStreamUnspecified, NULL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Stream_ReadText( stream, adReadAll, &str );
    ok( hr == MAKE_ADO_HRESULT( adErrIllegalOperation ), "got %08lx\n", hr );

    str = SysAllocString( L"test" );
    hr = _Stream_WriteText( stream, str, adWriteChar );
    ok( hr == MAKE_ADO_HRESULT( adErrIllegalOperation ), "got %08lx\n", hr );
    SysFreeString( str );

    VariantInit( &val );
    hr = _Stream_Read( stream, 1, &val );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( V_VT( &val ) == VT_NULL, "got %u\n", V_VT( &val ) );

    VariantInit( &val );
    hr = _Stream_Write( stream, val );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidArgument ), "got %08lx\n", hr );

    hr = str_to_byte_array( "data", &val );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = _Stream_Write( stream, val );
    ok( hr == S_OK, "got %08lx\n", hr );
    VariantClear( &val );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( pos == 4, "got %Id\n", pos );

    hr = _Stream_put_Position( stream, 0 );
    ok( hr == S_OK, "got %08lx\n", hr );

    VariantInit( &val );
    hr = _Stream_Read( stream, adReadAll, &val );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( V_VT( &val ) == (VT_ARRAY | VT_UI1), "got %04x\n", V_VT( &val ) );
    VariantClear( &val );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( pos == 4, "got %Id\n", pos );

    refs = _Stream_Release( stream );
    ok( !refs, "got %ld\n", refs );
}

static void test_Connection(void)
{
    HRESULT hr;
    _Connection *connection;
    IRunnableObject *runtime;
    ISupportErrorInfo *errorinfo;
    IConnectionPointContainer *pointcontainer;
    ADOConnectionConstruction15 *construct;
    LONG state, timeout;
    BSTR str, str2, str3;
    ConnectModeEnum mode;
    CursorLocationEnum location;

    hr = CoCreateInstance(&CLSID_Connection, NULL, CLSCTX_INPROC_SERVER, &IID__Connection, (void**)&connection);
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Connection_QueryInterface(connection, &IID_IRunnableObject, (void**)&runtime);
    ok(hr == E_NOINTERFACE, "Unexpected IRunnableObject interface\n");
    ok(runtime == NULL, "expected NULL\n");

    hr = _Connection_QueryInterface(connection, &IID_ISupportErrorInfo, (void**)&errorinfo);
    ok(hr == S_OK, "Failed to get ISupportErrorInfo interface\n");
    ISupportErrorInfo_Release(errorinfo);

    hr = _Connection_QueryInterface(connection, &IID_IConnectionPointContainer, (void**)&pointcontainer);
    ok(hr == S_OK, "Failed to get IConnectionPointContainer interface %08lx\n", hr);
    IConnectionPointContainer_Release(pointcontainer);

    hr = _Connection_QueryInterface(connection, &IID_ADOConnectionConstruction15, (void**)&construct);
    ok(hr == S_OK, "Failed to get ADOConnectionConstruction15 interface %08lx\n", hr);
    if (hr == S_OK)
        ADOConnectionConstruction15_Release(construct);

if (0)   /* Crashes on windows */
{
    hr = _Connection_get_State(connection, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr 0x%08lx\n", hr);
}

    state = -1;
    hr = _Connection_get_State(connection, &state);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);
    ok(state == adStateClosed, "Unexpected state value 0x%08lx\n", state);

    hr = _Connection_Close(connection);
    ok(hr == MAKE_ADO_HRESULT(adErrObjectClosed), "got %08lx\n", hr);

    timeout = 0;
    hr = _Connection_get_CommandTimeout(connection, &timeout);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);
    ok(timeout == 30, "Unexpected timeout value %ld\n", timeout);

    hr = _Connection_put_CommandTimeout(connection, 300);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);

    timeout = 0;
    hr = _Connection_get_CommandTimeout(connection, &timeout);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);
    ok(timeout == 300, "Unexpected timeout value %ld\n", timeout);

    location = 0;
    hr = _Connection_get_CursorLocation(connection, &location);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    ok(location == adUseServer, "Unexpected location value %d\n", location);

    hr = _Connection_put_CursorLocation(connection, adUseClient);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);

    location = 0;
    hr = _Connection_get_CursorLocation(connection, &location);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    ok(location == adUseClient, "Unexpected location value %d\n", location);

    hr = _Connection_put_CursorLocation(connection, adUseServer);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);

    mode = 0xdeadbeef;
    hr = _Connection_get_Mode(connection, &mode);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);
    ok(mode == adModeUnknown, "Unexpected mode value %d\n", mode);

    hr = _Connection_put_Mode(connection, adModeShareDenyNone);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);

    mode = adModeUnknown;
    hr = _Connection_get_Mode(connection, &mode);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);
    ok(mode == adModeShareDenyNone, "Unexpected mode value %d\n", mode);

    hr = _Connection_put_Mode(connection, adModeUnknown);
    ok(hr == S_OK, "Failed to get state, hr 0x%08lx\n", hr);

    /* Default */
    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_Provider(connection, &str);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    ok(!wcscmp(str, L"MSDASQL"), "wrong string %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = SysAllocString(L"MSDASQL.1");
    hr = _Connection_put_Provider(connection, str);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    SysFreeString(str);

    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_Provider(connection, &str);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    ok(!wcscmp(str, L"MSDASQL.1"), "wrong string %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* Restore default */
    str = SysAllocString(L"MSDASQL");
    hr = _Connection_put_Provider(connection, str);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    SysFreeString(str);

    hr = _Connection_put_Provider(connection, NULL);
    ok(hr == MAKE_ADO_HRESULT(adErrInvalidArgument), "got 0x%08lx\n", hr);

    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_ConnectionString(connection, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(str == NULL, "got %p\n", str);

    str = SysAllocString(L"Provider=MSDASQL.1;Persist Security Info=False;Data Source=wine_test");
    hr = _Connection_put_ConnectionString(connection, str);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);

    /* Show put_ConnectionString effects Provider */
    str3 = (BSTR)0xdeadbeef;
    hr = _Connection_get_Provider(connection, &str3);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    ok(str3 != NULL, "Expected value got NULL\n");
    todo_wine ok(!wcscmp(str3, L"MSDASQL.1"), "wrong string %s\n", wine_dbgstr_w(str3));
    SysFreeString(str3);

if (0) /* Crashes on windows */
{
    hr = _Connection_get_ConnectionString(connection, NULL);
    ok(hr == E_POINTER, "Failed, hr 0x%08lx\n", hr);
}

    str2 = NULL;
    hr = _Connection_get_ConnectionString(connection, &str2);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    ok(!wcscmp(str, str2), "wrong string %s\n", wine_dbgstr_w(str2));
    SysFreeString(str2);

    hr = _Connection_Open(connection, NULL, NULL, NULL, 0);
    ok(hr == E_FAIL, "Failed, hr 0x%08lx\n", hr);

    /* Open adds trailing ; if it's missing */
    str3 = SysAllocString(L"Provider=MSDASQL.1;Persist Security Info=False;Data Source=wine_test;");
    hr = _Connection_Open(connection, NULL, NULL, NULL, adConnectUnspecified);
    ok(hr == E_FAIL, "Failed, hr 0x%08lx\n", hr);

    str2 = NULL;
    hr = _Connection_get_ConnectionString(connection, &str2);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    todo_wine ok(!wcscmp(str3, str2) || broken(!wcscmp(str, str2)) /* XP */, "wrong string %s\n", wine_dbgstr_w(str2));
    SysFreeString(str2);

    hr = _Connection_Open(connection, str, NULL, NULL, adConnectUnspecified);
    ok(hr == E_FAIL, "Failed, hr 0x%08lx\n", hr);

    str2 = NULL;
    hr = _Connection_get_ConnectionString(connection, &str2);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    todo_wine ok(!wcscmp(str3, str2) || broken(!wcscmp(str, str2)) /* XP */, "wrong string %s\n", wine_dbgstr_w(str2));
    SysFreeString(str);
    SysFreeString(str2);
    SysFreeString(str3);

    hr = _Connection_put_ConnectionString(connection, NULL);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);

    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_ConnectionString(connection, &str);
    ok(hr == S_OK, "Failed, hr 0x%08lx\n", hr);
    ok(str == NULL, "got %p\n", str);
    ok(!_Connection_Release(connection), "_Connection not released\n");
}

static void test_Command(void)
{
    HRESULT hr;
    _Command *command;
    _ADO *ado;
    Command15 *command15;
    Command25 *command25;
    CommandTypeEnum cmd_type = adCmdUnspecified;
    BSTR cmd_text = (BSTR)"test";
    _Connection *connection;
    ADOCommandConstruction *adocommand;
    Parameters *parameters, *parameters2;
    _Parameter *parameter = NULL;
    IDispatch *disp;
    BSTR str;
    VARIANT value;

    hr = CoCreateInstance( &CLSID_Command, NULL, CLSCTX_INPROC_SERVER, &IID__Command, (void **)&command );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Command_QueryInterface( command, &IID__ADO, (void **)&ado );
    ok( hr == S_OK, "got %08lx\n", hr );
    _ADO_Release( ado );

    hr = _Command_QueryInterface( command, &IID_Command15, (void **)&command15 );
    ok( hr == S_OK, "got %08lx\n", hr );
    Command15_Release( command15 );

    hr = _Command_QueryInterface( command, &IID_Command25, (void **)&command25 );
    ok( hr == S_OK, "got %08lx\n", hr );
    Command25_Release( command25 );

    hr = _Command_QueryInterface( command, &IID_ADOCommandConstruction, (void **)&adocommand );
    ok( hr == S_OK, "got %08lx\n", hr );
    ADOCommandConstruction_Release( adocommand );

    hr = _Command_QueryInterface( command, &IID_Parameters, (void **)&parameters );
    ok( hr == E_NOINTERFACE, "got %08lx\n", hr );

    hr = _Command_get_CommandType( command, &cmd_type );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cmd_type == adCmdUnknown, "got %08x\n", cmd_type );

    _Command_put_CommandType( command, adCmdText );
    hr = _Command_get_CommandType( command, &cmd_type );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cmd_type == adCmdText, "got %08x\n", cmd_type );

    hr = _Command_put_CommandType( command, 0xdeadbeef );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidArgument ), "got %08lx\n", hr );

    hr = _Command_get_CommandText( command, &cmd_text );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !cmd_text, "got %s\n", wine_dbgstr_w( cmd_text ));

    hr = _Command_put_CommandText( command, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    cmd_text = SysAllocString( L"" );
    hr = _Command_put_CommandText( command, cmd_text );
    ok( hr == S_OK, "got %08lx\n", hr );
    SysFreeString( cmd_text );

    hr = _Command_get_CommandText( command,  &cmd_text );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cmd_text && !*cmd_text, "got %p\n", cmd_text );

    cmd_text = SysAllocString( L"test" );
    hr = _Command_put_CommandText( command, cmd_text );
    ok( hr == S_OK, "got %08lx\n", hr );
    SysFreeString( cmd_text );

    hr = _Command_get_CommandText( command,  &cmd_text );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !wcscmp( L"test", cmd_text ), "got %p\n", wine_dbgstr_w( cmd_text ) );

    connection = (_Connection*)0xdeadbeef;
    hr = _Command_get_ActiveConnection( command,  &connection );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( connection == NULL, "got %p\n", connection );

    hr = _Command_putref_ActiveConnection( command,  NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    VariantInit(&value);
    str = SysAllocString(L"_");
    hr = _Command_CreateParameter(command, str, adInteger, adParamInput, 0, value, &parameter );
    SysFreeString(str);
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( parameter != NULL, "Invalid pointer\n");

    hr = _Command_get_Parameters( command,  &parameters );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = _Parameter_QueryInterface(parameter, &IID_IDispatch, (void**)&disp);
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = Parameters_Append(parameters, disp);
    ok( hr == S_OK, "got %08lx\n", hr );

    IDispatch_Release(disp);
    _Parameter_Release(parameter);

    hr = _Command_get_Parameters( command,  &parameters2 );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( parameters == parameters2, "got %08lx\n", hr );
    Parameters_Release(parameters);
    Parameters_Release(parameters2);

    ok( !_Command_Release( command ), "_Command not released\n" );
}

struct conn_event {
    ConnectionEventsVt conn_event_sink;
    LONG refs;
};

static HRESULT WINAPI conneventvt_QueryInterface( ConnectionEventsVt *iface, REFIID riid, void **obj )
{
    struct conn_event *conn_event = CONTAINING_RECORD( iface, struct conn_event, conn_event_sink );

    if (IsEqualGUID( &IID_ConnectionEventsVt, riid ))
    {
        InterlockedIncrement( &conn_event->refs );
        *obj = iface;
        return S_OK;
    }

    ok( 0, "unexpected call\n" );
    return E_NOINTERFACE;
}

static ULONG WINAPI conneventvt_AddRef( ConnectionEventsVt *iface )
{
    struct conn_event *conn_event = CONTAINING_RECORD( iface, struct conn_event, conn_event_sink );
    return InterlockedIncrement( &conn_event->refs );
}

static ULONG WINAPI conneventvt_Release( ConnectionEventsVt *iface )
{
    struct conn_event *conn_event = CONTAINING_RECORD( iface, struct conn_event, conn_event_sink );
    return InterlockedDecrement( &conn_event->refs );
}

static HRESULT WINAPI conneventvt_InfoMessage( ConnectionEventsVt *iface, Error *error,
        EventStatusEnum *status, _Connection *Connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_BeginTransComplete( ConnectionEventsVt *iface, LONG TransactionLevel,
        Error *error, EventStatusEnum *status, _Connection *connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_CommitTransComplete( ConnectionEventsVt *iface, Error *error,
        EventStatusEnum *status, _Connection *connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_RollbackTransComplete( ConnectionEventsVt *iface, Error *error,
        EventStatusEnum *status, _Connection *connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_WillExecute( ConnectionEventsVt *iface, BSTR *source,
        CursorTypeEnum *cursor_type, LockTypeEnum *lock_type, LONG *options, EventStatusEnum *status,
        _Command *command, _Recordset *record_set, _Connection *connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_ExecuteComplete( ConnectionEventsVt *iface, LONG records_affected,
        Error *error, EventStatusEnum *status, _Command *command, _Recordset *record_set,
        _Connection *connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_WillConnect( ConnectionEventsVt *iface, BSTR *string, BSTR *userid,
        BSTR *password, LONG *options, EventStatusEnum *status, _Connection *connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_ConnectComplete( ConnectionEventsVt *iface, Error *error,
        EventStatusEnum *status, _Connection *connection )
{
    return E_NOTIMPL;
}

static HRESULT WINAPI conneventvt_Disconnect( ConnectionEventsVt *iface, EventStatusEnum *status,
        _Connection *connection )
{
    return E_NOTIMPL;
}

static const ConnectionEventsVtVtbl conneventvt_vtbl = {
    conneventvt_QueryInterface,
    conneventvt_AddRef,
    conneventvt_Release,
    conneventvt_InfoMessage,
    conneventvt_BeginTransComplete,
    conneventvt_CommitTransComplete,
    conneventvt_RollbackTransComplete,
    conneventvt_WillExecute,
    conneventvt_ExecuteComplete,
    conneventvt_WillConnect,
    conneventvt_ConnectComplete,
    conneventvt_Disconnect
};

static HRESULT WINAPI supporterror_QueryInterface( ISupportErrorInfo *iface, REFIID riid, void **obj )
{
    if (IsEqualGUID( &IID_ISupportErrorInfo, riid ))
    {
        *obj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI supporterror_AddRef( ISupportErrorInfo *iface )
{
    return 2;
}

static ULONG WINAPI supporterror_Release( ISupportErrorInfo *iface )
{
    return 1;
}

static HRESULT WINAPI supporterror_InterfaceSupportsErrorInfo( ISupportErrorInfo *iface, REFIID riid )
{
    return E_NOTIMPL;
}

static const struct ISupportErrorInfoVtbl support_error_vtbl =
{
    supporterror_QueryInterface,
    supporterror_AddRef,
    supporterror_Release,
    supporterror_InterfaceSupportsErrorInfo
};

static void test_ConnectionPoint(void)
{
    HRESULT hr;
    ULONG refs;
    DWORD cookie;
    IConnectionPoint *point;
    IConnectionPointContainer *pointcontainer;
    struct conn_event conn_event = { { &conneventvt_vtbl }, 0 };
    ISupportErrorInfo support_err_sink = { &support_error_vtbl };

    hr = CoCreateInstance( &CLSID_Connection, NULL, CLSCTX_INPROC_SERVER,
            &IID_IConnectionPointContainer, (void**)&pointcontainer );

    hr = IConnectionPointContainer_FindConnectionPoint( pointcontainer, &DIID_ConnectionEvents, NULL );
    ok( hr == E_POINTER, "got %08lx\n", hr );

    hr = IConnectionPointContainer_FindConnectionPoint( pointcontainer, &DIID_RecordsetEvents, &point );
    ok( hr == CONNECT_E_NOCONNECTION, "got %08lx\n", hr );

    hr = IConnectionPointContainer_FindConnectionPoint( pointcontainer, &DIID_ConnectionEvents, &point );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* nothing advised yet */
    hr = IConnectionPoint_Unadvise( point, 3 );
    ok( hr == E_FAIL, "got %08lx\n", hr );

    hr = IConnectionPoint_Advise( point, NULL, NULL );
    ok( hr == E_FAIL, "got %08lx\n", hr );

    hr = IConnectionPoint_Advise( point, (void*)&conn_event.conn_event_sink, NULL );
    ok( hr == E_FAIL, "got %08lx\n", hr );

    cookie = 0xdeadbeef;
    hr = IConnectionPoint_Advise( point, NULL, &cookie );
    ok( hr == E_FAIL, "got %08lx\n", hr );
    ok( cookie == 0xdeadbeef, "got %08lx\n", cookie );

    /* unsupported sink */
    cookie = 0xdeadbeef;
    hr = IConnectionPoint_Advise( point, (void*)&support_err_sink, &cookie );
    ok( hr == E_FAIL, "got %08lx\n", hr );
    ok( !cookie, "got %08lx\n", cookie );

    cookie = 0;
    hr = IConnectionPoint_Advise( point, (void*)&conn_event.conn_event_sink, &cookie );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cookie, "got %08lx\n", cookie );

    /* invalid cookie */
    hr = IConnectionPoint_Unadvise( point, 0 );
    ok( hr == E_FAIL, "got %08lx\n", hr );

    /* wrong cookie */
    hr = IConnectionPoint_Unadvise( point, cookie + 1 );
    ok( hr == E_FAIL, "got %08lx\n", hr );

    hr = IConnectionPoint_Unadvise( point, cookie );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* sinks are released when the connection is destroyed */
    cookie = 0;
    hr = IConnectionPoint_Advise( point, (void*)&conn_event.conn_event_sink, &cookie );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cookie, "got %08lx\n", cookie );
    ok( conn_event.refs == 1, "got %ld\n", conn_event.refs );

    refs = IConnectionPoint_Release( point );
    ok( refs == 1, "got %lu", refs );

    ok( !IConnectionPointContainer_Release( pointcontainer ), "IConnectionPointContainer not released\n" );

    ok( !conn_event.refs, "got %ld\n", conn_event.refs );
}

START_TEST(msado15)
{
    CoInitialize( NULL );
    test_Connection();
    test_ADORecordsetConstruction();
    test_ConnectionPoint();
    test_Fields();
    test_Recordset();
    test_Stream();
    test_Command();
    CoUninitialize();
}
