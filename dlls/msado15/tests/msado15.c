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
    LONG refs, size, pos;
    ObjectStateEnum state;
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

    state = 0xdeadbeef;
    hr = _Stream_get_State( stream, &state );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( state == adStateClosed, "got %u\n", state );

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

    pos = -1;
    hr = _Stream_get_Position( stream, &pos );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !pos, "got %d\n", pos );

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

START_TEST(msado15)
{
    CoInitialize( NULL );
    test_Stream();
    CoUninitialize();
}
