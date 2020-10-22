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

static LONG get_refs_field( Field *field )
{
    Field_AddRef( field );
    return Field_Release( field );
}

static LONG get_refs_fields( Fields *fields )
{
    Fields_AddRef( fields );
    return Fields_Release( fields );
}

static LONG get_refs_recordset( _Recordset *recordset )
{
    _Recordset_AddRef( recordset );
    return _Recordset_Release( recordset );
}

static void test_Recordset(void)
{
    _Recordset *recordset;
    IRunnableObject *runtime;
    ISupportErrorInfo *errorinfo;
    Fields *fields, *fields2;
    Field *field;
    LONG refs, count, state;
    VARIANT missing, val, index;
    CursorLocationEnum location;
    CursorTypeEnum cursor;
    BSTR name;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Recordset_QueryInterface( recordset, &IID_IRunnableObject, (void**)&runtime);
    ok(hr == E_NOINTERFACE, "Unexpected IRunnableObject interface\n");
    ok(runtime == NULL, "expected NULL\n");

    /* _Recordset object supports ISupportErrorInfo */
    errorinfo = NULL;
    hr = _Recordset_QueryInterface( recordset, &IID_ISupportErrorInfo, (void **)&errorinfo );
    ok( hr == S_OK, "got %08x\n", hr );
    refs = get_refs_recordset( recordset );
    ok( refs == 2, "got %d\n", refs );
    if (errorinfo) ISupportErrorInfo_Release( errorinfo );
    refs = get_refs_recordset( recordset );
    ok( refs == 1, "got %d\n", refs );

    /* handing out fields object increases recordset refcount */
    refs = get_refs_recordset( recordset );
    ok( refs == 1, "got %d\n", refs );
    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08x\n", hr );
    refs = get_refs_recordset( recordset );
    ok( refs == 2, "got %d\n", refs );
    refs = get_refs_fields( fields );
    ok( refs == 1, "got %d\n", refs );

    /* releasing fields object decreases recordset refcount, but fields refcount doesn't drop to zero */
    Fields_Release( fields );
    refs = get_refs_recordset( recordset );
    ok( refs == 1, "got %d\n", refs );
    refs = get_refs_fields( fields );
    ok( refs == 1, "got %d\n", refs );

    /* calling get_Fields again returns the same object with the same refcount and increases recordset refcount  */
    hr = _Recordset_get_Fields( recordset, &fields2 );
    ok( hr == S_OK, "got %08x\n", hr );
    refs = get_refs_recordset( recordset );
    ok( refs == 2, "got %d\n", refs );
    refs = get_refs_fields( fields2 );
    ok( refs == 1, "got %d\n", refs );
    ok( fields2 == fields, "expected same object\n" );
    refs = Fields_Release( fields2 );
    ok( refs == 1, "got %d\n", refs );

    count = -1;
    hr = Fields_get_Count( fields2, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !count, "got %d\n", count );

    hr = _Recordset_Close( recordset );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    refs = _Recordset_Release( recordset );
    ok( !refs, "got %d\n", refs );

    /* fields object still has a reference */
    refs = Fields_Release( fields2 );
    ok( refs == 1, "got %d\n", refs );

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08x\n", hr );

    state = -1;
    hr = _Recordset_get_State( recordset, &state );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == adStateClosed, "got %d\n", state );

    location = -1;
    hr = _Recordset_get_CursorLocation( recordset, &location );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( location == adUseServer, "got %d\n", location );

    cursor = adOpenUnspecified;
    hr = _Recordset_get_CursorType( recordset, &cursor );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( cursor == adOpenForwardOnly, "got %d\n", cursor );

    VariantInit( &missing );
    hr = _Recordset_AddNew( recordset, missing, missing );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    V_VT( &missing ) = VT_ERROR;
    V_ERROR( &missing ) = DISP_E_PARAMNOTFOUND;
    hr = _Recordset_Open( recordset, missing, missing, adOpenStatic, adLockBatchOptimistic, adCmdUnspecified );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidConnection ), "got %08x\n", hr );

    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08x\n", hr );

    name = SysAllocString( L"field" );
    hr = Fields__Append( fields, name, adInteger, 4, adFldUnspecified );
    ok( hr == S_OK, "got %08x\n", hr );

    V_VT( &index ) = VT_BSTR;
    V_BSTR( &index ) = name;
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( name );

    hr = _Recordset_Open( recordset, missing, missing, adOpenStatic, adLockBatchOptimistic, adCmdUnspecified );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( is_eof( recordset ), "not eof\n" );
    ok( is_bof( recordset ), "not bof\n" );

    hr = _Recordset_Open( recordset, missing, missing, adOpenStatic, adLockBatchOptimistic, adCmdUnspecified );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectOpen ), "got %08x\n", hr );

    state = -1;
    hr = _Recordset_get_State( recordset, &state );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == adStateOpen, "got %d\n", state );

    count = -1;
    hr = _Recordset_get_RecordCount( recordset, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !count, "got %d\n", count );

    hr = _Recordset_AddNew( recordset, missing, missing );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    count = -1;
    hr = _Recordset_get_RecordCount( recordset, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count == 1, "got %d\n", count );

    /* get_Fields still returns the same object */
    hr = _Recordset_get_Fields( recordset, &fields2 );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( fields2 == fields, "expected same object\n" );
    Fields_Release( fields2 );

    count = -1;
    hr = Fields_get_Count( fields2, &count );
    ok( count == 1, "got %d\n", count );

    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &val ) == VT_EMPTY, "got %u\n", V_VT( &val  ) );

    V_VT( &val ) = VT_I4;
    V_I4( &val ) = -1;
    hr = Field_put_Value( field, val );
    ok( hr == S_OK, "got %08x\n", hr );

    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &val ) == VT_I4, "got %u\n", V_VT( &val ) );
    ok( V_I4( &val ) == -1, "got %d\n", V_I4( &val ) );

    hr = _Recordset_AddNew( recordset, missing, missing );
    ok( hr == S_OK, "got %08x\n", hr );

    /* field object returns different value after AddNew */
    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &val ) == VT_EMPTY, "got %u\n", V_VT( &val ) );

    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );
    hr = _Recordset_MoveFirst( recordset );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Field_get_Value( field, &val );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &val ) == VT_I4, "got %u\n", V_VT( &val ) );
    ok( V_I4( &val ) == -1, "got %d\n", V_I4( &val ) );

    hr = _Recordset_MoveNext( recordset );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "not bof\n" );

    hr = _Recordset_MoveNext( recordset );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( is_eof( recordset ), "not eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    hr = _Recordset_MoveFirst( recordset );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( !is_bof( recordset ), "bof\n" );

    hr = _Recordset_MovePrevious( recordset );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !is_eof( recordset ), "eof\n" );
    ok( is_bof( recordset ), "not bof\n" );

    /* try get value at BOF */
    VariantInit( &val );
    hr = Field_get_Value( field, &val );
    ok( hr == MAKE_ADO_HRESULT( adErrNoCurrentRecord ), "got %08x\n", hr );

    hr = _Recordset_Close( recordset );
    ok( hr == S_OK, "got %08x\n", hr );

    state = -1;
    hr = _Recordset_get_State( recordset, &state );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == adStateClosed, "got %d\n", state );

    Field_Release( field );
    Fields_Release( fields );
    _Recordset_Release( recordset );
}

