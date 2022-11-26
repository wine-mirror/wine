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
#define IDD_TEST            1001
#define IDD_FORCEFEEDBACK   1002

#define IDC_JOYSTICKLIST    2000
#define IDC_DISABLEDLIST    2001
#define IDC_XINPUTLIST      2002
#define IDC_BUTTONDISABLE   2010
#define IDC_BUTTONENABLE    2011
#define IDC_BUTTONRESET     2012
#define IDC_BUTTONOVERRIDE  2013

#define IDC_TESTSELECTCOMBO 2100
#define IDC_TESTGROUPXY     2101
#define IDC_TESTGROUPRXRY   2102
#define IDC_TESTGROUPZRZ    2103
#define IDC_TESTGROUPPOV    2104

#define IDC_FFSELECTCOMBO   2200
#define IDC_FFEFFECTLIST    2201

#define ICO_MAIN            100

#endif
