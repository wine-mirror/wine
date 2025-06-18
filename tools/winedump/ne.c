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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winedump.h"

struct ne_segtable_entry
{
    WORD seg_data_offset;   /* Sector offset of segment data	*/
    WORD seg_data_length;   /* Length of segment data		*/
    WORD seg_flags;         /* Flags associated with this segment	*/
    WORD min_alloc;         /* Minimum allocation size for this	*/
};

struct relocation_entry
{
    BYTE address_type;    /* Relocation address type */
    BYTE relocation_type; /* Relocation type */
    WORD offset;          /* Offset in segment to fixup */
    WORD target1;         /* Target specification */
    WORD target2;         /* Target specification */
};

typedef struct
{
    WORD  offset;
    WORD  length;
    WORD  flags;
    WORD  id;
    WORD  handle;
    WORD  usage;
} NE_NAMEINFO;

typedef struct
{
    WORD  type_id;
    WORD  count;
    DWORD resloader;
} NE_TYPEINFO;

#define NE_RADDR_LOWBYTE      0
#define NE_RADDR_SELECTOR     2
#define NE_RADDR_POINTER32    3
#define NE_RADDR_OFFSET16     5
#define NE_RADDR_POINTER48    11
#define NE_RADDR_OFFSET32     13

#define NE_RELTYPE_INTERNAL  0
#define NE_RELTYPE_ORDINAL   1
#define NE_RELTYPE_NAME      2
#define NE_RELTYPE_OSFIXUP   3
#define NE_RELFLAG_ADDITIVE  4

#define NE_SEGFLAGS_DATA        0x0001
#define NE_SEGFLAGS_ALLOCATED   0x0002
#define NE_SEGFLAGS_LOADED      0x0004
#define NE_SEGFLAGS_ITERATED    0x0008
#define NE_SEGFLAGS_MOVEABLE    0x0010
#define NE_SEGFLAGS_SHAREABLE   0x0020
#define NE_SEGFLAGS_PRELOAD     0x0040
#define NE_SEGFLAGS_EXECUTEONLY 0x0080
#define NE_SEGFLAGS_READONLY    0x0080
#define NE_SEGFLAGS_RELOC_DATA  0x0100
#define NE_SEGFLAGS_SELFLOAD    0x0800
#define NE_SEGFLAGS_DISCARDABLE 0x1000
#define NE_SEGFLAGS_32BIT       0x2000

#define NE_RSCTYPE_CURSOR             0x8001
#define NE_RSCTYPE_BITMAP             0x8002
#define NE_RSCTYPE_ICON               0x8003
#define NE_RSCTYPE_MENU               0x8004
#define NE_RSCTYPE_DIALOG             0x8005
#define NE_RSCTYPE_STRING             0x8006
#define NE_RSCTYPE_FONTDIR            0x8007
#define NE_RSCTYPE_FONT               0x8008
#define NE_RSCTYPE_ACCELERATOR        0x8009
#define NE_RSCTYPE_RCDATA             0x800a
#define NE_RSCTYPE_GROUP_CURSOR       0x800c
#define NE_RSCTYPE_GROUP_ICON         0x800e
#define NE_RSCTYPE_SCALABLE_FONTPATH  0x80cc

static inline WORD get_word( const BYTE *ptr )
{
    return ptr[0] | (ptr[1] << 8);
}

static void dump_ne_header( const IMAGE_OS2_HEADER *ne )
{
    printf( "File header:\n" );
    printf( "Linker version:      %d.%d\n", ne->ne_ver, ne->ne_rev );
    printf( "Entry table:         %x len %d\n", ne->ne_enttab, ne->ne_cbenttab );
    printf( "Checksum:            %08x\n", (UINT)ne->ne_crc );
    printf( "Flags:               %04x\n", ne->ne_flags );
    printf( "Auto data segment:   %x\n", ne->ne_autodata );
    printf( "Heap size:           %d bytes\n", ne->ne_heap );
    printf( "Stack size:          %d bytes\n", ne->ne_stack );
    printf( "Stack pointer:       %x:%04x\n", HIWORD(ne->ne_sssp), LOWORD(ne->ne_sssp) );
    printf( "Entry point:         %x:%04x\n", HIWORD(ne->ne_csip), LOWORD(ne->ne_csip) );
    printf( "Number of segments:  %d\n", ne->ne_cseg );
    printf( "Number of modrefs:   %d\n", ne->ne_cmod );
    printf( "Segment table:       %x\n", ne->ne_segtab );
    printf( "Resource table:      %x\n", ne->ne_rsrctab );
    printf( "Resident name table: %x\n", ne->ne_restab );
    printf( "Module table:        %x\n", ne->ne_modtab );
    printf( "Import table:        %x\n", ne->ne_imptab );
    printf( "Non-resident table:  %x\n", (UINT)ne->ne_nrestab );
    printf( "Exe type:            %x\n", ne->ne_exetyp );
    printf( "Other flags:         %x\n", ne->ne_flagsothers );
    printf( "Fast load area:      %x-%x\n", ne->ne_pretthunks << ne->ne_align,
            (ne->ne_pretthunks+ne->ne_psegrefbytes) << ne->ne_align );
    printf( "Expected version:    %d.%d\n", HIBYTE(ne->ne_expver), LOBYTE(ne->ne_expver) );
}

