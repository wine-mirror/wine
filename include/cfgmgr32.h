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

#include <cfg.h>
#include <devpropdef.h>

#ifndef GUID_DEFINED
# include <guiddef.h>
#endif

/* cfgmgr32 doesn't use the normal convention, it adds an underscore before A/W */
#ifdef WINE_NO_UNICODE_MACROS
# define DECL_WINELIB_CFGMGR32_TYPE_AW(type)  /* nothing */
#else
# define DECL_WINELIB_CFGMGR32_TYPE_AW(type)  typedef WINELIB_NAME_AW(type##_) type;
#endif

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

#define MAX_DEVICE_ID_LEN               200
#define MAX_DEVNODE_ID_LEN              MAX_DEVICE_ID_LEN

#define MAX_CLASS_NAME_LEN              32
#define MAX_GUID_STRING_LEN             39
#define MAX_PROFILE_LEN                 80

#define CM_DEVCAP_LOCKSUPPORTED         0x00000001
#define CM_DEVCAP_EJECTSUPPORTED        0x00000002
#define CM_DEVCAP_REMOVABLE             0x00000004
#define CM_DEVCAP_DOCKDEVICE            0x00000008
#define CM_DEVCAP_UNIQUEID              0x00000010
#define CM_DEVCAP_SILENTINSTALL         0x00000020
#define CM_DEVCAP_RAWDEVICEOK           0x00000040
#define CM_DEVCAP_SURPRISEREMOVALOK     0x00000080
#define CM_DEVCAP_HARDWAREDISABLED      0x00000100
#define CM_DEVCAP_NONDYNAMIC            0x00000200
#define CM_DEVCAP_SECUREDEVICE          0x00000400

#define CM_DRP_DEVICEDESC               0x01
#define CM_DRP_HARDWAREID               0x02
#define CM_DRP_COMPATIBLEIDS            0x03
#define CM_DRP_UNUSED0                  0x04
#define CM_DRP_SERVICE                  0x05
#define CM_DRP_UNUSED1                  0x06
#define CM_DRP_UNUSED2                  0x07
#define CM_DRP_CLASS                    0x08
#define CM_DRP_CLASSGUID                0x09
#define CM_DRP_DRIVER                   0x0A
#define CM_DRP_CONFIGFLAGS              0x0B
#define CM_DRP_MFG                      0x0C
#define CM_DRP_FRIENDLYNAME             0x0D
#define CM_DRP_LOCATION_INFORMATION     0x0E
#define CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME 0x0F
#define CM_DRP_CAPABILITIES             0x10
#define CM_DRP_UI_NUMBER                0x11
#define CM_DRP_UPPERFILTERS             0x12
#define CM_DRP_LOWERFILTERS             0x13
#define CM_DRP_BUSTYPEGUID              0x14
#define CM_DRP_LEGACYBUSTYPE            0x15
#define CM_DRP_BUSNUMBER                0x16
#define CM_DRP_ENUMERATOR_NAME          0x17
#define CM_DRP_SECURITY                 0x18
#define CM_DRP_SECURITY_SDS             0x19
#define CM_DRP_DEVTYPE                  0x1A
#define CM_DRP_EXCLUSIVE                0x1B
#define CM_DRP_CHARACTERISTICS          0x1C
#define CM_DRP_ADDRESS                  0x1D
#define CM_DRP_UI_NUMBER_DESC_FORMAT    0x1E
#define CM_DRP_DEVICE_POWER_DATA        0x1F
#define CM_DRP_REMOVAL_POLICY           0x20
#define CM_DRP_REMOVAL_POLICY_HW_DEFAULT 0x21
#define CM_DRP_REMOVAL_POLICY_OVERRIDE  0x22
#define CM_DRP_INSTALL_STATE            0x23
#define CM_DRP_LOCATION_PATHS           0x24
#define CM_DRP_BASE_CONTAINERID         0x25
#define CM_DRP_MIN                      0x01
#define CM_DRP_MAX                      0x25

