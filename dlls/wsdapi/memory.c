/*
 * Web Services on Devices
 *
 * Copyright 2017 Owen Rudge for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wsdapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

#define MEMORY_ALLOCATION_MAGIC     0xB10C5EED

#define ALIGNMENT                   (2 * sizeof(void*))
#define ROUND_TO_ALIGNMENT(size)    (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
#define MEMORY_ALLOCATION_SIZE      ROUND_TO_ALIGNMENT(sizeof(struct memory_allocation))

struct memory_allocation
{
    int magic;
    struct list entry;

    struct list children;
};

static struct memory_allocation *find_allocation(void *ptr)
{
    struct memory_allocation *allocation;

    if (ptr == NULL)
    {
        return NULL;
    }

    allocation = (struct memory_allocation *)((char *)ptr - MEMORY_ALLOCATION_SIZE);

    if (allocation->magic != MEMORY_ALLOCATION_MAGIC)
    {
        return NULL;
    }

    return allocation;
}

static void free_allocation(struct memory_allocation *item)
{
    struct memory_allocation *child, *cursor;

    LIST_FOR_EACH_ENTRY_SAFE(child, cursor, &item->children, struct memory_allocation, entry)
    {
        free_allocation(child);
    }

    list_remove(&item->entry);
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory(&item->magic, sizeof(item->magic));
    free(item);
}

void * WINAPI WSDAllocateLinkedMemory(void *pParent, SIZE_T cbSize)
{
    struct memory_allocation *allocation, *parent;
    void *ptr;

    TRACE("(%p, %Iu)\n", pParent, cbSize);

    ptr = malloc(MEMORY_ALLOCATION_SIZE + cbSize);

    if (ptr == NULL)
    {
        return NULL;
    }

    allocation = ptr;
    allocation->magic = MEMORY_ALLOCATION_MAGIC;

    list_init(&allocation->children);

    /* See if we have a parent */
    parent = find_allocation(pParent);

    if (parent != NULL)
    {
        list_add_tail(&parent->children, &allocation->entry);
    }
    else
    {
        list_init(&allocation->entry);
    }

    return (char *)ptr + MEMORY_ALLOCATION_SIZE;
}

void WINAPI WSDAttachLinkedMemory(void *pParent, void *pChild)
{
    struct memory_allocation *parent, *child;

    TRACE("(%p, %p)\n", pParent, pChild);

    child = find_allocation(pChild);
    parent = find_allocation(pParent);

    TRACE("child: %p, parent: %p\n", child, parent);

    if ((child == NULL) || (parent == NULL))
    {
        return;
    }

    list_remove(&child->entry);
    list_add_tail(&parent->children, &child->entry);
}

void WINAPI WSDDetachLinkedMemory(void *pVoid)
{
    struct memory_allocation *allocation;

    TRACE("(%p)\n", pVoid);

    allocation = find_allocation(pVoid);

    if (allocation == NULL)
    {
        TRACE("Memory allocation not found\n");
        return;
    }

    list_remove(&allocation->entry);
}

void WINAPI WSDFreeLinkedMemory(void *pVoid)
{
    struct memory_allocation *allocation;

    TRACE("(%p)\n", pVoid);

    allocation = find_allocation(pVoid);

    if (allocation == NULL)
    {
        TRACE("Memory allocation not found\n");
        return;
    }

    free_allocation(allocation);
}
