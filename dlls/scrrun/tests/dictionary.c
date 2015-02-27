/*
 * Copyright (C) 2012 Alistair Leslie-Hughes
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
#include <stdio.h>

#include "windows.h"
#include "ole2.h"
#include "oleauto.h"
#include "olectl.h"
#include "dispex.h"

#include "wine/test.h"

#include "scrrun.h"

static void test_interfaces(void)
{
    static const WCHAR key_add[] = {'a', 0};
    static const WCHAR key_add_value[] = {'a', 0};
    static const WCHAR key_non_exist[] = {'b', 0};
    HRESULT hr;
    IDispatch *disp;
    IDispatchEx *dispex;
    IDictionary *dict;
    IObjectWithSite *site;
    VARIANT key, value;
    VARIANT_BOOL exists;
    LONG count = 0;

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDispatch, (void**)&disp);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    VariantInit(&key);
    VariantInit(&value);

    hr = IDispatch_QueryInterface(disp, &IID_IDictionary, (void**)&dict);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);

    hr = IDispatch_QueryInterface(disp, &IID_IObjectWithSite, (void**)&site);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    V_VT(&key) = VT_BSTR;
    V_BSTR(&key) = SysAllocString(key_add);
    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = SysAllocString(key_add_value);
    hr = IDictionary_Add(dict, &key, &value);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    VariantClear(&value);

    exists = VARIANT_FALSE;
    hr = IDictionary_Exists(dict, &key, &exists);
    todo_wine ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    todo_wine ok(exists == VARIANT_TRUE, "Expected TRUE but got FALSE.\n");
    VariantClear(&key);

    exists = VARIANT_TRUE;
    V_VT(&key) = VT_BSTR;
    V_BSTR(&key) = SysAllocString(key_non_exist);
    hr = IDictionary_Exists(dict, &key, &exists);
    todo_wine ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    todo_wine ok(exists == VARIANT_FALSE, "Expected FALSE but got TRUE.\n");
    VariantClear(&key);

    hr = IDictionary_get_Count(dict, &count);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(count == 1, "got %d, expected 1\n", count);

    IDictionary_Release(dict);
    IDispatch_Release(disp);
}

static void test_comparemode(void)
{
    CompareMethod method;
    IDictionary *dict;
    VARIANT key, item;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDictionary, (void**)&dict);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if (0) /* crashes on native */
    hr = IDictionary_get_CompareMode(dict, NULL);

    method = 10;
    hr = IDictionary_get_CompareMode(dict, &method);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(method == BinaryCompare, "got %d\n", method);

    /* invalid mode value is not checked */
    hr = IDictionary_put_CompareMode(dict, 10);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDictionary_get_CompareMode(dict, &method);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(method == 10, "got %d\n", method);

    hr = IDictionary_put_CompareMode(dict, DatabaseCompare);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDictionary_get_CompareMode(dict, &method);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(method == DatabaseCompare, "got %d\n", method);

    /* try to change mode of a non-empty dict */
    V_VT(&key) = VT_I2;
    V_I2(&key) = 0;
    VariantInit(&item);
    hr = IDictionary_Add(dict, &key, &item);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDictionary_put_CompareMode(dict, BinaryCompare);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL, "got 0x%08x\n", hr);

    IDictionary_Release(dict);
}

static DWORD get_str_hash(const WCHAR *str, CompareMethod method)
{
    DWORD hash = 0;

    while (*str) {
        WCHAR ch;

        if (method == TextCompare || method == DatabaseCompare)
            ch = PtrToInt(CharLowerW(IntToPtr(*str)));
        else
            ch = *str;

        hash += (hash << 4) + ch;
        str++;
    }

    return hash % 1201;
}

static DWORD get_num_hash(FLOAT num)
{
    return (*((DWORD*)&num)) % 1201;
}

typedef union
{
    struct
    {
        unsigned int m : 23;
        unsigned int exp_bias : 8;
        unsigned int sign : 1;
    } i;
    float f;
} R4_FIELDS;

typedef union
{
    struct
    {
        unsigned int m_lo : 32;     /* 52 bits of precision */
        unsigned int m_hi : 20;
        unsigned int exp_bias : 11; /* bias == 1023 */
        unsigned int sign : 1;
    } i;
    double d;
} R8_FIELDS;

