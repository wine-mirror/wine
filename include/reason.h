/*
 * ExitWindowsEx() reason codes
 *
 * Copyright (C) the Wine project
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

#ifndef __WINE_REASON_H
#define __WINE_REASON_H


#define SHTDN_REASON_FLAG_USER_DEFINED            0x40000000
#define SHTDN_REASON_FLAG_PLANNED                 0x80000000

#define SHTDN_REASON_MAJOR_OTHER                  0x00000000
#define SHTDN_REASON_MAJOR_NONE                   0x00000000
#define SHTDN_REASON_MAJOR_HARDWARE               0x00010000
#define SHTDN_REASON_MAJOR_OPERATINGSYSTEM        0x00020000
#define SHTDN_REASON_MAJOR_SOFTWARE               0x00030000
#define SHTDN_REASON_MAJOR_APPLICATION            0x00040000
#define SHTDN_REASON_MAJOR_SYSTEM                 0x00050000
#define SHTDN_REASON_MAJOR_POWER                  0x00060000
#define SHTDN_REASON_MAJOR_LEGACY_API             0x00070000

#define SHTDN_REASON_MINOR_OTHER                  0x00000000
#define SHTDN_REASON_MINOR_NONE                   0x000000ff

#define SHTDN_REASON_UNKNOWN                      SHTDN_REASON_MINOR_NONE
#define SHTDN_REASON_LEGACY_API                   (SHTDN_REASON_MAJOR_LEGACY_API | SHTDN_REASON_FLAG_PLANNED)

#endif
