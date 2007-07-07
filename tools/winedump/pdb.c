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
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winedump.h"
#include "wine/mscvpdb.h"

struct pdb_reader
{
    union
    {
        struct
        {
            const struct PDB_JG_HEADER* header;
            const struct PDB_JG_TOC*    toc;
        } jg;
        struct
        {
            const struct PDB_DS_HEADER* header;
            const struct PDB_DS_TOC*    toc;
        } ds;
    } u;
    void*       (*read_file)(struct pdb_reader*, DWORD);
    DWORD       file_used[1024];
};

static void* pdb_jg_read(const struct PDB_JG_HEADER* pdb, const WORD* block_list, int size)
{
    int                 i, nBlocks;
    BYTE*               buffer;

    if (!size) return NULL;

    nBlocks = (size + pdb->block_size - 1) / pdb->block_size;
    buffer = malloc(nBlocks * pdb->block_size);

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

    reader->file_used[file_nr / 32] |= 1 << (file_nr % 32);
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
#if 1
    unsigned            i;
    unsigned char*      file;
    DWORD               size;

    for (i = 0; i < pdb_get_num_files(reader); i++)
    {
        if (reader->file_used[i / 32] & (1 << (i % 32))) continue;

        file = reader->read_file(reader, i);
        if (!file) continue;

        size = pdb_get_file_size(reader, i);

        printf("File --unused-- #%d (%x)\n", i, size);
        dump_data(file, size, "    ");
        free(file);
    }
#endif
    if (reader->read_file == pdb_jg_read_file)
        free((char*)reader->u.jg.toc);
    else
        free((char*)reader->u.ds.toc);
}

