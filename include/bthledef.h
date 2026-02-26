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
#ifndef __BTHLEDEFS_H
#define __BTHLEDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#define BLUETOOTH_GATT_FLAG_NONE                   0
#define BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE 0x0004
#define BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_CACHE  0x0008

typedef struct _BTH_LE_UUID
{
    BOOLEAN IsShortUuid;
    union
    {
        USHORT ShortUuid;
        GUID LongUuid;
    } Value;
} BTH_LE_UUID, *PBTH_LE_UUID;

typedef struct _BTH_LE_GATT_SERVICE
{
    BTH_LE_UUID ServiceUuid;
    USHORT AttributeHandle;
} BTH_LE_GATT_SERVICE, *PBTH_LE_GATT_SERVICE;

typedef struct _BTH_LE_GATT_CHARACTERISTIC
{
    USHORT ServiceHandle;
    BTH_LE_UUID CharacteristicUuid;
    USHORT AttributeHandle;
    USHORT CharacteristicValueHandle;
    BOOLEAN IsBroadcastable;
    BOOLEAN IsReadable;
    BOOLEAN IsWritable;
    BOOLEAN IsWritableWithoutResponse;
    BOOLEAN IsSignedWritable;
    BOOLEAN IsNotifiable;
    BOOLEAN IsIndicatable;
    BOOLEAN HasExtendedProperties;
} BTH_LE_GATT_CHARACTERISTIC, *PBTH_LE_GATT_CHARACTERISTIC;

typedef struct _BTH_LE_GATT_CHARACTERISTIC_VALUE
{
    ULONG DataSize;
    UCHAR Data[1];
} BTH_LE_GATT_CHARACTERISTIC_VALUE, *PBTH_LE_GATT_CHARACTERISTIC_VALUE;

DEFINE_GUID( GUID_BLUETOOTHLE_DEVICE_INTERFACE, 0x781aee18, 0x7733, 0x4ce4, 0xad, 0xd0, 0x91, 0xf4, 0x1c, 0x67, 0xb5, 0x92 );
DEFINE_GUID( GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, 0x6e3bb679, 0x4372, 0x40c8, 0x9e, 0xaa, 0x45, 0x09, 0xdf, 0x26, 0x0c, 0xd8 );
DEFINE_GUID( BTH_LE_ATT_BLUETOOTH_BASE_GUID, 0, 0, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb );

FORCEINLINE BOOLEAN IsBthLEUuidMatch( BTH_LE_UUID u1, BTH_LE_UUID u2 )
{
    GUID full1, full2;

    if (u1.IsShortUuid && u2.IsShortUuid)
        return u1.Value.ShortUuid == u2.Value.ShortUuid;

    if (u1.IsShortUuid)
    {
        full1 = BTH_LE_ATT_BLUETOOTH_BASE_GUID;
        full1.Data1 = u1.Value.ShortUuid;
    }
    else
        full1 = u1.Value.LongUuid;
    if (u2.IsShortUuid)
    {
        full2 = BTH_LE_ATT_BLUETOOTH_BASE_GUID;
        full2.Data1 = u2.Value.ShortUuid;
    }
    else
        full2 = u2.Value.LongUuid;
    return !memcmp( &full1, &full2, sizeof( full1 ) );
}

#ifdef __cplusplus
}
#endif
#endif
