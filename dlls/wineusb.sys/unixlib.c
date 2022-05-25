/*
 * libusb backend
 *
 * Copyright 2020 Zebediah Figura
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

#if 0
#pragma makedep unix
#endif

#include <libusb.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "wine/list.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineusb);

static NTSTATUS usb_cancel_transfer(void *args)
{
    const struct usb_cancel_transfer_params *params = args;
    int ret;

    if ((ret = libusb_cancel_transfer(params->transfer)) < 0)
        ERR("Failed to cancel transfer: %s\n", libusb_strerror(ret));

    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
#define X(name) [unix_ ## name] = name
    X(usb_cancel_transfer),
};
