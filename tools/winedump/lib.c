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

/* FIXME: Add support for import library with the long format described in
 * Microsoft Portable Executable and Common Object File Format Specification.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "winedump.h"

static void dump_import_object(const IMPORT_OBJECT_HEADER *ioh)
{
    if (ioh->Version >= 1)
    {
#if 0 /* FIXME: supposed to handle uuid.lib but it doesn't */
        const ANON_OBJECT_HEADER *aoh = (const ANON_OBJECT_HEADER *)ioh;

        printf("CLSID {%08x-%04x-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}",
               aoh->ClassID.Data1, aoh->ClassID.Data2, aoh->ClassID.Data3,
               aoh->ClassID.Data4[0], aoh->ClassID.Data4[1], aoh->ClassID.Data4[2], aoh->ClassID.Data4[3],
               aoh->ClassID.Data4[4], aoh->ClassID.Data4[5], aoh->ClassID.Data4[6], aoh->ClassID.Data4[7]);
#endif
    }
    else
    {
        static const char * const obj_type[] = { "code", "data", "const" };
        static const char * const name_type[] = { "ordinal", "name", "no prefix", "undecorate" };
        const char *name;

        printf("  Version      : %X\n", ioh->Version);
        printf("  Machine      : %X (%s)\n", ioh->Machine, get_machine_str(ioh->Machine));
        printf("  TimeDateStamp: %08X %s\n", ioh->TimeDateStamp, get_time_str(ioh->TimeDateStamp));
        printf("  SizeOfData   : %08X\n", ioh->SizeOfData);
        name = (const char *)ioh + sizeof(*ioh);
        printf("  DLL name     : %s\n", name + strlen(name) + 1);
        printf("  Symbol name  : %s\n", name);
        printf("  Type         : %s\n", (ioh->Type < sizeof(obj_type)/sizeof(obj_type[0])) ? obj_type[ioh->Type] : "unknown");
        printf("  Name type    : %s\n", (ioh->NameType < sizeof(name_type)/sizeof(name_type[0])) ? name_type[ioh->NameType] : "unknown");
        printf("  %-13s: %u\n", (ioh->NameType == IMPORT_OBJECT_ORDINAL) ? "Ordinal" : "Hint", ioh->u.Ordinal);
        printf("\n");
    }
}

void lib_dump(const char *lib_base, unsigned long lib_size)
{
    long cur_file_pos;
    IMAGE_ARCHIVE_MEMBER_HEADER *iamh;

    if (strncmp(lib_base, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE))
    {
        printf("Not a valid COFF library file");
        return;
    }

    iamh = (IMAGE_ARCHIVE_MEMBER_HEADER *)(lib_base + IMAGE_ARCHIVE_START_SIZE);
    cur_file_pos = IMAGE_ARCHIVE_START_SIZE;

    while (cur_file_pos < lib_size)
    {
        IMPORT_OBJECT_HEADER *ioh;
        long size;

#if 0 /* left here for debugging purposes, also should be helpful for
       * adding support for new library formats.
       */
        printf("cur_file_pos %08lx\n", (ULONG_PTR)iamh - (ULONG_PTR)lib_base);

        printf("Name %.16s", iamh->Name);
        if (!strncmp(iamh->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(iamh->Name)))
        {
            printf(" - %s archive linker member\n",
                   cur_file_pos == IMAGE_ARCHIVE_START_SIZE ? "1st" : "2nd");
        }
        else
            printf("\n");
        printf("Date %.12s\n", iamh->Date);
        printf("UserID %.6s\n", iamh->UserID);
        printf("GroupID %.6s\n", iamh->GroupID);
        printf("Mode %.8s\n", iamh->Mode);
        printf("Size %.10s\n", iamh->Size);
#endif
        /* FIXME: only import library contents with the short format are
         * recognized.
         */
        ioh = (IMPORT_OBJECT_HEADER *)((char *)iamh + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
        if (ioh->Sig1 == IMAGE_FILE_MACHINE_UNKNOWN && ioh->Sig2 == IMPORT_OBJECT_HDR_SIG2)
        {
            dump_import_object(ioh);
        }

        size = strtoul((const char *)iamh->Size, NULL, 10);
        size = (size + 1) & ~1; /* align to an even address */

        cur_file_pos += size + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER);
        iamh = (IMAGE_ARCHIVE_MEMBER_HEADER *)((char *)iamh + size + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    }
}
