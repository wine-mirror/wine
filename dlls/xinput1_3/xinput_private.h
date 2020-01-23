/*
 * The Wine project - Xinput Joystick Library
 * Copyright 2018 Aric Stewart
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

typedef struct _xinput_controller
{
    CRITICAL_SECTION crit;
    XINPUT_CAPABILITIES caps;
    void *platform_private; /* non-NULL when device is valid; validity may be read without holding crit */
    XINPUT_STATE state;
    XINPUT_VIBRATION vibration;
} xinput_controller;

extern CRITICAL_SECTION xinput_crit;
extern xinput_controller controllers[XUSER_MAX_COUNT];

void HID_find_gamepads(xinput_controller *devices) DECLSPEC_HIDDEN;
void HID_destroy_gamepads(xinput_controller *devices) DECLSPEC_HIDDEN;
void HID_update_state(xinput_controller* device, XINPUT_STATE *state) DECLSPEC_HIDDEN;
DWORD HID_set_state(xinput_controller* device, XINPUT_VIBRATION* state) DECLSPEC_HIDDEN;
void HID_enable(xinput_controller* device, BOOL enable) DECLSPEC_HIDDEN;
