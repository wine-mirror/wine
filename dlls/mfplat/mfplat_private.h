/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#undef INITGUID
#include <guiddef.h>
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"

#include "wine/heap.h"

struct attribute
{
    GUID key;
    PROPVARIANT value;
};

typedef struct attributes
{
    IMFAttributes IMFAttributes_iface;
    LONG ref;
    CRITICAL_SECTION cs;
    struct attribute *attributes;
    size_t capacity;
    size_t count;
} mfattributes;

extern HRESULT init_attributes_object(struct attributes *object, UINT32 size) DECLSPEC_HIDDEN;
extern void clear_attributes_object(struct attributes *object) DECLSPEC_HIDDEN;

extern void init_system_queues(void) DECLSPEC_HIDDEN;
extern void shutdown_system_queues(void) DECLSPEC_HIDDEN;
extern BOOL is_platform_locked(void) DECLSPEC_HIDDEN;

static inline BOOL mf_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = heap_realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}