#define CM_CRP_UPPERFILTERS             CM_DRP_UPPERFILTERS
#define CM_CRP_LOWERFILTERS             CM_DRP_LOWERFILTERS
#define CM_CRP_SECURITY                 CM_DRP_SECURITY
#define CM_CRP_SECURITY_SDS             CM_DRP_SECURITY_SDS
#define CM_CRP_DEVTYPE                  CM_DRP_DEVTYPE
#define CM_CRP_EXCLUSIVE                CM_DRP_EXCLUSIVE
#define CM_CRP_CHARACTERISTICS          CM_DRP_CHARACTERISTICS
#define CM_CRP_MIN                      CM_DRP_MIN
#define CM_CRP_MAX                      CM_DRP_MAX

#define RegDisposition_OpenAlways       0x00
#define RegDisposition_OpenExisting     0x01

#define CM_REGISTRY_HARDWARE            0x0000
#define CM_REGISTRY_SOFTWARE            0x0001
#define CM_REGISTRY_USER                0x0100
#define CM_REGISTRY_CONFIG              0x0200

#define CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL       1
#define CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL  2
#define CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL 3

typedef DWORD DEVINST, *PDEVINST;
typedef DWORD DEVNODE, *PDEVNODE;
typedef HANDLE HMACHINE, *PHMACHINE;
typedef HANDLE HCMNOTIFICATION, *PHCMNOTIFICATION;
typedef CHAR *DEVNODEID_A, *DEVINSTID_A;
typedef WCHAR *DEVNODEID_W, *DEVINSTID_W;
typedef ULONG REGDISPOSITION;

typedef enum _CM_NOTIFY_FILTER_TYPE
{
    CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE,
    CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE,
    CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE,
    CM_NOTIFY_FILTER_TYPE_MAX
} CM_NOTIFY_FILTER_TYPE, *PCM_NOTIFY_FILTER_TYPE;

typedef enum _CM_NOTIFY_ACTION
{
    CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL,
    CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL,
    CM_NOTIFY_ACTION_DEVICEQUERYREMOVE,
    CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED,
    CM_NOTIFY_ACTION_DEVICEREMOVEPENDING,
    CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE,
    CM_NOTIFY_ACTION_DEVICECUSTOMEVENT,
    CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED,
    CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED,
    CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED,
    CM_NOTIFY_ACTION_MAX
} CM_NOTIFY_ACTION, *PCM_NOTIFY_ACTION;

typedef struct _CM_NOTIFY_FILTER
{
    DWORD cbSize;
    DWORD Flags;
    CM_NOTIFY_FILTER_TYPE FilterType;
    DWORD Reserved;
    union
    {
        struct
        {
            GUID ClassGuid;
        } DeviceInterface;
        struct
        {
            HANDLE hTarget;
        } DeviceHandle;
        struct
        {
            WCHAR InstanceId[MAX_DEVICE_ID_LEN];
        } DeviceInstance;
    } u;
} CM_NOTIFY_FILTER, *PCM_NOTIFY_FILTER;

typedef struct _CM_NOTIFY_EVENT_DATA
{
    CM_NOTIFY_FILTER_TYPE FilterType;
    DWORD Reserved;
    union
    {
        struct
        {
            GUID  ClassGuid;
            WCHAR SymbolicLink[ANYSIZE_ARRAY];
        } DeviceInterface;
        struct
        {
            GUID  EventGuid;
            LONG  NameOffset;
            DWORD DataSize;
            BYTE  Data[ANYSIZE_ARRAY];
        } DeviceHandle;
        struct
        {
            WCHAR InstanceId[ANYSIZE_ARRAY];
        } DeviceInstance;
    } u;
} CM_NOTIFY_EVENT_DATA, *PCM_NOTIFY_EVENT_DATA;

typedef DWORD (WINAPI *PCM_NOTIFY_CALLBACK)(HCMNOTIFICATION,void*,CM_NOTIFY_ACTION,CM_NOTIFY_EVENT_DATA*,DWORD);

DECL_WINELIB_CFGMGR32_TYPE_AW(DEVNODEID)
DECL_WINELIB_CFGMGR32_TYPE_AW(DEVINSTID)

