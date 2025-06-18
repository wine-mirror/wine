/*
 * Copyright 2022 Zhiyi Zhang for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <atlbase.h>

#include <wine/test.h>

static void test_AtlComModuleGetClassObject(void)
{
    _ATL_OBJMAP_ENTRY_EX *null_entry = NULL;
    _ATL_COM_MODULE module;
    HRESULT hr;
    void *ret;

    /* Test NULL module */
    hr = AtlComModuleGetClassObject(NULL, &GUID_NULL, &IID_NULL, &ret);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Test NULL m_ppAutoObjMapFirst and m_ppAutoObjMapLast */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = NULL;
    module.m_ppAutoObjMapLast = NULL;
    hr = AtlComModuleGetClassObject(&module, &GUID_NULL, &IID_NULL, &ret);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);

    /* Test m_ppAutoObjMapFirst and m_ppAutoObjMapLast both pointing to a NULL entry */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = &null_entry;
    module.m_ppAutoObjMapLast = &null_entry;
    hr = AtlComModuleGetClassObject(&module, &GUID_NULL, &IID_NULL, &ret);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);
}

static void test_AtlComModuleRegisterClassObjects(void)
{
    _ATL_OBJMAP_ENTRY_EX *null_entry = NULL;
    _ATL_COM_MODULE module;
    HRESULT hr;

    /* Test NULL module */
    hr = AtlComModuleRegisterClassObjects(NULL, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Test NULL m_ppAutoObjMapFirst and m_ppAutoObjMapLast */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = NULL;
    module.m_ppAutoObjMapLast = NULL;
    hr = AtlComModuleRegisterClassObjects(&module, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE);
    todo_wine_if(hr == S_OK)
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    /* Test m_ppAutoObjMapFirst and m_ppAutoObjMapLast both pointing to a NULL entry */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = &null_entry;
    module.m_ppAutoObjMapLast = &null_entry;
    hr = AtlComModuleRegisterClassObjects(&module, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE);
    todo_wine_if(hr == S_OK)
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
}

static void test_AtlComModuleRevokeClassObjects(void)
{
    _ATL_OBJMAP_ENTRY_EX *null_entry = NULL;
    _ATL_COM_MODULE module;
    HRESULT hr;

    /* Test NULL module */
    hr = AtlComModuleRevokeClassObjects(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Test NULL m_ppAutoObjMapFirst and m_ppAutoObjMapLast */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = NULL;
    module.m_ppAutoObjMapLast = NULL;
    hr = AtlComModuleRevokeClassObjects(&module);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Test m_ppAutoObjMapFirst and m_ppAutoObjMapLast both pointing to a NULL entry */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = &null_entry;
    module.m_ppAutoObjMapLast = &null_entry;
    hr = AtlComModuleRevokeClassObjects(&module);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

START_TEST(atl)
{
    CoInitialize(NULL);

    test_AtlComModuleGetClassObject();
    test_AtlComModuleRegisterClassObjects();
    test_AtlComModuleRevokeClassObjects();

    CoUninitialize();
}
