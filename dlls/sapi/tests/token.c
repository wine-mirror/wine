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
    WCHAR *value;

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

    hr = ISpRegDataKey_SetKey( data_key, key, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );
    hr = ISpRegDataKey_SetKey( data_key, key, FALSE );
    ok( hr == SPERR_ALREADY_INITIALIZED, "got %08lx\n", hr );

    hr = ISpRegDataKey_GetStringValue( data_key, L"Voice", &value );
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );

    hr = ISpRegDataKey_GetStringValue( data_key, L"", &value );
    ok( hr == SPERR_NOT_FOUND, "got %08lx\n", hr );

    hr = ISpRegDataKey_CreateKey( data_key, L"Testing", &sub );
    ok( hr == S_OK, "got %08lx\n", hr );
    ISpDataKey_Release(sub);

    ISpRegDataKey_Release( data_key );
}

static void test_token_category(void)
{
    ISpObjectTokenCategory *cat;
    IEnumSpObjectTokens *enum_tokens;
    HRESULT hr;
    ULONG count;

    hr = CoCreateInstance( &CLSID_SpObjectTokenCategory, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenCategory, (void **)&cat );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_EnumTokens( cat, NULL, NULL, &enum_tokens );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, L"bogus", FALSE );
    ok( hr == SPERR_INVALID_REGISTRY_KEY, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, SPCAT_VOICES, FALSE );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_SetId( cat, SPCAT_VOICES, FALSE );
    ok( hr == SPERR_ALREADY_INITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenCategory_EnumTokens( cat, NULL, NULL, &enum_tokens );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IEnumSpObjectTokens_GetCount( enum_tokens, &count );
    ok( hr == S_OK, "got %08lx\n", hr );

    IEnumSpObjectTokens_Release( enum_tokens );
    ISpObjectTokenCategory_Release( cat );
}

static void test_token_enum(void)
{
    ISpObjectTokenEnumBuilder *token_enum;
    HRESULT hr;
    ISpObjectToken *token;
    ULONG count;

    hr = CoCreateInstance( &CLSID_SpObjectTokenEnum, NULL, CLSCTX_INPROC_SERVER,
                           &IID_ISpObjectTokenEnumBuilder, (void **)&token_enum );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_GetCount( token_enum, &count );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 1, &token, &count );
    ok( hr == SPERR_UNINITIALIZED, "got %08lx\n", hr );

    hr = ISpObjectTokenEnumBuilder_SetAttribs( token_enum, NULL, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_GetCount( token_enum, &count );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( count == 0, "got %lu\n", count );

    count = 0xdeadbeef;
    hr = ISpObjectTokenEnumBuilder_Next( token_enum, 1, &token, &count );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    ok( count == 0, "got %lu\n", count );

    ISpObjectTokenEnumBuilder_Release( token_enum );
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

static void test_object_token(void)
{
    ISpObjectToken *token;
    ISpDataKey *sub_key;
    HRESULT hr;
    LPWSTR tempW, token_id;
    ISpObjectTokenCategory *cat;

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

    ISpObjectToken_Release( token );
}

START_TEST(token)
{
    CoInitialize( NULL );
    test_data_key();
    test_token_category();
    test_token_enum();
    test_default_token_id();
    test_object_token();
    CoUninitialize();
}
