/*
 * HID parsing library
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"

#include <ddk/wdm.h>
#include <ddk/hidpddi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hidp);

NTSTATUS WINAPI HidP_GetCollectionDescription( PHIDP_REPORT_DESCRIPTOR report_desc, ULONG report_desc_len,
                                               POOL_TYPE pool_type, HIDP_DEVICE_DESC *device_desc )
{
    FIXME( "report_desc %p, report_desc_len %u, pool_type %u, device_desc %p stub!\n",
           report_desc, report_desc_len, pool_type, device_desc );

    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI HidP_FreeCollectionDescription( HIDP_DEVICE_DESC *device_desc )
{
    FIXME( "device_desc %p stub!\n", device_desc );
}