static void test_hash_value(void)
{
    /* string test data */
    static const WCHAR str_hash_tests[][10] = {
        {'a','b','c','d',0},
        {'a','B','C','d','1',0},
        {'1','2','3',0},
        {'A',0},
        {'a',0},
        { 0 }
    };

    static const int int_hash_tests[] = {
        0, -1, 100, 1, 255
    };

    static const FLOAT float_hash_tests[] = {
        0.0, -1.0, 100.0, 1.0, 255.0, 1.234
    };

    IDictionary *dict;
    VARIANT key, hash;
    R8_FIELDS fx8;
    R4_FIELDS fx4;
    HRESULT hr;
    unsigned i;

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDictionary, (void**)&dict);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&key) = VT_BSTR;
    V_BSTR(&key) = NULL;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == 0, "got %d\n", V_I4(&hash));

    for (i = 0; i < sizeof(str_hash_tests)/sizeof(str_hash_tests[0]); i++) {
        DWORD expected = get_str_hash(str_hash_tests[i], BinaryCompare);

        hr = IDictionary_put_CompareMode(dict, BinaryCompare);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        V_VT(&key) = VT_BSTR;
        V_BSTR(&key) = SysAllocString(str_hash_tests[i]);
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: binary mode: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);
        VariantClear(&key);

        expected = get_str_hash(str_hash_tests[i], TextCompare);
        hr = IDictionary_put_CompareMode(dict, TextCompare);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        V_VT(&key) = VT_BSTR;
        V_BSTR(&key) = SysAllocString(str_hash_tests[i]);
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: text mode: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);
        VariantClear(&key);

        expected = get_str_hash(str_hash_tests[i], DatabaseCompare);
        hr = IDictionary_put_CompareMode(dict, DatabaseCompare);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        V_VT(&key) = VT_BSTR;
        V_BSTR(&key) = SysAllocString(str_hash_tests[i]);
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: db mode: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);
        VariantClear(&key);
    }

    V_VT(&key) = VT_INT;
    V_INT(&key) = 1;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u, "got hash 0x%08x\n", V_I4(&hash));

    V_VT(&key) = VT_UINT;
    V_UINT(&key) = 1;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u, "got hash 0x%08x\n", V_I4(&hash));

    V_VT(&key) = VT_I1;
    V_I1(&key) = 1;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u || broken(V_I4(&hash) == 0xa1), "got hash 0x%08x\n", V_I4(&hash));

    V_VT(&key) = VT_I8;
    V_I8(&key) = 1;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u, "got hash 0x%08x\n", V_I4(&hash));

    V_VT(&key) = VT_UI2;
    V_UI2(&key) = 1;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u, "got hash 0x%08x\n", V_I4(&hash));

    V_VT(&key) = VT_UI4;
    V_UI4(&key) = 1;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u, "got hash 0x%08x\n", V_I4(&hash));

    for (i = 0; i < sizeof(int_hash_tests)/sizeof(int_hash_tests[0]); i++) {
        DWORD expected = get_num_hash(int_hash_tests[i]);

        V_VT(&key) = VT_I2;
        V_I2(&key) = int_hash_tests[i];
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);

        V_VT(&key) = VT_I4;
        V_I4(&key) = int_hash_tests[i];
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);

        expected = get_num_hash((FLOAT)(BYTE)int_hash_tests[i]);
        V_VT(&key) = VT_UI1;
        V_UI1(&key) = int_hash_tests[i];
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);
    }

    /* nan */
    fx4.f = 10.0;
    fx4.i.exp_bias = 0xff;

    V_VT(&key) = VT_R4;
    V_R4(&key) = fx4.f;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u || broken(V_I4(&hash) == 0 /* win2k */ ||
        V_I4(&hash) == 0x1f4 /* vista, win2k8 */), "got hash 0x%08x\n", V_I4(&hash));

    /* inf */
    fx4.f = 10.0;
    fx4.i.m = 0;
    fx4.i.exp_bias = 0xff;

    V_VT(&key) = VT_R4;
    V_R4(&key) = fx4.f;
    V_I4(&hash) = 10;
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == 0, "got hash 0x%08x\n", V_I4(&hash));

    /* nan */
    fx8.d = 10.0;
    fx8.i.exp_bias = 0x7ff;

    V_VT(&key) = VT_R8;
    V_R8(&key) = fx8.d;
    VariantInit(&hash);
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL || broken(hr == S_OK) /* win2k, win2k3 */, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == ~0u || broken(V_I4(&hash) == 0 /* win2k */ ||
        V_I4(&hash) == 0x1f4 /* vista, win2k8 */), "got hash 0x%08x\n", V_I4(&hash));

    /* inf */
    fx8.d = 10.0;
    fx8.i.m_lo = 0;
    fx8.i.m_hi = 0;
    fx8.i.exp_bias = 0x7ff;

    V_VT(&key) = VT_R8;
    V_R8(&key) = fx8.d;
    V_I4(&hash) = 10;
    hr = IDictionary_get_HashVal(dict, &key, &hash);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
    ok(V_I4(&hash) == 0, "got hash 0x%08x\n", V_I4(&hash));

    for (i = 0; i < sizeof(float_hash_tests)/sizeof(float_hash_tests[0]); i++) {
        DWORD expected = get_num_hash(float_hash_tests[i]);

        V_VT(&key) = VT_R4;
        V_R4(&key) = float_hash_tests[i];
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);

        V_VT(&key) = VT_R8;
        V_R8(&key) = float_hash_tests[i];
        VariantInit(&hash);
        hr = IDictionary_get_HashVal(dict, &key, &hash);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(V_VT(&hash) == VT_I4, "got %d\n", V_VT(&hash));
        ok(V_I4(&hash) == expected, "%d: got hash 0x%08x, expected 0x%08x\n", i, V_I4(&hash),
            expected);
    }

    IDictionary_Release(dict);
}

