/*
 * File source.c - source files management
 *
 * Copyright (C) 2004,      Eric Pouech.
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
 *
 */
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static struct module*   rb_module;
struct source_rb
{
    struct wine_rb_entry        entry;
    unsigned                    source;
};

static void *source_rb_alloc(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static void *source_rb_realloc(void *ptr, size_t size)
{
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

static void source_rb_free(void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

static int source_rb_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct source_rb *t = WINE_RB_ENTRY_VALUE(entry, const struct source_rb, entry);

    return strcmp((const char*)key, rb_module->sources + t->source);
}

const struct wine_rb_functions source_rb_functions =
{
    source_rb_alloc,
    source_rb_realloc,
    source_rb_free,
    source_rb_compare,
};

/******************************************************************
 *		source_find
 *
 * check whether a source file has already been stored
 */
static unsigned source_find(const char* name)
{
    struct wine_rb_entry*       e;

    e = wine_rb_get(&rb_module->sources_offsets_tree, name);
    if (!e) return -1;
    return WINE_RB_ENTRY_VALUE(e, struct source_rb, entry)->source;
}

/******************************************************************
 *		source_new
 *
 * checks if source exists. if not, add it
 */
unsigned source_new(struct module* module, const char* base, const char* name)
{
    unsigned    ret = -1;
    const char* full;
    char*       tmp = NULL;

    if (!name) return ret;
    if (!base || *name == '/')
        full = name;
    else
    {
        unsigned bsz = strlen(base);

        tmp = HeapAlloc(GetProcessHeap(), 0, bsz + 1 + strlen(name) + 1);
        if (!tmp) return ret;
        full = tmp;
        strcpy(tmp, base);
        if (tmp[bsz - 1] != '/') tmp[bsz++] = '/';
        strcpy(&tmp[bsz], name);
    }
    rb_module = module;
    if (!module->sources || (ret = source_find(full)) == (unsigned)-1)
    {
        char* new;
        int len = strlen(full) + 1;
        struct source_rb* rb;

        if (module->sources_used + len + 1 > module->sources_alloc)
        {
            if (!module->sources)
            {
                module->sources_alloc = (module->sources_used + len + 1 + 255) & ~255;
                new = HeapAlloc(GetProcessHeap(), 0, module->sources_alloc);
            }
            else
            {
                module->sources_alloc = max( module->sources_alloc * 2,
                                             (module->sources_used + len + 1 + 255) & ~255 );
                new = HeapReAlloc(GetProcessHeap(), 0, module->sources,
                                  module->sources_alloc);
            }
            if (!new) goto done;
            module->sources = new;
        }
        ret = module->sources_used;
        memcpy(module->sources + module->sources_used, full, len);
        module->sources_used += len;
        module->sources[module->sources_used] = '\0';
        if ((rb = pool_alloc(&module->pool, sizeof(*rb))))
        {
            rb->source = ret;
            wine_rb_put(&module->sources_offsets_tree, full, &rb->entry);
        }
    }
done:
    HeapFree(GetProcessHeap(), 0, tmp);
    return ret;
}

/******************************************************************
 *		source_get
 *
 * returns a stored source file name
 */
const char* source_get(const struct module* module, unsigned idx)
{
    if (idx == -1) return "";
    assert(module->sources);
    return module->sources + idx;
}

/******************************************************************
 *		SymEnumSourceFiles (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSourceFiles(HANDLE hProcess, ULONG64 ModBase, PCSTR Mask,
                               PSYM_ENUMSOURCEFILES_CALLBACK cbSrcFiles,
                               PVOID UserContext)
{
    struct module_pair  pair;
    SOURCEFILE          sf;
    char*               ptr;
    
    if (!cbSrcFiles) return FALSE;
    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
         
    if (ModBase)
    {
        pair.requested = module_find_by_addr(pair.pcs, ModBase, DMT_UNKNOWN);
        if (!module_get_debug(&pair)) return FALSE;
    }
    else
    {
        if (Mask[0] == '!')
        {
            pair.requested = module_find_by_nameA(pair.pcs, Mask + 1);
            if (!module_get_debug(&pair)) return FALSE;
        }
        else
        {
            FIXME("Unsupported yet (should get info from current context)\n");
            return FALSE;
        }
    }
    if (!pair.effective->sources) return FALSE;
    for (ptr = pair.effective->sources; *ptr; ptr += strlen(ptr) + 1)
    {
        /* FIXME: not using Mask */
        sf.ModBase = ModBase;
        sf.FileName = ptr;
        if (!cbSrcFiles(&sf, UserContext)) break;
    }

    return TRUE;
}

/******************************************************************
 *		SymGetSourceFileToken (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSourceFileToken(HANDLE hProcess, ULONG64 base,
                                  PCSTR src, PVOID* token, DWORD* size)
{
    FIXME("%p %s %s %p %p: stub!\n",
          hProcess, wine_dbgstr_longlong(base), debugstr_a(src), token, size);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/******************************************************************
 *		SymGetSourceFileTokenW (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSourceFileTokenW(HANDLE hProcess, ULONG64 base,
                                   PCWSTR src, PVOID* token, DWORD* size)
{
    FIXME("%p %s %s %p %p: stub!\n",
          hProcess, wine_dbgstr_longlong(base), debugstr_w(src), token, size);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
