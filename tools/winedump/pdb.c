/*
 *	PDB dumping utility
 *
 * 	Copyright 2006 Eric Pouech
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winedump.h"

struct pdb_reader
{
    union
    {
        struct
        {
            const struct PDB_JG_HEADER* header;
            const struct PDB_JG_TOC*    toc;
            const struct PDB_JG_ROOT*   root;
        } jg;
        struct
        {
            const struct PDB_DS_HEADER* header;
            const struct PDB_DS_TOC*    toc;
            const struct PDB_DS_ROOT*   root;
        } ds;
    } u;
    void*       (*read_stream)(struct pdb_reader*, DWORD);
    DWORD       stream_used[1024];
    PDB_STRING_TABLE* global_string_table;
};

static inline BOOL has_stream_been_read(struct pdb_reader* reader, unsigned stream_nr)
{
    return reader->stream_used[stream_nr / 32] & (1 << (stream_nr % 32));
}

static inline void mark_stream_been_read(struct pdb_reader* reader, unsigned stream_nr)
{
    reader->stream_used[stream_nr / 32] |= 1 << (stream_nr % 32);
}

static inline void clear_stream_been_read(struct pdb_reader* reader, unsigned stream_nr)
{
    reader->stream_used[stream_nr / 32] &= ~(1 << (stream_nr % 32));
}

static void* pdb_jg_read(const struct PDB_JG_HEADER* pdb, const WORD* block_list, int size)
{
    int                 i, nBlocks;
    BYTE*               buffer;

    if (!size) return NULL;

    nBlocks = (size + pdb->block_size - 1) / pdb->block_size;
    buffer = xmalloc(nBlocks * pdb->block_size);

    for (i = 0; i < nBlocks; i++)
        memcpy(buffer + i * pdb->block_size,
               (const char*)pdb + block_list[i] * pdb->block_size, pdb->block_size);

    return buffer;
}

static void* pdb_jg_read_stream(struct pdb_reader* reader, DWORD stream_nr)
{
    const WORD*         block_list;
    DWORD               i;

    if (!reader->u.jg.toc || stream_nr >= reader->u.jg.toc->num_streams) return NULL;

    mark_stream_been_read(reader, stream_nr);
    if (reader->u.jg.toc->streams[stream_nr].size == 0 ||
        reader->u.jg.toc->streams[stream_nr].size == 0xFFFFFFFF)
        return NULL;
    block_list = (const WORD*) &reader->u.jg.toc->streams[reader->u.jg.toc->num_streams];
    for (i = 0; i < stream_nr; i++)
        block_list += (reader->u.jg.toc->streams[i].size +
                       reader->u.jg.header->block_size - 1) / reader->u.jg.header->block_size;

    return pdb_jg_read(reader->u.jg.header, block_list,
                       reader->u.jg.toc->streams[stream_nr].size);
}

static BOOL pdb_jg_init(struct pdb_reader* reader)
{
    reader->u.jg.header = PRD(0, sizeof(struct PDB_JG_HEADER));
    if (!reader->u.jg.header) return FALSE;
    reader->read_stream = pdb_jg_read_stream;
    reader->u.jg.toc = pdb_jg_read(reader->u.jg.header,
                                   reader->u.jg.header->toc_block,
                                   reader->u.jg.header->toc.size);
    memset(reader->stream_used, 0, sizeof(reader->stream_used));
    reader->u.jg.root = reader->read_stream(reader, 1);
    if (!reader->u.jg.root) return FALSE;
    return TRUE;
}

static DWORD    pdb_get_num_streams(const struct pdb_reader* reader)
{
    if (reader->read_stream == pdb_jg_read_stream)
        return reader->u.jg.toc->num_streams;
    else
        return reader->u.ds.toc->num_streams;
}

static DWORD    pdb_get_stream_size(const struct pdb_reader* reader, unsigned idx)
{
    if (reader->read_stream == pdb_jg_read_stream)
        return reader->u.jg.toc->streams[idx].size;
    else
        return reader->u.ds.toc->stream_size[idx];
}

static void pdb_exit(struct pdb_reader* reader)
{
    unsigned            i, size;
    unsigned char*      stream;

    if (globals_dump_sect("ALL")) /* otherwise we won't have loaded all streams */
    {
        for (i = 0; i < pdb_get_num_streams(reader); i++)
        {
            if (has_stream_been_read(reader, i)) continue;

            stream = reader->read_stream(reader, i);
            if (!stream) continue;

            size = pdb_get_stream_size(reader, i);

            printf("Stream --unused-- #%d (%x)\n", i, size);
            dump_data(stream, size, "    ");
            free(stream);
        }
    }
    free(reader->global_string_table);
    if (reader->read_stream == pdb_jg_read_stream)
    {
        free((char*)reader->u.jg.root);
        free((char*)reader->u.jg.toc);
    }
    else
    {
        free((char*)reader->u.ds.root);
        free((char*)reader->u.ds.toc);
    }
}

/* forward declarations */
static void pdb_dump_fpo(struct pdb_reader* reader, unsigned stream_idx);
static void pdb_dump_fpo_ext(struct pdb_reader* reader, unsigned stream_idx);
static void pdb_dump_sections(struct pdb_reader* reader, unsigned stream_idx);

static unsigned get_stream_by_name(struct pdb_reader* reader, const char* name)
{
    DWORD*      pdw;
    DWORD*      ok_bits;
    DWORD       cbstr, count;
    DWORD       string_idx, stream_idx;
    unsigned    i;
    const char* str;

    if (reader->read_stream == pdb_jg_read_stream)
    {
        str = reader->u.jg.root->names;
        cbstr = reader->u.jg.root->cbNames;
    }
    else
    {
        str = reader->u.ds.root->names;
        cbstr = reader->u.ds.root->cbNames;
    }

    pdw = (DWORD*)(str + cbstr);
    pdw++; /* number of ok entries */
    count = *pdw++;

    /* bitfield: first dword is len (in dword), then data */
    ok_bits = pdw;
    pdw += *ok_bits++ + 1;
    pdw += *pdw + 1; /* skip deleted vector */

    for (i = 0; i < count; i++)
    {
        if (ok_bits[i / 32] & (1 << (i % 32)))
        {
            string_idx = *pdw++;
            stream_idx = *pdw++;
            if (!strcmp(name, &str[string_idx])) return stream_idx;
        }
    }
    return -1;
}

