/*
 * Dumping of LE binaries
 *
 * Copyright 2004 Robert Reif
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

#include "config.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winedump.h"

struct o32_obj
{
    unsigned int        o32_size;
    unsigned int        o32_base;
    unsigned int        o32_flags;
    unsigned int        o32_pagemap;
    unsigned int        o32_mapsize;
    char                o32_name[4];
};

struct o32_map
{
    unsigned short      o32_pagedataoffset;
    unsigned char       o32_pagesize;
    unsigned char       o32_pageflags;
};

struct b32_bundle
{
    unsigned char       b32_cnt;
    unsigned char       b32_type;
};

struct vxd_descriptor
{
    unsigned int        next;
    unsigned short      sdk_version;
    unsigned short      device_number;
    unsigned char       version_major;
    unsigned char       version_minor;
    unsigned short      flags;
    char                name[8];
    unsigned int        init_order;
    unsigned int        ctrl_ofs;
    unsigned int        v86_ctrl_ofs;
    unsigned int        pm_ctrl_ofs;
    unsigned int        v86_ctrl_csip;
    unsigned int        pm_ctrl_csip;
    unsigned int        rm_ref_data;
    unsigned int        service_table_ofs;
    unsigned int        service_table_size;
    unsigned int        win32_service_table_ofs;
    unsigned int        prev;
    unsigned int        size;
    unsigned int        reserved0;
    unsigned int        reserved1;
    unsigned int        reserved2;
};

static inline WORD get_word( const BYTE *ptr )
{
    return ptr[0] | (ptr[1] << 8);
}

static void dump_le_header( const IMAGE_VXD_HEADER *le )
{
    printf( "File header:\n" );
    printf( "    Magic:                                %04x (%c%c)\n",
            le->e32_magic, LOBYTE(le->e32_magic), HIBYTE(le->e32_magic));
    printf( "    Byte order:                           %s\n",
            le->e32_border == 0 ? "little-indian" : "big-endian");
    printf( "    Word order:                           %s\n",
            le->e32_worder ==  0 ? "little-indian" : "big-endian");
    printf( "    Executable format level:              %d\n", (UINT)le->e32_level);
    printf( "    CPU type:                             %s\n",
            le->e32_cpu == 0x01 ? "Intel 80286" :
            le->e32_cpu == 0x02 ? "Intel 80386" :
            le->e32_cpu == 0x03 ? "Intel 80486" :
            le->e32_cpu == 0x04 ? "Intel 80586" :
            le->e32_cpu == 0x20 ? "Intel i860 (N10)" :
            le->e32_cpu == 0x21 ? "Intel i860 (N11)" :
            le->e32_cpu == 0x40 ? "MIPS Mark I" :
            le->e32_cpu == 0x41 ? "MIPS Mark II" :
            le->e32_cpu == 0x42 ? "MIPS Mark III" :
            "Unknown");
    printf( "    Target operating system:              %s\n",
            le->e32_os == 0x01 ? "OS/2" :
            le->e32_os == 0x02 ? "Windows" :
            le->e32_os == 0x03 ? "DOS 4.x" :
            le->e32_os == 0x04 ? "Windows 386" :
            "Unknown");
    printf( "    Module version:                       %d\n", (UINT)le->e32_ver);
    printf( "    Module type flags:                    %08x\n", (UINT)le->e32_mflags);
    if (le->e32_mflags & 0x8000)
    {
        if (le->e32_mflags & 0x0004)
            printf( "        Global initialization\n");
        else
            printf( "        Per-Process initialization\n");
        if (le->e32_mflags & 0x0010)
            printf( "        No internal fixup\n");
        if (le->e32_mflags & 0x0020)
            printf( "        No external fixup\n");
        if ((le->e32_mflags & 0x0700) == 0x0100)
            printf( "        Incompatible with PM windowing\n");
        else if ((le->e32_mflags & 0x0700) == 0x0200)
            printf( "        Compatible with PM windowing\n");
        else if ((le->e32_mflags & 0x0700) == 0x0300)
            printf( "        Uses PM windowing API\n");
        if (le->e32_mflags & 0x2000)
            printf( "        Module not loadable\n");
        if (le->e32_mflags & 0x8000)
            printf( "        Module is DLL\n");
    }
    printf( "    Number of memory pages:               %d\n", (UINT)le->e32_mpages);
    printf( "    Initial object CS number:             %08x\n", (UINT)le->e32_startobj);
    printf( "    Initial EIP:                          %08x\n", (UINT)le->e32_eip);
    printf( "    Initial object SS number:             %08x\n", (UINT)le->e32_stackobj);
    printf( "    Initial ESP:                          %08x\n", (UINT)le->e32_esp);
    printf( "    Memory page size:                     %d\n", (UINT)le->e32_pagesize);
    printf( "    Bytes on last page:                   %d\n", (UINT)le->e32_lastpagesize);
    printf( "    Fix-up section size:                  %d\n", (UINT)le->e32_fixupsize);
    printf( "    Fix-up section checksum:              %08x\n", (UINT)le->e32_fixupsum);
    printf( "    Loader section size:                  %d\n", (UINT)le->e32_ldrsize);
    printf( "    Loader section checksum:              %08x\n", (UINT)le->e32_ldrsum);
    printf( "    Offset of object table:               %08x\n", (UINT)le->e32_objtab);
    printf( "    Object table entries:                 %d\n", (UINT)le->e32_objcnt);
    printf( "    Object page map offset:               %08x\n", (UINT)le->e32_objmap);
    printf( "    Object iterate data map offset:       %08x\n", (UINT)le->e32_itermap);
    printf( "    Resource table offset:                %08x\n", (UINT)le->e32_rsrctab);
    printf( "    Resource table entries:               %d\n", (UINT)le->e32_rsrccnt);
    printf( "    Resident names table offset:          %08x\n", (UINT)le->e32_restab);
    printf( "    Entry table offset:                   %08x\n", (UINT)le->e32_enttab);
    printf( "    Module directives table offset:       %08x\n", (UINT)le->e32_dirtab);
    printf( "    Module directives entries:            %d\n", (UINT)le->e32_dircnt);
    printf( "    Fix-up page table offset:             %08x\n", (UINT)le->e32_fpagetab);
    printf( "    Fix-up record table offset:           %08x\n", (UINT)le->e32_frectab);
    printf( "    Imported modules name table offset:   %08x\n", (UINT)le->e32_impmod);
    printf( "    Imported modules count:               %d\n", (UINT)le->e32_impmodcnt);
    printf( "    Imported procedure name table offset: %08x\n", (UINT)le->e32_impproc);
    printf( "    Per-page checksum table offset:       %08x\n", (UINT)le->e32_pagesum);
    printf( "    Data pages offset from top of table:  %08x\n", (UINT)le->e32_datapage);
    printf( "    Preload page count:                   %08x\n", (UINT)le->e32_preload);
    printf( "    Non-resident names table offset:      %08x\n", (UINT)le->e32_nrestab);
    printf( "    Non-resident names table length:      %d\n", (UINT)le->e32_cbnrestab);
    printf( "    Non-resident names table checksum:    %08x\n", (UINT)le->e32_nressum);
    printf( "    Automatic data object:                %08x\n", (UINT)le->e32_autodata);
    printf( "    Debug information offset:             %08x\n", (UINT)le->e32_debuginfo);
    printf( "    Debug information length:             %d\n", (UINT)le->e32_debuglen);
    printf( "    Preload instance pages number:        %d\n", (UINT)le->e32_instpreload);
    printf( "    Demand instance pages number:         %d\n", (UINT)le->e32_instdemand);
    printf( "    Extra heap allocation:                %d\n", (UINT)le->e32_heapsize);
    printf( "    VxD resource table offset:            %08x\n", (UINT)le->e32_winresoff);
    printf( "    Size of VxD resource table:           %d\n", (UINT)le->e32_winreslen);
    printf( "    VxD identifier:                       %x\n", (UINT)le->e32_devid);
    printf( "    VxD DDK version:                      %x\n", (UINT)le->e32_ddkver);
}

static void dump_le_objects( const IMAGE_VXD_HEADER *le )
{
    const struct o32_obj *pobj;
    unsigned int i;

    printf("\nObject table:\n");
    pobj = (const struct o32_obj *)((const unsigned char *)le + le->e32_objtab);
    for (i = 0; i < le->e32_objcnt; i++)
    {
        unsigned int j;
        const struct o32_map *pmap=0;

        printf("    Obj. Rel.Base Codesize Flags    Tableidx Tablesize Name\n");
        printf("    %04X %08x %08x %08x %08x %08x  ", i + 1,
               pobj->o32_base, pobj->o32_size, pobj->o32_flags,
               pobj->o32_pagemap, pobj->o32_mapsize);
        for (j = 0; j < 4; j++)
        {
           if  (isprint(pobj->o32_name[j]))
                printf("%c", pobj->o32_name[j]);
           else
                printf(".");
        }
        printf("\n");

        if(pobj->o32_flags & 0x0001)
            printf("\tReadable\n");
        if(pobj->o32_flags & 0x0002)
            printf("\tWritable\n");
        if(pobj->o32_flags & 0x0004)
            printf("\tExecutable\n");
        if(pobj->o32_flags & 0x0008)
            printf("\tResource\n");
        if(pobj->o32_flags & 0x0010)
            printf("\tDiscardable\n");
        if(pobj->o32_flags & 0x0020)
            printf("\tShared\n");
        if(pobj->o32_flags & 0x0040)
            printf("\tPreloaded\n");
        if(pobj->o32_flags & 0x0080)
            printf("\tInvalid\n");
        if(pobj->o32_flags & 0x2000)
            printf("\tUse 32\n");

        printf("    Page tables:\n");
        printf("        Tableidx Offset Flags\n");
        pmap = (const struct o32_map *)((const unsigned char *)le + le->e32_objmap);
        pmap = &(pmap[pobj->o32_pagemap - 1]);
        for (j = 0; j < pobj->o32_mapsize; j++)
        {
            printf("        %08x %06x %02x\n",
                   pobj->o32_pagemap + j,
                   (pmap->o32_pagedataoffset << 8) + pmap->o32_pagesize,
                   (int)pmap->o32_pageflags);
            pmap++;
        }
        pobj++;
    }
}

static void dump_le_names( const IMAGE_VXD_HEADER *le )
{
    const unsigned char *pstr = (const unsigned char *)le + le->e32_restab;

    printf( "\nResident name table:\n" );
    while (*pstr)
    {
        printf( " %4d: %*.*s\n", get_word(pstr + *pstr + 1), *pstr, *pstr,
                pstr + 1 );
        pstr += *pstr + 1 + sizeof(WORD);
    }
    if (le->e32_cbnrestab)
    {
        printf( "\nNon-resident name table:\n" );
        pstr = PRD(le->e32_nrestab, 0);
        while (*pstr)
        {
            printf( " %4d: %*.*s\n", get_word(pstr + *pstr + 1), *pstr, *pstr,
                    pstr + 1 );
            pstr += *pstr + 1 + sizeof(WORD);
        }
    }
}

static void dump_le_resources( const IMAGE_VXD_HEADER *le )
{
    printf( "\nResources:\n" );
    printf( "    Not Implemented\n" );
}

static void dump_le_modules( const IMAGE_VXD_HEADER *le )
{
    printf( "\nImported modulename table:\n" );
    printf( "    Not Implemented\n" );
}

static void dump_le_entries( const IMAGE_VXD_HEADER *le )
{
    printf( "\nEntry table:\n" );
    printf( "    Not Implemented\n" );
}

static void dump_le_fixups( const IMAGE_VXD_HEADER *le )
{
    printf( "\nFixup table:\n" );
    printf( "    Not Implemented\n" );
}

static void dump_le_VxD( const IMAGE_VXD_HEADER *le )
{
    printf( "\nVxD descriptor:\n" );
    printf( "    Not Implemented\n" );
}

void le_dump( void )
{
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_VXD_HEADER *le;

    dos = PRD(0, sizeof(*dos));
    if (!dos) return;
    le = PRD(dos->e_lfanew, sizeof(*le));

    dump_le_header( le );
    dump_le_objects( le );
    dump_le_resources( le );
    dump_le_names( le );
    dump_le_entries( le );
    dump_le_modules( le );
    dump_le_fixups( le );
    dump_le_VxD( le );
}
