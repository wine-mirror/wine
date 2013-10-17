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
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(context);

void *Context_CreateDataContext(size_t contextSize, const context_vtbl_t *vtbl)
{
    context_t *context;

    context = CryptMemAlloc(sizeof(context_t) + contextSize);
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
    return context_ptr(context);
}

context_t *Context_CreateLinkContext(unsigned int contextSize, context_t *linked)
{
    context_t *context;

    TRACE("(%d, %p)\n", contextSize, linked);

    context = CryptMemAlloc(sizeof(context_t) + contextSize);
    if (!context)
        return NULL;

    memcpy(context_ptr(context), context_ptr(linked), contextSize);
    context->vtbl = linked->vtbl;
    context->ref = 1;
    context->linked = linked;
    context->properties = linked->properties;
    Context_AddRef(linked);

    TRACE("returning %p\n", context);
    return context;
}

void Context_AddRef(context_t *context)
{
    InterlockedIncrement(&context->ref);
    TRACE("(%p) ref=%d\n", context, context->ref);
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

    toProperties = context_from_ptr(to)->properties;
    fromProperties = context_from_ptr(from)->properties;
    assert(toProperties && fromProperties);
    ContextPropertyList_Copy(toProperties, fromProperties);
}