static void dump_string_table(const PDB_STRING_TABLE* strtable, const char* name, const char* pfx)
{
    const char* end;
    const char* ptr;
    unsigned* table;
    unsigned num_buckets;
    unsigned i;

    if (!strtable)
    {
        printf("%sString table (%s) isn't present\n", pfx, name);
        return;
    }
    printf("%sString table (%s)\n"
           "%s\tHeader:       %08x\n"
           "%s\tLength:       %08x\n"
           "%s\tHash version: %u\n",
           pfx, name, pfx, strtable->magic, pfx, strtable->length, pfx, strtable->hash_version);
    ptr = (const char*)(strtable + 1);
    end = ptr + strtable->length;
    while (ptr < end)
    {
        printf("%s\t%tu]     %s\n", pfx, ptr - (const char*)(strtable + 1), ptr);
        ptr += strlen(ptr) + 1;
    }
    table = (unsigned *)((char*)(strtable + 1) + strtable->length);
    num_buckets = *table++;

    if (globals_dump_sect("hash"))
    {
        printf("%s\tHash:\n"
               "%s\t\tnum_strings: %x\n"
               "%s\t\tnum_buckets: %x\n",
               pfx, pfx, table[num_buckets], pfx, num_buckets);

        for (i = 0; i < num_buckets; i++)
            printf("%s\t\t%x] %x\n", pfx, i, table[i]);
    }
}

static PDB_STRING_TABLE* read_string_table(struct pdb_reader* reader)
{
    unsigned            stream_idx;
    PDB_STRING_TABLE*   ret;
    unsigned            stream_size;

    stream_idx = get_stream_by_name(reader, "/names");
    if (stream_idx == -1) return NULL;
    ret = reader->read_stream(reader, stream_idx);
    if (!ret) return NULL;
    stream_size = pdb_get_stream_size(reader, stream_idx);
    if (globals_dump_sect("PDB")) dump_string_table(ret, "Global", "    ");
    if (ret->magic == 0xeffeeffe && sizeof(*ret) + ret->length < stream_size) return ret;
    printf("Improper string table header (magic=%x)\n", ret->magic);
    dump_data((const unsigned char*)ret, stream_size, "    ");
    free( ret );
    return NULL;
}

const char* pdb_get_string_table_entry(const PDB_STRING_TABLE* table, unsigned ofs)
{
    if (!table) return "<<no string table>>";
    if (ofs >= table->length) return "<<invalid string table offset>>";
    /* strings start after header */
    return (char*)(table + 1) + ofs;
}

static void dump_dbi_hash_table(const BYTE* root, unsigned size, const char* name, const char* pfx)
{
    if (!globals_dump_sect("hash")) return;
    if (size >= sizeof(DBI_HASH_HEADER))
    {
        const DBI_HASH_HEADER* hdr = (const DBI_HASH_HEADER*)root;

        printf("%s%s symbols hash:\n", pfx, name);
        printf("%s\tSignature: 0x%x\n", pfx, hdr->signature);
        printf("%s\tVersion: 0x%x (%u)\n", pfx, hdr->version, hdr->version - 0xeffe0000);
        printf("%s\tSize of hash records: %u\n", pfx, hdr->hash_records_size);
        printf("%s\tUnknown: %u\n", pfx, hdr->unknown);

        if (hdr->signature != 0xFFFFFFFF ||
            hdr->version != 0xeffe0000 + 19990810 ||
            (hdr->hash_records_size % sizeof(DBI_HASH_RECORD)) != 0 ||
            sizeof(DBI_HASH_HEADER) + hdr->hash_records_size + DBI_BITMAP_HASH_SIZE > size ||
            (size - (sizeof(DBI_HASH_HEADER) + hdr->hash_records_size + DBI_BITMAP_HASH_SIZE)) % sizeof(unsigned))
        {
            if (size >= sizeof(DBI_HASH_HEADER) && !hdr->hash_records_size)
                printf("%s\t\tEmpty hash structure\n", pfx);
            else
                printf("%s\t\tIncorrect hash structure\n", pfx);
        }
        else
        {
            unsigned i;
            unsigned num_hash_records = hdr->hash_records_size / sizeof(DBI_HASH_RECORD);
            const DBI_HASH_RECORD* hr = (const DBI_HASH_RECORD*)(hdr + 1);
            unsigned* bitmap = (unsigned*)((char*)(hdr + 1) + hdr->hash_records_size);
            unsigned* buckets = (unsigned*)((char*)(hdr + 1) + hdr->hash_records_size + DBI_BITMAP_HASH_SIZE);
            unsigned index, last_index = (size - (sizeof(DBI_HASH_HEADER) + hdr->hash_records_size + DBI_BITMAP_HASH_SIZE)) / sizeof(unsigned);

            /* Yes, offsets for accessiong hr[] are stored as multiple of 12; and not
             * as multiple of sizeof(*hr) = 8 as one might expect.
             * Perhaps, native implementation likes to keep the same offsets between
             * in memory representation vs on file representations.
             */
            for (index = 0, i = 0; i <= DBI_MAX_HASH; i++)
            {
                if (bitmap[i / 32] & (1u << (i % 32)))
                {
                    unsigned j;
                    printf("%s\t[%u]\n", pfx, i);
                    for (j = buckets[index] / 12; j < (index + 1 < last_index ? buckets[index + 1] / 12 : num_hash_records); j++)
                        printf("%s\t\t[%u] offset=%08x unk=%x\n", pfx, j, hr[j].offset - 1, hr[j].unknown);
                    index++;
                }
                else
                    printf("%s\t[%u] <<empty>>\n", pfx, i);
            }
            /* shouldn't happen */
            if (sizeof(DBI_HASH_HEADER) + hdr->hash_records_size + DBI_BITMAP_HASH_SIZE + index * sizeof(unsigned) > size)
            {
                printf("%s-- left over %u bytes\n", pfx,
                       size - (unsigned)(sizeof(DBI_HASH_HEADER) + hdr->hash_records_size + DBI_BITMAP_HASH_SIZE + index * sizeof(unsigned)));
            }
        }
    }
    else
        printf("%sNo header in symbols hash\n", pfx);
}

static void dump_global_symbol(struct pdb_reader* reader, unsigned stream)
{
    void*  global = NULL;
    DWORD  size;

    global = reader->read_stream(reader, stream);
    if (!global) return;

    size = pdb_get_stream_size(reader, stream);

    dump_dbi_hash_table(global, size, "Global", "");
    free(global);
}

static void dump_public_symbol(struct pdb_reader* reader, unsigned stream)
{
    unsigned            size;
    DBI_PUBLIC_HEADER*  hdr;
    const BYTE*         ptr;
    unsigned            i;

    if (!globals_dump_sect("public")) return;
    hdr = reader->read_stream(reader, stream);
    if (!hdr) return;

    size = pdb_get_stream_size(reader, stream);

    printf("Public symbols table: (%u)\n", size);

    printf("\tHash size:              %u\n", hdr->hash_size);
    printf("\tAddress map size:       %u\n", hdr->address_map_size);
    printf("\tNumber of thunks:       %u\n", hdr->num_thunks);
    printf("\tSize of thunk map:      %u\n", hdr->thunk_size);
    printf("\tSection of thunk table: %u\n", hdr->section_thunk_table);
    printf("\tOffset of thunk table:  %u\n", hdr->offset_thunk_table);
    printf("\tNumber of sections:     %u\n", hdr->num_sections);

    ptr = (const BYTE*)(hdr + 1);
    dump_dbi_hash_table(ptr, hdr->hash_size, "Public", "\t");

    ptr += hdr->hash_size;
    printf("\tAddress map:\n");
    for (i = 0; i < hdr->address_map_size / sizeof(unsigned); i++)
        printf("\t\t%u]     %08x\n", i, ((const unsigned*)ptr)[i]);

    ptr += hdr->address_map_size;
    printf("\tThunk map:\n");
    for (i = 0; i < hdr->num_thunks; i++)
        printf("\t\t%u]      %08x\n", i, ((const unsigned*)ptr)[i]);

    ptr += hdr->num_thunks * sizeof(unsigned);
    printf("\tSection map:\n");
    for (i = 0; i < hdr->num_sections; i++)
        printf("\t\t%u]      %04x:%08x\n", i, (unsigned short)((const unsigned*)ptr)[2 * i + 1], ((const unsigned*)ptr)[2 * i + 0]);

    if (ptr + hdr->num_sections * 8 != ((const BYTE*)hdr) + size)
        printf("Incorrect stream\n");
    free(hdr);
}

