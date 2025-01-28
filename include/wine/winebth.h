/*
 * Wine-specific IOCTL definitions for interfacing with winebth.sys
 *
 * Copyright 2025 Vibhav Pant
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

#ifndef __WINEBTH_H__
#define __WINEBTH_H__

/* Set the discoverability or connectable flag for a local radio. Enabling discoverability will also enable incoming
 * connections, while disabling incoming connections disables discoverability as well. */
#define IOCTL_WINEBTH_RADIO_SET_FLAG CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xa3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#include <pshpack1.h>

#define LOCAL_RADIO_DISCOVERABLE 0x0001
#define LOCAL_RADIO_CONNECTABLE  0x0002

struct winebth_radio_set_flag_params
{
    unsigned int flag: 2;
    unsigned int enable : 1;
};

#include <poppack.h>

#endif /* __WINEBTH_H__ */
