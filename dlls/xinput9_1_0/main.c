/*
 * The Wine project - Xinput Joystick Library
 * Copyright 2025 Brendan Shanks for CodeWeavers
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "winreg.h"
#include "wingdi.h"
#include "winnls.h"
#include "winternl.h"

#include "xinput.h"

#include "wine/debug.h"

#include "initguid.h"
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

WINE_DEFAULT_DEBUG_CHANNEL(xinput);

static INIT_ONCE init_xinput1_4_once = INIT_ONCE_STATIC_INIT;
static HMODULE xinput1_4;
static DWORD (WINAPI *pXInputGetCapabilities)(DWORD,DWORD,XINPUT_CAPABILITIES*);
static DWORD (WINAPI *pXInputGetState)(DWORD, XINPUT_STATE*);
static DWORD (WINAPI *pXInputSetState)(DWORD, XINPUT_VIBRATION*);

static BOOL WINAPI init_xinput1_4_funcs(INIT_ONCE *once, void *param, void **context)
{
    TRACE("xinput9_1_0: Loading functions from xinput1_4.dll\n");

    xinput1_4 = LoadLibraryExW(L"xinput1_4.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (xinput1_4)
    {
        pXInputGetCapabilities = (void*)GetProcAddress(xinput1_4, "XInputGetCapabilities");
        pXInputGetState = (void*)GetProcAddress(xinput1_4, "XInputGetState");
        pXInputSetState = (void*)GetProcAddress(xinput1_4, "XInputSetState");

        if (!pXInputGetCapabilities)
            ERR("Unable to get XInputGetCapabilities from xinput1_4\n");
        if (!pXInputGetState)
            ERR("Unable to get XInputGetState from xinput1_4\n");
        if (!pXInputSetState)
            ERR("Unable to get XInputSetState from xinput1_4\n");
    }
    else
        ERR("Unable to load xinput1_4.dll\n");

    return TRUE;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetCapabilities(DWORD index, DWORD flags, XINPUT_CAPABILITIES *capabilities)
{
    InitOnceExecuteOnce(&init_xinput1_4_once, init_xinput1_4_funcs, NULL, NULL);

    if (!pXInputGetCapabilities) return ERROR_DEVICE_NOT_CONNECTED;
    return pXInputGetCapabilities(index, flags, capabilities);
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetDSoundAudioDeviceGuids(DWORD index, GUID *render_guid, GUID *capture_guid)
{
    XINPUT_STATE state;
    DWORD ret;

    TRACE("index %lu, render_guid %s, capture_guid %s.\n", index, debugstr_guid(render_guid),
          debugstr_guid(capture_guid));

    InitOnceExecuteOnce(&init_xinput1_4_once, init_xinput1_4_funcs, NULL, NULL);

    if (index >= XUSER_MAX_COUNT || !render_guid || !capture_guid) return ERROR_BAD_ARGUMENTS;
    if (!pXInputGetState) return ERROR_DEVICE_NOT_CONNECTED;

    ret = pXInputGetState(index, &state);
    if (ret != ERROR_SUCCESS) return ret;

    /* Docs say this function returns no results on Win8 and later. */
    *render_guid = GUID_NULL;
    *capture_guid = GUID_NULL;

    return ERROR_SUCCESS;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetState(DWORD index, XINPUT_STATE *state)
{
    InitOnceExecuteOnce(&init_xinput1_4_once, init_xinput1_4_funcs, NULL, NULL);

    if (!pXInputGetState) return ERROR_DEVICE_NOT_CONNECTED;
    return pXInputGetState(index, state);
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputSetState(DWORD index, XINPUT_VIBRATION *vibration)
{
    InitOnceExecuteOnce(&init_xinput1_4_once, init_xinput1_4_funcs, NULL, NULL);

    if (!pXInputSetState) return ERROR_DEVICE_NOT_CONNECTED;
    return pXInputSetState(index, vibration);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    TRACE("inst %p, reason %lu, reserved %p.\n", inst, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        if (xinput1_4) FreeLibrary(xinput1_4);
        break;
    }
    return TRUE;
}