static const void* pdb_dump_dbi_module(struct pdb_reader* reader, const PDB_SYMBOL_FILE_EX* sym_file,
                                       const char* file_name)
{
    const char* lib_name;
    unsigned char* modimage;
    BOOL new_format = !file_name;

    if (new_format) file_name = sym_file->filename;
    printf("\t--------symbol file-----------\n");
    printf("\tName: %s\n", file_name);
    lib_name = file_name + strlen(file_name) + 1;
    if (strcmp(file_name, lib_name)) printf("\tLibrary: %s\n", lib_name);
    printf("\t\tunknown1:   %08x\n"
           "\t\trange\n"
           "\t\t\tsegment:         %04x\n"
           "\t\t\tpad1:            %04x\n"
           "\t\t\toffset:          %08x\n"
           "\t\t\tsize:            %08x\n"
           "\t\t\tcharacteristics: %08x",
           sym_file->unknown1,
           sym_file->range.segment,
           sym_file->range.pad1,
           sym_file->range.offset,
           sym_file->range.size,
           sym_file->range.characteristics);
    dump_section_characteristics(sym_file->range.characteristics, " ");
    printf("\n"
           "\t\t\tindex:           %04x\n"
           "\t\t\tpad2:            %04x\n",
           sym_file->range.index,
           sym_file->range.pad2);
    if (new_format)
        printf("\t\t\ttimestamp:       %08x\n"
               "\t\t\tunknown:         %08x\n",
               sym_file->range.timestamp,
               sym_file->range.unknown);
    printf("\t\tflag:       %04x\n"
           "\t\tstream:     %04x\n"
           "\t\tsymb size:  %08x\n"
           "\t\tline size:  %08x\n"
           "\t\tline2 size: %08x\n"
           "\t\tnSrcFiles:  %08x\n"
           "\t\tattribute:  %08x\n",
           sym_file->flag,
           sym_file->stream,
           sym_file->symbol_size,
           sym_file->lineno_size,
           sym_file->lineno2_size,
           sym_file->nSrcFiles,
           sym_file->attribute);
    if (new_format)
        printf("\t\treserved/0: %08x\n"
               "\t\treserved/1: %08x\n",
               sym_file->reserved[0],
               sym_file->reserved[1]);

    modimage = reader->read_stream(reader, sym_file->stream);
    if (modimage)
    {
        int total_size = pdb_get_stream_size(reader, sym_file->stream);

        if (sym_file->symbol_size)
            codeview_dump_symbols((const char*)modimage, sizeof(DWORD), sym_file->symbol_size);

        /* line number info */
        if (sym_file->lineno_size)
            codeview_dump_linetab((const char*)modimage + sym_file->symbol_size, TRUE, "        ");
        else if (sym_file->lineno2_size) /* actually, only one of the 2 lineno should be present */
            codeview_dump_linetab2((const char*)modimage + sym_file->symbol_size, sym_file->lineno2_size,
                                   reader->global_string_table, "        ");
        /* what's that part ??? */
        if (0)
            dump_data(modimage + sym_file->symbol_size + sym_file->lineno_size + sym_file->lineno2_size,
                      total_size - (sym_file->symbol_size + sym_file->lineno_size + sym_file->lineno2_size), "    ");
        free(modimage);
    }
    return (const void*)((DWORD_PTR)(lib_name + strlen(lib_name) + 1 + 3) & ~3);
}

