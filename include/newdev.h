/*
 * New Device installation API
 *
 * Copyright 2019 Zebediah Figura
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

#ifndef _INC_NEWDEV
#define _INC_NEWDEV

#include "setupapi.h"

#define INSTALLFLAG_FORCE           0x1
#define INSTALLFLAG_READONLY        0x2
#define INSTALLFLAG_NONINTERACTIVE  0x4
#define INSTALLFLAG_BITS            0x7

BOOL WINAPI UpdateDriverForPlugAndPlayDevicesA(HWND parent, const char *hardware_id, const char *inf_path, DWORD flags, BOOL *reboot);
BOOL WINAPI UpdateDriverForPlugAndPlayDevicesW(HWND parent, const WCHAR *hardware_id, const WCHAR *inf_path, DWORD flags, BOOL *reboot);
#define UpdateDriverForPlugAndPlayDevices WINELIB_NAME_AW(UpdateDriverForPlugAndPlayDevices)

#define DIIRFLAG_INF_ALREADY_COPIED 0x01
#define DIIRFLAG_FORCE_INF          0x02
#define DIIRFLAG_HW_USING_THE_INF   0x04
#define DIIRFLAG_HOTPATCH           0x08
#define DIIRFLAG_NOBACKUP           0x10
#define DIIRFLAG_PRE_CONFIGURE_INF  0x20
#define DIIRFLAG_INSTALL_AS_SET     0x40
#define DIIRFLAG_BITS (DIIRFLAG_FORCE_INF | DIIRFLAG_HOTPATCH | DIIRFLAG_PRE_CONFIGURE_INF | DIIRFLAG_INSTALL_AS_SET)
#define DIIRFLAG_SYSTEM_BITS (DIIRFLAG_INF_ALREADY_COPIED | DIIRFLAG_FORCE_INF | DIIRFLAG_HW_USING_THE_INF \
        | DIIRFLAG_HOTPATCH | DIIRFLAG_NOBACKUP | DIIRFLAG_PRE_CONFIGURE_INF | DIIRFLAG_INSTALL_AS_SET)

BOOL WINAPI DiInstallDriverA(HWND parent, const char *inf_path, DWORD flags, BOOL *reboot);
BOOL WINAPI DiInstallDriverW(HWND parent, const WCHAR *inf_path, DWORD flags, BOOL *reboot);
#define DiInstallDriver WINELIB_NAME_AW(DiInstallDriver)

#endif
