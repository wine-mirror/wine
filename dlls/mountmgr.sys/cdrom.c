/*
 * CD-ROM ioctls implementation
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999, 2001, 2003 Eric Pouech
 * Copyright 2000 Andreas Mohr
 * Copyright 2005 Ivan Leo Puoti
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

#include "config.h"
#include "mountmgr.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cdrom);

NTSTATUS cdrom_ioctl( void *args )
{
    struct cdrom_ioctl_params *params = args;
    unsigned int code = params->code;

    TRACE( "ioctl %#x\n", code );

    switch (code)
    {
        case IOCTL_CDROM_READ_TOC:
            return STATUS_INVALID_DEVICE_REQUEST;

        default:
            FIXME("Unsupported ioctl %#x (device=%#x access=%#x func=%#x method=%#x)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
            return STATUS_NOT_SUPPORTED;
    }
}