static void pdb_dump_symbols(struct pdb_reader* reader)
{
    PDB_SYMBOLS*        symbols;
    unsigned char*      modimage;
    const char*         file;
    char                tcver[32];
    const unsigned short* sub_streams = NULL;
    unsigned            num_sub_streams = 0;

    symbols = reader->read_stream(reader, 3);
    if (!symbols) return;

    if (globals_dump_sect("DBI"))
    {
        switch (symbols->version)
        {
        case 0:            /* VC 4.0 */
        case 19960307:     /* VC 5.0 */
        case 19970606:     /* VC 6.0 */
        case 19990903:     /* VC 7.0 */
            break;
        default:
            printf("-Unknown symbol info version %d\n", symbols->version);
        }
        if (symbols->flags & 0x8000) /* new */
            sprintf(tcver, "%u.%u", (symbols->flags >> 8) & 0x7f, symbols->flags & 0xff);
        else
            sprintf(tcver, "old-%x", symbols->flags);
        printf("Symbols:\n"
               "\tsignature:          %08x\n"
               "\tversion:            %u\n"
               "\tage:                %08x\n"
               "\tglobal_hash_stream: %u\n"
               "\tbuilder:            %s\n"
               "\tpublic_stream:      %u\n"
               "\tbldVer:             %u\n"
               "\tgsym_stream:        %u\n"
               "\trbldVer:            %u\n"
               "\tmodule_size:        %08x\n"
               "\tsectcontrib_size:   %08x\n"
               "\tsegmap_size:        %08x\n"
               "\tsrc_module_size:    %08x\n"
               "\tpdbimport_size:     %08x\n"
               "\tresvd0:             %08x\n"
               "\tstream_idx_size:    %08x\n"
               "\tunknown2_size:      %08x\n"
               "\tresvd3:             %04x\n"
               "\tmachine:            %s\n"
               "\tresvd4              %08x\n",
               symbols->signature,
               symbols->version,
               symbols->age,
               symbols->global_hash_stream,
               tcver, /* from symbols->flags */
               symbols->public_stream,
               symbols->bldVer,
               symbols->gsym_stream,
               symbols->rbldVer,
               symbols->module_size,
               symbols->sectcontrib_size,
               symbols->segmap_size,
               symbols->srcmodule_size,
               symbols->pdbimport_size,
               symbols->resvd0,
               symbols->stream_index_size,
               symbols->unknown2_size,
               symbols->resvd3,
               get_machine_str( symbols->machine ),
               symbols->resvd4);
    }

    if (symbols->sectcontrib_size && globals_dump_sect("image"))
    {
        const BYTE*                 src = (const BYTE*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size;
        const BYTE*                 last = src + symbols->sectcontrib_size;
        unsigned                    version, size;

        printf("\t----------section contrib------------\n");
        version = *(unsigned*)src;
        printf("\tVersion:      %#x (%d)\n", version, version - 0xeffe0000);
        switch (version)
        {
        case 0xeffe0000 + 19970605: size = sizeof(PDB_SYMBOL_RANGE_EX); break;
        case 0xeffe0000 + 20140516: size = sizeof(PDB_SYMBOL_RANGE_EX) + sizeof(unsigned); break;
        default: printf("\t\tUnsupported version number\n"); size = 0;
        }
        if (size)
        {
            const PDB_SYMBOL_RANGE_EX* range;

            if ((symbols->sectcontrib_size - sizeof(unsigned)) % size)
                printf("Incoherent size: %zu = %zu * %u + %zu\n",
                       symbols->sectcontrib_size - sizeof(unsigned),
                       (symbols->sectcontrib_size - sizeof(unsigned)) / size,
                       size,
                       (symbols->sectcontrib_size - sizeof(unsigned)) % size);
            for (src += sizeof(unsigned); src + size <= last; src += size)
            {
                range = (const PDB_SYMBOL_RANGE_EX*)src;
                printf("\tRange #%tu\n",
                       ((const BYTE*)range - ((const BYTE*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size)) / size);
                printf("\t\tsegment:         %04x\n"
                       "\t\tpad1:            %04x\n"
                       "\t\toffset:          %08x\n"
                       "\t\tsize:            %08x\n"
                       "\t\tcharacteristics: %08x",
                       range->segment,
                       range->pad1,
                       range->offset,
                       range->size,
                       range->characteristics);
                dump_section_characteristics(range->characteristics, " ");
                printf("\n"
                       "\t\tindex:           %04x\n"
                       "\t\tpad2:            %04x\n"
                       "\t\ttimestamp:       %08x\n"
                       "\t\tunknown:         %08x\n",
                       range->index,
                       range->pad2,
                       range->timestamp,
                       range->unknown);
                if (version == 0xeffe0000 + 20140516)
                    printf("\t\tcoff_section:    %08x\n", *(unsigned*)(range + 1));
            }
        }
    }

    if (symbols->srcmodule_size && globals_dump_sect("DBI"))
    {
        const PDB_SYMBOL_SOURCE*src;
        int                     i, j, cfile;
        const WORD*             indx;
        const DWORD*            offset;
        const char*             start_cstr;
        const char*             cstr;

        printf("\t----------src module------------\n");
        src = (const PDB_SYMBOL_SOURCE*)((const char*)symbols + sizeof(PDB_SYMBOLS) +
                                         symbols->module_size + symbols->sectcontrib_size + symbols->segmap_size);
        printf("\tSource Modules\n"
               "\t\tnModules:         %u\n"
               "\t\tnSrcFiles:        %u\n",
               src->nModules, src->nSrcFiles);

        /* usage of table seems to be as follows:
         * two arrays of WORD (src->nModules as size)
         *  - first array contains index into files for "module" compilation
         *    (module = compilation unit ??)
         *  - second array contains the number of source files in module
         *    an array of DWORD (src->nSrcFiles as size)
         *  - contains offset (in following string table) of the source file name
         *    a string table
         *  - each string is a pascal string (ie. with its length as first BYTE) or
         *    0-terminated string (depending on version)
         */
        indx = &src->table[src->nModules];
        offset = (const DWORD*)&src->table[2 * src->nModules];
        cstr = (const char*)&src->table[2 * (src->nModules + src->nSrcFiles)];
        start_cstr = cstr;

        for (i = cfile = 0; i < src->nModules; i++)
        {
            printf("\t\tModule[%2d]:\n", i);
            cfile = src->table[i];
            for (j = cfile; j < src->nSrcFiles && j < cfile + indx[i]; j++)
            {
                /* FIXME: in some cases, it's a p_string but WHEN ? */
                if (cstr + offset[j] >= start_cstr /* wrap around */ &&
                    cstr + offset[j] < (const char*)src + symbols->srcmodule_size)
                    printf("\t\t\tSource file: %s\n", cstr + offset[j]);
                else
                    printf("\t\t\tSource file: <<out of bounds>>\n");
            }
        }
    }
    if (symbols->pdbimport_size && globals_dump_sect("PDB"))
    {
        const PDB_SYMBOL_IMPORT*  imp;
        const char* first;
        const char* last;
        const char* ptr;

        printf("\t------------import--------------\n");
        imp = (const PDB_SYMBOL_IMPORT*)((const char*)symbols + sizeof(PDB_SYMBOLS) +
                                         symbols->module_size + symbols->sectcontrib_size +
                                         symbols->segmap_size + symbols->srcmodule_size);
        first = (const char*)imp;
        last = (const char*)imp + symbols->pdbimport_size;
        while (imp < (const PDB_SYMBOL_IMPORT*)last)
        {
            ptr = (const char*)imp + sizeof(*imp) + strlen(imp->filename);
            printf("\tImport: %lx\n"
                   "\t\tUnknown1:      %08x\n"
                   "\t\tUnknown2:      %08x\n"
                   "\t\tTimeDateStamp: %08x\n"
                   "\t\tAge:           %08u\n"
                   "\t\tfile1:         %s\n"
                   "\t\tfile2:         %s\n",
                   (ULONG_PTR)((const char*)imp - first),
                   imp->unknown1,
                   imp->unknown2,
                   imp->TimeDateStamp,
                   imp->Age,
                   imp->filename,
                   ptr);
            imp = (const PDB_SYMBOL_IMPORT*)(first + ((ptr - first + strlen(ptr) + 1 + 3) & ~3));
        }
    }
    if (symbols->segmap_size && globals_dump_sect("image"))
    {
        const struct OMFSegMap* segmap = (const struct OMFSegMap*)((const BYTE*)symbols + sizeof(PDB_SYMBOLS) +
                                                                   symbols->module_size + symbols->sectcontrib_size);
        const struct OMFSegMapDesc* desc = (const struct OMFSegMapDesc*)(segmap + 1);

        printf("\t--------------segment map----------------\n");
        printf("\tNumber of segments: %x\n", segmap->cSeg);
        printf("\tNumber of logical segments: %x\n", segmap->cSegLog);
        /* FIXME check mapping old symbols */
        for (; (const BYTE*)(desc + 1) <= ((const BYTE*)(segmap + 1) + symbols->segmap_size); desc++)
        {
            printf("\t\tSegment descriptor #%tu\n", desc - (const struct OMFSegMapDesc*)(segmap + 1));
            printf("\t\t\tFlags: %04x (%c%c%c%s%s%s%s)\n",
                   desc->flags,
                   (desc->flags & 0x01) ? 'R' : '-',
                   (desc->flags & 0x02) ? 'W' : '-',
                   (desc->flags & 0x04) ? 'X' : '-',
                   (desc->flags & 0x08) ? " 32bit-linear" : "",
                   (desc->flags & 0x100) ? " selector" : "",
                   (desc->flags & 0x200) ? " absolute" : "",
                   (desc->flags & 0x400) ? " group" : "");
            printf("\t\t\tOverlay: %04x\n", desc->ovl);
            printf("\t\t\tGroup: %04x\n", desc->group);
            printf("\t\t\tFrame: %04x\n", desc->frame);
            printf("\t\t\tSegment name: %s\n", desc->iSegName == 0xffff ? "none" : pdb_get_string_table_entry(reader->global_string_table, desc->iSegName));
            printf("\t\t\tClass name: %s\n",  desc->iClassName == 0xffff ? "none" : pdb_get_string_table_entry(reader->global_string_table, desc->iClassName));
            printf("\t\t\tOffset: %08x\n", desc->offset);
            printf("\t\t\tSize: %04x\n", desc->cbSeg);
        }
    }
    if (symbols->unknown2_size && globals_dump_sect("PDB"))
    {
        const char* ptr = (const char*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size +
            symbols->sectcontrib_size + symbols->segmap_size + symbols->srcmodule_size +
            symbols->pdbimport_size;
        printf("\t------------Unknown2--------------\n");
        dump_string_table((const PDB_STRING_TABLE*)ptr, "Unknown from DBI", "\t");
    }
    if (symbols->stream_index_size && globals_dump_sect("image"))
    {
        const char* sub_stream_names[] = {"FPO", NULL, NULL, NULL, NULL, "Sections stream", NULL, NULL, NULL, "FPO-ext"};
        int i;

        printf("\t------------stream indexes--------------\n");
        num_sub_streams = symbols->stream_index_size / sizeof(sub_streams[0]);
        sub_streams = (const unsigned short*)((const char*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size +
                                              symbols->sectcontrib_size + symbols->segmap_size + symbols->srcmodule_size +
                                              symbols->pdbimport_size + symbols->unknown2_size);
        for (i = 0; i < num_sub_streams; i++)
        {
            const char* name = "?";
            if (i < ARRAY_SIZE(sub_stream_names) && sub_stream_names[i])
                name = sub_stream_names[i];
            printf("\t%s:%.*s%04x\n", name, (int)(21 - strlen(name)), "", sub_streams[i]);
        }
    }

    /* Read global symbol table */
    modimage = reader->read_stream(reader, symbols->gsym_stream);
    if (modimage && globals_dump_sect("DBI"))
    {
        printf("\t------------globals-------------\n");
        codeview_dump_symbols(modimage, 0, pdb_get_stream_size(reader, symbols->gsym_stream));
        free(modimage);
    }

    /* Read per-module symbol / linenumber tables */
    if (symbols->module_size && globals_dump_sect("DBI"))
    {
        SIZE_T module_header_size = symbols->version < 19970000 ? sizeof(PDB_SYMBOL_FILE) : sizeof(PDB_SYMBOL_FILE_EX);

        file = (const char*)symbols + sizeof(PDB_SYMBOLS);
        while (file + module_header_size <= (const char*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size)
        {
            if (symbols->version < 19970000)
            {
                PDB_SYMBOL_FILE_EX copy;
                const PDB_SYMBOL_FILE* sym_file = (const PDB_SYMBOL_FILE*)file;

                copy.unknown1 = sym_file->unknown1;
                copy.range.segment = sym_file->range.segment;
                copy.range.pad1 = sym_file->range.pad1;
                copy.range.offset = sym_file->range.offset;
                copy.range.size = sym_file->range.size;
                copy.range.characteristics = sym_file->range.characteristics;
                copy.range.index = sym_file->range.index;
                copy.range.pad2 = sym_file->range.pad2;
                copy.range.timestamp = 0;
                copy.range.unknown = 0;
                copy.flag = sym_file->flag;
                copy.stream = sym_file->stream;
                copy.symbol_size = sym_file->symbol_size;
                copy.lineno_size = sym_file->lineno_size;
                copy.lineno2_size = sym_file->lineno2_size;
                copy.nSrcFiles = sym_file->nSrcFiles;
                copy.attribute = sym_file->attribute;
                copy.reserved[0] = 0;
                copy.reserved[1] = 0;
                file = pdb_dump_dbi_module(reader, &copy, sym_file->filename);
            }
            else
                file = pdb_dump_dbi_module(reader, (const PDB_SYMBOL_FILE_EX*)file, NULL);
        }
    }
    dump_global_symbol(reader, symbols->global_hash_stream);
    dump_public_symbol(reader, symbols->public_stream);

    if (sub_streams && globals_dump_sect("image"))
    {
        if (PDB_SIDX_FPO < num_sub_streams)
            pdb_dump_fpo(reader, sub_streams[PDB_SIDX_FPO]);
        if (PDB_SIDX_FPOEXT < num_sub_streams)
            pdb_dump_fpo_ext(reader, sub_streams[PDB_SIDX_FPOEXT]);
        if (PDB_SIDX_SECTIONS < num_sub_streams)
            pdb_dump_sections(reader, sub_streams[PDB_SIDX_SECTIONS]);
    }

    free(symbols);
}

static BOOL is_bit_set(const unsigned* dw, unsigned len, unsigned i)
{
    if (i >= len * sizeof(unsigned) * 8) return FALSE;
    return (dw[i >> 5] & (1u << (i & 31u))) != 0;
}

static void pdb_dump_hash_value(const BYTE* ptr, unsigned len)
{
    int i;

    printf("[");
    for (i = len - 1; i >= 0; i--)
        printf("%02x", ptr[i]);
    printf("]");
}

static struct
{
    const BYTE* hash;
    unsigned hash_size;
} collision_arg;

static int collision_compar(const void *p1, const void *p2)
{
    unsigned idx1 = *(unsigned*)p1;
    unsigned idx2 = *(unsigned*)p2;
    return memcmp(collision_arg.hash + idx1 * collision_arg.hash_size,
                  collision_arg.hash + idx2 * collision_arg.hash_size,
                  collision_arg.hash_size);
}

static void pdb_dump_types_hash(struct pdb_reader* reader, const PDB_TYPES* types, const char* strmname)
{
    void*  hash = NULL;
    unsigned i, strmsize;
    const unsigned* table;
    unsigned *collision;

    if (!globals_dump_sect("hash")) return;
    hash = reader->read_stream(reader, types->hash_stream);
    if (!hash) return;

    printf("Types (%s) hash:\n", strmname);
    strmsize = pdb_get_stream_size(reader, types->hash_stream);
    if (types->hash_offset + types->hash_size > strmsize ||
        (types->last_index - types->first_index) * types->hash_value_size != types->hash_size ||
        types->search_offset + types->search_size > strmsize ||
        types->type_remap_offset + types->type_remap_size > strmsize)
    {
        printf("\nIncoherent sizes... skipping\n");
        return;
    }
    printf("\n\tIndexes => hash value:\n");
    for (i = types->first_index; i < types->last_index; i++)
    {
        printf("\t\t%08x => ", i);
        pdb_dump_hash_value((const BYTE*)hash + types->hash_offset + (i - types->first_index) * types->hash_value_size, types->hash_value_size);
        printf("\n");
    }
    /* print collisions in hash table (if any) */
    collision = malloc((types->last_index - types->first_index) * sizeof(unsigned));
    if (collision)
    {
        unsigned head_printed = 0;

        collision_arg.hash = (const BYTE*)hash + types->hash_offset;
        collision_arg.hash_size = types->hash_value_size;

        for (i = 0; i < types->last_index - types->first_index; i++) collision[i] = i;
        qsort(collision, types->last_index - types->first_index, sizeof(unsigned), collision_compar);
        for (i = 0; i < types->last_index - types->first_index; i++)
        {
            unsigned j;
            for (j = i + 1; j < types->last_index - types->first_index; j++)
                if (memcmp((const BYTE*)hash + types->hash_offset + collision[i] * types->hash_value_size,
                           (const BYTE*)hash + types->hash_offset + collision[j] * types->hash_value_size,
                           types->hash_value_size))
                    break;
            if (j > i + 1)
            {
                unsigned k;
                if (!head_printed)
                {
                    printf("\n\t\tCollisions:\n");
                    head_printed = 1;
                }
                printf("\t\t\tHash ");
                pdb_dump_hash_value((const BYTE*)hash + types->hash_offset + collision[i] * types->hash_value_size, types->hash_value_size);
                printf(":");
                for (k = i; k < j; k++)
                    printf(" %x", types->first_index + collision[k]);
                printf("\n");
                i = j - 1;
            }
        }
        free(collision);
    }
    printf("\n\tIndexes => offsets:\n");
    table = (const unsigned*)((const BYTE*)hash + types->search_offset);
    for (i = 0; i < types->search_size / (2 * sizeof(unsigned)); i += 2)
    {
        printf("\t\t%08x => %08x\n", table[2 * i + 0], table[2 * i + 1]);
    }

    if (types->type_remap_size)
    {
        unsigned num, capa, count_present, count_deleted;
        const unsigned* present_bitset;
        const unsigned* deleted_bitset;

        printf("\n\tType remap:\n");
        table = (const unsigned*)((const BYTE*)hash + types->type_remap_offset);
        num = *table++;
        capa = *table++;
        count_present = *table++;
        present_bitset = table;
        table += count_present;
        count_deleted = *table++;
        deleted_bitset = table;
        table += count_deleted;
        printf("\t\tNumber of present entries: %u\n", num);
        printf("\t\tCapacity: %u\n", capa);
        printf("\t\tBitset present:\n");
        printf("\t\t\tCount: %u\n", count_present);
        printf("\t\t\tBitset: ");
        pdb_dump_hash_value((const BYTE*)present_bitset, count_present * sizeof(unsigned));
        printf("\n");
        printf("\t\tBitset deleted:\n");
        printf("\t\t\tCount: %u\n", count_deleted);
        printf("\t\t\tBitset: ");
        pdb_dump_hash_value((const BYTE*)deleted_bitset, count_deleted * sizeof(unsigned));
        printf("\n");
        for (i = 0; i < capa; ++i)
        {
            printf("\t\t%2u) %c",
                   i,
                   is_bit_set(present_bitset, count_present, i) ? 'P' :
                   is_bit_set(deleted_bitset, count_deleted, i) ? 'D' : '_');
            if (is_bit_set(present_bitset, count_present, i))
            {
                printf(" %s => ", pdb_get_string_table_entry(reader->global_string_table, *table++));
                pdb_dump_hash_value((const BYTE*)table, types->hash_value_size);
                table = (const unsigned*)((const BYTE*)table + types->hash_value_size);
            }
            printf("\n");
        }
        printf("\n");
    }
    free(hash);
}

/* there are two 'type' related streams, but with different indexes... */
static void pdb_dump_types(struct pdb_reader* reader, unsigned strmidx, const char* strmname)
{
    PDB_TYPES*  types = NULL;
    BOOL used = has_stream_been_read(reader, strmidx);

    if (!globals_dump_sect(strmidx == 2 ? "TPI" : "IPI")) return;
    if (pdb_get_stream_size(reader, strmidx) < sizeof(*types))
    {
        if (strmidx == 2)
            printf("-Too small type header\n");
        return;
    }
    types = reader->read_stream(reader, strmidx);
    if (!types) return;

    switch (types->version)
    {
    case 19950410:      /* VC 4.0 */
    case 19951122:
    case 19961031:      /* VC 5.0 / 6.0 */
    case 19990903:      /* VC 7.0 */
    case 20040203:      /* VC 8.0 */
        break;
    default:
        /* IPI stream is not always present in older PDB files */
        if (strmidx == 2)
            printf("-Unknown type info version %d\n", types->version);
        free(types);
        if (used) clear_stream_been_read(reader, strmidx);
        return;
    }

    /* Read type table */
    printf("Types (%s):\n"
           "\tversion:           %u\n"
           "\ttype_offset:       %08x\n"
           "\tfirst_index:       %x\n"
           "\tlast_index:        %x\n"
           "\ttype_size:         %x\n"
           "\thash_stream:       %x\n"
           "\tpad:               %x\n"
           "\thash_value_size:   %x\n"
           "\thash_buckets       %x\n"
           "\thash_offset:       %x\n"
           "\thash_size:         %x\n"
           "\tsearch_offset:     %x\n"
           "\tsearch_size:       %x\n"
           "\ttype_remap_offset: %x\n"
           "\ttype_remap_size:   %x\n",
           strmname,
           types->version,
           types->type_offset,
           types->first_index,
           types->last_index,
           types->type_size,
           types->hash_stream,
           types->pad,
           types->hash_value_size,
           types->hash_num_buckets,
           types->hash_offset,
           types->hash_size,
           types->search_offset,
           types->search_size,
           types->type_remap_offset,
           types->type_remap_size);
    codeview_dump_types_from_block((const char*)types + types->type_offset, types->type_size);
    pdb_dump_types_hash(reader, types, strmname);
    free(types);
}

static void pdb_dump_fpo(struct pdb_reader* reader, unsigned stream_idx)
{
    FPO_DATA*           fpo;
    unsigned            i, size;
    const char*         frame_type[4] = {"Fpo", "Trap", "Tss", "NonFpo"};

    if (stream_idx == (WORD)-1) return;
    fpo = reader->read_stream(reader, stream_idx);
    size = pdb_get_stream_size(reader, stream_idx);
    if (fpo && (size % sizeof(*fpo)) == 0)
    {
        size /= sizeof(*fpo);
        printf("FPO data:\n\t   Start   Length #loc #pmt #prolog #reg frame  SEH /BP\n");
        for (i = 0; i < size; i++)
        {
            printf("\t%08x %08x %4d %4d %7d %4d %6s  %c   %c\n",
                   (UINT)fpo[i].ulOffStart, (UINT)fpo[i].cbProcSize, (UINT)fpo[i].cdwLocals, fpo[i].cdwParams,
                   fpo[i].cbProlog, fpo[i].cbRegs, frame_type[fpo[i].cbFrame],
                   fpo[i].fHasSEH ? 'Y' : 'N', fpo[i].fUseBP ? 'Y' : 'N');
        }
    }
    free(fpo);
}

static void pdb_dump_fpo_ext(struct pdb_reader* reader, unsigned stream_idx)
{
    PDB_FPO_DATA*       fpoext;
    unsigned            i, size;

    if (stream_idx == (WORD)-1) return;

    fpoext = reader->read_stream(reader, stream_idx);
    size = pdb_get_stream_size(reader, stream_idx);
    if (fpoext && (size % sizeof(*fpoext)) == 0)
    {
        size /= sizeof(*fpoext);
        printf("FPO data (extended):\n"
               "\t   Start   Length   Locals   Params MaxStack Prolog #SavedRegs    Flags Command\n");
        for (i = 0; i < size; i++)
        {
            printf("\t%08x %08x %8x %8x %8x %6x   %8x %08x %s\n",
                   fpoext[i].start, fpoext[i].func_size, fpoext[i].locals_size, fpoext[i].params_size,
                   fpoext[i].maxstack_size, fpoext[i].prolog_size, fpoext[i].savedregs_size, fpoext[i].flags,
                   pdb_get_string_table_entry(reader->global_string_table, fpoext[i].str_offset));
        }
    }
    free(fpoext);
}

static void pdb_dump_sections(struct pdb_reader* reader, unsigned stream_idx)
{
    const char*                 segs;
    DWORD                       size;
    const IMAGE_SECTION_HEADER* sect_hdr;

    if (stream_idx == (WORD)-1) return;
    segs = reader->read_stream(reader, stream_idx);

    if (segs)
    {
        printf("Sections:\n");
        size = pdb_get_stream_size(reader, stream_idx);
        for (sect_hdr = (const IMAGE_SECTION_HEADER*)segs; (const char*)sect_hdr < segs + size; sect_hdr++)
        {
            printf("\tSection:                %-8.8s\n", sect_hdr->Name);
            printf("\t\tVirtual size:         %08x\n",   (unsigned)sect_hdr->Misc.VirtualSize);
            printf("\t\tVirtualAddress:       %08x\n",   (unsigned)sect_hdr->VirtualAddress);
            printf("\t\tSizeOfRawData:        %08x\n",   (unsigned)sect_hdr->SizeOfRawData);
            printf("\t\tPointerToRawData:     %08x\n",   (unsigned)sect_hdr->PointerToRawData);
            printf("\t\tPointerToRelocations: %08x\n",   (unsigned)sect_hdr->PointerToRelocations);
            printf("\t\tPointerToLinenumbers: %08x\n",   (unsigned)sect_hdr->PointerToLinenumbers);
            printf("\t\tNumberOfRelocations:  %u\n",     (unsigned)sect_hdr->NumberOfRelocations);
            printf("\t\tNumberOfLinenumbers:  %u\n",     (unsigned)sect_hdr->NumberOfLinenumbers);
            printf("\t\tCharacteristics:      %08x",     (unsigned)sect_hdr->Characteristics);
            dump_section_characteristics(sect_hdr->Characteristics, " ");
            printf("\n");
        }
        free((char*)segs);
    }
}

static const char       pdb2[] = "Microsoft C/C++ program database 2.00";

static void pdb_jg_dump_header_root(struct pdb_reader* reader)
{
    UINT *pdw, *ok_bits;
    UINT i, numok, count;

    if (!globals_dump_sect("PDB")) return;

    printf("Header (JG):\n"
           "\tident:             %.*s\n"
           "\tsignature:         %08x\n"
           "\tblock_size:        %08x\n"
           "\tfree_list_block:   %04x\n"
           "\ttotal_alloc:       %04x\n",
           (int)sizeof(pdb2) - 1, reader->u.jg.header->ident,
           reader->u.jg.header->signature,
           reader->u.jg.header->block_size,
           reader->u.jg.header->free_list_block,
           reader->u.jg.header->total_alloc);

    printf("Root:\n"
           "\tVersion:       %u\n"
           "\tTimeDateStamp: %08x\n"
           "\tAge:           %08x\n"
           "\tnames:         %d\n",
           reader->u.jg.root->Version,
           reader->u.jg.root->TimeDateStamp,
           reader->u.jg.root->Age,
           (unsigned)reader->u.jg.root->cbNames);

    pdw = (UINT *)(reader->u.jg.root->names + reader->u.jg.root->cbNames);
    numok = *pdw++;
    count = *pdw++;
    printf("\tStreams directory:\n"
           "\t\tok:        %08x\n"
           "\t\tcount:     %08x\n"
           "\t\ttable:\n",
           numok, count);

    /* bitfield: first dword is len (in dword), then data */
    ok_bits = pdw;
    pdw += *ok_bits++ + 1;
    pdw += *pdw + 1; /* skip deleted vector */

    for (i = 0; i < count; i++)
    {
        if (ok_bits[i / 32] & (1 << (i % 32)))
        {
            UINT string_idx, stream_idx;
            string_idx = *pdw++;
            stream_idx = *pdw++;
            printf("\t\t\t%2d) %-20s => %x\n", i, &reader->u.jg.root->names[string_idx], stream_idx);
            numok--;
        }
    }
    if (numok) printf(">>> unmatched present field with found\n");

    /* Check for unknown versions */
    switch (reader->u.jg.root->Version)
    {
    case 19950623:      /* VC 4.0 */
    case 19950814:
    case 19960307:      /* VC 5.0 */
    case 19970604:      /* VC 6.0 */
        break;
    default:
        printf("-Unknown root block version %d\n", reader->u.jg.root->Version);
    }
}

static void* pdb_ds_read(const struct PDB_DS_HEADER* header, const UINT *block_list, int size)
{
    int                 i, nBlocks;
    BYTE*               buffer;

    if (!size) return NULL;

    nBlocks = (size + header->block_size - 1) / header->block_size;
    buffer = xmalloc(nBlocks * header->block_size);

    for (i = 0; i < nBlocks; i++)
        memcpy(buffer + i * header->block_size,
               (const char*)header + block_list[i] * header->block_size, header->block_size);

    return buffer;
}

static void* pdb_ds_read_stream(struct pdb_reader* reader, DWORD stream_number)
{
    const UINT *block_list;
    UINT i;

    if (!reader->u.ds.toc || stream_number >= reader->u.ds.toc->num_streams) return NULL;

    mark_stream_been_read(reader, stream_number);
    if (reader->u.ds.toc->stream_size[stream_number] == 0 ||
        reader->u.ds.toc->stream_size[stream_number] == 0xFFFFFFFF)
        return NULL;
    block_list = reader->u.ds.toc->stream_size + reader->u.ds.toc->num_streams;
    for (i = 0; i < stream_number; i++)
        block_list += (reader->u.ds.toc->stream_size[i] + reader->u.ds.header->block_size - 1) /
            reader->u.ds.header->block_size;

    return pdb_ds_read(reader->u.ds.header, block_list, reader->u.ds.toc->stream_size[stream_number]);
}

static BOOL pdb_ds_init(struct pdb_reader* reader)
{
    reader->u.ds.header = PRD(0, sizeof(*reader->u.ds.header));
    if (!reader->u.ds.header) return FALSE;
    reader->read_stream = pdb_ds_read_stream;
    reader->u.ds.toc = pdb_ds_read(reader->u.ds.header,
                                   (const UINT *)((const char*)reader->u.ds.header + reader->u.ds.header->toc_block * reader->u.ds.header->block_size),
                                   reader->u.ds.header->toc_size);
    memset(reader->stream_used, 0, sizeof(reader->stream_used));
    reader->u.ds.root = reader->read_stream(reader, 1);
    if (!reader->u.ds.root) return FALSE;
    return TRUE;
}

static const char       pdb7[] = "Microsoft C/C++ MSF 7.00";

static void pdb_ds_dump_header_root(struct pdb_reader* reader)
{
    unsigned int i, j, ofs;
    const UINT *block_list;
    UINT *pdw, *ok_bits;
    UINT numok, count;
    unsigned strmsize;

    if (!globals_dump_sect("PDB")) return;
    strmsize = pdb_get_stream_size(reader, 1);
    printf("Header (DS)\n"
           "\tsignature:        %.*s\n"
           "\tblock_size:       %08x\n"
           "\tfree_list_block:  %08x\n"
           "\tnum_blocks:       %08x\n"
           "\ttoc_size:         %08x\n"
           "\tunknown2:         %08x\n"
           "\ttoc_block:        %08x\n",
           (int)sizeof(pdb7) - 1, reader->u.ds.header->signature,
           reader->u.ds.header->block_size,
           reader->u.ds.header->free_list_block,
           reader->u.ds.header->num_blocks,
           reader->u.ds.header->toc_size,
           reader->u.ds.header->unknown2,
           reader->u.ds.header->toc_block);

    block_list = reader->u.ds.toc->stream_size + reader->u.ds.toc->num_streams;
    printf("\t\tnum_streams:    %u\n", reader->u.ds.toc->num_streams);
    for (ofs = i = 0; i < reader->u.ds.toc->num_streams; i++)
    {
        unsigned int nblk = (reader->u.ds.toc->stream_size[i] + reader->u.ds.header->block_size - 1) / reader->u.ds.header->block_size;
        printf("\t\tstream[%#x]:\tsize: %u\n", i, reader->u.ds.toc->stream_size[i]);
        if (nblk)
        {
            for (j = 0; j < nblk; j++)
            {
                if (j % 16 == 0) printf("\t\t\t");
                printf("%4x ", block_list[ofs + j]);
                if (j % 16 == 15 || (j + 1 == nblk)) printf("\n");
            }
            ofs += nblk;
        }
    }

    printf("Root:\n"
           "\tVersion:              %u\n"
           "\tTimeDateStamp:        %08x\n"
           "\tAge:                  %08x\n"
           "\tguid                  %s\n"
           "\tcbNames:              %08x\n",
           reader->u.ds.root->Version,
           reader->u.ds.root->TimeDateStamp,
           reader->u.ds.root->Age,
           get_guid_str(&reader->u.ds.root->guid),
           reader->u.ds.root->cbNames);
    pdw = (UINT *)(reader->u.ds.root->names + reader->u.ds.root->cbNames);
    numok = *pdw++;
    count = *pdw++;
    printf("\tStreams directory:\n"
           "\t\tok:        %08x\n"
           "\t\tcount:     %08x\n"
           "\t\ttable:\n",
           numok, count);

    /* bitfield: first dword is len (in dword), then data */
    ok_bits = pdw;
    pdw += *ok_bits++ + 1;
    pdw += *pdw + 1; /* skip deleted vector */

    for (i = 0; i < count; i++)
    {
        if (ok_bits[i / 32] & (1 << (i % 32)))
        {
            UINT string_idx, stream_idx;
            string_idx = *pdw++;
            stream_idx = *pdw++;
            printf("\t\t\t%2d) %-20s => %x\n", i, &reader->u.ds.root->names[string_idx], stream_idx);
            numok--;
        }
    }
    if (numok) printf(">>> unmatched present field with found\n");
    if (*pdw++ != 0)
    {
        printf("unexpected value\n");
        return;
    }

    if (pdw + 1 <= (UINT*)((char*)reader->u.ds.root + strmsize))
    {
        /* extra information (version reference and features) */
        printf("\tVersion and features\n");
        while (pdw + 1 <= (UINT*)((char*)reader->u.ds.root + strmsize))
        {
            switch (*pdw)
            {
            /* version reference */
            case 20091201:              printf("\t\tVC110\n"); break;
            case 20140508:              printf("\t\tVC140\n"); break;
            /* features */
            case 0x4D544F4E /* NOTM */: printf("\t\tNo type merge\n"); break;
            case 0x494E494D /* MINI */: printf("\t\tMinimal debug info\n"); break;
            default:                    printf("\t\tUnknown value %x\n", *pdw);
            }
            pdw++;
        }
    }
}

enum FileSig get_kind_pdb(void)
{
    const char* head;

    head = PRD(0, sizeof(pdb2) - 1);
    if (head && !memcmp(head, pdb2, sizeof(pdb2) - 1))
        return SIG_PDB;
    head = PRD(0, sizeof(pdb7) - 1);
    if (head && !memcmp(head, pdb7, sizeof(pdb7) - 1))
        return SIG_PDB;
    return SIG_UNKNOWN;
}

void pdb_dump(void)
{
    const BYTE* head;
    const char** saved_dumpsect = globals.dumpsect;
    static const char* default_dumpsect[] = {"DBI", "TPI", "IPI", NULL};
    struct pdb_reader reader;

    if (!globals.dumpsect) globals.dumpsect = default_dumpsect;

    if ((head = PRD(0, sizeof(pdb2) - 1)) && !memcmp(head, pdb2, sizeof(pdb2) - 1))
    {
        if (!pdb_jg_init(&reader))
        {
            printf("Unable to get header information\n");
            return;
        }

        pdb_jg_dump_header_root(&reader);
    }
    else if ((head = PRD(0, sizeof(pdb7) - 1)) && !memcmp(head, pdb7, sizeof(pdb7) - 1))
    {
        if (!pdb_ds_init(&reader))
        {
            printf("Unable to get header information\n");
            return;
        }
        pdb_ds_dump_header_root(&reader);
    }
    mark_stream_been_read(&reader, 0); /* mark stream #0 (old TOC) as read */

    reader.global_string_table = read_string_table(&reader);

    pdb_dump_types(&reader, 2, "TPI");
    pdb_dump_types(&reader, 4, "IPI");
    pdb_dump_symbols(&reader);

    pdb_exit(&reader);

    globals.dumpsect = saved_dumpsect;
}
