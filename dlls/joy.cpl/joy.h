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

struct Effect {
    IDirectInputEffect *effect;
    DIEFFECT params;
    DIEFFECTINFOW info;
};

struct Joystick {
    IDirectInputDevice8W *device;
    DIDEVICEINSTANCEW instance;
    int num_buttons;
    int num_axes;
    BOOL forcefeedback;
    int num_effects;
    int cur_effect;
    int chosen_effect;
    struct Effect *effects;
};

#define TEST_MAX_BUTTONS    32
#define TEST_MAX_AXES       4

struct Graphics {
    HWND hwnd;
    HWND buttons[TEST_MAX_BUTTONS];
    HWND axes[TEST_MAX_AXES];
    HWND ff_axis;
};

struct JoystickData {
    IDirectInput8W *di;
    struct Joystick *joysticks;
    int num_joysticks;
    int num_ff;
    int cur_joystick;
    int chosen_joystick;
    struct Graphics graphics;
    BOOL stop;
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

#define IDC_JOYSTICKLIST    2000
#define IDC_BUTTONDISABLE   2001
#define IDC_BUTTONENABLE    2002
#define IDC_DISABLEDLIST    2003
#define IDC_TESTSELECTCOMBO 2004
#define IDC_TESTGROUPXY     2005
#define IDC_TESTGROUPRXRY   2006
#define IDC_TESTGROUPZRZ    2007
#define IDC_TESTGROUPPOV    2008

#define IDC_FFSELECTCOMBO   2009
#define IDC_FFEFFECTLIST    2010

#define ICO_MAIN            100

/* constants */
#define TEST_POLL_TIME      100

#define TEST_BUTTON_COL_MAX 8
#define TEST_BUTTON_X       8
#define TEST_BUTTON_Y       122
#define TEST_NEXT_BUTTON_X  30
#define TEST_NEXT_BUTTON_Y  25
#define TEST_BUTTON_SIZE_X  20
#define TEST_BUTTON_SIZE_Y  18

#define TEST_AXIS_X         43
#define TEST_AXIS_Y         60
#define TEST_NEXT_AXIS_X    77
#define TEST_AXIS_SIZE_X    3
#define TEST_AXIS_SIZE_Y    3
#define TEST_AXIS_MIN       -25
#define TEST_AXIS_MAX       25

#define FF_AXIS_X           248
#define FF_AXIS_Y           60
#define FF_AXIS_SIZE_X      3
#define FF_AXIS_SIZE_Y      3

#define FF_PLAY_TIME        2*DI_SECONDS
#define FF_PERIOD_TIME      FF_PLAY_TIME/4

#endif