static void test_Exists(void)
{
    VARIANT_BOOL exists;
    IDictionary *dict;
    VARIANT key, item;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDictionary, (void**)&dict);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if (0) /* crashes on native */
    hr = IDictionary_Exists(dict, NULL, NULL);

    V_VT(&key) = VT_I2;
    V_I2(&key) = 0;
    hr = IDictionary_Exists(dict, &key, NULL);
todo_wine
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL, "got 0x%08x\n", hr);

    V_VT(&key) = VT_I2;
    V_I2(&key) = 0;
    exists = VARIANT_TRUE;
    hr = IDictionary_Exists(dict, &key, &exists);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == VARIANT_FALSE, "got %x\n", exists);
}
    VariantInit(&item);
    hr = IDictionary_Add(dict, &key, &item);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&key) = VT_R4;
    V_R4(&key) = 0.0;
    hr = IDictionary_Add(dict, &key, &item);
    ok(hr == CTL_E_KEY_ALREADY_EXISTS, "got 0x%08x\n", hr);

    V_VT(&key) = VT_I2;
    V_I2(&key) = 0;
    hr = IDictionary_Exists(dict, &key, NULL);
todo_wine
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL, "got 0x%08x\n", hr);

    V_VT(&key) = VT_I2;
    V_I2(&key) = 0;
    exists = VARIANT_FALSE;
    hr = IDictionary_Exists(dict, &key, &exists);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == VARIANT_TRUE, "got %x\n", exists);
}
    /* key of different type, but resolves to same hash value */
    V_VT(&key) = VT_R4;
    V_R4(&key) = 0.0;
    exists = VARIANT_FALSE;
    hr = IDictionary_Exists(dict, &key, &exists);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(exists == VARIANT_TRUE, "got %x\n", exists);
}
    IDictionary_Release(dict);
}

static void test_Keys(void)
{
    VARIANT key, keys, item;
    IDictionary *dict;
    LONG index;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDictionary, (void**)&dict);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDictionary_Keys(dict, NULL);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    VariantInit(&keys);
    hr = IDictionary_Keys(dict, &keys);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&keys) == (VT_ARRAY|VT_VARIANT), "got %d\n", V_VT(&keys));
}
    VariantClear(&keys);

    V_VT(&key) = VT_R4;
    V_R4(&key) = 0.0;
    VariantInit(&item);
    hr = IDictionary_Add(dict, &key, &item);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    VariantInit(&keys);
    hr = IDictionary_Keys(dict, &keys);
todo_wine {
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&keys) == (VT_ARRAY|VT_VARIANT), "got %d\n", V_VT(&keys));
}
if (hr == S_OK)
{
    VariantInit(&key);
    index = 0;
    hr = SafeArrayGetElement(V_ARRAY(&keys), &index, &key);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&key) == VT_R4, "got %d\n", V_VT(&key));

    index = SafeArrayGetDim(V_ARRAY(&keys));
    ok(index == 1, "got %d\n", index);

    hr = SafeArrayGetUBound(V_ARRAY(&keys), 1, &index);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(index == 0, "got %d\n", index);
}
    VariantClear(&keys);

    IDictionary_Release(dict);
}

static void test_Remove(void)
{
    VARIANT key, item;
    IDictionary *dict;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDictionary, (void**)&dict);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if (0)
    hr = IDictionary_Remove(dict, NULL);

    /* nothing added yet */
    V_VT(&key) = VT_R4;
    V_R4(&key) = 0.0;
    hr = IDictionary_Remove(dict, &key);
todo_wine
    ok(hr == CTL_E_ELEMENT_NOT_FOUND, "got 0x%08x\n", hr);

    VariantInit(&item);
    hr = IDictionary_Add(dict, &key, &item);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDictionary_Remove(dict, &key);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IDictionary_Release(dict);
}

static void test_Item(void)
{
    VARIANT key, item;
    IDictionary *dict;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDictionary, (void**)&dict);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    V_VT(&key) = VT_I2;
    V_I2(&key) = 10;
    V_VT(&item) = VT_I2;
    V_I2(&item) = 123;
    hr = IDictionary_get_Item(dict, &key, &item);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&item) == VT_EMPTY, "got %d\n", V_VT(&item));

    V_VT(&key) = VT_I2;
    V_I2(&key) = 10;
    V_VT(&item) = VT_I2;
    hr = IDictionary_get_Item(dict, &key, &item);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&item) == VT_EMPTY, "got %d\n", V_VT(&item));

    IDictionary_Release(dict);
}

START_TEST(dictionary)
{
    IDispatch *disp;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_Dictionary, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDispatch, (void**)&disp);
    if(FAILED(hr)) {
        win_skip("Dictionary object is not supported: %08x\n", hr);
        CoUninitialize();
        return;
    }
    IDispatch_Release(disp);

    test_interfaces();
    test_comparemode();
    test_hash_value();
    test_Exists();
    test_Keys();
    test_Remove();
    test_Item();

    CoUninitialize();
}
