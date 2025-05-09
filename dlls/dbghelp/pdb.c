/*
 * File pdb.c - read debug information out of PDB files.
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004-2009, Eric Pouech.
 * Copyright (C) 2004-2025, Eric Pouech for CodeWeavers.
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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"

#include "wine/exception.h"
#include "wine/debug.h"
#include "dbghelp_private.h"
#include "wine/mscvpdb.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_pdb);

/* Note: this file contains the new implementation for reading PDB files.
 * msc.c contains the old implementation.
 */

/*========================================================================
 * PDB reader.
 * Design goal:
 * - maximize on-the-fly operations (doesn't use dbghelp internal representation)
 * - limit loaded and cached memory size
 * Limitations:
 * - doesn't support old JG format (could be added, but to be proven worthwile)
 */

/* Note:
 * - we use integer with known size to replicate serialized data inside the PDB file
 *   and plain integers for the rest of the code.
 * - except for file offset and stream offsets
 * - functions prefixed with pdb_reader_internal are for internal helpers and shouldn't be used
 */

/* some internal types */
typedef uint64_t pdboff_t; /* offset in whole PDB file (64bit) */
typedef uint32_t pdbsize_t; /* size inside a stream (including offset from beg of stream) (2G max) */

struct pdb_reader;
typedef enum pdb_result (*pdb_reader_fetch_block_t)(struct pdb_reader *pdb, unsigned block_no, void **block_buffer);

struct pdb_reader_walker
{
    unsigned stream_id;
    pdbsize_t offset;
    pdbsize_t last;
};

struct pdb_type_details
{
    pdbsize_t stream_offset; /* inside TPI stream */
    cv_typ_t resolved_cv_typeid;
};

enum pdb_action_type
{
    action_type_cv_typ_t, /* cv_typ_t or cv_typ16_t (depending on size) */
    action_type_field,    /* union codeview_fieldtype */
    action_type_globals,  /* union codeview_symbol in DBI's globals stream */
};

struct pdb_action_entry
{
    enum pdb_action_type action_type : 2;
    unsigned             action_length : 14;
    pdbsize_t            stream_offset;
    symref_t             container_symref;
};

struct pdb_type_hash_entry
{
    cv_typ_t cv_typeid;
    struct pdb_type_hash_entry *next;
};

struct pdb_dbi_hash_entry
{
    pdbsize_t dbi_stream_offset;
    struct pdb_dbi_hash_entry *next;
};

struct pdb_compiland
{
    pdbsize_t stream_offset; /* in DBI stream for compiland description */
    unsigned short are_symbols_loaded;
    unsigned short stream_id; /* for all symbols of given compiland */
    struct symt_compiland* compiland;
};

struct pdb_reader
{
    struct module *module;
    HANDLE file;
    /* using ad hoc pool (not the module one), so that we can measure memory of each PDB reader during transition */
    struct pool pool;

    /* header */
    unsigned block_size;
    struct PDB_DS_TOC *toc;

    /* stream information */
    struct
    {
        const uint32_t *blocks; /* points into toc */
        char *name;
    } *streams;
    char *stream_names;

    unsigned source_listed : 1,
        TPI_types_invalid : 1,
        IPI_types_invalid : 1;

    /* types management */
    PDB_TYPES tpi_header;
    struct pdb_reader_walker tpi_types_walker;
    struct pdb_type_details *tpi_typemap; /* from first to last */
    struct pdb_type_hash_entry *tpi_types_hash;

    PDB_TYPES ipi_header;
    struct pdb_reader_walker ipi_walker;

    /* symbol (and types) management */
    PDB_SYMBOLS dbi_header;
    unsigned num_action_globals;
    unsigned num_action_entries;
    struct pdb_action_entry *action_store;
    struct pdb_dbi_hash_entry *dbi_symbols_hash;

    /* compilands */
    unsigned num_compilands;
    struct pdb_compiland *compilands;

    /* cache PE module sections for mapping...
     * we should rather use pe_module information
     */
    const IMAGE_SECTION_HEADER *sections;
    unsigned num_sections;

    /* PDB file access */
    pdb_reader_fetch_block_t fetch;
    struct {unsigned block_no; unsigned age;} cache[4*4];
    char *fetch_cache_blocks;
};

enum pdb_result
{
    R_PDB_SUCCESS,
    R_PDB_IOERROR,
    R_PDB_OUT_OF_MEMORY,
    R_PDB_INVALID_ARGUMENT,
    R_PDB_INVALID_PDB_FILE,
    R_PDB_MISSING_INFORMATION,
    R_PDB_NOT_FOUND,
    R_PDB_BUFFER_TOO_SMALL,
};

#define PDB_REPORT_UNEXPECTED(k,i) pdb_reader_report_unexpected(k, __FUNCTION__, (i))
static enum pdb_result pdb_reader_report_unexpected(const char *kind, const char *function, unsigned id)
{
    WARN("%s: unexpected %s %x\n", function, kind, id);
    return R_PDB_INVALID_PDB_FILE;
}

static const unsigned short  PDB_STREAM_TPI = 2;
static const unsigned short  PDB_STREAM_DBI = 3;
static const unsigned short  PDB_STREAM_IPI = 4;

static enum pdb_result pdb_reader_fetch_file_no_cache(struct pdb_reader *pdb, void *buffer, pdboff_t offset, pdbsize_t size)
{
    OVERLAPPED ov = {.Offset = offset, .OffsetHigh = offset >> 32, .hEvent = (HANDLE)(DWORD_PTR)1};
    DWORD num_read;

    return ReadFile(pdb->file, buffer, size, &num_read, &ov) && num_read == size ? R_PDB_SUCCESS : R_PDB_IOERROR;
}

static enum pdb_result pdb_reader_fetch_block_from_file(struct pdb_reader *pdb, unsigned block_no, void **buffer)
{
    enum pdb_result result;
    unsigned i;
    unsigned lru = ARRAY_SIZE(pdb->cache), found = ARRAY_SIZE(pdb->cache);

    for (i = 0; i < ARRAY_SIZE(pdb->cache); i++)
    {
        if (pdb->cache[i].block_no == block_no)
            found = i;
        else
        {
            pdb->cache[i].age++;
            if (lru == ARRAY_SIZE(pdb->cache) || pdb->cache[lru].age < pdb->cache[i].age)
                lru = i;
        }
    }
    if (found == ARRAY_SIZE(pdb->cache))
    {
        if ((result = pdb_reader_fetch_file_no_cache(pdb, pdb->fetch_cache_blocks + lru * pdb->block_size,
                                                     (pdboff_t)block_no * pdb->block_size, pdb->block_size))) return result;
        pdb->cache[lru].block_no = block_no;
        found = lru;
    }

    pdb->cache[found].age = 0;
    *buffer = pdb->fetch_cache_blocks + found * pdb->block_size;
    return R_PDB_SUCCESS;
}

