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

#undef CMAPI /* FIXME */
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

#define CM_GETIDLIST_FILTER_NONE               0x00000000
#define CM_GETIDLIST_FILTER_ENUMERATOR         0x00000001
#define CM_GETIDLIST_FILTER_SERVICE            0x00000002
#define CM_GETIDLIST_FILTER_EJECTRELATIONS     0x00000004
#define CM_GETIDLIST_FILTER_REMOVALRELATIONS   0x00000008
#define CM_GETIDLIST_FILTER_POWERRELATIONS     0x00000010
#define CM_GETIDLIST_FILTER_BUSRELATIONS       0x00000020
#define CM_GETIDLIST_DONOTGENERATE             0x10000040
#define CM_GETIDLIST_FILTER_TRANSPORTRELATIONS 0x00000080
#define CM_GETIDLIST_FILTER_PRESENT            0x00000100
#define CM_GETIDLIST_FILTER_CLASS              0x00000200
#define CM_GETIDLIST_FILTER_BITS               0x100003FF

#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT     0x00000000
#define CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES 0x00000001
#define CM_GET_DEVICE_INTERFACE_LIST_BITS        0x00000001

#define CM_OPEN_CLASS_KEY_INSTALLER        0x00000000
#define CM_OPEN_CLASS_KEY_INTERFACE        0x00000001
#define CM_OPEN_CLASS_KEY_BITS             0x00000001

#define CM_ENUMERATE_CLASSES_INSTALLER     0x00000000
#define CM_ENUMERATE_CLASSES_INTERFACE     0x00000001
#define CM_ENUMERATE_CLASSES_BITS          0x00000001

typedef DWORD DEVINST, *PDEVINST;
typedef DWORD DEVNODE, *PDEVNODE;
typedef HANDLE HMACHINE, *PHMACHINE;
typedef HANDLE HCMNOTIFICATION, *PHCMNOTIFICATION;
typedef CHAR *DEVNODEID_A, *DEVINSTID_A;
typedef WCHAR *DEVNODEID_W, *DEVINSTID_W;
typedef ULONG REGDISPOSITION;
typedef DWORD_PTR LOG_CONF;
typedef ULONG PRIORITY;
typedef DWORD_PTR RANGE_LIST;
typedef DWORD_PTR RANGE_ELEMENT;
typedef DWORD_PTR RES_DES;
typedef ULONG RESOURCEID;
typedef ULONG_PTR CONFLICT_LIST;

#define CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES 0x0001
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

typedef struct
{
   ULONG  HWPI_ulHWProfile;
   char   HWPI_szFriendlyName[MAX_PROFILE_LEN];
   DWORD  HWPI_dwFlags;
} HWPROFILEINFO_A;

typedef struct
{
   ULONG  HWPI_ulHWProfile;
   WCHAR  HWPI_szFriendlyName[MAX_PROFILE_LEN];
   DWORD  HWPI_dwFlags;
} HWPROFILEINFO_W;

typedef struct
{
    ULONG       CD_ulSize;
    ULONG       CD_ulMask;
    DEVINST     CD_dnDevInst;
    RES_DES     CD_rdResDes;
    ULONG       CD_ulFlags;
    char        CD_szDescription[MAX_PATH];
} CONFLICT_DETAILS_A;

typedef struct
{
    ULONG       CD_ulSize;
    ULONG       CD_ulMask;
    DEVINST     CD_dnDevInst;
    RES_DES     CD_rdResDes;
    ULONG       CD_ulFlags;
    WCHAR       CD_szDescription[MAX_PATH];
} CONFLICT_DETAILS_W;

typedef DWORD (CALLBACK *PCM_NOTIFY_CALLBACK)( HCMNOTIFICATION notify, void *context, CM_NOTIFY_ACTION action, CM_NOTIFY_EVENT_DATA *data, DWORD data_size );

DECL_WINELIB_CFGMGR32_TYPE_AW(DEVNODEID)
DECL_WINELIB_CFGMGR32_TYPE_AW(DEVINSTID)

