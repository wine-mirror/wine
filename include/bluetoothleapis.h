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
#ifndef __BLUETOOTHLEAPIS_H
#define __BLUETOOTHLEAPIS_H

#include <bthledef.h>

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI BluetoothGATTGetServices( HANDLE, USHORT, BTH_LE_GATT_SERVICE *, USHORT *, ULONG );
HRESULT WINAPI BluetoothGATTGetCharacteristics( HANDLE, BTH_LE_GATT_SERVICE *, USHORT, BTH_LE_GATT_CHARACTERISTIC *, USHORT *, ULONG );

#ifdef __cplusplus
}
#endif
#endif