static const char       PDB_JG_IDENT[] = "Microsoft C/C++ program database 2.00\r\n\032JG\0";
static const char       PDB_DS_IDENT[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0";

static enum pdb_result pdb_reader_get_segment_address(struct pdb_reader *pdb, unsigned segment, unsigned offset, DWORD64 *address)
{
    if (!segment || segment > pdb->num_sections) return R_PDB_INVALID_PDB_FILE;
    *address = pdb->module->module.BaseOfImage +
        pdb->sections[segment - 1].VirtualAddress + offset;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_get_segment_offset_from_address(struct pdb_reader *pdb, DWORD64 address, unsigned *segment, unsigned *offset)
{
    unsigned i;

    for (i = 0; i < pdb->num_sections; i++)
    {
        if (address >= pdb->module->module.BaseOfImage + pdb->sections[i].VirtualAddress &&
            address < pdb->module->module.BaseOfImage + pdb->sections[i].VirtualAddress + pdb->sections[i].Misc.VirtualSize)
        {
            *segment = i + 1;
            *offset = address - (pdb->module->module.BaseOfImage + pdb->sections[i].VirtualAddress);
            return R_PDB_SUCCESS;
        }
    }
    return R_PDB_NOT_FOUND;
}

static inline enum pdb_result pdb_reader_alloc(struct pdb_reader *pdb, size_t size, void **ptr)
{
    return (*ptr = pool_alloc(&pdb->pool, size)) ? R_PDB_SUCCESS : R_PDB_OUT_OF_MEMORY;
}

static inline enum pdb_result pdb_reader_realloc(struct pdb_reader *pdb, void **ptr, size_t size)
{
    void *new = pool_realloc(&pdb->pool, *ptr, size);
    if (!new) return R_PDB_OUT_OF_MEMORY;
    *ptr = new;
    return R_PDB_SUCCESS;
}

static inline void pdb_reader_free(struct pdb_reader *pdb, void *ptr)
{
    pool_free(&pdb->pool, ptr);
}

static inline unsigned pdb_reader_num_blocks(struct pdb_reader *pdb, pdbsize_t size)
{
    return (size + pdb->block_size - 1) / pdb->block_size;
}

static enum pdb_result pdb_reader_get_stream_size(struct pdb_reader *pdb, unsigned stream_id, pdbsize_t *size)
{
    if (stream_id >= pdb->toc->num_streams) return R_PDB_INVALID_ARGUMENT;
    *size = pdb->toc->stream_size[stream_id];
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_internal_read_from_blocks(struct pdb_reader *pdb, const uint32_t *blocks, pdbsize_t delta,
                                                            void *buffer, pdbsize_t size, pdbsize_t *num_read)
{
    enum pdb_result result;
    pdbsize_t initial_size = size;
    pdbsize_t toread;
    void *block_buffer;

    while (size)
    {
        toread = min(pdb->block_size - delta, size);

        if ((result = (pdb->fetch)(pdb, *blocks, &block_buffer))) return result;
        memcpy(buffer, (char *)block_buffer + delta, toread);
        size -= toread;
        blocks++;
        buffer = (char*)buffer + toread;
        delta = 0;
    }

    if (num_read) *num_read = initial_size - size;
    return size != initial_size ? R_PDB_SUCCESS : R_PDB_INVALID_ARGUMENT;
}

static inline enum pdb_result pdb_reader_walker_init(struct pdb_reader *pdb, unsigned stream_id, struct pdb_reader_walker *walker)
{
    walker->stream_id = stream_id;
    walker->offset = 0;
    return pdb_reader_get_stream_size(pdb, stream_id, &walker->last);
}

static inline enum pdb_result pdb_reader_walker_narrow(struct pdb_reader_walker *walker, pdbsize_t offset, pdbsize_t len)
{
    if (offset < walker->offset || offset + len > walker->last) return R_PDB_INVALID_ARGUMENT;
    walker->offset = offset;
    walker->last = offset + len;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_read_from_stream(struct pdb_reader *pdb, const struct pdb_reader_walker *walker,
                                                   void *buffer, pdbsize_t size, pdbsize_t *num_read)
{
    enum pdb_result result;
    const uint32_t *blocks;
    pdbsize_t delta;

    if (walker->stream_id >= pdb->toc->num_streams) return R_PDB_INVALID_ARGUMENT;
    if (walker->offset >= pdb->toc->stream_size[walker->stream_id]) return R_PDB_INVALID_ARGUMENT;
    if (walker->offset >= walker->last) return R_PDB_INVALID_ARGUMENT;
    blocks = pdb->streams[walker->stream_id].blocks + walker->offset / pdb->block_size;

    if (walker->offset + size > pdb->toc->stream_size[walker->stream_id])
    {
        size = pdb->toc->stream_size[walker->stream_id] - walker->offset;
    }
    if (walker->offset + size > walker->last)
    {
        size = walker->last - walker->offset;
    }
    delta = walker->offset % pdb->block_size;

    if ((result = pdb_reader_internal_read_from_blocks(pdb, blocks, delta, buffer, size, num_read)))
        return result;
    return R_PDB_SUCCESS;
}

struct symref_code
{
    enum {symref_code_cv_typeid, symref_code_action} kind;
    cv_typ_t cv_typeid;
    unsigned action;
};

static inline struct symref_code *symref_code_init_from_cv_typeid(struct symref_code *code, cv_typ_t cv_typeid)
{
    code->kind = symref_code_cv_typeid;
    code->cv_typeid = cv_typeid;
    return code;
}

static inline struct symref_code *symref_code_init_from_action(struct symref_code *code, unsigned action)
{
    code->kind = symref_code_action;
    code->action = action;
    return code;
}

static enum pdb_result pdb_reader_encode_symref(struct pdb_reader *pdb, const struct symref_code *code, symref_t *symref)
{
    unsigned v;
    switch (code->kind)
    {
    case symref_code_cv_typeid:
        if (!code->cv_typeid)
        {
            *symref = 0;
            return R_PDB_SUCCESS;
        }
        if (code->cv_typeid < T_MAXPREDEFINEDTYPE)
            v = code->cv_typeid;
        else if (code->cv_typeid >= pdb->tpi_header.first_index && code->cv_typeid < pdb->tpi_header.last_index)
            v = T_MAXPREDEFINEDTYPE + (code->cv_typeid - pdb->tpi_header.first_index);
        else
            return R_PDB_INVALID_ARGUMENT;
        break;
    case symref_code_action:
        v = T_MAXPREDEFINEDTYPE + pdb->tpi_header.last_index - pdb->tpi_header.first_index + code->action;
        break;
    default:
        return R_PDB_INVALID_ARGUMENT;
    }
    *symref = (v << 2) | 1;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_decode_symref(struct pdb_reader *pdb, symref_t ref, struct symref_code *code)
{
    if ((ref & 3) != 1) return R_PDB_INVALID_ARGUMENT;
    ref >>= 2;
    if (ref < T_MAXPREDEFINEDTYPE)
        symref_code_init_from_cv_typeid(code, ref);
    else if (ref < T_MAXPREDEFINEDTYPE + pdb->tpi_header.last_index - pdb->tpi_header.first_index)
        symref_code_init_from_cv_typeid(code, pdb->tpi_header.first_index + (ref - T_MAXPREDEFINEDTYPE));
    else if (ref < T_MAXPREDEFINEDTYPE + pdb->tpi_header.last_index - pdb->tpi_header.first_index + pdb->num_action_entries)
        symref_code_init_from_action(code, ref - (T_MAXPREDEFINEDTYPE + pdb->tpi_header.last_index - pdb->tpi_header.first_index));
    else
        return R_PDB_INVALID_ARGUMENT;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_push_action(struct pdb_reader *pdb, enum pdb_action_type action, pdbsize_t stream_offset,
                                              unsigned id_size, symref_t container_symref, symref_t *symref)
{
    enum pdb_result result;
    struct symref_code code;

    if (!(pdb->num_action_entries & (pdb->num_action_entries - 1)))
    {
        unsigned n = pdb->num_action_entries;
        n = n ? n * 2 : 256;
        if ((result = pdb_reader_realloc(pdb, (void**)&pdb->action_store, n * sizeof(pdb->action_store[0])))) return result;
    }
    if ((result = pdb_reader_encode_symref(pdb, symref_code_init_from_action(&code, pdb->num_action_entries), symref))) return result;
    pdb->action_store[pdb->num_action_entries].action_type         = action;
    pdb->action_store[pdb->num_action_entries].action_length       = id_size;
    pdb->action_store[pdb->num_action_entries].stream_offset       = stream_offset;
    pdb->action_store[pdb->num_action_entries].container_symref    = container_symref;
    pdb->num_action_entries++;

    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_init_DBI(struct pdb_reader *pdb);
static enum pdb_result pdb_reader_internal_binary_search(size_t num_elt,
                                                         enum pdb_result (*cmp)(unsigned idx, int *cmp_ressult, void *user),
                                                         size_t *found, void *user);
static enum pdb_result pdb_reader_symref_from_cv_typeid(struct pdb_reader *pdb, cv_typ_t cv_typeid, symref_t *symref);

static enum pdb_result pdb_reader_init(struct pdb_reader *pdb, struct module *module, HANDLE file,
                                       const IMAGE_SECTION_HEADER *sections, unsigned num_sections)
{
    enum pdb_result result;
    struct PDB_DS_HEADER hdr;
    struct PDB_DS_TOC *toc = NULL;
    unsigned i;
    unsigned toc_blocks_size;
    uint32_t *toc_blocks = NULL;
    uint32_t *blocks;

    memset(pdb, 0, sizeof(*pdb));
    pdb->module = module;
    pdb->file = file;
    pool_init(&pdb->pool, 65536);

    pdb->module = module;
    pdb->sections = sections;
    pdb->num_sections = num_sections;

    if ((result = pdb_reader_fetch_file_no_cache(pdb, &hdr, 0, sizeof(hdr)))) return result;
    if (!memcmp(hdr.signature, PDB_JG_IDENT, sizeof(PDB_JG_IDENT)))
    {
        FIXME("PDB reader doesn't support old PDB JG file format\n");
        return R_PDB_INVALID_PDB_FILE;
    }
    if (memcmp(hdr.signature, PDB_DS_IDENT, sizeof(PDB_DS_IDENT)))
    {
        ERR("PDB reader doesn't recognize format (%s)\n", wine_dbgstr_a((char*)&hdr));
        return R_PDB_INVALID_PDB_FILE;
    }
    pdb->block_size = hdr.block_size;
    if ((result = pdb_reader_alloc(pdb, pdb->block_size * ARRAY_SIZE(pdb->cache), (void **)&pdb->fetch_cache_blocks))) goto failure;
    for (i = 0; i < ARRAY_SIZE(pdb->cache); i++)
    {
        pdb->cache[i].block_no = 0; /* block where PDB header is, so should never be matched by a read operation */
        pdb->cache[i].age = i;
    }
    pdb->fetch = &pdb_reader_fetch_block_from_file;
    toc_blocks_size = pdb_reader_num_blocks(pdb, hdr.toc_size) * sizeof(uint32_t);
    if ((result = pdb_reader_alloc(pdb, toc_blocks_size, (void**)&toc_blocks)) ||
        (result = pdb_reader_fetch_file_no_cache(pdb, toc_blocks, (pdboff_t)hdr.toc_block * hdr.block_size, toc_blocks_size)) ||
        (result = pdb_reader_alloc(pdb, hdr.toc_size, (void**)&toc)) ||
        (result = pdb_reader_internal_read_from_blocks(pdb, toc_blocks, 0, toc, hdr.toc_size, NULL)) ||
        (result = pdb_reader_alloc(pdb, toc->num_streams * sizeof(pdb->streams[0]), (void**)&pdb->streams)))
        goto failure;
    pdb_reader_free(pdb, toc_blocks);

    pdb->toc = toc;
    blocks = &toc->stream_size[toc->num_streams];
    for (i = 0; i < pdb->toc->num_streams; i++)
    {
        if (toc->stream_size[i] == 0 || toc->stream_size[i] == ~0u)
        {
            pdb->streams[i].blocks = NULL;
            pdb->toc->stream_size[i] = 0;
        }
        else
        {
            pdb->streams[i].blocks = blocks;
            blocks += pdb_reader_num_blocks(pdb, toc->stream_size[i]);
        }
        pdb->streams[i].name = NULL;
    }
    /* hack (must be set before loading debug info so it can be used therein) */
    pdb->module->ops_symref_modfmt = module->format_info[DFI_PDB];
    pdb_reader_init_DBI(pdb);

    return R_PDB_SUCCESS;

failure:
    WARN("Failed to load PDB header\n");
    pdb_reader_free(pdb, toc);
    pdb_reader_free(pdb, toc_blocks);
    return result;
}

void pdb_reader_dispose(struct pdb_reader *pdb)
{
    CloseHandle(pdb->file);
    /* note: pdb is allocated inside its pool, so this must be last line */
    pool_destroy(&pdb->pool);
}

static enum pdb_result pdb_reader_internal_read_advance(struct pdb_reader *pdb, struct pdb_reader_walker *walker,
                                                        void *buffer, pdbsize_t size)
{
    pdbsize_t num_read;
    enum pdb_result result;

    if (walker->offset + size > walker->last) return R_PDB_INVALID_ARGUMENT;
    result = pdb_reader_read_from_stream(pdb, walker, buffer, size, &num_read);
    if (result) return result;
    if (num_read != size) return R_PDB_IOERROR;
    walker->offset += size;
    return R_PDB_SUCCESS;
}
/* Handy macro to deserialize: ensure that read length is the type length, and advance offset in case of success */
#define pdb_reader_READ(pdb, walker, ptr) pdb_reader_internal_read_advance((pdb), (walker), (ptr), sizeof(*(ptr)))

static enum pdb_result pdb_reader_alloc_and_read(struct pdb_reader *pdb, struct pdb_reader_walker *walker,
                                                 pdbsize_t size, void **buffer)
{
    enum pdb_result result;

    if (walker->offset + size > walker->last) return R_PDB_INVALID_ARGUMENT;
    if ((result = pdb_reader_alloc(pdb, size, buffer))) return result;
    if ((result = pdb_reader_internal_read_advance(pdb, walker, *buffer, size)))
    {
        pdb_reader_free(pdb, *buffer);
        *buffer = NULL;
        return result;
    }
    return R_PDB_SUCCESS;
}


static enum pdb_result pdb_reader_fetch_string_from_stream(struct pdb_reader *pdb, struct pdb_reader_walker *walker, char *buffer, pdbsize_t length)
{
    enum pdb_result result;
    pdbsize_t num_read;
    char *zero;

    if (walker->offset + length > walker->last)
        length = walker->last - walker->offset;
    if ((result = pdb_reader_read_from_stream(pdb, walker, buffer, length, &num_read)))
        return result;
    if (!(zero = memchr(buffer, '\0', num_read)))
        return num_read == length ? R_PDB_BUFFER_TOO_SMALL : R_PDB_INVALID_ARGUMENT;
    walker->offset += zero - buffer + 1;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_load_stream_name_table(struct pdb_reader *pdb)
{
    struct pdb_reader_walker walker;
    struct pdb_reader_walker ok_bits_walker;
    struct PDB_DS_ROOT ds_root;
    enum pdb_result result;
    uint32_t count, numok, len, bitfield;
    unsigned i;

    if ((result = pdb_reader_walker_init(pdb, 1, &walker)) ||
        (result = pdb_reader_READ(pdb, &walker, &ds_root)))
        return result;

    if (ds_root.Version != 20000404)
    {
        ERR("-Unknown root block version %u\n", ds_root.Version);
        return R_PDB_INVALID_PDB_FILE;
    }

    if ((result = pdb_reader_alloc_and_read(pdb, &walker, ds_root.cbNames, (void**)&pdb->stream_names)))
        goto failure;

    if ((result = pdb_reader_READ(pdb, &walker, &numok)) ||
        (result = pdb_reader_READ(pdb, &walker, &count)))
        goto failure;

    /* account bitfield vector */
    if ((result = pdb_reader_READ(pdb, &walker, &len))) goto failure;
    ok_bits_walker = walker;
    walker.offset += len * sizeof(uint32_t);

    /* skip deleted vector */
    if ((result = pdb_reader_READ(pdb, &walker, &len))) goto failure;
    walker.offset += len * sizeof(uint32_t);

    for (i = 0; i < count; i++)
    {
        if ((i & 31u) == 0)
        {
            if ((result = pdb_reader_READ(pdb, &ok_bits_walker, &bitfield)))
                goto failure;
        }
        if (bitfield & (1u << (i & 31)))
        {
            uint32_t str_offset, stream_id;
            if ((result = pdb_reader_READ(pdb, &walker, &str_offset)) ||
                (result = pdb_reader_READ(pdb, &walker, &stream_id)))
                goto failure;
            pdb->streams[stream_id].name = pdb->stream_names + str_offset;
        }
    }
    return R_PDB_SUCCESS;
failure:
    pdb_reader_free(pdb, pdb->stream_names);
    pdb->stream_names = NULL;
    return result;
}

static enum pdb_result pdb_reader_get_stream_index_from_name(struct pdb_reader *pdb, const char *name,
                                                             unsigned *stream_id)
{
    unsigned i;

    if (!pdb->stream_names)
    {
        enum pdb_result result = pdb_reader_load_stream_name_table(pdb);
        if (result) return result;
    }
    for (i = 0; i < pdb->toc->num_streams; i++)
        if (pdb->streams[i].name && !strcmp(pdb->streams[i].name, name))
        {
            *stream_id = i;
            return R_PDB_SUCCESS;
        }
    return R_PDB_NOT_FOUND;
}

static enum pdb_result pdb_reader_alloc_and_fetch_string(struct pdb_reader *pdb, struct pdb_reader_walker *walker, char **string)
{
    static const unsigned int alloc_step = 256;
    enum pdb_result result;
    unsigned len = alloc_step;
    char *buffer;

    /* get string by chunks of alloc_step bytes... */
    /* Note: we never shrink the alloc buffer to its optimal size */
    for (buffer = NULL;; len += alloc_step)
    {
        if ((result = pdb_reader_realloc(pdb, (void**)&buffer, len)))
        {
            pdb_reader_free(pdb, buffer);
            return result;
        }
        result = pdb_reader_fetch_string_from_stream(pdb, walker, buffer + len - alloc_step, alloc_step);
        if (result != R_PDB_BUFFER_TOO_SMALL)
        {
            if (result == R_PDB_SUCCESS)
                *string = buffer;
            else
                pdb_reader_free(pdb, buffer);
            return result;
        }
        walker->offset += alloc_step;
    }
}

static enum pdb_result pdb_reader_skip_string(struct pdb_reader *pdb, struct pdb_reader_walker *walker)
{
    char tmp[256];
    enum pdb_result result;

    while ((result = pdb_reader_fetch_string_from_stream(pdb, walker, tmp, ARRAY_SIZE(tmp))))
    {
        if (result != R_PDB_BUFFER_TOO_SMALL) break;
        walker->offset += ARRAY_SIZE(tmp);
    }
    return result;
}

static enum pdb_result pdb_reader_alloc_and_fetch_global_string(struct pdb_reader *pdb, pdbsize_t str_offset, char **buffer)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    unsigned stream_id;

    if ((result = pdb_reader_get_stream_index_from_name(pdb, "/names", &stream_id)))
        return result;
    if ((result = pdb_reader_walker_init(pdb, stream_id, &walker))) return result;
    walker.offset = sizeof(PDB_STRING_TABLE) + str_offset;
    return pdb_reader_alloc_and_fetch_string(pdb, &walker, buffer);
}

static enum pdb_result pdb_reader_read_DBI_header(struct pdb_reader* pdb, PDB_SYMBOLS *dbi_header, struct pdb_reader_walker *walker)
{
    enum pdb_result result;

    /* assuming we always have that size (even for old format) in stream */
    if ((result = pdb_reader_walker_init(pdb, PDB_STREAM_DBI, walker)) ||
        (result = pdb_reader_READ(pdb, walker, dbi_header))) return result;
    if (dbi_header->signature != 0xffffffff)
    {
        /* Old version of the symbols record header */
        PDB_SYMBOLS_OLD old_dbi_header = *(const PDB_SYMBOLS_OLD*)dbi_header;

        dbi_header->version            = 0;
        dbi_header->module_size        = old_dbi_header.module_size;
        dbi_header->sectcontrib_size   = old_dbi_header.sectcontrib_size;
        dbi_header->segmap_size        = old_dbi_header.segmap_size;
        dbi_header->srcmodule_size     = old_dbi_header.srcmodule_size;
        dbi_header->pdbimport_size     = 0;
        dbi_header->global_hash_stream = old_dbi_header.global_hash_stream;
        dbi_header->public_stream      = old_dbi_header.public_stream;
        dbi_header->gsym_stream        = old_dbi_header.gsym_stream;

        walker->offset = sizeof(PDB_SYMBOLS_OLD);
    }

    return R_PDB_SUCCESS;
}

static UINT32 codeview_compute_hash(const char* ptr, unsigned len)
{
    const char* last = ptr + len;
    UINT32 ret = 0;

    while (ptr + sizeof(UINT32) <= last)
    {
        ret ^= *(UINT32*)ptr;
        ptr += sizeof(UINT32);
    }
    if (ptr + sizeof(UINT16) <= last)
    {
        ret ^= *(UINT16*)ptr;
        ptr += sizeof(UINT16);
    }
    if (ptr + sizeof(BYTE) <= last)
        ret ^= *(BYTE*)ptr;
    ret |= 0x20202020;
    ret ^= (ret >> 11);
    return ret ^ (ret >> 16);
}

static enum pdb_result pdb_reader_read_DBI_cu_header(struct pdb_reader* pdb, DWORD dbi_header_version,
                                                     struct pdb_reader_walker *walker,
                                                     PDB_SYMBOL_FILE_EX *dbi_cu_header)
{
    enum pdb_result result;

    if (dbi_header_version >= 19970000)
    {
        result = pdb_reader_READ(pdb, walker, dbi_cu_header);
    }
    else
    {
        PDB_SYMBOL_FILE old_dbi_cu_header;
        if (!(result = pdb_reader_READ(pdb, walker, &old_dbi_cu_header)))
        {
            memset(dbi_cu_header, 0, sizeof(*dbi_cu_header));
            dbi_cu_header->stream       = old_dbi_cu_header.stream;
            dbi_cu_header->range.index  = old_dbi_cu_header.range.index;
            dbi_cu_header->symbol_size  = old_dbi_cu_header.symbol_size;
            dbi_cu_header->lineno_size  = old_dbi_cu_header.lineno_size;
            dbi_cu_header->lineno2_size = old_dbi_cu_header.lineno2_size;
        }
    }
    return result;
}

struct pdb_reader_compiland_iterator
{
    struct pdb_reader_walker dbi_walker; /* in DBI stream */
    PDB_SYMBOLS dbi_header;
    PDB_SYMBOL_FILE_EX dbi_cu_header;
};

static enum pdb_result pdb_reader_compiland_iterator_init(struct pdb_reader *pdb, struct pdb_reader_compiland_iterator *iter)
{
    enum pdb_result result;
    if ((result = pdb_reader_read_DBI_header(pdb, &iter->dbi_header, &iter->dbi_walker))) return result;
    iter->dbi_walker.last = iter->dbi_walker.offset + iter->dbi_header.module_size;
    return pdb_reader_read_DBI_cu_header(pdb, iter->dbi_header.version, &iter->dbi_walker, &iter->dbi_cu_header);
}

static enum pdb_result pdb_reader_compiland_iterator_next(struct pdb_reader *pdb, struct pdb_reader_compiland_iterator *iter)
{
    enum pdb_result result;

    if ((result = pdb_reader_skip_string(pdb, &iter->dbi_walker)) ||
        (result = pdb_reader_skip_string(pdb, &iter->dbi_walker)))
    {
        return result;
    }
    iter->dbi_walker.offset = (iter->dbi_walker.offset + 3) & ~3u;
    return pdb_reader_read_DBI_cu_header(pdb, iter->dbi_header.version, &iter->dbi_walker, &iter->dbi_cu_header);
}

static enum pdb_result pdb_reader_compiland_iterator_alloc_and_read_compiland_name(struct pdb_reader *pdb, const struct pdb_reader_compiland_iterator *iter,
                                                                                   char **obj_name)
{
    struct pdb_reader_walker walker = iter->dbi_walker;

    return pdb_reader_alloc_and_fetch_string(pdb, &walker, obj_name);
}

struct pdb_compiland_lookup
{
    struct pdb_reader *pdb;
    struct pdb_reader_walker walker;
    unsigned segment;
    unsigned offset;
    unsigned range_size;
    PDB_SYMBOL_RANGE_EX range;
};

static enum pdb_result pdb_reader_contrib_range_cmp(unsigned idx, int *cmp, void *user)
{
    enum pdb_result result;
    struct pdb_compiland_lookup *lookup = user;
    struct pdb_reader_walker walker = lookup->walker;

    walker.offset += idx * lookup->range_size;
    if ((result = pdb_reader_READ(lookup->pdb, &walker, &lookup->range))) return result;
    *cmp = lookup->range.segment - lookup->segment;
    if (!*cmp)
        *cmp = lookup->range.offset - lookup->offset;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_lookup_compiland_by_segment_offset(struct pdb_reader *pdb, unsigned segment, unsigned offset, unsigned *compiland)
{
    enum pdb_result result;
    struct pdb_compiland_lookup lookup = {.pdb = pdb, .segment = segment, .offset = offset};
    UINT32 version;
    PDB_SYMBOLS dbi_header;
    size_t found;
    unsigned num_ranges;

    if ((result = pdb_reader_walker_init(pdb, PDB_STREAM_DBI, &lookup.walker))) return result;
    if ((result = pdb_reader_read_DBI_header(pdb, &dbi_header, &lookup.walker))) return result;
    if ((result = pdb_reader_walker_narrow(&lookup.walker, lookup.walker.offset + dbi_header.module_size, dbi_header.sectcontrib_size))) return result;

    if ((result = pdb_reader_READ(pdb, &lookup.walker, &version))) return result;
    lookup.range_size = sizeof(PDB_SYMBOL_RANGE_EX);
    switch (version)
    {
    case 0xeffe0000 + 19970605: break;
    case 0xeffe0000 + 20140516: lookup.range_size += sizeof(UINT32); break;
    default:
        WARN("Unsupported contrib version %x\n", version);
        return R_PDB_INVALID_PDB_FILE;
    }
    if ((lookup.walker.last - lookup.walker.offset) % lookup.range_size)
    {
        WARN("Unexpected lookup values\n");
        return R_PDB_INVALID_PDB_FILE;
    }
    num_ranges = (lookup.walker.last - lookup.walker.offset) / lookup.range_size;
    /* we assume contributions are stored in ascending order of segment / offset */
    result = pdb_reader_internal_binary_search(num_ranges, pdb_reader_contrib_range_cmp, &found, &lookup);
    if (result)
    {
        if (result != R_PDB_NOT_FOUND) return result;
        /* ensure address is within contribution range */
        if (lookup.segment != lookup.range.segment ||
            lookup.offset < lookup.range.offset ||
            lookup.offset >= lookup.range.offset + lookup.range.size) return R_PDB_NOT_FOUND;
    }
    *compiland = lookup.range.index;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_lookup_compiland_by_address(struct pdb_reader *pdb, DWORD_PTR address, unsigned *compiland)
{
    enum pdb_result result;
    unsigned segment, offset;
    if ((result = pdb_reader_get_segment_offset_from_address(pdb, address, &segment, &offset))) return result;
    return pdb_reader_lookup_compiland_by_segment_offset(pdb, segment, offset, compiland);
}

static enum pdb_result pdb_reader_subsection_next(struct pdb_reader *pdb, struct pdb_reader_walker *in_walker,
                                                  enum DEBUG_S_SUBSECTION_TYPE subsection_type,
                                                  struct pdb_reader_walker *sub_walker)
{
    enum pdb_result result;
    struct CV_DebugSSubsectionHeader_t hdr;

    for (; !(result = pdb_reader_READ(pdb, in_walker, &hdr)); in_walker->offset += hdr.cbLen)
    {
        if (hdr.type & DEBUG_S_IGNORE) continue;
        if (subsection_type && hdr.type != subsection_type) continue;
        *sub_walker = *in_walker;
        sub_walker->last = sub_walker->offset + hdr.cbLen;
        in_walker->offset += hdr.cbLen;
        return R_PDB_SUCCESS;
    }
    return result && result != R_PDB_INVALID_ARGUMENT ? result : R_PDB_NOT_FOUND;
}

struct pdb_reader_linetab2_location
{
    pdbsize_t dbi_cu_header_offset; /* in DBI stream */
    unsigned cu_stream_id;          /* compilation unit stream id */
    pdbsize_t lines_hdr_offset;     /* in cu_stream_id */
    pdbsize_t file_hdr_offset;      /* in cu_stream_id (inside lines block) */
    pdbsize_t filename_offset;      /* in global stream table (after S_FILECHKSUMS redirection) */
};

static enum pdb_result pdb_find_matching_linetab2(struct CV_Line_t *lines, unsigned num_lines, DWORD64 delta, unsigned *index)
{
    unsigned i;
    /* since the the address is inside the file_hdr, we assume then it's matched by last entry
     * (for which we don't have the next entry)
     */
    for (i = 0; i + 1 < num_lines; i++)
    {
        if (lines[i].offset == delta ||
            (lines[i].offset <= delta && delta < lines[i + 1].offset))
            break;
    }
    *index = i;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_walker_init_linetab2(struct pdb_reader *pdb, const PDB_SYMBOL_FILE_EX *dbi_cu_header, struct pdb_reader_walker *walker)
{
    walker->stream_id = dbi_cu_header->stream;
    walker->offset    = dbi_cu_header->symbol_size;
    walker->last      = walker->offset + dbi_cu_header->lineno2_size;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_locate_filehdr_in_linetab2(struct pdb_reader *pdb, struct pdb_reader_walker linetab2_walker,
                                                             DWORD64 address, DWORD64 *lineblk_base,
                                                             struct CV_DebugSLinesFileBlockHeader_t *files_hdr, struct CV_Line_t **lines)
{
    struct pdb_reader_walker sub_walker;
    enum pdb_result result;
    struct CV_DebugSLinesHeader_t lines_hdr;

    while (!(result = pdb_reader_subsection_next(pdb, &linetab2_walker, DEBUG_S_LINES, &sub_walker)))
    {
        /* Skip blocks that are too small - Intel C Compiler generates these. */
        if (sub_walker.offset + sizeof(lines_hdr) + sizeof(struct CV_DebugSLinesFileBlockHeader_t) > sub_walker.last)
            continue;
        if ((result = pdb_reader_READ(pdb, &sub_walker, &lines_hdr))) return result;
        if ((result = pdb_reader_get_segment_address(pdb, lines_hdr.segCon, lines_hdr.offCon, lineblk_base)))
            return result;
        if (*lineblk_base > address || address >= *lineblk_base + lines_hdr.cbCon)
            continue;
        if ((result = pdb_reader_READ(pdb, &sub_walker, files_hdr))) return result;
        return pdb_reader_alloc_and_read(pdb, &sub_walker, files_hdr->nLines * sizeof((*lines)[0]),(void**)lines);
        /*
          if (lines_hdr.flags & CV_LINES_HAVE_COLUMNS)
          sub_walker.offset += files_hdr.nLines * sizeof(struct CV_Column_t);
        */
    }
    return R_PDB_NOT_FOUND;
}

static enum pdb_result pdb_reader_set_lineinfo_filename(struct pdb_reader *pdb, struct pdb_reader_walker linetab2_walker,
                                                        unsigned file_offset, struct lineinfo_t *line_info)
{
    struct pdb_reader_walker checksum_walker;
    struct CV_Checksum_t checksum;
    enum pdb_result result;
    char *string;

    if ((result = pdb_reader_subsection_next(pdb, &linetab2_walker, DEBUG_S_FILECHKSMS, &checksum_walker)))
    {
        WARN("No DEBUG_S_FILECHKSMS found\n");
        return R_PDB_MISSING_INFORMATION;
    }
    checksum_walker.offset += file_offset;
    if ((result = pdb_reader_READ(pdb, &checksum_walker, &checksum))) return result;
    if ((result = pdb_reader_alloc_and_fetch_global_string(pdb, checksum.strOffset, &string))) return result;
    if (!lineinfo_set_nameA(pdb->module->process, line_info, string))
        result = R_PDB_OUT_OF_MEMORY;
    pdb_reader_free(pdb, string);
    return result;
}

static enum pdb_result pdb_reader_search_linetab2(struct pdb_reader *pdb, const PDB_SYMBOL_FILE_EX *dbi_cu_header,
                                                  DWORD64 address, struct lineinfo_t *line_info)
{
    struct pdb_reader_walker linetab2_walker;
    struct CV_DebugSLinesFileBlockHeader_t files_hdr;
    enum pdb_result result;
    DWORD64 lineblk_base;
    struct CV_Line_t *lines;

    if ((result = pdb_reader_walker_init_linetab2(pdb, dbi_cu_header, &linetab2_walker))) return result;

    if (!pdb_reader_locate_filehdr_in_linetab2(pdb, linetab2_walker, address, &lineblk_base, &files_hdr, &lines))
    {
        unsigned i;

        if (!pdb_find_matching_linetab2(lines, files_hdr.nLines, address - lineblk_base, &i))
        {
            /* found block... */
            line_info->address = lineblk_base + lines[i].offset;
            line_info->line_number = lines[i].linenumStart;
            return pdb_reader_set_lineinfo_filename(pdb, linetab2_walker, files_hdr.offFile, line_info);
        }
        pdb_reader_free(pdb, lines);
    }
    return R_PDB_NOT_FOUND;
}

static enum pdb_result pdb_reader_get_line_from_address_internal(struct pdb_reader *pdb,
                                                                 DWORD64 address, struct lineinfo_t *line_info,
                                                                 pdbsize_t *compiland_offset)
{
    struct pdb_reader_compiland_iterator compiland_iter;
    enum pdb_result result;

    if ((result = pdb_reader_compiland_iterator_init(pdb, &compiland_iter))) return result;
    do
    {
        if (compiland_iter.dbi_cu_header.lineno2_size)
        {
            result = pdb_reader_search_linetab2(pdb, &compiland_iter.dbi_cu_header, address, line_info);
            if (!result)
            {
                *compiland_offset = compiland_iter.dbi_walker.offset - sizeof(compiland_iter.dbi_cu_header);
                return result;
            }
            if (result != R_PDB_NOT_FOUND) return result;
        }
    } while (pdb_reader_compiland_iterator_next(pdb, &compiland_iter) == R_PDB_SUCCESS);

    return R_PDB_NOT_FOUND;
}

static enum method_result pdb_method_result(enum pdb_result result)
{
    switch (result)
    {
    case R_PDB_SUCCESS:   return MR_SUCCESS;
    case R_PDB_NOT_FOUND: return MR_NOT_FOUND;
    default:              return MR_FAILURE;
    }
}

static enum method_result pdb_method_get_line_from_address(struct module_format *modfmt,
                                                           DWORD64 address, struct lineinfo_t *line_info)
{
    enum pdb_result result;
    struct pdb_reader *pdb;
    pdbsize_t compiland_offset;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;
    result = pdb_reader_get_line_from_address_internal(pdb, address, line_info, &compiland_offset);
    return pdb_method_result(result);
}

static enum pdb_result pdb_reader_advance_line_info(struct pdb_reader *pdb,
                                                    struct lineinfo_t *line_info, BOOL forward)
{
    struct pdb_reader_compiland_iterator compiland_iter;
    struct pdb_reader_walker linetab2_walker;
    struct CV_DebugSLinesFileBlockHeader_t files_hdr;
    DWORD64 lineblk_base;
    struct CV_Line_t *lines;
    enum pdb_result result;
    unsigned i;

    if ((result = pdb_reader_compiland_iterator_init(pdb, &compiland_iter)))
        return result;
    do
    {
        if (compiland_iter.dbi_cu_header.lineno2_size)
        {
            if ((result = pdb_reader_walker_init_linetab2(pdb, &compiland_iter.dbi_cu_header, &linetab2_walker)))
                return result;
            result = pdb_reader_locate_filehdr_in_linetab2(pdb, linetab2_walker, line_info->address, &lineblk_base, &files_hdr, &lines);
            if (result == R_PDB_NOT_FOUND) continue;
            if (result) return result;
            if ((result = pdb_find_matching_linetab2(lines, files_hdr.nLines, line_info->address - lineblk_base, &i)))
                return result;

            /* It happens that several entries have same address (yet potentially different line numbers)
             * Simplify handling by getting the first entry (forward or backward) with a different address.
             * More tests from native are required.
             */
            if (forward)
            {
                for (; i + 1 < files_hdr.nLines; i++)
                    if (line_info->address != lineblk_base + lines[i + 1].offset)
                    {
                        line_info->address = lineblk_base + lines[i + 1].offset;
                        line_info->line_number = lines[i + 1].linenumStart;
                        break;
                    }
                if (i + 1 >= files_hdr.nLines)
                    result = R_PDB_INVALID_ARGUMENT;
            }
            else
            {
                for (; i; --i)
                {
                    if (line_info->address != lineblk_base + lines[i - 1].offset)
                    {
                        line_info->address = lineblk_base + lines[i - 1].offset;
                        line_info->line_number = lines[i - 1].linenumStart;
                        break;
                    }
                }
                if (!i)
                    result = R_PDB_INVALID_ARGUMENT;
            }
            pdb_reader_free(pdb, lines);
            /* refresh filename in case it has been tempered with */
            return result ? result : pdb_reader_set_lineinfo_filename(pdb, linetab2_walker, files_hdr.offFile, line_info);
        }
    } while (pdb_reader_compiland_iterator_next(pdb, &compiland_iter) == R_PDB_SUCCESS);

    return R_PDB_NOT_FOUND;
}

static enum method_result pdb_method_advance_line_info(struct module_format *modfmt,
                                                       struct lineinfo_t *line_info, BOOL forward)
{
    struct pdb_reader *pdb;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;
    return pdb_reader_advance_line_info(pdb, line_info, forward) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
}

static enum pdb_result pdb_reader_enum_lines_linetab2(struct pdb_reader *pdb, const PDB_SYMBOL_FILE_EX *dbi_cu_header,
                                                      const WCHAR *source_file_regex, SRCCODEINFO *source_code_info, PSYM_ENUMLINES_CALLBACK cb, void *user)
{
    struct pdb_reader_walker linetab2_walker = {dbi_cu_header->stream, dbi_cu_header->symbol_size, dbi_cu_header->symbol_size + dbi_cu_header->lineno2_size};
    struct pdb_reader_walker walker, sub_walker, checksum_walker;
    enum pdb_result result;
    struct CV_DebugSLinesHeader_t lines_hdr;
    struct CV_DebugSLinesFileBlockHeader_t files_hdr;
    DWORD64 lineblk_base;

    for (checksum_walker = linetab2_walker; !(result = pdb_reader_subsection_next(pdb, &checksum_walker, DEBUG_S_FILECHKSMS, &sub_walker)); )
    {
        checksum_walker = sub_walker;
        break;
    }
    if (result)
    {
        WARN("No DEBUG_S_FILECHKSMS found\n");
        return R_PDB_MISSING_INFORMATION;
    }

    for (walker = linetab2_walker;
         !(result = pdb_reader_subsection_next(pdb, &walker, DEBUG_S_LINES, &sub_walker));
        )
    {
        /* Skip blocks that are too small - Intel C Compiler generates these. */
        if (sub_walker.offset + sizeof(lines_hdr) + sizeof(struct CV_DebugSLinesFileBlockHeader_t) > sub_walker.last)
            continue;
        if ((result = pdb_reader_READ(pdb, &sub_walker, &lines_hdr))) return result;
        if ((result = pdb_reader_get_segment_address(pdb, lines_hdr.segCon, lines_hdr.offCon, &lineblk_base)))
            return result;
        for (; (result = pdb_reader_READ(pdb, &sub_walker, &files_hdr)) == R_PDB_SUCCESS; /*lineblk_base += files_hdr.cbBlock*/)
        {
            pdbsize_t current_stream_offset;
            struct CV_Checksum_t checksum;
            struct CV_Line_t *lines;
            char *string;
            unsigned i;
            BOOL match = TRUE;

            if ((result = pdb_reader_alloc_and_read(pdb, &sub_walker, files_hdr.nLines * sizeof(lines[0]),
                                                    (void**)&lines))) return result;

            /* should filter on filename */
            current_stream_offset = checksum_walker.offset;
            checksum_walker.offset += files_hdr.offFile;
            if ((result = pdb_reader_READ(pdb, &checksum_walker, &checksum))) return result;
            if ((result = pdb_reader_alloc_and_fetch_global_string(pdb, checksum.strOffset, &string))) return result;
            checksum_walker.offset = current_stream_offset;

            if (source_file_regex)
                match = symt_match_stringAW(string, source_file_regex, FALSE);
            if (!match)
            {
                sub_walker.offset += files_hdr.nLines * sizeof(struct CV_Line_t);
                if (lines_hdr.flags & CV_LINES_HAVE_COLUMNS)
                    sub_walker.offset += files_hdr.nLines * sizeof(struct CV_Column_t);
                continue;
            }
            if (strlen(string) < ARRAY_SIZE(source_code_info->FileName))
                strcpy(source_code_info->FileName, string);
            else
                source_code_info->FileName[0] = '\0';
            pdb_reader_free(pdb, string);

            for (i = 0; i < files_hdr.nLines; i++)
            {
                source_code_info->Address = lineblk_base + lines[i].offset;
                source_code_info->LineNumber = lines[i].linenumStart;
                if (!cb(source_code_info, user)) return R_PDB_NOT_FOUND;
            }
            pdb_reader_free(pdb, lines);
            if (lines_hdr.flags & CV_LINES_HAVE_COLUMNS)
                sub_walker.offset += files_hdr.nLines * sizeof(struct CV_Column_t);
        }
    }
    return result == R_PDB_INVALID_ARGUMENT ? R_PDB_SUCCESS : result;
}

static BOOL pdb_method_enumerate_lines_internal(struct pdb_reader *pdb, const WCHAR* compiland_regex,
                                                const WCHAR *source_file_regex, PSYM_ENUMLINES_CALLBACK cb, void *user)
{
    struct pdb_reader_compiland_iterator compiland_iter;
    enum pdb_result result;
    SRCCODEINFO source_code_info;

    source_code_info.SizeOfStruct = sizeof(source_code_info);
    source_code_info.ModBase = pdb->module->module.BaseOfImage;

    if ((result = pdb_reader_compiland_iterator_init(pdb, &compiland_iter))) return result;
    do
    {
        struct pdb_reader_walker walker = compiland_iter.dbi_walker;

        if ((result = pdb_reader_fetch_string_from_stream(pdb, &walker, source_code_info.Obj, sizeof(source_code_info.Obj))))
        {
            if (result == R_PDB_BUFFER_TOO_SMALL) FIXME("NOT EXPECTED --too small\n");
            return result;
        }

        /* FIXME should filter on compiland (if present) */
        if (compiland_iter.dbi_cu_header.lineno2_size)
        {
            result = pdb_reader_enum_lines_linetab2(pdb, &compiland_iter.dbi_cu_header, source_file_regex, &source_code_info, cb, user);
        }

    } while (pdb_reader_compiland_iterator_next(pdb, &compiland_iter) == R_PDB_SUCCESS);
    return R_PDB_SUCCESS;
}

static enum method_result pdb_method_enumerate_lines(struct module_format *modfmt, const WCHAR* compiland_regex,
                                                     const WCHAR *source_file_regex, PSYM_ENUMLINES_CALLBACK cb, void *user)
{
    struct pdb_reader *pdb;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;

    return pdb_method_result(pdb_method_enumerate_lines_internal(pdb, compiland_regex, source_file_regex, cb, user));
}

static enum pdb_result pdb_reader_load_sources_linetab2(struct pdb_reader *pdb, const PDB_SYMBOL_FILE_EX *dbi_cu_header)
{
    struct pdb_reader_walker linetab2_walker = {dbi_cu_header->stream, dbi_cu_header->symbol_size, dbi_cu_header->symbol_size + dbi_cu_header->lineno2_size};
    struct pdb_reader_walker sub_walker, checksum_walker;
    enum pdb_result result;
    struct CV_Checksum_t chksum;

    for (checksum_walker = linetab2_walker; !(result = pdb_reader_subsection_next(pdb, &checksum_walker, DEBUG_S_FILECHKSMS, &sub_walker)); )
    {
        for (; (result = pdb_reader_READ(pdb, &sub_walker, &chksum)) == R_PDB_SUCCESS; sub_walker.offset = (sub_walker.offset + chksum.size + 3) & ~3)
        {
            char *string;
            if ((result = pdb_reader_alloc_and_fetch_global_string(pdb, chksum.strOffset, &string))) return result;
            source_new(pdb->module, NULL, string);
            pdb_reader_free(pdb, string);
        }
    }
    return result == R_PDB_NOT_FOUND ? R_PDB_SUCCESS : result;
}

static enum pdb_result pdb_load_sources_internal(struct pdb_reader *pdb)
{
    enum pdb_result result;
    struct pdb_reader_compiland_iterator compiland_iter;

    if ((result = pdb_reader_compiland_iterator_init(pdb, &compiland_iter))) return result;
    do
    {
        if (compiland_iter.dbi_cu_header.lineno2_size)
        {
            result = pdb_reader_load_sources_linetab2(pdb, &compiland_iter.dbi_cu_header);
        }
    } while (pdb_reader_compiland_iterator_next(pdb, &compiland_iter) == R_PDB_SUCCESS);

    return R_PDB_SUCCESS;
}

static enum method_result pdb_method_enumerate_sources(struct module_format *modfmt, const WCHAR *source_file_regex,
                                                       PSYM_ENUMSOURCEFILES_CALLBACKW cb, void *user)
{
    struct pdb_reader *pdb;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;

    /* Note: in PDB, each compiland lists its used files, which are all in global string table,
     * but there's no global source files table AFAICT.
     * So, just walk (once) all compilands to grab all sources, and store them in generic source table.
     * But don't enumerate here, let generic function take care of it.
     */
    if (!pdb->source_listed)
    {
        enum pdb_result result = pdb_load_sources_internal(pdb);
        if (result) return pdb_method_result(result);
        pdb->source_listed = 1;
    }
    return MR_NOT_FOUND;
}

static unsigned codeview_get_leaf_length(unsigned short type)
{
    static unsigned char codeview_leaf_length[] =
    {
        [LF_CHAR       - LF_NUMERIC] = 1,
        [LF_SHORT      - LF_NUMERIC] = 2,
        [LF_USHORT     - LF_NUMERIC] = 2,
        [LF_LONG       - LF_NUMERIC] = 4,
        [LF_ULONG      - LF_NUMERIC] = 4,
        [LF_REAL32     - LF_NUMERIC] = 4,
        [LF_REAL64     - LF_NUMERIC] = 8,
        [LF_REAL80     - LF_NUMERIC] = 10,
        [LF_REAL128    - LF_NUMERIC] = 16,
        [LF_QUADWORD   - LF_NUMERIC] = 8,
        [LF_UQUADWORD  - LF_NUMERIC] = 8,
        [LF_REAL48     - LF_NUMERIC] = 6,
        [LF_COMPLEX32  - LF_NUMERIC] = 8,
        [LF_COMPLEX64  - LF_NUMERIC] = 16,
        [LF_COMPLEX80  - LF_NUMERIC] = 20,
        [LF_COMPLEX128 - LF_NUMERIC] = 32,
        [LF_VARSTRING  - LF_NUMERIC] = 0,
        [LF_OCTWORD    - LF_NUMERIC] = 16,
        [LF_UOCTWORD   - LF_NUMERIC] = 16,
        [LF_DECIMAL    - LF_NUMERIC] = 0,
        [LF_DATE       - LF_NUMERIC] = 0,
        [LF_UTF8STRING - LF_NUMERIC] = 0,
        [LF_REAL16     - LF_NUMERIC] = 2,
    };

    if (type < LF_NUMERIC) return 0;
    type -= LF_NUMERIC;
    return type < ARRAY_SIZE(codeview_leaf_length) ? codeview_leaf_length[type] : 0;
}

static int codeview_leaf_as_variant(const unsigned char *leaf, VARIANT *v)
{
    unsigned short int type = *(const unsigned short *)leaf;

    if (type < LF_NUMERIC)
    {
        V_VT(v) = VT_I2;
        V_I2(v) = type;
        return sizeof(unsigned short);
    }
    leaf += sizeof(unsigned short);
    switch (type)
    {
    case LF_CHAR:
        V_VT(v) = VT_I1;
        V_I1(v) = *(const char*)leaf;
        break;
    case LF_SHORT:
        V_VT(v) = VT_I2;
        V_I2(v) = *(const short*)leaf;
        break;
    case LF_USHORT:
        V_VT(v) = VT_UI2;
        V_UI2(v) = *(const unsigned short*)leaf;
        break;
    case LF_LONG:
        V_VT(v) = VT_I4;
        V_I4(v) = *(const int*)leaf;
        break;
    case LF_ULONG:
        V_VT(v) = VT_UI4;
        V_UI4(v) = *(const unsigned int*)leaf;
        break;
    case LF_QUADWORD:
        V_VT(v) = VT_I8;
        V_I8(v) = *(const long long int*)leaf;
        break;
    case LF_UQUADWORD:
        V_VT(v) = VT_UI8;
        V_UI8(v) = *(const long long unsigned int*)leaf;
        break;
    case LF_REAL32:
        V_VT(v) = VT_R4;
        V_R4(v) = *(const float*)leaf;
        break;
    case LF_REAL64:
        V_VT(v) = VT_R8;
        V_R8(v) = *(const double*)leaf;
        break;

    case LF_VARSTRING:
        PDB_REPORT_UNEXPECTED("numeric leaf", type);
        V_VT(v) = VT_EMPTY;     /* FIXME */
        return 2 + *leaf;

    case LF_REAL48:
    case LF_REAL80:
    case LF_REAL128:
    case LF_COMPLEX32:
    case LF_COMPLEX64:
    case LF_COMPLEX80:
    case LF_COMPLEX128:
        /* we can't convert them into an int */
    default:
        PDB_REPORT_UNEXPECTED("numeric leaf", type);
        V_VT(v) = VT_EMPTY;     /* FIXME */
        return 0;
    }
    return sizeof(unsigned short) + codeview_get_leaf_length(type);
}

/* read a codeview numeric leaf */
static enum pdb_result pdb_reader_read_leaf_as_variant(struct pdb_reader *pdb, struct pdb_reader_walker *walker, VARIANT *v)
{
    enum pdb_result result;
    unsigned short int type;

    if ((result = pdb_reader_READ(pdb, walker, &type))) return result;

    if (type < LF_NUMERIC)
    {
        V_VT(v) = VT_I2;
        V_I2(v) = type;
    }
    else
    {
        switch (type)
        {
        case LF_CHAR:      V_VT(v) = VT_I1;  return pdb_reader_READ(pdb, walker, &V_I1(v));
        case LF_SHORT:     V_VT(v) = VT_I2;  return pdb_reader_READ(pdb, walker, &V_I2(v));
        case LF_USHORT:    V_VT(v) = VT_UI2; return pdb_reader_READ(pdb, walker, &V_UI2(v));
        case LF_LONG:      V_VT(v) = VT_I4;  return pdb_reader_READ(pdb, walker, &V_I4(v));
        case LF_ULONG:     V_VT(v) = VT_UI4; return pdb_reader_READ(pdb, walker, &V_UI4(v));
        case LF_QUADWORD:  V_VT(v) = VT_I8;  return pdb_reader_READ(pdb, walker, &V_I8(v));
        case LF_UQUADWORD: V_VT(v) = VT_UI8; return pdb_reader_READ(pdb, walker, &V_UI8(v));
        case LF_REAL32:    V_VT(v) = VT_R4;  return pdb_reader_READ(pdb, walker, &V_R4(v));
        case LF_REAL64:    V_VT(v) = VT_R8;  return pdb_reader_READ(pdb, walker, &V_R8(v));

        /* types that don't simply fit inside VARIANT (would need conversion) */

        case LF_OCTWORD:
        case LF_UOCTWORD:
        case LF_REAL48:
        case LF_REAL80:
        case LF_REAL128:
        case LF_COMPLEX32:
        case LF_COMPLEX64:
        case LF_COMPLEX80:
        case LF_COMPLEX128:

        /* case LF_VARSTRING: */

        /* as we don't know the length... will lead to later issues */
        default:
            break;
        }
        PDB_REPORT_UNEXPECTED("numeric leaf", type);
        walker->offset += codeview_get_leaf_length(type);
        V_VT(v) = VT_EMPTY;
    }

    return R_PDB_SUCCESS;
}

/* read a codeview numeric leaf from a partially loaded codeview type */
static enum pdb_result codeview_leaf_as_int(const unsigned char *data, int *value)
{
    unsigned short int type;

    type = *(unsigned short int*)data;
    if (type < LF_NUMERIC)
    {
        *value = type;
    }
    else
    {
        switch (type)
        {
        case LF_CHAR:      *value = *(char*)(data + 2); break;
        case LF_SHORT:     *value = (int)*(short*)(data + 2); break;
        case LF_USHORT:    *value = *(unsigned short*)(data + 2); break;
        case LF_LONG:      *value = *(int*)(data + 2); break;
        case LF_ULONG:     *value = *(unsigned*)(data + 2); break;

        /* the leaves we can't convert into an int */
        case LF_QUADWORD:
        case LF_UQUADWORD:
        case LF_OCTWORD:
        case LF_UOCTWORD:
        case LF_REAL32:
        case LF_REAL48:
        case LF_REAL64:
        case LF_REAL80:
        case LF_REAL128:
        case LF_COMPLEX32:
        case LF_COMPLEX64:
        case LF_COMPLEX80:
        case LF_COMPLEX128:

        case LF_VARSTRING:
        /* as we don't know the length... will lead to later issues */

        default:
            PDB_REPORT_UNEXPECTED("numeric leaf", type);
            return R_PDB_INVALID_ARGUMENT;
        }
    }

    return R_PDB_SUCCESS;
}

static enum pdb_result codeview_fetch_leaf_as_int(const union codeview_type *cv_type, const unsigned char *data, int *value)
{
    unsigned data_len = min(sizeof(*cv_type), sizeof(cv_type->generic.len) + cv_type->generic.len) - (data - (const unsigned char*)cv_type);
    unsigned short int type;

    if (data_len < sizeof(unsigned short)) return R_PDB_BUFFER_TOO_SMALL;

    type = *(unsigned short int*)data;
    if (type >= LF_NUMERIC && data_len < codeview_get_leaf_length(type))
        return R_PDB_BUFFER_TOO_SMALL;
    return codeview_leaf_as_int(data, value);
}

static WCHAR *heap_allocate_symname(const char *string)
{
    unsigned sz = MultiByteToWideChar(CP_ACP, 0, string, -1, NULL, 0);
    WCHAR *stringW;

    stringW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR));
    if (stringW)
        MultiByteToWideChar(CP_ACP, 0, string, -1, stringW, sz);
    return stringW;
}

#define loc_cv_local_range (loc_user + 0) /* loc.offset contain the copy of all defrange* Codeview records following S_LOCAL */
#define loc_cv_defrange (loc_user + 1)    /* loc.register+offset contain the stream_id+stream_offset of S_LOCAL Codeview record to search into */

/* Some data (codeview_symbol, codeview_types...) are layed out with a 2 byte integer,
 * designing length of following blob.
 * Basic reading of that length + (part) of blob.
 * Walker is advanced by 2 only (so that any reading inside blob is possible).
  */
static enum pdb_result pdb_reader_read_partial_blob(struct pdb_reader *pdb, struct pdb_reader_walker *walker, void *blob, unsigned blob_size)
{
    enum pdb_result result;
    pdbsize_t num_read, toload;
    unsigned short len;

    if ((result = pdb_reader_internal_read_advance(pdb, walker, &len, sizeof(len)))) return result;
    toload = min(len, blob_size - sizeof(len));

    if ((result = pdb_reader_read_from_stream(pdb, walker, (char*)blob + sizeof(len), toload, &num_read))) return result;
    if (num_read != toload) return R_PDB_IOERROR;
    *(unsigned short*)blob = len;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_alloc_and_read_full_blob(struct pdb_reader *pdb, struct pdb_reader_walker *walker, void **blob)
{
    enum pdb_result result;
    unsigned short int len;

    if ((result = pdb_reader_READ(pdb, walker, &len))) return result;
    if ((result = pdb_reader_alloc(pdb, len + sizeof(len), blob)))
    {
        walker->offset -= sizeof(len);
        return result;
    }

    if ((result = pdb_reader_internal_read_advance(pdb, walker, (char*)*blob + sizeof(len), len)))
    {
        pdb_reader_free(pdb, *blob);
        walker->offset -= sizeof(len);
        return result;
    }
    *(unsigned short int*)*blob = len;
    return R_PDB_SUCCESS;
}

/* Read the fixed part of a CodeView symbol (enough to fit inside the union codeview) */
static enum pdb_result pdb_reader_read_partial_codeview_symbol(struct pdb_reader *pdb, struct pdb_reader_walker *walker, union codeview_symbol *cv_symbol)
{
    return pdb_reader_read_partial_blob(pdb, walker, (void*)cv_symbol, sizeof(*cv_symbol));
}

static enum pdb_result pdb_reader_alloc_and_read_full_codeview_symbol(struct pdb_reader *pdb, struct pdb_reader_walker *walker,
                                                                      union codeview_symbol **cv_symbol)
{
    return pdb_reader_alloc_and_read_full_blob(pdb, walker, (void **)cv_symbol);
}

static enum pdb_result pdb_reader_load_DBI_hash_table(struct pdb_reader *pdb)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    DBI_HASH_HEADER dbi_hash_header;
    unsigned num_hash_records;
    DBI_HASH_RECORD hash_record;
    UINT32 bitmap;
    UINT32 start_index, end_index;
    unsigned index, last_index, i, j;
    struct pdb_dbi_hash_entry *entry;

    if ((result = pdb_reader_walker_init(pdb, pdb->dbi_header.global_hash_stream, &walker))) return result;
    if ((result = pdb_reader_READ(pdb, &walker, &dbi_hash_header))) return result;
    if (dbi_hash_header.signature != 0xFFFFFFFF ||
        dbi_hash_header.version != 0xeffe0000 + 19990810)
    {
        WARN("Incorrect hash stream header\n");
        return R_PDB_INVALID_PDB_FILE;
    }
    if (dbi_hash_header.hash_records_size)
    {
        if ((dbi_hash_header.hash_records_size % sizeof(DBI_HASH_RECORD)) != 0 ||
            sizeof(DBI_HASH_HEADER) + dbi_hash_header.hash_records_size + DBI_BITMAP_HASH_SIZE > walker.last ||
            (walker.last - (sizeof(DBI_HASH_HEADER) + dbi_hash_header.hash_records_size + DBI_BITMAP_HASH_SIZE)) % sizeof(uint32_t))
        {
            WARN("Incorrect hash structure\n");
            return R_PDB_INVALID_PDB_FILE;
        }
    }

    if ((result = pdb_reader_alloc(pdb, sizeof(pdb->dbi_symbols_hash[0]) * (DBI_MAX_HASH + 1), (void **)&pdb->dbi_symbols_hash))) return result;
    memset(pdb->dbi_symbols_hash, 0, sizeof(pdb->dbi_symbols_hash[0]) * (DBI_MAX_HASH + 1));
    for (index = 0, i = 0; i <= DBI_MAX_HASH; i++)
        pdb->dbi_symbols_hash[i].next = &pdb->dbi_symbols_hash[i];

    if (!dbi_hash_header.hash_records_size) return R_PDB_SUCCESS;
    num_hash_records = dbi_hash_header.hash_records_size / sizeof(DBI_HASH_RECORD);
    last_index = (walker.last - (sizeof(DBI_HASH_HEADER) + dbi_hash_header.hash_records_size + DBI_BITMAP_HASH_SIZE)) / sizeof(UINT32);

    for (index = 0, i = 0; i <= DBI_MAX_HASH; i++)
    {
        if ((i & 31) == 0)
        {
            walker.offset = sizeof(DBI_HASH_HEADER) + dbi_hash_header.hash_records_size + (i / 32) * 4;
            if ((result = pdb_reader_READ(pdb, &walker, &bitmap))) goto on_error;
        }
        if (bitmap & (1u << (i % 32)))
        {
            walker.offset = sizeof(DBI_HASH_HEADER) + dbi_hash_header.hash_records_size + DBI_BITMAP_HASH_SIZE + index * sizeof(UINT32);
            /* Yes, offsets for accessing the hash_record:s are stored as multiple of 12;
             * and not as multiple of sizeof(hash_record) = 8 as one might expect.
             * Perhaps, native implementation likes to keep the same offsets between
             * in memory representation vs on file representations.
             */
            if ((result = pdb_reader_READ(pdb, &walker, &start_index))) goto on_error;
            start_index /= 12;
            if (index + 1 < last_index)
            {
                if ((result = pdb_reader_READ(pdb, &walker, &end_index))) goto on_error;
                end_index /= 12;
            }
            else
                end_index = num_hash_records;
            index++;

            for (j = start_index; j < end_index; j++)
            {
                walker.offset = sizeof(DBI_HASH_HEADER) + j * sizeof(DBI_HASH_RECORD);
                if ((result = pdb_reader_READ(pdb, &walker, &hash_record))) goto on_error;
                if (pdb->dbi_symbols_hash[i].next == &pdb->dbi_symbols_hash[i]) /* empty slot */
                {
                    pdb->dbi_symbols_hash[i].dbi_stream_offset = hash_record.offset - 1;
                    pdb->dbi_symbols_hash[i].next = NULL;
                }
                else
                {
                    struct pdb_dbi_hash_entry **last;
                    if ((result = pdb_reader_alloc(pdb, sizeof(*entry), (void **)&entry))) goto on_error;
                    entry->dbi_stream_offset = hash_record.offset - 1;
                    entry->next = NULL;
                    for (last = &pdb->dbi_symbols_hash[i].next; *last; last = &(*last)->next) {}
                    *last = entry;
                }
            }
        }
    }
    return R_PDB_SUCCESS;
on_error:
    for (i = 0; i <= DBI_MAX_HASH; i++)
    {
        struct pdb_dbi_hash_entry *current, *next;
        if (pdb->dbi_symbols_hash[i].next == &pdb->dbi_symbols_hash[i]) continue;
        for (current = pdb->dbi_symbols_hash[i].next; current; current = next)
        {
            next = current->next;
            pdb_reader_free(pdb, current);
        }
    }
    pdb_reader_free(pdb, pdb->dbi_symbols_hash);
    pdb->dbi_symbols_hash = NULL;
    return result;
}

static enum pdb_result pdb_reader_extract_name_out_of_codeview_symbol(union codeview_symbol *cv_symbol, char **name, size_t *length)
{
    switch (cv_symbol->generic.id)
    {
    case S_UDT:
        *name = cv_symbol->udt_v3.name;
        break;
    case S_PROCREF:
    case S_LPROCREF:
    case S_DATAREF:
        *name = cv_symbol->refsym2_v3.name;
        break;
    case S_CONSTANT:
        *name = (char *)(cv_symbol->constant_v3.data + codeview_get_leaf_length(*(unsigned short*)cv_symbol->constant_v3.data));
        break;
    case S_LDATA32:
    case S_GDATA32:
        *name = cv_symbol->data_v3.name;
        break;
    case S_LTHREAD32:
    case S_GTHREAD32:
        *name = cv_symbol->thread_v3.name;
        break;
    default:
        return R_PDB_INVALID_ARGUMENT;
    }
    *length = strlen(*name);
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_read_DBI_codeview_symbol_by_name(struct pdb_reader *pdb, const char *name,
                                                                   pdbsize_t *stream_offset, union codeview_symbol *cv_symbol)
{
    enum pdb_result result;
    UINT32 hash;
    struct pdb_reader_walker walker;
    union codeview_symbol *full_cv_symbol;
    char *cv_name;
    size_t cv_length;

    if ((result = pdb_reader_walker_init(pdb, pdb->dbi_header.gsym_stream, &walker))) return result;
    hash = codeview_compute_hash(name, strlen(name)) % DBI_MAX_HASH;
    if (pdb->dbi_symbols_hash[hash].next != &pdb->dbi_symbols_hash[hash])
    {
        struct pdb_dbi_hash_entry *entry;
        for (entry = &pdb->dbi_symbols_hash[hash]; entry; entry = entry->next)
        {
            walker.offset = entry->dbi_stream_offset;
            if ((result = pdb_reader_alloc_and_read_full_codeview_symbol(pdb, &walker, &full_cv_symbol))) return result;
            if (pdb_reader_extract_name_out_of_codeview_symbol(full_cv_symbol, &cv_name, &cv_length) == R_PDB_SUCCESS)
            {
                if (!strcmp(name, cv_name))
                {
                    *cv_symbol = *full_cv_symbol;
                    pdb_reader_free(pdb, full_cv_symbol);
                    *stream_offset = entry->dbi_stream_offset;
                    return R_PDB_SUCCESS;
                }
                pdb_reader_free(pdb, full_cv_symbol);
            }
        }
    }
    TRACE("not found in hash bucket %s\n", debugstr_a(name));
    return R_PDB_NOT_FOUND;
}

/* FIXME temp forward */
static enum pdb_result pdb_reader_ensure_symbols_loaded_from_compiland(struct pdb_reader *pdb, unsigned compiland_index);

static enum pdb_result pdb_reader_init_DBI(struct pdb_reader *pdb)
{
    enum pdb_result result;
    struct pdb_reader_compiland_iterator compiland_iter;
    struct pdb_reader_walker walker;
    union codeview_symbol cv_symbol;
    symref_t symref;
    unsigned i;

    if ((result = pdb_reader_read_DBI_header(pdb, &pdb->dbi_header, &walker))) return result;

    /* count number of compilands */
    if ((result = pdb_reader_compiland_iterator_init(pdb, &compiland_iter))) return result;
    do
    {
        pdb->num_compilands++;
    } while ((result = pdb_reader_compiland_iterator_next(pdb, &compiland_iter)) == R_PDB_SUCCESS);
    if ((result = pdb_reader_alloc(pdb, pdb->num_compilands * sizeof(pdb->compilands[0]), (void **)&pdb->compilands))) return result;
    memset(pdb->compilands, 0, pdb->num_compilands * sizeof(pdb->compilands[0]));
    /* fill-in compiland information */
    if ((result = pdb_reader_compiland_iterator_init(pdb, &compiland_iter))) return result;
    for (i = 0; i < pdb->num_compilands; i++)
    {
        pdb->compilands[i].stream_offset = compiland_iter.dbi_walker.offset;
        pdb->compilands[i].compiland = NULL;
        pdb->compilands[i].stream_id = compiland_iter.dbi_cu_header.stream;
        pdb->compilands[i].are_symbols_loaded = pdb->compilands[i].stream_id == 0xffff;
        result = pdb_reader_compiland_iterator_next(pdb, &compiland_iter);
        if ((result == R_PDB_SUCCESS) != (i + 1 < pdb->num_compilands)) return result ? result : R_PDB_INVALID_PDB_FILE;
    }
    /* TEMP force loading of all compilands */
    for (i = 0; i < pdb->num_compilands; i++)
    {
        if ((result = pdb_reader_ensure_symbols_loaded_from_compiland(pdb, i))) return result;
    }
    if ((result = pdb_reader_load_DBI_hash_table(pdb))) return result;

    /* register the globals entries not bound to a compiland */
    if ((result = pdb_reader_walker_init(pdb, pdb->dbi_header.gsym_stream, &walker))) return result;
    while (pdb_reader_READ(pdb, &walker, &cv_symbol.generic) == R_PDB_SUCCESS)
    {
        switch (cv_symbol.generic.id)
        {
        case S_UDT:
            if ((result = pdb_reader_push_action(pdb, action_type_globals, walker.offset - sizeof(cv_symbol.generic),
                                                 cv_symbol.generic.len + sizeof(cv_symbol.generic.len), 0, &symref))) return result;
            break;
        case S_GDATA32:
            {
                DWORD64 address;
                symref_t type_symref;
                union codeview_symbol *cv_global_symbol;
                union codeview_symbol zzcv;
                struct pdb_reader_walker global_walker = walker;
                pdbsize_t global_hash_offset, global_offset;

                /* There are cases (incremental linking) where we have several entries of same name, but
                 * only one is valid.
                 * We discriminate valid with:
                 * - the hash for that name points to this global entry
                 * - the address is valid
                 * - the typeid is valid
                 * Note: checking address map doesn't bring nothing as the invalid entries are also listed
                 * there.
                 */
                global_walker.offset -= sizeof(cv_symbol.generic);
                global_offset = global_walker.offset;
                if (!pdb_reader_alloc_and_read_full_codeview_symbol(pdb, &global_walker, &cv_global_symbol))
                {
                    if (!pdb_reader_read_DBI_codeview_symbol_by_name(pdb, cv_global_symbol->data_v3.name, &global_hash_offset, &zzcv) &&
                        global_hash_offset == global_offset &&
                        !pdb_reader_get_segment_address(pdb, cv_global_symbol->data_v3.segment, cv_global_symbol->data_v3.offset, &address) &&
                        !pdb_reader_symref_from_cv_typeid(pdb, cv_global_symbol->data_v3.symtype, &type_symref))
                    {
                        struct location loc = {.kind = loc_absolute, .reg = 0, .offset = address};
                        symt_new_global_variable(pdb->module, 0, cv_global_symbol->data_v3.name,
                                                 FALSE, loc, 0, type_symref);
                    }
                    pdb_reader_free(pdb, cv_global_symbol);
                }
            }
            break;
        }
        walker.offset += cv_symbol.generic.len - sizeof(cv_symbol.generic.id);
    }
    pdb->num_action_globals = pdb->num_action_entries;

    return R_PDB_SUCCESS;
}

static void pdb_method_location_compute(const struct module_format* modfmt,
                                        const struct symt_function* func,
                                        struct location* loc)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    struct pdb_reader *pdb;
    union codeview_symbol cv_local;
    union codeview_symbol *cv_def;
    DWORD64 ip = modfmt->module->process->localscope_pc;
    struct location in_loc = *loc;

    loc->kind = loc_error;
    loc->reg = loc_err_internal;

    if (!pdb_hack_get_main_info((struct module_format *)modfmt, &pdb, NULL)) return;
    if (in_loc.kind != loc_cv_defrange || pdb_reader_walker_init(pdb, in_loc.reg, &walker)) return;
    walker.offset = in_loc.offset;

    /* we have in location: in_loc.reg = stream_id, in_loc.offset offset in stream_id to point to S_LOCAL */
    if ((result = pdb_reader_read_partial_codeview_symbol(pdb, &walker, &cv_local))) return;
    walker.offset += cv_local.generic.len;

    while ((result = pdb_reader_alloc_and_read_full_codeview_symbol(pdb, &walker, &cv_def)) == R_PDB_SUCCESS &&
           cv_def->generic.id >= S_DEFRANGE && cv_def->generic.id <= S_DEFRANGE_REGISTER_REL)
    {
        BOOL inside = TRUE;

        /* S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE matches full function scope...
         * Assuming that if we're here, ip matches the function for which we're
         * considering the S_LOCAL and S_DEFRANGE_*, there's nothing to do.
         */
        if (cv_def->generic.id != S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE)
        {
            const struct cv_addr_range* range;
            const struct cv_addr_gap* gap;
            DWORD64 range_start;

            switch (cv_def->generic.id)
            {
            case S_DEFRANGE: range = &cv_def->defrange_v3.range; break;
            case S_DEFRANGE_SUBFIELD: range = &cv_def->defrange_subfield_v3.range; break;
            case S_DEFRANGE_REGISTER: range = &cv_def->defrange_register_v3.range; break;
            case S_DEFRANGE_FRAMEPOINTER_REL: range = &cv_def->defrange_frameptrrel_v3.range; break;
            case S_DEFRANGE_SUBFIELD_REGISTER: range = &cv_def->defrange_subfield_register_v3.range; break;
            case S_DEFRANGE_REGISTER_REL: range = &cv_def->defrange_registerrel_v3.range; break;
            default: range = NULL;
            }

            /* check if inside range */
            if ((result = pdb_reader_get_segment_address(pdb, range->isectStart, range->offStart, &range_start)))
            {
                pdb_reader_free(pdb, cv_def);
                return;
            }
            inside = range_start <= ip && ip < range_start + range->cbRange;

            /* the gaps describe part which shall be excluded from range */
            for (gap = (const void*)(range + 1);
                 inside && (const char*)(gap + 1) <= (const char*)cv_def + sizeof(cv_def->generic.len) + cv_def->generic.len;
                 gap++)
            {
                if (func->ranges[0].low + gap->gapStartOffset <= ip &&
                    ip < func->ranges[0].low + gap->gapStartOffset + gap->cbRange)
                    inside = FALSE;
            }
        }
        if (!inside)
        {
            pdb_reader_free(pdb, cv_def);
            continue;
        }

        switch (cv_def->generic.id)
        {
        case S_DEFRANGE:
        case S_DEFRANGE_SUBFIELD:
        default:
            WARN("Unsupported defrange %d\n", cv_def->generic.id);
            loc->kind = loc_error;
            loc->reg = loc_err_internal;
            break;
        case S_DEFRANGE_SUBFIELD_REGISTER:
            WARN("sub-field part not handled\n");
            /* fall through */
        case S_DEFRANGE_REGISTER:
            loc->kind = loc_register;
            loc->reg = cv_def->defrange_register_v3.reg;
            break;
        case S_DEFRANGE_REGISTER_REL:
            loc->kind = loc_regrel;
            loc->reg = cv_def->defrange_registerrel_v3.baseReg;
            loc->offset = cv_def->defrange_registerrel_v3.offBasePointer;
            break;
        case S_DEFRANGE_FRAMEPOINTER_REL:
            loc->kind = loc_regrel;
            loc->reg = modfmt->module->cpu->frame_regno;
            loc->offset = cv_def->defrange_frameptrrel_v3.offFramePointer;
            break;
        case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
            loc->kind = loc_regrel;
            loc->reg = modfmt->module->cpu->frame_regno;
            loc->offset = cv_def->defrange_frameptr_relfullscope_v3.offFramePointer;
            break;
        }
        pdb_reader_free(pdb, cv_def);
        return;
    }
    if (result == R_PDB_SUCCESS) pdb_reader_free(pdb, cv_def);
    loc->kind = loc_error;
    loc->reg = loc_err_out_of_scope;
}

static struct {enum BasicType bt; unsigned char size;} supported_basic[T_MAXBASICTYPE] =
{
    /* all others are defined as 0 = btNoType */
    [T_VOID]     = {btVoid, 0},
    [T_CURRENCY] = {btCurrency, 8},
    [T_CHAR]     = {btInt, 1},
    [T_SHORT]    = {btInt, 2},
    [T_LONG]     = {btLong, 4},
    [T_QUAD]     = {btInt, 8},
    [T_OCT]      = {btInt, 16},
    [T_UCHAR]    = {btUInt, 1},
    [T_USHORT]   = {btUInt, 2},
    [T_ULONG]    = {btULong, 4},
    [T_UQUAD]    = {btUInt, 8},
    [T_UOCT]     = {btUInt, 16},
    [T_BOOL08]   = {btBool, 1},
    [T_BOOL16]   = {btBool, 2},
    [T_BOOL32]   = {btBool, 4},
    [T_BOOL64]   = {btBool, 8},
    [T_REAL16]   = {btFloat, 2},
    [T_REAL32]   = {btFloat, 4},
    [T_REAL64]   = {btFloat, 8},
    [T_REAL80]   = {btFloat, 10},
    [T_REAL128]  = {btFloat, 16},
    [T_RCHAR]    = {btChar, 1},
    [T_WCHAR]    = {btWChar, 2},
    [T_CHAR16]   = {btChar16, 2},
    [T_CHAR32]   = {btChar32, 4},
    [T_CHAR8]    = {btChar8, 1},
    [T_INT2]     = {btInt, 2},
    [T_UINT2]    = {btUInt, 2},
    [T_INT4]     = {btInt, 4},
    [T_UINT4]    = {btUInt, 4},
    [T_INT8]     = {btInt, 8},
    [T_UINT8]    = {btUInt, 8},
    [T_HRESULT]  = {btUInt, 4},
    [T_CPLX32]   = {btComplex, 8},
    [T_CPLX64]   = {btComplex, 16},
    [T_CPLX128]  = {btComplex, 32},
};

static inline BOOL is_basic_supported(unsigned basic)
{
    return basic < T_MAXBASICTYPE && supported_basic[basic].bt != btNoType;
}

static enum method_result pdb_reader_default_request(struct pdb_reader *pdb, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    switch (req)
    {
    case TI_FINDCHILDREN:
        return ((TI_FINDCHILDREN_PARAMS*)data)->Count == 0 ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_CHILDRENCOUNT:
        *((DWORD*)data) = 0;
        return MR_SUCCESS;
    case TI_GET_LEXICALPARENT:
        *((DWORD*)data) = symt_ptr_to_index(pdb->module, &pdb->module->top->symt);
        return MR_SUCCESS;
    default:
        FIXME("Unexpected request %x\n", req);
        return MR_FAILURE;
    }
}

static enum method_result pdb_reader_basic_request(struct pdb_reader *pdb, unsigned basic, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    struct symref_code code;
    symref_t symref;

    if (!is_basic_supported(basic & T_BASICTYPE_MASK))
    {
        PDB_REPORT_UNEXPECTED("basic type", basic);
        return MR_FAILURE;
    }

    switch (req)
    {
    case TI_GET_BASETYPE:
        if (basic >= T_MAXBASICTYPE) return MR_FAILURE;
        *((DWORD*)data) = supported_basic[basic & T_BASICTYPE_MASK].bt;
        break;
    case TI_GET_LENGTH:
        switch (basic & T_MODE_MASK)
        {
        case 0:                *((DWORD64*)data) = supported_basic[basic & T_BASICTYPE_MASK].size; break;
        /* pointer type */
        case T_NEARPTR_BITS:   *((DWORD64*)data) = pdb->module->cpu->word_size; break;
        case T_NEAR32PTR_BITS: *((DWORD64*)data) = 4; break;
        case T_NEAR64PTR_BITS: *((DWORD64*)data) = 8; break;
        default: return MR_FAILURE;
        }
        break;
    case TI_GET_SYMTAG:
        *((DWORD*)data) = (basic < T_MAXBASICTYPE) ? SymTagBaseType : SymTagPointerType;
        break;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        if (basic < T_MAXBASICTYPE) return MR_FAILURE;
        if (pdb_reader_encode_symref(pdb, symref_code_init_from_cv_typeid(&code, basic & T_BASICTYPE_MASK), &symref)) return MR_FAILURE;
        *((DWORD*)data) = symt_symref_to_index(pdb->module, symref);
        break;
    case TI_FINDCHILDREN:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);
    default:
        return MR_FAILURE;
    }
    return MR_SUCCESS;
}

static enum pdb_result pdb_reader_read_cv_typeid_hash(struct pdb_reader *pdb, cv_typ_t cv_typeid, unsigned *hash)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    unsigned value = 0;

    if ((result = pdb_reader_walker_init(pdb, pdb->tpi_header.hash_stream, &walker))) return result;
    walker.offset += pdb->tpi_header.hash_offset + (cv_typeid - pdb->tpi_header.first_index) * pdb->tpi_header.hash_value_size;

    /* little endian reading */
    if ((result = pdb_reader_internal_read_advance(pdb, &walker, &value, pdb->tpi_header.hash_value_size))) return result;
    if (value >= pdb->tpi_header.hash_num_buckets)
    {
        WARN("hash value %x isn't within hash table boundaries (0, %x(\n", value, pdb->tpi_header.hash_num_buckets);
        return R_PDB_INVALID_PDB_FILE;
    }
    *hash = value;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_init_TPI(struct pdb_reader *pdb)
{
    enum pdb_result result;
    unsigned i;

    if (pdb->TPI_types_invalid) return R_PDB_INVALID_PDB_FILE;
    if (!pdb->tpi_typemap) /* load basic types information and hash table */
    {
        if ((result = pdb_reader_walker_init(pdb, PDB_STREAM_TPI, &pdb->tpi_types_walker))) goto invalid_file;
        /* assuming stream is always big enough go hold a full PDB_TYPES */
        if ((result = pdb_reader_READ(pdb, &pdb->tpi_types_walker, &pdb->tpi_header))) goto invalid_file;
        result = R_PDB_INVALID_PDB_FILE;
        if (pdb->tpi_header.version < 19960000 || pdb->tpi_header.type_offset < sizeof(PDB_TYPES))
        {
            /* not supported yet... */
            FIXME("Old PDB_TYPES header, skipping\n");
            goto invalid_file;
        }
        /* validate some bits */
        if (pdb->tpi_header.hash_size != (pdb->tpi_header.last_index - pdb->tpi_header.first_index) * pdb->tpi_header.hash_value_size ||
            pdb->tpi_header.search_size % sizeof(uint32_t[2]))
            goto invalid_file;
        if (pdb->tpi_header.hash_value_size > sizeof(unsigned))
        {
            FIXME("Unexpected hash value size %u\n", pdb->tpi_header.hash_value_size);
            goto invalid_file;
        }
        pdb->tpi_types_walker.offset = pdb->tpi_header.type_offset;

        if ((result = pdb_reader_alloc(pdb, (pdb->tpi_header.last_index - pdb->tpi_header.first_index) * sizeof(pdb->tpi_typemap[0]),
                                       (void **)&pdb->tpi_typemap)))
            goto invalid_file;
        memset(pdb->tpi_typemap, 0, (pdb->tpi_header.last_index - pdb->tpi_header.first_index) * sizeof(pdb->tpi_typemap[0]));
        if ((result = pdb_reader_alloc(pdb, pdb->tpi_header.hash_num_buckets * sizeof(struct pdb_type_hash_entry), (void **)&pdb->tpi_types_hash)))
            goto invalid_file;
        /* create hash table: mark empty slots */
        for (i = 0; i < pdb->tpi_header.hash_num_buckets; i++)
            pdb->tpi_types_hash[i].next = &pdb->tpi_types_hash[i];

        for (i = pdb->tpi_header.first_index; i < pdb->tpi_header.last_index; i++)
        {
            unsigned hash;

            if ((result = pdb_reader_read_cv_typeid_hash(pdb, i, &hash))) goto invalid_file;
            if (pdb->tpi_types_hash[hash].next == &pdb->tpi_types_hash[hash])
            {
                pdb->tpi_types_hash[hash].next = NULL;
            }
            else
            {
                struct pdb_type_hash_entry *hash_entry;
                if ((result = pdb_reader_alloc(pdb, sizeof(*hash_entry), (void**)&hash_entry))) goto invalid_file;
                *hash_entry = pdb->tpi_types_hash[hash];
                pdb->tpi_types_hash[hash].next = hash_entry;
            }
            pdb->tpi_types_hash[hash].cv_typeid = i;
        }
        if (pdb->tpi_header.type_remap_size)
        {
            struct pdb_reader_walker remap_walker, remap_bitset_walker;
            struct {uint32_t count, capacity, count_present;} head;
            uint32_t deleted_bitset_count, i, mask;
            unsigned hash;

            TRACE("Loading TPI remap information\n");
            if ((result = pdb_reader_walker_init(pdb, pdb->tpi_header.hash_stream, &remap_walker))) goto invalid_file;
            remap_walker.offset = pdb->tpi_header.type_remap_offset;
            if ((result = pdb_reader_READ(pdb, &remap_walker, &head))) goto invalid_file;
            remap_bitset_walker = remap_walker;
            remap_walker.offset += head.count_present * sizeof(uint32_t); /* skip bitset */
            if ((result = pdb_reader_READ(pdb, &remap_walker, &deleted_bitset_count))) goto invalid_file;
            remap_walker.offset += deleted_bitset_count * sizeof(uint32_t); /* skip deleted bitset */
            for (i = 0; i < head.capacity; ++i)
            {
                if ((i % (8 * sizeof(uint32_t))) == 0 &&
                    (result = pdb_reader_READ(pdb, &remap_bitset_walker, &mask))) goto invalid_file;
                if (mask & (1u << (i % (8 * sizeof(uint32_t)))))
                {
                    /* remap[0] is an offset for a string in /string stream, followed by type_id to force */
                    uint32_t target_cv_typeid;
                    struct pdb_type_hash_entry *hash_entry, *prev_hash_entry = NULL;

                    remap_walker.offset += sizeof(uint32_t); /* skip offset in /string stream */
                    if ((result = pdb_reader_READ(pdb, &remap_walker, &target_cv_typeid))) goto invalid_file;
                    if ((result = pdb_reader_read_cv_typeid_hash(pdb, target_cv_typeid, &hash))) goto invalid_file;
                    if (pdb->tpi_types_hash[hash].next == &pdb->tpi_types_hash[hash]) /* empty list */
                        goto invalid_file;
                    for (hash_entry = &pdb->tpi_types_hash[hash]; hash_entry; hash_entry = hash_entry->next)
                    {
                        if (hash_entry->cv_typeid == target_cv_typeid)
                        {
                            struct pdb_type_hash_entry swap;

                            TRACE("Remap: using cv_typeid %x instead of %x\n", hash_entry->cv_typeid, pdb->tpi_types_hash[hash].cv_typeid);
                            /* put hash_entry content at head in list */
                            if (prev_hash_entry)
                            {
                                prev_hash_entry->next = hash_entry->next;
                                swap = pdb->tpi_types_hash[hash];
                                pdb->tpi_types_hash[hash] = *hash_entry;
                                pdb->tpi_types_hash[hash].next = hash_entry;
                                *hash_entry = swap;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    return R_PDB_SUCCESS;
invalid_file:
    WARN("Invalid TPI hash table\n");
    /* free hash table upon error!! */
    if (pdb->tpi_types_hash)
    {
        struct pdb_type_hash_entry *hash_entry, *hash_entry_next;
        for (i = 0; i < pdb->tpi_header.hash_num_buckets; i++)
            if (pdb->tpi_types_hash[i].next != &pdb->tpi_types_hash[i])
            {
                for (hash_entry = pdb->tpi_types_hash[i].next; hash_entry; hash_entry = hash_entry_next)
                {
                    hash_entry_next = hash_entry->next;
                    pdb_reader_free(pdb, hash_entry);
                }
            }
        pdb_reader_free(pdb, pdb->tpi_types_hash);
        pdb->tpi_types_hash = NULL;
    }
    pdb_reader_free(pdb, pdb->tpi_typemap);
    pdb->TPI_types_invalid = 1;

    return result;
}

static enum pdb_result pdb_reader_get_type_details(struct pdb_reader *pdb, cv_typ_t cv_typeid, struct pdb_type_details **type_details)
{
    enum pdb_result result;

    if ((result = pdb_reader_init_TPI(pdb))) return result;
    if (cv_typeid < pdb->tpi_header.first_index || cv_typeid >= pdb->tpi_header.last_index) return R_PDB_INVALID_ARGUMENT;
    *type_details = &pdb->tpi_typemap[cv_typeid - pdb->tpi_header.first_index];
    return R_PDB_SUCCESS;
}

/* caller must ensure that num_elt are not zero */
static enum pdb_result pdb_reader_internal_binary_search(size_t num_elt,
                                                         enum pdb_result (*cmp)(unsigned idx, int *cmp_ressult, void *user),
                                                         size_t *found, void *user)
{
    enum pdb_result result;
    size_t low = 0, high = num_elt, mid;
    int res;

    *found = num_elt;
    while (high > low + 1)
    {
        mid = (high + low) / 2;
        if ((result = (*cmp)(mid, &res, user))) return result;
        if (!res)
        {
            *found = low;
            return R_PDB_SUCCESS;
        }
        if (res < 0)
            low = mid;
        else
            high = mid;
    }

    /* call again cmd so user can be filled with reported index */
    (*cmp)(low, &res, user);
    *found = low;
    return R_PDB_NOT_FOUND;
}

struct type_offset
{
    struct pdb_reader *pdb;
    struct pdb_reader_walker walker;
    cv_typ_t to_search;
    uint32_t values[2];
};

static enum pdb_result pdb_reader_type_offset_cmp(unsigned idx, int *cmp, void *user)
{
    enum pdb_result result;
    struct type_offset *type_offset = user;
    struct pdb_reader_walker walker = type_offset->walker;

    walker.offset += idx * sizeof(uint32_t[2]);
    if ((result = pdb_reader_READ(type_offset->pdb, &walker, &type_offset->values))) return result;
    *cmp = type_offset->values[0] - type_offset->to_search;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_TPI_offset_from_cv_typeid(struct pdb_reader *pdb, cv_typ_t cv_typeid, pdbsize_t *found_type_offset)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    struct type_offset type_offset;
    size_t found;
    unsigned short int cv_type_len;

    if ((result = pdb_reader_init_TPI(pdb))) return result;
    walker = pdb->tpi_types_walker;

    type_offset.pdb = pdb;
    if ((result = pdb_reader_walker_init(pdb, pdb->tpi_header.hash_stream, &type_offset.walker))) return result;
    type_offset.to_search = cv_typeid;
    if ((result = pdb_reader_walker_narrow(&type_offset.walker, pdb->tpi_header.search_offset, pdb->tpi_header.search_size))) return result;
    result = pdb_reader_internal_binary_search(pdb->tpi_header.search_size / sizeof(uint32_t[2]),
                                               pdb_reader_type_offset_cmp, &found, &type_offset);

    if (result)
    {
        if (result != R_PDB_NOT_FOUND) return result;

        if (type_offset.values[0] > cv_typeid) return R_PDB_INVALID_PDB_FILE;
        walker.offset += type_offset.values[1];

        for ( ; type_offset.values[0] < cv_typeid; type_offset.values[0]++)
        {
            if ((result = pdb_reader_READ(pdb, &walker, &cv_type_len))) return result;
            walker.offset += cv_type_len;
        }
    }
    else walker.offset += type_offset.values[1];
    *found_type_offset = walker.offset;

    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_read_partial_codeview_type(struct pdb_reader *pdb, struct pdb_reader_walker *walker,
                                                             union codeview_type *cv_type)
{
    return pdb_reader_read_partial_blob(pdb, walker, cv_type, sizeof(*cv_type));
}

/* deserialize the variable part of a codeview_type...
 * as output, string and/or decorated are optional
 */
static enum pdb_result pdb_reader_alloc_and_read_codeview_type_variablepart(struct pdb_reader *pdb, struct pdb_reader_walker walker,
                                                                            const union codeview_type *cv_type, VARIANT *variant,
                                                                            char **string, char **decorated)
{
    enum pdb_result result;
    size_t var_offset;
    BOOL has_leaf = TRUE, has_decorated = FALSE;
    unsigned leaf_len;

    switch (cv_type->generic.id)
    {
    case LF_CLASS_V3:
    case LF_STRUCTURE_V3:
        var_offset = offsetof(union codeview_type, struct_v3.data);
        has_decorated = cv_type->struct_v3.property.has_decorated_name;
        break;
    case LF_UNION_V3:
        var_offset = offsetof(union codeview_type, union_v3.data);
        has_decorated = cv_type->union_v3.property.has_decorated_name;
        break;
    case LF_ENUM_V3:
        var_offset = offsetof(union codeview_type, enumeration_v3.name);
        has_decorated = cv_type->enumeration_v3.property.has_decorated_name;
        has_leaf = FALSE;
        break;
    default:
        return R_PDB_NOT_FOUND;
    }
    walker.offset += var_offset - sizeof(cv_type->generic.len);
    leaf_len = walker.offset;
    if (!has_leaf)
        V_VT(variant) = VT_EMPTY;
    else if ((result = pdb_reader_read_leaf_as_variant(pdb, &walker, variant))) return result;
    leaf_len = walker.offset - leaf_len;

    if (string)
        result = pdb_reader_alloc_and_fetch_string(pdb, &walker, string);
    else if (decorated)
        result = pdb_reader_skip_string(pdb, &walker);
    if (result == R_PDB_SUCCESS && decorated)
    {
        if (!has_decorated)
            *decorated = NULL;
        else if ((result = pdb_reader_alloc_and_fetch_string(pdb, &walker, decorated))) return result;
    }
    return result;
}

static enum pdb_result pdb_reader_TPI_read_partial_reftype(struct pdb_reader *pdb, cv_typ_t cv_typeid, union codeview_reftype *cv_reftype,
                                                           pdbsize_t *tpi_offset)
{
    struct pdb_reader_walker walker;
    enum pdb_result result;

    if ((result = pdb_reader_init_TPI(pdb))) return result;
    if ((result = pdb_reader_TPI_offset_from_cv_typeid(pdb, cv_typeid, tpi_offset))) return result;
    walker = pdb->tpi_types_walker;
    walker.offset = *tpi_offset;

    return pdb_reader_read_partial_blob(pdb, &walker, (void*)cv_reftype, sizeof(*cv_reftype));
}

static enum pdb_result pdb_reader_TPI_alloc_and_read_full_reftype(struct pdb_reader *pdb, cv_typ_t cv_typeid, union codeview_reftype **cv_reftype,
                                                                  pdbsize_t *tpi_offset)
{
    struct pdb_reader_walker walker;
    enum pdb_result result;

    if ((result = pdb_reader_init_TPI(pdb))) return result;
    if ((result = pdb_reader_TPI_offset_from_cv_typeid(pdb, cv_typeid, tpi_offset))) return result;
    walker = pdb->tpi_types_walker;
    walker.offset = *tpi_offset;

    return pdb_reader_alloc_and_read_full_blob(pdb, &walker, (void**)cv_reftype);
}

static enum pdb_result pdb_reader_read_codeview_type_by_name(struct pdb_reader *pdb, const char *name, struct pdb_reader_walker *walker,
                                                             union codeview_type *cv_type, cv_typ_t *cv_typeid)
{
    enum pdb_result result;
    VARIANT v;
    UINT32 hash; /* for now we only support 1, 2 or 4 as hash size */
    struct pdb_type_hash_entry *entry;
    pdbsize_t tpi_offset;
    char *other_name;

    if ((result = pdb_reader_init_TPI(pdb))) return result;
    *walker = pdb->tpi_types_walker;

    hash = codeview_compute_hash(name, strlen(name)) % pdb->tpi_header.hash_num_buckets;
    entry = &pdb->tpi_types_hash[hash];
    if (entry->next != entry) /* not empty */
    {
        for (; entry; entry = entry->next)
        {
            int cmp;
            if ((result = pdb_reader_TPI_offset_from_cv_typeid(pdb, entry->cv_typeid, &tpi_offset))) return result;
            walker->offset = tpi_offset;
            if ((result = pdb_reader_read_partial_codeview_type(pdb, walker, cv_type))) return result;
            result = pdb_reader_alloc_and_read_codeview_type_variablepart(pdb, *walker, cv_type, &v, &other_name, NULL);
            if (result == R_PDB_NOT_FOUND) continue;
            if (result) return result;
            cmp = strcmp(name, other_name);
            pdb_reader_free(pdb, other_name);
            if (!cmp)
            {
                *cv_typeid = entry->cv_typeid;
                return R_PDB_SUCCESS;
            }
        }
    }
    return R_PDB_NOT_FOUND;
}

static int my_action_global_cmp(const void *p1, const void *p2)
{
    pdbsize_t o1 = *(pdbsize_t*)p1;
    pdbsize_t o2 = ((const struct pdb_action_entry *)p2)->stream_offset;

    if (o1 < o2) return -1;
    if (o1 > o2) return +1;
    return 0;
}

static enum method_result pdb_method_find_type(struct module_format *modfmt, const char *name, symref_t *ref)
{
    struct pdb_reader *pdb;
    enum pdb_result result;
    struct pdb_reader_walker walker;
    cv_typ_t cv_typeid;
    union codeview_type cv_type;
    union codeview_symbol cv_symbol;
    struct symref_code code;
    struct pdb_type_details *type_details;
    pdbsize_t stream_offset;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;
    if ((result = pdb_reader_init_TPI(pdb))) return pdb_method_result(result);
    /* search in TPI hash table */
    if ((result = pdb_reader_read_codeview_type_by_name(pdb, name, &walker, &cv_type, &cv_typeid)) == R_PDB_SUCCESS)
    {
        if ((result = pdb_reader_get_type_details(pdb, cv_typeid, &type_details))) return MR_FAILURE;
        return pdb_reader_encode_symref(pdb, symref_code_init_from_cv_typeid(&code, cv_typeid), ref) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    }
    /* search in DBI globals' hash table */
    if ((result = pdb_reader_read_DBI_codeview_symbol_by_name(pdb, name, &stream_offset, &cv_symbol)) == R_PDB_SUCCESS)
    {
        struct pdb_action_entry *entry;
        entry = bsearch(&stream_offset, pdb->action_store, pdb->num_action_globals, sizeof(pdb->action_store[0]),
                        &my_action_global_cmp);
        if (entry)
            return pdb_reader_encode_symref(pdb, symref_code_init_from_action(&code, entry - pdb->action_store),
                                            ref) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    }
    return MR_FAILURE;
}

static BOOL codeview_type_is_forward(const union codeview_type* cvtype)
{
    cv_property_t property;

    switch (cvtype->generic.id)
    {
    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:     property = cvtype->struct_v3.property;       break;
    case LF_UNION_V3:     property = cvtype->union_v3.property;        break;
    case LF_ENUM_V3:      property = cvtype->enumeration_v3.property;  break;
    default:              return FALSE;
    }
    return property.is_forward_defn;
}

/* resolves forward declaration to the actual implementation
 * resolves incremental linker remap bits
 */
static enum pdb_result pdb_reader_resolve_cv_typeid(struct pdb_reader *pdb, cv_typ_t raw_cv_typeid, cv_typ_t *cv_typeid)
{
    enum pdb_result result;
    pdbsize_t tpi_offset;
    union codeview_type cv_type;
    struct pdb_type_details *type_details;
    struct pdb_reader_walker type_walker;

    if (raw_cv_typeid < T_FIRSTDEFINABLETYPE)
    {
        *cv_typeid = raw_cv_typeid;
        return R_PDB_SUCCESS;
    }
    if ((result = pdb_reader_get_type_details(pdb, raw_cv_typeid, &type_details))) return result;
    if (type_details->resolved_cv_typeid)
    {
        *cv_typeid = type_details->resolved_cv_typeid;
        return R_PDB_SUCCESS;
    }
    if ((result = pdb_reader_TPI_offset_from_cv_typeid(pdb, raw_cv_typeid, &tpi_offset))) return result;

    type_walker = pdb->tpi_types_walker;
    type_walker.offset = tpi_offset;
    if ((result = pdb_reader_read_partial_codeview_type(pdb, &type_walker, &cv_type))) return result;

    if (codeview_type_is_forward(&cv_type))
    {
        VARIANT v;
        char *udt_name;
        struct pdb_reader_walker other_walker = pdb->tpi_types_walker;
        union codeview_type other_cv_type;
        cv_typ_t other_cv_typeid;

        if ((result = pdb_reader_alloc_and_read_codeview_type_variablepart(pdb, type_walker, &cv_type, &v, &udt_name, NULL))) return result;
        result = pdb_reader_read_codeview_type_by_name(pdb, udt_name, &other_walker, &other_cv_type, &other_cv_typeid);
        pdb_reader_free(pdb, udt_name);
        switch (result)
        {
        case R_PDB_SUCCESS:
            type_details->resolved_cv_typeid = other_cv_typeid;
            if (!type_details->stream_offset) type_details->stream_offset = other_walker.offset;
            break;
        case R_PDB_NOT_FOUND: /* we can have a forward decl without a real implementation */
            type_details->resolved_cv_typeid = raw_cv_typeid;
            if (!type_details->stream_offset) type_details->stream_offset = tpi_offset;
            break;
        default:
            return result;
        }
    }
    else
    {
        type_details->resolved_cv_typeid = raw_cv_typeid;
        if (!type_details->stream_offset) type_details->stream_offset = tpi_offset;
    }

    *cv_typeid = type_details->resolved_cv_typeid;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_symref_from_cv_typeid(struct pdb_reader *pdb, cv_typ_t cv_typeid, symref_t *symref)
{
    enum pdb_result result;
    struct symref_code code;

    if (cv_typeid > T_MAXPREDEFINEDTYPE)
        if ((result = pdb_reader_resolve_cv_typeid(pdb, cv_typeid, &cv_typeid))) return result;
    return pdb_reader_encode_symref(pdb, symref_code_init_from_cv_typeid(&code, cv_typeid), symref);
}

static enum method_result pdb_method_enumerate_types(struct module_format *modfmt, BOOL (*cb)(symref_t, const char *, void*), void *user)
{
    struct pdb_reader *pdb;
    enum pdb_result result;
    unsigned i;
    struct pdb_reader_walker walker;
    struct pdb_type_details *type_details;
    struct pdb_type_hash_entry *hash_entry;
    union codeview_type cv_type;
    struct symref_code code;
    symref_t symref;
    char *name;
    VARIANT v;
    BOOL ret;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;
    if ((result = pdb_reader_init_TPI(pdb))) return pdb_method_result(result);
    walker = pdb->tpi_types_walker;
    /* Note: walking the types through the hash table may not be the most efficient */
    for (i = 0; i < pdb->tpi_header.hash_num_buckets; i++)
    {
        if (&pdb->tpi_types_hash[i] == pdb->tpi_types_hash[i].next) continue; /* empty */
        for (hash_entry = &pdb->tpi_types_hash[i]; hash_entry; hash_entry = hash_entry->next)
        {
            cv_typ_t cv_typeid;

            /* We don't advertize a forward declaration unless a real declaration exists.
             * So advertize only TPI entries that are resolved to themselves.
             */
            if ((result = pdb_reader_resolve_cv_typeid(pdb, hash_entry->cv_typeid, &cv_typeid))) continue;
            if (hash_entry->cv_typeid != cv_typeid) continue;
            if ((result = pdb_reader_get_type_details(pdb, cv_typeid, &type_details))) continue;

            walker.offset = type_details->stream_offset;
            if ((result = pdb_reader_read_partial_codeview_type(pdb, &walker, &cv_type))) return pdb_method_result(result);
            result = pdb_reader_alloc_and_read_codeview_type_variablepart(pdb, walker, &cv_type, &v, &name, NULL);
            if (!result)
            {
                if (*name && pdb_reader_encode_symref(pdb, symref_code_init_from_cv_typeid(&code, hash_entry->cv_typeid), &symref) == R_PDB_SUCCESS)
                    ret = (*cb)(symref, name, user);
                else
                    ret = TRUE;
                pdb_reader_free(pdb, name);
                if (!ret) return MR_SUCCESS;
            }
        }
    }

    /* typedef:s are stored in DBI globals' stream */
    for (i = 0; i < pdb->num_action_globals; i++)
    {
        struct pdb_reader_walker walker;
        union codeview_symbol *cv_symbol;
        struct pdb_action_entry *entry;
        pdbsize_t num_read;

        entry = &pdb->action_store[i];
        if ((result = pdb_reader_walker_init(pdb, pdb->dbi_header.gsym_stream, &walker))) return MR_FAILURE;
        walker.offset = entry->stream_offset;
        if ((result = pdb_reader_alloc(pdb, entry->action_length, (void**)&cv_symbol))) return MR_FAILURE;
        if ((result = pdb_reader_read_from_stream(pdb, &walker, cv_symbol, entry->action_length, &num_read))) return MR_FAILURE;
        if ((result = pdb_reader_encode_symref(pdb, symref_code_init_from_action(&code, i), &symref))) return MR_FAILURE;
        if (num_read == entry->action_length)
        {
            switch (cv_symbol->generic.id)
            {
            case S_UDT:
                ret = (*cb)(symref, cv_symbol->udt_v3.name, user);
                break;
            default:
                WARN("Got unexpected %x\n", cv_symbol->generic.id);
                ret = FALSE;
                break;
            }
        }
        else ret = TRUE;
        pdb_reader_free(pdb, cv_symbol);
        if (!ret) break;
    }
    return MR_SUCCESS;
}

static enum pdb_result pdb_reader_index_from_cv_typeid(struct pdb_reader *pdb, cv_typ_t cv_typeid, DWORD *index)
{
    enum pdb_result result;
    struct symref_code code;
    symref_t symref;

    if (cv_typeid > T_MAXPREDEFINEDTYPE)
        if ((result = pdb_reader_resolve_cv_typeid(pdb, cv_typeid, &cv_typeid))) return result;
    if ((result = pdb_reader_encode_symref(pdb, symref_code_init_from_cv_typeid(&code, cv_typeid), &symref))) return result;

    *index = symt_symref_to_index(pdb->module, symref);
    return R_PDB_SUCCESS;
}

static enum method_result pdb_reader_request_symref_t(struct pdb_reader *pdb, symref_t symref, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data);

static enum method_result pdb_reader_request_cv_typeid(struct pdb_reader *pdb, cv_typ_t cv_typeid, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    struct symref_code code;
    symref_t target_symref;

    if (pdb_reader_encode_symref(pdb, symref_code_init_from_cv_typeid(&code, cv_typeid), &target_symref)) return MR_FAILURE;
    return pdb_reader_request_symref_t(pdb, target_symref, req, data);
}

static enum method_result pdb_reader_TPI_pointer_request(struct pdb_reader *pdb, const union codeview_type *cv_type, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    switch (req)
    {
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagPointerType;
        return MR_SUCCESS;
    case TI_GET_LENGTH:
        *((DWORD64*)data) = pdb->module->cpu->word_size;
        return MR_SUCCESS;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        return pdb_reader_index_from_cv_typeid(pdb, cv_type->pointer_v2.datatype,
                                               (DWORD*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_FINDCHILDREN:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);

    default:
        return MR_FAILURE;
    }
}

static enum method_result pdb_reader_TPI_array_request(struct pdb_reader *pdb, const union codeview_type *cv_type, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    enum method_result mr;
    DWORD64 element_length;
    int array_size;

    switch (req)
    {
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagArrayType;
        return MR_SUCCESS;
    case TI_GET_COUNT:
        if ((mr = pdb_reader_request_cv_typeid(pdb, cv_type->array_v3.elemtype, TI_GET_LENGTH, &element_length)) != MR_SUCCESS) return mr;
        if (codeview_fetch_leaf_as_int(cv_type, cv_type->array_v3.data, &array_size)) return MR_FAILURE;
        if (!element_length || (array_size % element_length))
            WARN("Incorrect array %u %I64u\n", array_size, element_length);
        *((DWORD*)data) = array_size / element_length;
        return MR_SUCCESS;
    case TI_GET_LENGTH:
        if (codeview_fetch_leaf_as_int(cv_type, cv_type->array_v3.data, &array_size)) return MR_FAILURE;
        *((DWORD64*)data) = (unsigned)array_size;
        return MR_SUCCESS;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        return pdb_reader_index_from_cv_typeid(pdb, cv_type->array_v3.elemtype, (DWORD*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_ARRAYINDEXTYPEID:
        return pdb_reader_index_from_cv_typeid(pdb, cv_type->array_v3.idxtype, (DWORD*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_FINDCHILDREN:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);
    default:
        return MR_FAILURE;
    }
}

static enum pdb_result pdb_reader_TPI_fillin_arglist(struct pdb_reader *pdb, unsigned num_ids, unsigned id_size, pdbsize_t tpi_offset,
                                                     TI_FINDCHILDREN_PARAMS *tfp)
{
    enum pdb_result result;
    symref_t symref;
    unsigned i;

    for (i = 0; i < num_ids; i++, tpi_offset += id_size)
    {
        if (i >= tfp->Start + tfp->Count) return R_PDB_BUFFER_TOO_SMALL;
        if (i >= tfp->Start)
        {
            if ((result = pdb_reader_push_action(pdb, action_type_cv_typ_t, tpi_offset, id_size, 0, &symref))) return result;
            tfp->ChildId[i] = symt_symref_to_index(pdb->module, symref);
        }
    }
    return R_PDB_SUCCESS;
}

static enum method_result pdb_reader_TPI_procsignature_request(struct pdb_reader *pdb, const union codeview_type *cv_type, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    enum pdb_result result;
    cv_typ_t return_cv_typeid, arglist_cv_typeid;
    unsigned char call_conv;
    unsigned num_args;

    switch (cv_type->generic.id)
    {
    case LF_PROCEDURE_V2:
        return_cv_typeid = cv_type->procedure_v2.rvtype;
        arglist_cv_typeid = cv_type->procedure_v2.arglist;
        call_conv = cv_type->procedure_v2.callconv;
        num_args = cv_type->procedure_v2.params;
        break;
    /* FIXME: for C++, this is plain wrong, but as we don't use arg types
     * nor class information, this would just do for now
     */
    case LF_MFUNCTION_V2:
        return_cv_typeid = cv_type->mfunction_v2.rvtype;
        arglist_cv_typeid = cv_type->mfunction_v2.arglist;
        call_conv = cv_type->mfunction_v2.callconv;
        num_args = cv_type->mfunction_v2.params;
        break;
    default:
        return MR_FAILURE;
    }

    switch (req)
    {
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagFunctionType;
        return MR_SUCCESS;
    case TI_FINDCHILDREN:
        {
            TI_FINDCHILDREN_PARAMS *p = data;
            union codeview_reftype cv_reftype;
            pdbsize_t tpi_arglist_offset;

            /* sigh... the codeview format doesn't have an explicit storage for the arg list item
             * so we have to fake one using the 'action' field in storage
             */
            if ((result = pdb_reader_TPI_read_partial_reftype(pdb, arglist_cv_typeid, &cv_reftype, &tpi_arglist_offset)))
                return MR_FAILURE;
            tpi_arglist_offset += offsetof(union codeview_reftype, arglist_v2.args[0]);
            if (num_args > cv_reftype.arglist_v2.num)
                return MR_FAILURE;
            result = pdb_reader_TPI_fillin_arglist(pdb, cv_reftype.arglist_v2.num, sizeof(cv_typ_t), tpi_arglist_offset, p);
            if (result && result != R_PDB_BUFFER_TOO_SMALL) return MR_FAILURE;
            if (p->Start + p->Count > cv_reftype.arglist_v2.num) return MR_FAILURE;
        }
        return MR_SUCCESS;
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_COUNT: /* should evolve when considering methods */
        {
            enum pdb_result result;
            union codeview_reftype cv_reftype;
            pdbsize_t tpi_offset;

            if ((result = pdb_reader_TPI_read_partial_reftype(pdb, arglist_cv_typeid, &cv_reftype, &tpi_offset)))
                return MR_FAILURE;
            *((DWORD*)data) = cv_reftype.arglist_v2.num;
        }
        return MR_SUCCESS;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        return pdb_reader_index_from_cv_typeid(pdb, return_cv_typeid, (DWORD*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_CALLING_CONVENTION:
        *((DWORD*)data) = call_conv;
        return MR_SUCCESS;
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);

    default:
        return MR_FAILURE;
    }
}

static enum pdb_result pdb_reader_push_action_and_filler(struct pdb_reader *pdb, enum pdb_action_type action, pdbsize_t stream_offset,
                                                         unsigned id_size, symref_t container_symref, unsigned *where, TI_FINDCHILDREN_PARAMS *tfp)
{
    enum pdb_result result;
    symref_t symref;

    if (tfp)
    {
        if (*where >= tfp->Start + tfp->Count) return R_PDB_BUFFER_TOO_SMALL;
        if (*where >= tfp->Start)
        {
            if ((result = pdb_reader_push_action(pdb, action, stream_offset, id_size, container_symref, &symref))) return result;
            tfp->ChildId[*where] = symt_symref_to_index(pdb->module, symref);
        }
    }
    (*where)++;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_TPI_fillin_enumlist(struct pdb_reader *pdb, symref_t container_symref,
                                                      cv_typ_t enumlist_cv_typeid, unsigned *where,
                                                      TI_FINDCHILDREN_PARAMS *tfp)
 {
    enum pdb_result result;
    union codeview_reftype *cv_reftype;
    pdbsize_t tpi_offset;
    const unsigned char *start, *ptr, *last;
    const union codeview_fieldtype *cv_field;
    unsigned length;
    VARIANT v;
    symref_t symref;

    if ((result = pdb_reader_TPI_alloc_and_read_full_reftype(pdb, enumlist_cv_typeid, &cv_reftype, &tpi_offset))) return result;
    if (cv_reftype->generic.id != LF_FIELDLIST_V2) return PDB_REPORT_UNEXPECTED("enum list", cv_reftype->generic.id);
    start = ptr = (const unsigned char *)cv_reftype->fieldlist.list;
    last = (const unsigned char *)cv_reftype + sizeof(cv_reftype->generic.id) + cv_reftype->generic.len;

    tpi_offset += offsetof(union codeview_reftype, fieldlist.list);

    while (ptr < last)
    {
        if (*ptr >= 0xf0) /* Padding */
        {
            ptr += *ptr & 0x0f;
            continue;
        }
        cv_field = (const union codeview_fieldtype *)ptr;
        switch (cv_field->generic.id)
        {
        case LF_ENUMERATE_V3:
            length = offsetof(union codeview_fieldtype, enumerate_v3.data);
            length += codeview_leaf_as_variant(ptr + length, &v);
            length += strlen((const char *)ptr + length) + 1;
            if (*where >= tfp->Start + tfp->Count)
            {
                pdb_reader_free(pdb, cv_reftype);
                return R_PDB_BUFFER_TOO_SMALL;
            }
            if (*where >= tfp->Start)
            {
                if ((result = pdb_reader_push_action(pdb, action_type_field, tpi_offset + (ptr - start), length, container_symref, &symref)))
                {
                    pdb_reader_free(pdb, cv_reftype);
                    return result;
                }
                tfp->ChildId[*where] = symt_symref_to_index(pdb->module, symref);
            }
            (*where)++;
            ptr += length;
            break;

        case LF_INDEX_V2:
            if ((result = pdb_reader_TPI_fillin_enumlist(pdb, container_symref, cv_field->index_v2.ref,
                                                         where, tfp))) return result;
            ptr += sizeof(cv_field->index_v2);
            break;

        default:
            pdb_reader_free(pdb, cv_reftype);
            return PDB_REPORT_UNEXPECTED("enum field list", cv_field->generic.id);
        }
    }
    pdb_reader_free(pdb, cv_reftype);
    return R_PDB_SUCCESS;
}

static enum method_result pdb_reader_TPI_enum_request(struct pdb_reader *pdb, symref_t symref, const union codeview_type *cv_type,
                                                      const struct pdb_reader_walker *walker, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    switch (req)
    {
    case TI_FINDCHILDREN:
        {
            TI_FINDCHILDREN_PARAMS *p = data;
            unsigned where = 0;
            enum pdb_result result;
            result = pdb_reader_TPI_fillin_enumlist(pdb, symref, cv_type->enumeration_v3.fieldlist, &where, p);
            if (result && result != R_PDB_BUFFER_TOO_SMALL) return MR_FAILURE;
            if (p->Start + p->Count > where) return MR_FAILURE;
        }
        return MR_SUCCESS;
    case TI_GET_CHILDRENCOUNT:
        *(DWORD*)data = cv_type->enumeration_v3.count;
         return MR_SUCCESS;
    case TI_GET_SYMNAME:
        {
            VARIANT v;
            char *string;

            if (pdb_reader_alloc_and_read_codeview_type_variablepart(pdb, *walker, cv_type, &v, &string, NULL))
                return MR_FAILURE;
            *((WCHAR**)data) = heap_allocate_symname(string);
            pdb_reader_free(pdb, string);
        }
        return *((WCHAR**)data) != NULL ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_BASETYPE:
    case TI_GET_LENGTH: /* forward to base type */
        if (cv_type->enumeration_v3.type >= T_MAXPREDEFINEDTYPE) return MR_FAILURE;
        return pdb_reader_basic_request(pdb, cv_type->enumeration_v3.type, req, data);
    case TI_GET_NESTED:
        *(DWORD*)data = 0;
        return MR_SUCCESS;
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagEnum;
        return MR_SUCCESS;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        return pdb_reader_index_from_cv_typeid(pdb, cv_type->enumeration_v3.type, (DWORD*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);
    default:
        return MR_FAILURE;
    }
}

static enum pdb_result pdb_reader_TPI_fillin_UDTlist(struct pdb_reader *pdb, symref_t container_symref,
                                                     cv_typ_t udtlist_cv_typeid, unsigned *where, TI_FINDCHILDREN_PARAMS *tfp)
{
    enum pdb_result result;
    union codeview_reftype *cv_reftype;
    pdbsize_t tpi_offset;
    const unsigned char *start, *ptr, *last;
    const union codeview_fieldtype *cv_field;
    unsigned length;
    VARIANT v;

    if ((result = pdb_reader_TPI_alloc_and_read_full_reftype(pdb, udtlist_cv_typeid, &cv_reftype, &tpi_offset))) return result;
    if (cv_reftype->generic.id != LF_FIELDLIST_V2) return PDB_REPORT_UNEXPECTED("list", cv_reftype->generic.id);
    start = ptr = (const unsigned char *)cv_reftype->fieldlist.list;
    last = (const unsigned char *)cv_reftype + sizeof(cv_reftype->generic.id) + cv_reftype->generic.len;

    tpi_offset += offsetof(union codeview_reftype, fieldlist.list);

    while (ptr < last)
    {
        if (*ptr >= 0xf0) /* Padding */
        {
            ptr += *ptr & 0x0f;
            continue;
        }
        cv_field = (const union codeview_fieldtype *)ptr;
        result = R_PDB_SUCCESS;
        switch (cv_field->generic.id)
        {
        case LF_BCLASS_V2:
            length = offsetof(union codeview_fieldtype, bclass_v2.data);
            length += codeview_leaf_as_variant(ptr + length, &v);
            /* FIXME: ignored for now */
            break;

        case LF_VBCLASS_V2:
        case LF_IVBCLASS_V2:
            length = offsetof(union codeview_fieldtype, vbclass_v2.data);
            length += codeview_leaf_as_variant(ptr + length, &v); /* vbpoff */
            length += codeview_leaf_as_variant(ptr + length, &v); /* vboff */
            /* FIXME: ignored for now */
            break;

         case LF_MEMBER_V3:
            length = offsetof(union codeview_fieldtype, member_v3.data);
            length += codeview_leaf_as_variant(ptr + length, &v);
            length += strlen((const char *)ptr + length) + 1;
            result = pdb_reader_push_action_and_filler(pdb, action_type_field, tpi_offset + (ptr - start), length, container_symref, where, tfp);
            break;

        case LF_STMEMBER_V3:
            /* FIXME: ignored for now */
            length = offsetof(union codeview_fieldtype, stmember_v3.name) + strlen(cv_field->stmember_v3.name) + 1;
            break;

        case LF_METHOD_V3:
            /* FIXME: ignored for now */
            length = offsetof(union codeview_fieldtype, method_v3.name) + strlen(cv_field->method_v3.name) + 1;
            break;

        case LF_NESTTYPE_V3:
            /* FIXME: ignored for now */
            length = offsetof(union codeview_fieldtype, nesttype_v3.name) + strlen(cv_field->nesttype_v3.name) + 1;
            break;

        case LF_VFUNCTAB_V2:
            /* FIXME: ignored for now */
            length = sizeof(cv_field->vfunctab_v2);
            break;

        case LF_ONEMETHOD_V3:
            /* FIXME: ignored for now */
            switch ((cv_field->onemethod_v3.attribute >> 2) & 7)
            {
            case 4: case 6: /* (pure) introducing virtual method */
                length = offsetof(union codeview_fieldtype, onemethod_virt_v3.name);
                break;
            default:
                length = offsetof(union codeview_fieldtype, onemethod_v3.name);
                break;
            }
            length += strlen((const char *)ptr + length) + 1;
            break;

        case LF_INDEX_V2:
            if ((result = pdb_reader_TPI_fillin_UDTlist(pdb, container_symref, cv_field->index_v2.ref, where, tfp))) return result;
            length = sizeof(cv_field->index_v2);
            break;

        default:
            result = PDB_REPORT_UNEXPECTED("UDT field list", cv_field->generic.id);
            length = 0; /* keep SAST happy */
        }
        ptr += length;
        if (result) break;
    }

    pdb_reader_free(pdb, cv_reftype);
    return result;
}

static enum pdb_result pdb_reader_count_advertized_in_UDT_fieldlist(struct pdb_reader *pdb, symref_t container_symref,
                                                                    cv_typ_t fieldlist_cv_typeid, DWORD *count)
{
    enum pdb_result result;
    unsigned where = 0;

    if ((result = pdb_reader_TPI_fillin_UDTlist(pdb, container_symref, fieldlist_cv_typeid, &where, NULL))) return result;
    *count = where;
    return R_PDB_SUCCESS;
}

static enum method_result pdb_reader_TPI_UDT_request(struct pdb_reader *pdb, symref_t symref, const union codeview_type *cv_type,
                                                     const struct pdb_reader_walker *walker, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    enum UdtKind kind;
    const unsigned char *var;
    cv_typ_t fieldlist_cv_typeid;
    cv_property_t property;
    enum pdb_result result;

    switch (cv_type->generic.id)
    {
    case LF_CLASS_V3:
    case LF_STRUCTURE_V3:
        kind = cv_type->generic.id == LF_CLASS_V3 ? UdtClass : UdtStruct;
        fieldlist_cv_typeid = cv_type->struct_v3.fieldlist;
        property = cv_type->struct_v3.property;
        var = cv_type->struct_v3.data;
        break;
    case LF_UNION_V3:
        kind = UdtUnion;
        fieldlist_cv_typeid = cv_type->union_v3.fieldlist;
        property = cv_type->union_v3.property;
        var = cv_type->union_v3.data;
        break;
    default:
        return MR_FAILURE;
    }
    /* Note: we can have a forward type here when its declared but not defined. */

    switch (req)
    {
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagUDT;
        return MR_SUCCESS;
    case TI_FINDCHILDREN:
        {
            TI_FINDCHILDREN_PARAMS *tfp = data;
            unsigned where = 0;

            if (property.is_forward_defn) return !tfp->Count ? MR_SUCCESS : MR_FAILURE;
            result = pdb_reader_TPI_fillin_UDTlist(pdb, symref, fieldlist_cv_typeid, &where, tfp);
            if (result && result != R_PDB_BUFFER_TOO_SMALL) return MR_FAILURE;
            if (tfp->Start + tfp->Count > where) return MR_FAILURE;
        }
        return MR_SUCCESS;
    case TI_GET_CHILDRENCOUNT:
        if (!property.is_forward_defn)
            /* Unfortunately the count field describes the number of entries in the field_list, not
             * the count of entries we currently advertize (we drop a bunch of them in the fillin_ helpers).
             * Be on the save side and always recompute.
             */
            return pdb_reader_count_advertized_in_UDT_fieldlist(pdb, symref, fieldlist_cv_typeid, (DWORD*)data) ? MR_FAILURE : MR_SUCCESS;
        *((DWORD*)data) = 0;
        return MR_SUCCESS;
    case TI_GET_SYMNAME:
        {
            VARIANT v;
            char *string;

            if (pdb_reader_alloc_and_read_codeview_type_variablepart(pdb, *walker, cv_type, &v, &string, NULL))
                return MR_FAILURE;
            *((WCHAR**)data) = heap_allocate_symname(string);
            pdb_reader_free(pdb, string);
        }
        return *((WCHAR**)data) != NULL ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_LENGTH:
        {
            int value;
            if (codeview_fetch_leaf_as_int(cv_type, var, &value)) return MR_FAILURE;
            *((DWORD64*)data) = (unsigned)value;
        }
        return MR_SUCCESS;
    case TI_GET_NESTED:
        *((DWORD*)data) = property.is_nested;
        return MR_SUCCESS;
    case TI_GET_UDTKIND:
        *((DWORD*)data) = kind;
        return MR_SUCCESS;
    case TI_GET_LEXICALPARENT:
        *((DWORD*)data) = symt_ptr_to_index(pdb->module, &pdb->module->top->symt);
        return MR_SUCCESS;
    default:
        return MR_FAILURE;
    }
}

static enum method_result pdb_reader_TPI_modifier_request(struct pdb_reader *pdb, symref_t symref, const union codeview_type *cv_type,
                                                          IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    return pdb_reader_request_cv_typeid(pdb, cv_type->modifier_v2.type, req, data);
}

static enum method_result pdb_reader_TPI_argtype_request(struct pdb_reader *pdb, struct pdb_action_entry *entry, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    enum pdb_result result;

    switch (req)
    {
    case TI_GET_SYMTAG:
        *(DWORD*)data = SymTagFunctionArgType;
        return MR_SUCCESS;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        {
            cv_typ_t cv_typeid;
            struct pdb_reader_walker walker = pdb->tpi_types_walker;
            walker.offset = entry->stream_offset;
            if (entry->action_length == sizeof(cv_typ16_t))
            {
                cv_typ16_t cv_typeid16;
                result = pdb_reader_READ(pdb, &walker, &cv_typeid16);
                cv_typeid = cv_typeid16;
            }
            else
                result = pdb_reader_READ(pdb, &walker, &cv_typeid);
            if (result || !cv_typeid) return MR_FAILURE;
            return pdb_reader_index_from_cv_typeid(pdb, cv_typeid, (DWORD*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
        }
    case TI_FINDCHILDREN:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);
    default:
        return MR_FAILURE;
    }
}

static enum method_result pdb_reader_TPI_enumerate_request(struct pdb_reader *pdb, struct pdb_action_entry *entry, const union codeview_fieldtype *cv_field,
                                                           IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    pdbsize_t var;
    VARIANT v;

    switch (req)
    {
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagData;
        return MR_SUCCESS;
    case TI_GET_SYMNAME:
        var = offsetof(union codeview_fieldtype, enumerate_v3.data);
        var += codeview_leaf_as_variant((const unsigned char *)cv_field + var, &v);
        *((WCHAR **)data) = heap_allocate_symname((const char *)cv_field + var);
        return *((WCHAR **)data) != NULL ? MR_SUCCESS  : MR_FAILURE;
    case TI_GET_DATAKIND:
        *((DWORD*)data) = DataIsConstant;
        return MR_SUCCESS;
    case TI_GET_LEXICALPARENT:
        *((DWORD*)data) = symt_symref_to_index(pdb->module, entry->container_symref);
        return MR_SUCCESS;
    case TI_GET_LENGTH:
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        /* forward to container type */
        return pdb_reader_request_symref_t(pdb, entry->container_symref, req, data);
    case TI_GET_VALUE:
        var = offsetof(union codeview_fieldtype, enumerate_v3.data);
        return codeview_leaf_as_variant((const unsigned char *)cv_field + var, (VARIANT*)data) != 0 ? MR_SUCCESS : MR_FAILURE;
    case TI_FINDCHILDREN:
    case TI_GET_CHILDRENCOUNT:
        return pdb_reader_default_request(pdb, req, data);
    default:
        return MR_FAILURE;
    }
}

static enum method_result pdb_reader_TPI_UDT_field_request(struct pdb_reader *pdb, struct pdb_action_entry *entry, const union codeview_fieldtype *cv_field,
                                                           IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    union codeview_reftype cv_reftype;
    pdbsize_t tpi_offset;

    switch (req)
    {
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagData;
        return MR_SUCCESS;
    case TI_GET_SYMNAME:
        {
            const unsigned char *var;
            VARIANT v;
            var = cv_field->member_v3.data;
            var += codeview_leaf_as_variant(var, &v);
            *((WCHAR **)data) = heap_allocate_symname((const char *)var);
        }
        return *((WCHAR **)data) != NULL ? MR_SUCCESS : MR_FAILURE;
     case TI_GET_DATAKIND:
        *((DWORD*)data) = DataIsMember;
        return MR_SUCCESS;
    case TI_GET_OFFSET:
        return codeview_leaf_as_int( cv_field->member_v3.data, (int*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_LENGTH:
        if (cv_field->member_v3.type < T_FIRSTDEFINABLETYPE ||
            pdb_reader_TPI_read_partial_reftype(pdb, cv_field->member_v3.type, &cv_reftype, &tpi_offset) ||
            cv_reftype.generic.id != LF_BITFIELD_V2)
            return MR_FAILURE;
        *((DWORD64*)data) = cv_reftype.bitfield_v2.nbits;
        return MR_SUCCESS;
    case TI_GET_BITPOSITION:
        if (cv_field->member_v3.type < T_FIRSTDEFINABLETYPE ||
            pdb_reader_TPI_read_partial_reftype(pdb, cv_field->member_v3.type, &cv_reftype, &tpi_offset) ||
            cv_reftype.generic.id != LF_BITFIELD_V2)
            return MR_FAILURE;
        *((DWORD*)data) = cv_reftype.bitfield_v2.bitoff;
        return MR_SUCCESS;
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        if (cv_field->member_v3.type >= T_FIRSTDEFINABLETYPE &&
            !pdb_reader_TPI_read_partial_reftype(pdb, cv_field->member_v3.type, &cv_reftype, &tpi_offset) &&
            cv_reftype.generic.id == LF_BITFIELD_V2)
            return pdb_reader_index_from_cv_typeid(pdb, cv_reftype.bitfield_v2.type, (DWORD *)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
        return pdb_reader_index_from_cv_typeid(pdb, cv_field->member_v3.type, (DWORD *)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_FINDCHILDREN:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);
    default:
        return MR_FAILURE;
    }
}

static enum method_result pdb_reader_TPI_field_request(struct pdb_reader *pdb, struct pdb_action_entry *entry, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    union codeview_fieldtype *cv_field;
    pdbsize_t num_read;
    enum method_result ret = MR_FAILURE;

    walker = pdb->tpi_types_walker;
    walker.offset = entry->stream_offset;

    if ((result = pdb_reader_alloc(pdb, entry->action_length, (void**)&cv_field))) return MR_FAILURE;
    if ((result = pdb_reader_read_from_stream(pdb, &walker, cv_field, entry->action_length, &num_read))) return MR_FAILURE;
    if (num_read == entry->action_length)
    {
        switch (cv_field->generic.id)
        {
        case LF_ENUMERATE_V3:
            ret = pdb_reader_TPI_enumerate_request(pdb, entry, cv_field, req, data);
            break;
        case LF_MEMBER_V3:
            ret = pdb_reader_TPI_UDT_field_request(pdb, entry, cv_field, req, data);
            break;
        default:
            PDB_REPORT_UNEXPECTED("field list", cv_field->generic.id);
            break;
        }
    }
    pdb_reader_free(pdb, cv_field);
    return ret;
}

static enum method_result pdb_reader_DBI_typedef_request(struct pdb_reader *pdb, struct pdb_action_entry *entry, union codeview_symbol *cv_symbol,
                                                         IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    switch (req)
    {
    case TI_GET_SYMTAG:
        *((DWORD*)data) = SymTagTypedef;
        return MR_SUCCESS;
    case TI_GET_SYMNAME:
        *((WCHAR **)data) = heap_allocate_symname(cv_symbol->udt_v3.name);
        return *((WCHAR **)data) != NULL ? MR_SUCCESS : MR_FAILURE;
    case TI_GET_LENGTH:
        return pdb_reader_request_cv_typeid(pdb, cv_symbol->udt_v3.type, req, data);
    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        return pdb_reader_index_from_cv_typeid(pdb, cv_symbol->udt_v3.type, (DWORD*)data) == R_PDB_SUCCESS ? MR_SUCCESS : MR_FAILURE;
    case TI_FINDCHILDREN:
    case TI_GET_CHILDRENCOUNT:
    case TI_GET_LEXICALPARENT:
        return pdb_reader_default_request(pdb, req, data);
    default:
        return MR_FAILURE;
    }
}

static enum method_result pdb_reader_DBI_globals_request(struct pdb_reader *pdb, struct pdb_action_entry *entry, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    union codeview_symbol *cv_symbol;
    pdbsize_t num_read;
    enum method_result ret = MR_FAILURE;

    if ((result = pdb_reader_walker_init(pdb, pdb->dbi_header.gsym_stream, &walker))) return MR_FAILURE;
    walker.offset = entry->stream_offset;
    if ((result = pdb_reader_alloc(pdb, entry->action_length, (void**)&cv_symbol))) return MR_FAILURE;
    if ((result = pdb_reader_read_from_stream(pdb, &walker, cv_symbol, entry->action_length, &num_read))) return MR_FAILURE;
    if (num_read == entry->action_length)
    {
        switch (cv_symbol->generic.id)
        {
        case S_UDT:
            ret = pdb_reader_DBI_typedef_request(pdb, entry, cv_symbol, req, data);
            break;
        default:
            WARN("Got unexpected %x\n", cv_symbol->generic.id);
            break;
        }
    }
    pdb_reader_free(pdb, cv_symbol);
    return ret;
}

static enum method_result pdb_reader_TPI_request(struct pdb_reader *pdb, symref_t symref, struct pdb_type_details *type_details, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    union codeview_type cv_type;
    enum method_result ret;

    if ((result = pdb_reader_init_TPI(pdb))) return MR_FAILURE;
    walker = pdb->tpi_types_walker;
    walker.offset = type_details->stream_offset;
    if ((result = pdb_reader_read_partial_codeview_type(pdb, &walker, &cv_type))) return MR_FAILURE;

    switch (cv_type.generic.id)
    {
    case LF_POINTER_V2:
        ret = pdb_reader_TPI_pointer_request(pdb, &cv_type, req, data);
        break;
    case LF_ARRAY_V3:
        ret = pdb_reader_TPI_array_request(pdb, &cv_type, req, data);
        break;

    case LF_PROCEDURE_V2:
    case LF_MFUNCTION_V2:
        return pdb_reader_TPI_procsignature_request(pdb, &cv_type, req, data);

    case LF_ENUM_V3:
        return pdb_reader_TPI_enum_request(pdb, symref, &cv_type, &walker, req, data);

    case LF_UNION_V3:
    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:
        return pdb_reader_TPI_UDT_request(pdb, symref, &cv_type, &walker, req, data);

    case LF_MODIFIER_V2:
        return pdb_reader_TPI_modifier_request(pdb, symref, &cv_type, req, data);

    default:
        PDB_REPORT_UNEXPECTED("codeview type id", cv_type.generic.id);
        ret = MR_FAILURE;
        break;
    }

    return ret;
}

static enum method_result pdb_reader_request_symref_t(struct pdb_reader *pdb, symref_t symref, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    struct pdb_type_details *type_details;
    struct symref_code code;

    if (req == TI_GET_SYMINDEX)
    {
        *((DWORD*)data) = symt_symref_to_index(pdb->module, symref);
        return MR_SUCCESS;
    }

    if (pdb_reader_decode_symref(pdb, symref, &code)) return MR_FAILURE;
    switch (code.kind)
    {
    case symref_code_cv_typeid:
        if (code.cv_typeid < T_MAXPREDEFINEDTYPE)
            return pdb_reader_basic_request(pdb, code.cv_typeid, req, data);
        if (pdb_reader_resolve_cv_typeid(pdb, code.cv_typeid, &code.cv_typeid)) return MR_FAILURE;
        if (pdb_reader_get_type_details(pdb, code.cv_typeid, &type_details)) return MR_FAILURE;
        return pdb_reader_TPI_request(pdb, symref, type_details, req, data);
    case symref_code_action:
        {
            struct pdb_action_entry *entry = &pdb->action_store[code.action];

            switch (entry->action_type)
            {
            case action_type_cv_typ_t:
                return pdb_reader_TPI_argtype_request(pdb, entry, req, data);
            case action_type_field:
                return pdb_reader_TPI_field_request(pdb, entry, req, data);
            case action_type_globals:
                return pdb_reader_DBI_globals_request(pdb, entry, req, data);
            default:
                return MR_FAILURE;
            }
        }
        return MR_FAILURE;
    default:
        return MR_FAILURE;
    }
}

static enum method_result pdb_method_request_symref_t(struct module_format *modfmt, symref_t symref, IMAGEHLP_SYMBOL_TYPE_INFO req, void *data)
{
    struct pdb_reader *pdb;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;
    if (pdb_reader_init_TPI(pdb)) return MR_FAILURE;
    return pdb_reader_request_symref_t(pdb, symref, req, data);
}

BOOL cv_hack_ptr_to_symref(struct pdb_reader *pdb, cv_typ_t cv_typeid, symref_t *symref)
{
    struct symref_code code;

    if (!pdb) return FALSE;
    if (pdb_reader_init_TPI(pdb)) return FALSE;
    return pdb_reader_encode_symref(pdb, symref_code_init_from_cv_typeid(&code, cv_typeid), symref) == R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_walker_from_compiland_index(struct pdb_reader *pdb, unsigned compiland,
                                                              struct pdb_reader_walker *compiland_walker,
                                                              /* optional */
                                                              PDB_SYMBOL_FILE_EX *dbi_cu_header, char **obj_name)
{
    enum pdb_result result;
    struct pdb_reader_compiland_iterator compiland_iter;

    if ((result = pdb_reader_compiland_iterator_init(pdb, &compiland_iter))) return result;
    do
    {
        if (!compiland--)
        {
            if ((result = pdb_reader_walker_init(pdb, compiland_iter.dbi_cu_header.stream, compiland_walker))) return result;
            if (obj_name && (result = pdb_reader_compiland_iterator_alloc_and_read_compiland_name(pdb, &compiland_iter, obj_name))) return result;
            if (dbi_cu_header) *dbi_cu_header = compiland_iter.dbi_cu_header;
            compiland_walker->offset += sizeof(UINT32);
            return R_PDB_SUCCESS;
        }
    } while (pdb_reader_compiland_iterator_next(pdb, &compiland_iter) == R_PDB_SUCCESS);
    return R_PDB_NOT_FOUND;
}

static enum pdb_result pdb_reader_init_IPI(struct pdb_reader *pdb)
{
    enum pdb_result result;

    if (pdb->IPI_types_invalid) return R_PDB_INVALID_PDB_FILE;
    if (pdb->ipi_walker.stream_id == 0) /* load basic types information and hash table */
    {
        if ((result = pdb_reader_walker_init(pdb, PDB_STREAM_IPI, &pdb->ipi_walker))) goto invalid_file;
        /* assuming stream is always big enough go hold a full PDB_TYPES */
        if ((result = pdb_reader_READ(pdb, &pdb->ipi_walker, &pdb->ipi_header))) goto invalid_file;
        result = R_PDB_INVALID_PDB_FILE;
        if (pdb->ipi_header.version < 19960000 || pdb->ipi_header.type_offset < sizeof(PDB_TYPES))
        {
            /* not supported yet... */
            FIXME("Old PDB_TYPES header, skipping\n");
            goto invalid_file;
        }
        /* validate some bits */
        if (pdb->ipi_header.hash_size != (pdb->ipi_header.last_index - pdb->ipi_header.first_index) * pdb->ipi_header.hash_value_size ||
            pdb->ipi_header.search_size % sizeof(uint32_t[2]))
            goto invalid_file;
        if (pdb->ipi_header.hash_value_size > sizeof(unsigned))
        {
            FIXME("Unexpected hash value size %u\n", pdb->ipi_header.hash_value_size);
            goto invalid_file;
        }
        pdb->ipi_walker.offset = pdb->ipi_header.type_offset;
    }
    return R_PDB_SUCCESS;
invalid_file:
    memset(&pdb->ipi_walker, 0, sizeof(pdb->ipi_walker));
    pdb->IPI_types_invalid = 1;
    return result;
}

static enum pdb_result pdb_reader_IPI_offset_from_cv_itemid(struct pdb_reader *pdb, cv_itemid_t cv_itemid, pdbsize_t *found_type_offset)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;
    struct type_offset type_offset;
    size_t found;
    unsigned short int cv_type_len;

    if ((result = pdb_reader_init_IPI(pdb))) return result;
    walker = pdb->ipi_walker;

    type_offset.pdb = pdb;
    if ((result = pdb_reader_walker_init(pdb, pdb->ipi_header.hash_stream, &type_offset.walker))) return result;
    type_offset.to_search = cv_itemid;
    if ((result = pdb_reader_walker_narrow(&type_offset.walker, pdb->ipi_header.search_offset, pdb->ipi_header.search_size))) return result;
    result = pdb_reader_internal_binary_search(pdb->ipi_header.search_size / sizeof(uint32_t[2]),
                                               pdb_reader_type_offset_cmp, &found, &type_offset);

    if (result)
    {
        if (result != R_PDB_NOT_FOUND) return result;

        if (type_offset.values[0] > cv_itemid) {WARN("Out of bounds\n"); return R_PDB_INVALID_PDB_FILE;}
        walker.offset += type_offset.values[1];

        for ( ; type_offset.values[0] < cv_itemid; type_offset.values[0]++)
        {
            if ((result = pdb_reader_READ(pdb, &walker, &cv_type_len))) return result;
            walker.offset += cv_type_len;
        }
    }
    else walker.offset += type_offset.values[1];
    *found_type_offset = walker.offset;

    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_IPI_alloc_and_read_full_codeview_type(struct pdb_reader *pdb, cv_itemid_t cv_itemid, union codeview_type **cv_type)
{
    enum pdb_result result;
    struct pdb_reader_walker walker;

    if ((result = pdb_reader_init_IPI(pdb))) return result;
    walker = pdb->ipi_walker;
    if ((result = pdb_reader_IPI_offset_from_cv_itemid(pdb, cv_itemid, &walker.offset))) return result;
    return pdb_reader_alloc_and_read_full_blob(pdb, &walker, (void **)cv_type);
}

/* walk the top level global symbols to find matching address */
static enum pdb_result pdb_reader_search_codeview_symbol_by_address(struct pdb_reader *pdb, struct pdb_reader_walker *walker,
                                                                    DWORD_PTR address, union codeview_symbol *cv_symbol, pdbsize_t *end_stream_offset)
{
    enum pdb_result result;
    unsigned short segment;
    unsigned offset, pend;
    DWORD64 symbol_address;

    while (pdb_reader_read_partial_codeview_symbol(pdb, walker, cv_symbol) == R_PDB_SUCCESS && cv_symbol->generic.id)
    {
        switch (cv_symbol->generic.id)
        {
        case S_GDATA32:
        case S_LDATA32:
            segment = cv_symbol->data_v3.segment;
            offset = cv_symbol->data_v3.offset;
            pend = 0;
            break;
        case S_THUNK32:
            segment = cv_symbol->thunk_v3.segment;
            offset = cv_symbol->thunk_v3.offset;
            pend = cv_symbol->thunk_v3.pend;
            break;
        case S_GPROC32:
        case S_LPROC32:
            segment = cv_symbol->proc_v3.segment;
            offset = cv_symbol->proc_v3.offset;
            pend = cv_symbol->proc_v3.pend;
            break;

        default:
            WARN("Unexpected codeview symbol id %x\n", cv_symbol->generic.id);
            /* fall through */
        case S_OBJNAME:
        case S_COMPILE:
        case S_COMPILE2:
        case S_COMPILE3:
        case S_BUILDINFO:
        case S_UDT:
        case S_UNAMESPACE:
            segment = 0;
            offset = 0;
            pend = 0;
            break;
        }
        if (segment)
        {
            if ((result = pdb_reader_get_segment_address(pdb, segment, offset, &symbol_address))) return result;
            if (address == symbol_address)
            {
                *end_stream_offset = pend;
                return R_PDB_SUCCESS;
            }
        }
        if (pend) /* jump to S_END, and skip it */
        {
            walker->offset = pend;
            if ((result = pdb_reader_read_partial_codeview_symbol(pdb, walker, cv_symbol))) return result;
            if (cv_symbol->generic.id != S_END) return R_PDB_INVALID_PDB_FILE;
        }
        walker->offset += cv_symbol->generic.len;
    }
    return R_PDB_NOT_FOUND;
}

static enum pdb_result pdb_reader_alloc_and_fetch_from_checksum(struct pdb_reader *pdb, struct pdb_reader_walker checksum_walker,
                                                                unsigned chksum_offset, char **string)
{
    enum pdb_result result;
    struct CV_Checksum_t checksum;

    checksum_walker.offset += chksum_offset;
    if ((result = pdb_reader_READ(pdb, &checksum_walker, &checksum))) return result;
    return pdb_reader_alloc_and_fetch_global_string(pdb, checksum.strOffset, string);
}

static enum pdb_result pdb_reader_alloc_and_fetch_from_checksum_subsection(struct pdb_reader *pdb, struct pdb_reader_walker linetab2_walker,
                                                                           cv_itemid_t cv_inlinee, char **string, unsigned *line_number)
{
    enum pdb_result result;
    struct pdb_reader_walker sub_walker;
    struct pdb_reader_walker checksum_walker;
    struct pdb_reader_walker inlineelines_walker;

    sub_walker = linetab2_walker;
    if ((result = pdb_reader_subsection_next(pdb, &sub_walker, DEBUG_S_FILECHKSMS, &checksum_walker)))
    {
        WARN("No DEBUG_S_FILECHKSMS found\n");
        return R_PDB_MISSING_INFORMATION;
    }

    for (sub_walker = linetab2_walker; !(result = pdb_reader_subsection_next(pdb, &sub_walker, DEBUG_S_INLINEELINES, &inlineelines_walker)); )
    {
        UINT32 inlinee_kind;
        struct CV_InlineeSourceLine_t inlsrc;
        struct CV_InlineeSourceLineEx_t inlsrcex;

        if ((result = pdb_reader_READ(pdb, &inlineelines_walker, &inlinee_kind))) return result;
        switch (inlinee_kind)
        {
        case CV_INLINEE_SOURCE_LINE_SIGNATURE:
            while (!pdb_reader_READ(pdb, &inlineelines_walker, &inlsrc))
            {
                if (inlsrc.inlinee == cv_inlinee)
                {
                    if ((result = pdb_reader_alloc_and_fetch_from_checksum(pdb, checksum_walker, inlsrc.fileId, string))) return result;
                    *line_number = inlsrc.sourceLineNum;
                    return R_PDB_SUCCESS;
                }
            }
            break;
        case CV_INLINEE_SOURCE_LINE_SIGNATURE_EX:
            while (!pdb_reader_READ(pdb, &inlineelines_walker, &inlsrcex))
            {
                if (inlsrcex.inlinee == cv_inlinee)
                {
                    if ((result = pdb_reader_alloc_and_fetch_from_checksum(pdb, checksum_walker, inlsrcex.fileId, string))) return result;
                    *line_number = inlsrcex.sourceLineNum;
                    return R_PDB_SUCCESS;
                }
                inlineelines_walker.offset += inlsrcex.countOfExtraFiles * sizeof(inlsrcex.extraFileId[0]);
            }
            break;
        default:
            WARN("Unknown signature %x in INLINEELINES subsection\n", inlinee_kind);
            break;
        }
    }
    return R_PDB_NOT_FOUND;
}

static enum pdb_result pdb_reader_uncompress_inlinesite_annotation(struct pdb_reader *pdb, struct pdb_reader_walker *walker, unsigned *value)
{
    enum pdb_result result;
    unsigned res;
    unsigned char ch;
    unsigned i, num_shift;

    if ((result = pdb_reader_READ(pdb, walker, &ch))) return result;

    if ((ch & 0x80) == 0x00)
    {
        res = ch;
        num_shift = 0;
    }
    else if ((ch & 0xC0) == 0x80)
    {
        res = ch & 0x3f;
        num_shift = 1;
    }
    else if ((ch & 0xE0) == 0xC0)
    {
        res = (ch & 0x1f);
        num_shift = 3;
    }
    else
    {
        res = (unsigned)(-1);
        num_shift = 0;
    }
    for (i = 0; i < num_shift; i++)
    {
        if ((result = pdb_reader_READ(pdb, walker, &ch))) return result;
        res <<= 8;
        res |= ch;
    }
    *value = res;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_read_inlinesite_annotation(struct pdb_reader *pdb, struct pdb_reader_walker *annotation_walker,
                                                             unsigned *opcode, unsigned *arg1, unsigned *arg2)
{
    enum pdb_result result;

    if ((result = pdb_reader_uncompress_inlinesite_annotation(pdb, annotation_walker, opcode))) return result;
    if (*opcode == BA_OP_Invalid)
        *arg1 = *arg2 = 0;
    else if (*opcode <= BA_OP_ChangeColumnEnd)
    {
        if ((result = pdb_reader_uncompress_inlinesite_annotation(pdb, annotation_walker, arg1))) return result;
        if (*opcode == BA_OP_ChangeCodeOffsetAndLineOffset)
        {
            *arg2 = *arg1 >> 4;
            *arg1 &= 0x0F;
        }
        else if (*opcode == BA_OP_ChangeCodeLengthAndCodeOffset)
        {
            if ((result = pdb_reader_uncompress_inlinesite_annotation(pdb, annotation_walker, arg2))) return result;
        }
        else *arg2 = 0;
    }
    else
    {
        WARN("Unexpected BA annotation options %x\n", *opcode);
        return R_PDB_INVALID_PDB_FILE;
    }
    return R_PDB_SUCCESS;
}

static inline int pdb_reader_convert_binannot_to_signed(unsigned i)
{
    return (i & 1) ? -(int)(i >> 1) : (int)(i >> 1);
}

static enum pdb_result pdb_method_get_line_from_inlined_address_internal(struct pdb_reader *pdb, struct symt_function *inlined,
                                                                         DWORD64 address, struct lineinfo_t *line_info)
{
    struct symt_function *function = symt_get_function_from_inlined(inlined);
    enum pdb_result result;
    PDB_SYMBOL_FILE_EX dbi_cu_header;
    struct pdb_reader_walker cu_walker;
    struct pdb_reader_walker linetab2_walker;
    struct pdb_reader_walker inlinee_walker;
    DWORD64 top_function_address;
    unsigned compiland;
    union codeview_symbol cv_top_function_symbol;
    union codeview_symbol cv_inlinee_symbol;
    pdbsize_t end_stream_offset;
    cv_itemid_t cv_inlinee;
    size_t annotation_offset;
    char *source_file_name;
    unsigned line_number;
    unsigned opcode, arg1, arg2;
    unsigned offset_top_function;

    if (!symt_get_info(pdb->module, &function->symt, TI_GET_ADDRESS, &top_function_address)) return R_PDB_INVALID_ARGUMENT;
    if ((result = pdb_reader_lookup_compiland_by_address(pdb, top_function_address, &compiland))) return result;
    if ((result = pdb_reader_walker_from_compiland_index(pdb, compiland, &cu_walker, &dbi_cu_header, NULL))) return result;

    if ((result = pdb_reader_search_codeview_symbol_by_address(pdb, &cu_walker, top_function_address, &cv_top_function_symbol, &end_stream_offset))) return result;
    if (inlined->user < cu_walker.offset || inlined->user >= end_stream_offset) return R_PDB_INVALID_ARGUMENT;

    inlinee_walker.stream_id = cu_walker.stream_id;
    inlinee_walker.offset = inlined->user;
    inlinee_walker.last = end_stream_offset;
    if ((result = pdb_reader_read_partial_codeview_symbol(pdb, &inlinee_walker, &cv_inlinee_symbol))) return result;
    switch (cv_inlinee_symbol.generic.id)
    {
    case S_INLINESITE:
        cv_inlinee = cv_inlinee_symbol.inline_site_v3.inlinee;
        annotation_offset = offsetof(union codeview_symbol, inline_site_v3.binaryAnnotations);
        break;
    case S_INLINESITE2:
        cv_inlinee = cv_inlinee_symbol.inline_site2_v3.inlinee;
        annotation_offset = offsetof(union codeview_symbol, inline_site2_v3.binaryAnnotations);
        break;
    default:
        WARN("Unexpected symbol id %x for %u\n", cv_inlinee_symbol.generic.id, inlined->symt.tag);
        return R_PDB_INVALID_PDB_FILE;
    }
    if ((result = pdb_reader_walker_init_linetab2(pdb, &dbi_cu_header, &linetab2_walker))) return result;

    if ((result = pdb_reader_alloc_and_fetch_from_checksum_subsection(pdb, linetab2_walker, cv_inlinee, &source_file_name, &line_number))) return result;

    /* then walk annotations */
    if ((result = pdb_reader_walker_narrow(&inlinee_walker,
                                           inlinee_walker.offset + annotation_offset - sizeof(cv_inlinee_symbol.generic.len),
                                           cv_inlinee_symbol.generic.len - annotation_offset + sizeof(cv_inlinee_symbol.generic.len)))) return result;
    offset_top_function = 0;
    while (!(result = pdb_reader_read_inlinesite_annotation(pdb, &inlinee_walker, &opcode, &arg1, &arg2)) &&
           opcode != BA_OP_Invalid)
    {
        BOOL check_address = FALSE;
        switch (opcode)
        {
        case BA_OP_CodeOffset:
            offset_top_function = arg1;
            break;
        case BA_OP_ChangeCodeOffset:
            offset_top_function += arg1;
            check_address = TRUE;
            break;
        case BA_OP_ChangeCodeLength:
            /* this op isn't widely used by MSVC, but clang uses it a lot... */
            offset_top_function += arg1;
            break;
        case BA_OP_ChangeFile:
            pdb_reader_free(pdb, source_file_name);
            {
                struct pdb_reader_walker sub_walker = linetab2_walker;
                struct pdb_reader_walker checksum_walker = linetab2_walker;
                if ((result = pdb_reader_subsection_next(pdb, &sub_walker, DEBUG_S_FILECHKSMS, &checksum_walker)))
                {
                    WARN("No DEBUG_S_FILECHKSMS found\n");
                    return R_PDB_MISSING_INFORMATION;
                }
                if ((result = pdb_reader_alloc_and_fetch_from_checksum(pdb, checksum_walker, arg1, &source_file_name))) return result;
            }
            break;
        case BA_OP_ChangeLineOffset:
            line_number += pdb_reader_convert_binannot_to_signed(arg1);
            break;
        case BA_OP_ChangeCodeOffsetAndLineOffset:
            line_number += pdb_reader_convert_binannot_to_signed(arg2);
            offset_top_function += arg1;
            check_address = TRUE;
            break;
        case BA_OP_ChangeCodeLengthAndCodeOffset:
            offset_top_function += arg2;
            check_address = TRUE;
            break;
        default:
            WARN("Unsupported op %d\n", opcode);
            break;
        }
        if (check_address)
        {
            if (top_function_address + offset_top_function > address) /* we're above the searched address */
                break;
            line_info->address = top_function_address + offset_top_function;
            line_info->line_number = line_number;
            if (top_function_address + offset_top_function == address) /* we've reached our address */
                break;
        }
    }
    line_info->key = NULL;
    result = lineinfo_set_nameA(pdb->module->process, line_info, source_file_name) ? R_PDB_SUCCESS : R_PDB_OUT_OF_MEMORY;
    pdb_reader_free(pdb, source_file_name);

    return result;
}

static enum method_result pdb_method_get_line_from_inlined_address(struct module_format *modfmt, struct symt_function *inlined,
                                                                   DWORD64 address, struct lineinfo_t *line_info)
{
    struct pdb_reader *pdb;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;

    return pdb_method_result(pdb_method_get_line_from_inlined_address_internal(pdb, inlined, address, line_info));
}

static enum pdb_result pdb_reader_symbol_skip_defranges(struct pdb_reader *pdb, struct pdb_reader_walker *walker)
{
    enum pdb_result result;
    union codeview_symbol cv_symbol;

    while ((result = pdb_reader_read_partial_codeview_symbol(pdb, walker, &cv_symbol)) == R_PDB_SUCCESS)
    {
        if (cv_symbol.generic.id < S_DEFRANGE || cv_symbol.generic.id > S_DEFRANGE_REGISTER_REL)
        {
            walker->offset -= sizeof(cv_symbol.generic.len);
            break;
        }
        walker->offset += cv_symbol.generic.len;
    }
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_symbol_count_range_annotations(struct pdb_reader *pdb, struct pdb_reader_walker annotation_walker,
                                                                 unsigned *pnum_ranges)
{
    enum pdb_result result;
    unsigned num_ranges = 0, opcode, arg1, arg2;

    while ((result = pdb_reader_read_inlinesite_annotation(pdb, &annotation_walker, &opcode, &arg1, &arg2)) == R_PDB_SUCCESS)
    {
        if (opcode == BA_OP_Invalid) break;
        switch (opcode)
        {
        case BA_OP_CodeOffset:
        case BA_OP_ChangeCodeLength:
        case BA_OP_ChangeFile:
        case BA_OP_ChangeLineOffset:
            break;
        case BA_OP_ChangeCodeOffset:
        case BA_OP_ChangeCodeOffsetAndLineOffset:
        case BA_OP_ChangeCodeLengthAndCodeOffset:
            num_ranges++;
            break;
        default:
            WARN("Unsupported op %d\n", opcode);
            break;
        }
    }
    *pnum_ranges = num_ranges;
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_symbol_skip_if(struct pdb_reader *pdb, struct pdb_reader_walker *walker, unsigned id)
{
    enum pdb_result result;
    union codeview_symbol cv_symbol;

    if ((result = pdb_reader_read_partial_codeview_symbol(pdb, walker, &cv_symbol))) return result;
    if (cv_symbol.generic.id != id)
    {
        walker->offset -= sizeof(cv_symbol.generic.len);
        return R_PDB_NOT_FOUND;
    }
    walker->offset += cv_symbol.generic.len;
    return R_PDB_SUCCESS;
}

static inline void inline_site_update_last_range(struct symt_function* inlined, unsigned index, ULONG_PTR hi)
{
    if (index && index <= inlined->num_ranges)
    {
        struct addr_range* range = &inlined->ranges[index - 1];
        /* only change range if it has no span (code start without code end) */
        if (range->low == range->high)
            range->high = hi;
    }
}

static enum pdb_result pdb_reader_create_inline_site(struct pdb_reader *pdb, struct symt_function *top_func,
                                                     struct symt *container,
                                                     cv_itemid_t cv_inlinee_id,
                                                     pdbsize_t symbol_start_offset,
                                                     struct pdb_reader_walker *annotation_walker,
                                                     struct symt_function **pinlined)
{
    enum pdb_result result;
    union codeview_type *cv_type;
    struct symt_function *inlined;
    unsigned num_ranges;
    unsigned offset, index;
    symref_t type_symref;
    unsigned opcode, arg1, arg2;

    if ((result = pdb_reader_IPI_alloc_and_read_full_codeview_type(pdb, cv_inlinee_id, &cv_type)))
    {
        WARN("Couldn't find type %x in IPI stream\n", cv_inlinee_id);
        return result;
    }
    if ((result = pdb_reader_symbol_count_range_annotations(pdb, *annotation_walker, &num_ranges))) return result;
    if (!num_ranges) return R_PDB_INVALID_PDB_FILE;

    switch (cv_type->generic.id)
    {
    case LF_FUNC_ID:
        if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_type->func_id_v3.type, &type_symref))) return result;
        inlined = symt_new_inlinesite(pdb->module, top_func, container, cv_type->func_id_v3.name,
                                      type_symref, symbol_start_offset, num_ranges);
        break;
    case LF_MFUNC_ID:
        /* FIXME we just declare a function, not a method */
        if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_type->mfunc_id_v3.type, &type_symref))) return result;
        inlined = symt_new_inlinesite(pdb->module, top_func, container, cv_type->mfunc_id_v3.name,
                                      type_symref, symbol_start_offset, num_ranges);
        break;
    default:
        result = PDB_REPORT_UNEXPECTED("inlinee kind", cv_type->generic.id);
        pdb_reader_free(pdb, cv_type);
        return result;
    }
    pdb_reader_free(pdb, cv_type);

    /* rescan all annotations and store ranges & line information */
    offset = 0;
    index = 0;

    while (pdb_reader_read_inlinesite_annotation(pdb, annotation_walker, &opcode, &arg1, &arg2) == R_PDB_SUCCESS)
    {
        switch (opcode)
        {
        case BA_OP_CodeOffset:
            offset = arg1;
            break;
        case BA_OP_ChangeCodeOffset:
            offset += arg1;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            inlined->ranges[index  ].low = top_func->ranges[0].low + offset;
            inlined->ranges[index++].high = top_func->ranges[0].low + offset;
            break;
        case BA_OP_ChangeCodeLength:
            /* this op isn't widely used by MSVC, but clang uses it a lot... */
            offset += arg1;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            break;
        case BA_OP_ChangeFile:
            break;
        case BA_OP_ChangeLineOffset:
            break;
        case BA_OP_ChangeCodeOffsetAndLineOffset:
            offset += arg1;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            inlined->ranges[index  ].low = top_func->ranges[0].low + offset;
            inlined->ranges[index++].high = top_func->ranges[0].low + offset;
            break;
        case BA_OP_ChangeCodeLengthAndCodeOffset:
            offset += arg2;
            inline_site_update_last_range(inlined, index, top_func->ranges[0].low + offset);
            inlined->ranges[index  ].low = top_func->ranges[0].low + offset;
            inlined->ranges[index++].high = top_func->ranges[0].low + offset + arg1;
            break;
        default:
            WARN("Unsupported op %d\n", opcode);
            break;
        }
    }
    if (index != num_ranges) /* sanity check */
        return PDB_REPORT_UNEXPECTED("Internal logic error", 0);
    if (inlined->num_ranges)
    {
        struct addr_range* range = &inlined->ranges[inlined->num_ranges - 1];
        if (range->low == range->high) WARN("pending empty range at end of %s inside %s\n",
                                            debugstr_a(inlined->hash_elt.name),
                                            debugstr_a(top_func->hash_elt.name));
    }
    *pinlined = inlined;
    return R_PDB_SUCCESS;
}

/* likely to review */
static enum pdb_result pdb_reader_create_variable(struct pdb_reader *pdb,
                                                  struct symt_compiland *compiland,
                                                  struct symt_function* func,
                                                  struct symt_block* block,
                                                  const char* name,
                                                  struct location *loc,
                                                  cv_typ_t cv_typeid, BOOL is_local)
{
    if (name && *name)
    {
        enum pdb_result result;
        symref_t symref;

        if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_typeid, &symref))) return result;
        if (func)
        {
            if (!is_local || loc->kind == loc_tlsrel) WARN("Unsupported construct\n");
            symt_add_func_local(pdb->module, func, DataIsStaticLocal, loc, block, symref, name);
            return R_PDB_SUCCESS;
        }

        if (is_local ^ (compiland != 0)) FIXME("Unsupported construct\n");
        symt_new_global_variable(pdb->module, compiland, name, is_local, *loc, 0, symref);
    }
    return R_PDB_SUCCESS;
}

static BOOL symt_function_has_local_variable(struct symt_function* func, const char* name)
{
    int i;

    for (i = 0; i < vector_length(&func->vchildren); ++i)
    {
        struct symt* p = *(struct symt**)vector_at(&func->vchildren, i);
        if (symt_check_tag(p, SymTagData) && !strcmp(((struct symt_data*)p)->hash_elt.name, name))
            return TRUE;
    }
    return FALSE;
}

static enum pdb_result pdb_reader_load_compiland_symbols(struct pdb_reader *pdb, struct symt_compiland *compiland, struct pdb_reader_walker *walker)
{
    enum pdb_result result;
    union codeview_symbol *cv_symbol;
    struct symt_function *top_func = NULL;
    struct symt_function *curr_func = NULL;
    struct symt_block *block = NULL;
    struct location loc;
    unsigned top_frame_size = -1;
    DWORD64 address;
    symref_t type_symref;
    /*
     * Loop over the different types of records and whenever we
     * find something we are interested in, record it and move on.
     */
    while ((result = pdb_reader_alloc_and_read_full_codeview_symbol(pdb, walker, &cv_symbol)) == R_PDB_SUCCESS)
    {
        pdbsize_t symbol_start_offset = walker->offset - (sizeof(cv_symbol->generic.len) + cv_symbol->generic.len);
        if (!cv_symbol->generic.id || cv_symbol->generic.len < 2) break;
        if ((cv_symbol->generic.len + 2) & 3) WARN("unpadded len %u\n", cv_symbol->generic.len + 2);

        switch (cv_symbol->generic.id)
        {
        case S_LDATA32:
            loc.kind = loc_absolute;
            loc.reg = 0;
            if ((result = pdb_reader_get_segment_address(pdb, cv_symbol->data_v3.segment, cv_symbol->data_v3.offset, &address)))
                goto failure;
            loc.offset = address;

            if ((result = pdb_reader_create_variable(pdb, compiland, curr_func, block, cv_symbol->data_v3.name, &loc,
                                                     cv_symbol->data_v3.symtype, TRUE))) goto failure;
            break;

        /* variables with thread storage */
        case S_GTHREAD32:
        case S_LTHREAD32:
            loc.kind = loc_tlsrel;
            loc.reg = 0;
            loc.offset = cv_symbol->thread_v3.offset;

            if ((result = pdb_reader_create_variable(pdb, compiland, curr_func, block, cv_symbol->thread_v3.name,
                                                     &loc, cv_symbol->thread_v3.symtype,
                                                     cv_symbol->generic.id == S_LTHREAD32))) goto failure;
            break;

        case S_THUNK32:
            if ((result = pdb_reader_get_segment_address(pdb, cv_symbol->thunk_v3.segment, cv_symbol->thunk_v3.offset, &address))) goto failure;
            symt_new_thunk(pdb->module, compiland,
                           cv_symbol->thunk_v3.name, cv_symbol->thunk_v3.thtype,
                           address, cv_symbol->thunk_v3.thunk_len);
            break;

        /*
         * Global and static functions.
         */
        case S_GPROC32:
        case S_LPROC32:
            if (top_func) WARN("nested function\n");
            if ((result = pdb_reader_get_segment_address(pdb, cv_symbol->proc_v3.segment, cv_symbol->proc_v3.offset, &address))) goto failure;
            if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_symbol->proc_v3.proctype, &type_symref))) goto failure;
            if ((top_func = symt_new_function(pdb->module, compiland,
                                              cv_symbol->proc_v3.name,
                                              address, cv_symbol->proc_v3.proc_len,
                                              type_symref, symbol_start_offset)))
            {
                curr_func = top_func;
                loc.kind = loc_absolute;
                loc.offset = cv_symbol->proc_v3.debug_start;
                symt_add_function_point(pdb->module, curr_func, SymTagFuncDebugStart, &loc, NULL);
                loc.offset = cv_symbol->proc_v3.debug_end;
                symt_add_function_point(pdb->module, curr_func, SymTagFuncDebugEnd, &loc, NULL);
            }
            break;
        /*
         * Function parameters and stack variables.
         */
        case S_BPREL32:
            /* S_BPREL32 can be present after S_LOCAL; prefer S_LOCAL when present */
            if (symt_function_has_local_variable(curr_func, cv_symbol->stack_v3.name)) break;
            if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_symbol->stack_v3.symtype, &type_symref))) goto failure;
            loc.kind = loc_regrel;
            /* Yes, it's i386 dependent, but that's the symbol purpose. S_REGREL is used on other CPUs */
            loc.reg = CV_REG_EBP;
            loc.offset = cv_symbol->stack_v3.offset;
            symt_add_func_local(pdb->module, curr_func,
                                cv_symbol->stack_v3.offset > 0 ? DataIsParam : DataIsLocal,
                                &loc, block, type_symref, cv_symbol->stack_v3.name);
            break;
        case S_REGREL32:
            /* S_REGREL32 can be present after S_LOCAL; prefer S_LOCAL when present */
            if (symt_function_has_local_variable(curr_func, cv_symbol->regrel_v3.name)) break;
            if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_symbol->regrel_v3.symtype, &type_symref))) goto failure;
            loc.kind = loc_regrel;
            loc.reg = cv_symbol->regrel_v3.reg;
            loc.offset = cv_symbol->regrel_v3.offset;
            if (top_frame_size == -1) WARN("S_REGREL32 without frameproc\n");
            symt_add_func_local(pdb->module, curr_func,
                                cv_symbol->regrel_v3.offset >= top_frame_size ? DataIsParam : DataIsLocal,
                                &loc, block, type_symref, cv_symbol->regrel_v3.name);
            break;

        case S_REGISTER:
            /* S_REGISTER can be present after S_LOCAL; prefer S_LOCAL when present */
            if (symt_function_has_local_variable(curr_func, cv_symbol->register_v3.name)) break;
            if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_symbol->register_v3.type, &type_symref))) goto failure;
            loc.kind = loc_register;
            loc.reg = cv_symbol->register_v3.reg;
            loc.offset = 0;
            symt_add_func_local(pdb->module, curr_func,
                                DataIsLocal, &loc, block, type_symref, cv_symbol->register_v3.name);
            break;

        case S_BLOCK32:
            if ((result = pdb_reader_get_segment_address(pdb, cv_symbol->block_v3.segment, cv_symbol->block_v3.offset, &address))) goto failure;
            block = symt_open_func_block(pdb->module, curr_func, block, 1);
            block->ranges[0].low = address;
            block->ranges[0].high = block->ranges[0].low + cv_symbol->block_v3.length;
            break;

        case S_END:
            if (block)
            {
                block = symt_close_func_block(pdb->module, curr_func, block);
            }
            else if (top_func)
            {
                if (curr_func != top_func) {FIXME("shouldn't close a top function with an opened inlined function\n"); result = R_PDB_INVALID_PDB_FILE; goto failure;}
                top_func = curr_func = NULL;
                top_frame_size = -1;
            }
            break;

        case S_COMPILE2:
            TRACE("S-Compile-V3 machine:%x language:%x %s\n",
                  cv_symbol->compile2_v3.machine, cv_symbol->compile2_v3.flags.iLanguage, debugstr_a(cv_symbol->compile2_v3.name));
            break;
        case S_COMPILE3:
            TRACE("S-Compile3-V3 machine:%x language:%x %s\n",
                  cv_symbol->compile3_v3.machine, cv_symbol->compile3_v3.flags.iLanguage, debugstr_a(cv_symbol->compile3_v3.name));
            break;

        case S_ENVBLOCK:
            break;

        case S_OBJNAME:
            TRACE("S-ObjName-V3 %s\n", debugstr_a(cv_symbol->objname_v3.name));
            break;

        case S_LABEL32:
            if ((result = pdb_reader_get_segment_address(pdb, cv_symbol->label_v3.segment, cv_symbol->label_v3.offset, &address))) goto failure;
            if (curr_func)
            {
                loc.kind = loc_absolute;
                loc.offset = address - curr_func->ranges[0].low;
                symt_add_function_point(pdb->module, curr_func, SymTagLabel, &loc, cv_symbol->label_v3.name);
            }
            else symt_new_label(pdb->module, compiland, cv_symbol->label_v3.name, address);
            break;

        case S_LOCAL:
            /* FIXME: don't store global/static variables accessed through registers... we don't support that
             * in locals... anyway, global data record should be present as well (so the variable will be available
             * through the global definition, but potentially not updated)
             */
            if (!cv_symbol->local_v3.varflags.enreg_global && !cv_symbol->local_v3.varflags.enreg_static)
            {
                loc.kind = loc_cv_defrange;
                loc.reg = walker->stream_id;
                loc.offset = symbol_start_offset;

                if ((result = pdb_reader_symref_from_cv_typeid(pdb, cv_symbol->local_v3.symtype, &type_symref))) goto failure;
                symt_add_func_local(pdb->module, curr_func,
                                    cv_symbol->local_v3.varflags.is_param ? DataIsParam : DataIsLocal,
                                    &loc, block, type_symref, cv_symbol->local_v3.name);
            }
            if ((result = pdb_reader_symbol_skip_defranges(pdb, walker))) goto failure;
            break;
        case S_INLINESITE:
            {
                struct pdb_reader_walker annotation_walker;
                struct symt_function* inlined;

                annotation_walker.stream_id = walker->stream_id;
                annotation_walker.offset = symbol_start_offset + offsetof(union codeview_symbol, inline_site_v3.binaryAnnotations);
                annotation_walker.last = symbol_start_offset + sizeof(cv_symbol->generic.len) + cv_symbol->generic.len;

                if ((result = pdb_reader_create_inline_site(pdb, top_func, block ? &block->symt : &curr_func->symt,
                                                            cv_symbol->inline_site_v3.inlinee, symbol_start_offset, &annotation_walker, &inlined)))
                {
                    /* skip whole inlined block */
                    walker->offset = cv_symbol->inline_site_v3.pEnd;
                    if ((result = pdb_reader_symbol_skip_if(pdb, walker, S_INLINESITE_END))) goto failure;
                }
                else
                {
                    curr_func = inlined;
                    block = NULL;
                }
            }
            break;
        case S_INLINESITE2:
            {
                struct pdb_reader_walker annotation_walker = *walker;
                struct symt_function* inlined;

                annotation_walker.stream_id = walker->stream_id;
                annotation_walker.offset = symbol_start_offset + offsetof(union codeview_symbol, inline_site2_v3.binaryAnnotations);
                annotation_walker.last = symbol_start_offset + sizeof(cv_symbol->generic.len) + cv_symbol->generic.len;

                if ((result = pdb_reader_create_inline_site(pdb, top_func, block ? &block->symt : &curr_func->symt,
                                                            cv_symbol->inline_site2_v3.inlinee, symbol_start_offset, &annotation_walker, &inlined)))
                {
                    /* skip whole inlined block */
                    walker->offset = cv_symbol->inline_site2_v3.pEnd;
                    if ((result = pdb_reader_symbol_skip_if(pdb, walker, S_INLINESITE_END))) goto failure;
                }
                else
                {
                    curr_func = inlined;
                    block = NULL;
                }
            }
            break;

        case S_INLINESITE_END:
            block = symt_is_symref_ptr(curr_func->container) && symt_check_tag(SYMT_SYMREF_TO_PTR(curr_func->container), SymTagBlock) ?
                (struct symt_block*)curr_func->container : NULL;
            curr_func = (struct symt_function*)symt_get_upper_inlined(curr_func);
            break;

        case S_SSEARCH:
            TRACE("Start search: seg=0x%x at offset 0x%08x\n",
                  cv_symbol->ssearch_v1.segment, cv_symbol->ssearch_v1.offset);
            break;

        case S_ALIGN:
            TRACE("S-Align V1\n");
            break;
        case S_HEAPALLOCSITE:
            TRACE("S-heap site V3: offset=0x%08x at sect_idx 0x%04x, inst_len 0x%08x, index 0x%08x\n",
                  cv_symbol->heap_alloc_site_v3.offset, cv_symbol->heap_alloc_site_v3.sect_idx,
                  cv_symbol->heap_alloc_site_v3.inst_len, cv_symbol->heap_alloc_site_v3.index);
            break;

        case S_SEPCODE:
            if (!top_func)
            {
                struct symt_ht* parent;
                if ((result = pdb_reader_get_segment_address(pdb, cv_symbol->sepcode_v3.sectParent, cv_symbol->sepcode_v3.offParent, &address))) goto failure;
                parent = symt_find_symbol_at(pdb->module, address);
                if (symt_check_tag(&parent->symt, SymTagFunction))
                {
                    struct symt_function* pfunc = (struct symt_function*)parent;
                    if ((result = pdb_reader_get_segment_address(pdb, cv_symbol->sepcode_v3.sect, cv_symbol->sepcode_v3.off, &address))) goto failure;
                    top_func = symt_new_function(pdb->module, compiland, pfunc->hash_elt.name,
                                                 address, cv_symbol->sepcode_v3.length, pfunc->type, symbol_start_offset);
                    curr_func = top_func;
                }
                else
                    WARN("Couldn't find function referenced by S_SEPCODE at %04x:%08x\n",
                         cv_symbol->sepcode_v3.sectParent, cv_symbol->sepcode_v3.offParent);
            }
            else
            {
                FIXME("S_SEPCODE inside top-level function %s\n", debugstr_a(top_func->hash_elt.name));
                result = R_PDB_INVALID_PDB_FILE;
                goto failure;
            }
            break;

        case S_FRAMEPROC:
            /* expecting only S_FRAMEPROC once for top level functions */
            if (top_frame_size == -1 && curr_func && curr_func == top_func)
                top_frame_size = cv_symbol->frame_info_v2.sz_frame;
            else
            {
                FIXME("Unexpected S_FRAMEPROC %d (%p %p) %x\n", top_frame_size, top_func, curr_func, walker->offset);
                result = R_PDB_INVALID_PDB_FILE;
                goto failure;
            }
            break;

        case S_GMANPROC:
        case S_LMANPROC:
            walker->offset = cv_symbol->managed_proc_v3.pend;
            if ((result = pdb_reader_symbol_skip_if(pdb, walker, S_END))) goto failure;
            break;

        /* symbols only expected in globals' DBI stream */
        case S_PUB32:
        case S_PROCREF:
        case S_LPROCREF:
        case S_TOKENREF:
        case S_GDATA32:
        case S_UDT:
            PDB_REPORT_UNEXPECTED("(compiland stream) symbol id", cv_symbol->generic.id);
            break;

        /* the symbols we can safely ignore for now */
        case S_SKIP:
        case S_TRAMPOLINE:
        case S_FRAMECOOKIE:
        case S_SECTION:
        case S_COFFGROUP:
        case S_EXPORT:
        case S_CALLSITEINFO:
        case S_ARMSWITCHTABLE:
        case S_CONSTANT:
        case S_MANSLOT:
        case S_OEM:
            /* even if S_LOCAL groks all the S_DEFRANGE* records following itself,
             * those kinds of records can also be present after a S_FILESTATIC record
             * so silence them until (at least) S_FILESTATIC is supported
             */
        case S_DEFRANGE_REGISTER:
        case S_DEFRANGE_FRAMEPOINTER_REL:
        case S_DEFRANGE_SUBFIELD_REGISTER:
        case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
        case S_DEFRANGE_REGISTER_REL:
        case S_BUILDINFO:
        case S_FILESTATIC:
        case S_CALLEES:
        case S_CALLERS:
        case S_UNAMESPACE:
        case S_INLINEES:
        case S_POGODATA:
            TRACE("Unsupported symbol id %x\n", cv_symbol->generic.id);
            break;

        default:
            PDB_REPORT_UNEXPECTED("symbol id", cv_symbol->generic.id);
            break;
        }
        pdb_reader_free(pdb, cv_symbol);
    }
    return R_PDB_SUCCESS;

failure:
    pdb_reader_free(pdb, cv_symbol);
    return result;
}

static enum pdb_result pdb_reader_ensure_symbols_loaded_from_compiland(struct pdb_reader *pdb, unsigned compiland_index)
{
    enum pdb_result result;
    struct pdb_compiland *compiland = &pdb->compilands[compiland_index];

    if (compiland_index >= pdb->num_compilands) return R_PDB_INVALID_ARGUMENT;
    if (!compiland->are_symbols_loaded)
    {
        struct pdb_reader_walker walker;
        char *obj_name;

        if ((result = pdb_reader_walker_init(pdb, PDB_STREAM_DBI, &walker))) return result;
        walker.offset = compiland->stream_offset;
        if ((result = pdb_reader_alloc_and_fetch_string(pdb, &walker, &obj_name))) return result;
        compiland->compiland = symt_new_compiland(pdb->module, obj_name);
        pdb_reader_free(pdb, obj_name);

        if ((result = pdb_reader_walker_init(pdb, compiland->stream_id, &walker))) return result;
        walker.offset = sizeof(UINT32);
        if ((result = pdb_reader_load_compiland_symbols(pdb, (struct symt_compiland *)compiland->compiland, &walker))) return result;
        compiland->are_symbols_loaded = TRUE;
    }
    return R_PDB_SUCCESS;
}

static enum pdb_result pdb_reader_lookup_top_symbol_by_segment_offset(struct pdb_reader *pdb, unsigned segment, unsigned offset, symref_t *symref)
{
    enum pdb_result result;
    unsigned compiland_index;
    DWORD64 in_address;
    struct symt_ht *symbol;

    if ((result = pdb_reader_get_segment_address(pdb, segment, offset, &in_address))) return result;
    result = pdb_reader_lookup_compiland_by_segment_offset(pdb, segment, offset, &compiland_index);
    if (result == R_PDB_SUCCESS)
    {
        if ((result = pdb_reader_ensure_symbols_loaded_from_compiland(pdb, compiland_index)))
            return result;
    }
    /* don't fail if not found as some symbols are only present in DBI, but not in compiland */
    else if (result != R_PDB_NOT_FOUND) return result;
    /* fallback to ptr symbols lookup (as ptr should be loaded by now) */
    symbol = symt_find_symbol_at(pdb->module, in_address);
    if (!symbol) return R_PDB_NOT_FOUND;
    *symref = symt_ptr_to_symref(&symbol->symt);
    return R_PDB_SUCCESS;
}

static enum method_result pdb_method_lookup_symbol_by_address(struct module_format *modfmt, DWORD_PTR address, symref_t *symref)
{
    enum pdb_result result;
    struct pdb_reader *pdb;
    unsigned segment, offset;

    if (!pdb_hack_get_main_info(modfmt, &pdb, NULL)) return MR_FAILURE;
    if ((result = pdb_reader_get_segment_offset_from_address(pdb, address, &segment, &offset))) return MR_FAILURE;
    return pdb_method_result(pdb_reader_lookup_top_symbol_by_segment_offset(pdb, segment, offset, symref));
}

static struct module_format_vtable pdb_module_format_vtable =
{
    NULL,/*pdb_module_remove*/
    pdb_method_request_symref_t,
    pdb_method_lookup_symbol_by_address,
    pdb_method_find_type,
    pdb_method_enumerate_types,
    pdb_method_location_compute,
    pdb_method_get_line_from_address,
    pdb_method_advance_line_info,
    pdb_method_enumerate_lines,
    pdb_method_get_line_from_inlined_address,
    pdb_method_enumerate_sources,
};

struct pdb_reader *pdb_hack_reader_init(struct module *module, HANDLE file, const IMAGE_SECTION_HEADER *sections, unsigned num_sections)
{
    struct pdb_reader *pdb = pool_alloc(&module->pool, sizeof(*pdb) + num_sections * sizeof(*sections));
    if (pdb)
    {
        IMAGE_SECTION_HEADER *new_sections = (void*)(pdb + 1);
        memcpy(new_sections, sections, num_sections * sizeof(*sections));
        if (pdb_reader_init(pdb, module, file, new_sections, num_sections) == R_PDB_SUCCESS)
        {
            /* hack (copy old pdb methods until they are moved here) */
            pdb_module_format_vtable.remove = module->format_info[DFI_PDB]->vtable->remove;

            module->format_info[DFI_PDB]->vtable = &pdb_module_format_vtable;
            return pdb;
        }
    }
    pool_free(&module->pool, pdb);
    return NULL;
}

/*========================================================================
 * FPO unwinding code
 */

/* Stack unwinding is based on postfixed operations.
 * Let's define our Postfix EValuator
 */
#define PEV_MAX_LEN      32
struct pevaluator
{
    struct cpu_stack_walk*  csw;
    struct pool             pool;
    struct vector           stack;
    unsigned                stk_index;
    struct hash_table       values;
    char                    error[64];
};

struct zvalue
{
    DWORD_PTR                   value;
    struct hash_table_elt       elt;
};

static void pev_set_error(struct pevaluator* pev, const char* msg, ...) __WINE_PRINTF_ATTR(2,3);
static void pev_set_error(struct pevaluator* pev, const char* msg, ...)
{
    va_list args;

    va_start(args, msg);
    vsnprintf(pev->error, sizeof(pev->error), msg, args);
    va_end(args);
}

#if 0
static void pev_dump_stack(struct pevaluator* pev)
{
    unsigned i;
    struct hash_table_iter      hti;

    FIXME("stack #%d\n", pev->stk_index);
    for (i = 0; i < pev->stk_index; i++)
    {
        FIXME("\t%d) %s\n", i, *(char**)vector_at(&pev->stack, i));
    }
    hash_table_iter_init(&pev->values, &hti, str);
    FIXME("hash\n");
    while ((ptr = hash_table_iter_up(&hti)))
    {
        struct zvalue* zval = CONTAINING_RECORD(ptr, struct zvalue, elt);
        FIXME("\t%s: Ix\n", zval->elt.name, zval->value);
    }

}
#endif

/* get the value out of an operand (variable or literal) */
static BOOL  pev_get_val(struct pevaluator* pev, const char* str, DWORD_PTR* val)
{
    char*                       n;
    struct hash_table_iter      hti;
    void*                       ptr;

    switch (str[0])
    {
    case '$':
    case '.':
        hash_table_iter_init(&pev->values, &hti, str);
        while ((ptr = hash_table_iter_up(&hti)))
        {
            if (!strcmp(CONTAINING_RECORD(ptr, struct zvalue, elt)->elt.name, str))
            {
                *val = CONTAINING_RECORD(ptr, struct zvalue, elt)->value;
                return TRUE;
            }
        }
        pev_set_error(pev, "get_zvalue: no value found (%s)", str);
        return FALSE;
    default:
        *val = strtol(str, &n, 10);
        if (n != str && *n == '\0') return TRUE;
        pev_set_error(pev, "get_val: not a literal (%s)", str);
        return FALSE;
    }
}

/* push an operand onto the stack */
static BOOL  pev_push(struct pevaluator* pev, const char* elt)
{
    char**      at;
    if (pev->stk_index < vector_length(&pev->stack))
        at = vector_at(&pev->stack, pev->stk_index);
    else
        at = vector_add(&pev->stack, &pev->pool);
    if (!at)
    {
        pev_set_error(pev, "push: out of memory");
        return FALSE;
    }
    *at = pool_strdup(&pev->pool, elt);
    pev->stk_index++;
    return TRUE;
}

/* pop an operand from the stack */
static BOOL  pev_pop(struct pevaluator* pev, char* elt)
{
    char**      at = vector_at(&pev->stack, --pev->stk_index);
    if (!at)
    {
        pev_set_error(pev, "pop: stack empty");
        return FALSE;
    }
    strcpy(elt, *at);
    return TRUE;
}

/* pop an operand from the stack, and gets its value */
static BOOL  pev_pop_val(struct pevaluator* pev, DWORD_PTR* val)
{
    char        p[PEV_MAX_LEN];

    return pev_pop(pev, p) && pev_get_val(pev, p, val);
}

/* set var 'name' a new value (creates the var if it doesn't exist) */
static BOOL  pev_set_value(struct pevaluator* pev, const char* name, DWORD_PTR val)
{
    struct hash_table_iter      hti;
    void*                       ptr;

    hash_table_iter_init(&pev->values, &hti, name);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        if (!strcmp(CONTAINING_RECORD(ptr, struct zvalue, elt)->elt.name, name))
        {
            CONTAINING_RECORD(ptr, struct zvalue, elt)->value = val;
            break;
        }
    }
    if (!ptr)
    {
        struct zvalue* zv = pool_alloc(&pev->pool, sizeof(*zv));
        if (!zv)
        {
            pev_set_error(pev, "set_value: out of memory");
            return FALSE;
        }
        zv->value = val;

        zv->elt.name = pool_strdup(&pev->pool, name);
        hash_table_add(&pev->values, &zv->elt);
    }
    return TRUE;
}

/* execute a binary operand from the two top most values on the stack.
 * puts result on top of the stack */
static BOOL  pev_binop(struct pevaluator* pev, char op)
{
    char        res[PEV_MAX_LEN];
    DWORD_PTR   v1, v2, c;

    if (!pev_pop_val(pev, &v2) || !pev_pop_val(pev, &v1)) return FALSE;
    if ((op == '/' || op == '%') && v2 == 0)
    {
        pev_set_error(pev, "binop: division by zero");
        return FALSE;
    }
    switch (op)
    {
    case '+': c = v1 + v2; break;
    case '-': c = v1 - v2; break;
    case '*': c = v1 * v2; break;
    case '/': c = v1 / v2; break;
    case '%': c = v1 % v2; break;
    default:
        pev_set_error(pev, "binop: unknown op (%c)", op);
        return FALSE;
    }
    snprintf(res, sizeof(res), "%Id", c);
    pev_push(pev, res);
    return TRUE;
}

/* pops top most operand, dereference it, on pushes the result on top of the stack */
static BOOL  pev_deref(struct pevaluator* pev)
{
    char        res[PEV_MAX_LEN];
    DWORD_PTR   v1, v2 = 0;

    if (!pev_pop_val(pev, &v1)) return FALSE;
    if (!sw_read_mem(pev->csw, v1, &v2, pev->csw->cpu->word_size))
    {
        pev_set_error(pev, "deref: cannot read mem at %Ix", v1);
        return FALSE;
    }
    snprintf(res, sizeof(res), "%Id", v2);
    pev_push(pev, res);
    return TRUE;
}

/* assign value to variable (from two top most operands) */
static BOOL  pev_assign(struct pevaluator* pev)
{
    char                p2[PEV_MAX_LEN];
    DWORD_PTR           v1;

    if (!pev_pop_val(pev, &v1) || !pev_pop(pev, p2)) return FALSE;
    if (p2[0] != '$')
    {
        pev_set_error(pev, "assign: %s isn't a variable", p2);
        return FALSE;
    }
    pev_set_value(pev, p2, v1);

    return TRUE;
}

/* initializes the postfix evaluator */
static void  pev_init(struct pevaluator* pev, struct cpu_stack_walk* csw,
                      const PDB_FPO_DATA* fpoext, struct pdb_cmd_pair* cpair)
{
    pev->csw = csw;
    pool_init(&pev->pool, 512);
    vector_init(&pev->stack, sizeof(char*), 0);
    pev->stk_index = 0;
    hash_table_init(&pev->pool, &pev->values, 8);
    pev->error[0] = '\0';
    for (; cpair->name; cpair++)
        pev_set_value(pev, cpair->name, *cpair->pvalue);
    pev_set_value(pev, ".raSearchStart", fpoext->start);
    pev_set_value(pev, ".cbLocals",      fpoext->locals_size);
    pev_set_value(pev, ".cbParams",      fpoext->params_size);
    pev_set_value(pev, ".cbSavedRegs",   fpoext->savedregs_size);
}

static BOOL  pev_free(struct pevaluator* pev, struct pdb_cmd_pair* cpair)
{
    DWORD_PTR   val;

    if (cpair) for (; cpair->name; cpair++)
    {
        if (pev_get_val(pev, cpair->name, &val))
            *cpair->pvalue = val;
    }
    pool_destroy(&pev->pool);
    return TRUE;
}

BOOL  pdb_fpo_unwind_parse_cmd_string(struct cpu_stack_walk* csw, PDB_FPO_DATA* fpoext,
                                      const char* cmd, struct pdb_cmd_pair* cpair)
{
    char                token[PEV_MAX_LEN];
    char*               ptok = token;
    const char*         ptr;
    BOOL                over = FALSE;
    struct pevaluator   pev;

    if (!cmd) return FALSE;
    pev_init(&pev, csw, fpoext, cpair);
    for (ptr = cmd; !over; ptr++)
    {
        if (*ptr == ' ' || (over = *ptr == '\0'))
        {
            *ptok = '\0';

            if (!strcmp(token, "+") || !strcmp(token, "-") || !strcmp(token, "*") ||
                !strcmp(token, "/") || !strcmp(token, "%"))
            {
                if (!pev_binop(&pev, token[0])) goto done;
            }
            else if (!strcmp(token, "^"))
            {
                if (!pev_deref(&pev)) goto done;
            }
            else if (!strcmp(token, "="))
            {
                if (!pev_assign(&pev)) goto done;
            }
            else
            {
                if (!pev_push(&pev, token)) goto done;
            }
            ptok = token;
        }
        else
        {
            if (ptok - token >= PEV_MAX_LEN - 1)
            {
                pev_set_error(&pev, "parse: token too long (%s)", ptr - (ptok - token));
                goto done;
            }
            *ptok++ = *ptr;
        }
    }
    pev_free(&pev, cpair);
    return TRUE;
done:
    FIXME("Couldn't evaluate %s => %s\n", debugstr_a(cmd), pev.error);
    pev_free(&pev, NULL);
    return FALSE;
}

BOOL pdb_virtual_unwind(struct cpu_stack_walk *csw, DWORD_PTR ip,
                        union ctx *context, struct pdb_cmd_pair *cpair)
{
    struct pdb_reader          *pdb;
    struct pdb_reader_walker    walker;
    struct module_pair          pair;
    unsigned                    fpoext_stream;
    PDB_FPO_DATA                fpoext;
    BOOL                        ret = FALSE;

    if (!module_init_pair(&pair, csw->hProcess, ip)) return FALSE;
    if (!pdb_hack_get_main_info(pair.effective->format_info[DFI_PDB], &pdb, &fpoext_stream)) return FALSE;

    if (!pdb)
        return pdb_old_virtual_unwind(csw, ip, context, cpair);
    TRACE("searching %Ix => %Ix\n", ip, ip - (DWORD_PTR)pair.effective->module.BaseOfImage);
    ip -= (DWORD_PTR)pair.effective->module.BaseOfImage;

    if (!pdb_reader_walker_init(pdb, fpoext_stream, &walker) &&
        (walker.last % sizeof(fpoext)) == 0)
    {
        /* FIXME likely a binary search should be more appropriate here */
        while (pdb_reader_READ(pdb, &walker, &fpoext) == R_PDB_SUCCESS)
        {
            if (fpoext.start <= ip && ip < fpoext.start + fpoext.func_size)
            {
                char *cmd;

                if (pdb_reader_alloc_and_fetch_global_string(pdb, fpoext.str_offset, &cmd)) break;
                TRACE("\t%08x %08x %8x %8x %4x %4x %4x %08x %s\n",
                      fpoext.start, fpoext.func_size, fpoext.locals_size,
                      fpoext.params_size, fpoext.maxstack_size, fpoext.prolog_size,
                      fpoext.savedregs_size, fpoext.flags,
                      debugstr_a(cmd));

                ret = pdb_fpo_unwind_parse_cmd_string(csw, &fpoext, cmd, cpair);
                pdb_reader_free(pdb, cmd);
                break;
            }
        }
    }

    return ret;
}