static void pdb_dump_symbols(struct pdb_reader* reader)
{
    PDB_SYMBOLS*    symbols;
    unsigned char*  modimage;
    const char*     file;

    symbols = reader->read_file(reader, 3);

    if (!symbols) return;

    switch (symbols->version)
    {
    case 0:            /* VC 4.0 */
    case 19960307:     /* VC 5.0 */
    case 19970606:     /* VC 6.0 */
        break;
    default:
        printf("-Unknown symbol info version %d\n", symbols->version);
    }
    printf("Symbols:\n"
           "\tsignature:      %08x\n"
           "\tversion:        %u\n"
           "\tunknown:        %08x\n"
           "\thash1_file:     %08x\n"
           "\thash2_file:     %08x\n"
           "\tgsym_file:      %08x\n"
           "\tmodule_size:    %08x\n"
           "\toffset_size:    %08x\n"
           "\thash_size:      %08x\n"
           "\tsrc_module_size %08x\n"
           "\tpdbimport_size  %08x\n",
           symbols->signature,
           symbols->version,
           symbols->unknown,
           symbols->hash1_file,
           symbols->hash2_file,
           symbols->gsym_file,
           symbols->module_size,
           symbols->offset_size,
           symbols->hash_size,
           symbols->srcmodule_size,
           symbols->pdbimport_size);

    if (symbols->offset_size)
    {
        const BYTE*                 src;

        printf("\t----------offsets------------\n");
        src = (const BYTE*)((const char*)symbols + sizeof(PDB_SYMBOLS) + symbols->module_size);
        dump_data(src, symbols->offset_size, "    ");
    }

    if (symbols->srcmodule_size)
    {
        const PDB_SYMBOL_SOURCE*src;
        int                     i;
        const DWORD*            offset;
        const char*             cstr;

        printf("\t----------src module------------\n");
        src = (const PDB_SYMBOL_SOURCE*)((const char*)symbols + sizeof(PDB_SYMBOLS) + 
                                         symbols->module_size + symbols->offset_size + symbols->hash_size);
        printf("\tSource Modules\n"
               "\t\tnModules:         %u\n"
               "\t\tnSrcFiles:        %u\n"
               "\t\ttable:\n",
               src->nModules, src->nSrcFiles);

        /* usage of table seems to be as follows:
         * two arrays of WORD (src->nModules as size)
         *  - first array contains index into files for "module" compilation (module = compilation unit ??)
         *  - second array contains increment in index (if needed)
         *  - usage of this later is not very clear. it could be that if second array entry is null, then no file
         *    name is to be used ?
         * an array of DWORD (src->nSrcFiles as size)
         *  - contains offset (in following string table) of the source file name
         * a string table
         * - each string is a pascal string (ie. with its length as first BYTE) or
         *   0-terminated string (depending on version)
         */

        offset = (const DWORD*)&src->table[2 * src->nModules];
        cstr = (const char*)&src->table[2 * (src->nModules + src->nSrcFiles)];

        for (i = 0; i < src->nModules; i++)
        {
            /* FIXME: in some cases, it's a p_string but WHEN ? */
            if (cstr + offset[src->table[i]] < (const char*)src + symbols->srcmodule_size)
                printf("\t\t\tmodule[%2d]: src=%s (%04x)\n",
                       i, cstr + offset[src->table[i]], src->table[src->nModules + i]);
            else
                printf("\t\t\tmodule[%2d]: src=<<out of bounds>> (%04x)\n",
                       i, src->table[src->nModules + i]);
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
                   (ULONG_PTR)((const char*)imp - (const char*)first),
                   imp->unknown1,
                   imp->unknown2,
                   imp->TimeDateStamp,
                   imp->Age,
                   imp->filename,
                   ptr);
            imp = (const PDB_SYMBOL_IMPORT*)(first + ((ptr - first + strlen(ptr) + 1 + 3) & ~3));
        }
    }

    /* Read global symbol table */
    modimage = reader->read_file(reader, symbols->gsym_file);
    if (modimage)
    {
        printf("\t------------globals-------------\n"); 
        codeview_dump_symbols(modimage, pdb_get_file_size(reader, symbols->gsym_file));
        free(modimage);
    }

    /* Read per-module symbol / linenumber tables */
    file = (const char*)symbols + sizeof(PDB_SYMBOLS);
    while (file - (const char*)symbols < sizeof(PDB_SYMBOLS) + symbols->module_size)
    {
        int file_nr, symbol_size, lineno_size;
        const char* file_name;
            
        if (symbols->version < 19970000)
        {
            const PDB_SYMBOL_FILE*      sym_file = (const PDB_SYMBOL_FILE*) file;
            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
            printf("\t--------symbol file----------- %s\n", file_name);
            printf("\tgot symbol_file\n"
                   "\t\tunknown1:   %08x \n"
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
                   "\t\tunknown2:   %08x\n"
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
                   sym_file->unknown2,
                   sym_file->nSrcFiles,
                   sym_file->attribute);
        }
        else
        {
            const PDB_SYMBOL_FILE_EX*   sym_file = (const PDB_SYMBOL_FILE_EX*) file;
            file_nr     = sym_file->file;
            file_name   = sym_file->filename;
            symbol_size = sym_file->symbol_size;
            lineno_size = sym_file->lineno_size;
            printf("\t--------symbol file----------- %s\n", file_name);
            printf("\t\tunknown1:   %08x \n"
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
                   "\t\tunknown2:   %08x\n"
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
                   sym_file->unknown2,
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
                codeview_dump_symbols((const char*)modimage + sizeof(DWORD), symbol_size);

            /* what's that part ??? */
            if (0)
                dump_data(modimage + symbol_size + lineno_size, total_size - (symbol_size + lineno_size), "    ");
            free(modimage);
        }

        file_name += strlen(file_name) + 1;
        file = (char*)((DWORD_PTR)(file_name + strlen(file_name) + 1 + 3) & ~3);
    }
    free(symbols);
}

static void pdb_dump_types(struct pdb_reader* reader)
{
    PDB_TYPES*  types = NULL;

    types = reader->read_file(reader, 2);

    switch (types->version)
    {
    case 19950410:      /* VC 4.0 */
    case 19951122:
    case 19961031:      /* VC 5.0 / 6.0 */
    case 19990903:      /* VC 7.0 */
        break;
    default:
        printf("-Unknown type info version %d\n", types->version);
    }

    /* Read type table */
    printf("Types:\n"
           "\tversion:        %u\n"
           "\ttype_offset:    %08x\n"
           "\tfirst_index:    %x\n"
           "\tlast_index:     %x\n"
           "\ttype_size:      %x\n"
           "\tfile:           %x\n"
           "\tpad:            %x\n"
           "\thash_size:      %x\n"
           "\thash_base:      %x\n"
           "\thash_offset:    %x\n"
           "\thash_len:       %x\n"
           "\tsearch_offset:  %x\n"
           "\tsearch_len:     %x\n"
           "\tunknown_offset: %x\n"
           "\tunknown_len:    %x\n",
           types->version,
           types->type_offset,
           types->first_index,
           types->last_index,
           types->type_size,
           types->file,
           types->pad,
           types->hash_size,
           types->hash_base,
           types->hash_offset,
           types->hash_len,
           types->search_offset,
           types->search_len,
           types->unknown_offset,
           types->unknown_len);
    codeview_dump_types_from_block((const char*)types + types->type_offset, types->type_size);
    free(types);
}

static const char       pdb2[] = "Microsoft C/C++ program database 2.00";

