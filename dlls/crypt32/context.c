/*
 * Copyright 2006 Juan Lang
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
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(context);

#define CONTEXT_FROM_BASE_CONTEXT(p) (void*)(p+1)
#define BASE_CONTEXT_FROM_CONTEXT(p) ((BASE_CONTEXT*)(p)-1)

void *Context_CreateDataContext(size_t contextSize, const context_vtbl_t *vtbl)
{
    BASE_CONTEXT *context;

    context = CryptMemAlloc(contextSize + sizeof(BASE_CONTEXT));
    if (!context)
        return NULL;

    context->vtbl = vtbl;
    context->ref = 1;
    context->linked = NULL;
    context->properties = ContextPropertyList_Create();
    if (!context->properties)
    {
        CryptMemFree(context);
        return NULL;
    }

    TRACE("returning %p\n", context);
    return CONTEXT_FROM_BASE_CONTEXT(context);
}

context_t *Context_CreateLinkContext(unsigned int contextSize, context_t *linked, unsigned int extra)
{
    context_t *context;

    TRACE("(%d, %p, %d)\n", contextSize, linked, extra);

    context = CryptMemAlloc(contextSize + sizeof(BASE_CONTEXT) + extra);
    if (!context)
        return NULL;

    memcpy(context_ptr(context), context_ptr(linked), contextSize);
    context->vtbl = linked->vtbl;
    context->ref = 1;
    context->linked = linked;
    Context_AddRef(linked);

    TRACE("returning %p\n", context);
    return context;
}

void Context_AddRef(context_t *context)
{
    InterlockedIncrement(&context->ref);
    TRACE("(%p) ref=%d\n", context, context->ref);
}

void *Context_GetExtra(const void *context, size_t contextSize)
{
    BASE_CONTEXT *baseContext = BASE_CONTEXT_FROM_CONTEXT(context);

    assert(baseContext->linked != NULL);
    return (LPBYTE)CONTEXT_FROM_BASE_CONTEXT(baseContext) + contextSize;
}

void *Context_GetLinkedContext(void *context)
{
    BASE_CONTEXT *baseContext = BASE_CONTEXT_FROM_CONTEXT(context);

    assert(baseContext->linked != NULL);
    return CONTEXT_FROM_BASE_CONTEXT(baseContext->linked);
}

CONTEXT_PROPERTY_LIST *Context_GetProperties(const void *context)
{
    BASE_CONTEXT *ptr = BASE_CONTEXT_FROM_CONTEXT(context);

    while (ptr && ptr->linked)
        ptr = ptr->linked;

    return ptr->properties;
}

BOOL Context_Release(context_t *context)
{
    BOOL ret = TRUE;

    if (context->ref <= 0)
    {
        ERR("%p's ref count is %d\n", context, context->ref);
        return FALSE;
    }
    if (InterlockedDecrement(&context->ref) == 0)
    {
        TRACE("freeing %p\n", context);
        if (!context->linked)
        {
            ContextPropertyList_Free(context->properties);
            context->vtbl->free(context);
        } else {
            Context_Release(context->linked);
        }
        CryptMemFree(context);
    }
    else
        TRACE("%p's ref count is %d\n", context, context->ref);
    return ret;
}

void Context_CopyProperties(const void *to, const void *from)
{
    CONTEXT_PROPERTY_LIST *toProperties, *fromProperties;

    toProperties = Context_GetProperties(to);
    fromProperties = Context_GetProperties(from);
    assert(toProperties && fromProperties);
    ContextPropertyList_Copy(toProperties, fromProperties);
}

struct ContextList
{
    const WINE_CONTEXT_INTERFACE *contextInterface;
    size_t contextSize;
    CRITICAL_SECTION cs;
    struct list contexts;
};

struct ContextList *ContextList_Create(
 const WINE_CONTEXT_INTERFACE *contextInterface, size_t contextSize)
{
    struct ContextList *list = CryptMemAlloc(sizeof(struct ContextList));

    if (list)
    {
        list->contextInterface = contextInterface;
        list->contextSize = contextSize;
        InitializeCriticalSection(&list->cs);
        list->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ContextList.cs");
        list_init(&list->contexts);
    }
    return list;
}

static inline struct list *ContextList_ContextToEntry(const struct ContextList *list,
 const void *context)
{
    struct list *ret;

    if (context)
        ret = Context_GetExtra(context, list->contextSize);
    else
        ret = NULL;
    return ret;
}

static inline void *ContextList_EntryToContext(const struct ContextList *list,
 struct list *entry)
{
    return (LPBYTE)entry - list->contextSize;
}

void *ContextList_Add(struct ContextList *list, void *toLink, void *toReplace)
{
    context_t *context;

    TRACE("(%p, %p, %p)\n", list, toLink, toReplace);

    context = Context_CreateLinkContext(list->contextSize, context_from_ptr(toLink), sizeof(struct list));
    if (context)
    {
        struct list *entry = ContextList_ContextToEntry(list, CONTEXT_FROM_BASE_CONTEXT(context));

        TRACE("adding %p\n", context);
        EnterCriticalSection(&list->cs);
        if (toReplace)
        {
            struct list *existing = ContextList_ContextToEntry(list, toReplace);

            entry->prev = existing->prev;
            entry->next = existing->next;
            entry->prev->next = entry;
            entry->next->prev = entry;
            existing->prev = existing->next = existing;
            Context_Release(context_from_ptr(toReplace));
        }
        else
            list_add_head(&list->contexts, entry);
        LeaveCriticalSection(&list->cs);
    }
    return CONTEXT_FROM_BASE_CONTEXT(context);
}

void *ContextList_Enum(struct ContextList *list, void *pPrev)
{
    struct list *listNext;
    void *ret;

    EnterCriticalSection(&list->cs);
    if (pPrev)
    {
        struct list *prevEntry = ContextList_ContextToEntry(list, pPrev);

        listNext = list_next(&list->contexts, prevEntry);
        Context_Release(context_from_ptr(pPrev));
    }
    else
        listNext = list_next(&list->contexts, &list->contexts);
    LeaveCriticalSection(&list->cs);

    if (listNext)
    {
        ret = ContextList_EntryToContext(list, listNext);
        Context_AddRef(context_from_ptr(ret));
    }
    else
        ret = NULL;
    return ret;
}

BOOL ContextList_Remove(struct ContextList *list, void *context)
{
    struct list *entry = ContextList_ContextToEntry(list, context);
    BOOL inList = FALSE;

    EnterCriticalSection(&list->cs);
    if (!list_empty(entry))
    {
        list_remove(entry);
        inList = TRUE;
    }
    LeaveCriticalSection(&list->cs);
    if (inList)
        list_init(entry);
    return inList;
}

static void ContextList_Empty(struct ContextList *list)
{
    struct list *entry, *next;

    EnterCriticalSection(&list->cs);
    LIST_FOR_EACH_SAFE(entry, next, &list->contexts)
    {
        const void *context = ContextList_EntryToContext(list, entry);

        TRACE("removing %p\n", context);
        list_remove(entry);
        Context_Release(context_from_ptr(context));
    }
    LeaveCriticalSection(&list->cs);
}

void ContextList_Free(struct ContextList *list)
{
    ContextList_Empty(list);
    list->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&list->cs);
    CryptMemFree(list);
}
