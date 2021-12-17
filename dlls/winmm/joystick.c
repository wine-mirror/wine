/*
 * joystick functions
 *
 * Copyright 1997 Andreas Mohr
 * Copyright 2000 Wolfgang Schwotzer
 * Copyright 2000 Eric Pouech
 * Copyright 2020 Zebediah Figura for CodeWeavers
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"

#include "initguid.h"
#include "dinput.h"

#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

static CRITICAL_SECTION joystick_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &joystick_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": joystick_cs") }
};
static CRITICAL_SECTION joystick_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

struct joystick_state
{
    LONG x;
    LONG y;
    LONG z;
    LONG u;
    LONG v;
    LONG r;
    LONG pov;
    BYTE buttons[32];
};

static const DIOBJECTDATAFORMAT object_formats[] =
{
    { &GUID_XAxis, offsetof(struct joystick_state, x), DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION },
    { &GUID_YAxis, offsetof(struct joystick_state, y), DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION },
    { &GUID_ZAxis, offsetof(struct joystick_state, z), DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION },
    { &GUID_RzAxis, offsetof(struct joystick_state, r), DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION },
    { &GUID_Slider, offsetof(struct joystick_state, u), DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION },
    { &GUID_RxAxis, offsetof(struct joystick_state, v), DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION },
    { &GUID_POV, offsetof(struct joystick_state, pov), DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[0]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[1]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[2]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[3]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[4]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[5]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[6]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[7]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[8]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[9]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[10]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[11]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[12]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[13]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[14]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[15]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[16]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[17]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[18]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[19]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[20]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[21]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[22]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[23]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[24]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[25]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[26]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[27]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[28]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[29]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[30]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
    { NULL, offsetof(struct joystick_state, buttons[31]), DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE, 0 },
};

static const DIDATAFORMAT data_format =
{
    .dwSize = sizeof(DIDATAFORMAT),
    .dwObjSize = sizeof(DIOBJECTDATAFORMAT),
    .dwFlags = DIDF_ABSAXIS,
    .dwDataSize = sizeof(struct joystick_state),
    .dwNumObjs = ARRAY_SIZE(object_formats),
    .rgodf = (DIOBJECTDATAFORMAT *)object_formats,
};

#define JOY_PERIOD_MIN	(10)	/* min Capture time period */
#define JOY_PERIOD_MAX	(1000)	/* max Capture time period */

struct joystick
{
    DIDEVICEINSTANCEW instance;
    IDirectInputDevice8W *device;
    struct joystick_state state;
    HANDLE event;

    JOYINFO info;
    HWND capture;
    UINT timer;
    DWORD threshold;
    BOOL changed;
    ULONG last_check;
};

static DIDEVICEINSTANCEW instances[16];
static struct joystick joysticks[16];
static IDirectInput8W *dinput;

static BOOL CALLBACK enum_instances( const DIDEVICEINSTANCEW *instance, void *context )
{
    ULONG index = *(ULONG *)context;
    BYTE type = instance->dwDevType;

    if (type == DI8DEVTYPE_MOUSE) return DIENUM_CONTINUE;
    if (type == DI8DEVTYPE_KEYBOARD) return DIENUM_CONTINUE;

    instances[index++] = *instance;
    if (index >= ARRAY_SIZE(instances)) return DIENUM_STOP;
    *(ULONG *)context = index;
    return DIENUM_CONTINUE;
}

static BOOL WINAPI joystick_load_once( INIT_ONCE *once, void *param, void **context )
{
    HRESULT hr = DirectInput8Create( hWinMM32Instance, DIRECTINPUT_VERSION, &IID_IDirectInput8W,
                                     (void **)&dinput, NULL );
    if (FAILED(hr)) ERR( "Could not create dinput instance, hr %#x\n", hr );
    return TRUE;
}

void joystick_unload()
{
    int i;

    if (!dinput) return;

    for (i = 0; i < ARRAY_SIZE(joysticks); i++)
    {
        if (!joysticks[i].device) continue;
        IDirectInputDevice8_Release( joysticks[i].device );
        CloseHandle( joysticks[i].event );
    }

    IDirectInput8_Release( dinput );
}

