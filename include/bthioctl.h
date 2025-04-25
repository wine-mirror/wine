/*
 * IOCTL definitions for interfacing with winebth.sys
 *
 * Copyright 2024 Vibhav Pant
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

#ifndef __BTHIOCTL_H__
#define __BTHIOCTL_H__

#define IOCTL_BTH_GET_LOCAL_INFO    CTL_CODE(FILE_DEVICE_BLUETOOTH, 0x00, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BTH_GET_DEVICE_INFO   CTL_CODE(FILE_DEVICE_BLUETOOTH, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BTH_DISCONNECT_DEVICE CTL_CODE(FILE_DEVICE_BLUETOOTH, 0x03, METHOD_BUFFERED, FILE_ANY_ACCESS)

#include <pshpack1.h>

typedef struct _BTH_RADIO_INFO
{
    ULONGLONG lmpSupportedFeatures;
    USHORT mfg;
    USHORT lmpSubversion;
    UCHAR lmpVersion;
} BTH_RADIO_INFO, *PBTH_RADIO_INFO;

typedef struct _BTH_LOCAL_RADIO_INFO
{
    BTH_DEVICE_INFO localInfo;
    ULONG flags;
    USHORT hciRevision;
    UCHAR hciVersion;
    BTH_RADIO_INFO radioInfo;
} BTH_LOCAL_RADIO_INFO, *PBTH_LOCAL_RADIO_INFO;

typedef struct _BTH_DEVICE_INFO_LIST
{
    ULONG numOfDevices;
    BTH_DEVICE_INFO deviceList[1];
} BTH_DEVICE_INFO_LIST, *PBTH_DEVICE_INFO_LIST;

#include <poppack.h>

#endif /* __BTHIOCTL_H__ */
