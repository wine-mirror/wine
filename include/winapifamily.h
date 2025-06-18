/*
 * Copyright 2020 Jacek Caban for CodeWeavers
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

#ifndef _INC_WINAPIFAMILY
#define _INC_WINAPIFAMILY

#define WINAPI_FAMILY_PC_APP         2
#define WINAPI_FAMILY_PHONE_APP      3
#define WINAPI_FAMILY_SYSTEM         4
#define WINAPI_FAMILY_SERVER         5
#define WINAPI_FAMILY_DESKTOP_APP  100

#define WINAPI_FAMILY_APP WINAPI_FAMILY_PC_APP

#ifndef WINAPI_FAMILY
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#endif

#ifndef WINAPI_PARTITION_DESKTOP
#define WINAPI_PARTITION_DESKTOP   (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif

#ifndef WINAPI_PARTITION_APP
#define WINAPI_PARTITION_APP       (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP || \
                                    WINAPI_FAMILY == WINAPI_FAMILY_PC_APP || \
                                    WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#endif

#ifndef WINAPI_PARTITION_PC_APP
#define WINAPI_PARTITION_PC_APP    (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP || \
                                    WINAPI_FAMILY == WINAPI_FAMILY_PC_APP)
#endif

#ifndef WINAPI_PARTITION_PHONE_APP
#define WINAPI_PARTITION_PHONE_APP (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#endif

#ifndef WINAPI_PARTITION_SYSTEM
#define WINAPI_PARTITION_SYSTEM    (WINAPI_FAMILY == WINAPI_FAMILY_SYSTEM || \
                                    WINAPI_FAMILY == WINAPI_FAMILY_SERVER)
#endif

#define WINAPI_PARTITION_PHONE  WINAPI_PARTITION_PHONE_APP

#define WINAPI_FAMILY_PARTITION(x) x

#endif /* _INC_WINAPIFAMILY */
