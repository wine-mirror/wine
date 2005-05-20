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

#include "config.h"
#include "wine/port.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winedump.h"

static inline WORD get_word( const BYTE *ptr )
{
    return ptr[0] | (ptr[1] << 8);
}

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
    printf( "Fast load area:      %x-%x\n", ne->ne_pretthunks << ne->ne_align,
            (ne->ne_pretthunks+ne->ne_psegrefbytes) << ne->ne_align );
    printf( "Expected version:    %d.%d\n", HIBYTE(ne->ne_expver), LOBYTE(ne->ne_expver) );
}

static void dump_ne_names( const void *base, const IMAGE_OS2_HEADER *ne )
{
    const char *pstr = (const char *)ne + ne->ne_restab;

    printf( "\nResident name table:\n" );
    while (*pstr)
    {
        printf( " %4d: %*.*s\n", get_word(pstr + *pstr + 1), *pstr, *pstr, pstr + 1 );
        pstr += *pstr + 1 + sizeof(WORD);
    }
    if (ne->ne_cbnrestab)
    {
        printf( "\nNon-resident name table:\n" );
        pstr = (char *)base + ne->ne_nrestab;
        while (*pstr)
        {
            printf( " %4d: %*.*s\n", get_word(pstr + *pstr + 1), *pstr, *pstr, pstr + 1 );
            pstr += *pstr + 1 + sizeof(WORD);
        }
    }
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
    const NE_NAMEINFO *name;
    const void *res_ptr = (const char *)ne + ne->ne_rsrctab;
    WORD size_shift = get_word(res_ptr);
    const NE_TYPEINFO *info = (const NE_TYPEINFO *)((const WORD *)res_ptr + 1);
    int count;

    printf( "\nResources:\n" );
    while (info->type_id != 0 && (const char *)info < (const char *)ne + ne->ne_restab)
    {
        name = (const NE_NAMEINFO *)(info + 1);
        for (count = info->count; count > 0; count--, name++)
        {
            if (name->id & 0x8000) printf( "  %d", (name->id & ~0x8000) );
            else printf( "  %.*s", *((const unsigned char *)res_ptr + name->id),
                         (const char *)res_ptr + name->id + 1 );
            if (info->type_id & 0x8000) printf( " %s", get_resource_type(info->type_id) );
            else printf( " %.*s", *((const unsigned char *)res_ptr + info->type_id),
                         (const char *)res_ptr + info->type_id + 1 );
            printf(" flags %04x length %04x\n", name->flags, name->length << size_shift);
            dump_data( (const unsigned char *)base + (name->offset << size_shift),
                       name->length << size_shift, "    " );
        }
        info = (const NE_TYPEINFO *)name;
    }
}

static const char *get_export_name( const void *base, const IMAGE_OS2_HEADER *ne, int ordinal )
{
    static char name[256];
    BYTE *pstr;
    int pass = 0;

    /* search the resident names */

    while (pass < 2)
    {
        if (pass == 0)  /* resident names */
        {
            pstr = (BYTE *)ne + ne->ne_restab;
            if (*pstr) pstr += *pstr + 1 + sizeof(WORD);  /* skip first entry (module name) */
        }
        else  /* non-resident names */
        {
            if (!ne->ne_cbnrestab) break;
            pstr = (BYTE *)base + ne->ne_nrestab;
        }
        while (*pstr)
        {
            WORD ord = get_word(pstr + *pstr + 1);
            if (ord == ordinal)
            {
                memcpy( name, pstr + 1, *pstr );
                name[*pstr] = 0;
                return name;
            }
            pstr += *pstr + 1 + sizeof(WORD);
        }
        pass++;
    }
    name[0] = 0;
    return name;
}

static void dump_ne_exports( const void *base, const IMAGE_OS2_HEADER *ne )
{
    BYTE *ptr = (BYTE *)ne + ne->ne_enttab;
    BYTE *end = ptr + ne->ne_cbenttab;
    int i, ordinal = 1;

    if (!ne->ne_cbenttab || !*ptr) return;

    printf( "\nExported entry points:\n" );

    while (ptr < end && *ptr)
    {
        BYTE count = *ptr++;
        BYTE type = *ptr++;
        switch(type)
        {
        case 0:  /* next bundle */
            ordinal += count;
            break;
        case 0xff:  /* movable */
            for (i = 0; i < count; i++)
            {
                printf( " %4d MOVABLE %d:%04x %s\n",
                        ordinal + i, ptr[3], get_word(ptr + 4),
                        get_export_name( base, ne, ordinal + i ) );
                ptr += 6;
            }
            ordinal += count;
            break;
        case 0xfe:  /* constant */
            for (i = 0; i < count; i++)
            {
                printf( " %4d CONST     %04x %s\n",
                        ordinal + i, get_word(ptr + 1),
                        get_export_name( base, ne, ordinal + i ) );
                ptr += 3;
            }
            ordinal += count;
            break;
        default:  /* fixed */
            for (i = 0; i < count; i++)
            {
                printf( " %4d FIXED   %d:%04x %s\n",
                        ordinal + i, type, get_word(ptr + 1),
                        get_export_name( base, ne, ordinal + i ) );
                ptr += 3;
            }
            ordinal += count;
            break;
        }
    }
}

void ne_dump( const void *exe, size_t exe_size )
{
    const IMAGE_DOS_HEADER *dos = exe;
    const IMAGE_OS2_HEADER *ne = (const IMAGE_OS2_HEADER *)((const char *)dos + dos->e_lfanew);

    dump_ne_header( ne );
    dump_ne_names( exe, ne );
    dump_ne_resources( exe, ne );
    dump_ne_exports( exe, ne );
}
