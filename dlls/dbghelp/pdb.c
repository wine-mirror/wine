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
#include "winternl.h"

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
typedef enum pdb_result (*pdb_reader_fetch_t)(struct pdb_reader *pdb, void *buffer, pdboff_t offset, pdbsize_t size);

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

    pdb_reader_fetch_t fetch;
};

enum pdb_result
{
    R_PDB_SUCCESS,
    R_PDB_IOERROR,
    R_PDB_OUT_OF_MEMORY,
    R_PDB_INVALID_ARGUMENT,
    R_PDB_INVALID_PDB_FILE,
    R_PDB_BUFFER_TOO_SMALL,
};

static enum pdb_result pdb_reader_fetch_file(struct pdb_reader *pdb, void *buffer, pdboff_t offset, pdbsize_t size)
{
    OVERLAPPED ov = {.Offset = offset, .OffsetHigh = offset >> 32, .hEvent = (HANDLE)(DWORD_PTR)1};
    DWORD num_read;

    return ReadFile(pdb->file, buffer, size, &num_read, &ov) && num_read == size ? R_PDB_SUCCESS : R_PDB_IOERROR;
}

static const char       PDB_JG_IDENT[] = "Microsoft C/C++ program database 2.00\r\n\032JG\0";
static const char       PDB_DS_IDENT[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0";

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

    while (size)
    {
        toread = min(pdb->block_size - delta, size);

        if ((result = (*pdb->fetch)(pdb, buffer, (pdboff_t)*blocks * pdb->block_size + delta, toread)))
            return result;
        size -= toread;
        blocks++;
        buffer = (char*)buffer + toread;
        delta = 0;
    }

    if (num_read) *num_read = initial_size - size;
    return size != initial_size ? R_PDB_SUCCESS : R_PDB_INVALID_ARGUMENT;
}

struct pdb_reader_walker
{
    unsigned stream_id;
    pdbsize_t offset;
    pdbsize_t last;
};

static inline enum pdb_result pdb_reader_walker_init(struct pdb_reader *pdb, unsigned stream_id, struct pdb_reader_walker *walker)
{
    walker->stream_id = stream_id;
    walker->offset = 0;
    return pdb_reader_get_stream_size(pdb, stream_id, &walker->last);
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

static enum pdb_result pdb_reader_init(struct pdb_reader *pdb, struct module *module, HANDLE file)
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
    pdb->fetch = &pdb_reader_fetch_file;

    if ((result = (*pdb->fetch)(pdb, &hdr, 0, sizeof(hdr)))) return result;
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
    toc_blocks_size = pdb_reader_num_blocks(pdb, hdr.toc_size) * sizeof(uint32_t);
    if ((result = pdb_reader_alloc(pdb, toc_blocks_size, (void**)&toc_blocks)) ||
        (result = (*pdb->fetch)(pdb, toc_blocks, (pdboff_t)hdr.toc_block * hdr.block_size, toc_blocks_size)) ||
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
    pool_destroy(&pdb->pool);
}

static enum pdb_result pdb_reader_internal_read_advance(struct pdb_reader *pdb, struct pdb_reader_walker *walker,
                                                        void *buffer, pdbsize_t size)
{
    pdbsize_t num_read;
    enum pdb_result result = pdb_reader_read_from_stream(pdb, walker, buffer, size, &num_read);
    if (result) return result;
    if (num_read != size) return R_PDB_INVALID_ARGUMENT;
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
        return R_PDB_INVALID_ARGUMENT;
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
    return R_PDB_INVALID_ARGUMENT;
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

struct pdb_reader *pdb_hack_reader_init(struct module *module, HANDLE file)
{
    struct pdb_reader *pdb = pool_alloc(&module->pool, sizeof(*pdb));
    if (pdb && pdb_reader_init(pdb, module, file) == R_PDB_SUCCESS) return pdb;
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
