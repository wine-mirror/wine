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

GST_DEBUG_CATEGORY_EXTERN(wine);
#define GST_CAT_DEFAULT wine

typedef struct
{
    GstMemory parent;

    GstMemory *unix_memory;
    GstMapInfo unix_map_info;
} WgMemory;

typedef struct
{
    GstAllocator parent;
} WgAllocator;

typedef struct
{
    GstAllocatorClass parent_class;
} WgAllocatorClass;

G_DEFINE_TYPE(WgAllocator, wg_allocator, GST_TYPE_ALLOCATOR);

static gpointer wg_allocator_map(GstMemory *gst_memory, GstMapInfo *info, gsize maxsize)
{
    WgMemory *memory = (WgMemory *)gst_memory;

    if (gst_memory->parent)
        return wg_allocator_map(gst_memory->parent, info, maxsize);

    GST_LOG("memory %p, info %p, maxsize %#zx", memory, info, maxsize);

    info->data = memory->unix_map_info.data;

    GST_INFO("Mapped memory %p to %p", memory, info->data);
    return info->data;
}

static void wg_allocator_unmap(GstMemory *gst_memory, GstMapInfo *info)
{
    WgMemory *memory = (WgMemory *)gst_memory;

    if (gst_memory->parent)
        return wg_allocator_unmap(gst_memory->parent, info);

    GST_LOG("memory %p, info %p", memory, info);
}

static void wg_allocator_init(WgAllocator *allocator)
{
    GST_LOG("allocator %p", allocator);

    allocator->parent.mem_type = "Wine";

    allocator->parent.mem_map_full = wg_allocator_map;
    allocator->parent.mem_unmap_full = wg_allocator_unmap;

    GST_OBJECT_FLAG_SET(allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);
}

static void wg_allocator_finalize(GObject *object)
{
    WgAllocator *allocator = (WgAllocator *)object;

    GST_LOG("allocator %p", allocator);

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
    memory->unix_memory = gst_allocator_alloc(NULL, size, params);
    gst_memory_map(memory->unix_memory, &memory->unix_map_info, GST_MAP_WRITE);

    GST_INFO("Allocated memory %p, unix_memory %p, data %p", memory, memory->unix_memory,
            memory->unix_map_info.data);
    return (GstMemory *)memory;
}

static void wg_allocator_free(GstAllocator *gst_allocator, GstMemory *gst_memory)
{
    WgAllocator *allocator = (WgAllocator *)gst_allocator;
    WgMemory *memory = (WgMemory *)gst_memory;

    GST_LOG("allocator %p, memory %p", allocator, memory);

    gst_memory_unmap(memory->unix_memory, &memory->unix_map_info);
    gst_memory_unref(memory->unix_memory);
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
    return g_object_new(wg_allocator_get_type(), NULL);
}

void wg_allocator_destroy(GstAllocator *gst_allocator)
{
    WgAllocator *allocator = (WgAllocator *)gst_allocator;

    GST_LOG("allocator %p", allocator);

    g_object_unref(allocator);

    GST_INFO("Destroyed buffer allocator %p", allocator);
}
