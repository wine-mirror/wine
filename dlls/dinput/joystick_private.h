/*
 * Copyright 2009, Aric Stewart, Codeweavers
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

#ifndef __WINE_DLLS_DINPUT_JOYSTICK_PRIVATE_H
#define __WINE_DLLS_DINPUT_JOYSTICK_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dinput.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "dinput_private.h"
#include "device_private.h"

struct JoystickGenericImpl;

typedef void joy_polldev_handler(struct JoystickGenericImpl *This);

typedef struct JoystickGenericImpl
{
    struct IDirectInputDevice2AImpl base;

    ObjProps    *props;
    DIDEVCAPS   devcaps;
    DIJOYSTATE2 js;     /* wine data */
    GUID        guidProduct;
    char        *name;

    joy_polldev_handler *joy_polldev;
} JoystickGenericImpl;


HRESULT WINAPI JoystickWGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
        LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow);

HRESULT WINAPI JoystickAGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow);

HRESULT WINAPI JoystickAGenericImpl_GetProperty( LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid, LPDIPROPHEADER pdiph);

HRESULT WINAPI JoystickAGenericImpl_GetCapabilities( LPDIRECTINPUTDEVICE8A iface,
    LPDIDEVCAPS lpDIDevCaps);

void _dump_DIDEVCAPS(const DIDEVCAPS *lpDIDevCaps);

HRESULT WINAPI JoystickAGenericImpl_SetProperty( LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid, LPCDIPROPHEADER ph);

HRESULT WINAPI JoystickAGenericImpl_GetDeviceInfo( LPDIRECTINPUTDEVICE8A iface,
    LPDIDEVICEINSTANCEA pdidi);

HRESULT WINAPI JoystickWGenericImpl_GetDeviceInfo( LPDIRECTINPUTDEVICE8W iface,
    LPDIDEVICEINSTANCEW pdidi);

HRESULT WINAPI JoystickAGenericImpl_Poll(LPDIRECTINPUTDEVICE8A iface);

HRESULT WINAPI JoystickAGenericImpl_GetDeviceState( LPDIRECTINPUTDEVICE8A iface,
    DWORD len, LPVOID ptr);

#endif /* __WINE_DLLS_DINPUT_JOYSTICK_PRIVATE_H */