static void dump_ne_names( const IMAGE_OS2_HEADER *ne )
{
    const unsigned char *pstr = (const unsigned char *)ne + ne->ne_restab;

    printf( "\nResident name table:\n" );
    while (*pstr)
    {
        printf( " %4d: %*.*s\n", get_word(pstr + *pstr + 1), *pstr, *pstr, pstr + 1 );
        pstr += *pstr + 1 + sizeof(WORD);
    }
    if (ne->ne_cbnrestab)
    {
        unsigned int pos = ne->ne_nrestab;
        printf( "\nNon-resident name table:\n" );
        while ((pstr = PRD(pos, 0)) && *pstr)
        {
            printf( " %4d: %*.*s\n", get_word(pstr + *pstr + 1), *pstr, *pstr, pstr + 1 );
            pos += *pstr + 1 + sizeof(WORD);
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

static void dump_ne_resources( const IMAGE_OS2_HEADER *ne )
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
            dump_data( PRD(name->offset << size_shift, name->length << size_shift),
                       name->length << size_shift, "    " );
        }
        info = (const NE_TYPEINFO *)name;
    }
}

static const char *get_export_name( const IMAGE_OS2_HEADER *ne, int ordinal )
{
    static char name[256];
    const BYTE *pstr;
    int pass = 0;

    /* search the resident names */

    while (pass < 2)
    {
        if (pass == 0)  /* resident names */
        {
            pstr = (const BYTE *)ne + ne->ne_restab;
            if (*pstr) pstr += *pstr + 1 + sizeof(WORD);  /* skip first entry (module name) */
        }
        else  /* non-resident names */
        {
            if (!ne->ne_cbnrestab) break;
            pstr = PRD(ne->ne_nrestab, 0);
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

static void dump_ne_exports( const IMAGE_OS2_HEADER *ne )
{
    const BYTE *ptr = (const BYTE *)ne + ne->ne_enttab;
    const BYTE *end = ptr + ne->ne_cbenttab;
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
                        get_export_name( ne, ordinal + i ) );
                ptr += 6;
            }
            ordinal += count;
            break;
        case 0xfe:  /* constant */
            for (i = 0; i < count; i++)
            {
                printf( " %4d CONST     %04x %s\n",
                        ordinal + i, get_word(ptr + 1),
                        get_export_name( ne, ordinal + i ) );
                ptr += 3;
            }
            ordinal += count;
            break;
        default:  /* fixed */
            for (i = 0; i < count; i++)
            {
                printf( " %4d FIXED   %d:%04x %s\n",
                        ordinal + i, type, get_word(ptr + 1),
                        get_export_name( ne, ordinal + i ) );
                ptr += 3;
            }
            ordinal += count;
            break;
        }
    }
}

static const char *get_reloc_name( BYTE addr_type, int additive )
{
    switch(addr_type & 0x7f)
    {
    case NE_RADDR_LOWBYTE:   return additive ? "byte add" : "byte";
    case NE_RADDR_OFFSET16:  return additive ? "off16 add" : "off16";
    case NE_RADDR_POINTER32: return additive ? "ptr32 add" : "ptr32";
    case NE_RADDR_SELECTOR:  return additive ? "sel add" : "sel";
    case NE_RADDR_POINTER48: return additive ? "ptr48 add" : "ptr48";
    case NE_RADDR_OFFSET32:  return additive ? "off32 add" : "off32";
    }
    return "???";
}

static const char *get_seg_flags( WORD flags )
{
    static char buffer[256];

    buffer[0] = 0;
#define ADD_FLAG(x) if (flags & NE_SEGFLAGS_##x) strcat( buffer, " " #x );
    ADD_FLAG(DATA);
    ADD_FLAG(ALLOCATED);
    ADD_FLAG(LOADED);
    ADD_FLAG(ITERATED);
    ADD_FLAG(MOVEABLE);
    ADD_FLAG(SHAREABLE);
    ADD_FLAG(PRELOAD);
    ADD_FLAG(EXECUTEONLY);
    ADD_FLAG(READONLY);
    ADD_FLAG(RELOC_DATA);
    ADD_FLAG(SELFLOAD);
    ADD_FLAG(DISCARDABLE);
    ADD_FLAG(32BIT);
#undef ADD_FLAG
    if (buffer[0])
    {
        buffer[0] = '(';
        strcat( buffer, ")" );
    }
    return buffer;
}

