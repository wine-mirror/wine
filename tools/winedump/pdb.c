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
    void*       (*read_file)(struct pdb_reader*, DWORD);
    DWORD       file_used[1024];
};

static inline BOOL has_file_been_read(struct pdb_reader* reader, unsigned file_nr)
{
    return reader->file_used[file_nr / 32] & (1 << (file_nr % 32));
}

static inline void mark_file_been_read(struct pdb_reader* reader, unsigned file_nr)
{
    reader->file_used[file_nr / 32] |= 1 << (file_nr % 32);
}

static inline void clear_file_been_read(struct pdb_reader* reader, unsigned file_nr)
{
    reader->file_used[file_nr / 32] &= ~(1 << (file_nr % 32));
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

static void* pdb_jg_read_file(struct pdb_reader* reader, DWORD file_nr)
{
    const WORD*         block_list;
    DWORD               i;

    if (!reader->u.jg.toc || file_nr >= reader->u.jg.toc->num_files) return NULL;

    mark_file_been_read(reader, file_nr);
    if (reader->u.jg.toc->file[file_nr].size == 0 ||
        reader->u.jg.toc->file[file_nr].size == 0xFFFFFFFF)
        return NULL;
    block_list = (const WORD*) &reader->u.jg.toc->file[reader->u.jg.toc->num_files];
    for (i = 0; i < file_nr; i++)
        block_list += (reader->u.jg.toc->file[i].size +
                       reader->u.jg.header->block_size - 1) / reader->u.jg.header->block_size;

    return pdb_jg_read(reader->u.jg.header, block_list,
                       reader->u.jg.toc->file[file_nr].size);
}

static void pdb_jg_init(struct pdb_reader* reader)
{
    reader->u.jg.header = PRD(0, sizeof(struct PDB_JG_HEADER));
    reader->read_file = pdb_jg_read_file;
    reader->u.jg.toc = pdb_jg_read(reader->u.jg.header, 
                                   reader->u.jg.header->toc_block,
                                   reader->u.jg.header->toc.size);
    memset(reader->file_used, 0, sizeof(reader->file_used));
}

static DWORD    pdb_get_num_files(const struct pdb_reader* reader)
{
    if (reader->read_file == pdb_jg_read_file)
        return reader->u.jg.toc->num_files;
    else
        return reader->u.ds.toc->num_files;
}

static DWORD    pdb_get_file_size(const struct pdb_reader* reader, unsigned idx)
{
    if (reader->read_file == pdb_jg_read_file)
        return reader->u.jg.toc->file[idx].size;
    else
        return reader->u.ds.toc->file_size[idx];
}

static void pdb_exit(struct pdb_reader* reader)
{
    unsigned            i, size;
    unsigned char*      file;

    for (i = 0; i < pdb_get_num_files(reader); i++)
    {
        if (has_file_been_read(reader, i)) continue;

        file = reader->read_file(reader, i);
        if (!file) continue;

        size = pdb_get_file_size(reader, i);

        printf("File --unused-- #%d (%x)\n", i, size);
        dump_data(file, size, "    ");
        free(file);
    }

    if (reader->read_file == pdb_jg_read_file)
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

static unsigned get_stream_by_name(struct pdb_reader* reader, const char* name)
{
    DWORD*      pdw;
    DWORD*      ok_bits;
    DWORD       cbstr, count;
    DWORD       string_idx, stream_idx;
    unsigned    i;
    const char* str;

    if (reader->read_file == pdb_jg_read_file)
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
    if (*pdw++ != 0)
    {
        printf("unexpected value\n");
        return -1;
    }

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

static PDB_STRING_TABLE* read_string_table(struct pdb_reader* reader)
{
    unsigned            stream_idx;
    PDB_STRING_TABLE*   ret;
    unsigned            stream_size;

    stream_idx = get_stream_by_name(reader, "/names");
    if (stream_idx == -1) return NULL;
    ret = reader->read_file(reader, stream_idx);
    if (!ret) return NULL;
    stream_size = pdb_get_file_size(reader, stream_idx);
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
    if (size >= sizeof(DBI_HASH_HEADER))
    {
        const DBI_HASH_HEADER* hdr = (const DBI_HASH_HEADER*)root;

        printf("%s%s symbols hash:\n", pfx, name);
        printf("%s\tSignature: 0x%x\n", pfx, hdr->signature);
        printf("%s\tVersion: 0x%x (%u)\n", pfx, hdr->version, hdr->version - 0xeffe0000);
        printf("%s\tSize of hash records: %u\n", pfx, hdr->size_hash_records);
        printf("%s\tUnknown: %u\n", pfx, hdr->unknown);

        if (hdr->signature != 0xFFFFFFFF ||
            hdr->version != 0xeffe0000 + 19990810 ||
            (hdr->size_hash_records % sizeof(DBI_HASH_RECORD)) != 0 ||
            sizeof(DBI_HASH_HEADER) + hdr->size_hash_records + DBI_BITMAP_HASH_SIZE > size ||
            (size - (sizeof(DBI_HASH_HEADER) + hdr->size_hash_records + DBI_BITMAP_HASH_SIZE)) % sizeof(unsigned))
        {
            printf("%s\t\tIncorrect hash structure\n", pfx);
        }
        else
        {
            unsigned i;
            unsigned num_hash_records = hdr->size_hash_records / sizeof(DBI_HASH_RECORD);
            const DBI_HASH_RECORD* hr = (const DBI_HASH_RECORD*)(hdr + 1);
            unsigned* bitmap = (unsigned*)((char*)(hdr + 1) + hdr->size_hash_records);
            unsigned* buckets = (unsigned*)((char*)(hdr + 1) + hdr->size_hash_records + DBI_BITMAP_HASH_SIZE);
            unsigned index, last_index = (size - (sizeof(DBI_HASH_HEADER) + hdr->size_hash_records + DBI_BITMAP_HASH_SIZE)) / sizeof(unsigned);

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
            if (sizeof(DBI_HASH_HEADER) + hdr->size_hash_records + DBI_BITMAP_HASH_SIZE + index * sizeof(unsigned) > size)
            {
                printf("%s-- left over %u bytes\n", pfx,
                       size - (unsigned)(sizeof(DBI_HASH_HEADER) + hdr->size_hash_records + DBI_BITMAP_HASH_SIZE + index * sizeof(unsigned)));
            }
        }
    }
    else
        printf("%sNo header in symbols hash\n", pfx);
}

static void dump_global_symbol(struct pdb_reader* reader, unsigned file)
{
    void*  global = NULL;
    DWORD  size;

    global = reader->read_file(reader, file);
    if (!global) return;

    size = pdb_get_file_size(reader, file);

    dump_dbi_hash_table(global, size, "Global", "");
    free(global);
}

static void dump_public_symbol(struct pdb_reader* reader, unsigned file)
{
    unsigned            size;
    DBI_PUBLIC_HEADER*  hdr;

    hdr = reader->read_file(reader, file);
    if (!hdr) return;

    size = pdb_get_file_size(reader, file);

    printf("Public symbols table: (%u)\n", size);

    printf("\tHash size:              %u\n", hdr->hash_size);
    printf("\tAddress map size:       %u\n", hdr->address_map_size);
    printf("\tNumber of thunks:       %u\n", hdr->num_thunks);
    printf("\tSize of thunk:          %u\n", hdr->size_thunk);
    printf("\tSection of thunk table: %u\n", hdr->section_thunk_table);
    printf("\tOffset of thunk table:  %u\n", hdr->offset_thunk_table);
    printf("\tNumber of sections:     %u\n", hdr->num_sects);

    dump_dbi_hash_table((const BYTE*)(hdr + 1), hdr->hash_size, "Public", "\t");
    free(hdr);
}

static void pdb_dump_symbols(struct pdb_reader* reader, PDB_STREAM_INDEXES* sidx)
{
    PDB_SYMBOLS*        symbols;
    unsigned char*      modimage;
    const char*         file;
    PDB_STRING_TABLE*   filesimage;
    char                tcver[32];

    sidx->FPO = sidx->unk0 = sidx->unk1 = sidx->unk2 = sidx->unk3 = sidx->segments =
        sidx->unk4 = sidx->unk5 = sidx->unk6 = sidx->FPO_EXT = sidx->unk7 = -1;

    symbols = reader->read_file(reader, 3);
    if (!symbols) return;

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
           "\tsignature:        %08x\n"
           "\tversion:          %u\n"
           "\tage:              %08x\n"
           "\tglobal_hash_file: %u\n"
           "\tbuilder:          %s\n"
           "\tpublic_file:      %u\n"
           "\tbldVer:           %u\n"
           "\tgsym_file:        %u\n"
           "\trbldVer:          %u\n"
           "\tmodule_size:      %08x\n"
           "\toffset_size:      %08x\n"
           "\thash_size:        %08x\n"
           "\tsrc_module_size:  %08x\n"
           "\tpdbimport_size:   %08x\n"
           "\tresvd0:           %08x\n"
           "\tstream_idx_size:  %08x\n"
           "\tunknown2_size:    %08x\n"
           "\tresvd3:           %04x\n"
           "\tmachine:          %s\n"
           "\tresvd4            %08x\n",
           symbols->signature,
           symbols->version,
           symbols->age,
           symbols->global_hash_file,
           tcver, /* from symbols->flags */
           symbols->public_file,
           symbols->bldVer,
           symbols->gsym_file,
           symbols->rbldVer,
           symbols->module_size,
           symbols->offset_size,
           symbols->hash_size,
           symbols->srcmodule_size,
           symbols->pdbimport_size,
           symbols->resvd0,
           symbols->stream_index_size,
           symbols->unknown2_size,
           symbols->resvd3,
           get_machine_str( symbols->machine ),
           symbols->resvd4);

    if (symbols->offset_size)
    {
        const BYTE*                 src;

        printf("\t----------offsets------------\n");
        src = (const BYTE*)((const char*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size);
        dump_data(src, symbols->offset_size, "    ");
    }

    if (!(filesimage = read_string_table(reader))) printf("string table not found\n");

    if (symbols->srcmodule_size)
    {
        const PDB_SYMBOL_SOURCE*src;
        int                     i, j, cfile;
        const WORD*             indx;
        const DWORD*            offset;
        const char*             start_cstr;
        const char*             cstr;

        printf("\t----------src module------------\n");
        src = (const PDB_SYMBOL_SOURCE*)((const char*)symbols + sizeof(PDB_SYMBOLS) + 
                                         symbols->module_size + symbols->offset_size + symbols->hash_size);
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
    if (symbols->pdbimport_size)
    {
        const PDB_SYMBOL_IMPORT*  imp;
        const char* first;
        const char* last;
        const char* ptr;

        printf("\t------------import--------------\n");
        imp = (const PDB_SYMBOL_IMPORT*)((const char*)symbols + sizeof(PDB_SYMBOLS) + 
                                         symbols->module_size + symbols->offset_size + 
                                         symbols->hash_size + symbols->srcmodule_size);
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
    if (symbols->stream_index_size)
    {
        printf("\t------------stream indexes--------------\n");
        switch (symbols->stream_index_size)
        {
        case sizeof(PDB_STREAM_INDEXES_OLD):
            /* PDB_STREAM_INDEXES is a superset of PDB_STREAM_INDEX_OLD
             * FIXME: to be confirmed when all fields are fully understood
             */
            memcpy(sidx,
                   (const char*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size +
                   symbols->offset_size + symbols->hash_size + symbols->srcmodule_size +
                   symbols->pdbimport_size + symbols->unknown2_size,
                   sizeof(PDB_STREAM_INDEXES_OLD));
            printf("\tFPO:                  %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\tSegments:             %04x\n",
                   sidx->FPO, sidx->unk0, sidx->unk1, sidx->unk2, sidx->unk3,
                   sidx->segments);
            break;
        case sizeof(PDB_STREAM_INDEXES):
            memcpy(sidx,
                   (const char*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size +
                   symbols->offset_size + symbols->hash_size + symbols->srcmodule_size +
                   symbols->pdbimport_size + symbols->unknown2_size,
                   sizeof(*sidx));
            printf("\tFPO:                  %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\tSegments:             %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\t?:                    %04x\n"
                   "\tFPO-ext:              %04x\n"
                   "\t?:                    %04x\n",
                   sidx->FPO, sidx->unk0, sidx->unk1, sidx->unk2, sidx->unk3,
                   sidx->segments, sidx->unk4, sidx->unk5, sidx->unk6, sidx->FPO_EXT,
                   sidx->unk7);
            break;
        default:
            printf("unexpected size for stream index %d\n", symbols->stream_index_size);
            break;
        }
    }

    /* Read global symbol table */
    modimage = reader->read_file(reader, symbols->gsym_file);
    if (modimage)
    {
        printf("\t------------globals-------------\n"); 
        codeview_dump_symbols(modimage, 0, pdb_get_file_size(reader, symbols->gsym_file));
        free(modimage);
    }

    /* Read per-module symbol / linenumber tables */
    file = (const char*)symbols + sizeof(PDB_SYMBOLS);
    while (file - (const char*)symbols < sizeof(PDB_SYMBOLS) + symbols->module_size)
    {
        int file_nr, symbol_size, lineno_size, lineno2_size;
        const char* file_name;
        const char* lib_name;

        if (symbols->version < 19970000)
        {
            const PDB_SYMBOL_FILE*      sym_file = (const PDB_SYMBOL_FILE*) file;
            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            lib_name    = file_name + strlen(file_name) + 1;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
            lineno2_size = sym_file->lineno2_size;
            printf("\t--------symbol file-----------\n");
            printf("\tName: %s\n", file_name);
            if (strcmp(file_name, lib_name)) printf("\tLibrary: %s\n", lib_name);
            printf("\t\tunknown1:   %08x\n"
                   "\t\trange\n"
                   "\t\t\tsegment:         %04x\n"
                   "\t\t\tpad1:            %04x\n"
                   "\t\t\toffset:          %08x\n"
                   "\t\t\tsize:            %08x\n"
                   "\t\t\tcharacteristics: %08x\n"
                   "\t\t\tindex:           %04x\n"
                   "\t\t\tpad2:            %04x\n"
                   "\t\tflag:       %04x\n"
                   "\t\tfile:       %04x\n"
                   "\t\tsymb size:  %08x\n"
                   "\t\tline size:  %08x\n"
                   "\t\tline2 size:  %08x\n"
                   "\t\tnSrcFiles:  %08x\n"
                   "\t\tattribute:  %08x\n",
                   sym_file->unknown1,
                   sym_file->range.segment,
                   sym_file->range.pad1,
                   sym_file->range.offset,
                   sym_file->range.size,
                   sym_file->range.characteristics,
                   sym_file->range.index,
                   sym_file->range.pad2,
                   sym_file->flag,
                   sym_file->file,
                   sym_file->symbol_size,
                   sym_file->lineno_size,
                   sym_file->lineno2_size,
                   sym_file->nSrcFiles,
                   sym_file->attribute);
        }
        else
        {
            const PDB_SYMBOL_FILE_EX*   sym_file = (const PDB_SYMBOL_FILE_EX*) file;

            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            lib_name    = file_name + strlen(file_name) + 1;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
            lineno2_size = sym_file->lineno2_size;
            printf("\t--------symbol file-----------\n");
            printf("\tName: %s\n", file_name);
            if (strcmp(file_name, lib_name)) printf("\tLibrary: %s\n", lib_name);
            printf("\t\tunknown1:   %08x\n"
                   "\t\trange\n"
                   "\t\t\tsegment:         %04x\n"
                   "\t\t\tpad1:            %04x\n"
                   "\t\t\toffset:          %08x\n"
                   "\t\t\tsize:            %08x\n"
                   "\t\t\tcharacteristics: %08x\n"
                   "\t\t\tindex:           %04x\n"
                   "\t\t\tpad2:            %04x\n"
                   "\t\t\ttimestamp:       %08x\n"
                   "\t\t\tunknown:         %08x\n"
                   "\t\tflag:       %04x\n"
                   "\t\tfile:       %04x\n"
                   "\t\tsymb size:  %08x\n"
                   "\t\tline size:  %08x\n"
                   "\t\tline2 size: %08x\n"
                   "\t\tnSrcFiles:  %08x\n"
                   "\t\tattribute:  %08x\n"
                   "\t\treserved/0: %08x\n"
                   "\t\treserved/1: %08x\n",
                   sym_file->unknown1,
                   sym_file->range.segment,
                   sym_file->range.pad1,
                   sym_file->range.offset,
                   sym_file->range.size,
                   sym_file->range.characteristics,
                   sym_file->range.index,
                   sym_file->range.pad2,
                   sym_file->range.timestamp,
                   sym_file->range.unknown,
                   sym_file->flag,
                   sym_file->file,
                   sym_file->symbol_size,
                   sym_file->lineno_size,
                   sym_file->lineno2_size,
                   sym_file->nSrcFiles,
                   sym_file->attribute,
                   sym_file->reserved[0],
                   sym_file->reserved[1]);
        }
        modimage = reader->read_file(reader, file_nr);
        if (modimage)
        {
            int total_size = pdb_get_file_size(reader, file_nr);

            if (symbol_size)
                codeview_dump_symbols((const char*)modimage, sizeof(DWORD), symbol_size);

            /* line number info */
            if (lineno_size)
                codeview_dump_linetab((const char*)modimage + symbol_size, TRUE, "        ");
            else if (lineno2_size) /* actually, only one of the 2 lineno should be present */
                codeview_dump_linetab2((const char*)modimage + symbol_size, lineno2_size,
                                       filesimage, "        ");
            /* what's that part ??? */
            if (0)
                dump_data(modimage + symbol_size + lineno_size + lineno2_size,
                          total_size - (symbol_size + lineno_size + lineno2_size), "    ");
            free(modimage);
        }

        file = (char*)((DWORD_PTR)(lib_name + strlen(lib_name) + 1 + 3) & ~3);
    }
    dump_global_symbol(reader, symbols->global_hash_file);
    dump_public_symbol(reader, symbols->public_file);

    free(symbols);
    free(filesimage);
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
    PDB_STRING_TABLE* strbase;
    unsigned *collision;
    hash = reader->read_file(reader, types->hash_file);
    if (!hash) return;

    printf("Types (%s) hash:\n", strmname);
    strmsize = pdb_get_file_size(reader, types->hash_file);
    if (types->hash_offset + types->hash_len > strmsize ||
        (types->last_index - types->first_index) * types->hash_size != types->hash_len ||
        types->search_offset + types->search_len > strmsize ||
        types->type_remap_offset + types->type_remap_len > strmsize)
    {
        printf("\nIncoherent sizes... skipping\n");
        return;
    }
    printf("\n\tIndexes => hash value:\n");
    for (i = types->first_index; i < types->last_index; i++)
    {
        printf("\t\t%08x => ", i);
        pdb_dump_hash_value((const BYTE*)hash + types->hash_offset + (i - types->first_index) * types->hash_size, types->hash_size);
        printf("\n");
    }
    /* print collisions in hash table (if any) */
    collision = malloc((types->last_index - types->first_index) * sizeof(unsigned));
    if (collision)
    {
        unsigned head_printed = 0;

        collision_arg.hash = (const BYTE*)hash + types->hash_offset;
        collision_arg.hash_size = types->hash_size;

        for (i = 0; i < types->last_index - types->first_index; i++) collision[i] = i;
        qsort(collision, types->last_index - types->first_index, sizeof(unsigned), collision_compar);
        for (i = 0; i < types->last_index - types->first_index; i++)
        {
            unsigned j;
            for (j = i + 1; j < types->last_index - types->first_index; j++)
                if (memcmp((const BYTE*)hash + types->hash_offset + collision[i] * types->hash_size,
                           (const BYTE*)hash + types->hash_offset + collision[j] * types->hash_size,
                           types->hash_size))
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
                pdb_dump_hash_value((const BYTE*)hash + types->hash_offset + collision[i] * types->hash_size, types->hash_size);
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
    for (i = 0; i < types->search_len / (2 * sizeof(unsigned)); i += 2)
    {
        printf("\t\t%08x => %08x\n", table[2 * i + 0], table[2 * i + 1]);
    }
    if (types->type_remap_len && (strbase = read_string_table(reader)))
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
                printf(" %s => ", pdb_get_string_table_entry(strbase, *table++));
                pdb_dump_hash_value((const BYTE*)table, types->hash_size);
                table = (const unsigned*)((const BYTE*)table + types->hash_size);
            }
            printf("\n");
        }
        free(strbase);
        printf("\n");
    }
    free(hash);
}

/* there are two 'type' related streams, but with different indexes... */
static void pdb_dump_types(struct pdb_reader* reader, unsigned strmidx, const char* strmname)
{
    PDB_TYPES*  types = NULL;
    BOOL used = has_file_been_read(reader, strmidx);

    if (pdb_get_file_size(reader, strmidx) < sizeof(*types))
    {
        if (strmidx == 2)
            printf("-Too small type header\n");
        return;
    }
    types = reader->read_file(reader, strmidx);
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
        if (used) clear_file_been_read(reader, strmidx);
        return;
    }

    /* Read type table */
    printf("Types (%s):\n"
           "\tversion:           %u\n"
           "\ttype_offset:       %08x\n"
           "\tfirst_index:       %x\n"
           "\tlast_index:        %x\n"
           "\ttype_size:         %x\n"
           "\thash_file:         %x\n"
           "\tpad:               %x\n"
           "\thash_size:         %x\n"
           "\thash_buckets       %x\n"
           "\thash_offset:       %x\n"
           "\thash_len:          %x\n"
           "\tsearch_offset:     %x\n"
           "\tsearch_len:        %x\n"
           "\ttype_remap_offset: %x\n"
           "\ttype_remap_len:    %x\n",
           strmname,
           types->version,
           types->type_offset,
           types->first_index,
           types->last_index,
           types->type_size,
           types->hash_file,
           types->pad,
           types->hash_size,
           types->hash_num_buckets,
           types->hash_offset,
           types->hash_len,
           types->search_offset,
           types->search_len,
           types->type_remap_offset,
           types->type_remap_len);
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
    fpo = reader->read_file(reader, stream_idx);
    size = pdb_get_file_size(reader, stream_idx);
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
    PDB_STRING_TABLE*   strbase;

    if (stream_idx == (WORD)-1) return;
    strbase = read_string_table(reader);
    if (!strbase) return;

    fpoext = reader->read_file(reader, stream_idx);
    size = pdb_get_file_size(reader, stream_idx);
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
                   pdb_get_string_table_entry(strbase, fpoext[i].str_offset));
        }
    }
    free(fpoext);
    free(strbase);
}

static void pdb_dump_segments(struct pdb_reader* reader, unsigned stream_idx)
{
    const char* segs;
    DWORD       size;
    const char* ptr;

    if (stream_idx == (WORD)-1) return;
    segs = reader->read_file(reader, stream_idx);

    if (segs)
    {
        size = pdb_get_file_size(reader, stream_idx);
        for (ptr = segs; ptr < segs + size; )
        {
            printf("Segment %s\n", ptr);
            ptr += (strlen(ptr) + 1 + 3) & ~3;
            printf("\tdword[0]: %08x\n", *(UINT *)ptr); ptr += 4;
            printf("\tdword[1]: %08x\n", *(UINT *)ptr); ptr += 4;
            printf("\tdword[2]: %08x\n", *(UINT *)ptr); ptr += 4;
            printf("\tdword[3]: %08x\n", *(UINT *)ptr); ptr += 4;
            printf("\tdword[4]: %08x\n", *(UINT *)ptr); ptr += 4;
            printf("\tdword[5]: %08x\n", *(UINT *)ptr); ptr += 4;
            printf("\tdword[6]: %08x\n", *(UINT *)ptr); ptr += 4;
            printf("\tdword[7]: %08x\n", *(UINT *)ptr); ptr += 4;
        }
        free((char*)segs);
    } else printf("nosdfsdffd\n");
}

static const char       pdb2[] = "Microsoft C/C++ program database 2.00";

static void pdb_jg_dump(void)
{
    struct pdb_reader   reader;

    /*
     * Read in TOC and well-known files
     */
    pdb_jg_init(&reader);
    printf("Header (JG):\n"
           "\tident:      %.*s\n"
           "\tsignature:  %08x\n"
           "\tblock_size: %08x\n"
           "\tfree_list:  %04x\n"
           "\ttotal_alloc:%04x\n",
           (int)sizeof(pdb2) - 1, reader.u.jg.header->ident,
           reader.u.jg.header->signature,
           reader.u.jg.header->block_size,
           reader.u.jg.header->free_list,
           reader.u.jg.header->total_alloc);

    reader.u.jg.root = reader.read_file(&reader, 1);
    if (reader.u.jg.root)
    {
        UINT *pdw, *ok_bits;
        UINT i, numok, count;
        PDB_STREAM_INDEXES sidx;

        printf("Root:\n"
               "\tVersion:       %u\n"
               "\tTimeDateStamp: %08x\n"
               "\tAge:           %08x\n"
               "\tnames:         %d\n",
               reader.u.jg.root->Version,
               reader.u.jg.root->TimeDateStamp,
               reader.u.jg.root->Age,
               (unsigned)reader.u.jg.root->cbNames);

        pdw = (UINT *)(reader.u.jg.root->names + reader.u.jg.root->cbNames);
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
        if (*pdw++ != 0)
        {
            printf("unexpected value\n");
            return;
        }

        for (i = 0; i < count; i++)
        {
            if (ok_bits[i / 32] & (1 << (i % 32)))
            {
                UINT string_idx, stream_idx;
                string_idx = *pdw++;
                stream_idx = *pdw++;
                printf("\t\t\t%2d) %-20s => %x\n", i, &reader.u.jg.root->names[string_idx], stream_idx);
                numok--;
            }
        }
        if (numok) printf(">>> unmatched present field with found\n");

        /* Check for unknown versions */
        switch (reader.u.jg.root->Version)
        {
        case 19950623:      /* VC 4.0 */
        case 19950814:
        case 19960307:      /* VC 5.0 */
        case 19970604:      /* VC 6.0 */
            break;
        default:
            printf("-Unknown root block version %d\n", reader.u.jg.root->Version);
        }
        pdb_dump_types(&reader, 2, "TPI");
        pdb_dump_types(&reader, 4, "IPI");
        pdb_dump_symbols(&reader, &sidx);
        pdb_dump_fpo(&reader, sidx.FPO);
        pdb_dump_segments(&reader, sidx.segments);
    }
    else printf("-Unable to get root\n");

    pdb_exit(&reader);
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

static void* pdb_ds_read_file(struct pdb_reader* reader, DWORD file_number)
{
    const UINT *block_list;
    UINT i;

    if (!reader->u.ds.toc || file_number >= reader->u.ds.toc->num_files) return NULL;

    mark_file_been_read(reader, file_number);
    if (reader->u.ds.toc->file_size[file_number] == 0 ||
        reader->u.ds.toc->file_size[file_number] == 0xFFFFFFFF)
        return NULL;
    block_list = reader->u.ds.toc->file_size + reader->u.ds.toc->num_files;
    for (i = 0; i < file_number; i++)
        block_list += (reader->u.ds.toc->file_size[i] + reader->u.ds.header->block_size - 1) /
            reader->u.ds.header->block_size;

    return pdb_ds_read(reader->u.ds.header, block_list, reader->u.ds.toc->file_size[file_number]);
}

static BOOL pdb_ds_init(struct pdb_reader* reader)
{
    reader->u.ds.header = PRD(0, sizeof(*reader->u.ds.header));
    if (!reader->u.ds.header) return FALSE;
    reader->read_file = pdb_ds_read_file;
    reader->u.ds.toc = pdb_ds_read(reader->u.ds.header,
                                   (const UINT *)((const char*)reader->u.ds.header + reader->u.ds.header->toc_page * reader->u.ds.header->block_size),
                                   reader->u.ds.header->toc_size);
    memset(reader->file_used, 0, sizeof(reader->file_used));
    return TRUE;
}

static const char       pdb7[] = "Microsoft C/C++ MSF 7.00";

static void pdb_ds_dump(void)
{
    struct pdb_reader   reader;

    pdb_ds_init(&reader);
    printf("Header (DS)\n"
           "\tsignature:        %.*s\n"
           "\tblock_size:       %08x\n"
           "\tunknown1:         %08x\n"
           "\tnum_pages:        %08x\n"
           "\ttoc_size:         %08x\n"
           "\tunknown2:         %08x\n"
           "\ttoc_page:         %08x\n",
           (int)sizeof(pdb7) - 1, reader.u.ds.header->signature,
           reader.u.ds.header->block_size,
           reader.u.ds.header->unknown1,
           reader.u.ds.header->num_pages,
           reader.u.ds.header->toc_size,
           reader.u.ds.header->unknown2,
           reader.u.ds.header->toc_page);

    /* files with static indexes:
     *  0: JG says old toc pages
     *  1: root structure
     *  2: types
     *  3: modules
     *  4: types (second stream)
     * other known streams:
     * - string table: its index is in the stream table from ROOT object under "/names"
     * - type hash table: its index is in the types header (2 and 4)
     * - global and public streams: from symbol stream header
     * those streams get their indexes out of the PDB_STREAM_INDEXES object
     * - FPO data
     * - segments
     * - extended FPO data
     */
    mark_file_been_read(&reader, 0); /* mark stream #0 as read */
    reader.u.ds.root = reader.read_file(&reader, 1);
    if (reader.u.ds.root)
    {
        UINT *pdw, *ok_bits;
        UINT i, numok, count;
        PDB_STREAM_INDEXES sidx;

        printf("Root:\n"
               "\tVersion:              %u\n"
               "\tTimeDateStamp:        %08x\n"
               "\tAge:                  %08x\n"
               "\tguid                  %s\n"
               "\tcbNames:              %08x\n",
               reader.u.ds.root->Version,
               reader.u.ds.root->TimeDateStamp,
               reader.u.ds.root->Age,
               get_guid_str(&reader.u.ds.root->guid),
               reader.u.ds.root->cbNames);
        pdw = (UINT *)(reader.u.ds.root->names + reader.u.ds.root->cbNames);
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
        if (*pdw++ != 0)
        {
            printf("unexpected value\n");
            return;
        }

        for (i = 0; i < count; i++)
        {
            if (ok_bits[i / 32] & (1 << (i % 32)))
            {
                UINT string_idx, stream_idx;
                string_idx = *pdw++;
                stream_idx = *pdw++;
                printf("\t\t\t%2d) %-20s => %x\n", i, &reader.u.ds.root->names[string_idx], stream_idx);
                numok--;
            }
        }
        if (numok) printf(">>> unmatched present field with found\n");

        pdb_dump_types(&reader, 2, "TPI");
        pdb_dump_types(&reader, 4, "IPI");
        pdb_dump_symbols(&reader, &sidx);
        pdb_dump_fpo(&reader, sidx.FPO);
        pdb_dump_fpo_ext(&reader, sidx.FPO_EXT);
        pdb_dump_segments(&reader, sidx.segments);
    }
    else printf("-Unable to get root\n");

    pdb_exit(&reader);
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
    const char* head;

/*    init_types(); */
    head = PRD(0, sizeof(pdb2) - 1);
    if (head && !memcmp(head, pdb2, sizeof(pdb2) - 1))
    {
        pdb_jg_dump();
        return;
    }
    head = PRD(0, sizeof(pdb7) - 1);
    if (head && !memcmp(head, pdb7, sizeof(pdb7) - 1))
    {
        pdb_ds_dump();
        return;
    }
    printf("Unrecognized header %s\n", head);
}
