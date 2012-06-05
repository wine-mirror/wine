/*
 * Joystick testing control panel applet resources and definitions
 *
 * Copyright 2012 Lucas Fialho Zawacki
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
 *
 */

#ifndef __WINE_JOYSTICKCPL__
#define __WINE_JOYSTICKCPL__

#include <winuser.h>
#include <windef.h>
#include <commctrl.h>
#include <dinput.h>

extern HMODULE hcpl;

struct Joystick {
    IDirectInputDevice8W *device;
    DIDEVICEINSTANCEW instance;
    int num_buttons;
    int num_axes;
};

struct JoystickData {
    IDirectInput8W *di;
    struct Joystick *joysticks;
    int num_joysticks;
    int cur_joystick;
    int chosen_joystick;
};

#define NUM_PROPERTY_PAGES 3

/* strings */
#define IDS_CPL_NAME        1
#define IDS_CPL_INFO        2

/* dialogs */
#define IDC_STATIC          -1

#define IDD_LIST            1000
#define IDD_TEST            1001
#define IDD_FORCEFEEDBACK   1002

#endif