static void dump_relocations( const IMAGE_OS2_HEADER *ne, WORD count,
                              const struct relocation_entry *rep )
{
    const WORD *modref = (const WORD *)((const BYTE *)ne + ne->ne_modtab);
    const BYTE *mod_name, *func_name;
    WORD i;

    for (i = 0; i < count; i++, rep++)
    {
        int additive = rep->relocation_type & NE_RELFLAG_ADDITIVE;
        switch (rep->relocation_type & 3)
        {
        case NE_RELTYPE_ORDINAL:
            mod_name = (const BYTE *)ne + ne->ne_imptab + modref[rep->target1 - 1];
            printf( "%6d: %s = %*.*s.%d\n", i + 1, get_reloc_name( rep->address_type, additive ),
                    *mod_name, *mod_name, mod_name + 1, rep->target2 );
            break;
        case NE_RELTYPE_NAME:
            mod_name = (const BYTE *)ne + ne->ne_imptab + modref[rep->target1 - 1];
            func_name = (const BYTE *)ne + ne->ne_imptab + rep->target2;
            printf( "%6d: %s = %*.*s.%*.*s\n", i + 1, get_reloc_name( rep->address_type, additive ),
                    *mod_name, *mod_name, mod_name + 1,
                    *func_name, *func_name, func_name + 1 );
            break;
        case NE_RELTYPE_INTERNAL:
            if ((rep->target1 & 0xff) == 0xff)
            {
                /* the module itself */
                mod_name = (const BYTE *)ne + ne->ne_restab;
                printf( "%6d: %s = %*.*s.%d\n", i + 1, get_reloc_name( rep->address_type, additive ),
                        *mod_name, *mod_name, mod_name + 1, rep->target2 );
            }
            else
                printf( "%6d: %s = %d:%04x\n", i + 1, get_reloc_name( rep->address_type, additive ),
                        rep->target1, rep->target2 );
            break;
        case NE_RELTYPE_OSFIXUP:
            /* Relocation type 7:
             *
             *    These appear to be used as fixups for the Windows
             * floating point emulator.  Let's just ignore them and
             * try to use the hardware floating point.  Linux should
             * successfully emulate the coprocessor if it doesn't
             * exist.
             */
            printf( "%6d: %s = TYPE %d, OFFSET %04x, TARGET %04x %04x\n",
                    i + 1, get_reloc_name( rep->address_type, additive ),
                    rep->relocation_type, rep->offset,
                    rep->target1, rep->target2 );
            break;
        }
    }
}

static void dump_ne_segment( const IMAGE_OS2_HEADER *ne, int segnum )
{
    const struct ne_segtable_entry *table = (const struct ne_segtable_entry *)((const BYTE *)ne + ne->ne_segtab);
    const struct ne_segtable_entry *seg = table + segnum - 1;

    printf( "\nSegment %d:\n", segnum );
    printf( "  File offset: %08x\n", seg->seg_data_offset << ne->ne_align );
    printf( "  Length:      %08x\n", seg->seg_data_length );
    printf( "  Flags:       %08x %s\n", seg->seg_flags, get_seg_flags(seg->seg_flags) );
    printf( "  Alloc size:  %08x\n", seg->min_alloc );
    if (seg->seg_flags & NE_SEGFLAGS_RELOC_DATA)
    {
        const BYTE *ptr = PRD((seg->seg_data_offset << ne->ne_align) + seg->seg_data_length, 0);
        WORD count = get_word(ptr);
        ptr += sizeof(WORD);
        printf( "  Relocations:\n" );
        dump_relocations( ne, count, (const struct relocation_entry *)ptr );
    }
}

void ne_dump( void )
{
    unsigned int i;
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_OS2_HEADER *ne;

    dos = PRD(0, sizeof(*dos));
    if (!dos) return;
    ne = PRD(dos->e_lfanew, sizeof(*ne));
    print_fake_dll();

    if (globals.do_dumpheader || !globals.dumpsect)
        dump_ne_header( ne );
    if (globals.do_dumpheader)
        dump_ne_names( ne );

    if (globals_dump_sect("resource"))
        dump_ne_resources( ne );
    if (globals_dump_sect("export"))
        dump_ne_exports( ne );
    if (globals.do_dumpheader)
        for (i = 1; i <= ne->ne_cseg; i++) dump_ne_segment( ne, i );
}