static void pdb_jg_dump(void)
{
    struct pdb_reader   reader;
    struct PDB_JG_ROOT* root = NULL;

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

    root = reader.read_file(&reader, 1);
    
    if (root)
    {
        printf("Root:\n"
               "\tVersion:       %u\n"
               "\tTimeDateStamp: %08x\n"
               "\tAge:           %08x\n"
               "\tnames:         %.*s\n",
               root->Version,
               root->TimeDateStamp,
               root->Age,
               (unsigned)root->cbNames,
               root->names);

        /* Check for unknown versions */
        switch (root->Version)
        {
        case 19950623:      /* VC 4.0 */
        case 19950814:
        case 19960307:      /* VC 5.0 */
        case 19970604:      /* VC 6.0 */
            break;
        default:
            printf("-Unknown root block version %d\n", root->Version);
        }
        free(root);
    }
    else printf("-Unable to get root\n");

    pdb_dump_types(&reader);
#if 0
    /* segments info, index is unknown */
    {
        const void*     segs = pdb_read_file(pdb, toc, 8); /* FIXME which index ??? */
        const void*     ptr = segs;

        if (segs) while (ptr < segs + toc->file[8].size)
        {
            printf("Segment %s\n", (const char*)ptr);
            ptr += (strlen(ptr) + 1 + 3) & ~3;
            printf("\tdword[0]: %08lx\n", *(DWORD*)ptr); ptr += 4;
            printf("\tdword[1]: %08lx\n", *(DWORD*)ptr); ptr += 4;
            printf("\tdword[2]: %08lx\n", *(DWORD*)ptr); ptr += 4;
            printf("\tdword[3]: %08lx\n", *(DWORD*)ptr); ptr += 4;
            printf("\tdword[4]: %08lx\n", *(DWORD*)ptr); ptr += 4;
            printf("\tdword[5]: %08lx\n", *(DWORD*)ptr); ptr += 4;
            printf("\tdword[6]: %08lx\n", *(DWORD*)ptr); ptr += 4;
            printf("\tdword[7]: %08lx\n", *(DWORD*)ptr); ptr += 4;
        }
        free(segs);
    }
#endif

    pdb_dump_symbols(&reader);
    pdb_exit(&reader);
}

static void* pdb_ds_read(const struct PDB_DS_HEADER* header, const DWORD* block_list, int size)
{
    int                 i, nBlocks;
    BYTE*               buffer;

    if (!size) return NULL;

    nBlocks = (size + header->block_size - 1) / header->block_size;
    buffer = malloc(nBlocks * header->block_size);

    for (i = 0; i < nBlocks; i++)
        memcpy(buffer + i * header->block_size,
               (const char*)header + block_list[i] * header->block_size, header->block_size);

    return buffer;
}

static void* pdb_ds_read_file(struct pdb_reader* reader, DWORD file_number)
{
    const DWORD*        block_list;
    DWORD               i;

    if (!reader->u.ds.toc || file_number >= reader->u.ds.toc->num_files) return NULL;

    reader->file_used[file_number / 32] |= 1 << (file_number % 32);
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
                                   (const DWORD*)((const char*)reader->u.ds.header + reader->u.ds.header->toc_page * reader->u.ds.header->block_size),
                                   reader->u.ds.header->toc_size);
    memset(reader->file_used, 0, sizeof(reader->file_used));
    return TRUE;
}

static const char       pdb7[] = "Microsoft C/C++ MSF 7.00";

static void pdb_ds_dump(void)
{
    struct pdb_reader   reader;
    struct PDB_DS_ROOT* root;

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

    /* files:
     * 0: JG says old toc pages, I'd say free pages (tbc, low prio)
     * 1: root structure
     * 2: types
     * 3: modules
     */
    root = reader.read_file(&reader, 1);
    if (root)
    {
        const char*     ptr;

        printf("Root:\n"
               "\tVersion:              %u\n"
               "\tTimeDateStamp:        %08x\n"
               "\tAge:                  %08x\n"
               "\tguid                  %s\n"
               "\tcbNames:              %08x\n",
               root->Version,
               root->TimeDateStamp,
               root->Age,
               get_guid_str(&root->guid),
               root->cbNames);
        for (ptr = &root->names[0]; ptr < &root->names[0] + root->cbNames; ptr += strlen(ptr) + 1)
            printf("\tString:               %s\n", ptr);
        /* follows an unknown list of DWORDs */
        free(root);
    }
    else printf("-Unable to get root\n");

    pdb_dump_types(&reader);
    pdb_dump_symbols(&reader);

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
