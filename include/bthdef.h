/*
 * Copyright (C) 2024 Vibhav Pant
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

#ifndef __BTHDEF_H__
#define __BTHDEF_H__

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID( GUID_BTHPORT_DEVICE_INTERFACE, 0x850302a, 0xb344, 0x4fda, 0x9b, 0xe9, 0x90, 0x57, 0x6b,
             0x8d, 0x46, 0xf0 );

DEFINE_GUID( GUID_BLUETOOTH_RADIO_INTERFACE, 0x92383b0e, 0xf90e, 0x4ac9, 0x8d, 0x44, 0x8c, 0x2d,
             0x0d, 0x0e, 0xbd, 0xa2 );

#ifdef __cplusplus
}
#endif

#endif /* __BTHDEF_H__ */
