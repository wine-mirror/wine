/*
 * Copyright (C) 2022 Mohamad Al-Jaf
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
#include "initguid.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winstring.h"

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_UI
#include "windows.ui.h"
#define WIDL_using_Windows_Media_ClosedCaptioning
#include "windows.media.closedcaptioning.h"
#define WIDL_using_Windows_Media_Transcoding
#include "windows.media.transcoding.h"

#include "wine/test.h"

#define check_interface( obj, iid, exp ) check_interface_( __LINE__, obj, iid, exp )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL supported )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr, expected_hr;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_( __FILE__, line )( hr == expected_hr || broken(hr == E_NOINTERFACE ), "Got hr %#lx, expected %#lx.\n", hr, expected_hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static void test_CaptionStatics(void)
{
    static const WCHAR *caption_properties_name = L"Windows.Media.ClosedCaptioning.ClosedCaptionProperties";
    IClosedCaptionPropertiesStatics *caption_statics;
    IActivationFactory *factory;
    HSTRING str;
    HRESULT hr;
    LONG ref;
    /* Properties */
    ClosedCaptionColor color;
    ClosedCaptionOpacity opacity;
    ClosedCaptionSize size;
    ClosedCaptionStyle style;
    ClosedCaptionEdgeEffect effect;
    Color computed_color;

    hr = WindowsCreateString( caption_properties_name, wcslen( caption_properties_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( caption_properties_name ));
        return;
    }

    check_interface( factory, &IID_IUnknown, TRUE );
    check_interface( factory, &IID_IInspectable, TRUE );
    check_interface( factory, &IID_IAgileObject, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_IClosedCaptionPropertiesStatics, (void **)&caption_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    color = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_FontColor( caption_statics, &color );
    ok( hr == S_OK, "get_FontColor returned %#lx\n", hr );
    ok( color == ClosedCaptionColor_Default, "expected default font color, got %d\n", color );

    hr = IClosedCaptionPropertiesStatics_get_ComputedFontColor( caption_statics, &computed_color );
    todo_wine ok( hr == E_INVALIDARG, "get_ComputedFontColor returned %#lx\n", hr );
    hr = IClosedCaptionPropertiesStatics_get_ComputedFontColor( caption_statics, NULL );
    todo_wine ok( hr == E_INVALIDARG, "get_ComputedFontColor returned %#lx\n", hr );

    opacity = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_FontOpacity( caption_statics, &opacity );
    ok( hr == S_OK, "get_FontOpacity returned %#lx\n", hr );
    ok( opacity == ClosedCaptionOpacity_Default, "expected default font opacity, got %d\n", opacity );

    size = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_FontSize( caption_statics, &size );
    ok( hr == S_OK, "get_FontSize returned %#lx\n", hr );
    ok( size == ClosedCaptionSize_Default, "expected default font size, got %d\n", size );

    style = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_FontStyle( caption_statics, &style );
    ok( hr == S_OK, "get_FontStyle returned %#lx\n", hr );
    ok( style == ClosedCaptionStyle_Default, "expected default font style, got %d\n", style );

    effect = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_FontEffect( caption_statics, &effect );
    ok( hr == S_OK, "get_FontEffect returned %#lx\n", hr );
    ok( effect == ClosedCaptionEdgeEffect_Default, "expected default font effect, got %d\n", effect );

    color = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_BackgroundColor( caption_statics, &color );
    ok( hr == S_OK, "get_BackgroundColor returned %#lx\n", hr );
    ok( color == ClosedCaptionColor_Default, "expected default background color, got %d\n", color );

    hr = IClosedCaptionPropertiesStatics_get_ComputedBackgroundColor( caption_statics, &computed_color );
    todo_wine ok( hr == E_INVALIDARG, "get_ComputedBackgroundColor returned %#lx\n", hr );
    hr = IClosedCaptionPropertiesStatics_get_ComputedBackgroundColor( caption_statics, NULL );
    todo_wine ok( hr == E_INVALIDARG, "get_ComputedBackgroundColor returned %#lx\n", hr );

    opacity = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_BackgroundOpacity( caption_statics, &opacity );
    ok( hr == S_OK, "get_BackgroundOpacity returned %#lx\n", hr );
    ok( opacity == ClosedCaptionOpacity_Default, "expected default background opacity, got %d\n", opacity );

    color = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_RegionColor( caption_statics, &color );
    ok( hr == S_OK, "get_RegionColor returned %#lx\n", hr );
    ok( color == ClosedCaptionColor_Default, "expected default region color, got %d\n", color );

    hr = IClosedCaptionPropertiesStatics_get_ComputedRegionColor( caption_statics, &computed_color );
    todo_wine ok( hr == E_INVALIDARG, "get_ComputedRegionColor returned %#lx\n", hr );
    hr = IClosedCaptionPropertiesStatics_get_ComputedRegionColor( caption_statics, NULL );
    todo_wine ok( hr == E_INVALIDARG, "get_ComputedRegionColor returned %#lx\n", hr );

    opacity = 0xdeadbeef;
    hr = IClosedCaptionPropertiesStatics_get_RegionOpacity( caption_statics, &opacity );
    ok( hr == S_OK, "get_RegionOpacity returned %#lx\n", hr );
    ok( opacity == ClosedCaptionOpacity_Default, "expected default region opacity, got %d\n", opacity );

    ref = IClosedCaptionPropertiesStatics_Release( caption_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_MediaTranscoderStatics(void)
{
    static const WCHAR *media_transcoder_name = L"Windows.Media.Transcoding.MediaTranscoder";
    IMediaTranscoder *media_transcoder = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    IInspectable *inspectable = (void *)0xdeadbeef;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( media_transcoder_name, wcslen( media_transcoder_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        WindowsDeleteString( str );
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( media_transcoder_name ));
        return;
    }

    check_interface( factory, &IID_IUnknown, TRUE );
    check_interface( factory, &IID_IInspectable, TRUE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_IMediaTranscoder, FALSE );

    hr = RoActivateInstance( str, &inspectable );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    hr = IInspectable_QueryInterface( inspectable, &IID_IMediaTranscoder, (void **)&media_transcoder );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( media_transcoder, &IID_IAgileObject, TRUE );

    ref = IMediaTranscoder_Release( media_transcoder );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IInspectable_Release( inspectable );
    ok( ref == 0, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(media)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_CaptionStatics();
    test_MediaTranscoderStatics();

    RoUninitialize();
}
