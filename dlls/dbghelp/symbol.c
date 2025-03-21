/*
 * File symbol.c - management of symbols (lexical tree)
 *
 * Copyright (C) 1993, Eric Youngdale.
 *               2004, Eric Pouech
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include "wine/debug.h"
#include "dbghelp_private.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);
WINE_DECLARE_DEBUG_CHANNEL(dbghelp_symt);

extern char * CDECL __unDName(char *buffer, const char *mangled, int len,
        void * (CDECL *pfn_alloc)(size_t), void (CDECL *pfn_free)(void *), unsigned short flags);

static inline int cmp_addr(ULONG64 a1, ULONG64 a2)
{
    if (a1 > a2) return 1;
    if (a1 < a2) return -1;
    return 0;
}

static inline int cmp_sorttab_addr(struct module* module, int idx, ULONG64 addr)
{
    ULONG64     ref;
    symt_get_address(&module->addr_sorttab[idx]->symt, &ref);
    return cmp_addr(ref, addr);
}

int __cdecl symt_cmp_addr(const void* p1, const void* p2)
{
    const struct symt*  sym1 = *(const struct symt* const *)p1;
    const struct symt*  sym2 = *(const struct symt* const *)p2;
    ULONG64     a1, a2;

    symt_get_address(sym1, &a1);
    symt_get_address(sym2, &a2);
    return cmp_addr(a1, a2);
}

#define BASE_CUSTOM_SYMT 0x80000000

/* dbghelp exposes the internal symbols/types with DWORD indexes.
 * - custom symbols are always stored with index starting at BASE_CUSTOM_SYMT
 * - for all the other (non custom) symbols:
 *   + on 32-bit machines, index is set to the actual address of symt
 *   + on 64-bit machines, the symt address is stored in a dedicated array
 *     which is exposed to the caller and index is the index of the symbol in
 *     this array
 */
DWORD             symt_ptr2index(struct module* module, const struct symt* sym)
{
    struct vector* vector;
    DWORD offset;
    const struct symt** c;
    int len, i;

    if (!sym) return 0;
    if (sym->tag == SymTagCustom)
    {
        vector = &module->vcustom_symt;
        offset = BASE_CUSTOM_SYMT;
    }
    else
    {
#ifdef _WIN64
        vector = &module->vsymt;
        offset = 1;
#else
        return (DWORD)sym;
#endif
    }
    len = vector_length(vector);
    /* FIXME: this is inefficient */
    for (i = 0; i < len; i++)
    {
        if (*(struct symt**)vector_at(vector, i) == sym)
            return i + offset;
    }
    /* not found */
    c = vector_add(vector, &module->pool);
    if (c) *c = sym;
    return len + offset;
}

struct symt*      symt_index2ptr(struct module* module, DWORD id)
{
    struct vector* vector;
    if (id >= BASE_CUSTOM_SYMT)
    {
        id -= BASE_CUSTOM_SYMT;
        vector = &module->vcustom_symt;
    }
    else
    {
#ifdef _WIN64
        if (!id--) return NULL;
        vector = &module->vsymt;
#else
        return (struct symt*)id;
#endif
    }
    return (id >= vector_length(vector)) ? NULL : *(struct symt**)vector_at(vector, id);
}

static BOOL symt_grow_sorttab(struct module* module, unsigned sz)
{
    struct symt_ht**    new;
    unsigned int size;

    if (sz <= module->sorttab_size) return TRUE;
    if (module->addr_sorttab)
    {
        size = module->sorttab_size * 2;
        new = HeapReAlloc(GetProcessHeap(), 0, module->addr_sorttab,
                          size * sizeof(struct symt_ht*));
    }
    else
    {
        size = 64;
        new = HeapAlloc(GetProcessHeap(), 0, size * sizeof(struct symt_ht*));
    }
    if (!new) return FALSE;
    module->sorttab_size = size;
    module->addr_sorttab = new;
    return TRUE;
}

static void symt_add_module_addr(struct module* module, struct symt_ht* ht)
{
    ULONG64             addr;

    /* Don't store in sorttab a symbol without address, they are of
     * no use here (e.g. constant values)
     */
    if (symt_get_address(&ht->symt, &addr) &&
        symt_grow_sorttab(module, module->num_symbols + 1))
    {
        module->addr_sorttab[module->num_symbols++] = ht;
        module->sortlist_valid = FALSE;
    }
}

static void symt_add_module_ht(struct module* module, struct symt_ht* ht)
{
    hash_table_add(&module->ht_symbols, &ht->hash_elt);
    symt_add_module_addr(module, ht);
}

static WCHAR* file_regex(const char* srcfile)
{
    WCHAR* mask;
    WCHAR* p;

    if (!srcfile || !*srcfile)
    {
        if (!(p = mask = HeapAlloc(GetProcessHeap(), 0, 3 * sizeof(WCHAR)))) return NULL;
        *p++ = '?';
        *p++ = '#';
    }
    else
    {
        DWORD  sz = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
        WCHAR* srcfileW;

        /* FIXME: we use here the largest conversion for every char... could be optimized */
        p = mask = HeapAlloc(GetProcessHeap(), 0, (5 * strlen(srcfile) + 1 + sz) * sizeof(WCHAR));
        if (!mask) return NULL;
        srcfileW = mask + 5 * strlen(srcfile) + 1;
        MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, sz);

        while (*srcfileW)
        {
            switch (*srcfileW)
            {
            case '\\':
            case '/':
                *p++ = '[';
                *p++ = '\\';
                *p++ = '\\';
                *p++ = '/';
                *p++ = ']';
                break;
            case '.':
                *p++ = '?';
                break;
            default:
                *p++ = *srcfileW;
                break;
            }
            srcfileW++;
        }
    }
    *p = 0;
    return mask;
}

struct symt_module* symt_new_module(struct module* module)
{
    struct symt_module*    sym;

    TRACE_(dbghelp_symt)("Adding toplevel exe symbol %s\n", debugstr_w(module->modulename));
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagExe;
        sym->module   = module;
        vector_init(&sym->vchildren, sizeof(struct symt*), 0);
    }
    return sym;
}

struct symt_compiland* symt_new_compiland(struct module* module, unsigned src_idx)
{
    struct symt_compiland*    sym;
    struct symt_compiland**   p;

    TRACE_(dbghelp_symt)("Adding compiland symbol %s:%s\n",
                         debugstr_w(module->modulename), debugstr_a(source_get(module, src_idx)));
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag  = SymTagCompiland;
        sym->container = module->top;
        sym->address   = 0;
        sym->source    = src_idx;
        vector_init(&sym->vchildren, sizeof(struct symt*), 0);
        sym->user      = NULL;
        p = vector_add(&module->top->vchildren, &module->pool);
        *p = sym;
    }
    return sym;
}