static void find_joysticks(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

    IDirectInputDevice8W *device;
    HANDLE event;
    DWORD index;
    HRESULT hr;

    InitOnceExecuteOnce( &init_once, joystick_load_once, NULL, NULL );

    if (!dinput) return;

    index = 0;
    IDirectInput8_EnumDevices( dinput, DI8DEVCLASS_ALL, enum_instances, &index, DIEDFL_ATTACHEDONLY );
    TRACE( "found %u device instances\n", index );

    while (index--)
    {
        if (!memcmp( &joysticks[index].instance, &instances[index], sizeof(DIDEVICEINSTANCEW) ))
            continue;

        if (joysticks[index].device)
        {
            IDirectInputDevice8_Release( joysticks[index].device );
            CloseHandle( joysticks[index].event );
        }

        if (!(event = CreateEventW( NULL, FALSE, FALSE, NULL )))
            WARN( "could not event for device, error %u\n", GetLastError() );
        else if (FAILED(hr = IDirectInput8_CreateDevice( dinput, &instances[index].guidInstance, &device, NULL )))
            WARN( "could not create device %s instance, hr %#x\n",
                  debugstr_guid( &instances[index].guidInstance ), hr );
        else if (FAILED(hr = IDirectInputDevice8_SetEventNotification( device, event )))
            WARN( "SetEventNotification device %p hr %#x\n", device, hr );
        else if (FAILED(hr = IDirectInputDevice8_SetCooperativeLevel( device, NULL, DISCL_NONEXCLUSIVE|DISCL_BACKGROUND )))
            WARN( "SetCooperativeLevel device %p hr %#x\n", device, hr );
        else if (FAILED(hr = IDirectInputDevice8_SetDataFormat( device, &data_format )))
            WARN( "SetDataFormat device %p hr %#x\n", device, hr );
        else if (FAILED(hr = IDirectInputDevice8_Acquire( device )))
            WARN( "Acquire device %p hr %#x\n", device, hr );
        else
        {
            TRACE( "opened device %p event %p\n", device, event );

            memset( &joysticks[index], 0, sizeof(struct joystick) );
            joysticks[index].instance = instances[index];
            joysticks[index].device = device;
            joysticks[index].event = event;
            continue;
        }

        CloseHandle( event );
        if (device) IDirectInputDevice8_Release( device );
        memmove( joysticks + index, joysticks + index + 1,
                 (ARRAY_SIZE(joysticks) - index - 1) * sizeof(struct joystick) );
        memset( &joysticks[ARRAY_SIZE(joysticks) - 1], 0, sizeof(struct joystick) );
    }
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;
    return diff <= max_diff;
}

static void CALLBACK joystick_timer( HWND hwnd, UINT msg, UINT_PTR timer, DWORD time )
{
    MMRESULT res;
    JOYINFO info;
    WORD change;
    LONG pos;
    int i;

    EnterCriticalSection( &joystick_cs );

    for (i = 0; i < ARRAY_SIZE(joysticks); i++)
    {
        if (joysticks[i].capture != hwnd) continue;
        if ((res = joyGetPos( i, &info )))
        {
            WARN( "joyGetPos failed: %08x\n", res );
            continue;
        }

        pos = MAKELONG( info.wXpos, info.wYpos );

        if (!joysticks[i].changed ||
            !compare_uint( joysticks[i].info.wXpos, info.wXpos, joysticks[i].threshold ) ||
            !compare_uint( joysticks[i].info.wYpos, info.wYpos, joysticks[i].threshold ))
        {
            SendMessageA( hwnd, MM_JOY1MOVE + i, info.wButtons, pos );
            joysticks[i].info.wXpos = info.wXpos;
            joysticks[i].info.wYpos = info.wYpos;
        }
        if (!joysticks[i].changed ||
            !compare_uint( joysticks[i].info.wZpos, info.wZpos, joysticks[i].threshold ))
        {
            SendMessageA( hwnd, MM_JOY1ZMOVE + i, info.wButtons, pos );
            joysticks[i].info.wZpos = info.wZpos;
        }
        if ((change = joysticks[i].info.wButtons ^ info.wButtons) != 0)
        {
            if (info.wButtons & change)
                SendMessageA( hwnd, MM_JOY1BUTTONDOWN + i, (change << 8) | (info.wButtons & change), pos );
            if (joysticks[i].info.wButtons & change)
                SendMessageA( hwnd, MM_JOY1BUTTONUP + i, (change << 8) | (joysticks[i].info.wButtons & change), pos );
            joysticks[i].info.wButtons = info.wButtons;
        }
    }

    LeaveCriticalSection( &joystick_cs );
}

