/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef _CFGMGR32_H_
#define _CFGMGR32_H_

typedef DWORD CONFIGRET;

#define CR_SUCCESS 0

#define MAX_CLASS_NAME_LEN  32
#define MAX_GUID_STRING_LEN 39
#define MAX_PROFILE_LEN     80


#ifdef __cplusplus
extern "C" {
#endif

CONFIGRET WINAPI CM_Get_Device_ID_ListA( PCSTR, PCHAR, ULONG, ULONG );
CONFIGRET WINAPI CM_Get_Device_ID_ListW( PCWSTR, PWCHAR, ULONG, ULONG );
#define     CM_Get_Device_ID_List WINELIB_NAME_AW(CM_Get_Device_ID_List)

#ifdef __cplusplus
}
#endif

#endif /* _CFGMGR32_H_ */