struct symt_public* symt_new_public(struct module* module, 
                                    struct symt_compiland* compiland,
                                    const char* name,
                                    BOOL is_function,
                                    ULONG_PTR address, unsigned size)
{
    struct symt_public* sym;
    struct symt**       p;

    TRACE_(dbghelp_symt)("Adding public symbol %s:%s @%Ix\n",
                         debugstr_w(module->modulename), debugstr_a(name), address);
    if ((dbghelp_options & SYMOPT_AUTO_PUBLICS) &&
        symt_find_nearest(module, address) != NULL)
        return NULL;
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagPublicSymbol;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->is_function   = is_function;
        sym->address       = address;
        sym->size          = size;
        symt_add_module_ht(module, (struct symt_ht*)sym);
        if (compiland)
        {
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_data* symt_new_global_variable(struct module* module, 
                                           struct symt_compiland* compiland, 
                                           const char* name, unsigned is_static,
                                           struct location loc, ULONG_PTR size,
                                           struct symt* type)
{
    struct symt_data*   sym;
    struct symt**       p;
    DWORD64             tsz;

    TRACE_(dbghelp_symt)("Adding global symbol %s:%s %d@%Ix %p\n",
                         debugstr_w(module->modulename), debugstr_a(name), loc.kind, loc.offset, type);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagData;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->kind          = is_static ? DataIsFileStatic : DataIsGlobal;
        sym->container     = compiland ? &compiland->symt : &module->top->symt;
        sym->type          = type;
        sym->u.var         = loc;
        if (type && size && symt_get_info(module, type, TI_GET_LENGTH, &tsz))
        {
            if (tsz != size)
                FIXME("Size mismatch for %s.%s between type (%I64u) and src (%Iu)\n",
                      debugstr_w(module->modulename), debugstr_a(name), tsz, size);
        }
        symt_add_module_ht(module, (struct symt_ht*)sym);
        p = vector_add(compiland ? &compiland->vchildren : &module->top->vchildren, &module->pool);
        *p = &sym->symt;
    }
    return sym;
}

static struct symt_function* init_function_or_inlinesite(struct module* module,
                                                         DWORD tag,
                                                         struct symt* container,
                                                         const char* name,
                                                         struct symt* sig_type,
                                                         unsigned num_ranges)
{
    struct symt_function* sym;

    assert(!sig_type || sig_type->tag == SymTagFunctionType);
    if ((sym = pool_alloc(&module->pool, offsetof(struct symt_function, ranges[num_ranges]))))
    {
        sym->symt.tag  = tag;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->container = container;
        sym->type      = sig_type;
        vector_init(&sym->vlines,  sizeof(struct line_info), 0);
        vector_init(&sym->vchildren, sizeof(struct symt*), 0);
        sym->num_ranges = num_ranges;
    }
    return sym;
}

struct symt_function* symt_new_function(struct module* module,
                                        struct symt_compiland* compiland,
                                        const char* name,
                                        ULONG_PTR addr, ULONG_PTR size,
                                        struct symt* sig_type)
{
    struct symt_function* sym;

    TRACE_(dbghelp_symt)("Adding global function %s:%s @%Ix-%Ix\n",
                         debugstr_w(module->modulename), debugstr_a(name), addr, addr + size - 1);
    if ((sym = init_function_or_inlinesite(module, SymTagFunction, &compiland->symt, name, sig_type, 1)))
    {
        struct symt** p;
        sym->ranges[0].low = addr;
        sym->ranges[0].high = addr + size;
        sym->next_inlinesite = NULL; /* first of list */
        symt_add_module_ht(module, (struct symt_ht*)sym);
        if (compiland)
        {
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_function* symt_new_inlinesite(struct module* module,
                                          struct symt_function* func,
                                          struct symt* container,
                                          const char* name,
                                          struct symt* sig_type,
                                          unsigned num_ranges)
{
    struct symt_function* sym;

    TRACE_(dbghelp_symt)("Adding inline site %s\n", debugstr_a(name));
    if ((sym = init_function_or_inlinesite(module, SymTagInlineSite, container, name, sig_type, num_ranges)))
    {
        struct symt** p;
        assert(container);

        /* chain inline sites */
        sym->next_inlinesite = func->next_inlinesite;
        func->next_inlinesite = sym;
        if (container->tag == SymTagFunction || container->tag == SymTagInlineSite)
            p = vector_add(&((struct symt_function*)container)->vchildren, &module->pool);
        else
        {
            assert(container->tag == SymTagBlock);
            p = vector_add(&((struct symt_block*)container)->vchildren, &module->pool);
        }
        *p = &sym->symt;
    }
    return sym;
}

void symt_add_func_line(struct module* module, struct symt_function* func,
                        unsigned source_idx, int line_num, ULONG_PTR addr)
{
    struct line_info*   dli;
    unsigned            vlen;
    struct line_info*   prev;
    BOOL                last_matches = FALSE;
    int                 i;

    if (func == NULL || !(dbghelp_options & SYMOPT_LOAD_LINES)) return;

    TRACE_(dbghelp_symt)("(%p)%s:%Ix %s:%u\n",
                         func, debugstr_a(func->hash_elt.name), addr,
                         debugstr_a(source_get(module, source_idx)), line_num);

    assert(func->symt.tag == SymTagFunction || func->symt.tag == SymTagInlineSite);

    for (i=vector_length(&func->vlines)-1; i>=0; i--)
    {
        dli = vector_at(&func->vlines, i);
        if (dli->is_source_file)
        {
            last_matches = (source_idx == dli->u.source_file);
            break;
        }
    }
    vlen = vector_length(&func->vlines);
    prev = vlen ? vector_at(&func->vlines, vlen - 1) : NULL;
    if (last_matches && prev && addr == prev->u.address)
    {
        WARN("Duplicate addition of line number in %s\n", debugstr_a(func->hash_elt.name));
        return;
    }
    if (!last_matches)
    {
        /* we shouldn't have line changes on first line of function */
        dli = vector_add(&func->vlines, &module->pool);
        dli->is_source_file = 1;
        dli->is_first       = (prev == NULL);
        dli->is_last        = 0;
        dli->line_number    = 0;
        dli->u.source_file  = source_idx;
    }
    /* clear previous last */
    if (prev) prev->is_last = 0;
    dli = vector_add(&func->vlines, &module->pool);
    dli->is_source_file = 0;
    dli->is_first       = 0; /* only a source file can be first */
    dli->is_last        = 1;
    dli->line_number    = line_num;
    dli->u.address      = addr;
}

/******************************************************************
 *             symt_add_func_local
 *
 * Adds a new local/parameter to a given function:
 * In any cases, dt tells whether it's a local variable or a parameter
 * or a static variable inside the function.
 * If regno it's not 0:
 *      - then variable is stored in a register
 *      - otherwise, value is referenced by register + offset
 * Otherwise, the variable is stored on the stack:
 *      - offset is then the offset from the frame register
 */
struct symt_data* symt_add_func_local(struct module* module, 
                                      struct symt_function* func, 
                                      enum DataKind dt,
                                      const struct location* loc,
                                      struct symt_block* block, 
                                      struct symt* type, const char* name)
{
    struct symt_data*   locsym;
    struct symt**       p;

    TRACE_(dbghelp_symt)("Adding local symbol (%s:%s): %s %p\n",
                         debugstr_w(module->modulename), debugstr_a(func->hash_elt.name),
                         debugstr_a(name), type);

    assert(symt_check_tag(&func->symt, SymTagFunction) || symt_check_tag(&func->symt, SymTagInlineSite));
    assert(dt == DataIsParam || dt == DataIsLocal || dt == DataIsStaticLocal);

    locsym = pool_alloc(&module->pool, sizeof(*locsym));
    locsym->symt.tag      = SymTagData;
    locsym->hash_elt.name = pool_strdup(&module->pool, name);
    locsym->hash_elt.next = NULL;
    locsym->kind          = dt;
    locsym->container     = block ? &block->symt : &func->symt;
    locsym->type          = type;
    locsym->u.var         = *loc;
    if (block)
        p = vector_add(&block->vchildren, &module->pool);
    else
        p = vector_add(&func->vchildren, &module->pool);
    *p = &locsym->symt;
    if (dt == DataIsStaticLocal)
        symt_add_module_addr(module, (struct symt_ht*)locsym);
    return locsym;
}

/******************************************************************
 *             symt_add_func_local
 *
 * Adds a new (local) constant to a given function
 */
struct symt_data* symt_add_func_constant(struct module* module,
                                         struct symt_function* func,
                                         struct symt_block* block,
                                         struct symt* type, const char* name,
                                         VARIANT* v)
{
    struct symt_data*   locsym;
    struct symt**       p;

    TRACE_(dbghelp_symt)("Adding local constant (%s:%s): %s %p\n",
                         debugstr_w(module->modulename), debugstr_a(func->hash_elt.name),
                         debugstr_a(name), type);

    assert(symt_check_tag(&func->symt, SymTagFunction) || symt_check_tag(&func->symt, SymTagInlineSite));

    locsym = pool_alloc(&module->pool, sizeof(*locsym));
    locsym->symt.tag      = SymTagData;
    locsym->hash_elt.name = pool_strdup(&module->pool, name);
    locsym->hash_elt.next = NULL;
    locsym->kind          = DataIsConstant;
    locsym->container     = block ? &block->symt : &func->symt;
    locsym->type          = type;
    locsym->u.value       = *v;
    if (block)
        p = vector_add(&block->vchildren, &module->pool);
    else
        p = vector_add(&func->vchildren, &module->pool);
    *p = &locsym->symt;
    return locsym;
}

struct symt_block* symt_open_func_block(struct module* module,
                                        struct symt_function* func,
                                        struct symt_block* parent_block,
                                        unsigned num_ranges)
{
    struct symt_block*  block;
    struct symt**       p;

    assert(symt_check_tag(&func->symt, SymTagFunction) || symt_check_tag(&func->symt, SymTagInlineSite));
    assert(num_ranges > 0);
    assert(!parent_block || parent_block->symt.tag == SymTagBlock);

    block = pool_alloc(&module->pool, offsetof(struct symt_block, ranges[num_ranges]));
    block->symt.tag = SymTagBlock;
    block->num_ranges = num_ranges;
    block->container = parent_block ? &parent_block->symt : &func->symt;
    vector_init(&block->vchildren, sizeof(struct symt*), 0);
    if (parent_block)
        p = vector_add(&parent_block->vchildren, &module->pool);
    else
        p = vector_add(&func->vchildren, &module->pool);
    *p = &block->symt;

    return block;
}

struct symt_block* symt_close_func_block(struct module* module,
                                         const struct symt_function* func,
                                         struct symt_block* block)
{
    assert(symt_check_tag(&func->symt, SymTagFunction) || symt_check_tag(&func->symt, SymTagInlineSite));

    return (block->container->tag == SymTagBlock) ?
        CONTAINING_RECORD(block->container, struct symt_block, symt) : NULL;
}

struct symt_hierarchy_point* symt_add_function_point(struct module* module,
                                                     struct symt_function* func,
                                                     enum SymTagEnum point,
                                                     const struct location* loc,
                                                     const char* name)
{
    struct symt_hierarchy_point*sym;
    struct symt**               p;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = point;
        sym->parent   = &func->symt;
        sym->loc      = *loc;
        sym->hash_elt.name = name ? pool_strdup(&module->pool, name) : NULL;
        p = vector_add(&func->vchildren, &module->pool);
        *p = &sym->symt;
    }
    return sym;
}

struct symt_thunk* symt_new_thunk(struct module* module, 
                                  struct symt_compiland* compiland, 
                                  const char* name, THUNK_ORDINAL ord,
                                  ULONG_PTR addr, ULONG_PTR size)
{
    struct symt_thunk*  sym;

    TRACE_(dbghelp_symt)("Adding global thunk %s:%s @%Ix-%Ix\n",
                         debugstr_w(module->modulename), debugstr_a(name), addr, addr + size - 1);

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag  = SymTagThunk;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->container = &compiland->symt;
        sym->address   = addr;
        sym->size      = size;
        sym->ordinal   = ord;
        symt_add_module_ht(module, (struct symt_ht*)sym);
        if (compiland)
        {
            struct symt**       p;
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_data* symt_new_constant(struct module* module,
                                    struct symt_compiland* compiland,
                                    const char* name, struct symt* type,
                                    const VARIANT* v)
{
    struct symt_data*  sym;

    TRACE_(dbghelp_symt)("Adding constant value %s:%s\n",
                         debugstr_w(module->modulename), debugstr_a(name));

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagData;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->kind          = DataIsConstant;
        sym->container     = compiland ? &compiland->symt : &module->top->symt;
        sym->type          = type;
        sym->u.value       = *v;
        symt_add_module_ht(module, (struct symt_ht*)sym);
        if (compiland)
        {
            struct symt**       p;
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_hierarchy_point* symt_new_label(struct module* module,
                                            struct symt_compiland* compiland,
                                            const char* name, ULONG_PTR address)
{
    struct symt_hierarchy_point*        sym;

    TRACE_(dbghelp_symt)("Adding global label value %s:%s\n",
                         debugstr_w(module->modulename), debugstr_a(name));

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagLabel;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->loc.kind      = loc_absolute;
        sym->loc.offset    = address;
        sym->parent        = compiland ? &compiland->symt : NULL;
        symt_add_module_ht(module, (struct symt_ht*)sym);
        if (compiland)
        {
            struct symt**       p;
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_custom* symt_new_custom(struct module* module, const char* name,
                                    DWORD64 addr, DWORD size)
{
    struct symt_custom*        sym;

    TRACE_(dbghelp_symt)("Adding custom symbol %s:%s\n",
                         debugstr_w(module->modulename), debugstr_a(name));

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagCustom;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->address       = addr;
        sym->size          = size;
        symt_add_module_ht(module, (struct symt_ht*)sym);
    }
    return sym;
}

/* expect sym_info->MaxNameLen to be set before being called */
static void symt_fill_sym_info(struct module_pair* pair,
                               const struct symt_function* func,
                               const struct symt* sym, SYMBOL_INFO* sym_info)
{
    const char* name;
    DWORD64 size;
    char* tmp;

    if (!symt_get_info(pair->effective, sym, TI_GET_TYPE, &sym_info->TypeIndex))
        sym_info->TypeIndex = 0;
    sym_info->Index = symt_ptr2index(pair->effective, sym);
    sym_info->Reserved[0] = sym_info->Reserved[1] = 0;
    if (!symt_get_info(pair->effective, sym, TI_GET_LENGTH, &size) &&
        (!sym_info->TypeIndex ||
         !symt_get_info(pair->effective, symt_index2ptr(pair->effective, sym_info->TypeIndex),
                         TI_GET_LENGTH, &size)))
        size = 0;
    sym_info->Size = (DWORD)size;
    sym_info->ModBase = pair->requested->module.BaseOfImage;
    sym_info->Flags = 0;
    sym_info->Value = 0;

    switch (sym->tag)
    {
    case SymTagData:
        {
            const struct symt_data*  data = (const struct symt_data*)sym;
            switch (data->kind)
            {
            case DataIsParam:
                sym_info->Flags |= SYMFLAG_PARAMETER;
                /* fall through */
            case DataIsLocal:
                sym_info->Flags |= SYMFLAG_LOCAL;
                {
                    struct location loc = data->u.var;

                    if (loc.kind >= loc_user)
                    {
                        struct module_format_vtable_iterator iter = {};

                        while ((module_format_vtable_iterator_next(pair->effective, &iter,
                                                                   MODULE_FORMAT_VTABLE_INDEX(loc_compute))))
                        {
                            iter.modfmt->vtable->loc_compute(iter.modfmt, func, &loc);
                            break;
                        }
                    }
                    switch (loc.kind)
                    {
                    case loc_error:
                        /* for now we report error cases as a negative register number */
                        /* fall through */
                    case loc_register:
                        sym_info->Flags |= SYMFLAG_REGISTER;
                        sym_info->Register = loc.reg;
                        sym_info->Address = 0;
                        break;
                    case loc_regrel:
                        sym_info->Flags |= SYMFLAG_REGREL;
                        sym_info->Register = loc.reg;
                        if (loc.reg == CV_REG_NONE || (int)loc.reg < 0 /* error */)
                            FIXME("suspicious register value %x\n", loc.reg);
                        sym_info->Address = loc.offset;
                        break;
                    case loc_absolute:
                        sym_info->Flags |= SYMFLAG_VALUEPRESENT;
                        sym_info->Value = loc.offset;
                        break;
                    default:
                        FIXME("Shouldn't happen (kind=%d), debug reader backend is broken\n", loc.kind);
                        assert(0);
                    }
                }
                break;
            case DataIsGlobal:
            case DataIsFileStatic:
            case DataIsStaticLocal:
                switch (data->u.var.kind)
                {
                case loc_tlsrel:
                    sym_info->Flags |= SYMFLAG_TLSREL;
                    /* fall through */
                case loc_absolute:
                    symt_get_address(sym, &sym_info->Address);
                    sym_info->Register = 0;
                    break;
                default:
                    FIXME("Shouldn't happen (kind=%d), debug reader backend is broken\n", data->u.var.kind);
                    assert(0);
                }
                break;
            case DataIsConstant:
                sym_info->Flags |= SYMFLAG_VALUEPRESENT;
                if (data->container &&
                    (data->container->tag == SymTagFunction || data->container->tag == SymTagBlock))
                    sym_info->Flags |= SYMFLAG_LOCAL;
                switch (V_VT(&data->u.value))
                {
                case VT_I8:  sym_info->Value = (LONG64)V_I8(&data->u.value); break;
                case VT_I4:  sym_info->Value = (LONG64)V_I4(&data->u.value); break;
                case VT_I2:  sym_info->Value = (LONG64)V_I2(&data->u.value); break;
                case VT_I1:  sym_info->Value = (LONG64)V_I1(&data->u.value); break;
                case VT_UINT:sym_info->Value = V_UINT(&data->u.value); break;
                case VT_UI8: sym_info->Value = V_UI8(&data->u.value); break;
                case VT_UI4: sym_info->Value = V_UI4(&data->u.value); break;
                case VT_UI2: sym_info->Value = V_UI2(&data->u.value); break;
                case VT_UI1: sym_info->Value = V_UI1(&data->u.value); break;
                case VT_BYREF: sym_info->Value = (DWORD_PTR)V_BYREF(&data->u.value); break;
                case VT_EMPTY: sym_info->Value = 0; break;
                default:
                    FIXME("Unsupported variant type (%u)\n", V_VT(&data->u.value));
                    sym_info->Value = 0;
                    break;
                }
                break;
            default:
                FIXME("Unhandled kind (%u) in sym data\n", data->kind);
            }
        }
        break;
    case SymTagPublicSymbol:
        {
            const struct symt_public* pub = (const struct symt_public*)sym;
            if (pub->is_function)
                sym_info->Flags |= SYMFLAG_PUBLIC_CODE;
            else
                sym_info->Flags |= SYMFLAG_EXPORT;
            symt_get_address(sym, &sym_info->Address);
        }
        break;
    case SymTagFunction:
    case SymTagInlineSite:
        symt_get_address(sym, &sym_info->Address);
        break;
    case SymTagThunk:
        sym_info->Flags |= SYMFLAG_THUNK;
        symt_get_address(sym, &sym_info->Address);
        break;
    case SymTagCustom:
        symt_get_address(sym, &sym_info->Address);
        sym_info->Flags |= SYMFLAG_VIRTUAL;
        break;
    default:
        symt_get_address(sym, &sym_info->Address);
        sym_info->Register = 0;
        break;
    }
    sym_info->Scope = 0; /* FIXME */
    sym_info->Tag = sym->tag;
    name = symt_get_name(sym);
    if (sym_info->MaxNameLen &&
        sym->tag == SymTagPublicSymbol && (dbghelp_options & SYMOPT_UNDNAME) &&
        (tmp = __unDName(NULL, name, 0, malloc, free, UNDNAME_NAME_ONLY)) != NULL)
    {
        symbol_setname(sym_info, tmp);
        free(tmp);
    }
    else
        symbol_setname(sym_info, name);

    TRACE_(dbghelp_symt)("%p => %s %lu %I64x\n",
                         sym, debugstr_a(sym_info->Name), sym_info->Size, sym_info->Address);
}

struct sym_enum
{
    PSYM_ENUMERATESYMBOLS_CALLBACK      cb;
    PVOID                               user;
    SYMBOL_INFO*                        sym_info;
    DWORD                               index;
    DWORD                               tag;
    DWORD64                             addr;
    char                                buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
};

static BOOL send_symbol(const struct sym_enum* se, struct module_pair* pair,
                        const struct symt_function* func, const struct symt* sym)
{
    symt_fill_sym_info(pair, func, sym, se->sym_info);
    if (se->index && se->sym_info->Index != se->index) return FALSE;
    if (se->tag && se->sym_info->Tag != se->tag) return FALSE;
    if (se->addr && !(se->addr >= se->sym_info->Address && se->addr < se->sym_info->Address + se->sym_info->Size)) return FALSE;
    return !se->cb(se->sym_info, se->sym_info->Size, se->user);
}

static BOOL symt_enum_module(struct module_pair* pair, const WCHAR* match,
                             const struct sym_enum* se)
{
    void*                       ptr;
    struct symt_ht*             sym = NULL;
    struct hash_table_iter      hti;
    WCHAR*                      nameW;
    BOOL                        ret;

    hash_table_iter_init(&pair->effective->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = CONTAINING_RECORD(ptr, struct symt_ht, hash_elt);
        nameW = symt_get_nameW(&sym->symt);
        ret = SymMatchStringW(nameW, match, FALSE);
        HeapFree(GetProcessHeap(), 0, nameW);
        if (ret)
        {
            se->sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            se->sym_info->MaxNameLen = sizeof(se->buffer) - sizeof(SYMBOL_INFO);
            if (send_symbol(se, pair, NULL, &sym->symt)) return TRUE;
        }
    }
    return FALSE;
}

static inline unsigned where_to_insert(struct module* module, unsigned high, const struct symt_ht* elt)
{
    unsigned    low = 0, mid = high / 2;
    ULONG64     addr;

    if (!high) return 0;
    symt_get_address(&elt->symt, &addr);
    do
    {
        switch (cmp_sorttab_addr(module, mid, addr))
        {
        case 0: return mid;
        case -1: low = mid + 1; break;
        case 1: high = mid; break;
        }
        mid = low + (high - low) / 2;
    } while (low < high);
    return mid;
}

/***********************************************************************
 *              resort_symbols
 *
 * Rebuild sorted list of symbols for a module.
 */
static BOOL resort_symbols(struct module* module)
{
    int delta;

    if (!(module->module.NumSyms = module->num_symbols))
        return FALSE;

    /* we know that set from 0 up to num_sorttab is already sorted
     * so sort the remaining (new) symbols, and merge the two sets
     * (unless the first set is empty)
     */
    delta = module->num_symbols - module->num_sorttab;
    qsort(&module->addr_sorttab[module->num_sorttab], delta, sizeof(struct symt_ht*), symt_cmp_addr);
    if (module->num_sorttab)
    {
        int     i, ins_idx = module->num_sorttab, prev_ins_idx;
        static struct symt_ht** tmp;
        static unsigned num_tmp;

        if (num_tmp < delta)
        {
            static struct symt_ht** new;
            if (tmp)
                new = HeapReAlloc(GetProcessHeap(), 0, tmp, delta * sizeof(struct symt_ht*));
            else
                new = HeapAlloc(GetProcessHeap(), 0, delta * sizeof(struct symt_ht*));
            if (!new)
            {
                module->num_sorttab = 0;
                return resort_symbols(module);
            }
            tmp = new;
            num_tmp = delta;
        }
        memcpy(tmp, &module->addr_sorttab[module->num_sorttab], delta * sizeof(struct symt_ht*));
        qsort(tmp, delta, sizeof(struct symt_ht*), symt_cmp_addr);

        for (i = delta - 1; i >= 0; i--)
        {
            prev_ins_idx = ins_idx;
            ins_idx = where_to_insert(module, ins_idx, tmp[i]);
            memmove(&module->addr_sorttab[ins_idx + i + 1],
                    &module->addr_sorttab[ins_idx],
                    (prev_ins_idx - ins_idx) * sizeof(struct symt_ht*));
            module->addr_sorttab[ins_idx + i] = tmp[i];
        }
    }
    module->num_sorttab = module->num_symbols;
    return module->sortlist_valid = TRUE;
}

static void symt_get_length(struct module* module, const struct symt* symt, ULONG64* size)
{
    DWORD       type_index;

    if (symt_get_info(module,  symt, TI_GET_LENGTH, size) && *size)
        return;

    if (symt_get_info(module, symt, TI_GET_TYPE, &type_index) &&
        symt_get_info(module, symt_index2ptr(module, type_index), TI_GET_LENGTH, size)) return;
    *size = 1; /* no size info */
}

/* needed by symt_find_nearest */
static int symt_get_best_at(struct module* module, int idx_sorttab)
{
    ULONG64 ref_addr;
    int idx_sorttab_orig = idx_sorttab;
    if (module->addr_sorttab[idx_sorttab]->symt.tag == SymTagPublicSymbol)
    {
        symt_get_address(&module->addr_sorttab[idx_sorttab]->symt, &ref_addr);
        while (idx_sorttab > 0 &&
               module->addr_sorttab[idx_sorttab]->symt.tag == SymTagPublicSymbol &&
               !cmp_sorttab_addr(module, idx_sorttab - 1, ref_addr))
            idx_sorttab--;
        if (module->addr_sorttab[idx_sorttab]->symt.tag == SymTagPublicSymbol)
        {
            idx_sorttab = idx_sorttab_orig;
            while (idx_sorttab < module->num_sorttab - 1 &&
                   module->addr_sorttab[idx_sorttab]->symt.tag == SymTagPublicSymbol &&
                   !cmp_sorttab_addr(module, idx_sorttab + 1, ref_addr))
                idx_sorttab++;
        }
        /* if no better symbol was found restore the original */
        if (module->addr_sorttab[idx_sorttab]->symt.tag == SymTagPublicSymbol)
            idx_sorttab = idx_sorttab_orig;
    }
    return idx_sorttab;
}

/* assume addr is in module */
struct symt_ht* symt_find_nearest(struct module* module, DWORD_PTR addr)
{
    int         mid, high, low;
    ULONG64     ref_addr, ref_size;

    if (!module->sortlist_valid || !module->addr_sorttab)
    {
        if (!resort_symbols(module)) return NULL;
    }

    /*
     * Binary search to find closest symbol.
     */
    low = 0;
    high = module->num_sorttab;

    symt_get_address(&module->addr_sorttab[0]->symt, &ref_addr);
    if (addr < ref_addr) return NULL;

    if (high)
    {
        symt_get_address(&module->addr_sorttab[high - 1]->symt, &ref_addr);
        symt_get_length(module, &module->addr_sorttab[high - 1]->symt, &ref_size);
        if (addr >= ref_addr + ref_size) return NULL;
    }
    
    while (high > low + 1)
    {
        mid = (high + low) / 2;
        if (cmp_sorttab_addr(module, mid, addr) < 0)
            low = mid;
        else
            high = mid;
    }
    if (low != high && high != module->num_sorttab &&
        cmp_sorttab_addr(module, high, addr) <= 0)
        low = high;

    /* If found symbol is a public symbol, check if there are any other entries that
     * might also have the same address, but would get better information
     */
    low = symt_get_best_at(module, low);

    return module->addr_sorttab[low];
}

struct symt_ht* symt_find_symbol_at(struct module* module, DWORD_PTR addr)
{
    struct symt_ht* nearest = symt_find_nearest(module, addr);
    if (nearest)
    {
        ULONG64     symaddr, symsize;
        symt_get_address(&nearest->symt, &symaddr);
        symt_get_length(module, &nearest->symt, &symsize);
        if (addr < symaddr || addr >= symaddr + symsize)
            nearest = NULL;
    }
    return nearest;
}

static BOOL symt_enum_locals_helper(struct module_pair* pair,
                                    const WCHAR* match, const struct sym_enum* se,
                                    struct symt_function* func, const struct vector* v)
{
    struct symt*        lsym = NULL;
    DWORD_PTR           pc = pair->pcs->localscope_pc;
    unsigned int        i;
    WCHAR*              nameW;
    BOOL                ret;

    for (i=0; i<vector_length(v); i++)
    {
        lsym = *(struct symt**)vector_at(v, i);
        switch (lsym->tag)
        {
        case SymTagBlock:
            {
                struct symt_block*  block = (struct symt_block*)lsym;
                unsigned j;
                for (j = 0; j < block->num_ranges; j++)
                {
                    if (pc >= block->ranges[j].low && pc < block->ranges[j].high)
                    {
                        if (!symt_enum_locals_helper(pair, match, se, func, &block->vchildren))
                            return FALSE;
                    }
                }
            }
            break;
        case SymTagData:
            nameW = symt_get_nameW(lsym);
            ret = SymMatchStringW(nameW, match,
                                  !(dbghelp_options & SYMOPT_CASE_INSENSITIVE));
            HeapFree(GetProcessHeap(), 0, nameW);
            if (ret)
            {
                if (send_symbol(se, pair, func, lsym)) return FALSE;
            }
            break;
        case SymTagLabel:
        case SymTagFuncDebugStart:
        case SymTagFuncDebugEnd:
        case SymTagCustom:
        case SymTagInlineSite:
            break;
        default:
            FIXME("Unknown type: %u (%x)\n", lsym->tag, lsym->tag);
            assert(0);
        }
    }
    return TRUE;
}

static BOOL symt_enum_locals(struct process* pcs, const WCHAR* mask,
                             const struct sym_enum* se)
{
    struct module_pair  pair;

    se->sym_info->SizeOfStruct = sizeof(*se->sym_info);
    se->sym_info->MaxNameLen = sizeof(se->buffer) - sizeof(SYMBOL_INFO);

    pair.pcs = pcs;
    pair.requested = module_find_by_addr(pair.pcs, pcs->localscope_pc);
    if (!module_get_debug(&pair)) return FALSE;

    if (symt_check_tag(pcs->localscope_symt, SymTagFunction) ||
        symt_check_tag(pcs->localscope_symt, SymTagInlineSite))
    {
        struct symt_function* func = (struct symt_function*)pcs->localscope_symt;
        return symt_enum_locals_helper(&pair, mask ? mask : L"*", se, func, &func->vchildren);
    }
    return FALSE;
}

/**********************************************************
 *              symbol_setname
 *
 * Properly sets Name and NameLen in SYMBOL_INFO
 * according to MaxNameLen value
 */
void symbol_setname(SYMBOL_INFO* sym_info, const char* name)
{
    SIZE_T len = 0;
    if (name)
    {
        sym_info->NameLen = strlen(name);
        if (sym_info->MaxNameLen)
        {
            len = min(sym_info->NameLen, sym_info->MaxNameLen - 1);
            memcpy(sym_info->Name, name, len);
        }
    }
    else
        sym_info->NameLen = 0;
    sym_info->Name[len] = '\0';
}

/******************************************************************
 *		copy_symbolW
 *
 * Helper for transforming an ANSI symbol info into a UNICODE one.
 * Assume that MaxNameLen is the same for both version (A & W).
 */
void copy_symbolW(SYMBOL_INFOW* siw, const SYMBOL_INFO* si)
{
    siw->SizeOfStruct = si->SizeOfStruct;
    siw->TypeIndex = si->TypeIndex; 
    siw->Reserved[0] = si->Reserved[0];
    siw->Reserved[1] = si->Reserved[1];
    siw->Index = si->Index;
    siw->Size = si->Size;
    siw->ModBase = si->ModBase;
    siw->Flags = si->Flags;
    siw->Value = si->Value;
    siw->Address = si->Address;
    siw->Register = si->Register;
    siw->Scope = si->Scope;
    siw->Tag = si->Tag;
    siw->NameLen = si->NameLen;
    siw->MaxNameLen = si->MaxNameLen;
    MultiByteToWideChar(CP_ACP, 0, si->Name, -1, siw->Name, siw->MaxNameLen);
}

/* return the lowest inline site inside a function */
struct symt_function* symt_find_lowest_inlined(struct symt_function* func, DWORD64 addr)
{
    struct symt_function* current;
    int i;

    assert(func->symt.tag == SymTagFunction);
    for (current = func->next_inlinesite; current; current = current->next_inlinesite)
    {
        for (i = 0; i < current->num_ranges; ++i)
        {
            /* first matching range gives the lowest inline site; see dbghelp_private.h for details */
            if (current->ranges[i].low <= addr && addr < current->ranges[i].high)
                return current;
        }
    }
    return NULL;
}

/* from an inline function, get either the enclosing inlined function, or the top function when no inlined */
struct symt* symt_get_upper_inlined(struct symt_function* inlined)
{
    struct symt* symt = &inlined->symt;

    do
    {
        assert(symt);
        if (symt->tag == SymTagBlock)
            symt = ((struct symt_block*)symt)->container;
        else
            symt = ((struct symt_function*)symt)->container;
    } while (symt->tag == SymTagBlock);
    assert(symt->tag == SymTagFunction || symt->tag == SymTagInlineSite);
    return symt;
}

/* lookup in module for an inline site (from addr and inline_ctx) */
struct symt_function* symt_find_inlined_site(struct module* module, DWORD64 addr, DWORD inline_ctx)
{
    struct symt_ht* symt = symt_find_symbol_at(module, addr);

    if (symt_check_tag(&symt->symt, SymTagFunction))
    {
        struct symt_function* func = (struct symt_function*)symt;
        struct symt_function* curr = symt_find_lowest_inlined(func, addr);
        DWORD depth = IFC_DEPTH(inline_ctx);

        if (curr)
            for ( ; curr != func; curr = (struct symt_function*)symt_get_upper_inlined(curr))
                if (depth-- == 0) return curr;
    }
    return NULL;
}

/******************************************************************
 *		sym_enum
 *
 * Core routine for most of the enumeration of symbols
 */
static BOOL sym_enum(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR Mask,
                     const struct sym_enum* se)
{
    struct module_pair  pair;
    const WCHAR*        bang;
    WCHAR*              mod;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    if (BaseOfDll == 0)
    {
        /* do local variables ? */
        if (!Mask || !(bang = wcschr(Mask, '!')))
            return symt_enum_locals(pair.pcs, Mask, se);

        if (bang == Mask) return FALSE;

        mod = HeapAlloc(GetProcessHeap(), 0, (bang - Mask + 1) * sizeof(WCHAR));
        if (!mod) return FALSE;
        memcpy(mod, Mask, (bang - Mask) * sizeof(WCHAR));
        mod[bang - Mask] = 0;

        for (pair.requested = pair.pcs->lmodules; pair.requested; pair.requested = pair.requested->next)
        {
            if (pair.requested->type == DMT_PE && module_get_debug(&pair))
            {
                if (SymMatchStringW(pair.requested->modulename, mod, FALSE) &&
                    symt_enum_module(&pair, bang + 1, se))
                    break;
            }
        }
        /* not found in PE modules, retry on the ELF ones
         */
        if (!pair.requested && dbghelp_opt_native)
        {
            for (pair.requested = pair.pcs->lmodules; pair.requested; pair.requested = pair.requested->next)
            {
                if ((pair.requested->type == DMT_ELF || pair.requested->type == DMT_MACHO) &&
                    !module_get_containee(pair.pcs, pair.requested) &&
                    module_get_debug(&pair))
                {
                    if (SymMatchStringW(pair.requested->modulename, mod, FALSE) &&
                        symt_enum_module(&pair, bang + 1, se))
                    break;
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, mod);
        return TRUE;
    }
    pair.requested = module_find_by_addr(pair.pcs, BaseOfDll);
    if (!module_get_debug(&pair))
        return FALSE;

    /* we always ignore module name from Mask when BaseOfDll is defined */
    if (Mask && (bang = wcschr(Mask, '!')))
    {
        if (bang == Mask) return FALSE;
        Mask = bang + 1;
    }

    symt_enum_module(&pair, Mask ? Mask : L"*", se);

    return TRUE;
}

static inline BOOL doSymEnumSymbols(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR Mask,
                                    PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                                    PVOID UserContext)
{
    struct sym_enum     se;

    se.cb = EnumSymbolsCallback;
    se.user = UserContext;
    se.index = 0;
    se.tag = 0;
    se.addr = 0;
    se.sym_info = (PSYMBOL_INFO)se.buffer;

    return sym_enum(hProcess, BaseOfDll, Mask, &se);
}

/******************************************************************
 *		SymEnumSymbols (DBGHELP.@)
 *
 * cases BaseOfDll = 0
 *      !foo fails always (despite what MSDN states)
 *      RE1!RE2 looks up all modules matching RE1, and in all these modules, lookup RE2
 *      no ! in Mask, lookup in local Context
 * cases BaseOfDll != 0
 *      !foo fails always (despite what MSDN states)
 *      RE1!RE2 gets RE2 from BaseOfDll (whatever RE1 is)
 */
BOOL WINAPI SymEnumSymbols(HANDLE hProcess, ULONG64 BaseOfDll, PCSTR Mask,
                           PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                           PVOID UserContext)
{
    BOOL                ret;
    PWSTR               maskW = NULL;

    TRACE("(%p %I64x %s %p %p)\n",
          hProcess, BaseOfDll, debugstr_a(Mask), EnumSymbolsCallback, UserContext);

    if (Mask)
    {
        DWORD sz = MultiByteToWideChar(CP_ACP, 0, Mask, -1, NULL, 0);
        if (!(maskW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, Mask, -1, maskW, sz);
    }
    ret = doSymEnumSymbols(hProcess, BaseOfDll, maskW, EnumSymbolsCallback, UserContext);
    HeapFree(GetProcessHeap(), 0, maskW);
    return ret;
}

struct sym_enumW
{
    PSYM_ENUMERATESYMBOLS_CALLBACKW     cb;
    void*                               ctx;
    PSYMBOL_INFOW                       sym_info;
    char                                buffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(WCHAR)];
};
    
static BOOL CALLBACK sym_enumW(PSYMBOL_INFO si, ULONG size, PVOID ctx)
{
    struct sym_enumW*   sew = ctx;

    copy_symbolW(sew->sym_info, si);

    return (sew->cb)(sew->sym_info, size, sew->ctx);
}

/******************************************************************
 *		SymEnumSymbolsW (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSymbolsW(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR Mask,
                            PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
                            PVOID UserContext)
{
    struct sym_enumW    sew;

    sew.ctx = UserContext;
    sew.cb = EnumSymbolsCallback;
    sew.sym_info = (PSYMBOL_INFOW)sew.buffer;

    return doSymEnumSymbols(hProcess, BaseOfDll, Mask, sym_enumW, &sew);
}

struct sym_enumerate
{
    void*                       ctx;
    PSYM_ENUMSYMBOLS_CALLBACK   cb;
};

static BOOL CALLBACK sym_enumerate_cb(PSYMBOL_INFO syminfo, ULONG size, void* ctx)
{
    struct sym_enumerate*       se = ctx;
    return (se->cb)(syminfo->Name, syminfo->Address, syminfo->Size, se->ctx);
}

/***********************************************************************
 *		SymEnumerateSymbols (DBGHELP.@)
 */
BOOL WINAPI SymEnumerateSymbols(HANDLE hProcess, DWORD BaseOfDll,
                                PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback, 
                                PVOID UserContext)
{
    struct sym_enumerate        se;

    se.ctx = UserContext;
    se.cb  = EnumSymbolsCallback;
    
    return SymEnumSymbols(hProcess, BaseOfDll, NULL, sym_enumerate_cb, &se);
}

struct sym_enumerate64
{
    void*                       ctx;
    PSYM_ENUMSYMBOLS_CALLBACK64 cb;
};

static BOOL CALLBACK sym_enumerate_cb64(PSYMBOL_INFO syminfo, ULONG size, void* ctx)
{
    struct sym_enumerate64*     se = ctx;
    return (se->cb)(syminfo->Name, syminfo->Address, syminfo->Size, se->ctx);
}

/***********************************************************************
 *              SymEnumerateSymbols64 (DBGHELP.@)
 */
BOOL WINAPI SymEnumerateSymbols64(HANDLE hProcess, DWORD64 BaseOfDll,
                                  PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
                                  PVOID UserContext)
{
    struct sym_enumerate64      se;

    se.ctx = UserContext;
    se.cb  = EnumSymbolsCallback;

    return SymEnumSymbols(hProcess, BaseOfDll, NULL, sym_enumerate_cb64, &se);
}

/******************************************************************
 *		SymFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromAddr(HANDLE hProcess, DWORD64 Address, 
                        DWORD64* Displacement, PSYMBOL_INFO Symbol)
{
    struct module_pair  pair;
    struct symt_ht*     sym;

    if (!module_init_pair(&pair, hProcess, Address)) return FALSE;
    if ((sym = symt_find_symbol_at(pair.effective, Address)) == NULL) return FALSE;

    symt_fill_sym_info(&pair, NULL, &sym->symt, Symbol);
    if (Displacement)
        *Displacement = (Address >= Symbol->Address) ? (Address - Symbol->Address) : (DWORD64)-1;
    return TRUE;
}

/******************************************************************
 *		SymFromAddrW (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromAddrW(HANDLE hProcess, DWORD64 Address, 
                         DWORD64* Displacement, PSYMBOL_INFOW Symbol)
{
    PSYMBOL_INFO        si;
    unsigned            len;
    BOOL                ret;

    len = sizeof(*si) + Symbol->MaxNameLen * sizeof(WCHAR);
    si = HeapAlloc(GetProcessHeap(), 0, len);
    if (!si) return FALSE;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = Symbol->MaxNameLen;
    if ((ret = SymFromAddr(hProcess, Address, Displacement, si)))
    {
        copy_symbolW(Symbol, si);
    }
    HeapFree(GetProcessHeap(), 0, si);
    return ret;
}

/******************************************************************
 *		SymGetSymFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSymFromAddr(HANDLE hProcess, DWORD Address,
                              PDWORD Displacement, PIMAGEHLP_SYMBOL Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;
    DWORD64     Displacement64;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromAddr(hProcess, Address, &Displacement64, si))
        return FALSE;

    if (Displacement)
        *Displacement = Displacement64;
    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    lstrcpynA(Symbol->Name, si->Name, len);
    return TRUE;
}

/******************************************************************
 *		SymGetSymFromAddr64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSymFromAddr64(HANDLE hProcess, DWORD64 Address,
                                PDWORD64 Displacement, PIMAGEHLP_SYMBOL64 Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;
    DWORD64     Displacement64;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromAddr(hProcess, Address, &Displacement64, si))
        return FALSE;

    if (Displacement)
        *Displacement = Displacement64;
    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    lstrcpynA(Symbol->Name, si->Name, len);
    return TRUE;
}

static BOOL find_name(struct process* pcs, struct module* module, const char* name,
                      SYMBOL_INFO* symbol)
{
    struct hash_table_iter      hti;
    void*                       ptr;
    struct symt_ht*             sym = NULL;
    struct module_pair          pair;

    pair.pcs = pcs;
    if (!(pair.requested = module)) return FALSE;
    if (!module_get_debug(&pair)) return FALSE;

    hash_table_iter_init(&pair.effective->ht_symbols, &hti, name);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = CONTAINING_RECORD(ptr, struct symt_ht, hash_elt);

        if (!strcmp(sym->hash_elt.name, name))
        {
            symt_fill_sym_info(&pair, NULL, &sym->symt, symbol);
            return TRUE;
        }
    }
    return FALSE;

}
/******************************************************************
 *		SymFromName (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromName(HANDLE hProcess, PCSTR Name, PSYMBOL_INFO Symbol)
{
    struct process*             pcs = process_find_by_handle(hProcess);
    struct module_pair          pair;
    struct module*              module;
    const char*                 name;

    TRACE("(%p, %s, %p)\n", hProcess, debugstr_a(Name), Symbol);
    if (!pcs) return FALSE;
    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    name = strchr(Name, '!');
    if (name)
    {
        char    tmp[128];
        assert(name - Name < sizeof(tmp));
        memcpy(tmp, Name, name - Name);
        tmp[name - Name] = '\0';
        module = module_find_by_nameA(pcs, tmp);
        return find_name(pcs, module, name + 1, Symbol);
    }

    /* search first in local context */
    pair.pcs = pcs;
    pair.requested = module_find_by_addr(pair.pcs, pcs->localscope_pc);
    if (module_get_debug(&pair) &&
        (symt_check_tag(pcs->localscope_symt, SymTagFunction) ||
         symt_check_tag(pcs->localscope_symt, SymTagInlineSite)))
    {
        struct symt_function* func = (struct symt_function*)pcs->localscope_symt;
        struct vector* v = &func->vchildren;
        unsigned i;

        for (i = 0; i < vector_length(v); i++)
        {
            struct symt* lsym = *(struct symt**)vector_at(v, i);
            switch (lsym->tag)
            {
            case SymTagBlock: /* no recursion */
                break;
            case SymTagData:
                name = symt_get_name(lsym);
                if (name && !strcmp(name, Name))
                {
                    symt_fill_sym_info(&pair, func, lsym, Symbol);
                    return TRUE;
                }
                break;
            case SymTagLabel: /* not returned here */
            case SymTagFuncDebugStart:
            case SymTagFuncDebugEnd:
            case SymTagCustom:
            case SymTagInlineSite:
                break;
            default:
                WARN("Unsupported tag: %u (%x)\n", lsym->tag, lsym->tag);
            }
        }
    }
    /* lookup at global scope */
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_PE && find_name(pcs, module, Name, Symbol))
            return TRUE;
    }
    /* not found in PE modules, retry on the ELF ones
     */
    if (dbghelp_opt_native)
    {
        for (module = pcs->lmodules; module; module = module->next)
        {
            if ((module->type == DMT_ELF || module->type == DMT_MACHO) &&
                !module_get_containee(pcs, module) &&
                find_name(pcs, module, Name, Symbol))
                return TRUE;
        }
    }
    SetLastError(ERROR_MOD_NOT_FOUND);
    return FALSE;
}

/***********************************************************************
 *      SymFromNameW (DBGHELP.@)
 */
BOOL WINAPI SymFromNameW(HANDLE process, const WCHAR *name, SYMBOL_INFOW *symbol)
{
    SYMBOL_INFO *si;
    DWORD len;
    char *tmp;
    BOOL ret;

    TRACE("(%p, %s, %p)\n", process, debugstr_w(name), symbol);

    len = sizeof(*si) + symbol->MaxNameLen;
    if (!(si = HeapAlloc(GetProcessHeap(), 0, len))) return FALSE;

    len = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
    if (!(tmp = HeapAlloc(GetProcessHeap(), 0, len)))
    {
        HeapFree(GetProcessHeap(), 0, si);
        return FALSE;
    }
    WideCharToMultiByte(CP_ACP, 0, name, -1, tmp, len, NULL, NULL);

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = symbol->MaxNameLen;
    if ((ret = SymFromName(process, tmp, si)))
        copy_symbolW(symbol, si);

    HeapFree(GetProcessHeap(), 0, tmp);
    HeapFree(GetProcessHeap(), 0, si);
    return ret;
}

/***********************************************************************
 *		SymGetSymFromName64 (DBGHELP.@)
 */
BOOL WINAPI SymGetSymFromName64(HANDLE hProcess, PCSTR Name, PIMAGEHLP_SYMBOL64 Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromName(hProcess, Name, si)) return FALSE;

    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    lstrcpynA(Symbol->Name, si->Name, len);
    return TRUE;
}

/***********************************************************************
 *		SymGetSymFromName (DBGHELP.@)
 */
BOOL WINAPI SymGetSymFromName(HANDLE hProcess, PCSTR Name, PIMAGEHLP_SYMBOL Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromName(hProcess, Name, si)) return FALSE;

    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    lstrcpynA(Symbol->Name, si->Name, len);
    return TRUE;
}

struct lineinfo_t
{
    BOOL                        unicode;
    PVOID                       key;
    DWORD                       line_number;
    union
    {
        CHAR*                   file_nameA;
        WCHAR*                  file_nameW;
    };
    DWORD64                     address;
};

static void init_lineinfo(struct lineinfo_t* line_info, BOOL unicode)
{
    line_info->unicode = unicode;
    line_info->key = NULL;
    line_info->line_number = 0;
    line_info->file_nameA = NULL;
    line_info->address = 0;
}

static BOOL lineinfo_copy_toA32(const struct lineinfo_t* line_info, IMAGEHLP_LINE* l32)
{
    if (line_info->unicode) return FALSE;
    l32->Key = line_info->key;
    l32->LineNumber = line_info->line_number;
    l32->FileName = line_info->file_nameA;
    l32->Address = line_info->address;
    return TRUE;
}

static BOOL lineinfo_copy_toA64(const struct lineinfo_t* line_info, IMAGEHLP_LINE64* l64)
{
    if (line_info->unicode) return FALSE;
    l64->Key = line_info->key;
    l64->LineNumber = line_info->line_number;
    l64->FileName = line_info->file_nameA;
    l64->Address = line_info->address;
    return TRUE;
}

static BOOL lineinfo_copy_toW64(const struct lineinfo_t* line_info, IMAGEHLP_LINEW64* l64)
{
    if (!line_info->unicode) return FALSE;
    l64->Key = line_info->key;
    l64->LineNumber = line_info->line_number;
    l64->FileName = line_info->file_nameW;
    l64->Address = line_info->address;
    return TRUE;
}

static BOOL lineinfo_set_nameA(struct process* pcs, struct lineinfo_t* line_info, char* str)
{
    DWORD len;

    if (line_info->unicode)
    {
        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        if (!(line_info->file_nameW = fetch_buffer(pcs, len * sizeof(WCHAR)))) return FALSE;
        MultiByteToWideChar(CP_ACP, 0, str, -1, line_info->file_nameW, len);
    }
    else
    {
        len = strlen(str) + 1;
        if (!(line_info->file_nameA = fetch_buffer(pcs, len))) return FALSE;
        memcpy(line_info->file_nameA, str, len);
    }
    return TRUE;
}

static BOOL lineinfo_set_nameW(struct process* pcs, struct lineinfo_t* line_info, WCHAR* wstr)
{
    DWORD len;

    if (line_info->unicode)
    {
        len = (lstrlenW(wstr) + 1) * sizeof(WCHAR);
        if (!(line_info->file_nameW = fetch_buffer(pcs, len))) return FALSE;
        memcpy(line_info->file_nameW, wstr, len);
    }
    else
    {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
        if (!(line_info->file_nameA = fetch_buffer(pcs, len))) return FALSE;
        WideCharToMultiByte(CP_ACP, 0, wstr, -1, line_info->file_nameA, len, NULL, NULL);
    }
    return TRUE;
}

static BOOL get_line_from_function(struct module_pair* pair, struct symt_function* func, DWORD64 addr,
                                   PDWORD pdwDisplacement, struct lineinfo_t* line_info)
{
    struct line_info*           dli = NULL;
    struct line_info*           found_dli = NULL;
    int                         i;

    for (i = vector_length(&func->vlines) - 1; i >= 0; i--)
    {
        dli = vector_at(&func->vlines, i);
        if (!dli->is_source_file)
        {
            if (found_dli || dli->u.address > addr) continue;
            line_info->line_number = dli->line_number;
            line_info->address     = dli->u.address;
            line_info->key         = dli;
            found_dli = dli;
            continue;
        }
        if (found_dli)
        {
            BOOL ret;
            if (dbghelp_opt_source_actual_path)
            {
                /* Return native file paths when using winedbg */
                ret = lineinfo_set_nameA(pair->pcs, line_info, (char*)source_get(pair->effective, dli->u.source_file));
            }
            else
            {
                WCHAR *dospath = wine_get_dos_file_name(source_get(pair->effective, dli->u.source_file));
                ret = lineinfo_set_nameW(pair->pcs, line_info, dospath);
                HeapFree( GetProcessHeap(), 0, dospath );
            }
            if (ret && pdwDisplacement) *pdwDisplacement = addr - found_dli->u.address;
            return ret;
        }
    }
    return FALSE;
}

/******************************************************************
 *		get_line_from_addr
 *
 * fills source file information from an address
 */
static BOOL get_line_from_addr(HANDLE hProcess, DWORD64 addr,
                               PDWORD pdwDisplacement, struct lineinfo_t* line_info)
{
    struct module_pair          pair;
    struct symt_ht*             symt;

    if (!module_init_pair(&pair, hProcess, addr)) return FALSE;
    symt = symt_find_symbol_at(pair.effective, addr);

    if (!symt_check_tag(&symt->symt, SymTagFunction)) return FALSE;
    return get_line_from_function(&pair, (struct symt_function*)symt, addr, pdwDisplacement, line_info);
}

/***********************************************************************
 *		SymGetSymNext64 (DBGHELP.@)
 */
BOOL WINAPI SymGetSymNext64(HANDLE hProcess, PIMAGEHLP_SYMBOL64 Symbol)
{
    /* algo:
     * get module from Symbol.Address
     * get index in module.addr_sorttab of Symbol.Address
     * increment index
     * if out of module bounds, move to next module in process address space
     */
    FIXME("(%p, %p): stub\n", hProcess, Symbol);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SymGetSymNext (DBGHELP.@)
 */
BOOL WINAPI SymGetSymNext(HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
    FIXME("(%p, %p): stub\n", hProcess, Symbol);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SymGetSymPrev64 (DBGHELP.@)
 */
BOOL WINAPI SymGetSymPrev64(HANDLE hProcess, PIMAGEHLP_SYMBOL64 Symbol)
{
    FIXME("(%p, %p): stub\n", hProcess, Symbol);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SymGetSymPrev (DBGHELP.@)
 */
BOOL WINAPI SymGetSymPrev(HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
    FIXME("(%p, %p): stub\n", hProcess, Symbol);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *		SymGetLineFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddr(HANDLE hProcess, DWORD dwAddr,
                               PDWORD pdwDisplacement, PIMAGEHLP_LINE Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, FALSE);
    if (!get_line_from_addr(hProcess, dwAddr, pdwDisplacement, &line_info)) return FALSE;
    return lineinfo_copy_toA32(&line_info, Line);
}

/******************************************************************
 *		SymGetLineFromAddr64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddr64(HANDLE hProcess, DWORD64 dwAddr, 
                                 PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, FALSE);
    if (!get_line_from_addr(hProcess, dwAddr, pdwDisplacement, &line_info)) return FALSE;
    return lineinfo_copy_toA64(&line_info, Line);
}

/******************************************************************
 *		SymGetLineFromAddrW64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddrW64(HANDLE hProcess, DWORD64 dwAddr, 
                                  PDWORD pdwDisplacement, PIMAGEHLP_LINEW64 Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, TRUE);
    if (!get_line_from_addr(hProcess, dwAddr, pdwDisplacement, &line_info)) return FALSE;
    return lineinfo_copy_toW64(&line_info, Line);
}

static BOOL symt_get_func_line_prev(HANDLE hProcess, struct lineinfo_t* line_info, void* key, DWORD64 addr)
{
    struct module_pair  pair;
    struct line_info*   li;
    struct line_info*   srcli;

    if (!module_init_pair(&pair, hProcess, addr)) return FALSE;

    if (key == NULL) return FALSE;

    li = key;

    while (!li->is_first)
    {
        li--;
        if (!li->is_source_file)
        {
            line_info->line_number = li->line_number;
            line_info->address     = li->u.address;
            line_info->key         = li;
            /* search source file */
            for (srcli = li; !srcli->is_source_file; srcli--);

            return lineinfo_set_nameA(pair.pcs, line_info, (char*)source_get(pair.effective, srcli->u.source_file));
        }
    }
    SetLastError(ERROR_NO_MORE_ITEMS); /* FIXME */
    return FALSE;
}

/******************************************************************
 *             SymGetLinePrev64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrev64(HANDLE hProcess, PIMAGEHLP_LINE64 Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, FALSE);
    if (!symt_get_func_line_prev(hProcess, &line_info, Line->Key, Line->Address)) return FALSE;
    return lineinfo_copy_toA64(&line_info, Line);
}

/******************************************************************
 *             SymGetLinePrev (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrev(HANDLE hProcess, PIMAGEHLP_LINE Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, FALSE);
    if (!symt_get_func_line_prev(hProcess, &line_info, Line->Key, Line->Address)) return FALSE;
    return lineinfo_copy_toA32(&line_info, Line);
}

/******************************************************************
 *             SymGetLinePrevW64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrevW64(HANDLE hProcess, PIMAGEHLP_LINEW64 Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, TRUE);
    if (!symt_get_func_line_prev(hProcess, &line_info, Line->Key, Line->Address)) return FALSE;
    return lineinfo_copy_toW64(&line_info, Line);
}

static BOOL symt_get_func_line_next(HANDLE hProcess, struct lineinfo_t* line_info, void* key, DWORD64 addr)
{
    struct module_pair  pair;
    struct line_info*   li;
    struct line_info*   srcli;

    if (key == NULL) return FALSE;
    if (!module_init_pair(&pair, hProcess, addr)) return FALSE;

    /* search current source file */
    for (srcli = key; !srcli->is_source_file; srcli--);

    li = key;
    while (!li->is_last)
    {
        li++;
        if (!li->is_source_file)
        {
            line_info->line_number = li->line_number;
            line_info->address     = li->u.address;
            line_info->key         = li;
            return lineinfo_set_nameA(pair.pcs, line_info, (char*)source_get(pair.effective, srcli->u.source_file));
        }
        srcli = li;
    }
    SetLastError(ERROR_NO_MORE_ITEMS); /* FIXME */
    return FALSE;
}

/******************************************************************
 *		SymGetLineNext64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineNext64(HANDLE hProcess, PIMAGEHLP_LINE64 Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, FALSE);
    if (!symt_get_func_line_next(hProcess, &line_info, Line->Key, Line->Address)) return FALSE;
    return lineinfo_copy_toA64(&line_info, Line);
}

/******************************************************************
 *		SymGetLineNext (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineNext(HANDLE hProcess, PIMAGEHLP_LINE Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, FALSE);
    if (!symt_get_func_line_next(hProcess, &line_info, Line->Key, Line->Address)) return FALSE;
    return lineinfo_copy_toA32(&line_info, Line);
}

/******************************************************************
 *		SymGetLineNextW64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineNextW64(HANDLE hProcess, PIMAGEHLP_LINEW64 Line)
{
    struct lineinfo_t line_info;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    init_lineinfo(&line_info, TRUE);
    if (!symt_get_func_line_next(hProcess, &line_info, Line->Key, Line->Address)) return FALSE;
    return lineinfo_copy_toW64(&line_info, Line);
}

/***********************************************************************
 *		SymUnDName (DBGHELP.@)
 */
BOOL WINAPI SymUnDName(PIMAGEHLP_SYMBOL sym, PSTR UnDecName, DWORD UnDecNameLength)
{
    return UnDecorateSymbolName(sym->Name, UnDecName, UnDecNameLength,
                                UNDNAME_COMPLETE) != 0;
}

/***********************************************************************
 *		SymUnDName64 (DBGHELP.@)
 */
BOOL WINAPI SymUnDName64(PIMAGEHLP_SYMBOL64 sym, PSTR UnDecName, DWORD UnDecNameLength)
{
    return UnDecorateSymbolName(sym->Name, UnDecName, UnDecNameLength,
                                UNDNAME_COMPLETE) != 0;
}

/***********************************************************************
 *		UnDecorateSymbolName (DBGHELP.@)
 */
DWORD WINAPI UnDecorateSymbolName(const char *decorated_name, char *undecorated_name,
                                  DWORD undecorated_length, DWORD flags)
{
    TRACE("(%s, %p, %ld, 0x%08lx)\n",
          debugstr_a(decorated_name), undecorated_name, undecorated_length, flags);

    if (!undecorated_name || !undecorated_length)
        return 0;
    if (!__unDName(undecorated_name, decorated_name, undecorated_length, malloc, free, flags))
        return 0;
    return strlen(undecorated_name);
}

/***********************************************************************
 *		UnDecorateSymbolNameW (DBGHELP.@)
 */
DWORD WINAPI UnDecorateSymbolNameW(const WCHAR *decorated_name, WCHAR *undecorated_name,
                                   DWORD undecorated_length, DWORD flags)
{
    char *buf, *ptr;
    int len, ret = 0;

    TRACE("(%s, %p, %ld, 0x%08lx)\n",
          debugstr_w(decorated_name), undecorated_name, undecorated_length, flags);

    if (!undecorated_name || !undecorated_length)
        return 0;

    len = WideCharToMultiByte(CP_ACP, 0, decorated_name, -1, NULL, 0, NULL, NULL);
    if ((buf = HeapAlloc(GetProcessHeap(), 0, len)))
    {
        WideCharToMultiByte(CP_ACP, 0, decorated_name, -1, buf, len, NULL, NULL);
        if ((ptr = __unDName(NULL, buf, 0, malloc, free, flags)))
        {
            MultiByteToWideChar(CP_ACP, 0, ptr, -1, undecorated_name, undecorated_length);
            undecorated_name[undecorated_length - 1] = 0;
            ret = lstrlenW(undecorated_name);
            free(ptr);
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }

    return ret;
}

#define WILDCHAR(x)      (-(x))

static  int     re_fetch_char(const WCHAR** re)
{
    switch (**re)
    {
    case '\\': (*re)++; return *(*re)++;
    case '*': case '[': case '?': case '+': case '#': case ']': return WILDCHAR(*(*re)++);
    default: return *(*re)++;
    }
}

static inline int  re_match_char(WCHAR ch1, WCHAR ch2, BOOL _case)
{
    return _case ? ch1 - ch2 : towupper(ch1) - towupper(ch2);
}

static const WCHAR* re_match_one(const WCHAR* string, const WCHAR* elt, BOOL _case)
{
    int         ch1, prev = 0;
    unsigned    state = 0;

    switch (ch1 = re_fetch_char(&elt))
    {
    default:
        return (ch1 >= 0 && re_match_char(*string, ch1, _case) == 0) ? ++string : NULL;
    case WILDCHAR('?'): return *string ? ++string : NULL;
    case WILDCHAR('*'): assert(0);
    case WILDCHAR('['): break;
    }

    for (;;)
    {
        ch1 = re_fetch_char(&elt);
        if (ch1 == WILDCHAR(']')) return NULL;
        if (state == 1 && ch1 == '-') state = 2;
        else
        {
            if (re_match_char(*string, ch1, _case) == 0) return ++string;
            switch (state)
            {
            case 0:
                state = 1;
                prev = ch1;
                break;
            case 1:
                state = 0;
                break;
            case 2:
                if (prev >= 0 && ch1 >= 0 && re_match_char(prev, *string, _case) <= 0 &&
                    re_match_char(*string, ch1, _case) <= 0)
                    return ++string;
                state = 0;
                break;
            }
        }
    }
}

/******************************************************************
 *		re_match_multi
 *
 * match a substring of *pstring according to *pre regular expression
 * pstring and pre are only updated in case of successful match
 */
static BOOL re_match_multi(const WCHAR** pstring, const WCHAR** pre, BOOL _case)
{
    const WCHAR* re_end = *pre;
    const WCHAR* string_end = *pstring;
    const WCHAR* re_beg;
    const WCHAR* string_beg;
    const WCHAR* next;
    int          ch;

    while (*re_end && *string_end)
    {
        string_beg = string_end;
        re_beg = re_end;
        switch (ch = re_fetch_char(&re_end))
        {
        case WILDCHAR(']'): case WILDCHAR('+'): case WILDCHAR('#'): return FALSE;
        case WILDCHAR('*'):
            /* transform '*' into '?#' */
            re_beg = L"?";
            goto closure;
        case WILDCHAR('['):
            do
            {
                if (!(ch = re_fetch_char(&re_end))) return FALSE;
            } while (ch != WILDCHAR(']'));
            /* fall through */
        case WILDCHAR('?'):
        default:
            break;
        }

        switch (*re_end)
        {
        case '+':
            if (!(next = re_match_one(string_end, re_beg, _case))) return FALSE;
            string_beg++;
            /* fall through */
        case '#':
            re_end++;
        closure:
            while ((next = re_match_one(string_end, re_beg, _case))) string_end = next;
            for ( ; string_end >= string_beg; string_end--)
            {
                if (re_match_multi(&string_end, &re_end, _case)) goto found;
            }
            return FALSE;
        default:
            if (!(next = re_match_one(string_end, re_beg, _case))) return FALSE;
            string_end = next;
        }
    }

    if (*re_end || *string_end) return FALSE;

found:
    *pre = re_end;
    *pstring = string_end;
    return TRUE;
}

BOOL symt_match_stringAW(const char *string, const WCHAR *re, BOOL _case)
{
    WCHAR*      strW;
    BOOL        ret = FALSE;
    DWORD       sz;

    sz = MultiByteToWideChar(CP_ACP, 0, string, -1, NULL, 0);
    if ((strW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
    {
        MultiByteToWideChar(CP_ACP, 0, string, -1, strW, sz);
        ret = SymMatchStringW(strW, re, _case);
        HeapFree(GetProcessHeap(), 0, strW);
    }
    return ret;
}

/******************************************************************
 *		SymMatchStringA (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchStringA(PCSTR string, PCSTR re, BOOL _case)
{
    WCHAR*      reW;
    BOOL        ret = FALSE;
    DWORD       sz;

    if (!string || !re)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    TRACE("%s %s %c\n", debugstr_a(string), debugstr_a(re), _case ? 'Y' : 'N');

    sz = MultiByteToWideChar(CP_ACP, 0, re, -1, NULL, 0);
    if ((reW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
    {
        MultiByteToWideChar(CP_ACP, 0, re, -1, reW, sz);
        ret = symt_match_stringAW(string, reW, _case);
        HeapFree(GetProcessHeap(), 0, reW);
    }
    return ret;
}

/******************************************************************
 *		SymMatchStringW (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchStringW(PCWSTR string, PCWSTR re, BOOL _case)
{
    TRACE("%s %s %c\n", debugstr_w(string), debugstr_w(re), _case ? 'Y' : 'N');

    if (!string || !re)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    return re_match_multi(&string, &re, _case);
}

static inline BOOL doSymSearch(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index,
                               DWORD SymTag, PCWSTR Mask, DWORD64 Address,
                               PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                               PVOID UserContext, DWORD Options)
{
    struct sym_enum     se;

    if (Options != SYMSEARCH_GLOBALSONLY)
    {
        FIXME("Unsupported searching with options (%lx)\n", Options);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    se.cb = EnumSymbolsCallback;
    se.user = UserContext;
    se.index = Index;
    se.tag = SymTag;
    se.addr = Address;
    se.sym_info = (PSYMBOL_INFO)se.buffer;

    return sym_enum(hProcess, BaseOfDll, Mask, &se);
}

/******************************************************************
 *		SymSearch (DBGHELP.@)
 */
BOOL WINAPI SymSearch(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index,
                      DWORD SymTag, PCSTR Mask, DWORD64 Address,
                      PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                      PVOID UserContext, DWORD Options)
{
    LPWSTR      maskW = NULL;
    BOOLEAN     ret;

    TRACE("(%p %I64x %lu %lu %s %I64x %p %p %lx)\n",
          hProcess, BaseOfDll, Index, SymTag, debugstr_a(Mask),
          Address, EnumSymbolsCallback, UserContext, Options);

    if (Mask)
    {
        DWORD sz = MultiByteToWideChar(CP_ACP, 0, Mask, -1, NULL, 0);

        if (!(maskW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, Mask, -1, maskW, sz);
    }
    ret = doSymSearch(hProcess, BaseOfDll, Index, SymTag, maskW, Address,
                      EnumSymbolsCallback, UserContext, Options);
    HeapFree(GetProcessHeap(), 0, maskW);
    return ret;
}

/******************************************************************
 *		SymSearchW (DBGHELP.@)
 */
BOOL WINAPI SymSearchW(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index,
                       DWORD SymTag, PCWSTR Mask, DWORD64 Address,
                       PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
                       PVOID UserContext, DWORD Options)
{
    struct sym_enumW    sew;

    TRACE("(%p %I64x %lu %lu %s %I64x %p %p %lx)\n",
          hProcess, BaseOfDll, Index, SymTag, debugstr_w(Mask),
          Address, EnumSymbolsCallback, UserContext, Options);

    sew.ctx = UserContext;
    sew.cb = EnumSymbolsCallback;
    sew.sym_info = (PSYMBOL_INFOW)sew.buffer;

    return doSymSearch(hProcess, BaseOfDll, Index, SymTag, Mask, Address,
                       sym_enumW, &sew, Options);
}

/******************************************************************
 *		SymAddSymbol (DBGHELP.@)
 *
 */
BOOL WINAPI SymAddSymbol(HANDLE hProcess, ULONG64 BaseOfDll, PCSTR name,
                         DWORD64 addr, DWORD size, DWORD flags)
{
    struct module_pair  pair;

    TRACE("(%p %s %I64x %lu)\n", hProcess, debugstr_a(name), addr, size);

    if (!module_init_pair(&pair, hProcess, BaseOfDll)) return FALSE;

    return symt_new_custom(pair.effective, name, addr, size) != NULL;
}

/******************************************************************
 *		SymAddSymbolW (DBGHELP.@)
 *
 */
BOOL WINAPI SymAddSymbolW(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR nameW,
                          DWORD64 addr, DWORD size, DWORD flags)
{
    char       name[MAX_SYM_NAME];

    TRACE("(%p %s %I64x %lu)\n", hProcess, debugstr_w(nameW), addr, size);

    WideCharToMultiByte(CP_ACP, 0, nameW, -1, name, ARRAY_SIZE(name), NULL, NULL);

    return SymAddSymbol(hProcess, BaseOfDll, name, addr, size, flags);
}

/******************************************************************
 *		SymEnumLines (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumLines(HANDLE hProcess, ULONG64 base, PCSTR compiland,
                         PCSTR srcfile, PSYM_ENUMLINES_CALLBACK cb, PVOID user)
{
    struct module_pair          pair;
    struct hash_table_iter      hti;
    struct symt_ht*             sym;
    WCHAR*                      srcmask;
    struct line_info*           dli;
    void*                       ptr;
    SRCCODEINFO                 sci;
    const char*                 file;

    if (!cb) return FALSE;
    if (!(dbghelp_options & SYMOPT_LOAD_LINES)) return TRUE;

    if (!module_init_pair(&pair, hProcess, base)) return FALSE;
    if (compiland) FIXME("Unsupported yet (filtering on compiland %s)\n", debugstr_a(compiland));
    if (!(srcmask = file_regex(srcfile))) return FALSE;

    sci.SizeOfStruct = sizeof(sci);
    sci.ModBase      = base;

    hash_table_iter_init(&pair.effective->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        unsigned int    i;

        sym = CONTAINING_RECORD(ptr, struct symt_ht, hash_elt);
        if (sym->symt.tag != SymTagFunction) continue;

        sci.FileName[0] = '\0';
        for (i=0; i<vector_length(&((struct symt_function*)sym)->vlines); i++)
        {
            dli = vector_at(&((struct symt_function*)sym)->vlines, i);
            if (dli->is_source_file)
            {
                file = source_get(pair.effective, dli->u.source_file);
                if (file && symt_match_stringAW(file, srcmask, FALSE))
                    strcpy(sci.FileName, file);
                else
                    sci.FileName[0] = '\0';
            }
            else if (sci.FileName[0])
            {
                sci.Key = dli;
                sci.Obj[0] = '\0'; /* FIXME */
                sci.LineNumber = dli->line_number;
                sci.Address = dli->u.address;
                if (!cb(&sci, user)) break;
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, srcmask);
    return TRUE;
}

BOOL WINAPI SymGetLineFromName(HANDLE hProcess, PCSTR ModuleName, PCSTR FileName,
                DWORD dwLineNumber, PLONG plDisplacement, PIMAGEHLP_LINE Line)
{
    FIXME("(%p) (%s, %s, %ld %p %p): stub\n", hProcess, debugstr_a(ModuleName), debugstr_a(FileName),
                dwLineNumber, plDisplacement, Line);
    return FALSE;
}

BOOL WINAPI SymGetLineFromName64(HANDLE hProcess, PCSTR ModuleName, PCSTR FileName,
                DWORD dwLineNumber, PLONG lpDisplacement, PIMAGEHLP_LINE64 Line)
{
    FIXME("(%p) (%s, %s, %ld %p %p): stub\n", hProcess, debugstr_a(ModuleName), debugstr_a(FileName),
                dwLineNumber, lpDisplacement, Line);
    return FALSE;
}

BOOL WINAPI SymGetLineFromNameW64(HANDLE hProcess, PCWSTR ModuleName, PCWSTR FileName,
                DWORD dwLineNumber, PLONG plDisplacement, PIMAGEHLP_LINEW64 Line)
{
    FIXME("(%p) (%s, %s, %ld %p %p): stub\n", hProcess, debugstr_w(ModuleName), debugstr_w(FileName),
                dwLineNumber, plDisplacement, Line);
    return FALSE;
}

/******************************************************************
 *		SymFromIndex (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromIndex(HANDLE hProcess, ULONG64 BaseOfDll, DWORD index, PSYMBOL_INFO symbol)
{
    struct module_pair  pair;
    struct symt*        sym;

    TRACE("hProcess = %p, BaseOfDll = %I64x, index = %ld, symbol = %p\n",
          hProcess, BaseOfDll, index, symbol);

    if (!module_init_pair(&pair, hProcess, BaseOfDll)) return FALSE;
    if ((sym = symt_index2ptr(pair.effective, index)) == NULL) return FALSE;
    symt_fill_sym_info(&pair, NULL, sym, symbol);
    return TRUE;
}

/******************************************************************
 *		SymFromIndexW (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromIndexW(HANDLE hProcess, ULONG64 BaseOfDll, DWORD index, PSYMBOL_INFOW symbol)
{
    PSYMBOL_INFO        si;
    BOOL                ret;

    TRACE("hProcess = %p, BaseOfDll = %I64x, index = %ld, symbol = %p\n",
          hProcess, BaseOfDll, index, symbol);

    si = HeapAlloc(GetProcessHeap(), 0, sizeof(*si) + symbol->MaxNameLen * sizeof(WCHAR));
    if (!si) return FALSE;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = symbol->MaxNameLen;
    if ((ret = SymFromIndex(hProcess, BaseOfDll, index, si)))
        copy_symbolW(symbol, si);
    HeapFree(GetProcessHeap(), 0, si);
    return ret;
}

/******************************************************************
 *		SymSetHomeDirectory (DBGHELP.@)
 *
 */
PCHAR WINAPI SymSetHomeDirectory(HANDLE hProcess, PCSTR dir)
{
    FIXME("(%p, %s): stub\n", hProcess, debugstr_a(dir));

    return NULL;
}

/******************************************************************
 *		SymSetHomeDirectoryW (DBGHELP.@)
 *
 */
PWSTR WINAPI SymSetHomeDirectoryW(HANDLE hProcess, PCWSTR dir)
{
    FIXME("(%p, %s): stub\n", hProcess, debugstr_w(dir));

    return NULL;
}

/******************************************************************
 *		SymFromInlineContext (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromInlineContext(HANDLE hProcess, DWORD64 addr, ULONG inline_ctx, PDWORD64 disp, PSYMBOL_INFO si)
{
    struct module_pair pair;
    struct symt_function* inlined;

    TRACE("(%p, %#I64x, 0x%lx, %p, %p)\n", hProcess, addr, inline_ctx, disp, si);

    switch (IFC_MODE(inline_ctx))
    {
    case IFC_MODE_INLINE:
        if (!module_init_pair(&pair, hProcess, addr)) return FALSE;
        inlined = symt_find_inlined_site(pair.effective, addr, inline_ctx);
        if (inlined)
        {
            symt_fill_sym_info(&pair, NULL, &inlined->symt, si);
            if (disp) *disp = addr - inlined->ranges[0].low;
            return TRUE;
        }
        /* fall through */
    case IFC_MODE_IGNORE:
    case IFC_MODE_REGULAR:
        return SymFromAddr(hProcess, addr, disp, si);
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}

/******************************************************************
 *		SymFromInlineContextW (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromInlineContextW(HANDLE hProcess, DWORD64 addr, ULONG inline_ctx, PDWORD64 disp, PSYMBOL_INFOW siW)
{
    PSYMBOL_INFO        si;
    unsigned            len;
    BOOL                ret;

    TRACE("(%p, %#I64x, 0x%lx, %p, %p)\n", hProcess, addr, inline_ctx, disp, siW);

    len = sizeof(*si) + siW->MaxNameLen * sizeof(WCHAR);
    si = HeapAlloc(GetProcessHeap(), 0, len);
    if (!si) return FALSE;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = siW->MaxNameLen;
    if ((ret = SymFromInlineContext(hProcess, addr, inline_ctx, disp, si)))
    {
        copy_symbolW(siW, si);
    }
    HeapFree(GetProcessHeap(), 0, si);
    return ret;
}

static BOOL get_line_from_inline_context(HANDLE hProcess, DWORD64 addr, ULONG inline_ctx, DWORD64 mod_addr, PDWORD disp,
                                         struct lineinfo_t* line_info)
{
    struct module_pair pair;
    struct symt_function* inlined;

    if (!module_init_pair(&pair, hProcess, mod_addr ? mod_addr : addr)) return FALSE;
    switch (IFC_MODE(inline_ctx))
    {
    case IFC_MODE_INLINE:
        inlined = symt_find_inlined_site(pair.effective, addr, inline_ctx);
        if (inlined && get_line_from_function(&pair, inlined, addr, disp, line_info))
            return TRUE;
        /* fall through: check if we can find line info at top function level */
    case IFC_MODE_IGNORE:
    case IFC_MODE_REGULAR:
        return get_line_from_addr(hProcess, addr, disp, line_info);
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}

/******************************************************************
 *		SymGetLineFromInlineContext (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromInlineContext(HANDLE hProcess, DWORD64 addr, ULONG inline_ctx, DWORD64 mod_addr, PDWORD disp, PIMAGEHLP_LINE64 line)
{
    struct lineinfo_t line_info;

    TRACE("(%p, %#I64x, 0x%lx, %#I64x, %p, %p)\n",
          hProcess, addr, inline_ctx, mod_addr, disp, line);

    if (line->SizeOfStruct < sizeof(*line)) return FALSE;
    init_lineinfo(&line_info, FALSE);

    if (!get_line_from_inline_context(hProcess, addr, inline_ctx, mod_addr, disp, &line_info)) return FALSE;
    return lineinfo_copy_toA64(&line_info, line);
}

/******************************************************************
 *		SymGetLineFromInlineContextW (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromInlineContextW(HANDLE hProcess, DWORD64 addr, ULONG inline_ctx, DWORD64 mod_addr, PDWORD disp, PIMAGEHLP_LINEW64 line)
{
    struct lineinfo_t line_info;

    TRACE("(%p, %#I64x, 0x%lx, %#I64x, %p, %p)\n",
          hProcess, addr, inline_ctx, mod_addr, disp, line);

    if (line->SizeOfStruct < sizeof(*line)) return FALSE;
    init_lineinfo(&line_info, TRUE);

    if (!get_line_from_inline_context(hProcess, addr, inline_ctx, mod_addr, disp, &line_info)) return FALSE;
    return lineinfo_copy_toW64(&line_info, line);
}

/******************************************************************
 *		SymAddrIncludeInlineTrace (DBGHELP.@)
 *
 * MSDN doesn't state that the maximum depth (of embedded inline sites) at <addr>
 * is actually returned. (It just says non zero means that there are some inline site(s)).
 * But this is what native actually returns.
 */
DWORD WINAPI SymAddrIncludeInlineTrace(HANDLE hProcess, DWORD64 addr)
{
    struct module_pair pair;
    DWORD depth = 0;

    TRACE("(%p, %#I64x)\n", hProcess, addr);

    if (module_init_pair(&pair, hProcess, addr))
    {
        struct symt_ht* symt = symt_find_symbol_at(pair.effective, addr);
        if (symt_check_tag(&symt->symt, SymTagFunction))
        {
            struct symt_function* inlined = symt_find_lowest_inlined((struct symt_function*)symt, addr);
            if (inlined)
            {
                for ( ; &inlined->symt != &symt->symt; inlined = (struct symt_function*)symt_get_upper_inlined(inlined))
                    ++depth;
            }
        }
    }
    return depth;
}

/******************************************************************
 *		SymQueryInlineTrace (DBGHELP.@)
 *
 */
BOOL WINAPI SymQueryInlineTrace(HANDLE hProcess, DWORD64 StartAddress, DWORD StartContext,
                                DWORD64 StartRetAddress, DWORD64 CurAddress,
                                LPDWORD CurContext, LPDWORD CurFrameIndex)
{
    struct module_pair  pair;
    struct symt_ht*     sym_curr;
    struct symt_ht*     sym_start;
    struct symt_ht*     sym_startret;
    DWORD               depth;

    TRACE("(%p, %#I64x, 0x%lx, %#I64x, %I64x, %p, %p)\n",
          hProcess, StartAddress, StartContext, StartRetAddress, CurAddress, CurContext, CurFrameIndex);

    if (!module_init_pair(&pair, hProcess, CurAddress)) return FALSE;
    if (!(sym_curr = symt_find_symbol_at(pair.effective, CurAddress))) return FALSE;
    if (!symt_check_tag(&sym_curr->symt, SymTagFunction)) return FALSE;

    sym_start = symt_find_symbol_at(pair.effective, StartAddress);
    sym_startret = symt_find_symbol_at(pair.effective, StartRetAddress);
    if (sym_start != sym_curr && sym_startret != sym_curr)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (sym_start != sym_curr || StartContext)
    {
        FIXME("(%p, %#I64x, 0x%lx, %#I64x, %I64x, %p, %p): semi-stub\n",
              hProcess, StartAddress, StartContext, StartRetAddress, CurAddress, CurContext, CurFrameIndex);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    depth = SymAddrIncludeInlineTrace(hProcess, CurAddress);
    if (depth)
    {
        *CurContext = IFC_MODE_INLINE; /* deepest inline site */
        *CurFrameIndex = depth;
    }
    else
    {
        *CurContext = IFC_MODE_REGULAR;
        *CurFrameIndex = 0;
    }
    return TRUE;
}
