/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#include "windef.h"
#include "winbase.h"

#define COBJMACROS
#include "dmo.h"
#include "uuids.h"
#include "control.h"
#include "wmcodecdsp.h"
#include "mftransform.h"

#include "wine/test.h"

static void test_DMOGetTypes(void)
{
    DMO_PARTIAL_MEDIATYPE expect_output[2] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_PCM},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_IEEE_FLOAT},
    };
    DMO_PARTIAL_MEDIATYPE expect_input[4] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_MSAUDIO1},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO2},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO3},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO_LOSSLESS},
    };

    ULONG i, input_count = 0, output_count = 0;
    DMO_PARTIAL_MEDIATYPE output[16] = {{{0}}};
    DMO_PARTIAL_MEDIATYPE input[16] = {{{0}}};
    HRESULT hr;

    hr = DMOGetTypes( &CLSID_CWMADecMediaObject, ARRAY_SIZE(input), &input_count, input,
                      ARRAY_SIZE(output), &output_count, output );
    todo_wine
    ok( hr == S_OK, "DMOGetTypes returned %#lx\n", hr );
    todo_wine
    ok( input_count == ARRAY_SIZE(expect_input), "got input_count %lu\n", input_count );
    todo_wine
    ok( output_count == ARRAY_SIZE(expect_output), "got output_count %lu\n", output_count );

    for (i = 0; i < input_count; ++i)
    {
        winetest_push_context( "in %lu", i );
        ok( IsEqualGUID( &input[i].type, &expect_input[i].type ),
            "got type %s\n", debugstr_guid( &input[i].type ) );
        ok( IsEqualGUID( &input[i].subtype, &expect_input[i].subtype ),
            "got subtype %s\n", debugstr_guid( &input[i].subtype ) );
        winetest_pop_context();
    }

    for (i = 0; i < output_count; ++i)
    {
        winetest_push_context( "out %lu", i );
        ok( IsEqualGUID( &output[i].type, &expect_output[i].type ),
            "got type %s\n", debugstr_guid( &output[i].type ) );
        ok( IsEqualGUID( &output[i].subtype, &expect_output[i].subtype ),
            "got subtype %s\n", debugstr_guid( &output[i].subtype ) );
        winetest_pop_context();
    }
}

static HRESULT WINAPI outer_QueryInterface( IUnknown *iface, REFIID iid, void **obj )
{
    if (IsEqualGUID( iid, &IID_IUnknown ))
    {
        *obj = (IUnknown *)iface;
        return S_OK;
    }

    ok( 0, "unexpected call %s\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef( IUnknown *iface )
{
    return 2;
}

static ULONG WINAPI outer_Release( IUnknown *iface )
{
    return 1;
}

static IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown outer = {&outer_vtbl};

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface, REFIID iid, BOOL supported)
{
    HRESULT hr, expected_hr;
    IUnknown *tmp;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface( (IUnknown *)iface, iid, (void **)&tmp );
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr)) IUnknown_Release( tmp );
}

static void test_interfaces(void)
{
    IUnknown *unknown, *tmp_unknown;
    IMediaObject *media_object;
    IPropertyBag *property_bag;
    IMFTransform *transform;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance( &CLSID_CWMADecMediaObject, &outer, CLSCTX_INPROC_SERVER, &IID_IUnknown,
                           (void **)&unknown );
    todo_wine
    ok( hr == S_OK, "CoCreateInstance returned %#lx\n", hr );
    if (FAILED(hr)) return;
    hr = IUnknown_QueryInterface( unknown, &IID_IMFTransform, (void **)&transform );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IMediaObject, (void **)&media_object );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( unknown, &IID_IPropertyBag, (void **)&property_bag );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IUnknown_QueryInterface( media_object, &IID_IUnknown, (void **)&tmp_unknown );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    ok( unknown != &outer, "got outer IUnknown\n" );
    ok( transform != (void *)unknown, "got IUnknown == IMFTransform\n" );
    ok( media_object != (void *)unknown, "got IUnknown == IMediaObject\n" );
    ok( property_bag != (void *)unknown, "got IUnknown == IPropertyBag\n" );
    ok( tmp_unknown != unknown, "got inner IUnknown\n" );

    check_interface( unknown, &IID_IPersistPropertyBag, FALSE );
    check_interface( unknown, &IID_IAMFilterMiscFlags, FALSE );
    check_interface( unknown, &IID_IMediaSeeking, FALSE );
    check_interface( unknown, &IID_IMediaPosition, FALSE );
    check_interface( unknown, &IID_IReferenceClock, FALSE );
    check_interface( unknown, &IID_IBasicAudio, FALSE );

    ref = IUnknown_Release( tmp_unknown );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IPropertyBag_Release( property_bag );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IMediaObject_Release( media_object );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IMFTransform_Release( transform );
    ok( ref == 1, "Release returned %lu\n", ref );
    ref = IUnknown_Release( unknown );
    ok( ref == 0, "Release returned %lu\n", ref );
}

START_TEST( wmadmod )
{
    CoInitialize( NULL );

    test_DMOGetTypes();
    test_interfaces();

    CoUninitialize();
}
