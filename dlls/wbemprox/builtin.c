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
#include "tlhelp32.h"
#include "initguid.h"
#include "d3d10.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static const WCHAR class_biosW[] =
    {'W','i','n','3','2','_','B','I','O','S',0};
static const WCHAR class_compsysW[] =
    {'W','i','n','3','2','_','C','o','m','p','u','t','e','r','S','y','s','t','e','m',0};
static const WCHAR class_osW[] =
    {'W','i','n','3','2','_','O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR class_processW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s',0};
static const WCHAR class_processorW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s','o','r',0};
static const WCHAR class_videocontrollerW[] =
    {'W','i','n','3','2','_','V','i','d','e','o','C','o','n','t','r','o','l','l','e','r',0};

static const WCHAR prop_adapterramW[] =
    {'A','d','a','p','t','e','r','R','A','M',0};
static const WCHAR prop_captionW[] =
    {'C','a','p','t','i','o','n',0};
static const WCHAR prop_commandlineW[] =
    {'C','o','m','m','a','n','d','L','i','n','e',0};
static const WCHAR prop_descriptionW[] =
    {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR prop_deviceidW[] =
    {'D','e','v','i','c','e','I','d',0};
static const WCHAR prop_handleW[] =
    {'H','a','n','d','l','e',0};
static const WCHAR prop_manufacturerW[] =
    {'M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR prop_modelW[] =
    {'M','o','d','e','l',0};
static const WCHAR prop_osarchitectureW[] =
    {'O','S','A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR prop_pprocessidW[] =
    {'P','a','r','e','n','t','P','r','o','c','e','s','s','I','D',0};
static const WCHAR prop_processidW[] =
    {'P','r','o','c','e','s','s','I','D',0};
static const WCHAR prop_releasedateW[] =
    {'R','e','l','e','a','s','e','D','a','t','e',0};
static const WCHAR prop_serialnumberW[] =
    {'S','e','r','i','a','l','N','u','m','b','e','r',0};
static const WCHAR prop_threadcountW[] =
    {'T','h','r','e','a','d','C','o','u','n','t',0};

static const struct column col_bios[] =
{
    { prop_descriptionW,  CIM_STRING },
    { prop_manufacturerW, CIM_STRING },
    { prop_releasedateW,  CIM_DATETIME },
    { prop_serialnumberW, CIM_STRING }
};
static const struct column col_compsys[] =
{
    { prop_descriptionW,  CIM_STRING },
    { prop_manufacturerW, CIM_STRING },
    { prop_modelW,        CIM_STRING }
};
static const struct column col_os[] =
{
    { prop_captionW,        CIM_STRING },
    { prop_osarchitectureW, CIM_STRING }
};
static const struct column col_process[] =
{
    { prop_captionW,     CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_commandlineW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_descriptionW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_handleW,      CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_pprocessidW,  CIM_UINT32 },
    { prop_processidW,   CIM_UINT32 },
    { prop_threadcountW, CIM_UINT32 }
};
static const struct column col_processor[] =
{
    { prop_manufacturerW, CIM_STRING }
};
static const struct column col_videocontroller[] =
{
    { prop_adapterramW,  CIM_UINT32 },
    { prop_deviceidW,    CIM_STRING|COL_FLAG_KEY }
};

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
static const WCHAR os_captionW[] =
    {'W','i','n','e',0};
static const WCHAR os_32bitW[] =
    {'3','2','-','b','i','t',0};
static const WCHAR os_64bitW[] =
    {'6','4','-','b','i','t',0};
static const WCHAR processor_manufacturerW[] =
    {'G','e','n','u','i','n','e','I','n','t','e','l',0};
static const WCHAR videocontroller_deviceidW[] =
    {'V','i','d','e','o','C','o','n','t','r','o','l','l','e','r','1',0};

#include "pshpack1.h"
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
};
struct record_operatingsystem
{
    const WCHAR *caption;
    const WCHAR *osarchitecture;
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
    const WCHAR *manufacturer;
};
struct record_videocontroller
{
    UINT32       adapter_ram;
    const WCHAR *device_id;
};
#include "poppack.h"

static const struct record_bios data_bios[] =
{
    { bios_descriptionW, bios_manufacturerW, bios_releasedateW, bios_serialnumberW }
};
static const struct record_computersystem data_compsys[] =
{
    { compsys_descriptionW, compsys_manufacturerW, compsys_modelW }
};
static const struct record_processor data_processor[] =
{
    { processor_manufacturerW }
};

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

static void fill_os( struct table *table )
{
    struct record_operatingsystem *rec;
    SYSTEM_INFO info;

    if (!(table->data = heap_alloc( sizeof(*rec) ))) return;

    rec = (struct record_operatingsystem *)table->data;
    rec->caption = os_captionW;

    GetNativeSystemInfo( &info );
    if (info.u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        rec->osarchitecture = os_64bitW;
    else
        rec->osarchitecture = os_32bitW;

    TRACE("created 1 row\n");
    table->num_rows = 1;
}

static void fill_videocontroller( struct table *table )
{

    struct record_videocontroller *rec;
    HRESULT hr;
    IDXGIFactory *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC desc;
    UINT vidmem = 512 * 1024 * 1024;

    if (!(table->data = heap_alloc( sizeof(*rec) ))) return;

    hr = CreateDXGIFactory( &IID_IDXGIFactory, (void **)&factory );
    if (FAILED(hr)) goto done;

    hr = IDXGIFactory_EnumAdapters( factory, 0, &adapter );
    if (FAILED(hr)) goto done;

    hr = IDXGIAdapter_GetDesc( adapter, &desc );
    if (SUCCEEDED(hr)) vidmem = desc.DedicatedVideoMemory;

done:
    rec = (struct record_videocontroller *)table->data;
    rec->device_id   = videocontroller_deviceidW;
    rec->adapter_ram = vidmem;

    TRACE("created 1 row\n");
    table->num_rows = 1;

    if (adapter) IDXGIAdapter_Release( adapter );
    if (factory) IDXGIFactory_Release( factory );
}

static struct table classtable[] =
{
    { class_biosW, SIZEOF(col_bios), col_bios, SIZEOF(data_bios), (BYTE *)data_bios, NULL },
    { class_compsysW, SIZEOF(col_compsys), col_compsys, SIZEOF(data_compsys), (BYTE *)data_compsys, NULL },
    { class_osW, SIZEOF(col_os), col_os, 0, NULL, fill_os },
    { class_processW, SIZEOF(col_process), col_process, 0, NULL, fill_process },
    { class_processorW, SIZEOF(col_processor), col_processor, SIZEOF(data_processor), (BYTE *)data_processor, NULL },
    { class_videocontrollerW, SIZEOF(col_videocontroller), col_videocontroller, 0, NULL, fill_videocontroller }
};

struct table *get_table( const WCHAR *name )
{
    UINT i;
    struct table *table = NULL;

    for (i = 0; i < SIZEOF(classtable); i++)
    {
        if (!strcmpiW( classtable[i].name, name ))
        {
            table = &classtable[i];
            if (table->fill && !table->data) table->fill( table );
            break;
        }
    }
    return table;
}
