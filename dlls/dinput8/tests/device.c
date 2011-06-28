/*
 * Copyright (c) 2011 Lucas Fialho Zawacki
 * Copyright (c) 2006 Vitaliy Margolen
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
#include <windows.h>

#include "wine/test.h"
#include "windef.h"
#include "initguid.h"
#include "dinput.h"

struct enum_data {
    LPDIRECTINPUT8 pDI;
    LPDIACTIONFORMAT lpdiaf;
    LPDIRECTINPUTDEVICE8 keyboard;
    LPDIRECTINPUTDEVICE8 mouse;
    int ndevices;
};

/* Dummy GUID */
static const GUID ACTION_MAPPING_GUID = { 0x1, 0x2, 0x3, { 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb } };

static DIACTION actionMapping[]=
{
  /* axis */
  { 0, 0x01008A01 /* DIAXIS_DRIVINGR_STEER */ , 0, { "Steer" } },
  /* button */
  { 1, 0x01000C01 /* DIBUTTON_DRIVINGR_SHIFTUP */ , 0, { "Upshift" } },
  /* keyboard key */
  { 2, DIKEYBOARD_SPACE , 0, { "Missile" } },
  /* mouse button */
  { 3, DIMOUSE_BUTTON0 , 0, { "Select" } }
};

static BOOL CALLBACK enumeration_callback(
    LPCDIDEVICEINSTANCE lpddi,
    LPDIRECTINPUTDEVICE8 lpdid,
    DWORD dwFlags,
    DWORD dwRemaining,
    LPVOID pvRef)
{
    struct enum_data *data = pvRef;
    if (!data) return DIENUM_CONTINUE;

    data->ndevices++;

    /* collect the mouse and keyboard */
    if (IsEqualGUID(&lpddi->guidInstance, &GUID_SysKeyboard))
    {
        IDirectInputDevice_AddRef(lpdid);
        data->keyboard = lpdid;

        ok (dwFlags & DIEDBS_MAPPEDPRI1, "Keyboard should be mapped as pri1 dwFlags=%08x\n", dwFlags);
    }

    if (IsEqualGUID(&lpddi->guidInstance, &GUID_SysMouse))
    {
        IDirectInputDevice_AddRef(lpdid);
        data->mouse = lpdid;

        ok (dwFlags & DIEDBS_MAPPEDPRI1, "Mouse should be mapped as pri1 dwFlags=%08x\n", dwFlags);
    }

    return DIENUM_CONTINUE;
}


static void test_action_mapping(void)
{
    HRESULT hr;
    HINSTANCE hinst = GetModuleHandle(NULL);
    LPDIRECTINPUT8 pDI = NULL;
    DIACTIONFORMAT af;
    struct enum_data data = {pDI, &af, NULL, NULL, 0};

    hr = CoCreateInstance(&CLSID_DirectInput8, 0, 1, &IID_IDirectInput8A, (LPVOID*)&pDI);
    if (hr == DIERR_OLDDIRECTINPUTVERSION ||
        hr == DIERR_BETADIRECTINPUTVERSION ||
        hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("ActionMapping requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8 Create failed: hr=%08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInput8_Initialize(pDI,hinst, DIRECTINPUT_VERSION);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_BETADIRECTINPUTVERSION)
    {
        win_skip("ActionMapping requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8 Initialize failed: hr=%08x\n", hr);
    if (FAILED(hr)) return;

    memset (&af, 0, sizeof(af));
    af.dwSize = sizeof(af);
    af.dwActionSize = sizeof(DIACTION);
    af.dwDataSize = 4 * sizeof(actionMapping) / sizeof(actionMapping[0]);
    af.dwNumActions = sizeof(actionMapping) / sizeof(actionMapping[0]);
    af.rgoAction = actionMapping;
    af.guidActionMap = ACTION_MAPPING_GUID;
    af.dwGenre = 0x01000000; /* DIVIRTUAL_DRIVING_RACE */

    hr = IDirectInput8_EnumDevicesBySemantics(pDI, 0, &af, enumeration_callback, &data, 0);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed: hr=%08x\n", hr);
    ok (data.ndevices > 0, "EnumDevicesBySemantics did not call the callback hr=%08x\n", hr);
    ok (data.keyboard != NULL, "EnumDevicesBySemantics should enumerate the keyboard\n");
    ok (data.mouse != NULL, "EnumDevicesBySemantics should enumerate the mouse\n");

    /* The call fails with a zeroed GUID */
    memset(&af.guidActionMap, 0, sizeof(GUID));
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, 0, &af, enumeration_callback, 0, 0);
    todo_wine ok(FAILED(hr), "EnumDevicesBySemantics succeeded with invalid GUID hr=%08x\n", hr);
}

START_TEST(device)
{
    CoInitialize(NULL);

    test_action_mapping();

    CoUninitialize();
}