static void test_Fields(void)
{
    _Recordset *recordset;
    ISupportErrorInfo *errorinfo;
    Fields *fields;
    Field *field, *field2;
    VARIANT val, index;
    BSTR name;
    LONG refs, count, size;
    DataTypeEnum type;
    FieldAttributeEnum attrs;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08x\n", hr );

    /* Fields object supports ISupportErrorInfo */
    errorinfo = NULL;
    hr = Fields_QueryInterface( fields, &IID_ISupportErrorInfo, (void **)&errorinfo );
    ok( hr == S_OK, "got %08x\n", hr );
    refs = get_refs_fields( fields );
    ok( refs == 2, "got %d\n", refs );
    if (errorinfo) ISupportErrorInfo_Release( errorinfo );
    refs = get_refs_fields( fields );
    ok( refs == 1, "got %d\n", refs );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( !count, "got %d\n", count );

    name = SysAllocString( L"field" );
    V_VT( &val ) = VT_ERROR;
    V_ERROR( &val ) = DISP_E_PARAMNOTFOUND;
    hr = Fields_Append( fields, name, adInteger, 4, adFldUnspecified, val );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( name );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( count == 1, "got %d\n", count );

    name = SysAllocString( L"field2" );
    hr = Fields__Append( fields, name, adInteger, 4, adFldUnspecified );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( name );

    count = -1;
    hr = Fields_get_Count( fields, &count );
    ok( count == 2, "got %d\n", count );

    /* handing out field object doesn't add reference to fields or recordset object */
    name = SysAllocString( L"field" );
    V_VT( &index ) = VT_BSTR;
    V_BSTR( &index ) = name;
    refs = get_refs_recordset( recordset );
    ok( refs == 2, "got %d\n", refs );
    refs = get_refs_fields( fields );
    ok( refs == 1, "got %d\n", refs );
    hr = Fields_get_Item( fields, index, &field );
    ok( hr == S_OK, "got %08x\n", hr );
    refs = get_refs_field( field );
    ok( refs == 1, "got %d\n", refs );
    refs = get_refs_recordset( recordset );
    ok( refs == 2, "got %d\n", refs );
    refs = get_refs_fields( fields );
    ok( refs == 1, "got %d\n", refs );

    /* calling get_Item again returns the same object and adds reference */
    hr = Fields_get_Item( fields, index, &field2 );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( field2 == field, "expected same object\n" );
    refs = get_refs_field( field2 );
    ok( refs == 2, "got %d\n", refs );
    refs = get_refs_recordset( recordset );
    ok( refs == 2, "got %d\n", refs );
    refs = get_refs_fields( fields );
    ok( refs == 1, "got %d\n", refs );
    Field_Release( field2 );
    SysFreeString( name );

    /* Field object supports ISupportErrorInfo */
    errorinfo = NULL;
    hr = Field_QueryInterface( field, &IID_ISupportErrorInfo, (void **)&errorinfo );
    ok( hr == S_OK, "got %08x\n", hr );
    refs = get_refs_field( field );
    ok( refs == 2, "got %d\n", refs );
    if (errorinfo) ISupportErrorInfo_Release( errorinfo );
    refs = get_refs_field( field );
    ok( refs == 1, "got %d\n", refs );

    /* verify values set with _Append */
    hr = Field_get_Name( field, &name );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( name, L"field" ), "got %s\n", wine_dbgstr_w(name) );
    SysFreeString( name );
    type = 0xdead;
    hr = Field_get_Type( field, &type );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == adInteger, "got %d\n", type );
    size = -1;
    hr = Field_get_DefinedSize( field, &size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( size == 4, "got %d\n", size );
    attrs = 0xdead;
    hr = Field_get_Attributes( field, &attrs );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !attrs, "got %d\n", attrs );

    Field_Release( field );
    Fields_Release( fields );
    _Recordset_Release( recordset );
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
    LONG refs, size, pos;
    ObjectStateEnum state;
    ConnectModeEnum mode;
    BSTR charset, str;
    VARIANT missing, val;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Stream, NULL, CLSCTX_INPROC_SERVER, &IID__Stream, (void **)&stream );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Stream_get_Size( stream, &size );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    hr = _Stream_get_EOS( stream, &eos );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    hr = _Stream_get_Position( stream, &pos );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    hr = _Stream_put_Position( stream, 0 );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    /* check default type */
    type = 0;
    hr = _Stream_get_Type( stream, &type );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == adTypeText, "got %u\n", type );

    hr = _Stream_put_Type( stream, adTypeBinary );
    ok( hr == S_OK, "got %08x\n", hr );

    type = 0;
    hr = _Stream_get_Type( stream, &type );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( type == adTypeBinary, "got %u\n", type );

    /* revert */
    hr = _Stream_put_Type( stream, adTypeText );
    ok( hr == S_OK, "got %08x\n", hr );

    sep = 0;
    hr = _Stream_get_LineSeparator( stream, &sep );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( sep == adCRLF, "got %d\n", sep );

    hr = _Stream_put_LineSeparator( stream, adLF );
    ok( hr == S_OK, "got %08x\n", hr );

    state = 0xdeadbeef;
    hr = _Stream_get_State( stream, &state );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == adStateClosed, "got %u\n", state );

    mode = 0xdeadbeef;
    hr = _Stream_get_Mode( stream, &mode );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( mode == adModeUnknown, "got %u\n", mode );

    hr = _Stream_put_Mode( stream, adModeReadWrite );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Stream_get_Charset( stream, &charset );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( charset, L"Unicode" ), "got %s\n", wine_dbgstr_w(charset) );
    SysFreeString( charset );

    str = SysAllocString( L"Unicode" );
    hr = _Stream_put_Charset( stream, str );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( str );

    hr = _Stream_Read( stream, 2, &val );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    V_VT( &missing ) = VT_ERROR;
    V_ERROR( &missing ) = DISP_E_PARAMNOTFOUND;
    hr = _Stream_Open( stream, missing, adModeUnknown, adOpenStreamUnspecified, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Stream_Open( stream, missing, adModeUnknown, adOpenStreamUnspecified, NULL, NULL );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectOpen ), "got %08x\n", hr );

    state = 0xdeadbeef;
    hr = _Stream_get_State( stream, &state );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == adStateOpen, "got %u\n", state );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !size, "got %d\n", size );

    eos = VARIANT_FALSE;
    hr = _Stream_get_EOS( stream, &eos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( eos == VARIANT_TRUE, "got %04x\n", eos );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !pos, "got %d\n", pos );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !size, "got %d\n", size );

    hr = _Stream_Read( stream, 2, &val );
    ok( hr == MAKE_ADO_HRESULT( adErrIllegalOperation ), "got %08x\n", hr );

    hr = _Stream_ReadText( stream, 2, &str );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !str[0], "got %s\n", wine_dbgstr_w(str) );
    SysFreeString( str );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !pos, "got %d\n", pos );

    str = SysAllocString( L"test" );
    hr = _Stream_WriteText( stream, str, adWriteChar );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( str );

    hr = _Stream_ReadText( stream, adReadAll, &str );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !str[0], "got %s\n", wine_dbgstr_w(str) );
    SysFreeString( str );

    hr = _Stream_put_Position( stream, 0 );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = _Stream_ReadText( stream, adReadAll, &str );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( str, L"test" ), "got %s\n", wine_dbgstr_w(str) );
    SysFreeString( str );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( pos == 10, "got %d\n", pos );

    eos = VARIANT_FALSE;
    hr = _Stream_get_EOS( stream, &eos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( eos == VARIANT_TRUE, "got %04x\n", eos );

    hr = _Stream_put_Position( stream, 6 );
    ok( hr == S_OK, "got %08x\n", hr );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( size == 10, "got %d\n", size );

    hr = _Stream_put_Position( stream, 2 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Stream_SetEOS( stream );
    ok( hr == S_OK, "got %08x\n", hr );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( pos == 2, "got %d\n", pos );

    size = -1;
    hr = _Stream_get_Size( stream, &size );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( size == 2, "got %d\n", size );

    hr = _Stream_Close( stream );
    ok( hr == S_OK, "got %08x\n", hr );

    state = 0xdeadbeef;
    hr = _Stream_get_State( stream, &state );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == adStateClosed, "got %u\n", state );

    hr = _Stream_Close( stream );
    ok( hr == MAKE_ADO_HRESULT( adErrObjectClosed ), "got %08x\n", hr );

    refs = _Stream_Release( stream );
    ok( !refs, "got %d\n", refs );

    /* binary type */
    hr = CoCreateInstance( &CLSID_Stream, NULL, CLSCTX_INPROC_SERVER, &IID__Stream, (void **)&stream );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Stream_put_Type( stream, adTypeBinary );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Stream_Open( stream, missing, adModeUnknown, adOpenStreamUnspecified, NULL, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Stream_ReadText( stream, adReadAll, &str );
    ok( hr == MAKE_ADO_HRESULT( adErrIllegalOperation ), "got %08x\n", hr );

    str = SysAllocString( L"test" );
    hr = _Stream_WriteText( stream, str, adWriteChar );
    ok( hr == MAKE_ADO_HRESULT( adErrIllegalOperation ), "got %08x\n", hr );
    SysFreeString( str );

    VariantInit( &val );
    hr = _Stream_Read( stream, 1, &val );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &val ) == VT_NULL, "got %u\n", V_VT( &val ) );

    VariantInit( &val );
    hr = _Stream_Write( stream, val );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidArgument ), "got %08x\n", hr );

    hr = str_to_byte_array( "data", &val );
    ok( hr == S_OK, "got %08x\n", hr );
    hr = _Stream_Write( stream, val );
    ok( hr == S_OK, "got %08x\n", hr );
    VariantClear( &val );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( pos == 4, "got %d\n", pos );

    hr = _Stream_put_Position( stream, 0 );
    ok( hr == S_OK, "got %08x\n", hr );

    VariantInit( &val );
    hr = _Stream_Read( stream, adReadAll, &val );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( V_VT( &val ) == (VT_ARRAY | VT_UI1), "got %04x\n", V_VT( &val ) );
    VariantClear( &val );

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( pos == 4, "got %d\n", pos );

    refs = _Stream_Release( stream );
    ok( !refs, "got %d\n", refs );
}

