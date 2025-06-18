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

DEFINE_GUID( GUID_BLUETOOTH_RADIO_IN_RANGE, 0xea3b5b82, 0x26ee, 0x450e, 0xb0, 0xd8, 0xd2, 0x6f,
             0xe3, 0x0a, 0x38, 0x69 );

DEFINE_GUID( GUID_BLUETOOTH_RADIO_OUT_OF_RANGE, 0xe28867c9, 0xc2aa, 0x4ced, 0xb9, 0x69, 0x45, 0x70,
             0x86, 0x60, 0x37, 0xc4);

typedef ULONG BTH_COD;
typedef ULONGLONG BTH_ADDR, *PBTH_ADDR;

#define BTH_MAX_NAME_SIZE  (248)

#define BDIF_ADDRESS   0x00000001
#define BDIF_COD       0x00000002
#define BDIF_NAME      0x00000004
#define BDIF_PAIRED    0x00000008
#define BDIF_PERSONAL  0x00000010
#define BDIF_CONNECTED 0x00000020

#define BDIF_SSP_SUPPORTED      0x00000100
#define BDIF_SSP_PAIRED         0x00000200
#define BDIF_SSP_MITM_PROTECTED 0x00000200

typedef struct _BTH_DEVICE_INFO
{
    ULONG flags;
    BTH_ADDR address;
    BTH_COD classOfDevice;
    CHAR name[BTH_MAX_NAME_SIZE];
} BTH_DEVICE_INFO, *PBTH_DEVICE_INFO;

/* Buffer for GUID_BLUETOOTH_RADIO_IN_RANGE events. */
typedef struct _BTH_RADIO_IN_RANGE
{
    BTH_DEVICE_INFO deviceInfo;
    ULONG previousDeviceFlags;
} BTH_RADIO_IN_RANGE, *PBTH_RADIO_IN_RANGE;


typedef enum _AUTHENTICATION_REQUIREMENTS {
    MITMProtectionNotRequired = 0x00,
    MITMProtectionRequired = 0x01,
    MITMProtectionNotRequiredBonding = 0x02,
    MITMProtectionRequiredBonding = 0x03,
    MITMProtectionNotRequiredGeneralBonding = 0x04,
    MITMProtectionRequiredGeneralBonding = 0x05,
    MITMProtectionNotDefined = 0xff
} AUTHENTICATION_REQUIREMENTS;

#ifdef __cplusplus
}
#endif

#endif /* __BTHDEF_H__ */
