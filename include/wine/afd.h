/*
 * Socket driver ioctls
 *
 * Copyright 2020 Zebediah Figura for CodeWeavers
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

#ifndef __WINE_WINE_AFD_H
#define __WINE_WINE_AFD_H

#include <winioctl.h>

#define IOCTL_AFD_CREATE                    CTL_CODE(FILE_DEVICE_NETWORK, 200, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_AFD_ADDRESS_LIST_CHANGE       CTL_CODE(FILE_DEVICE_NETWORK, 323, METHOD_BUFFERED, 0)

struct afd_create_params
{
    int family, type, protocol;
    unsigned int flags;
};

#endif
