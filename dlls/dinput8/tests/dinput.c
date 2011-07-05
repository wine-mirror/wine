/*
 * Copyright (c) 2011 Andrew Nguyen
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

#define DIRECTINPUT_VERSION 0x0800

#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <dinput.h>

#include "wine/test.h"

HINSTANCE hInstance;

static void test_DirectInput8Create(void)
{
    static const struct
    {
        BOOL hinst;
        DWORD dwVersion;
        REFIID riid;
        BOOL ppdi;
        HRESULT expected_hr;
    } invalid_param_list[] =
    {
        {FALSE, 0,                       &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, 0,                       &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, 0,                       &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, 0,                       &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {TRUE,  0,                       &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  0,                       &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  0,                       &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  0,                       &IID_IDirectInput8A, TRUE,  DIERR_NOTINITIALIZED},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, TRUE,  DIERR_BETADIRECTINPUTVERSION},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, TRUE,  DIERR_OLDDIRECTINPUTVERSION},
    };

    static const REFIID no_interface_list[] = {&IID_IDirectInputA, &IID_IDirectInputW,
                                               &IID_IDirectInput2A, &IID_IDirectInput2W,
                                               &IID_IDirectInput7A, &IID_IDirectInput7W,
                                               &IID_IDirectInputDeviceA, &IID_IDirectInputDeviceW,
                                               &IID_IDirectInputDevice2A, &IID_IDirectInputDevice2W,
                                               &IID_IDirectInputDevice7A, &IID_IDirectInputDevice7W,
                                               &IID_IDirectInputDevice8A, &IID_IDirectInputDevice8W,
                                               &IID_IDirectInputEffect};

    static const REFIID iid_list[] = {&IID_IUnknown, &IID_IDirectInput8A, &IID_IDirectInput8W};

    int i;
    IUnknown *pUnk;
    HRESULT hr;

    for (i = 0; i < sizeof(invalid_param_list)/sizeof(invalid_param_list[0]); i++)
    {
        if (invalid_param_list[i].ppdi) pUnk = (void *)0xdeadbeef;
        hr = DirectInput8Create(invalid_param_list[i].hinst ? hInstance : NULL,
                                invalid_param_list[i].dwVersion,
                                invalid_param_list[i].riid,
                                invalid_param_list[i].ppdi ? (void **)&pUnk : NULL,
                                NULL);
        ok(hr == invalid_param_list[i].expected_hr, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        if (invalid_param_list[i].ppdi)
            ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < sizeof(no_interface_list)/sizeof(no_interface_list[0]); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, no_interface_list[i], (void **)&pUnk, NULL);
        ok(hr == DIERR_NOINTERFACE, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < sizeof(iid_list)/sizeof(iid_list[0]); i++)
    {
        pUnk = NULL;
        hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, iid_list[i], (void **)&pUnk, NULL);
        ok(hr == DI_OK, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk)
            IUnknown_Release(pUnk);
    }
}

START_TEST(dinput)
{
    hInstance = GetModuleHandleA(NULL);

    CoInitialize(NULL);
    test_DirectInput8Create();
    CoUninitialize();
}
