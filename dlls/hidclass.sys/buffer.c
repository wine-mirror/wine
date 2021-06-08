/*  Implementation of a ring buffer for reports
 *
 * Copyright 2015 CodeWeavers, Aric Stewart
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
#include <stdlib.h>
#include "hid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

#define POINTER_UNUSED 0xffffffff
#define BASE_BUFFER_SIZE 32
#define MIN_BUFFER_SIZE 2
#define MAX_BUFFER_SIZE 512

struct ReportRingBuffer
{
    UINT start, end, size;

    UINT *pointers;
    UINT pointer_alloc;
    UINT buffer_size;

    CRITICAL_SECTION lock;

    BYTE *buffer;
};

struct ReportRingBuffer* RingBuffer_Create(UINT buffer_size)
{
    struct ReportRingBuffer *ring;
    int i;

    TRACE("Create Ring Buffer with buffer size %i\n",buffer_size);

    ring = malloc(sizeof(*ring));
    if (!ring)
        return NULL;
    ring->start = ring->end = 0;
    ring->size = BASE_BUFFER_SIZE;
    ring->buffer_size = buffer_size;
    ring->pointer_alloc = 2;
    ring->pointers = malloc(sizeof(UINT) * ring->pointer_alloc);
    if (!ring->pointers)
    {
        free(ring);
        return NULL;
    }
    for (i = 0; i < ring->pointer_alloc; i++)
        ring->pointers[i] = POINTER_UNUSED;
    ring->buffer = malloc(buffer_size * ring->size);
    if (!ring->buffer)
    {
        free(ring->pointers);
        free(ring);
        return NULL;
    }
    InitializeCriticalSection(&ring->lock);
    ring->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": RingBuffer.lock");
    return ring;
}

void RingBuffer_Destroy(struct ReportRingBuffer *ring)
{
    free(ring->buffer);
    free(ring->pointers);
    ring->lock.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&ring->lock);
    free(ring);
}

UINT RingBuffer_GetBufferSize(struct ReportRingBuffer *ring)
{
    return ring->buffer_size;
}

UINT RingBuffer_GetSize(struct ReportRingBuffer *ring)
{
    return ring->size;
}

NTSTATUS RingBuffer_SetSize(struct ReportRingBuffer *ring, UINT size)
{
    BYTE* new_buffer;
    int i;

    if (size < MIN_BUFFER_SIZE || size > MAX_BUFFER_SIZE)
        return STATUS_INVALID_PARAMETER;
    if (size == ring->size)
        return STATUS_SUCCESS;

    EnterCriticalSection(&ring->lock);
    ring->start = ring->end = 0;
    for (i = 0; i < ring->pointer_alloc; i++)
    {
        if (ring->pointers[i] != POINTER_UNUSED)
            ring->pointers[i] = 0;
    }
    new_buffer = malloc(ring->buffer_size * size);
    if (!new_buffer)
    {
        LeaveCriticalSection(&ring->lock);
        return STATUS_NO_MEMORY;
    }
    free(ring->buffer);
    ring->buffer = new_buffer;
    ring->size = size;
    LeaveCriticalSection(&ring->lock);
    return STATUS_SUCCESS;
}

void RingBuffer_ReadNew(struct ReportRingBuffer *ring, UINT index, void *output, UINT *size)
{
    void *ret = NULL;

    EnterCriticalSection(&ring->lock);
    if (index >= ring->pointer_alloc || ring->pointers[index] == POINTER_UNUSED)
    {
        LeaveCriticalSection(&ring->lock);
        *size = 0;
        return;
    }
    if (ring->pointers[index] == ring->end)
    {
        LeaveCriticalSection(&ring->lock);
        *size = 0;
    }
    else
    {
        ret = &ring->buffer[ring->pointers[index] * ring->buffer_size];
        memcpy(output, ret, ring->buffer_size);
        ring->pointers[index]++;
        if (ring->pointers[index] == ring->size)
            ring->pointers[index] = 0;
        LeaveCriticalSection(&ring->lock);
        *size = ring->buffer_size;
    }
}

void RingBuffer_Read(struct ReportRingBuffer *ring, UINT index, void *output, UINT *size)
{
    int pointer;
    void *ret = NULL;

    EnterCriticalSection(&ring->lock);
    if (index >= ring->pointer_alloc || ring->pointers[index] == POINTER_UNUSED
        || ring->end == ring->start)
    {
        LeaveCriticalSection(&ring->lock);
        *size = 0;
        return;
    }

    pointer = ring->pointers[index];

    if (pointer == ring->end)
        pointer--;

    if (pointer < 0)
        pointer = ring->size - 1;

    ret = &ring->buffer[pointer * ring->buffer_size];
    memcpy(output, ret, ring->buffer_size);
    if (pointer == ring->pointers[index])
    {
        ring->pointers[index]++;
        if (ring->pointers[index] == ring->size)
            ring->pointers[index] = 0;
    }
    LeaveCriticalSection(&ring->lock);
    *size = ring->buffer_size;
}

UINT RingBuffer_AddPointer(struct ReportRingBuffer *ring)
{
    UINT idx;
    EnterCriticalSection(&ring->lock);
    for (idx = 0; idx < ring->pointer_alloc; idx++)
        if (ring->pointers[idx] == POINTER_UNUSED)
            break;
    if (idx >= ring->pointer_alloc)
    {
        int count = idx = ring->pointer_alloc;
        ring->pointer_alloc *= 2;
        ring->pointers = realloc(ring->pointers, sizeof(UINT) * ring->pointer_alloc);
        for( ;count < ring->pointer_alloc; count++)
            ring->pointers[count] = POINTER_UNUSED;
    }
    ring->pointers[idx] = ring->end;
    LeaveCriticalSection(&ring->lock);
    return idx;
}

void RingBuffer_RemovePointer(struct ReportRingBuffer *ring, UINT index)
{
    EnterCriticalSection(&ring->lock);
    if (index < ring->pointer_alloc)
        ring->pointers[index] = POINTER_UNUSED;
    LeaveCriticalSection(&ring->lock);
}

void RingBuffer_Write(struct ReportRingBuffer *ring, void *data)
{
    UINT i;

    EnterCriticalSection(&ring->lock);
    memcpy(&ring->buffer[ring->end * ring->buffer_size], data, ring->buffer_size);
    ring->end++;
    if (ring->end == ring->size)
        ring->end = 0;
    if (ring->start == ring->end)
    {
        ring->start++;
        if (ring->start == ring->size)
            ring->start = 0;
    }
    for (i = 0; i < ring->pointer_alloc; i++)
        if (ring->pointers[i] == ring->end)
            ring->pointers[i] = ring->start;
    LeaveCriticalSection(&ring->lock);
}
