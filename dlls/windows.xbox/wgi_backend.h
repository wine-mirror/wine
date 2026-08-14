/* WinRT Windows.Xbox.Input implementation
 *
 * Thin bridge to the real Windows.Gaming.Input.Gamepad backend
 * (windows.gaming.input.dll), used to back Windows.Xbox.Input.Gamepad
 * with live controller data instead of stub values.
 *
 * This header intentionally only uses plain C types: it is included both
 * from wgi_backend.c (which also includes windows.gaming.input.h) and from
 * private.h (which also includes our own input.h). Windows.Gaming.Input and
 * Windows.Xbox.Input both declare an IGamepad/IGamepadStatics/GamepadReading/
 * GamepadButtons/etc with the same unprefixed WIDL_using names, so the two
 * generated headers cannot be included together in one translation unit;
 * this struct-based boundary is what keeps them apart.
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

#ifndef __WINE_XBOX_WGI_BACKEND_H
#define __WINE_XBOX_WGI_BACKEND_H

/* mirrors Windows.Gaming.Input.GamepadReading, Buttons uses WGI's
 * GamepadButtons bit values (NOT Windows.Xbox.Input.GamepadButtons ones,
 * the two enums do not share bit values and must be remapped by the
 * caller). */
struct wgi_gamepad_reading
{
    UINT64 timestamp;
    UINT32 buttons;
    float left_trigger;
    float right_trigger;
    float left_thumbstick_x;
    float left_thumbstick_y;
    float right_thumbstick_x;
    float right_thumbstick_y;
};

/* mirrors Windows.Gaming.Input.GamepadVibration */
struct wgi_gamepad_vibration
{
    float left_motor;
    float right_motor;
    float left_trigger;
    float right_trigger;
};

/* Loads windows.gaming.input.dll on demand (via LoadLibraryW +
 * GetProcAddress(DllGetActivationFactory), not RoGetActivationFactory) and
 * returns the AddRef'd Windows.Gaming.Input.IGamepad instances currently
 * known to Windows.Gaming.Input.Gamepad.Gamepads, as opaque handles.
 * The returned array must be freed with free(), each handle released with
 * wgi_backend_release(). */
extern HRESULT wgi_backend_get_gamepads( void ***handles, UINT32 *count );

extern void wgi_backend_addref( void *handle );
extern void wgi_backend_release( void *handle );

extern HRESULT wgi_backend_get_reading( void *handle, struct wgi_gamepad_reading *reading );
extern HRESULT wgi_backend_set_vibration( void *handle, const struct wgi_gamepad_vibration *vibration );

#endif /* __WINE_XBOX_WGI_BACKEND_H */
