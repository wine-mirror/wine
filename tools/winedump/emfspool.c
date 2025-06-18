/*
 *  Dump an EMF Spool File
 *
 *  Copyright 2022 Piotr Caban for CodeWeavers
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
#include "winedump.h"

#define EMFSPOOL_VERSION 0x10000

typedef enum
{
    EMRI_METAFILE = 1,
    EMRI_ENGINE_FONT,
    EMRI_DEVMODE,
    EMRI_TYPE1_FONT,
    EMRI_PRESTARTPAGE,
    EMRI_DESIGNVECTOR,
    EMRI_SUBSET_FONT,
    EMRI_DELTA_FONT,
    EMRI_FORM_METAFILE,
    EMRI_BW_METAFILE,
    EMRI_BW_FORM_METAFILE,
    EMRI_METAFILE_DATA,
    EMRI_METAFILE_EXT,
    EMRI_BW_METAFILE_EXT,
    EMRI_ENGINE_FONT_EXT,
    EMRI_TYPE1_FONT_EXT,
    EMRI_DESIGNVECTOR_EXT,
    EMRI_SUBSET_FONT_EXT,
    EMRI_DELTA_FONT_EXT,
    EMRI_PS_JOB_DATA,
    EMRI_EMBED_FONT_EXT,
} record_type;

typedef struct
{
    unsigned int dwVersion;
    unsigned int cjSize;
    unsigned int dpszDocName;
    unsigned int dpszOutput;
} header;

typedef struct
{
    unsigned int ulID;
    unsigned int cjSize;
} record_hdr;

static inline void print_longlong(ULONGLONG value)
{
    if (sizeof(value) > sizeof(unsigned long) && value >> 32)
        printf("0x%lx%08lx", (unsigned long)(value >> 32), (unsigned long)value);
    else
        printf("0x%lx", (unsigned long)value);
}

static const WCHAR* read_wstr(unsigned long off)
{
    const WCHAR *beg, *end;

    if (!off)
        return NULL;

    beg = end = PRD(off, sizeof(WCHAR));
    off += sizeof(WCHAR);
    if (!beg)
        fatal("can't read Unicode string, corrupted file\n");

    while (*end)
    {
        end = PRD(off, sizeof(WCHAR));
        off += sizeof(WCHAR);
        if (!end)
            fatal("can't read Unicode string, corrupted file\n");
    }
    return beg;
}

#define EMRICASE(x) case x: printf("%-24s %08x\n", #x, hdr->cjSize); break
static unsigned long dump_emfspool_record(unsigned long off)
{
    const record_hdr *hdr;

    hdr = PRD(off, sizeof(*hdr));
    if (!hdr)
        return 0;

    switch (hdr->ulID)
    {
    EMRICASE(EMRI_METAFILE);
    EMRICASE(EMRI_ENGINE_FONT);
    EMRICASE(EMRI_DEVMODE);
    EMRICASE(EMRI_TYPE1_FONT);
    EMRICASE(EMRI_PRESTARTPAGE);
    EMRICASE(EMRI_DESIGNVECTOR);
    EMRICASE(EMRI_SUBSET_FONT);
    EMRICASE(EMRI_DELTA_FONT);
    EMRICASE(EMRI_FORM_METAFILE);
    EMRICASE(EMRI_BW_METAFILE);
    EMRICASE(EMRI_BW_FORM_METAFILE);
    EMRICASE(EMRI_METAFILE_DATA);
    EMRICASE(EMRI_METAFILE_EXT);
    EMRICASE(EMRI_BW_METAFILE_EXT);
    EMRICASE(EMRI_ENGINE_FONT_EXT);
    EMRICASE(EMRI_TYPE1_FONT_EXT);
    EMRICASE(EMRI_DESIGNVECTOR_EXT);
    EMRICASE(EMRI_SUBSET_FONT_EXT);
    EMRICASE(EMRI_DELTA_FONT_EXT);
    EMRICASE(EMRI_PS_JOB_DATA);
    EMRICASE(EMRI_EMBED_FONT_EXT);
    default:
        printf("%u %08x\n", hdr->ulID, hdr->cjSize);
        break;
    }

    switch (hdr->ulID)
    {
    case EMRI_METAFILE:
    case EMRI_FORM_METAFILE:
    case EMRI_BW_METAFILE:
    case EMRI_BW_FORM_METAFILE:
    case EMRI_METAFILE_DATA:
    {
        unsigned long emf_off = off + sizeof(*hdr);
        while ((emf_off = dump_emfrecord("    ", emf_off)) && emf_off < off + sizeof(*hdr) + hdr->cjSize);
        break;
    }

    case EMRI_METAFILE_EXT:
    case EMRI_BW_METAFILE_EXT:
    {
        const ULONGLONG *emf_off = PRD(off + sizeof(*hdr), sizeof(*emf_off));
        if (!emf_off)
            fatal("truncated file\n");
        printf("  %-20s ", "offset");
        print_longlong(*emf_off);
        printf(" (absolute position ");
        print_longlong(off - *emf_off);
        printf(")\n");
        break;
    }

    default:
        dump_data((const unsigned char *)(hdr + 1), hdr->cjSize, "");
        break;
    }

    return off + sizeof(*hdr) + hdr->cjSize;
}

enum FileSig get_kind_emfspool(void)
{
    const header *hdr;

    hdr = PRD(0, sizeof(*hdr));
    if (hdr && hdr->dwVersion == EMFSPOOL_VERSION)
        return SIG_EMFSPOOL;
    return SIG_UNKNOWN;
}

void emfspool_dump(void)
{
    const WCHAR *doc_name, *output;
    unsigned long off;
    const header *hdr;

    hdr = PRD(0, sizeof(*hdr));
    if(!hdr)
        return;
    doc_name = read_wstr(hdr->dpszDocName);
    output = read_wstr(hdr->dpszOutput);

    printf("File Header\n");
    printf("  %-20s %#x\n", "dwVersion:", hdr->dwVersion);
    printf("  %-20s %#x\n", "cjSize:", hdr->cjSize);
    printf("  %-20s %#x %s\n", "dpszDocName:", hdr->dpszDocName, get_unicode_str(doc_name, -1));
    printf("  %-20s %#x %s\n", "dpszOutput:", hdr->dpszOutput, get_unicode_str(output, -1));
    printf("\n");

    off = hdr->cjSize;
    while ((off = dump_emfspool_record(off)));
}