/**************************************************************************
 *                              joyConfigChanged        [WINMM.@]
 */
MMRESULT WINAPI joyConfigChanged(DWORD flags)
{
    FIXME( "flags %#x stub!\n", flags );
    if (flags) return JOYERR_PARMS;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyGetNumDevs		[WINMM.@]
 */
UINT WINAPI DECLSPEC_HOTPATCH joyGetNumDevs(void)
{
    return ARRAY_SIZE(joysticks);
}

/**************************************************************************
 * 				joyGetDevCapsW		[WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetDevCapsW( UINT_PTR id, JOYCAPSW *caps, UINT size )
{
    DIDEVICEOBJECTINSTANCEW instance = {.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW)};
    DIDEVCAPS dicaps = {.dwSize = sizeof(DIDEVCAPS)};
    DIPROPDWORD diprop =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    MMRESULT res = JOYERR_NOERROR;
    IDirectInputDevice8W *device;
    ULONG ticks = GetTickCount();
    HRESULT hr;

    TRACE( "id %d, caps %p, size %u.\n", (int)id, caps, size );

    if (!caps) return MMSYSERR_INVALPARAM;
    if (size != sizeof(JOYCAPSW) && size != sizeof(JOYCAPS2W)) return JOYERR_PARMS;

    memset( caps, 0, size );
    wcscpy( caps->szRegKey, L"DINPUT.DLL" );
    if (id == ~(UINT_PTR)0) return JOYERR_NOERROR;

    if (id >= ARRAY_SIZE(joysticks)) return JOYERR_PARMS;

    EnterCriticalSection( &joystick_cs );

    if (!(device = joysticks[id].device) && (ticks - joysticks[id].last_check) >= 2000)
    {
        joysticks[id].last_check = ticks;
        find_joysticks();
    }

    if (!(device = joysticks[id].device)) res = JOYERR_PARMS;
    else if (FAILED(hr = IDirectInputDevice8_GetCapabilities( device, &dicaps )))
    {
        WARN( "GetCapabilities device %p returned %#x\n", device, hr );
        res = JOYERR_PARMS;
    }
    else
    {
        hr = IDirectInputDevice8_GetProperty( device, DIPROP_VIDPID, &diprop.diph );
        if (FAILED(hr)) WARN( "GetProperty device %p returned %#x\n", device, hr );
        else
        {
            caps->wMid = LOWORD(diprop.dwData);
            caps->wPid = HIWORD(diprop.dwData);
        }

        wcscpy( caps->szPname, L"Wine joystick driver" );
        caps->wXmin = 0;
        caps->wXmax = 0xffff;
        caps->wYmin = 0;
        caps->wYmax = 0xffff;
        caps->wZmin = 0;
        caps->wZmax = 0xffff;
        caps->wNumButtons = dicaps.dwButtons;
        caps->wPeriodMin = JOY_PERIOD_MIN;
        caps->wPeriodMax = JOY_PERIOD_MAX;
        caps->wRmin = 0;
        caps->wRmax = 0xffff;
        caps->wUmin = 0;
        caps->wUmax = 0xffff;
        caps->wVmin = 0;
        caps->wVmax = 0xffff;
        caps->wCaps = 0;
        caps->wMaxAxes = 6;
        caps->wNumAxes = min( dicaps.dwAxes, caps->wMaxAxes );
        caps->wMaxButtons = 32;

        hr = IDirectInputDevice8_GetObjectInfo( device, &instance, offsetof(struct joystick_state, z), DIPH_BYOFFSET );
        if (SUCCEEDED(hr)) caps->wCaps |= JOYCAPS_HASZ;
        hr = IDirectInputDevice8_GetObjectInfo( device, &instance, offsetof(struct joystick_state, r), DIPH_BYOFFSET );
        if (SUCCEEDED(hr)) caps->wCaps |= JOYCAPS_HASR;
        hr = IDirectInputDevice8_GetObjectInfo( device, &instance, offsetof(struct joystick_state, u), DIPH_BYOFFSET );
        if (SUCCEEDED(hr)) caps->wCaps |= JOYCAPS_HASU;
        hr = IDirectInputDevice8_GetObjectInfo( device, &instance, offsetof(struct joystick_state, v), DIPH_BYOFFSET );
        if (SUCCEEDED(hr)) caps->wCaps |= JOYCAPS_HASV;
        hr = IDirectInputDevice8_GetObjectInfo( device, &instance, offsetof(struct joystick_state, pov), DIPH_BYOFFSET );
        if (SUCCEEDED(hr)) caps->wCaps |= JOYCAPS_HASPOV|JOYCAPS_POV4DIR;
    }

    LeaveCriticalSection( &joystick_cs );

    return res;
}

/**************************************************************************
 * 				joyGetDevCapsA		[WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetDevCapsA( UINT_PTR id, JOYCAPSA *caps, UINT size )
{
    UINT size_w = sizeof(JOYCAPS2W);
    JOYCAPS2W caps_w;
    MMRESULT res;

    TRACE( "id %d, caps %p, size %u.\n", (int)id, caps, size );

    if (!caps) return MMSYSERR_INVALPARAM;
    if (size != sizeof(JOYCAPSA) && size != sizeof(JOYCAPS2A)) return JOYERR_PARMS;

    if (size == sizeof(JOYCAPSA)) size_w = sizeof(JOYCAPSW);
    res = joyGetDevCapsW( id, (JOYCAPSW *)&caps_w, size_w );
    if (res) return res;

    caps->wMid = caps_w.wMid;
    caps->wPid = caps_w.wPid;
    WideCharToMultiByte( CP_ACP, 0, caps_w.szPname, -1, caps->szPname,
                         sizeof(caps->szPname), NULL, NULL );
    caps->wXmin = caps_w.wXmin;
    caps->wXmax = caps_w.wXmax;
    caps->wYmin = caps_w.wYmin;
    caps->wYmax = caps_w.wYmax;
    caps->wZmin = caps_w.wZmin;
    caps->wZmax = caps_w.wZmax;
    caps->wNumButtons = caps_w.wNumButtons;
    caps->wPeriodMin = caps_w.wPeriodMin;
    caps->wPeriodMax = caps_w.wPeriodMax;
    caps->wRmin = caps_w.wRmin;
    caps->wRmax = caps_w.wRmax;
    caps->wUmin = caps_w.wUmin;
    caps->wUmax = caps_w.wUmax;
    caps->wVmin = caps_w.wVmin;
    caps->wVmax = caps_w.wVmax;
    caps->wCaps = caps_w.wCaps;
    caps->wMaxAxes = caps_w.wMaxAxes;
    caps->wNumAxes = caps_w.wNumAxes;
    caps->wMaxButtons = caps_w.wMaxButtons;
    WideCharToMultiByte( CP_ACP, 0, caps_w.szRegKey, -1, caps->szRegKey,
                         sizeof(caps->szRegKey), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, caps_w.szOEMVxD, -1, caps->szOEMVxD,
                         sizeof(caps->szOEMVxD), NULL, NULL );

    if (size == sizeof(JOYCAPS2A))
    {
        JOYCAPS2A *caps2_a = (JOYCAPS2A *)caps;
        caps2_a->ManufacturerGuid = caps_w.ManufacturerGuid;
        caps2_a->ProductGuid = caps_w.ProductGuid;
        caps2_a->NameGuid = caps_w.NameGuid;
    }

    return JOYERR_NOERROR;
}

/**************************************************************************
 *                              joyGetPosEx             [WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetPosEx( UINT id, JOYINFOEX *info )
{
    DWORD i, ticks = GetTickCount();
    MMRESULT res = JOYERR_NOERROR;
    IDirectInputDevice8W *device;
    struct joystick_state state;
    HRESULT hr;

    TRACE( "id %u, info %p.\n", id, info );

    if (!info) return MMSYSERR_INVALPARAM;
    if (id >= ARRAY_SIZE(joysticks) || info->dwSize < sizeof(JOYINFOEX)) return JOYERR_PARMS;

    EnterCriticalSection( &joystick_cs );

    if (!(device = joysticks[id].device) && (ticks - joysticks[id].last_check) >= 2000)
    {
        joysticks[id].last_check = ticks;
        find_joysticks();
    }

    if (!(device = joysticks[id].device))
        res = JOYERR_PARMS;
    else if (FAILED(hr = IDirectInputDevice8_GetDeviceState( device, sizeof(struct joystick_state), &state )))
    {
        WARN( "GetDeviceState device %p returned %#x\n", device, hr );
        res = JOYERR_PARMS;
    }
    else
    {
        if (info->dwFlags & JOY_RETURNX) info->dwXpos = state.x;
        if (info->dwFlags & JOY_RETURNY) info->dwYpos = state.y;
        if (info->dwFlags & JOY_RETURNZ) info->dwZpos = state.z;
        if (info->dwFlags & JOY_RETURNR) info->dwRpos = state.r;
        if (info->dwFlags & JOY_RETURNU) info->dwUpos = state.u;
        if (info->dwFlags & JOY_RETURNV) info->dwVpos = state.v;
        if (info->dwFlags & JOY_RETURNPOV)
        {
            if (state.pov == ~0) info->dwPOV = 0xffff;
            else info->dwPOV = state.pov;
        }
        if (info->dwFlags & JOY_RETURNBUTTONS)
        {
            info->dwButtonNumber = info->dwButtons = 0;
            for (i = 0; i < ARRAY_SIZE(state.buttons); ++i)
            {
                if (!state.buttons[i]) continue;
                info->dwButtonNumber++;
                info->dwButtons |= 1 << i;
            }
        }
    }

    LeaveCriticalSection( &joystick_cs );

    return res;
}

/**************************************************************************
 * 				joyGetPos	       	[WINMM.@]
 */
MMRESULT WINAPI joyGetPos( UINT id, JOYINFO *info )
{
    JOYINFOEX infoex =
    {
        .dwSize = sizeof(JOYINFOEX),
        .dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNBUTTONS,
    };
    MMRESULT res;

    TRACE( "id %u, info %p.\n", id, info );

    if (!info) return MMSYSERR_INVALPARAM;
    if ((res = joyGetPosEx( id, &infoex ))) return res;

    info->wXpos = infoex.dwXpos;
    info->wYpos = infoex.dwYpos;
    info->wZpos = infoex.dwZpos;
    info->wButtons = infoex.dwButtons;

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyGetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joyGetThreshold( UINT id, UINT *threshold )
{
    TRACE( "id %u, threshold %p.\n", id, threshold );

    if (id >= ARRAY_SIZE(joysticks)) return JOYERR_PARMS;

    EnterCriticalSection( &joystick_cs );
    *threshold = joysticks[id].threshold;
    LeaveCriticalSection( &joystick_cs );

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyReleaseCapture	[WINMM.@]
 */
MMRESULT WINAPI joyReleaseCapture( UINT id )
{
    TRACE( "id %u.\n", id );

    if (id >= ARRAY_SIZE(joysticks)) return JOYERR_PARMS;

    EnterCriticalSection( &joystick_cs );

    if (!joysticks[id].capture)
        TRACE("Joystick is not captured, ignoring request.\n");
    else
    {
        KillTimer( joysticks[id].capture, joysticks[id].timer );
        joysticks[id].capture = 0;
        joysticks[id].timer = 0;
    }

    LeaveCriticalSection( &joystick_cs );

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joySetCapture		[WINMM.@]
 */
MMRESULT WINAPI joySetCapture( HWND hwnd, UINT id, UINT period, BOOL changed )
{
    MMRESULT res = JOYERR_NOERROR;

    TRACE( "hwnd %p, id %u, period %u, changed %u.\n", hwnd, id, period, changed );

    if (id >= ARRAY_SIZE(joysticks) || hwnd == 0) return JOYERR_PARMS;
    if (period < JOY_PERIOD_MIN) period = JOY_PERIOD_MIN;
    else if (period > JOY_PERIOD_MAX) period = JOY_PERIOD_MAX;

    EnterCriticalSection( &joystick_cs );

    if (joysticks[id].capture || !IsWindow( hwnd ))
        res = JOYERR_NOCANDO; /* FIXME: what should be returned ? */
    else if (joyGetPos( id, &joysticks[id].info ) != JOYERR_NOERROR)
        res = JOYERR_UNPLUGGED;
    else if ((joysticks[id].timer = SetTimer( hwnd, 0, period, joystick_timer )) == 0)
        res = JOYERR_NOCANDO;
    else
    {
        joysticks[id].capture = hwnd;
        joysticks[id].changed = changed;
    }

    LeaveCriticalSection( &joystick_cs );
    return res;
}

/**************************************************************************
 * 				joySetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joySetThreshold( UINT id, UINT threshold )
{
    TRACE( "id %u, threshold %u.\n", id, threshold );

    if (id >= ARRAY_SIZE(joysticks) || threshold > 65535) return MMSYSERR_INVALPARAM;

    EnterCriticalSection( &joystick_cs );
    joysticks[id].threshold = threshold;
    LeaveCriticalSection( &joystick_cs );

    return JOYERR_NOERROR;
}