#ifdef __cplusplus
extern "C" {
#endif

CMAPI CONFIGRET WINAPI CM_Add_Empty_Log_Conf( LOG_CONF *conf, DEVINST node, PRIORITY priority, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Add_Empty_Log_Conf_Ex( LOG_CONF *conf, DEVINST node, PRIORITY priority, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Add_ID_ExA( DEVINST node, char *id, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Add_ID_ExW( DEVINST node, WCHAR  *id, ULONG flags, HMACHINE machine );
#define                CM_Add_ID_Ex WINELIB_NAME_AW(CM_Add_ID_Ex)
CMAPI CONFIGRET WINAPI CM_Add_IDA( DEVINST node, char *id, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Add_IDW( DEVINST node, WCHAR  *id, ULONG flags );
#define                CM_Add_ID WINELIB_NAME_AW(CM_Add_ID)
CMAPI CONFIGRET WINAPI CM_Add_Range( DWORDLONG start, DWORDLONG end, RANGE_LIST ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Add_Res_Des( RES_DES *res, LOG_CONF conf, RESOURCEID id, const void *resource, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Add_Res_Des_Ex( RES_DES *res, LOG_CONF conf, RESOURCEID id, const void *resource, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Connect_MachineA( const char *server_name, HMACHINE *machine );
CMAPI CONFIGRET WINAPI CM_Connect_MachineW( const WCHAR *server_name, HMACHINE *machine );
#define                CM_Connect_Machine WINELIB_NAME_AW(CM_Connect_Machine)
CMAPI CONFIGRET WINAPI CM_Create_DevNode_ExA( DEVINST *node, DEVINSTID_A instance_id, DEVINST parent, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Create_DevNode_ExW( DEVINST *node, DEVINSTID_W instance_id, DEVINST parent, ULONG flags, HMACHINE machine );
#define                CM_Create_DevNode_Ex WINELIB_NAME_AW(CM_Create_DevNode_Ex)
CMAPI CONFIGRET WINAPI CM_Create_DevNodeA( DEVINST *node, DEVINSTID_A instance_id, DEVINST parent, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Create_DevNodeW( DEVINST *node, DEVINSTID_W instance_id, DEVINST parent, ULONG flags );
#define                CM_Create_DevNode WINELIB_NAME_AW(CM_Create_DevNode)
CMAPI CONFIGRET WINAPI CM_Create_Range_List( RANGE_LIST *ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Delete_Class_Key( GUID *class_, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Delete_Class_Key_Ex( GUID *class_, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Delete_Device_Interface_Key_ExA( const char *iface, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Delete_Device_Interface_Key_ExW( const WCHAR *iface, ULONG flags, HMACHINE machine );
#define                CM_Delete_Device_Interface_Key_Ex WINELIB_NAME_AW(CM_Delete_Device_Interface_Key_Ex)
CMAPI CONFIGRET WINAPI CM_Delete_Device_Interface_KeyA( const char *iface, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Delete_Device_Interface_KeyW( const WCHAR *iface, ULONG flags );
#define                CM_Delete_Device_Interface_Key WINELIB_NAME_AW(CM_Delete_Device_Interface_Key)
CMAPI CONFIGRET WINAPI CM_Delete_DevNode_Key( DEVNODE node, ULONG profile, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Delete_DevNode_Key_Ex( DEVNODE node, ULONG profile, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Delete_Range( DWORDLONG start, DWORDLONG end, RANGE_LIST ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Detect_Resource_Conflict( DEVINST node, RESOURCEID id, const void *resource, ULONG len, BOOL *detected, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Detect_Resource_Conflict_Ex( DEVINST node, RESOURCEID id, const void *resource, ULONG len, BOOL *detected, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Disable_DevNode( DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Disable_DevNode_Ex( DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Disconnect_Machine( HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Dup_Range_List( RANGE_LIST old_ranges, RANGE_LIST new_ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Enable_DevNode( DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Enable_DevNode_Ex( DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Enumerate_Classes( ULONG index, GUID *class_, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Enumerate_Classes_Ex( ULONG index, GUID *class_, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Enumerate_Enumerators_ExA( ULONG index, char *buffer, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Enumerate_Enumerators_ExW( ULONG index, WCHAR *buffer, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Enumerate_Enumerators_Ex WINELIB_NAME_AW(CM_Enumerate_Enumerators_Ex)
CMAPI CONFIGRET WINAPI CM_Enumerate_EnumeratorsA( ULONG index, char *buffer, ULONG *len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Enumerate_EnumeratorsW( ULONG index, WCHAR *buffer, ULONG *len, ULONG flags );
#define                CM_Enumerate_Enumerators WINELIB_NAME_AW(CM_Enumerate_Enumerators)
CMAPI CONFIGRET WINAPI CM_Find_Range( DWORDLONG *start, DWORDLONG from, ULONG length, DWORDLONG alignment, DWORDLONG end, RANGE_LIST ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_First_Range( RANGE_LIST ranges, DWORDLONG *start, DWORDLONG *end, RANGE_ELEMENT *element, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Free_Log_Conf( LOG_CONF conf, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Free_Log_Conf_Ex( LOG_CONF conf, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Free_Log_Conf_Handle( LOG_CONF conf );
CMAPI CONFIGRET WINAPI CM_Free_Range_List( RANGE_LIST ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Free_Res_Des( RES_DES *previous, RES_DES desc, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Free_Res_Des_Ex( RES_DES *previous, RES_DES desc, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Free_Res_Des_Handle( RES_DES desc );
CMAPI CONFIGRET WINAPI CM_Free_Resource_Conflict_Handle( CONFLICT_LIST conflicts );
CMAPI CONFIGRET WINAPI CM_Get_Child( DEVINST *child, DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Child_Ex( DEVINST *child, DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Class_Key_Name_ExA( GUID *class_, char *name, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Class_Key_Name_ExW( GUID *class_, WCHAR *name, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Get_Class_Key_Name_Ex WINELIB_NAME_AW(CM_Get_Class_Key_Name_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Class_Key_NameA( GUID *class_, char *name, ULONG *len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Class_Key_NameW( GUID *class_, WCHAR *name, ULONG *len, ULONG flags );
#define                CM_Get_Class_Key_Name WINELIB_NAME_AW(CM_Get_Class_Key_Name)
CMAPI CONFIGRET WINAPI CM_Get_Class_Name_ExA( GUID *class_, char *buffer, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Class_Name_ExW( GUID *class_, WCHAR *buffer, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Get_Class_Name_Ex WINELIB_NAME_AW(CM_Get_Class_Name_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Class_NameA( GUID *class_, char *buffer, ULONG *len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Class_NameW( GUID *class_, WCHAR *buffer, ULONG *len, ULONG flags );
#define                CM_Get_Class_Name WINELIB_NAME_AW(CM_Get_Class_Name)
CMAPI CONFIGRET WINAPI CM_Get_Class_Property_ExW( const GUID *class_, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Class_Property_Keys( const GUID *class_, DEVPROPKEY *keys, ULONG *count, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Class_Property_Keys_Ex( const GUID *class_, DEVPROPKEY *keys, ULONG *count, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Class_PropertyW( const GUID *class_, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Class_Registry_PropertyA( GUID *class_, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Class_Registry_PropertyW( GUID *class_, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Get_Class_Registry_Property WINELIB_NAME_AW(CM_Get_Class_Registry_Property)
CMAPI CONFIGRET WINAPI CM_Get_Depth( ULONG *depth, DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Depth_Ex( ULONG *depth, DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ExA( DEVINST node, char *buffer, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ExW( DEVINST node, WCHAR *buffer, ULONG len, ULONG flags, HMACHINE machine );
#define                CM_Get_Device_ID_Ex WINELIB_NAME_AW(CM_Get_Device_ID_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_ExA( const char *filter, char *buffer, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_ExW( const WCHAR *filter, WCHAR *buffer, ULONG len, ULONG flags, HMACHINE machine );
#define                CM_Get_Device_ID_List_Ex WINELIB_NAME_AW(CM_Get_Device_ID_List_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExA( ULONG *len, const char *filter, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExW( ULONG *len, const WCHAR *filter, ULONG flags, HMACHINE machine );
#define                CM_Get_Device_ID_List_Size_Ex WINELIB_NAME_AW(CM_Get_Device_ID_List_Size_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_SizeA( ULONG *len, const char *filter, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_List_SizeW( ULONG *len, const WCHAR *filter, ULONG flags );
#define                CM_Get_Device_ID_List_Size WINELIB_NAME_AW(CM_Get_Device_ID_List_Size)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ListA( const char *filter, char *buffer, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_ListW( const WCHAR *filter, WCHAR *buffer, ULONG len, ULONG flags );
#define                CM_Get_Device_ID_List WINELIB_NAME_AW(CM_Get_Device_ID_List)
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_Size( ULONG *len, DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_ID_Size_Ex( ULONG *len, DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_IDA( DEVINST node, char *buffer, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_IDW( DEVINST node, WCHAR *buffer, ULONG len, ULONG flags );
#define                CM_Get_Device_ID WINELIB_NAME_AW(CM_Get_Device_ID)
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_Alias_ExA( const char *iface, GUID *alias_guid, char *alias_iface, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_Alias_ExW( const WCHAR *iface, GUID *alias_guid, WCHAR *alias_iface, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Get_Device_Interface_Alias_Ex WINELIB_NAME_AW(CM_Get_Device_Interface_Alias_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_AliasA( const char *iface, GUID *alias_guid, char *alias_iface, ULONG *len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_AliasW( const WCHAR *iface, GUID *alias_guid, WCHAR *alias_iface, ULONG *len, ULONG flags );
#define                CM_Get_Device_Interface_Alias WINELIB_NAME_AW(CM_Get_Device_Interface_Alias)
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_List_ExA( GUID *class_, DEVINSTID_A instance_id, char *buffer, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_List_ExW( GUID *class_, DEVINSTID_W instance_id, WCHAR *buffer, ULONG len, ULONG flags, HMACHINE machine );
#define                CM_Get_Device_Interface_List_Ex WINELIB_NAME_AW(CM_Get_Device_Interface_List_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExA( ULONG *len, GUID *class_, DEVINSTID_A instance_id, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExW( ULONG *len, GUID *class_, DEVINSTID_W instance_id, ULONG flags, HMACHINE machine );
#define                CM_Get_Device_Interface_List_Size_Ex WINELIB_NAME_AW(CM_Get_Device_Interface_List_Size_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeA( ULONG *len, GUID *class_, DEVINSTID_A instance_id, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeW( ULONG *len, GUID *class_, DEVINSTID_W instance_id, ULONG flags );
#define                CM_Get_Device_Interface_List_Size WINELIB_NAME_AW(CM_Get_Device_Interface_List_Size)
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_ListA( GUID *class_, DEVINSTID_A instance_id, char *buffer, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_ListW( GUID *class_, DEVINSTID_W instance_id, WCHAR *buffer, ULONG len, ULONG flags );
#define                CM_Get_Device_Interface_List WINELIB_NAME_AW(CM_Get_Device_Interface_List)
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_Property_ExW( const WCHAR *iface, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_Property_Keys_ExW( const WCHAR *iface, DEVPROPKEY *keys, ULONG *count, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_Property_KeysW( const WCHAR *iface, DEVPROPKEY *keys, ULONG *count, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_PropertyW( const WCHAR *iface, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Custom_Property_ExA( DEVINST node, const char *name, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Custom_Property_ExW( DEVINST node, const WCHAR *name, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Get_DevNode_Custom_Property_Ex WINELIB_NAME_AW(CM_Get_DevNode_Custom_Property_Ex)
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Custom_PropertyA( DEVINST node, const char *name, ULONG *type, void *buffer, ULONG *len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Custom_PropertyW( DEVINST node, const WCHAR *name, ULONG *type, void *buffer, ULONG *len, ULONG flags );
#define                CM_Get_DevNode_Custom_Property WINELIB_NAME_AW(CM_Get_DevNode_Custom_Property)
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Property_ExW( DEVINST node, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Property_Keys( DEVINST node, DEVPROPKEY *keys, ULONG *count, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Property_Keys_Ex( DEVINST node, DEVPROPKEY *keys, ULONG *count, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_PropertyW( DEVINST node, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExA( DEVINST node, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExW( DEVINST node, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Get_DevNode_Registry_Property_Ex WINELIB_NAME_AW(CM_Get_DevNode_Registry_Property_Ex)
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyA( DEVINST node, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyW( DEVINST node, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags );
#define                CM_Get_DevNode_Registry_Property WINELIB_NAME_AW(CM_Get_DevNode_Registry_Property)
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Status( ULONG *status, ULONG *number, DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_DevNode_Status_Ex( ULONG *status, ULONG *number, DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_First_Log_Conf( LOG_CONF *conf, DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_First_Log_Conf_Ex( LOG_CONF *conf, DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Global_State( ULONG *state, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Global_State_Ex( ULONG *state, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Hardware_Profile_Info_ExA( ULONG index, HWPROFILEINFO_A *info, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Hardware_Profile_Info_ExW( ULONG index, HWPROFILEINFO_W *info, ULONG flags, HMACHINE machine );
#define                CM_Get_Hardware_Profile_Info_Ex WINELIB_NAME_AW(CM_Get_Hardware_Profile_Info_Ex)
CMAPI CONFIGRET WINAPI CM_Get_Hardware_Profile_InfoA( ULONG index, HWPROFILEINFO_A *info, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Hardware_Profile_InfoW( ULONG index, HWPROFILEINFO_W *info, ULONG flags );
#define                CM_Get_Hardware_Profile_Info WINELIB_NAME_AW(CM_Get_Hardware_Profile_Info)
CMAPI CONFIGRET WINAPI CM_Get_HW_Prof_Flags_ExA( DEVINSTID_A instance_id, ULONG profile, ULONG *value, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_HW_Prof_Flags_ExW( DEVINSTID_W instance_id, ULONG profile, ULONG *value, ULONG flags, HMACHINE machine );
#define                CM_Get_HW_Prof_Flags_Ex WINELIB_NAME_AW(CM_Get_HW_Prof_Flags_Ex)
CMAPI CONFIGRET WINAPI CM_Get_HW_Prof_FlagsA( DEVINSTID_A instance_id, ULONG profile, ULONG *value, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_HW_Prof_FlagsW( DEVINSTID_W instance_id, ULONG profile, ULONG *value, ULONG flags );
#define                CM_Get_HW_Prof_Flags WINELIB_NAME_AW(CM_Get_HW_Prof_Flags)
CMAPI CONFIGRET WINAPI CM_Get_Log_Conf_Priority( LOG_CONF conf, PRIORITY *priority, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Log_Conf_Priority_Ex( LOG_CONF conf, PRIORITY *priority, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Next_Log_Conf( LOG_CONF *next, LOG_CONF conf, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Next_Log_Conf_Ex( LOG_CONF *next, LOG_CONF conf, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Next_Res_Des( RES_DES *next, RES_DES desc, RESOURCEID resource, RESOURCEID *next_resource, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Next_Res_Des_Ex( RES_DES *next, RES_DES desc, RESOURCEID resource, RESOURCEID *next_resource, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Parent( DEVINST *parent, DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Parent_Ex( DEVINST *parent, DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Res_Des_Data( RES_DES desc, void *buffer, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Res_Des_Data_Ex( RES_DES desc, void *buffer, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Res_Des_Data_Size( ULONG *size, RES_DES desc, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Res_Des_Data_Size_Ex( ULONG *size, RES_DES desc, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Get_Resource_Conflict_Count( CONFLICT_LIST conflicts, ULONG *count );
CMAPI CONFIGRET WINAPI CM_Get_Resource_Conflict_DetailsA( CONFLICT_LIST conflicts, ULONG index, CONFLICT_DETAILS_A *details );
CMAPI CONFIGRET WINAPI CM_Get_Resource_Conflict_DetailsW( CONFLICT_LIST conflicts, ULONG index, CONFLICT_DETAILS_W *details );
#define                CM_Get_Resource_Conflict_Details WINELIB_NAME_AW(CM_Get_Resource_Conflict_Details)
CMAPI CONFIGRET WINAPI CM_Get_Sibling( DEVINST *sibling, DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Get_Sibling_Ex( DEVINST *sibling, DEVINST node, ULONG flags, HMACHINE machine );
CMAPI WORD      WINAPI CM_Get_Version(void);
CMAPI WORD      WINAPI CM_Get_Version_Ex( HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Intersect_Range_List( RANGE_LIST old_ranges_1, RANGE_LIST old_ranges_2, RANGE_LIST new_ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Invert_Range_List( RANGE_LIST old_ranges, RANGE_LIST new_ranges, DWORDLONG max_value, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Is_Dock_Station_Present( BOOL *present );
CMAPI CONFIGRET WINAPI CM_Is_Dock_Station_Present_Ex( BOOL *present, HMACHINE machine );
CMAPI BOOL      WINAPI CM_Is_Version_Available( WORD version );
CMAPI BOOL      WINAPI CM_Is_Version_Available_Ex( WORD version, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Locate_DevNode_ExA( DEVINST *node, DEVINSTID_A instance_id, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Locate_DevNode_ExW( DEVINST *node, DEVINSTID_W instance_id, ULONG flags, HMACHINE machine );
#define                CM_Locate_DevNode_Ex WINELIB_NAME_AW(CM_Locate_DevNode_Ex)
CMAPI CONFIGRET WINAPI CM_Locate_DevNodeA( DEVINST *node, DEVINSTID_A instance_id, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Locate_DevNodeW( DEVINST *node, DEVINSTID_W instance_id, ULONG flags );
#define                CM_Locate_DevNode WINELIB_NAME_AW(CM_Locate_DevNode)
CMAPI DWORD     WINAPI CM_MapCrToWin32Err( CONFIGRET ret, DWORD default_err );
CMAPI CONFIGRET WINAPI CM_Merge_Range_List( RANGE_LIST old_ranges_1, RANGE_LIST old_ranges_2, RANGE_LIST new_ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Modify_Res_Des( RES_DES *new_desc, RES_DES old_desc, RESOURCEID id, const void *resource, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Modify_Res_Des_Ex( RES_DES *new_desc, RES_DES old_desc, RESOURCEID id, const void *resource, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Move_DevNode( DEVINST node_src, DEVINST node_dst, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Move_DevNode_Ex( DEVINST node_src, DEVINST node_dst, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Next_Range( RANGE_ELEMENT *element, DWORDLONG *start, DWORDLONG *end, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Open_Class_Key_ExA( GUID *class_, const char *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Open_Class_Key_ExW( GUID *class_, const WCHAR *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine );
#define                CM_Open_Class_Key_Ex WINELIB_NAME_AW(CM_Open_Class_Key_Ex)
CMAPI CONFIGRET WINAPI CM_Open_Class_KeyA( GUID *class_, const char *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Open_Class_KeyW( GUID *class_, const WCHAR *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags );
#define                CM_Open_Class_Key WINELIB_NAME_AW(CM_Open_Class_Key)
CMAPI CONFIGRET WINAPI CM_Open_Device_Interface_Key_ExA( const char *iface, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Open_Device_Interface_Key_ExW( const WCHAR *iface, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine );
#define                CM_Open_Device_Interface_Key_Ex WINELIB_NAME_AW(CM_Open_Device_Interface_Key_Ex)
CMAPI CONFIGRET WINAPI CM_Open_Device_Interface_KeyA( const char *iface, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Open_Device_Interface_KeyW( const WCHAR *iface, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags );
#define                CM_Open_Device_Interface_Key WINELIB_NAME_AW(CM_Open_Device_Interface_Key)
CMAPI CONFIGRET WINAPI CM_Open_DevNode_Key( DEVINST node, REGSAM access, ULONG profile, REGDISPOSITION disposition, HKEY *hkey, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Open_DevNode_Key_Ex( DEVINST node, REGSAM access, ULONG profile, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Query_And_Remove_SubTree_ExA( DEVINST ancestor, PNP_VETO_TYPE *type, char *name, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Query_And_Remove_SubTree_ExW( DEVINST ancestor, PNP_VETO_TYPE *type, WCHAR *name, ULONG len, ULONG flags, HMACHINE machine );
#define                CM_Query_And_Remove_SubTree_Ex WINELIB_NAME_AW(CM_Query_And_Remove_SubTree_Ex)
CMAPI CONFIGRET WINAPI CM_Query_And_Remove_SubTreeA( DEVINST ancestor, PNP_VETO_TYPE *type, char *name, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Query_And_Remove_SubTreeW( DEVINST ancestor, PNP_VETO_TYPE *type, WCHAR *name, ULONG len, ULONG flags );
#define                CM_Query_And_Remove_SubTree WINELIB_NAME_AW(CM_Query_And_Remove_SubTree)
CMAPI CONFIGRET WINAPI CM_Query_Arbitrator_Free_Data( void *data, ULONG len, DEVINST node, RESOURCEID id, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Query_Arbitrator_Free_Data_Ex( void *data, ULONG len, DEVINST node, RESOURCEID id, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Query_Arbitrator_Free_Size( ULONG *size, DEVINST node, RESOURCEID id, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Query_Arbitrator_Free_Size_Ex( ULONG *size, DEVINST node, RESOURCEID id, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Query_Remove_SubTree( DEVINST ancestor, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Query_Remove_SubTree_Ex( DEVINST ancestor, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Query_Resource_Conflict_List( CONFLICT_LIST *conflicts, DEVINST node, RESOURCEID id, const void *resource, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Reenumerate_DevNode( DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Reenumerate_DevNode_Ex( DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Register_Device_Driver( DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Register_Device_Driver_Ex( DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Register_Device_Interface_ExA( DEVINST node, GUID *class_, const char *reference, char *iface, ULONG *len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Register_Device_Interface_ExW( DEVINST node, GUID *class_, const WCHAR *reference, WCHAR *iface, ULONG *len, ULONG flags, HMACHINE machine );
#define                CM_Register_Device_Interface_Ex WINELIB_NAME_AW(CM_Register_Device_Interface_Ex)
CMAPI CONFIGRET WINAPI CM_Register_Device_InterfaceA( DEVINST node, GUID *class_, const char *reference, char *iface, ULONG *len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Register_Device_InterfaceW( DEVINST node, GUID *class_, const WCHAR *reference, WCHAR *iface, ULONG *len, ULONG flags );
#define                CM_Register_Device_Interface WINELIB_NAME_AW(CM_Register_Device_Interface)
CMAPI CONFIGRET WINAPI CM_Register_Notification( CM_NOTIFY_FILTER *filter, void *context, PCM_NOTIFY_CALLBACK callback, HCMNOTIFICATION *notify );
CMAPI CONFIGRET WINAPI CM_Remove_SubTree( DEVINST ancestor, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Remove_SubTree_Ex( DEVINST ancestor, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Request_Device_Eject_ExA( DEVINST node, PNP_VETO_TYPE *type, char *name, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Request_Device_Eject_ExW( DEVINST node, PNP_VETO_TYPE *type, WCHAR *name, ULONG len, ULONG flags, HMACHINE machine );
#define                CM_Request_Device_Eject_Ex WINELIB_NAME_AW(CM_Request_Device_Eject_Ex)
CMAPI CONFIGRET WINAPI CM_Request_Device_EjectA( DEVINST node, PNP_VETO_TYPE *type, char *name, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Request_Device_EjectW( DEVINST node, PNP_VETO_TYPE *type, WCHAR *name, ULONG len, ULONG flags );
#define                CM_Request_Device_Eject WINELIB_NAME_AW(CM_Request_Device_Eject)
CMAPI CONFIGRET WINAPI CM_Request_Eject_PC(void);
CMAPI CONFIGRET WINAPI CM_Request_Eject_PC_Ex( HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Run_Detection( ULONG flags );
CMAPI CONFIGRET WINAPI CM_Run_Detection_Ex( ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_Class_Property_ExW( const GUID *class_, const DEVPROPKEY *key, DEVPROPTYPE type, const BYTE *buffer, ULONG size, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_Class_PropertyW( const GUID *class_, const DEVPROPKEY *key, DEVPROPTYPE type, const BYTE *buffer, ULONG size, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Set_Class_Registry_PropertyA( GUID *class_, ULONG property, const void *buffer, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_Class_Registry_PropertyW( GUID *class_, ULONG property, const void *buffer, ULONG len, ULONG flags, HMACHINE machine );
#define                CM_Set_Class_Registry_Property WINELIB_NAME_AW(CM_Set_Class_Registry_Property)
CMAPI CONFIGRET WINAPI CM_Set_Device_Interface_Property_ExW( const WCHAR *iface, const DEVPROPKEY *key, DEVPROPTYPE type, const BYTE *buffer, ULONG size, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_Device_Interface_PropertyW( const WCHAR *iface, const DEVPROPKEY *key, DEVPROPTYPE type, const BYTE *buffer, ULONG size, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Set_DevNode_Problem( DEVINST node, ULONG problem, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Set_DevNode_Problem_Ex( DEVINST node, ULONG problem, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_DevNode_Property_ExW( DEVINST node, const DEVPROPKEY *key, DEVPROPTYPE type, const BYTE *buffer, ULONG size, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_DevNode_PropertyW( DEVINST node, const DEVPROPKEY *key, DEVPROPTYPE type, const BYTE *buffer, ULONG size, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Set_DevNode_Registry_Property_ExA( DEVINST node, ULONG property, const void *buffer, ULONG len, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_DevNode_Registry_Property_ExW( DEVINST node, ULONG property, const void *buffer, ULONG len, ULONG flags, HMACHINE machine );
#define                CM_Set_DevNode_Registry_Property_Ex WINELIB_NAME_AW(CM_Set_DevNode_Registry_Property_Ex)
CMAPI CONFIGRET WINAPI CM_Set_DevNode_Registry_PropertyA( DEVINST node, ULONG property, const void *buffer, ULONG len, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Set_DevNode_Registry_PropertyW( DEVINST node, ULONG property, const void *buffer, ULONG len, ULONG flags );
#define                CM_Set_DevNode_Registry_Property WINELIB_NAME_AW(CM_Set_DevNode_Registry_Property)
CMAPI CONFIGRET WINAPI CM_Set_HW_Prof( ULONG profile, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Set_HW_Prof_Ex( ULONG profile, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_HW_Prof_Flags_ExA( DEVINSTID_A instance_id, ULONG config, ULONG value, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Set_HW_Prof_Flags_ExW( DEVINSTID_W instance_id, ULONG config, ULONG value, ULONG flags, HMACHINE machine );
#define                CM_Set_HW_Prof_Flags_Ex WINELIB_NAME_AW(CM_Set_HW_Prof_Flags_Ex)
CMAPI CONFIGRET WINAPI CM_Set_HW_Prof_FlagsA( DEVINSTID_A instance_id, ULONG config, ULONG value, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Set_HW_Prof_FlagsW( DEVINSTID_W instance_id, ULONG config, ULONG value, ULONG flags );
#define                CM_Set_HW_Prof_Flags WINELIB_NAME_AW(CM_Set_HW_Prof_Flags)
CMAPI CONFIGRET WINAPI CM_Setup_DevNode( DEVINST node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Setup_DevNode_Ex( DEVINST node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Test_Range_Available( DWORDLONG start, DWORDLONG end, RANGE_LIST ranges, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Uninstall_DevNode( DEVNODE node, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Uninstall_DevNode_Ex( DEVNODE node, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Unregister_Device_Interface_ExA( const char *iface, ULONG flags, HMACHINE machine );
CMAPI CONFIGRET WINAPI CM_Unregister_Device_Interface_ExW( const WCHAR *iface, ULONG flags, HMACHINE machine );
#define                CM_Unregister_Device_Interface_Ex WINELIB_NAME_AW(CM_Unregister_Device_Interface_Ex)
CMAPI CONFIGRET WINAPI CM_Unregister_Device_InterfaceA( const char *iface, ULONG flags );
CMAPI CONFIGRET WINAPI CM_Unregister_Device_InterfaceW( const WCHAR *iface, ULONG flags );
#define                CM_Unregister_Device_Interface WINELIB_NAME_AW(CM_Unregister_Device_Interface)
CMAPI CONFIGRET WINAPI CM_Unregister_Notification( HCMNOTIFICATION notify );

#ifdef __cplusplus
}
#endif

#undef DECL_WINELIB_CFGMGR32_TYPE_AW

#endif /* _CFGMGR32_H_ */
