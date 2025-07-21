/*
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
 */

#ifndef __DDK_BTHGUID_H__
#define __DDK_BTHGUID_H__

/* DEVPROP_TYPE_STRING */
DEFINE_DEVPROPKEY( DEVPKEY_Bluetooth_DeviceAddress, 0x2bd67d8b,0x8beb,0x48d5,0x87,0xe0,0x6c,0xda,0x34,0x28,0x04,0x0a,1 );
/* DEVPROP_TYPE_UINT32 */
DEFINE_DEVPROPKEY( DEVPKEY_Bluetooth_ClassOfDevice, 0x2bd67d8b,0x8beb,0x48d5,0x87,0xe0,0x6c,0xda,0x34,0x28,0x04,0x0a,10 );
/* DEVPROP_TYPE_FILETIME */
DEFINE_DEVPROPKEY( DEVPKEY_Bluetooth_LastConnectedTime, 0x2bd67d8b,0x8beb,0x48d5,0x87,0xe0,0x6c,0xda,0x34,0x28,0x04,0x0a,11 );

#endif
