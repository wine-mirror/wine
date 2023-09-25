/*
 *  Dump a COFF library (lib) file
 *
 * Copyright 2006 Dmitry Timoshkov
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
#include <stdlib.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "winedump.h"

static inline USHORT ushort_bswap(USHORT s)
{
    return (s >> 8) | (s << 8);
}

static inline UINT ulong_bswap(UINT l)
{
    return ((UINT)ushort_bswap((USHORT)l) << 16) | ushort_bswap(l >> 16);
}

static void dump_import_object(const IMPORT_OBJECT_HEADER *ioh)
{
    if (ioh->Version == 0)
    {
        static const char * const obj_type[] = { "code", "data", "const" };
        static const char * const name_type[] = { "ordinal", "name", "no prefix", "undecorate", "export as" };
        const char *name, *dll_name;

        printf("  Version      : %X\n", ioh->Version);
        printf("  Machine      : %X (%s)\n", ioh->Machine, get_machine_str(ioh->Machine));
        printf("  TimeDateStamp: %08X %s\n", (UINT)ioh->TimeDateStamp, get_time_str(ioh->TimeDateStamp));
        printf("  SizeOfData   : %08X\n", (UINT)ioh->SizeOfData);
        name = (const char *)ioh + sizeof(*ioh);
        dll_name = name + strlen(name) + 1;
        printf("  DLL name     : %s\n", dll_name);
        printf("  Symbol name  : %s\n", name);
        printf("  Type         : %s\n", (ioh->Type < ARRAY_SIZE(obj_type)) ? obj_type[ioh->Type] : "unknown");
        printf("  Name type    : %s\n", (ioh->NameType < ARRAY_SIZE(name_type)) ? name_type[ioh->NameType] : "unknown");
        if (ioh->NameType == IMPORT_OBJECT_NAME_EXPORTAS)
            printf("  Export name  : %s\n", dll_name + strlen(dll_name) + 1);
        printf("  %-13s: %u\n", (ioh->NameType == IMPORT_OBJECT_ORDINAL) ? "Ordinal" : "Hint", ioh->Ordinal);
        printf("\n");
    }
}

static void dump_long_import(const void *base, const IMAGE_SECTION_HEADER *ish, unsigned num_sect)
{
    unsigned i;
    const DWORD *imp_data5 = NULL;
    const WORD *imp_data6 = NULL;

    if (globals.do_dumpheader)
        printf("Section Table\n");

    for (i = 0; i < num_sect; i++)
    {
        if (globals.do_dumpheader)
            dump_section(&ish[i], NULL);

        if (globals.do_dump_rawdata)
        {
            dump_data((const unsigned char *)base + ish[i].PointerToRawData, ish[i].SizeOfRawData, "    " );
            printf("\n");
        }

        if (!strcmp((const char *)ish[i].Name, ".idata$5"))
        {
            imp_data5 = (const DWORD *)((const char *)base + ish[i].PointerToRawData);
        }
        else if (!strcmp((const char *)ish[i].Name, ".idata$6"))
        {
            imp_data6 = (const WORD *)((const char *)base + ish[i].PointerToRawData);
        }
        else if (globals.do_debug && !strcmp((const char *)ish[i].Name, ".debug$S"))
        {
            const char *imp_debugS = (const char *)base + ish[i].PointerToRawData;

            codeview_dump_symbols(imp_debugS, 0, ish[i].SizeOfRawData);
            printf("\n");
        }
    }

    if (imp_data5)
    {
        WORD ordinal = 0;
        const char *name = NULL;

        if (imp_data5[0] & 0x80000000)
            ordinal = (WORD)(imp_data5[0] & ~0x80000000);

        if (imp_data6)
        {
            if (!ordinal) ordinal = imp_data6[0];
            name = (const char *)(imp_data6 + 1);
        }
        else
        {
            /* FIXME: find out a name in the section's data */
        }

        if (ordinal)
        {
            printf("  Symbol name  : %s\n", name ? name : "(ordinal import) /* FIXME */");
            printf("  %-13s: %u\n", (imp_data5[0] & 0x80000000) ? "Ordinal" : "Hint", ordinal);
            printf("\n");
        }
    }
}

enum FileSig get_kind_lib(void)
{
    const char*         arch = PRD(0, IMAGE_ARCHIVE_START_SIZE);
    if (arch && !strncmp(arch, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE))
        return SIG_COFFLIB;
    return SIG_UNKNOWN;
}

