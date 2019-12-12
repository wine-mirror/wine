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
#include <msado15_backcompat.h>
#include "wine/test.h"

#define MAKE_ADO_HRESULT( err ) MAKE_HRESULT( SEVERITY_ERROR, FACILITY_CONTROL, err )

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
    Fields *fields, *fields2;
    LONG refs, count;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08x\n", hr );

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

    refs = _Recordset_Release( recordset );
    ok( !refs, "got %d\n", refs );

    /* fields object still has a reference */
    refs = Fields_Release( fields2 );
    ok( refs == 1, "got %d\n", refs );
}

static void test_Fields(void)
{
    _Recordset *recordset;
    Fields *fields;
    VARIANT val;
    BSTR name;
    LONG count;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_Recordset, NULL, CLSCTX_INPROC_SERVER, &IID__Recordset, (void **)&recordset );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Recordset_get_Fields( recordset, &fields );
    ok( hr == S_OK, "got %08x\n", hr );

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
    LONG state, timeout;

    hr = CoCreateInstance(&CLSID_Connection, NULL, CLSCTX_INPROC_SERVER, &IID__Connection, (void**)&connection);
    ok( hr == S_OK, "got %08x\n", hr );

    hr = _Connection_QueryInterface(connection, &IID_IRunnableObject, (void**)&runtime);
    ok(hr == E_NOINTERFACE, "Unexpected IRunnableObject interface\n");

    hr = _Connection_QueryInterface(connection, &IID_ISupportErrorInfo, (void**)&errorinfo);
    ok(hr == S_OK, "Failed to get ISupportErrorInfo interface\n");
    ISupportErrorInfo_Release(errorinfo);

if (0)   /* Crashes on windows */
{
    hr = _Connection_get_State(connection, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr 0x%08x\n", hr);
}

    state = -1;
    hr = _Connection_get_State(connection, &state);
    ok(hr == S_OK, "Failed to get state, hr 0x%08x\n", hr);
    ok(state == adStateClosed, "Unexpected state value 0x%08x\n", state);

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

    _Connection_Release(connection);
}

START_TEST(msado15)
{
    CoInitialize( NULL );
    test_Connection();
    test_Fields();
    test_Recordset();
    test_Stream();
    CoUninitialize();
}
