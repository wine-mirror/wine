/*
 * Speech API (SAPI) token tests.
 *
 * Copyright (C) 2017 Huw Davies
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
#include "sapiddk.h"
#include "sperror.h"

#include "wine/test.h"

static void test_data_key(void)
{
    ISpRegDataKey *data_key;
    ISpDataKey *sub;
    HRESULT hr;
    HKEY key;
    LONG res;
    WCHAR *value = NULL;

    hr = CoCreateInstance( &CLSID_SpDataKey, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpRegDataKey, (void **)&data_key );
    ok( hr == S_OK, "got %08lx\n", hr );

    res = RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\Winetest\\sapi", 0, NULL, 0, KEY_ALL_ACCESS,
                           NULL, &key, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    hr = ISpRegDataKey_CreateKey( data_key, L"Testing", &sub );
    ok( hr == E_HANDLE, "got %08lx\n", hr );

    hr = ISpRegDataKey_GetStringValue( data_key, L"Voice", &value );
    ok( hr == E_HANDLE, "got %08lx\n", hr );

    hr = ISpRegDataKey_SetStringValue( data_key, L"Voice", L"Test" );
    ok( hr == E_HANDLE, "got %08lx\n", hr );

    hr = ISpRegDataKey_SetKey( data_key, key, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpRegDataKey_SetKey( data_key, key, FALSE );
    ok( hr == SPERR_ALREADY_INITIALIZED, "got %08lx\n", hr );

    hr = ISpRegDataKey_GetStringValue( data_key, L"Voice", &value );
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );

    hr = ISpRegDataKey_GetStringValue( data_key, L"", &value );
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );

    hr = ISpRegDataKey_SetStringValue( data_key, L"Voice", L"Test" );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpRegDataKey_GetStringValue( data_key, L"Voice", &value );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !wcscmp( value, L"Test" ), "got %s\n", wine_dbgstr_w(value) );
    CoTaskMemFree( value );

    hr = ISpRegDataKey_OpenKey( data_key, L"Testing", &sub );
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );

    hr = ISpRegDataKey_CreateKey( data_key, L"Testing", &sub );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpDataKey_Release(sub);

    hr = ISpRegDataKey_OpenKey( data_key, L"Testing", &sub );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpDataKey_Release(sub);

    ISpRegDataKey_Release( data_key );

    hr = CoCreateInstance( &CLSID_SpDataKey, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpRegDataKey, (void **)&data_key );
    ok( hr == S_OK, "got %08lx\n", hr );

    res = RegOpenKeyExA( HKEY_CURRENT_USER, "Software\\Winetest\\sapi", 0, KEY_ALL_ACCESS, &key );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    hr = ISpRegDataKey_SetKey( data_key, key, TRUE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpRegDataKey_SetStringValue( data_key, L"Voice2", L"Test2" );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpRegDataKey_GetStringValue( data_key, L"Voice2", &value );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( !wcscmp( value, L"Test2" ), "got %s\n", wine_dbgstr_w(value) );
    CoTaskMemFree( value );

    hr = ISpRegDataKey_CreateKey( data_key, L"Testing2", &sub );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpDataKey_Release(sub);

    ISpRegDataKey_Release( data_key );
}

static void setup_test_voice_tokens(void)
{
    HKEY key;
    ISpRegDataKey *data_key;
    ISpDataKey *attrs_key;
    LSTATUS ret;
    HRESULT hr;

    ret = RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\Winetest\\sapi\\TestVoices\\Tokens", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( ret == ERROR_SUCCESS, "got %ld\n", ret );

    hr = CoCreateInstance( &CLSID_SpDataKey, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpRegDataKey, (void **)&data_key );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpRegDataKey_SetKey( data_key, key, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );

    ISpRegDataKey_CreateKey( data_key, L"Voice1\\Attributes", &attrs_key );
    ISpDataKey_SetStringValue( attrs_key, L"Language", L"409" );
    ISpDataKey_SetStringValue( attrs_key, L"Gender", L"Female" );
    ISpDataKey_SetStringValue( attrs_key, L"Age", L"Child" );
    ISpDataKey_SetStringValue( attrs_key, L"Vendor", L"Vendor2" );
    ISpDataKey_Release( attrs_key );

    ISpRegDataKey_CreateKey( data_key, L"Voice2\\Attributes", &attrs_key );
    ISpDataKey_SetStringValue( attrs_key, L"Language", L"406;407;408;409;40a" );
    ISpDataKey_SetStringValue( attrs_key, L"Gender", L"Female" );
    ISpDataKey_SetStringValue( attrs_key, L"Age", L"Adult" );
    ISpDataKey_SetStringValue( attrs_key, L"Vendor", L"Vendor1" );
    ISpDataKey_Release( attrs_key );

    ISpRegDataKey_CreateKey( data_key, L"Voice3\\Attributes", &attrs_key );
    ISpDataKey_SetStringValue( attrs_key, L"Language", L"409;411" );
    ISpDataKey_SetStringValue( attrs_key, L"Gender", L"Female" );
    ISpDataKey_SetStringValue( attrs_key, L"Age", L"Child" );
    ISpDataKey_SetStringValue( attrs_key, L"Vendor", L"Vendor1" );
    ISpDataKey_Release( attrs_key );

    ISpRegDataKey_CreateKey( data_key, L"Voice4\\Attributes", &attrs_key );
    ISpDataKey_SetStringValue( attrs_key, L"Language", L"411" );
    ISpDataKey_SetStringValue( attrs_key, L"Gender", L"Male" );
    ISpDataKey_SetStringValue( attrs_key, L"Age", L"Adult" );
    ISpDataKey_Release( attrs_key );

    ISpRegDataKey_CreateKey( data_key, L"Voice5\\Attributes", &attrs_key );
    ISpDataKey_SetStringValue( attrs_key, L"Language", L"411" );
    ISpDataKey_SetStringValue( attrs_key, L"Gender", L"Female" );
    ISpDataKey_SetStringValue( attrs_key, L"Age", L"Adult" );
    ISpDataKey_SetStringValue( attrs_key, L"Vendor", L"Vendor2" );
    ISpDataKey_Release( attrs_key );

    ISpRegDataKey_Release( data_key );
}

static const WCHAR test_cat[] = L"HKEY_CURRENT_USER\\Software\\Winetest\\sapi\\TestVoices";

static void test_token_category(void)
{
    ISpObjectTokenCategory *cat;
    IEnumSpObjectTokens *enum_tokens;
    HRESULT hr;
    ULONG count;
    int i;
    ISpObjectToken *token;
    WCHAR *token_id;
    WCHAR tmp[MAX_PATH];

    hr = CoCreateInstance( &CLSID_SpObjectTokenCategory, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenCategory, (void **)&cat );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_EnumTokens( cat, NULL, NULL, &enum_tokens );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, L"bogus", FALSE );
    ok( hr == SPERR_INVALID_REGISTRY_KEY, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, test_cat, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, test_cat, FALSE );
    ok( hr == SPERR_ALREADY_INITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_EnumTokens( cat, NULL, NULL, &enum_tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = IEnumSpObjectTokens_GetCount( enum_tokens, &count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( count == 5, "got %lu\n", count );

    IEnumSpObjectTokens_Release( enum_tokens );

    hr = ISpObjectTokenCategory_EnumTokens( cat, L"Language=409", NULL, &enum_tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = IEnumSpObjectTokens_GetCount( enum_tokens, &count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( count == 3, "got %lu\n", count );

    IEnumSpObjectTokens_Release( enum_tokens );

    hr = ISpObjectTokenCategory_EnumTokens( cat, L"Language=409", L"Vendor=Vendor1;Age=Child;Gender=Female",
                                            &enum_tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = IEnumSpObjectTokens_GetCount( enum_tokens, &count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( count == 3, "got %lu\n", count );

    for ( i = 0; i < 3; i++ ) {
        token = NULL;
        hr = IEnumSpObjectTokens_Item( enum_tokens, i, &token );
        ok( hr == S_OK, "i = %d: got %08lx\n", i, hr );

        token_id = NULL;
        hr = ISpObjectToken_GetId( token, &token_id );
        ok( hr == S_OK, "i = %d: got %08lx\n", i, hr );
        swprintf( tmp, MAX_PATH, L"%ls\\Tokens\\Voice%d", test_cat, 3 - i );
        ok( !wcscmp( token_id, tmp ), "i = %d: got %s\n", i, wine_dbgstr_w(token_id) );

        CoTaskMemFree( token_id );
        ISpObjectToken_Release( token );
    }

    IEnumSpObjectTokens_Release( enum_tokens );

    ISpObjectTokenCategory_Release( cat );
}

static void test_token_enum(void)
{
    ISpObjectTokenEnumBuilder *token_enum;
    HRESULT hr;
    IDispatch *disp;
    ISpeechObjectTokens *speech_tokens;
    IUnknown *unk;
    IEnumVARIANT *enumvar;
    ISpObjectToken *tokens[5];
    ISpObjectToken *out_tokens[5];
    WCHAR token_id[MAX_PATH];
    ULONG count;
    int i;

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_QueryInterface( token_enum,
                                                   &IID_IDispatch, (void **)&disp );
    ok( hr == S_OK, "got %08lx\n", hr );
    IDispatch_Release( disp );

    hr = ISpObjectTokenEnumBuilder_QueryInterface( token_enum,
                                                   &IID_ISpeechObjectTokens,
                                                   (void **)&speech_tokens );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpeechObjectTokens_Release( speech_tokens );

    ISpObjectTokenEnumBuilder_Release( token_enum );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IDispatch, (void **)&disp );
    ok( hr == S_OK, "got %08lx\n", hr );
    IDispatch_Release( disp );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpeechObjectTokens, (void **)&speech_tokens );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpeechObjectTokens_Release( speech_tokens );


    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_GetCount( token_enum, &count );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 1, tokens, &count );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, NULL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_GetCount( token_enum, &count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( count == 0, "got %lu\n", count );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 1, &out_tokens[0], &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 0, "got %lu\n", count );

    for ( i = 0; i < 5; i++ ) {
        hr = CoCreateInstance( &CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                               &IID_ISpObjectToken, (void **)&tokens[i] );
        ok( hr == S_OK, "got %08lx\n", hr );

        swprintf( token_id, MAX_PATH, L"%ls\\Tokens\\Voice%d", test_cat, i + 1 );
        hr = ISpObjectToken_SetId( tokens[i], NULL, token_id, FALSE );
        ok( hr == S_OK, "got %08lx\n", hr );
    }

    hr = ISpObjectTokenEnumBuilder_AddTokens( token_enum, 3, tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    out_tokens[0] = (ISpObjectToken *)0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 1, &out_tokens[0], NULL );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( out_tokens[0] == tokens[0], "got %p\n", out_tokens[0] );

    out_tokens[0] = (ISpObjectToken *)0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Item( token_enum, 0, &out_tokens[0] );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( out_tokens[0] == tokens[0], "got %p\n", out_tokens[0] );

    hr = ISpObjectTokenEnumBuilder_Item( token_enum, 3, &out_tokens[0] );
    ok( hr == SPERR_NO_MORE_ITEMS, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 3, &out_tokens[1], NULL );
    ok( hr == E_POINTER, "got %08lx\n", hr );

    count = 0xdeadbeef;
    out_tokens[1] = out_tokens[2] = (ISpObjectToken *)0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 3, &out_tokens[1], &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 2, "got %lu\n", count );
    ok( out_tokens[1] == tokens[1], "got %p\n", out_tokens[1] );
    ok( out_tokens[2] == tokens[2], "got %p\n", out_tokens[2] );

    ISpObjectTokenEnumBuilder_Release( token_enum );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_QueryInterface( token_enum,
                                                   &IID_ISpeechObjectTokens,
                                                   (void **)&speech_tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, NULL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpObjectTokenEnumBuilder_AddTokens( token_enum, 3, tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpeechObjectTokens_get__NewEnum( speech_tokens, &unk );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = IUnknown_QueryInterface( unk, &IID_IEnumVARIANT, (void **)&enumvar );
    ok( hr == S_OK, "got %08lx\n", hr );
    IUnknown_Release( unk );
    IEnumVARIANT_Release( enumvar );

    ISpeechObjectTokens_Release( speech_tokens );
    ISpObjectTokenEnumBuilder_Release( token_enum );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Vendor attribute must exist */
    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, L"Vendor", NULL );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpObjectTokenEnumBuilder_AddTokens( token_enum, 5, tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 5, &out_tokens[0], &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 4, "got %lu\n", count );

    ISpObjectTokenEnumBuilder_Release( token_enum );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Vendor attribute must contain Vendor1 */
    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, L"Vendor=Vendor1", NULL );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpObjectTokenEnumBuilder_AddTokens( token_enum, 5, tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 5, &out_tokens[0], &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 2, "got %lu\n", count );

    ISpObjectTokenEnumBuilder_Release( token_enum );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Vendor attribute must not contain Vendor1 */
    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, L"Vendor!=Vendor1", NULL );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpObjectTokenEnumBuilder_AddTokens( token_enum, 5, tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 5, &out_tokens[0], &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 3, "got %lu\n", count );

    ISpObjectTokenEnumBuilder_Release( token_enum );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* Vendor attribute must contain Vendor1 and Language attribute must contain 407 */
    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, L"Vendor=Vendor1;Language=407", NULL );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpObjectTokenEnumBuilder_AddTokens( token_enum, 5, tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 5, &out_tokens[0], &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 1, "got %lu\n", count );

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, L"Language=409",
                                               L"Vendor=Vendor1;Age=Child;Gender=Female" );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpObjectTokenEnumBuilder_AddTokens( token_enum, 5, tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_Sort( token_enum, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 5, &out_tokens[0], &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 3, "got %lu\n", count );
    ok( out_tokens[0] == tokens[2], "got %p\n", out_tokens[0] );
    ok( out_tokens[1] == tokens[1], "got %p\n", out_tokens[1] );
    ok( out_tokens[2] == tokens[0], "got %p\n", out_tokens[2] );

    ISpObjectTokenEnumBuilder_Release( token_enum );

    for ( i = 0; i < 5; i++ ) ISpObjectToken_Release( tokens[i] );
}

