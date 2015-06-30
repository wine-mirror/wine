/*
 * Human Input Devices
 *
 * Copyright (C) 2015 Aric Stewart
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

#include "config.h"

#include <stdarg.h>

#define NONAMELESSUNION
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "hidusage.h"
#include "ddk/hidpi.h"
#include "parse.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hidp);

NTSTATUS WINAPI HidP_GetCaps(PHIDP_PREPARSED_DATA PreparsedData,
    PHIDP_CAPS Capabilities)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;

    TRACE("(%p, %p)\n",PreparsedData, Capabilities);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    *Capabilities = data->caps;

    return HIDP_STATUS_SUCCESS;
}
