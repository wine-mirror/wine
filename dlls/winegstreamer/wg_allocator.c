/*
 * GStreamer memory allocator
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <stdarg.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/audio/audio.h>

#include "unix_private.h"

#include "wine/list.h"

typedef struct
{
    GstMemory parent;
    struct list entry;

    GstAllocationParams alloc_params;
    GstMemory *unix_memory;
    GstMapInfo unix_map_info;

    struct wg_sample *sample;
    gsize written;
} WgMemory;

typedef struct
{
    GstAllocator parent;

    pthread_mutex_t mutex;
    pthread_cond_t release_cond;
    struct list memory_list;

    struct wg_sample *next_sample;
} WgAllocator;

typedef struct
{
    GstAllocatorClass parent_class;
} WgAllocatorClass;

G_DEFINE_TYPE(WgAllocator, wg_allocator, GST_TYPE_ALLOCATOR);

static void *get_unix_memory_data(WgMemory *memory)
{
    if (!memory->unix_memory)
    {
        memory->unix_memory = gst_allocator_alloc(NULL, memory->parent.maxsize, &memory->alloc_params);
        gst_memory_map(memory->unix_memory, &memory->unix_map_info, GST_MAP_WRITE);
        GST_INFO("Allocated unix memory %p, data %p for memory %p, sample %p", memory->unix_memory,
                memory->unix_map_info.data, memory, memory->sample);
    }

    return memory->unix_map_info.data;
}

static void release_memory_sample(WgAllocator *allocator, WgMemory *memory, bool discard_data)
{
    struct wg_sample *sample;

    if (!(sample = memory->sample))
        return;

    while (sample->refcount > 1)
    {
        GST_WARNING("Waiting for sample %p to be unmapped", sample);
        pthread_cond_wait(&allocator->release_cond, &allocator->mutex);
    }
    InterlockedDecrement(&sample->refcount);

    if (memory->written && !discard_data)
    {
        GST_WARNING("Copying %#zx bytes from sample %p, back to memory %p", memory->written, sample, memory);
        memcpy(get_unix_memory_data(memory), wg_sample_data(memory->sample), memory->written);
    }

    memory->sample = NULL;
    GST_INFO("Released sample %p from memory %p", sample, memory);
}

static gpointer wg_allocator_map(GstMemory *gst_memory, GstMapInfo *info, gsize maxsize)
{
    WgAllocator *allocator = (WgAllocator *)gst_memory->allocator;
    WgMemory *memory = (WgMemory *)gst_memory;

    if (gst_memory->parent)
        return wg_allocator_map(gst_memory->parent, info, maxsize);

    GST_LOG("memory %p, info %p, maxsize %#zx", memory, info, maxsize);

    pthread_mutex_lock(&allocator->mutex);

    if (!memory->sample)
        info->data = get_unix_memory_data(memory);
    else
    {
        InterlockedIncrement(&memory->sample->refcount);
        info->data = wg_sample_data(memory->sample);
    }
    if (info->flags & GST_MAP_WRITE)
        memory->written = max(memory->written, maxsize);

    pthread_mutex_unlock(&allocator->mutex);

    GST_INFO("Mapped memory %p to %p", memory, info->data);
    return info->data;
}

static void wg_allocator_unmap(GstMemory *gst_memory, GstMapInfo *info)
{
    WgAllocator *allocator = (WgAllocator *)gst_memory->allocator;
    WgMemory *memory = (WgMemory *)gst_memory;

    if (gst_memory->parent)
        return wg_allocator_unmap(gst_memory->parent, info);

    GST_LOG("memory %p, info %p", memory, info);

    pthread_mutex_lock(&allocator->mutex);

    if (memory->sample && info->data == wg_sample_data(memory->sample))
    {
        InterlockedDecrement(&memory->sample->refcount);
        pthread_cond_signal(&allocator->release_cond);
    }

    pthread_mutex_unlock(&allocator->mutex);
}

static void wg_allocator_init(WgAllocator *allocator)
{
    GST_LOG("allocator %p", allocator);

    allocator->parent.mem_type = "Wine";

    allocator->parent.mem_map_full = wg_allocator_map;
    allocator->parent.mem_unmap_full = wg_allocator_unmap;

    GST_OBJECT_FLAG_SET(allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);

    pthread_mutex_init(&allocator->mutex, NULL);
    pthread_cond_init(&allocator->release_cond, NULL);
    list_init(&allocator->memory_list);
}

static void wg_allocator_finalize(GObject *object)
{
    WgAllocator *allocator = (WgAllocator *)object;

    GST_LOG("allocator %p", allocator);

    pthread_cond_destroy(&allocator->release_cond);
    pthread_mutex_destroy(&allocator->mutex);

    G_OBJECT_CLASS(wg_allocator_parent_class)->finalize(object);
}

static GstMemory *wg_allocator_alloc(GstAllocator *gst_allocator, gsize size,
        GstAllocationParams *params)
{
    WgAllocator *allocator = (WgAllocator *)gst_allocator;
    WgMemory *memory;

    GST_LOG("allocator %p, size %#zx, params %p", allocator, size, params);

    memory = g_slice_new0(WgMemory);
    gst_memory_init(GST_MEMORY_CAST(memory), 0, GST_ALLOCATOR_CAST(allocator),
            NULL, size, 0, 0, size);
    memory->alloc_params = *params;

    pthread_mutex_lock(&allocator->mutex);

    memory->sample = allocator->next_sample;
    allocator->next_sample = NULL;

    if (memory->sample && memory->sample->max_size < size)
        release_memory_sample(allocator, memory, true);

    list_add_tail(&allocator->memory_list, &memory->entry);

    pthread_mutex_unlock(&allocator->mutex);

    GST_INFO("Allocated memory %p, sample %p", memory, memory->sample);
    return (GstMemory *)memory;
}

static void wg_allocator_free(GstAllocator *gst_allocator, GstMemory *gst_memory)
{
    WgAllocator *allocator = (WgAllocator *)gst_allocator;
    WgMemory *memory = (WgMemory *)gst_memory;

    GST_LOG("allocator %p, memory %p", allocator, memory);

    pthread_mutex_lock(&allocator->mutex);

    if (memory->sample)
        InterlockedDecrement(&memory->sample->refcount);
    memory->sample = NULL;

    list_remove(&memory->entry);

    pthread_mutex_unlock(&allocator->mutex);

    if (memory->unix_memory)
    {
        gst_memory_unmap(memory->unix_memory, &memory->unix_map_info);
        gst_memory_unref(memory->unix_memory);
    }
    g_slice_free(WgMemory, memory);
}

static void wg_allocator_class_init(WgAllocatorClass *klass)
{
    GstAllocatorClass *parent_class = (GstAllocatorClass *)klass;
    GObjectClass *root_class = (GObjectClass *)klass;

    GST_LOG("klass %p", klass);

    parent_class->alloc = wg_allocator_alloc;
    parent_class->free = wg_allocator_free;
    root_class->finalize = wg_allocator_finalize;
}

GstAllocator *wg_allocator_create(void)
{
    WgAllocator *allocator;

    if (!(allocator = g_object_new(wg_allocator_get_type(), NULL)))
        return NULL;

    return GST_ALLOCATOR(allocator);
}

void wg_allocator_destroy(GstAllocator *gst_allocator)
{
    WgAllocator *allocator = (WgAllocator *)gst_allocator;
    WgMemory *memory;

    GST_LOG("allocator %p", allocator);

    pthread_mutex_lock(&allocator->mutex);
    LIST_FOR_EACH_ENTRY(memory, &allocator->memory_list, WgMemory, entry)
        release_memory_sample(allocator, memory, true);
    pthread_mutex_unlock(&allocator->mutex);

    g_object_unref(allocator);

    GST_INFO("Destroyed buffer allocator %p", allocator);
}

static WgMemory *find_sample_memory(WgAllocator *allocator, struct wg_sample *sample)
{
    WgMemory *memory;

    LIST_FOR_EACH_ENTRY(memory, &allocator->memory_list, WgMemory, entry)
        if (memory->sample == sample)
            return memory;

    return NULL;
}

void wg_allocator_provide_sample(GstAllocator *gst_allocator, struct wg_sample *sample)
{
    WgAllocator *allocator = (WgAllocator *)gst_allocator;
    struct wg_sample *previous;

    GST_LOG("allocator %p, sample %p", allocator, sample);

    if (sample)
        InterlockedIncrement(&sample->refcount);

    pthread_mutex_lock(&allocator->mutex);
    previous = allocator->next_sample;
    allocator->next_sample = sample;
    pthread_mutex_unlock(&allocator->mutex);

    if (previous)
        InterlockedDecrement(&previous->refcount);
}

void wg_allocator_release_sample(GstAllocator *gst_allocator, struct wg_sample *sample,
        bool discard_data)
{
    WgAllocator *allocator = (WgAllocator *)gst_allocator;
    WgMemory *memory;

    GST_LOG("allocator %p, sample %p, discard_data %u", allocator, sample, discard_data);

    pthread_mutex_lock(&allocator->mutex);
    if ((memory = find_sample_memory(allocator, sample)))
        release_memory_sample(allocator, memory, discard_data);
    else if (sample->refcount)
        GST_ERROR("Couldn't find memory for sample %p", sample);
    pthread_mutex_unlock(&allocator->mutex);
}
