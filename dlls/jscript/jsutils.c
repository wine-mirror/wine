/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

const char *debugstr_variant(const VARIANT *v)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
        return wine_dbg_sprintf("{VT_EMPTY}");
    case VT_NULL:
        return wine_dbg_sprintf("{VT_NULL}");
    case VT_I4:
        return wine_dbg_sprintf("{VT_I4: %d}", V_I4(v));
    case VT_R8:
        return wine_dbg_sprintf("{VT_R8: %lf}", V_R8(v));
    case VT_BSTR:
        return wine_dbg_sprintf("{VT_BSTR: %s}", debugstr_w(V_BSTR(v)));
    case VT_DISPATCH:
        return wine_dbg_sprintf("{VT_DISPATCH: %p}", V_DISPATCH(v));
    case VT_BOOL:
        return wine_dbg_sprintf("{VT_BOOL: %x}", V_BOOL(v));
    default:
        return wine_dbg_sprintf("{vt %d}", V_VT(v));
    }
}

#define MIN_BLOCK_SIZE  128

static inline DWORD block_size(DWORD block)
{
    return MIN_BLOCK_SIZE << block;
}

void jsheap_init(jsheap_t *heap)
{
    memset(heap, 0, sizeof(*heap));
    list_init(&heap->custom_blocks);
}

void *jsheap_alloc(jsheap_t *heap, DWORD size)
{
    struct list *list;
    void *tmp;

    if(!heap->block_cnt) {
        if(!heap->blocks) {
            heap->blocks = heap_alloc(sizeof(void*));
            if(!heap->blocks)
                return NULL;
        }

        tmp = heap_alloc(block_size(0));
        if(!tmp)
            return NULL;

        heap->blocks[0] = tmp;
        heap->block_cnt = 1;
    }

    if(heap->offset + size < block_size(heap->last_block)) {
        tmp = ((BYTE*)heap->blocks[heap->last_block])+heap->offset;
        heap->offset += size;
        return tmp;
    }

    if(size < block_size(heap->last_block+1)) {
        if(heap->last_block+1 == heap->block_cnt) {
            tmp = heap_realloc(heap->blocks, (heap->block_cnt+1)*sizeof(void*));
            if(!tmp)
                return NULL;
            heap->blocks = tmp;
        }

        tmp = heap_alloc(block_size(heap->block_cnt+1));
        if(!tmp)
            return NULL;

        heap->blocks[heap->block_cnt++] = tmp;

        heap->last_block++;
        heap->offset = size;
        return heap->blocks[heap->last_block];
    }

    list = heap_alloc(size + sizeof(struct list));
    if(!list)
        return NULL;

    list_add_head(&heap->custom_blocks, list);
    return list+1;
}

void jsheap_clear(jsheap_t *heap)
{
    struct list *tmp;

    while((tmp = list_next(&heap->custom_blocks, &heap->custom_blocks))) {
        list_remove(tmp);
        heap_free(tmp);
    }
}

void jsheap_free(jsheap_t *heap)
{
    DWORD i;

    jsheap_clear(heap);

    for(i=0; i < heap->block_cnt; i++)
        heap_free(heap->blocks[i]);
    heap_free(heap->blocks);

    jsheap_init(heap);
}
