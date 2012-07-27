/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"
#include "winsock2.h"
#include "iphlpapi.h"
#include "tlhelp32.h"
#include "initguid.h"
#include "d3d10.h"
#include "winternl.h"
#include "winioctl.h"
#include "winsvc.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static const WCHAR class_baseboardW[] =
    {'W','i','n','3','2','_','B','a','s','e','B','o','a','r','d',0};
static const WCHAR class_biosW[] =
    {'W','i','n','3','2','_','B','I','O','S',0};
static const WCHAR class_compsysW[] =
    {'W','i','n','3','2','_','C','o','m','p','u','t','e','r','S','y','s','t','e','m',0};
static const WCHAR class_logicaldiskW[] =
    {'W','i','n','3','2','_','L','o','g','i','c','a','l','D','i','s','k',0};
static const WCHAR class_networkadapterW[] =
    {'W','i','n','3','2','_','N','e','t','w','o','r','k','A','d','a','p','t','e','r',0};
static const WCHAR class_osW[] =
    {'W','i','n','3','2','_','O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR class_paramsW[] =
    {'_','_','P','A','R','A','M','E','T','E','R','S',0};
static const WCHAR class_processW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s',0};
static const WCHAR class_processorW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s','o','r',0};
static const WCHAR class_serviceW[] =
    {'W','i','n','3','2','_','S','e','r','v','i','c','e',0};
static const WCHAR class_stdregprovW[] =
    {'S','t','d','R','e','g','P','r','o','v',0};
static const WCHAR class_videocontrollerW[] =
    {'W','i','n','3','2','_','V','i','d','e','o','C','o','n','t','r','o','l','l','e','r',0};

static const WCHAR prop_adapterramW[] =
    {'A','d','a','p','t','e','r','R','A','M',0};
static const WCHAR prop_captionW[] =
    {'C','a','p','t','i','o','n',0};
static const WCHAR prop_classW[] =
    {'C','l','a','s','s',0};
static const WCHAR prop_commandlineW[] =
    {'C','o','m','m','a','n','d','L','i','n','e',0};
static const WCHAR prop_cpustatusW[] =
    {'C','p','u','S','t','a','t','u','s',0};
static const WCHAR prop_csdversionW[] =
    {'C','S','D','V','e','r','s','i','o','n',0};
static const WCHAR prop_currentbitsperpixelW[] =
    {'C','u','r','r','e','n','t','B','i','t','s','P','e','r','P','i','x','e','l',0};
static const WCHAR prop_currenthorizontalresW[] =
    {'C','u','r','r','e','n','t','H','o','r','i','z','o','n','t','a','l','R','e','s','o','l','u','t','i','o','n',0};
static const WCHAR prop_currentverticalresW[] =
    {'C','u','r','r','e','n','t','V','e','r','t','i','c','a','l','R','e','s','o','l','u','t','i','o','n',0};
static const WCHAR prop_defaultvalueW[] =
    {'D','e','f','a','u','l','t','V','a','l','u','e',0};
static const WCHAR prop_descriptionW[] =
    {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR prop_deviceidW[] =
    {'D','e','v','i','c','e','I','d',0};
static const WCHAR prop_directionW[] =
    {'D','i','r','e','c','t','i','o','n',0};
static const WCHAR prop_displaynameW[] =
    {'D','i','s','p','l','a','y','N','a','m','e',0};
static const WCHAR prop_drivetypeW[] =
    {'D','r','i','v','e','T','y','p','e',0};
static const WCHAR prop_filesystemW[] =
    {'F','i','l','e','S','y','s','t','e','m',0};
static const WCHAR prop_freespaceW[] =
    {'F','r','e','e','S','p','a','c','e',0};
static const WCHAR prop_handleW[] =
    {'H','a','n','d','l','e',0};
static const WCHAR prop_interfaceindexW[] =
    {'I','n','t','e','r','f','a','c','e','I','n','d','e','x',0};
static const WCHAR prop_macaddressW[] =
    {'M','A','C','A','d','d','r','e','s','s',0};
static const WCHAR prop_manufacturerW[] =
    {'M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR prop_methodW[] =
    {'M','e','t','h','o','d',0};
static const WCHAR prop_modelW[] =
    {'M','o','d','e','l',0};
static const WCHAR prop_nameW[] =
    {'N','a','m','e',0};
static const WCHAR prop_netconnectionstatusW[] =
    {'N','e','t','C','o','n','n','e','c','t','i','o','n','S','t','a','t','u','s',0};
static const WCHAR prop_numlogicalprocessorsW[] =
    {'N','u','m','b','e','r','O','f','L','o','g','i','c','a','l','P','r','o','c','e','s','s','o','r','s',0};
static const WCHAR prop_numprocessorsW[] =
    {'N','u','m','b','e','r','O','f','P','r','o','c','e','s','s','o','r','s',0};
static const WCHAR prop_osarchitectureW[] =
    {'O','S','A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR prop_oslanguageW[] =
    {'O','S','L','a','n','g','u','a','g','e',0};
static const WCHAR prop_parameterW[] =
    {'P','a','r','a','m','e','t','e','r',0};
static const WCHAR prop_pnpdeviceidW[] =
    {'P','N','P','D','e','v','i','c','e','I','D',0};
static const WCHAR prop_pprocessidW[] =
    {'P','a','r','e','n','t','P','r','o','c','e','s','s','I','D',0};
static const WCHAR prop_processidW[] =
    {'P','r','o','c','e','s','s','I','D',0};
static const WCHAR prop_processoridW[] =
    {'P','r','o','c','e','s','s','o','r','I','d',0};
static const WCHAR prop_releasedateW[] =
    {'R','e','l','e','a','s','e','D','a','t','e',0};
static const WCHAR prop_serialnumberW[] =
    {'S','e','r','i','a','l','N','u','m','b','e','r',0};
static const WCHAR prop_servicetypeW[] =
    {'S','e','r','v','i','c','e','T','y','p','e',0};
static const WCHAR prop_sizeW[] =
    {'S','i','z','e',0};
static const WCHAR prop_speedW[] =
    {'S','p','e','e','d',0};
static const WCHAR prop_stateW[] =
    {'S','t','a','t','e',0};
static const WCHAR prop_systemdirectoryW[] =
    {'S','y','s','t','e','m','D','i','r','e','c','t','o','r','y',0};
static const WCHAR prop_tagW[] =
    {'T','a','g',0};
static const WCHAR prop_threadcountW[] =
    {'T','h','r','e','a','d','C','o','u','n','t',0};
static const WCHAR prop_totalphysicalmemoryW[] =
    {'T','o','t','a','l','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
static const WCHAR prop_typeW[] =
    {'T','y','p','e',0};

static const WCHAR method_enumkeyW[] =
    {'E','n','u','m','K','e','y',0};
static const WCHAR method_enumvaluesW[] =
    {'E','n','u','m','V','a','l','u','e','s',0};

static const WCHAR param_defkeyW[] =
    {'h','D','e','f','K','e','y',0};
static const WCHAR param_namesW[] =
    {'N','a','m','e','s',0};
static const WCHAR param_returnvalueW[] =
    {'R','e','t','u','r','n','V','a','l','u','e',0};
static const WCHAR param_subkeynameW[] =
    {'s','S','u','b','K','e','y','N','a','m','e',0};
static const WCHAR param_typesW[] =
    {'T','y','p','e','s',0};

/* column definitions must be kept in sync with record structures below */
static const struct column col_baseboard[] =
{
    { prop_manufacturerW,  CIM_STRING },
    { prop_serialnumberW,  CIM_STRING },
    { prop_tagW,           CIM_STRING|COL_FLAG_KEY }
};
static const struct column col_bios[] =
{
    { prop_descriptionW,  CIM_STRING },
    { prop_manufacturerW, CIM_STRING },
    { prop_releasedateW,  CIM_DATETIME },
    { prop_serialnumberW, CIM_STRING }
};
static const struct column col_compsys[] =
{
    { prop_descriptionW,          CIM_STRING },
    { prop_manufacturerW,         CIM_STRING },
    { prop_modelW,                CIM_STRING },
    { prop_numlogicalprocessorsW, CIM_UINT32, VT_I4 },
    { prop_numprocessorsW,        CIM_UINT32, VT_I4 },
    { prop_totalphysicalmemoryW,  CIM_UINT64 }
};
static const struct column col_logicaldisk[] =
{
    { prop_deviceidW,   CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_drivetypeW,  CIM_UINT32, VT_I4 },
    { prop_filesystemW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_freespaceW,  CIM_UINT64 },
    { prop_sizeW,       CIM_UINT64 }
};
static const struct column col_networkadapter[] =
{
    { prop_deviceidW,            CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_interfaceindexW,      CIM_UINT32, VT_I4 },
    { prop_macaddressW,          CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_netconnectionstatusW, CIM_UINT16, VT_I4 },
    { prop_pnpdeviceidW,         CIM_STRING },
    { prop_speedW,               CIM_UINT64 }
};
static const struct column col_os[] =
{
    { prop_captionW,         CIM_STRING },
    { prop_csdversionW,      CIM_STRING },
    { prop_osarchitectureW,  CIM_STRING },
    { prop_oslanguageW,      CIM_UINT32, VT_I4 },
    { prop_systemdirectoryW, CIM_STRING }
};
static const struct column col_params[] =
{
    { prop_classW,        CIM_STRING },
    { prop_methodW,       CIM_STRING },
    { prop_directionW,    CIM_SINT32 },
    { prop_parameterW,    CIM_STRING },
    { prop_typeW,         CIM_UINT32 },
    { prop_defaultvalueW, CIM_UINT32 }
};
static const struct column col_process[] =
{
    { prop_captionW,     CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_commandlineW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_descriptionW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_handleW,      CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_pprocessidW,  CIM_UINT32, VT_I4 },
    { prop_processidW,   CIM_UINT32, VT_I4 },
    { prop_threadcountW, CIM_UINT32, VT_I4 }
};
static const struct column col_processor[] =
{
    { prop_cpustatusW,    CIM_UINT16 },
    { prop_deviceidW,     CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_manufacturerW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_nameW,         CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_processoridW,  CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_service[] =
{
    { prop_displaynameW,      CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_nameW,             CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_processidW,        CIM_UINT32 },
    { prop_servicetypeW,      CIM_STRING },
    { prop_stateW,            CIM_STRING }
};
static const struct column col_stdregprov[] =
{
    { method_enumkeyW,    CIM_OBJECT|COL_FLAG_METHOD },
    { method_enumvaluesW, CIM_OBJECT|COL_FLAG_METHOD }
};
static const struct column col_videocontroller[] =
{
    { prop_adapterramW,           CIM_UINT32 },
    { prop_currentbitsperpixelW,  CIM_UINT32 },
    { prop_currenthorizontalresW, CIM_UINT32 },
    { prop_currentverticalresW,   CIM_UINT32 },
    { prop_deviceidW,             CIM_STRING|COL_FLAG_KEY },
    { prop_nameW,                 CIM_STRING|COL_FLAG_DYNAMIC }
};

static const WCHAR baseboard_manufacturerW[] =
    {'I','n','t','e','l',' ','C','o','r','p','o','r','a','t','i','o','n',0};
static const WCHAR baseboard_serialnumberW[] =
    {'N','o','n','e',0};
static const WCHAR baseboard_tagW[] =
    {'B','a','s','e',' ','B','o','a','r','d',0};
static const WCHAR bios_descriptionW[] =
    {'D','e','f','a','u','l','t',' ','S','y','s','t','e','m',' ','B','I','O','S',0};
static const WCHAR bios_manufacturerW[] =
    {'T','h','e',' ','W','i','n','e',' ','P','r','o','j','e','c','t',0};
static const WCHAR bios_releasedateW[] =
    {'2','0','1','2','0','6','0','8','0','0','0','0','0','0','.','0','0','0','0','0','0','+','0','0','0',0};
static const WCHAR bios_serialnumberW[] =
    {'0',0};
static const WCHAR compsys_descriptionW[] =
    {'A','T','/','A','T',' ','C','O','M','P','A','T','I','B','L','E',0};
static const WCHAR compsys_manufacturerW[] =
    {'T','h','e',' ','W','i','n','e',' ','P','r','o','j','e','c','t',0};
static const WCHAR compsys_modelW[] =
    {'W','i','n','e',0};
static const WCHAR networkadapter_pnpdeviceidW[]=
    {'P','C','I','\\','V','E','N','_','8','0','8','6','&','D','E','V','_','1','0','0','E','&',
     'S','U','B','S','Y','S','_','0','0','1','E','8','0','8','6','&','R','E','V','_','0','2','\\',
     '3','&','2','6','7','A','6','1','6','A','&','1','&','1','8',0};
static const WCHAR os_captionW[] =
    {'M','i','c','r','o','s','o','f','t',' ','W','i','n','d','o','w','s',' ','X','P',' ',
     'V','e','r','s','i','o','n',' ','=',' ','5','.','1','.','2','6','0','0',0};
static const WCHAR os_csdversionW[] =
    {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','3',0};
static const WCHAR os_32bitW[] =
    {'3','2','-','b','i','t',0};
static const WCHAR os_64bitW[] =
    {'6','4','-','b','i','t',0};
static const WCHAR videocontroller_deviceidW[] =
    {'V','i','d','e','o','C','o','n','t','r','o','l','l','e','r','1',0};

#include "pshpack1.h"
struct record_baseboard
{
    const WCHAR *manufacturer;
    const WCHAR *serialnumber;
    const WCHAR *tag;
};
struct record_bios
{
    const WCHAR *description;
    const WCHAR *manufacturer;
    const WCHAR *releasedate;
    const WCHAR *serialnumber;
};
struct record_computersystem
{
    const WCHAR *description;
    const WCHAR *manufacturer;
    const WCHAR *model;
    UINT32       num_logical_processors;
    UINT32       num_processors;
    UINT64       total_physical_memory;
};
struct record_logicaldisk
{
    const WCHAR *device_id;
    UINT32       drivetype;
    const WCHAR *filesystem;
    UINT64       freespace;
    UINT64       size;
};
struct record_networkadapter
{
    const WCHAR *device_id;
    INT32        interface_index;
    const WCHAR *mac_address;
    UINT16       netconnection_status;
    const WCHAR *pnpdevice_id;
    UINT64       speed;
};
struct record_operatingsystem
{
    const WCHAR *caption;
    const WCHAR *csdversion;
    const WCHAR *osarchitecture;
    UINT32       oslanguage;
    const WCHAR *systemdirectory;
};
struct record_params
{
    const WCHAR *class;
    const WCHAR *method;
    INT32        direction;
    const WCHAR *parameter;
    UINT32       type;
    UINT32       defaultvalue;
};
struct record_process
{
    const WCHAR *caption;
    const WCHAR *commandline;
    const WCHAR *description;
    const WCHAR *handle;
    UINT32       pprocess_id;
    UINT32       process_id;
    UINT32       thread_count;
};
struct record_processor
{
    UINT16       cpu_status;
    const WCHAR *device_id;
    const WCHAR *manufacturer;
    const WCHAR *name;
    const WCHAR *processor_id;
};
struct record_service
{
    const WCHAR *displayname;
    const WCHAR *name;
    UINT32       process_id;
    const WCHAR *servicetype;
    const WCHAR *state;
};
struct record_stdregprov
{
    class_method *enumkey;
    class_method *enumvalues;
};
struct record_videocontroller
{
    UINT32       adapter_ram;
    UINT32       current_bitsperpixel;
    UINT32       current_horizontalres;
    UINT32       current_verticalres;
    const WCHAR *device_id;
    const WCHAR *name;
};
#include "poppack.h"

static HRESULT reg_enumkey( IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("\n");
    return E_NOTIMPL;
}
static HRESULT reg_enumvalues( IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct record_baseboard data_baseboard[] =
{
    { baseboard_manufacturerW, baseboard_serialnumberW, baseboard_tagW }
};
static const struct record_bios data_bios[] =
{
    { bios_descriptionW, bios_manufacturerW, bios_releasedateW, bios_serialnumberW }
};
static const struct record_params data_params[] =
{
    { class_stdregprovW, method_enumkeyW, 1, param_defkeyW, CIM_UINT32, 0x80000002 },
    { class_stdregprovW, method_enumkeyW, 1, param_subkeynameW, CIM_STRING },
    { class_stdregprovW, method_enumkeyW, -1, param_returnvalueW, CIM_UINT32 },
    { class_stdregprovW, method_enumkeyW, -1, param_namesW, CIM_STRING|CIM_FLAG_ARRAY },
    { class_stdregprovW, method_enumvaluesW, 1, param_defkeyW, CIM_UINT32, 0x80000002 },
    { class_stdregprovW, method_enumvaluesW, 1, param_subkeynameW, CIM_STRING },
    { class_stdregprovW, method_enumvaluesW, -1, param_returnvalueW, CIM_UINT32 },
    { class_stdregprovW, method_enumvaluesW, -1, param_namesW, CIM_STRING|CIM_FLAG_ARRAY },
    { class_stdregprovW, method_enumvaluesW, -1, param_typesW, CIM_SINT32|CIM_FLAG_ARRAY }
};
static const struct record_stdregprov data_stdregprov[] =
{
    { reg_enumkey, reg_enumvalues }
};

static UINT get_processor_count(void)
{
    SYSTEM_BASIC_INFORMATION info;

    if (NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL )) return 1;
    return info.NumberOfProcessors;
}

static UINT64 get_total_physical_memory(void)
{
    MEMORYSTATUSEX status;

    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx( &status )) return 1024 * 1024 * 1024;
    return status.ullTotalPhys;
}

static void fill_compsys( struct table *table )
{
    struct record_computersystem *rec;

    if (!(table->data = heap_alloc( sizeof(*rec) ))) return;

    rec = (struct record_computersystem *)table->data;
    rec->description            = compsys_descriptionW;
    rec->manufacturer           = compsys_manufacturerW;
    rec->model                  = compsys_modelW;
    rec->num_logical_processors = get_processor_count();
    rec->num_processors         = rec->num_logical_processors;
    rec->total_physical_memory  = get_total_physical_memory();

    TRACE("created 1 row\n");
    table->num_rows = 1;
}

static WCHAR *get_filesystem( const WCHAR *root )
{
    static const WCHAR ntfsW[] = {'N','T','F','S',0};
    WCHAR buffer[MAX_PATH + 1];

    if (GetVolumeInformationW( root, NULL, 0, NULL, NULL, NULL, buffer, MAX_PATH + 1 ))
        return heap_strdupW( buffer );
    return heap_strdupW( ntfsW );
}

static UINT64 get_freespace( const WCHAR *dir, UINT64 *disksize )
{
    WCHAR root[] = {'\\','\\','.','\\','A',':',0};
    ULARGE_INTEGER free;
    DISK_GEOMETRY_EX info;
    HANDLE handle;

    free.QuadPart = 512 * 1024 * 1024;
    GetDiskFreeSpaceExW( dir, NULL, NULL, &free );

    root[4] = dir[0];
    handle = CreateFileW( root, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (handle != INVALID_HANDLE_VALUE)
    {
        if (DeviceIoControl( handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &info, sizeof(info), NULL, NULL ))
            *disksize = info.DiskSize.QuadPart;
        CloseHandle( handle );
    }
    return free.QuadPart;
}

static void fill_logicaldisk( struct table *table )
{
    static const WCHAR fmtW[] = {'%','c',':',0};
    WCHAR device_id[3], root[] = {'A',':','\\',0};
    struct record_logicaldisk *rec;
    UINT i, num_rows = 0, offset = 0, count = 4, type;
    UINT64 size = 1024 * 1024 * 1024;
    DWORD drives = GetLogicalDrives();

    if (!(table->data = heap_alloc( count * sizeof(*rec) ))) return;

    for (i = 0; i < sizeof(drives); i++)
    {
        if (drives & (1 << i))
        {
            root[0] = 'A' + i;
            type = GetDriveTypeW( root );
            if (type != DRIVE_FIXED && type != DRIVE_CDROM && type != DRIVE_REMOVABLE)
                continue;

            if (num_rows > count)
            {
                BYTE *data;
                count *= 2;
                if (!(data = heap_realloc( table->data, count * sizeof(*rec) ))) return;
                table->data = data;
            }
            rec = (struct record_logicaldisk *)(table->data + offset);
            sprintfW( device_id, fmtW, 'A' + i );
            rec->device_id  = heap_strdupW( device_id );
            rec->drivetype  = type;
            rec->filesystem = get_filesystem( root );
            rec->freespace  = get_freespace( root, &size );
            rec->size       = size;
            offset += sizeof(*rec);
            num_rows++;
        }
    }
    TRACE("created %u rows\n", num_rows);
    table->num_rows = num_rows;
}

static UINT16 get_connection_status( IF_OPER_STATUS status )
{
    switch (status)
    {
    case IfOperStatusDown:
        return 0; /* Disconnected */
    case IfOperStatusUp:
        return 2; /* Connected */
    default:
        ERR("unhandled status %u\n", status);
        break;
    }
    return 0;
}
static WCHAR *get_mac_address( const BYTE *addr, DWORD len )
{
    static const WCHAR fmtW[] =
        {'%','0','2','x',':','%','0','2','x',':','%','0','2','x',':',
         '%','0','2','x',':','%','0','2','x',':','%','0','2','x',0};
    WCHAR *ret;

    if (len != 6 || !(ret = heap_alloc( 18 * sizeof(WCHAR) ))) return NULL;
    sprintfW( ret, fmtW, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5] );
    return ret;
}

static void fill_networkadapter( struct table *table )
{
    static const WCHAR fmtW[] = {'%','u',0};
    WCHAR device_id[11];
    struct record_networkadapter *rec;
    IP_ADAPTER_ADDRESSES *aa, *buffer;
    UINT num_rows = 0, offset = 0;
    DWORD size = 0, ret;

    ret = GetAdaptersAddresses( AF_UNSPEC, 0, NULL, NULL, &size );
    if (ret != ERROR_BUFFER_OVERFLOW) return;

    if (!(buffer = heap_alloc( size ))) return;
    if (GetAdaptersAddresses( AF_UNSPEC, 0, NULL, buffer, &size ))
    {
        heap_free( buffer );
        return;
    }
    for (aa = buffer; aa; aa = aa->Next) num_rows++;
    if (!(table->data = heap_alloc( sizeof(*rec) * num_rows )))
    {
        heap_free( buffer );
        return;
    }
    for (aa = buffer; aa; aa = aa->Next)
    {
        rec = (struct record_networkadapter *)(table->data + offset);
        sprintfW( device_id, fmtW, aa->u.s.IfIndex );
        rec->device_id            = heap_strdupW( device_id );
        rec->interface_index      = aa->u.s.IfIndex;
        rec->mac_address          = get_mac_address( aa->PhysicalAddress, aa->PhysicalAddressLength );
        rec->netconnection_status = get_connection_status( aa->OperStatus );
        rec->pnpdevice_id         = networkadapter_pnpdeviceidW;
        rec->speed                = 1000000;
        offset += sizeof(*rec);
    }
    TRACE("created %u rows\n", num_rows);
    table->num_rows = num_rows;

    heap_free( buffer );
}

static WCHAR *get_cmdline( DWORD process_id )
{
    if (process_id == GetCurrentProcessId()) return heap_strdupW( GetCommandLineW() );
    return NULL; /* FIXME handle different process case */
}

static void fill_process( struct table *table )
{
    static const WCHAR fmtW[] = {'%','u',0};
    WCHAR handle[11];
    struct record_process *rec;
    PROCESSENTRY32W entry;
    HANDLE snap;
    UINT num_rows = 0, offset = 0, count = 8;

    snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if (snap == INVALID_HANDLE_VALUE) return;

    entry.dwSize = sizeof(entry);
    if (!Process32FirstW( snap, &entry )) goto done;
    if (!(table->data = heap_alloc( count * sizeof(*rec) ))) goto done;

    do
    {
        if (num_rows > count)
        {
            BYTE *data;
            count *= 2;
            if (!(data = heap_realloc( table->data, count * sizeof(*rec) ))) goto done;
            table->data = data;
        }
        rec = (struct record_process *)(table->data + offset);
        rec->caption      = heap_strdupW( entry.szExeFile );
        rec->commandline  = get_cmdline( entry.th32ProcessID );
        rec->description  = heap_strdupW( entry.szExeFile );
        sprintfW( handle, fmtW, entry.th32ProcessID );
        rec->handle       = heap_strdupW( handle );
        rec->process_id   = entry.th32ProcessID;
        rec->pprocess_id  = entry.th32ParentProcessID;
        rec->thread_count = entry.cntThreads;
        offset += sizeof(*rec);
        num_rows++;
    } while (Process32NextW( snap, &entry ));

    TRACE("created %u rows\n", num_rows);
    table->num_rows = num_rows;

done:
    CloseHandle( snap );
}

static inline void do_cpuid( unsigned int ax, unsigned int *p )
{
#ifdef __i386__
    __asm__("pushl %%ebx\n\t"
                "cpuid\n\t"
                "movl %%ebx, %%esi\n\t"
                "popl %%ebx"
                : "=a" (p[0]), "=S" (p[1]), "=c" (p[2]), "=d" (p[3])
                :  "0" (ax));
#endif
}

static void get_processor_id( WCHAR *processor_id )
{
    static const WCHAR fmtW[] = {'%','0','8','X','%','0','8','X',0};
    unsigned int regs[4] = {0, 0, 0, 0};

    do_cpuid( 1, regs );
    sprintfW( processor_id, fmtW, regs[3], regs[0] );
}
static void regs_to_str( unsigned int *regs, unsigned int len, WCHAR *buffer )
{
    unsigned int i;
    unsigned char *p = (unsigned char *)regs;

    for (i = 0; i < len; i++)
    {
        buffer[i] = *p++;
    }
    buffer[i] = 0;
}
static void get_processor_manufacturer( WCHAR *manufacturer )
{
    unsigned int tmp, regs[4] = {0, 0, 0, 0};

    do_cpuid( 0, regs );
    tmp = regs[2];      /* swap edx and ecx */
    regs[2] = regs[3];
    regs[3] = tmp;

    regs_to_str( regs + 1, 12, manufacturer );
}
static void get_processor_name( WCHAR *name )
{
    unsigned int regs[4] = {0, 0, 0, 0};

    do_cpuid( 0x80000000, regs );
    if (regs[0] >= 0x80000004)
    {
        do_cpuid( 0x80000002, regs );
        regs_to_str( regs, 16, name );
        do_cpuid( 0x80000003, regs );
        regs_to_str( regs, 16, name + 16 );
        do_cpuid( 0x80000004, regs );
        regs_to_str( regs, 16, name + 32 );
    }
}

static void fill_processor( struct table *table )
{
    static const WCHAR fmtW[] = {'C','P','U','%','u',0};
    WCHAR device_id[14], processor_id[17], manufacturer[13], name[49] = {0};
    struct record_processor *rec;
    UINT i, offset = 0, count = get_processor_count();

    if (!(table->data = heap_alloc( sizeof(*rec) * count ))) return;

    get_processor_id( processor_id );
    get_processor_manufacturer( manufacturer );
    get_processor_name( name );

    for (i = 0; i < count; i++)
    {
        rec = (struct record_processor *)(table->data + offset);
        rec->cpu_status   = 1; /* CPU Enabled */
        sprintfW( device_id, fmtW, i );
        rec->device_id    = heap_strdupW( device_id );
        rec->manufacturer = heap_strdupW( manufacturer );
        rec->name         = heap_strdupW( name );
        rec->processor_id = heap_strdupW( processor_id );
        offset += sizeof(*rec);
    }

    TRACE("created %u rows\n", count);
    table->num_rows = count;
}

static void fill_os( struct table *table )
{
    struct record_operatingsystem *rec;
    WCHAR path[MAX_PATH];
    SYSTEM_INFO info;
    void *redir;

    if (!(table->data = heap_alloc( sizeof(*rec) ))) return;

    rec = (struct record_operatingsystem *)table->data;
    rec->caption    = os_captionW;
    rec->csdversion = os_csdversionW;

    GetNativeSystemInfo( &info );
    if (info.u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        rec->osarchitecture = os_64bitW;
    else
        rec->osarchitecture = os_32bitW;

    rec->oslanguage = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );

    Wow64DisableWow64FsRedirection( &redir );
    GetSystemDirectoryW( path, MAX_PATH );
    Wow64RevertWow64FsRedirection( redir );
    rec->systemdirectory = heap_strdupW( path );

    TRACE("created 1 row\n");
    table->num_rows = 1;
}

static const WCHAR *get_service_type( DWORD type )
{
    static const WCHAR filesystem_driverW[] =
        {'F','i','l','e',' ','S','y','s','t','e','m',' ','D','r','i','v','e','r',0};
    static const WCHAR kernel_driverW[] =
        {'K','e','r','n','e','l',' ','D','r','i','v','e','r',0};
    static const WCHAR own_processW[] =
        {'O','w','n',' ','P','r','o','c','e','s','s',0};
    static const WCHAR share_processW[] =
        {'S','h','a','r','e',' ','P','r','o','c','e','s','s',0};

    if (type & SERVICE_KERNEL_DRIVER)            return kernel_driverW;
    else if (type & SERVICE_FILE_SYSTEM_DRIVER)  return filesystem_driverW;
    else if (type & SERVICE_WIN32_OWN_PROCESS)   return own_processW;
    else if (type & SERVICE_WIN32_SHARE_PROCESS) return share_processW;
    else ERR("unhandled type 0x%08x\n", type);
    return NULL;
}
static const WCHAR *get_service_state( DWORD state )
{
    static const WCHAR runningW[] =
        {'R','u','n','n','i','n','g',0};
    static const WCHAR start_pendingW[] =
        {'S','t','a','r','t',' ','P','e','n','d','i','n','g',0};
    static const WCHAR stop_pendingW[] =
        {'S','t','o','p',' ','P','e','n','d','i','n','g',0};
    static const WCHAR stoppedW[] =
        {'S','t','o','p','p','e','d',0};
    static const WCHAR unknownW[] =
        {'U','n','k','n','o','w','n',0};

    switch (state)
    {
    case SERVICE_STOPPED:       return stoppedW;
    case SERVICE_START_PENDING: return start_pendingW;
    case SERVICE_STOP_PENDING:  return stop_pendingW;
    case SERVICE_RUNNING:       return runningW;
    default:
        ERR("unknown state %u\n", state);
        return unknownW;
    }
}

static void fill_service( struct table *table )
{
    struct record_service *rec;
    SC_HANDLE manager;
    ENUM_SERVICE_STATUS_PROCESSW *tmp, *services = NULL;
    SERVICE_STATUS_PROCESS *status;
    UINT i, num_rows = 0, offset = 0, size = 256, needed, count;
    BOOL ret;

    if (!(manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE ))) return;
    if (!(services = heap_alloc( size ))) goto done;

    ret = EnumServicesStatusExW( manager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL,
                                 SERVICE_STATE_ALL, (BYTE *)services, size, &needed,
                                 &count, NULL, NULL );
    if (!ret)
    {
        if (GetLastError() != ERROR_MORE_DATA) goto done;
        size = needed;
        if (!(tmp = heap_realloc( services, size ))) goto done;
        services = tmp;
        ret = EnumServicesStatusExW( manager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL,
                                     SERVICE_STATE_ALL, (BYTE *)services, size, &needed,
                                     &count, NULL, NULL );
        if (!ret) goto done;
    }
    if (!(table->data = heap_alloc( sizeof(*rec) * count ))) goto done;

    for (i = 0; i < count; i++)
    {
        status = &services[i].ServiceStatusProcess;
        rec = (struct record_service *)(table->data + offset);
        rec->displayname = heap_strdupW( services[i].lpDisplayName );
        rec->name        = heap_strdupW( services[i].lpServiceName );
        rec->process_id  = status->dwProcessId;
        rec->servicetype = get_service_type( status->dwServiceType );
        rec->state       = get_service_state( status->dwCurrentState );
        offset += sizeof(*rec);
        num_rows++;
    }

    TRACE("created %u rows\n", num_rows);
    table->num_rows = num_rows;

done:
    CloseServiceHandle( manager );
    heap_free( services );
}

static UINT32 get_bits_per_pixel( UINT *hres, UINT *vres )
{
    HDC hdc = GetDC( NULL );
    UINT32 ret;

    if (!hdc) return 32;
    ret = GetDeviceCaps( hdc, BITSPIXEL );
    *hres = GetDeviceCaps( hdc, HORZRES );
    *vres = GetDeviceCaps( hdc, VERTRES );
    ReleaseDC( NULL, hdc );
    return ret;
}

static void fill_videocontroller( struct table *table )
{

    struct record_videocontroller *rec;
    HRESULT hr;
    IDXGIFactory *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC desc;
    UINT hres = 1024, vres = 768, vidmem = 512 * 1024 * 1024;
    const WCHAR *name = videocontroller_deviceidW;

    if (!(table->data = heap_alloc( sizeof(*rec) ))) return;

    hr = CreateDXGIFactory( &IID_IDXGIFactory, (void **)&factory );
    if (FAILED(hr)) goto done;

    hr = IDXGIFactory_EnumAdapters( factory, 0, &adapter );
    if (FAILED(hr)) goto done;

    hr = IDXGIAdapter_GetDesc( adapter, &desc );
    if (SUCCEEDED(hr))
    {
        vidmem = desc.DedicatedVideoMemory;
        name   = desc.Description;
    }

done:
    rec = (struct record_videocontroller *)table->data;
    rec->adapter_ram           = vidmem;
    rec->current_bitsperpixel  = get_bits_per_pixel( &hres, &vres );
    rec->current_horizontalres = hres;
    rec->current_verticalres   = vres;
    rec->device_id             = videocontroller_deviceidW;
    rec->name                  = heap_strdupW( name );

    TRACE("created 1 row\n");
    table->num_rows = 1;

    if (adapter) IDXGIAdapter_Release( adapter );
    if (factory) IDXGIFactory_Release( factory );
}

static struct table builtin_classes[] =
{
    { class_baseboardW, SIZEOF(col_baseboard), col_baseboard, SIZEOF(data_baseboard), (BYTE *)data_baseboard },
    { class_biosW, SIZEOF(col_bios), col_bios, SIZEOF(data_bios), (BYTE *)data_bios },
    { class_compsysW, SIZEOF(col_compsys), col_compsys, 0, NULL, fill_compsys },
    { class_logicaldiskW, SIZEOF(col_logicaldisk), col_logicaldisk, 0, NULL, fill_logicaldisk },
    { class_networkadapterW, SIZEOF(col_networkadapter), col_networkadapter, 0, NULL, fill_networkadapter },
    { class_osW, SIZEOF(col_os), col_os, 0, NULL, fill_os },
    { class_paramsW, SIZEOF(col_params), col_params, SIZEOF(data_params), (BYTE *)data_params },
    { class_processW, SIZEOF(col_process), col_process, 0, NULL, fill_process },
    { class_processorW, SIZEOF(col_processor), col_processor, 0, NULL, fill_processor },
    { class_serviceW, SIZEOF(col_service), col_service, 0, NULL, fill_service },
    { class_stdregprovW, SIZEOF(col_stdregprov), col_stdregprov, SIZEOF(data_stdregprov), (BYTE *)data_stdregprov },
    { class_videocontrollerW, SIZEOF(col_videocontroller), col_videocontroller, 0, NULL, fill_videocontroller }
};

void init_table_list( void )
{
    static struct list tables = LIST_INIT( tables );
    UINT i;

    for (i = 0; i < SIZEOF(builtin_classes); i++)
    {
        list_add_tail( &tables, &builtin_classes[i].entry );
    }
    table_list = &tables;
}
