/*
 * Dumping of NE binaries
 *
 * Copyright 2002 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "winnt.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winedump.h"

static void dump_ne_header( const IMAGE_OS2_HEADER *ne )
{
    printf( "File header:\n" );
    printf( "Linker version:      %d.%d\n", ne->ne_ver, ne->ne_rev );
    printf( "Entry table:         %x len %d\n", ne->ne_enttab, ne->ne_cbenttab );
    printf( "Checksum:            %08lx\n", ne->ne_crc );
    printf( "Flags:               %04x\n", ne->ne_flags );
    printf( "Auto data segment:   %x\n", ne->ne_autodata );
    printf( "Heap size:           %d bytes\n", ne->ne_heap );
    printf( "Stack size:          %d bytes\n", ne->ne_stack );
    printf( "Stack pointer:       %x:%04x\n", SELECTOROF(ne->ne_sssp), OFFSETOF(ne->ne_sssp) );
    printf( "Entry point:         %x:%04x\n", SELECTOROF(ne->ne_csip), OFFSETOF(ne->ne_csip) );
    printf( "Number of segments:  %d\n", ne->ne_cseg );
    printf( "Number of modrefs:   %d\n", ne->ne_cmod );
    printf( "Segment table:       %x\n", ne->ne_segtab );
    printf( "Resource table:      %x\n", ne->ne_rsrctab );
    printf( "Resident name table: %x\n", ne->ne_restab );
    printf( "Module table:        %x\n", ne->ne_modtab );
    printf( "Import table:        %x\n", ne->ne_imptab );
    printf( "Non-resident table:  %lx\n", ne->ne_nrestab );
    printf( "Exe type:            %x\n", ne->ne_exetyp );
    printf( "Other flags:         %x\n", ne->ne_flagsothers );
    printf( "Fast load area:      %x-%x\n", ne->fastload_offset << ne->ne_align,
            (ne->fastload_offset+ne->fastload_length) << ne->ne_align );
    printf( "Expected version:    %d.%d\n", HIBYTE(ne->ne_expver), LOBYTE(ne->ne_expver) );
}

static const char *get_resource_type( WORD id )
{
    static char buffer[5];
    switch(id)
    {
    case NE_RSCTYPE_CURSOR: return "CURSOR";
    case NE_RSCTYPE_BITMAP: return "BITMAP";
    case NE_RSCTYPE_ICON: return "ICON";
    case NE_RSCTYPE_MENU: return "MENU";
    case NE_RSCTYPE_DIALOG: return "DIALOG";
    case NE_RSCTYPE_STRING: return "STRING";
    case NE_RSCTYPE_FONTDIR: return "FONTDIR";
    case NE_RSCTYPE_FONT: return "FONT";
    case NE_RSCTYPE_ACCELERATOR: return "ACCELERATOR";
    case NE_RSCTYPE_RCDATA: return "RCDATA";
    case NE_RSCTYPE_GROUP_CURSOR: return "CURSOR_GROUP";
    case NE_RSCTYPE_GROUP_ICON: return "ICON_GROUP";
    default:
        sprintf( buffer, "%04x", id );
        return buffer;
    }
}

static void dump_ne_resources( const void *base, const IMAGE_OS2_HEADER *ne )
{
    NE_NAMEINFO *name;
    const void *res_ptr = (char *)ne + ne->ne_rsrctab;
    WORD size_shift = *(WORD *)res_ptr;
    NE_TYPEINFO *info = (NE_TYPEINFO *)((WORD *)res_ptr + 1);
    int count;

    printf( "\nResources:\n" );
    while (info->type_id != 0 && (char *)info < (char *)ne + ne->ne_restab)
    {
        name = (NE_NAMEINFO *)(info + 1);
        for (count = info->count; count > 0; count--, name++)
        {
            if (name->id & 0x8000) printf( "  %d", (name->id & ~0x8000) );
            else printf( "  %.*s", *((unsigned char *)res_ptr + name->id),
                         (char *)res_ptr + name->id + 1 );
            if (info->type_id & 0x8000) printf( " %s\n", get_resource_type(info->type_id) );
            else printf( " %.*s\n", *((unsigned char *)res_ptr + info->type_id),
                         (char *)res_ptr + info->type_id + 1 );
            dump_data( (unsigned char *)base + (name->offset << size_shift),
                       name->length << size_shift, "    " );
        }
        info = (NE_TYPEINFO *)name;
    }
}

void ne_dump( const void *exe, size_t exe_size )
{
    const IMAGE_DOS_HEADER *dos = exe;
    const IMAGE_OS2_HEADER *ne = (IMAGE_OS2_HEADER *)((char *)dos + dos->e_lfanew);

    dump_ne_header( ne );
    dump_ne_resources( exe, ne );
}
