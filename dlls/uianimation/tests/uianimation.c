/*
 * UI Animation tests
 *
 * Copyright 2019 Alistair Leslie-Hughes
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

#include "windows.h"
#include "initguid.h"
#include "uianimation.h"

#include "wine/test.h"

static void test_UIAnimationManager(void)
{
    HRESULT hr;
    IUIAnimationManager *manager;
    IUIAnimationVariable *variable;

    hr = CoCreateInstance( &CLSID_UIAnimationManager, NULL, CLSCTX_ALL, &IID_IUIAnimationManager, (LPVOID*)&manager);
    if(FAILED(hr))
    {
        win_skip("UIAnimationManager not found\n");
        return;
    }

    hr = IUIAnimationManager_CreateAnimationVariable(manager, 1.0f, &variable);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    if (hr == S_OK)
        IUIAnimationVariable_Release(variable);

    IUIAnimationManager_Release(manager);
}

START_TEST(uianimation)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok(hr == S_OK, "failed to init com\n");
    if(hr != S_OK)
        return;

    test_UIAnimationManager();

    CoUninitialize();
}
