/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef _CFGMGR32_H_
#define _CFGMGR32_H_

/* FIXME: #include <cfg.h> */

#define CMAPI
typedef DWORD CONFIGRET;

#define CR_SUCCESS                      0x00
#define CR_DEFAULT                      0x01
#define CR_OUT_OF_MEMORY                0x02
#define CR_INVALID_POINTER              0x03
#define CR_INVALID_FLAG                 0x04
#define CR_INVALID_DEVNODE              0x05
#define CR_INVALID_DEVINST              CR_INVALID_DEVNODE
#define CR_INVALID_RES_DES              0x06
#define CR_INVALID_LOG_CONF             0x07
#define CR_INVALID_ARBITRATOR           0x08
#define CR_INVALID_NODELIST             0x09
#define CR_DEVNODE_HAS_REQS             0x0a
#define CR_DEVINST_HAS_REQS             CR_DEVNODE_HAS_REQS
#define CR_INVALID_RESOURCEID           0x0b
#define CR_DLVXD_NOT_FOUND              0x0c
#define CR_NO_SUCH_DEVNODE              0x0d
#define CR_NO_SUCH_DEVINST              CR_NO_SUCH_DEVNODE
#define CR_NO_MORE_LOG_CONF             0x0e
#define CR_NO_MORE_RES_DES              0x0f
#define CR_ALREADY_SUCH_DEVNODE         0x10
#define CR_ALREADY_SUCH_DEVINST         CR_ALREADY_SUCH_DEVNODE
#define CR_INVALID_RANGE_LIST           0x11
#define CR_INVALID_RANGE                0x12
#define CR_FAILURE                      0x13
#define CR_NO_SUCH_LOGICAL_DEV          0x14
#define CR_CREATE_BLOCKED               0x15
#define CR_NOT_SYSTEM_VM                0x16
#define CR_REMOVE_VETOED                0x17
#define CR_APM_VETOED                   0x18
#define CR_INVALID_LOAD_TYPE            0x19
#define CR_BUFFER_SMALL                 0x1a
#define CR_NO_ARBITRATOR                0x1b
#define CR_NO_REGISTRY_HANDLE           0x1c
#define CR_REGISTRY_ERROR               0x1d
#define CR_INVALID_DEVICE_ID            0x1e
#define CR_INVALID_DATA                 0x1f
#define CR_INVALID_API                  0x20
#define CR_DEVLOADER_NOT_READY          0x21
#define CR_NEED_RESTART                 0x22
#define CR_NO_MORE_HW_PROFILES          0x23
#define CR_DEVICE_NOT_THERE             0x24
#define CR_NO_SUCH_VALUE                0x25
#define CR_WRONG_TYPE                   0x26
#define CR_INVALID_PRIORITY             0x27
#define CR_NOT_DISABLEABLE              0x28
#define CR_FREE_RESOURCES               0x29
#define CR_QUERY_VETOED                 0x2a
#define CR_CANT_SHARE_IRQ               0x2b
#define CR_NO_DEPENDENT                 0x2c
#define CR_SAME_RESOURCES               0x2d
#define CR_NO_SUCH_REGISTRY_KEY         0x2e
#define CR_INVALID_MACHINENAME          0x2f
#define CR_REMOTE_COMM_FAILURE          0x30
#define CR_MACHINE_UNAVAILABLE          0x31
#define CR_NO_CM_SERVICES               0x32
#define CR_ACCESS_DENIED                0x33
#define CR_CALL_NOT_IMPLEMENTED         0x34
#define CR_INVALID_PROPERTY             0x35
#define CR_DEVICE_INTERFACE_ACTIVE      0x36
#define CR_NO_SUCH_DEVICE_INTERFACE     0x37
#define CR_INVALID_REFERENCE_STRING     0x38
#define CR_INVALID_CONFLICT_LIST        0x39
#define CR_INVALID_INDEX                0x3a
#define CR_INVALID_STRUCTURE_SIZE       0x3b
#define NUM_CR_RESULTS                  0x3c

#define MAX_CLASS_NAME_LEN              32
#define MAX_GUID_STRING_LEN             39
#define MAX_PROFILE_LEN                 80

typedef DWORD DEVINST, *PDEVINST;
typedef DWORD DEVNODE, *PDEVNODE;
typedef HANDLE HMACHINE, *PHMACHINE;

#ifdef __cplusplus
extern "C" {
#endif

CMAPI CONFIGRET WINAPI CM_Connect_MachineA(PCSTR,PHMACHINE);
CMAPI CONFIGRET WINAPI CM_Connect_MachineW(PCWSTR,PHMACHINE);
#define     CM_Connect_Machine WINELIB_NAME_AW(CM_Connect_Machine)
CMAPI CONFIGRET WINAPI CM_Disconnect_Machine(HMACHINE);
CMAPI CONFIGRET WINAPI CM_Get_Device_IDA(DEVINST,PSTR,ULONG,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_Device_IDW(DEVINST,PWSTR,ULONG,ULONG);
#define     CM_Get_Device_ID WINELIB_NAME_AW(CM_Get_Device_ID)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ListA(PCSTR,PCHAR,ULONG,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ListW(PCWSTR,PWCHAR,ULONG,ULONG);
#define     CM_Get_Device_ID_List WINELIB_NAME_AW(CM_Get_Device_ID_List)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_ExA(PCSTR,PCHAR,ULONG,ULONG,HMACHINE);
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_ExW(PCWSTR,PWCHAR,ULONG,ULONG,HMACHINE);
#define     CM_Get_Device_ID_List_Ex WINELIB_NAME_AW(CM_Get_Device_ID_List_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_Size(PULONG,DEVINST,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_Size_Ex(PULONG,DEVINST,ULONG,HMACHINE);

#ifdef __cplusplus
}
#endif

#endif /* _CFGMGR32_H_ */
