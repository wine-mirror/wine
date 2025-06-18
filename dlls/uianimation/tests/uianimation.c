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
    IUIAnimationStoryboard *storyboard;

    hr = CoCreateInstance( &CLSID_UIAnimationManager, NULL, CLSCTX_ALL, &IID_IUIAnimationManager, (LPVOID*)&manager);
    if(FAILED(hr))
    {
        win_skip("UIAnimationManager not found\n");
        return;
    }

    hr = IUIAnimationManager_CreateAnimationVariable(manager, 1.0f, &variable);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (hr == S_OK)
        IUIAnimationVariable_Release(variable);

    hr = IUIAnimationManager_CreateStoryboard(manager, &storyboard);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (hr == S_OK)
        IUIAnimationStoryboard_Release(storyboard);

    IUIAnimationManager_Release(manager);
}

static void test_IUIAnimationTimer(void)
{
    HRESULT hr;
    IUIAnimationTimer *timer;

    hr = CoCreateInstance( &CLSID_UIAnimationTimer, NULL, CLSCTX_ALL, &IID_IUIAnimationTimer, (void**)&timer);
    if(FAILED(hr))
    {
        win_skip("IUIAnimationTimer not found\n");
        return;
    }

    hr = IUIAnimationTimer_IsEnabled(timer);
    todo_wine ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    hr = IUIAnimationTimer_Enable(timer);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IUIAnimationTimer_Enable(timer);
    todo_wine ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    hr = IUIAnimationTimer_IsEnabled(timer);
    todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IUIAnimationTimer_Disable(timer);
    todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IUIAnimationTimer_Disable(timer);
    todo_wine ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    hr = IUIAnimationTimer_IsEnabled(timer);
    todo_wine ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    IUIAnimationTimer_Release(timer);
}

static void test_IUIAnimationTransitionFactory(void)
{
    HRESULT hr;
    IUIAnimationTransitionFactory *factory;
    IUIAnimationTransition *transition = NULL;

    hr = CoCreateInstance( &CLSID_UIAnimationTransitionFactory, NULL, CLSCTX_ALL,
                            &IID_IUIAnimationTransitionFactory, (void**)&factory);
    if (FAILED(hr))
    {
        win_skip("IUIAnimationTransitionFactory not found\n");
        return;
    }

    hr = IUIAnimationTransitionFactory_CreateTransition(factory, NULL, &transition);
    todo_wine ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    IUIAnimationTransitionFactory_Release(factory);
}

static void test_IUIAnimationTransitionLibrary(void)
{
    HRESULT hr;
    IUIAnimationTransitionLibrary *library;
    IUIAnimationTransition *instantaneous, *linear, *smooth;

    hr = CoCreateInstance( &CLSID_UIAnimationTransitionLibrary, NULL, CLSCTX_ALL,
                            &IID_IUIAnimationTransitionLibrary, (void**)&library);
    if (FAILED(hr))
    {
        win_skip("IUIAnimationTransitionLibrary not found\n");
        return;
    }

    hr = IUIAnimationTransitionLibrary_CreateInstantaneousTransition(library, 100.0, &instantaneous);
    todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
    if  (hr == S_OK)
        IUIAnimationTransition_Release(instantaneous);

    hr = IUIAnimationTransitionLibrary_CreateLinearTransition(library, 500.0, 100.0, &linear);
    todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
    if  (hr == S_OK)
        IUIAnimationTransition_Release(linear);

    hr = IUIAnimationTransitionLibrary_CreateSmoothStopTransition(library, 500.0, 100.0, &smooth);
    todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);
    if  (hr == S_OK)
        IUIAnimationTransition_Release(smooth);

    IUIAnimationTransitionLibrary_Release(library);
}

START_TEST(uianimation)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok(hr == S_OK, "failed to init com\n");
    if(hr != S_OK)
        return;

    test_UIAnimationManager();
    test_IUIAnimationTimer();
    test_IUIAnimationTransitionFactory();
    test_IUIAnimationTransitionLibrary();

    CoUninitialize();
}