static void test_default_token_id(void)
{
    ISpObjectTokenCategory *cat;
    HRESULT hr;
    LPWSTR token_id = NULL;
    LONG res;
    WCHAR regvalue[512];
    DWORD regvalue_size;

    hr = CoCreateInstance( &CLSID_SpObjectTokenCategory, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenCategory, (void **)&cat );
    ok( hr == S_OK, "got %08lx\n", hr );

    token_id = (LPWSTR)0xdeadbeef;
    hr = ISpObjectTokenCategory_GetDefaultTokenId( cat, &token_id );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );
    ok( token_id == (LPWSTR)0xdeadbeef, "got %p\n", token_id );

    hr = ISpObjectTokenCategory_GetDefaultTokenId( cat, NULL );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, SPCAT_AUDIOOUT, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_GetDefaultTokenId( cat, NULL );
    ok( hr == E_POINTER, "got %08lx\n", hr );

    token_id = (LPWSTR)0xdeadbeef;
    hr = ISpObjectTokenCategory_GetDefaultTokenId( cat, &token_id );

    /* AudioOutput under windows server returns this error */
    if (hr == SPERR_NOT_FOUND) {
        /* also happens if TokenEnums/Tokens is empty or doesn't exist */
        skip( "AudioOutput category not found for GetDefaultTokenId\n" );
        return;
    }

    ok( hr == S_OK, "got %08lx\n", hr );
    ok( token_id != (LPWSTR)0xdeadbeef && token_id != NULL, "got %p\n", token_id );

    regvalue_size = sizeof( regvalue );
    res = RegGetValueW( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Speech\\AudioOutput",
                        L"DefaultDefaultTokenId", RRF_RT_REG_SZ, NULL,
                        (LPVOID)&regvalue, &regvalue_size);
    if (res == ERROR_FILE_NOT_FOUND) {
        skip( "DefaultDefaultTokenId not found for AudioOutput category (%s)\n",
              wine_dbgstr_w(token_id) );
    } else {
        ok( res == ERROR_SUCCESS, "got %08lx\n", res );
        ok( !wcscmp(regvalue, token_id),
            "GetDefaultTokenId (%s) should be equal to the DefaultDefaultTokenId key (%s)\n",
            wine_dbgstr_w(token_id), wine_dbgstr_w(regvalue) );
    }

    CoTaskMemFree( token_id );
    ISpObjectTokenCategory_Release( cat );
}

