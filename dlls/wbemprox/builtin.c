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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"
#include "tlhelp32.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static const WCHAR class_biosW[] =
    {'W','i','n','3','2','_','B','I','O','S',0};
static const WCHAR class_compsysW[] =
    {'W','i','n','3','2','_','C','o','m','p','u','t','e','r','S','y','s','t','e','m',0};
static const WCHAR class_processW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s',0};
static const WCHAR class_processorW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s','o','r',0};

static const WCHAR prop_captionW[] =
    {'C','a','p','t','i','o','n',0};
static const WCHAR prop_descriptionW[] =
    {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR prop_manufacturerW[] =
    {'M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR prop_modelW[] =
    {'M','o','d','e','l',0};
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
static const struct column col_process[] =
{
    { prop_captionW,     CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_descriptionW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_pprocessidW,  CIM_UINT32 },
    { prop_processidW,   CIM_UINT32 },
    { prop_threadcountW, CIM_UINT32 }
};
static const struct column col_processor[] =
{
    { prop_manufacturerW, CIM_STRING }
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
static const WCHAR processor_manufacturerW[] =
    {'G','e','n','u','i','n','e','I','n','t','e','l',0};

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
struct record_process
{
    const WCHAR *caption;
    const WCHAR *description;
    UINT32       pprocess_id;
    UINT32       process_id;
    UINT32       thread_count;
};
struct record_processor
{
    const WCHAR *manufacturer;
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

static void fill_process( struct table *table )
{
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
        rec->description  = heap_strdupW( entry.szExeFile );
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

static struct table classtable[] =
{
    { class_biosW, SIZEOF(col_bios), col_bios, SIZEOF(data_bios), (BYTE *)data_bios, NULL },
    { class_compsysW, SIZEOF(col_compsys), col_compsys, SIZEOF(data_compsys), (BYTE *)data_compsys, NULL },
    { class_processW, SIZEOF(col_process), col_process, 0, NULL, fill_process },
    { class_processorW, SIZEOF(col_processor), col_processor, SIZEOF(data_processor), (BYTE *)data_processor, NULL }
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
