/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#include <stddef.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>

#include <winuser.h>
#include <commctrl.h>

/* strings */
#define IDS_CPL_NAME        1
#define IDS_CPL_INFO        2

/* dialogs */
#define IDD_DESKTOP         1000

/* controls */
#define IDC_STATIC          -1

#define IDC_VIRTUAL_DESKTOP         2000
#define IDC_DISPLAY_SETTINGS_LIST   2001
#define IDC_DISPLAY_SETTINGS_RESET  2002
#define IDC_DISPLAY_SETTINGS_APPLY  2003
#define IDC_EMULATE_MODESET         2004

#define ICO_MAIN            100
