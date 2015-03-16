/*
 * WinMM joystick driver header
 *
 * Copyright 2015 Ken Thomases for CodeWeavers Inc.
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

#include "windef.h"
#include "winbase.h"
#include "mmddk.h"
#include "winuser.h"


LRESULT driver_open(LPSTR str, DWORD index) DECLSPEC_HIDDEN;
LRESULT driver_close(DWORD_PTR device_id) DECLSPEC_HIDDEN;
LRESULT driver_joyGetDevCaps(DWORD_PTR device_id, JOYCAPSW* caps, DWORD size) DECLSPEC_HIDDEN;
LRESULT driver_joyGetPosEx(DWORD_PTR device_id, JOYINFOEX* info) DECLSPEC_HIDDEN;
LRESULT driver_joyGetPos(DWORD_PTR device_id, JOYINFO* info) DECLSPEC_HIDDEN;