#ifdef __cplusplus
extern "C" {
#endif

CMAPI CONFIGRET WINAPI CM_Connect_MachineA(PCSTR,PHMACHINE);
CMAPI CONFIGRET WINAPI CM_Connect_MachineW(PCWSTR,PHMACHINE);
#define     CM_Connect_Machine WINELIB_NAME_AW(CM_Connect_Machine)
CMAPI CONFIGRET WINAPI CM_Create_DevNodeA(PDEVINST,DEVINSTID_A,DEVINST,ULONG);
CMAPI CONFIGRET WINAPI CM_Create_DevNodeW(PDEVINST,DEVINSTID_W,DEVINST,ULONG);
#define     CM_Create_DevNode WINELIB_NAME_AW(CM_Create_DevNode)
CMAPI CONFIGRET WINAPI CM_Disconnect_Machine(HMACHINE);
CMAPI CONFIGRET WINAPI CM_Get_Child(PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags);
CMAPI CONFIGRET WINAPI CM_Get_Child_Ex(PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine);
CMAPI CONFIGRET WINAPI CM_Get_Device_IDA(DEVINST,PSTR,ULONG,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_Device_IDW(DEVINST,PWSTR,ULONG,ULONG);
#define     CM_Get_Device_ID WINELIB_NAME_AW(CM_Get_Device_ID)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ExA(DEVINST,PSTR,ULONG,ULONG,HMACHINE);
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ExW(DEVINST,PWSTR,ULONG,ULONG,HMACHINE);
#define     CM_Get_Device_ID_Ex WINELIB_NAME_AW(CM_Get_Device_ID_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ListA(PCSTR,PCHAR,ULONG,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ListW(PCWSTR,PWCHAR,ULONG,ULONG);
#define     CM_Get_Device_ID_List WINELIB_NAME_AW(CM_Get_Device_ID_List)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_ExA(PCSTR,PCHAR,ULONG,ULONG,HMACHINE);
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_ExW(PCWSTR,PWCHAR,ULONG,ULONG,HMACHINE);
#define     CM_Get_Device_ID_List_Ex WINELIB_NAME_AW(CM_Get_Device_ID_List_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_Size(PULONG,DEVINST,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_Size_Ex(PULONG,DEVINST,ULONG,HMACHINE);
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_PropertyW(LPCWSTR,const DEVPROPKEY*,DEVPROPTYPE*,PBYTE,PULONG,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Status(PULONG,PULONG,DEVINST,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Status_Ex(PULONG,PULONG,DEVINST,ULONG,HMACHINE);
CMAPI CONFIGRET WINAPI CM_Get_Sibling(PDEVINST,DEVINST,ULONG);
CMAPI CONFIGRET WINAPI CM_Get_Sibling_Ex(PDEVINST pdnDevInst, DEVINST DevInst, ULONG ulFlags, HMACHINE hMachine);
CMAPI WORD      WINAPI CM_Get_Version(void);
CMAPI CONFIGRET WINAPI CM_Locate_DevNodeA(PDEVINST,DEVINSTID_A,ULONG);
CMAPI CONFIGRET WINAPI CM_Locate_DevNodeW(PDEVINST,DEVINSTID_W,ULONG);
#define     CM_Locate_DevNode WINELIB_NAME_AW(CM_Locate_DevNode)
CMAPI DWORD     WINAPI CM_MapCrToWin32Err(CONFIGRET,DWORD);
CMAPI CONFIGRET WINAPI CM_Open_DevNode_Key(DEVINST dnDevInst, REGSAM access, ULONG ulHardwareProfile,
                                           REGDISPOSITION disposition, PHKEY phkDevice, ULONG ulFlags);
CMAPI CONFIGRET WINAPI CM_Register_Notification(PCM_NOTIFY_FILTER,PVOID,PCM_NOTIFY_CALLBACK,PHCMNOTIFICATION);
CMAPI CONFIGRET WINAPI CM_Request_Device_EjectA(DEVINST dev, PPNP_VETO_TYPE type, LPSTR name, ULONG length, ULONG flags);
CMAPI CONFIGRET WINAPI CM_Request_Device_EjectW(DEVINST dev, PPNP_VETO_TYPE type, LPWSTR name, ULONG length, ULONG flags);
#define     CM_Request_Device_Eject WINELIB_NAME_AW(CM_Get_Device_ID_List_Ex)

#ifdef __cplusplus
}
#endif

#undef DECL_WINELIB_CFGMGR32_TYPE_AW

#endif /* _CFGMGR32_H_ */