static void test_Connection(void)
{
    HRESULT hr;
    _Connection *connection;
    IRunnableObject *runtime;
    ISupportErrorInfo *errorinfo;
    IConnectionPointContainer *pointcontainer;
    LONG state, timeout;
    BSTR str, str2, str3;
    ConnectModeEnum mode;
    CursorLocationEnum location;

    hr = CoCreateInstance(&CLSID_Connection, NULL, CLSCTX_INPROC_SERVER, &IID__Connection, (void**)&connection);
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Connection_QueryInterface(connection, &IID_IRunnableObject, (void**)&runtime);
    ok(hr == E_NOINTERFACE, "Unexpected IRunnableObject interface\n");
    ok(runtime == NULL, "expected NULL\n");

    hr = _Connection_QueryInterface(connection, &IID_ISupportErrorInfo, (void**)&errorinfo);
    ok(hr == S_OK, "Failed to get ISupportErrorInfo interface\n");
    ISupportErrorInfo_Release(errorinfo);

    hr = _Connection_QueryInterface(connection, &IID_IConnectionPointContainer, (void**)&pointcontainer);
    ok(hr == S_OK, "Failed to get IConnectionPointContainer interface %08x\n", hr);
    IConnectionPointContainer_Release(pointcontainer);

if (0)   /* Crashes on windows */
{
    hr = _Connection_get_State(connection, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr 0x%08x\n", hr);
}

    state = -1;
    hr = _Connection_get_State(connection, &state);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);
    ok(state == adStateClosed, "Unexpected state value 0x%08x\n", state);

    hr = _Connection_Close(connection);
    ok(hr == MAKE_ADO_HRESULT(adErrObjectClosed), "got %08x\n", hr);

    timeout = 0;
    hr = _Connection_get_CommandTimeout(connection, &timeout);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);
    ok(timeout == 30, "Unexpected timeout value %d\n", timeout);

    hr = _Connection_put_CommandTimeout(connection, 300);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);

    timeout = 0;
    hr = _Connection_get_CommandTimeout(connection, &timeout);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);
    ok(timeout == 300, "Unexpected timeout value %d\n", timeout);

    location = 0;
    hr = _Connection_get_CursorLocation(connection, &location);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    ok(location == adUseServer, "Unexpected location value %d\n", location);

    hr = _Connection_put_CursorLocation(connection, adUseClient);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);

    location = 0;
    hr = _Connection_get_CursorLocation(connection, &location);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    ok(location == adUseClient, "Unexpected location value %d\n", location);

    hr = _Connection_put_CursorLocation(connection, adUseServer);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);

    mode = 0xdeadbeef;
    hr = _Connection_get_Mode(connection, &mode);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);
    ok(mode == adModeUnknown, "Unexpected mode value %d\n", mode);

    hr = _Connection_put_Mode(connection, adModeShareDenyNone);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);

    mode = adModeUnknown;
    hr = _Connection_get_Mode(connection, &mode);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);
    ok(mode == adModeShareDenyNone, "Unexpected mode value %d\n", mode);

    hr = _Connection_put_Mode(connection, adModeUnknown);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);

    /* Default */
    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_Provider(connection, &str);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    ok(!wcscmp(str, L"MSDASQL"), "wrong string %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = SysAllocString(L"MSDASQL.1");
    hr = _Connection_put_Provider(connection, str);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    SysFreeString(str);

    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_Provider(connection, &str);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    ok(!wcscmp(str, L"MSDASQL.1"), "wrong string %s\n", wine_dbgstr_w(str));

    /* Restore default */
    str = SysAllocString(L"MSDASQL");
    hr = _Connection_put_Provider(connection, str);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    SysFreeString(str);

    hr = _Connection_put_Provider(connection, NULL);
    ok(hr == MAKE_ADO_HRESULT(adErrInvalidArgument), "got 0x%08x\n", hr);
    SysFreeString(str);

    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_ConnectionString(connection, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(str == NULL, "got %p\n", str);

    str = SysAllocString(L"Provider=MSDASQL.1;Persist Security Info=False;Data Source=wine_test");
    hr = _Connection_put_ConnectionString(connection, str);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);

    /* Show put_ConnectionString effects Provider */
    str3 = (BSTR)0xdeadbeef;
    hr = _Connection_get_Provider(connection, &str3);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    ok(str3 != NULL, "Expected value got NULL\n");
    todo_wine ok(!wcscmp(str3, L"MSDASQL.1"), "wrong string %s\n", wine_dbgstr_w(str3));
    SysFreeString(str3);

if (0) /* Crashes on windows */
{
    hr = _Connection_get_ConnectionString(connection, NULL);
    ok(hr == E_POINTER, "Failed, hr 0x%08x\n", hr);
}

    str2 = NULL;
    hr = _Connection_get_ConnectionString(connection, &str2);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    ok(!wcscmp(str, str2), "wrong string %s\n", wine_dbgstr_w(str2));

    hr = _Connection_Open(connection, NULL, NULL, NULL, 0);
    ok(hr == E_FAIL, "Failed, hr 0x%08x\n", hr);

    /* Open adds trailing ; if it's missing */
    str3 = SysAllocString(L"Provider=MSDASQL.1;Persist Security Info=False;Data Source=wine_test;");
    hr = _Connection_Open(connection, NULL, NULL, NULL, adConnectUnspecified);
    ok(hr == E_FAIL, "Failed, hr 0x%08x\n", hr);

    str2 = NULL;
    hr = _Connection_get_ConnectionString(connection, &str2);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    todo_wine ok(!wcscmp(str3, str2) || broken(!wcscmp(str, str2)) /* XP */, "wrong string %s\n", wine_dbgstr_w(str2));

    hr = _Connection_Open(connection, str, NULL, NULL, adConnectUnspecified);
    todo_wine ok(hr == E_FAIL, "Failed, hr 0x%08x\n", hr);
    SysFreeString(str);

    str2 = NULL;
    hr = _Connection_get_ConnectionString(connection, &str2);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    todo_wine ok(!wcscmp(str3, str2) || broken(!wcscmp(str, str2)) /* XP */, "wrong string %s\n", wine_dbgstr_w(str2));
    SysFreeString(str2);
    SysFreeString(str3);

    hr = _Connection_put_ConnectionString(connection, NULL);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);

    str = (BSTR)0xdeadbeef;
    hr = _Connection_get_ConnectionString(connection, &str);
    ok(hr == S_OK, "Failed, hr 0x%08x\n", hr);
    ok(str == NULL, "got %p\n", str);
    _Connection_Release(connection);
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

    hr = CoCreateInstance( &CLSID_Command, NULL, CLSCTX_INPROC_SERVER, &IID__Command, (void **)&command );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Command_QueryInterface( command, &IID__ADO, (void **)&ado );
    ok( hr == S_OK, "got %08x\n", hr );
    _ADO_Release( ado );

    hr = _Command_QueryInterface( command, &IID_Command15, (void **)&command15 );
    ok( hr == S_OK, "got %08x\n", hr );
    Command15_Release( command15 );

    hr = _Command_QueryInterface( command, &IID_Command25, (void **)&command25 );
    ok( hr == S_OK, "got %08x\n", hr );
    Command25_Release( command25 );

    hr = _Command_get_CommandType( command, &cmd_type );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( cmd_type == adCmdUnknown, "got %08x\n", cmd_type );

    _Command_put_CommandType( command, adCmdText );
    hr = _Command_get_CommandType( command, &cmd_type );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( cmd_type == adCmdText, "got %08x\n", cmd_type );

    hr = _Command_put_CommandType( command, 0xdeadbeef );
    ok( hr == MAKE_ADO_HRESULT( adErrInvalidArgument ), "got %08x\n", hr );

    hr = _Command_get_CommandText( command, &cmd_text );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !cmd_text, "got %s\n", wine_dbgstr_w( cmd_text ));

    hr = _Command_put_CommandText( command, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    cmd_text = SysAllocString( L"" );
    hr = _Command_put_CommandText( command, cmd_text );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( cmd_text );

    hr = _Command_get_CommandText( command,  &cmd_text );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( cmd_text && !*cmd_text, "got %p\n", cmd_text );

    cmd_text = SysAllocString( L"test" );
    hr = _Command_put_CommandText( command, cmd_text );
    ok( hr == S_OK, "got %08x\n", hr );
    SysFreeString( cmd_text );

    hr = _Command_get_CommandText( command,  &cmd_text );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !wcscmp( L"test", cmd_text ), "got %p\n", wine_dbgstr_w( cmd_text ) );

    connection = (_Connection*)0xdeadbeef;
    hr = _Command_get_ActiveConnection( command,  &connection );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( connection == NULL, "got %p\n", connection );

    hr = _Command_putref_ActiveConnection( command,  NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    _Command_Release( command );
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
    ok( hr == E_POINTER, "got %08x\n", hr );

    hr = IConnectionPointContainer_FindConnectionPoint( pointcontainer, &DIID_RecordsetEvents, &point );
    ok( hr == CONNECT_E_NOCONNECTION, "got %08x\n", hr );

    hr = IConnectionPointContainer_FindConnectionPoint( pointcontainer, &DIID_ConnectionEvents, &point );
    ok( hr == S_OK, "got %08x\n", hr );

    /* nothing advised yet */
    hr = IConnectionPoint_Unadvise( point, 3 );
    ok( hr == E_FAIL, "got %08x\n", hr );

    hr = IConnectionPoint_Advise( point, NULL, NULL );
    ok( hr == E_FAIL, "got %08x\n", hr );

    hr = IConnectionPoint_Advise( point, (void*)&conn_event.conn_event_sink, NULL );
    ok( hr == E_FAIL, "got %08x\n", hr );

    cookie = 0xdeadbeef;
    hr = IConnectionPoint_Advise( point, NULL, &cookie );
    ok( hr == E_FAIL, "got %08x\n", hr );
    ok( cookie == 0xdeadbeef, "got %08x\n", cookie );

    /* unsupported sink */
    cookie = 0xdeadbeef;
    hr = IConnectionPoint_Advise( point, (void*)&support_err_sink, &cookie );
    ok( hr == E_FAIL, "got %08x\n", hr );
    ok( !cookie, "got %08x\n", cookie );

    cookie = 0;
    hr = IConnectionPoint_Advise( point, (void*)&conn_event.conn_event_sink, &cookie );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( cookie, "got %08x\n", cookie );

    /* invalid cookie */
    hr = IConnectionPoint_Unadvise( point, 0 );
    ok( hr == E_FAIL, "got %08x\n", hr );

    /* wrong cookie */
    hr = IConnectionPoint_Unadvise( point, cookie + 1 );
    ok( hr == E_FAIL, "got %08x\n", hr );

    hr = IConnectionPoint_Unadvise( point, cookie );
    ok( hr == S_OK, "got %08x\n", hr );

    /* sinks are released when the connection is destroyed */
    cookie = 0;
    hr = IConnectionPoint_Advise( point, (void*)&conn_event.conn_event_sink, &cookie );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( cookie, "got %08x\n", cookie );
    ok( conn_event.refs == 1, "got %d\n", conn_event.refs );

    refs = IConnectionPoint_Release( point );
    ok( refs == 1, "got %u", refs );

    IConnectionPointContainer_Release( pointcontainer );

    ok( !conn_event.refs, "got %d\n", conn_event.refs );
}

START_TEST(msado15)
{
    CoInitialize( NULL );
    test_Connection();
    test_ConnectionPoint();
    test_Fields();
    test_Recordset();
    test_Stream();
    test_Command();
    CoUninitialize();
}
