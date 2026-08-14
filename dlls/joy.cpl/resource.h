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

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>

/* strings */
#define IDS_CPL_NAME        1
#define IDS_CPL_INFO        2

/* dialogs */
#define IDC_STATIC          -1

#define IDD_LIST            1000
#define IDD_TEST_DI         1001
#define IDD_TEST_XI         1002
#define IDD_TEST_WGI        1003

#define IDC_DI_ENABLED_LIST    2000
#define IDC_XI_ENABLED_LIST    2001
#define IDC_DISABLED_LIST      2002
#define IDC_ADVANCED           2003
#define IDC_DISABLE_HIDRAW     2004
#define IDC_ENABLE_SDL         2005

#define IDC_BUTTON_DI_DISABLE   2010
#define IDC_BUTTON_DI_RESET     2011
#define IDC_BUTTON_XI_OVERRIDE  2013
#define IDC_BUTTON_ENABLE       2014

#define IDC_DI_DEVICES      2100
#define IDC_DI_AXES         2101
#define IDC_DI_POVS         2102
#define IDC_DI_BUTTONS      2103
#define IDC_DI_EFFECTS      2104

#define IDC_XI_USER_0       2200
#define IDC_XI_USER_1       2201
#define IDC_XI_USER_2       2202
#define IDC_XI_USER_3       2203
#define IDC_XI_NO_USER_0    2210
#define IDC_XI_NO_USER_1    2211
#define IDC_XI_NO_USER_2    2212
#define IDC_XI_NO_USER_3    2213
#define IDC_XI_RUMBLE_0     2220
#define IDC_XI_RUMBLE_1     2221
#define IDC_XI_RUMBLE_2     2222
#define IDC_XI_RUMBLE_3     2223

#define IDC_WGI_DEVICES     2300
#define IDC_WGI_INTERFACE   2301
#define IDC_WGI_GAMEPAD     2302
#define IDC_WGI_RUMBLE      2303
#define IDC_WGI_AXES        2304
#define IDC_WGI_POVS        2305
#define IDC_WGI_BUTTONS     2306

#define ICO_MAIN            100

#endif
