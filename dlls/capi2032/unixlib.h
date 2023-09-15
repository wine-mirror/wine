/*
 * capi20 Unix library
 *
 * Copyright 2021 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/unixlib.h"

struct register_params
{
    DWORD MessageBufferSize;
    DWORD maxLogicalConnection;
    DWORD maxBDataBlocks;
    DWORD maxBDataLen;
    DWORD *pApplID;
};

struct release_params
{
    DWORD ApplID;
};

struct put_message_params
{
    DWORD ApplID;
    PVOID pCAPIMessage;
};

struct get_message_params
{
    DWORD ApplID;
    PVOID *ppCAPIMessage;
};

struct waitformessage_params
{
    DWORD ApplID;
};

struct get_manufacturer_params
{
    char *SzBuffer;
};

struct get_version_params
{
    DWORD *pCAPIMajor;
    DWORD *pCAPIMinor;
    DWORD *pManufacturerMajor;
    DWORD *pManufacturerMinor;
};

struct get_serial_number_params
{
    char *SzBuffer;
};

struct get_profile_params
{
    PVOID SzBuffer;
    DWORD CtlrNr;
};

enum capi20_funcs
{
    unix_register,
    unix_release,
    unix_put_message,
    unix_get_message,
    unix_waitformessage,
    unix_get_manufacturer,
    unix_get_version,
    unix_get_serial_number,
    unix_get_profile,
    unix_isinstalled,
    unix_funcs_count
};