void lib_dump(void)
{
    BOOL first_linker_member = TRUE;
    unsigned long cur_file_pos, long_names_size = 0;
    const IMAGE_ARCHIVE_MEMBER_HEADER *iamh;
    const char *long_names = NULL;

    cur_file_pos = IMAGE_ARCHIVE_START_SIZE;

    for (;;)
    {
        const IMPORT_OBJECT_HEADER *ioh;
        unsigned long size;

        if (!(iamh = PRD(cur_file_pos, sizeof(*iamh)))) break;

        if (globals.do_dumpheader)
        {
            printf("Archive member name at %08lx\n", Offset(iamh));

            printf("Name %.16s", iamh->Name);
            if (!strncmp((const char *)iamh->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(iamh->Name)))
            {
                printf(" - %s archive linker member\n", first_linker_member ? "1st" : "2nd");
            }
            else
            {
                if (long_names && iamh->Name[0] == '/')
                {
                    unsigned long long_names_offset = atol((const char *)&iamh->Name[1]);
                    if (long_names_offset < long_names_size)
                        printf("%s\n", long_names + long_names_offset);
                }
                printf("\n");
            }
            printf("Date %.12s %s\n", iamh->Date, get_time_str(strtoul((const char *)iamh->Date, NULL, 10)));
            printf("UserID %.6s\n", iamh->UserID);
            printf("GroupID %.6s\n", iamh->GroupID);
            printf("Mode %.8s\n", iamh->Mode);
            printf("Size %.10s\n\n", iamh->Size);
        }

        cur_file_pos += sizeof(IMAGE_ARCHIVE_MEMBER_HEADER);

        size = strtoul((const char *)iamh->Size, NULL, 10);
        size = (size + 1) & ~1; /* align to an even address */

        if (!(ioh = PRD(cur_file_pos, sizeof(*ioh)))) break;

        if (ioh->Sig1 == IMAGE_FILE_MACHINE_UNKNOWN && ioh->Sig2 == IMPORT_OBJECT_HDR_SIG2)
        {
            dump_import_object(ioh);
        }
        else if (!strncmp((const char *)iamh->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(iamh->Name)))
        {
            const UINT *offset = (const UINT *)ioh;
            const char *name;
            UINT i, count;

            if (first_linker_member) /* 1st archive linker member, BE format */
            {
                count = ulong_bswap(*offset++);
                name = (const char *)(offset + count);
                printf("%u public symbols\n", count);
                for (i = 0; i < count; i++)
                {
                    printf("%8x %s\n", ulong_bswap(offset[i]), name);
                    name += strlen(name) + 1;
                }
                printf("\n");
            }
            else /* 2nd archive linker member, LE format */
            {
                const WORD *idx;

                count = *offset++;
                printf("%u offsets\n", count);
                for (i = 0; i < count; i++)
                {
                    printf("%8x %8x\n", i, offset[i]);
                }
                printf("\n");

                offset += count;
                count = *offset++;
                idx = (const WORD *)offset;
                name = (const char *)(idx + count);
                printf("%u public symbols\n", count);
                for (i = 0; i < count; i++)
                {
                    printf("%8x %s\n", idx[i], name);
                    name += strlen(name) + 1;
                }
                printf("\n");
            }
        }
        else if (!strncmp((const char *)iamh->Name, "/<ECSYMBOLS>/   ", sizeof(iamh->Name)))
        {
            unsigned int i, *count = (unsigned int *)ioh;
            unsigned short *offsets = (unsigned short *)(count + 1);
            const char *name = (const char *)(offsets + *count);

            printf("%u EC symbols\n", *count);
            for (i = 0; i < *count; i++)
            {
                printf("%8x %s\n", offsets[i], name);
                name += strlen(name) + 1;
            }
            printf("\n");
        }
        else if (!strncmp((const char *)iamh->Name, IMAGE_ARCHIVE_LONGNAMES_MEMBER, sizeof(iamh->Name)))
        {
            long_names = PRD(cur_file_pos, size);
            long_names_size = size;
        }
        else
        {
            unsigned long expected_size;
            const IMAGE_FILE_HEADER *fh = (const IMAGE_FILE_HEADER *)ioh;

            if (globals.do_dumpheader)
            {
                dump_file_header(fh, FALSE);
                if (fh->SizeOfOptionalHeader)
                {
                    const IMAGE_OPTIONAL_HEADER32 *oh = (const IMAGE_OPTIONAL_HEADER32 *)((const char *)fh + sizeof(*fh));
                    dump_optional_header(oh);
                }
            }
            /* Sanity check */
            expected_size = sizeof(*fh) + fh->SizeOfOptionalHeader + fh->NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
            if (size > expected_size)
                dump_long_import(fh, (const IMAGE_SECTION_HEADER *)((const char *)fh + sizeof(*fh) + fh->SizeOfOptionalHeader), fh->NumberOfSections);
        }

        first_linker_member = FALSE;
        cur_file_pos += size;
    }
}