static void tests_token_voices(void)
{
    ISpObjectTokenCategory *cat;
    HRESULT hr;
    IEnumSpObjectTokens *tokens;
    ISpObjectToken *item;
    ULONG fetched = 0;
    ULONG count;

    hr = CoCreateInstance( &CLSID_SpObjectTokenCategory, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenCategory, (void **)&cat );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, SPCAT_VOICES, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_EnumTokens(cat, NULL, NULL, &tokens);
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IEnumSpObjectTokens_GetCount( tokens, &count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( count != 0, "got %lu\n", count );
    ISpObjectTokenCategory_Release( cat );

    hr = IEnumSpObjectTokens_Item(tokens, 0, &item);
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpObjectToken_Release(item);

    hr = IEnumSpObjectTokens_Next(tokens, 1, &item, &fetched);
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpObjectToken_Release(item);

    IEnumSpObjectTokens_Release(tokens);
}


#define TESTCLASS_CLSID L"{67DD26B6-50BA-3297-253E-619346F177F8}"
static const GUID CLSID_TestClass = {0x67DD26B6,0x50BA,0x3297,{0x25,0x3E,0x61,0x93,0x46,0xF1,0x77,0xF8}};

static ISpObjectToken *test_class_token;

static HRESULT WINAPI test_class_QueryInterface(ISpObjectWithToken *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID( riid, &IID_IUnknown ) || IsEqualGUID( riid, &IID_ISpObjectWithToken ))
    {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_class_AddRef(ISpObjectWithToken *iface)
{
    return 2;
}

static ULONG WINAPI test_class_Release(ISpObjectWithToken *iface)
{
    return 1;
}

static HRESULT WINAPI test_class_SetObjectToken(ISpObjectWithToken *iface, ISpObjectToken *token)
{
    ok( token != NULL, "token == NULL\n" );
    test_class_token = token;
    ISpObjectToken_AddRef(test_class_token);
    return S_OK;
}

static HRESULT WINAPI test_class_GetObjectToken(ISpObjectWithToken *iface, ISpObjectToken **token)
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static const ISpObjectWithTokenVtbl test_class_vtbl = {
    test_class_QueryInterface,
    test_class_AddRef,
    test_class_Release,
    test_class_SetObjectToken,
    test_class_GetObjectToken
};

static ISpObjectWithToken test_class = { &test_class_vtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID( &IID_IUnknown, riid ) || IsEqualGUID( &IID_IClassFactory, riid ))
    {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface,
        IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    ok( pUnkOuter == NULL, "pUnkOuter != NULL\n" );
    ok( IsEqualGUID(riid, &IID_IUnknown), "riid = %s\n", wine_dbgstr_guid(riid) );

    *ppv = &test_class;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    ok( 0, "unexpected call\n" );
    return E_NOTIMPL;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory test_class_cf = { &ClassFactoryVtbl };

static void test_object_token(void)
{
    static const WCHAR test_token_id[] = L"HKEY_LOCAL_MACHINE\\Software\\Wine\\Winetest\\sapi\\TestToken";

    ISpObjectToken *token;
    IDispatch *disp;
    ISpeechObjectToken *speech_token;
    ISpDataKey *sub_key;
    HRESULT hr;
    LPWSTR tempW, token_id;
    ISpObjectTokenCategory *cat;
    DWORD regid;
    IUnknown *obj;

    hr = CoCreateInstance( &CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectToken, (void **)&token );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectToken_QueryInterface( token, &IID_IDispatch, (void **)&disp );
    ok( hr == S_OK, "got %08lx\n", hr );
    IDispatch_Release( disp );

    hr = ISpObjectToken_QueryInterface( token, &IID_ISpeechObjectToken, (void **)&speech_token );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpeechObjectToken_Release( speech_token );

    ISpObjectToken_Release( token );

    hr = CoCreateInstance( &CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IDispatch, (void **)&disp );
    ok( hr == S_OK, "got %08lx\n", hr );
    IDispatch_Release( disp );

    hr = CoCreateInstance( &CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpeechObjectToken, (void **)&speech_token );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpeechObjectToken_Release( speech_token );


    hr = CoCreateInstance( &CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectToken, (void **)&token );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectToken_GetId( token, NULL );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    tempW = (LPWSTR)0xdeadbeef;
    hr = ISpObjectToken_GetId( token, &tempW );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );
    ok( tempW == (LPWSTR)0xdeadbeef, "got %s\n", wine_dbgstr_w(tempW) );

    hr = ISpObjectToken_GetCategory( token, NULL );
    todo_wine ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    cat = (LPVOID)0xdeadbeef;
    hr = ISpObjectToken_GetCategory( token, &cat );
    todo_wine ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );
    ok( cat == (LPVOID)0xdeadbeef, "got %p\n", cat );

    hr = ISpObjectToken_SetId( token, NULL, NULL, FALSE );
    ok( hr == E_POINTER, "got %08lx\n", hr );
    hr = ISpObjectToken_SetId( token, L"bogus", NULL, FALSE );
    ok( hr == E_POINTER, "got %08lx\n", hr );

    hr = ISpObjectToken_SetId( token, NULL, L"bogus", FALSE );
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );
    hr = ISpObjectToken_SetId( token, NULL, L"HKEY_LOCAL_MACHINE\\SOFTWARE\\winetest bogus", FALSE );
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );

    /* SetId succeeds even if the key is invalid, but exists */
    hr = ISpObjectToken_SetId( token, NULL, L"HKEY_LOCAL_MACHINE\\SOFTWARE", FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectToken_SetId( token, NULL, NULL, FALSE );
    ok( hr == SPERR_ALREADY_INITIALIZED, "got %08lx\n", hr );
    hr = ISpObjectToken_SetId( token, NULL, L"bogus", FALSE );
    ok( hr == SPERR_ALREADY_INITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectToken_GetId( token, NULL );
    ok( hr == E_POINTER, "got %08lx\n", hr );

    hr = ISpObjectToken_GetCategory( token, NULL );
    todo_wine ok( hr == E_POINTER, "got %08lx\n", hr );

    tempW = NULL;
    hr = ISpObjectToken_GetId( token, &tempW );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( tempW != NULL, "got %p\n", tempW );
    if (tempW) {
        ok( !wcscmp(tempW, L"HKEY_LOCAL_MACHINE\\SOFTWARE"), "got %s\n",
            wine_dbgstr_w(tempW) );
        CoTaskMemFree( tempW );
    }

    hr = ISpObjectToken_OpenKey(token, L"Non-exist", &sub_key);
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );

    hr = ISpObjectToken_OpenKey(token, L"Classes", &sub_key);
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpDataKey_Release(sub_key);

    cat = (LPVOID)0xdeadbeef;
    hr = ISpObjectToken_GetCategory( token, &cat );
    todo_wine ok( hr == SPERR_INVALID_REGISTRY_KEY, "got %08lx\n", hr );
    ok( cat == (LPVOID)0xdeadbeef, "got %p\n", cat );

    /* get the default token id for SPCAT_AUDIOOUT */
    hr = CoCreateInstance( &CLSID_SpObjectTokenCategory, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenCategory, (void **)&cat );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpObjectTokenCategory_SetId( cat, SPCAT_AUDIOOUT, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );
    token_id = (LPWSTR)0xdeadbeef;
    hr = ISpObjectTokenCategory_GetDefaultTokenId( cat, &token_id );
    if (hr == SPERR_NOT_FOUND) {
        skip( "AudioOutput category not found for GetDefaultTokenId\n" );
        return;
    }
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( token_id != (LPWSTR)0xdeadbeef && token_id != NULL, "got %p\n", token_id );
    ISpObjectTokenCategory_Release( cat );

    /* recreate token in order to SetId again */
    ISpObjectToken_Release( token );
    hr = CoCreateInstance( &CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectToken, (void **)&token );
    ok( hr == S_OK, "got %08lx\n", hr );

    /* NULL appears to auto-detect the category */
    hr = ISpObjectToken_SetId( token, NULL, token_id, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );

    tempW = NULL;
    hr = ISpObjectToken_GetId( token, &tempW );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( tempW != NULL, "got %p\n", tempW );
    if (tempW) {
        ok( !wcsncmp(tempW, token_id, wcslen(token_id)),
            "got %s (expected %s)\n", wine_dbgstr_w(tempW), wine_dbgstr_w(token_id) );
        CoTaskMemFree( tempW );
    }

    cat = (LPVOID)0xdeadbeef;
    hr = ISpObjectToken_GetCategory( token, &cat );
    todo_wine ok( hr == S_OK, "got %08lx\n", hr );
    todo_wine ok( cat != (LPVOID)0xdeadbeef, "got %p\n", cat );
    if (cat != (LPVOID)0xdeadbeef) {
        tempW = NULL;
        hr = ISpObjectTokenCategory_GetId( cat, &tempW );
        todo_wine ok( hr == S_OK, "got %08lx\n", hr );
        todo_wine ok( tempW != NULL, "got %p\n", tempW );
        if (tempW) {
            ok( !wcscmp(tempW, SPCAT_AUDIOOUT), "got %s\n", wine_dbgstr_w(tempW) );
            CoTaskMemFree( tempW );
        }

        /* not freed by ISpObjectToken_Release */
        ISpObjectTokenCategory_Release( cat );
    }

    hr = CoRegisterClassObject( &CLSID_TestClass, (IUnknown *)&test_class_cf,
                                CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid );
    ok( hr == S_OK, "got %08lx\n", hr );

    ISpObjectToken_Release( token );
    hr = CoCreateInstance( &CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectToken, (void **)&token );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectToken_SetId( token, NULL, test_token_id, TRUE );
    ok( hr == S_OK || broken(hr == E_ACCESSDENIED) /* win1064_adm */, "got %08lx\n", hr );
    if (hr == E_ACCESSDENIED) {
        win_skip( "token SetId access denied\n" );
        ISpObjectToken_Release( token );
        return;
    }

    hr = ISpObjectToken_CreateKey( token, L"Attributes", &sub_key );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpDataKey_Release( sub_key );

    hr = ISpObjectToken_SetStringValue( token, L"CLSID", TESTCLASS_CLSID );
    ok( hr == S_OK, "got %08lx\n", hr );

    tempW = NULL;
    hr = ISpObjectToken_GetStringValue( token, L"CLSID", &tempW );

    ok( hr == S_OK, "got %08lx\n", hr );
    if ( tempW ) {
        ok( !wcsncmp( tempW, TESTCLASS_CLSID, wcslen(TESTCLASS_CLSID) ),
            "got %s (expected %s)\n", wine_dbgstr_w(tempW), wine_dbgstr_w(TESTCLASS_CLSID) );
        CoTaskMemFree( tempW );
    }

    test_class_token = NULL;
    hr = ISpObjectToken_CreateInstance( token, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&obj );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( test_class_token != NULL, "test_class_token not set\n" );

    tempW = NULL;
    hr = ISpObjectToken_GetId( test_class_token, &tempW );
    ok( tempW != NULL, "got %p\n", tempW );
    if (tempW) {
        ok( !wcsncmp(tempW, test_token_id, wcslen(test_token_id)),
            "got %s (expected %s)\n", wine_dbgstr_w(tempW), wine_dbgstr_w(test_token_id) );
        CoTaskMemFree( tempW );
    }

    ISpObjectToken_Release( test_class_token );
    IUnknown_Release( obj );
    ISpObjectToken_Release( token );

    RegDeleteTreeA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest\\sapi" );
}

START_TEST(token)
{
    CoInitialize( NULL );
    RegDeleteTreeA( HKEY_CURRENT_USER, "Software\\Winetest\\sapi" );
    test_data_key();
    setup_test_voice_tokens();
    test_token_category();
    test_token_enum();
    test_default_token_id();
    test_object_token();
    tests_token_voices();
    CoUninitialize();
}
