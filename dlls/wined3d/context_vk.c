/*
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006, 2008 Henri Verbeet
 * Copyright 2007-2011, 2013 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2020 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

void *wined3d_allocator_chunk_vk_map(struct wined3d_allocator_chunk_vk *chunk_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkResult vr;

    TRACE("chunk %p, memory 0x%s, map_ptr %p.\n", chunk_vk,
            wine_dbgstr_longlong(chunk_vk->vk_memory), chunk_vk->c.map_ptr);

    if (!chunk_vk->c.map_ptr && (vr = VK_CALL(vkMapMemory(device_vk->vk_device,
            chunk_vk->vk_memory, 0, VK_WHOLE_SIZE, 0, &chunk_vk->c.map_ptr))) < 0)
    {
        ERR("Failed to map chunk memory, vr %s.\n", wined3d_debug_vkresult(vr));
        return NULL;
    }

    ++chunk_vk->c.map_count;

    return chunk_vk->c.map_ptr;
}

void wined3d_allocator_chunk_vk_unmap(struct wined3d_allocator_chunk_vk *chunk_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;

    TRACE("chunk_vk %p, context_vk %p.\n", chunk_vk, context_vk);

    if (!--chunk_vk->c.map_count)
        VK_CALL(vkUnmapMemory(device_vk->vk_device, chunk_vk->vk_memory));
    chunk_vk->c.map_ptr = NULL;
}

VkDeviceMemory wined3d_context_vk_allocate_vram_chunk_memory(struct wined3d_context_vk *context_vk,
        unsigned int pool, size_t size)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkMemoryAllocateInfo allocate_info;
    VkDeviceMemory vk_memory;
    VkResult vr;

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.allocationSize = size;
    allocate_info.memoryTypeIndex = pool;
    if ((vr = VK_CALL(vkAllocateMemory(device_vk->vk_device, &allocate_info, NULL, &vk_memory))) < 0)
    {
        ERR("Failed to allocate memory, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    return vk_memory;
}

struct wined3d_allocator_block *wined3d_context_vk_allocate_memory(struct wined3d_context_vk *context_vk,
        unsigned int memory_type, VkDeviceSize size, VkDeviceMemory *vk_memory)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    struct wined3d_allocator *allocator = &device_vk->allocator;
    struct wined3d_allocator_block *block;

    if (size > WINED3D_ALLOCATOR_CHUNK_SIZE / 2)
    {
        *vk_memory = wined3d_context_vk_allocate_vram_chunk_memory(context_vk, memory_type, size);
        return NULL;
    }

    if (!(block = wined3d_allocator_allocate(allocator, &context_vk->c, memory_type, size)))
    {
        *vk_memory = VK_NULL_HANDLE;
        return NULL;
    }

    *vk_memory = wined3d_allocator_chunk_vk(block->chunk)->vk_memory;

    return block;
}

static bool wined3d_context_vk_create_slab_bo(struct wined3d_context_vk *context_vk,
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_type, struct wined3d_bo_vk *bo)
{
    const struct wined3d_adapter_vk *adapter_vk = wined3d_adapter_vk(context_vk->c.device->adapter);
    const VkPhysicalDeviceLimits *limits = &adapter_vk->device_limits;
    struct wined3d_bo_slab_vk_key key;
    struct wined3d_bo_slab_vk *slab;
    struct wine_rb_entry *entry;
    size_t object_size, idx;
    size_t alignment;

    if (size > WINED3D_ALLOCATOR_MIN_BLOCK_SIZE / 2)
        return false;

    alignment = WINED3D_SLAB_BO_MIN_OBJECT_ALIGN;
    if ((usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT))
            && limits->minTexelBufferOffsetAlignment > alignment)
        alignment = limits->minTexelBufferOffsetAlignment;
    if ((usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) && limits->minUniformBufferOffsetAlignment)
        alignment = limits->minUniformBufferOffsetAlignment;
    if ((usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) && limits->minStorageBufferOffsetAlignment)
        alignment = limits->minStorageBufferOffsetAlignment;

    object_size = (size + (alignment - 1)) & ~(alignment - 1);
    if (object_size < WINED3D_ALLOCATOR_MIN_BLOCK_SIZE / 32)
        object_size = WINED3D_ALLOCATOR_MIN_BLOCK_SIZE / 32;
    key.memory_type = memory_type;
    key.usage = usage;
    key.size = 32 * object_size;

    if ((entry = wine_rb_get(&context_vk->bo_slab_available, &key)))
    {
        slab = WINE_RB_ENTRY_VALUE(entry, struct wined3d_bo_slab_vk, entry);
        TRACE("Using existing bo slab %p.\n", slab);
    }
    else
    {
        if (!(slab = heap_alloc_zero(sizeof(*slab))))
        {
            ERR("Failed to allocate bo slab.\n");
            return false;
        }

        if (!wined3d_context_vk_create_bo(context_vk, key.size, usage, memory_type, &slab->bo))
        {
            ERR("Failed to create slab bo.\n");
            heap_free(slab);
            return false;
        }
        slab->map = ~0u;

        if (wine_rb_put(&context_vk->bo_slab_available, &key, &slab->entry) < 0)
        {
            ERR("Failed to add slab to available tree.\n");
            wined3d_context_vk_destroy_bo(context_vk, &slab->bo);
            heap_free(slab);
            return false;
        }

        TRACE("Created new bo slab %p.\n", slab);
    }

    idx = wined3d_bit_scan(&slab->map);
    if (!slab->map)
    {
        if (slab->next)
        {
            wine_rb_replace(&context_vk->bo_slab_available, &slab->entry, &slab->next->entry);
            slab->next = NULL;
        }
        else
        {
            wine_rb_remove(&context_vk->bo_slab_available, &slab->entry);
        }
    }

    *bo = slab->bo;
    bo->memory = NULL;
    bo->slab = slab;
    bo->buffer_offset = idx * object_size;
    bo->memory_offset = slab->bo.memory_offset + bo->buffer_offset;
    bo->size = size;
    bo->command_buffer_id = 0;

    TRACE("Using buffer 0x%s, memory 0x%s, offset 0x%s for bo %p.\n",
            wine_dbgstr_longlong(bo->vk_buffer), wine_dbgstr_longlong(bo->vk_memory),
            wine_dbgstr_longlong(bo->buffer_offset), bo);

    return true;
}

BOOL wined3d_context_vk_create_bo(struct wined3d_context_vk *context_vk, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_type, struct wined3d_bo_vk *bo)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkMemoryRequirements memory_requirements;
    struct wined3d_adapter_vk *adapter_vk;
    VkBufferCreateInfo create_info;
    unsigned int memory_type_idx;
    VkResult vr;

    if (wined3d_context_vk_create_slab_bo(context_vk, size, usage, memory_type, bo))
        return TRUE;

    adapter_vk = wined3d_adapter_vk(device_vk->d.adapter);

    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL;

    if ((vr = VK_CALL(vkCreateBuffer(device_vk->vk_device, &create_info, NULL, &bo->vk_buffer))) < 0)
    {
        ERR("Failed to create Vulkan buffer, vr %s.\n", wined3d_debug_vkresult(vr));
        return FALSE;
    }

    VK_CALL(vkGetBufferMemoryRequirements(device_vk->vk_device, bo->vk_buffer, &memory_requirements));

    memory_type_idx = wined3d_adapter_vk_get_memory_type_index(adapter_vk,
            memory_requirements.memoryTypeBits, memory_type);
    if (memory_type_idx == ~0u)
    {
        ERR("Failed to find suitable memory type.\n");
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }
    bo->memory = wined3d_context_vk_allocate_memory(context_vk,
            memory_type_idx, memory_requirements.size, &bo->vk_memory);
    if (!bo->vk_memory)
    {
        ERR("Failed to allocate buffer memory.\n");
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }
    bo->memory_offset = bo->memory ? bo->memory->offset : 0;

    if ((vr = VK_CALL(vkBindBufferMemory(device_vk->vk_device, bo->vk_buffer,
            bo->vk_memory, bo->memory_offset))) < 0)
    {
        ERR("Failed to bind buffer memory, vr %s.\n", wined3d_debug_vkresult(vr));
        if (bo->memory)
            wined3d_allocator_block_free(bo->memory);
        else
            VK_CALL(vkFreeMemory(device_vk->vk_device, bo->vk_memory, NULL));
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }

    bo->buffer_offset = 0;
    bo->size = size;
    bo->usage = usage;
    bo->memory_type = adapter_vk->memory_properties.memoryTypes[memory_type_idx].propertyFlags;
    bo->command_buffer_id = 0;
    bo->slab = NULL;

    TRACE("Created buffer 0x%s, memory 0x%s for bo %p.\n",
            wine_dbgstr_longlong(bo->vk_buffer), wine_dbgstr_longlong(bo->vk_memory), bo);

    return TRUE;
}

static struct wined3d_retired_object_vk *wined3d_context_vk_get_retired_object_vk(struct wined3d_context_vk *context_vk)
{
    struct wined3d_retired_objects_vk *retired = &context_vk->retired;
    struct wined3d_retired_object_vk *o;

    if (retired->free)
    {
        o = retired->free;
        retired->free = o->u.next;
        return o;
    }

    if (!wined3d_array_reserve((void **)&retired->objects, &retired->size,
            retired->count + 1, sizeof(*retired->objects)))
        return NULL;

    return &retired->objects[retired->count++];
}

void wined3d_context_vk_destroy_framebuffer(struct wined3d_context_vk *context_vk,
        VkFramebuffer vk_framebuffer, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyFramebuffer(device_vk->vk_device, vk_framebuffer, NULL));
        TRACE("Destroyed framebuffer 0x%s.\n", wine_dbgstr_longlong(vk_framebuffer));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking framebuffer 0x%s.\n", wine_dbgstr_longlong(vk_framebuffer));
        return;
    }

    o->type = WINED3D_RETIRED_FRAMEBUFFER_VK;
    o->u.vk_framebuffer = vk_framebuffer;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_memory(struct wined3d_context_vk *context_vk,
        VkDeviceMemory vk_memory, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkFreeMemory(device_vk->vk_device, vk_memory, NULL));
        TRACE("Freed memory 0x%s.\n", wine_dbgstr_longlong(vk_memory));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking memory 0x%s.\n", wine_dbgstr_longlong(vk_memory));
        return;
    }

    o->type = WINED3D_RETIRED_MEMORY_VK;
    o->u.vk_memory = vk_memory;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_allocator_block(struct wined3d_context_vk *context_vk,
        struct wined3d_allocator_block *block, uint64_t command_buffer_id)
{
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        wined3d_allocator_block_free(block);
        TRACE("Freed block %p.\n", block);
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking block %p.\n", block);
        return;
    }

    o->type = WINED3D_RETIRED_ALLOCATOR_BLOCK_VK;
    o->u.block = block;
    o->command_buffer_id = command_buffer_id;
}

static void wined3d_bo_slab_vk_free_slice(struct wined3d_bo_slab_vk *slab,
        SIZE_T idx, struct wined3d_context_vk *context_vk)
{
    struct wined3d_bo_slab_vk_key key;
    struct wine_rb_entry *entry;

    TRACE("slab %p, idx %lu, context_vk %p.\n", slab, idx, context_vk);

    if (!slab->map)
    {
        key.memory_type = slab->bo.memory_type;
        key.usage = slab->bo.usage;
        key.size = slab->bo.size;

        if ((entry = wine_rb_get(&context_vk->bo_slab_available, &key)))
        {
            slab->next = WINE_RB_ENTRY_VALUE(entry, struct wined3d_bo_slab_vk, entry);
            wine_rb_replace(&context_vk->bo_slab_available, entry, &slab->entry);
        }
        else if (wine_rb_put(&context_vk->bo_slab_available, &key, &slab->entry) < 0)
        {
            ERR("Unable to return slab %p (map 0x%08x) to available tree.\n", slab, slab->map);
        }
    }
    slab->map |= 1u << idx;
}

static void wined3d_context_vk_destroy_bo_slab_slice(struct wined3d_context_vk *context_vk,
        struct wined3d_bo_slab_vk *slab, SIZE_T idx, uint64_t command_buffer_id)
{
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        wined3d_bo_slab_vk_free_slice(slab, idx, context_vk);
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking slab %p, slice %#lx.\n", slab, idx);
        return;
    }

    o->type = WINED3D_RETIRED_BO_SLAB_SLICE_VK;
    o->u.slice.slab = slab;
    o->u.slice.idx = idx;
    o->command_buffer_id = command_buffer_id;
}

static void wined3d_context_vk_destroy_buffer(struct wined3d_context_vk *context_vk,
        VkBuffer vk_buffer, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, vk_buffer, NULL));
        TRACE("Destroyed buffer 0x%s.\n", wine_dbgstr_longlong(vk_buffer));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking buffer 0x%s.\n", wine_dbgstr_longlong(vk_buffer));
        return;
    }

    o->type = WINED3D_RETIRED_BUFFER_VK;
    o->u.vk_buffer = vk_buffer;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_image(struct wined3d_context_vk *context_vk,
        VkImage vk_image, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyImage(device_vk->vk_device, vk_image, NULL));
        TRACE("Destroyed image 0x%s.\n", wine_dbgstr_longlong(vk_image));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking image 0x%s.\n", wine_dbgstr_longlong(vk_image));
        return;
    }

    o->type = WINED3D_RETIRED_IMAGE_VK;
    o->u.vk_image = vk_image;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_image_view(struct wined3d_context_vk *context_vk,
        VkImageView vk_view, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyImageView(device_vk->vk_device, vk_view, NULL));
        TRACE("Destroyed image view 0x%s.\n", wine_dbgstr_longlong(vk_view));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking image view 0x%s.\n", wine_dbgstr_longlong(vk_view));
        return;
    }

    o->type = WINED3D_RETIRED_IMAGE_VIEW_VK;
    o->u.vk_image_view = vk_view;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_bo(struct wined3d_context_vk *context_vk, const struct wined3d_bo_vk *bo)
{
    size_t object_size, idx;

    TRACE("context_vk %p, bo %p.\n", context_vk, bo);

    if (bo->slab)
    {
        object_size = bo->slab->bo.size / 32;
        idx = bo->buffer_offset / object_size;
        wined3d_context_vk_destroy_bo_slab_slice(context_vk, bo->slab, idx, bo->command_buffer_id);
        return;
    }

    wined3d_context_vk_destroy_buffer(context_vk, bo->vk_buffer, bo->command_buffer_id);
    if (bo->memory)
    {
        wined3d_context_vk_destroy_allocator_block(context_vk, bo->memory, bo->command_buffer_id);
        return;
    }

    wined3d_context_vk_destroy_memory(context_vk, bo->vk_memory, bo->command_buffer_id);
}

static void wined3d_context_vk_cleanup_resources(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    struct wined3d_retired_objects_vk *retired = &context_vk->retired;
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_command_buffer_vk *buffer;
    struct wined3d_retired_object_vk *o;
    uint64_t command_buffer_id;
    SIZE_T i = 0;

    while (i < context_vk->submitted.buffer_count)
    {
        buffer = &context_vk->submitted.buffers[i];
        if (VK_CALL(vkGetFenceStatus(device_vk->vk_device, buffer->vk_fence)) == VK_NOT_READY)
        {
            ++i;
            continue;
        }

        TRACE("Command buffer %p with id 0x%s has finished.\n",
                buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));
        VK_CALL(vkDestroyFence(device_vk->vk_device, buffer->vk_fence, NULL));
        VK_CALL(vkFreeCommandBuffers(device_vk->vk_device,
                context_vk->vk_command_pool, 1, &buffer->vk_command_buffer));

        if (buffer->id > context_vk->completed_command_buffer_id)
            context_vk->completed_command_buffer_id = buffer->id;
        *buffer = context_vk->submitted.buffers[--context_vk->submitted.buffer_count];
    }
    command_buffer_id = context_vk->completed_command_buffer_id;

    retired->free = NULL;
    for (i = retired->count; i; --i)
    {
        o = &retired->objects[i - 1];

        if (o->type != WINED3D_RETIRED_FREE_VK && o->command_buffer_id > command_buffer_id)
            continue;

        switch (o->type)
        {
            case WINED3D_RETIRED_FREE_VK:
                /* Nothing to do. */
                break;

            case WINED3D_RETIRED_FRAMEBUFFER_VK:
                VK_CALL(vkDestroyFramebuffer(device_vk->vk_device, o->u.vk_framebuffer, NULL));
                TRACE("Destroyed framebuffer 0x%s.\n", wine_dbgstr_longlong(o->u.vk_framebuffer));
                break;

            case WINED3D_RETIRED_MEMORY_VK:
                VK_CALL(vkFreeMemory(device_vk->vk_device, o->u.vk_memory, NULL));
                TRACE("Freed memory 0x%s.\n", wine_dbgstr_longlong(o->u.vk_memory));
                break;

            case WINED3D_RETIRED_ALLOCATOR_BLOCK_VK:
                TRACE("Destroying block %p.\n", o->u.block);
                wined3d_allocator_block_free(o->u.block);
                break;

            case WINED3D_RETIRED_BO_SLAB_SLICE_VK:
                wined3d_bo_slab_vk_free_slice(o->u.slice.slab, o->u.slice.idx, context_vk);
                break;

            case WINED3D_RETIRED_BUFFER_VK:
                VK_CALL(vkDestroyBuffer(device_vk->vk_device, o->u.vk_buffer, NULL));
                TRACE("Destroyed buffer 0x%s.\n", wine_dbgstr_longlong(o->u.vk_buffer));
                break;

            case WINED3D_RETIRED_IMAGE_VK:
                VK_CALL(vkDestroyImage(device_vk->vk_device, o->u.vk_image, NULL));
                TRACE("Destroyed image 0x%s.\n", wine_dbgstr_longlong(o->u.vk_image));
                break;

            case WINED3D_RETIRED_IMAGE_VIEW_VK:
                VK_CALL(vkDestroyImageView(device_vk->vk_device, o->u.vk_image_view, NULL));
                TRACE("Destroyed image view 0x%s.\n", wine_dbgstr_longlong(o->u.vk_image_view));
                break;

            default:
                ERR("Unhandled object type %#x.\n", o->type);
                break;
        }

        if (i == retired->count)
        {
            --retired->count;
            continue;
        }

        o->type = WINED3D_RETIRED_FREE_VK;
        o->u.next = retired->free;
        retired->free = o;
    }
}

static void wined3d_context_vk_destroy_bo_slab(struct wine_rb_entry *entry, void *ctx)
{
    struct wined3d_context_vk *context_vk = ctx;
    struct wined3d_bo_slab_vk *slab, *next;

    slab = WINE_RB_ENTRY_VALUE(entry, struct wined3d_bo_slab_vk, entry);
    while (slab)
    {
        next = slab->next;
        wined3d_context_vk_destroy_bo(context_vk, &slab->bo);
        heap_free(slab);
        slab = next;
    }
}

static void wined3d_render_pass_key_vk_init(struct wined3d_render_pass_key_vk *key,
        const struct wined3d_fb_state *fb, unsigned int rt_count, bool depth_stencil, uint32_t clear_flags)
{
    struct wined3d_render_pass_attachment_vk *a;
    struct wined3d_rendertarget_view *view;
    unsigned int i;

    memset(key, 0, sizeof(*key));

    for (i = 0; i < rt_count; ++i)
    {
        if (!(view = fb->render_targets[i]) || view->format->id == WINED3DFMT_NULL)
            continue;

        a = &key->rt[i];
        a->vk_format = wined3d_format_vk(view->format)->vk_format;
        a->vk_samples = max(1, wined3d_resource_get_sample_count(view->resource));
        a->vk_layout = wined3d_texture_vk(wined3d_texture_from_resource(view->resource))->layout;
        key->rt_mask |= 1u << i;
    }

    if (depth_stencil && (view = fb->depth_stencil))
    {
        a = &key->ds;
        a->vk_format = wined3d_format_vk(view->format)->vk_format;
        a->vk_samples = max(1, wined3d_resource_get_sample_count(view->resource));
        a->vk_layout = wined3d_texture_vk(wined3d_texture_from_resource(view->resource))->layout;
        key->rt_mask |= 1u << WINED3D_MAX_RENDER_TARGETS;
    }

    key->clear_flags = clear_flags;
}

static void wined3d_render_pass_vk_cleanup(struct wined3d_render_pass_vk *pass,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;

    VK_CALL(vkDestroyRenderPass(device_vk->vk_device, pass->vk_render_pass, NULL));
}

static bool wined3d_render_pass_vk_init(struct wined3d_render_pass_vk *pass,
        struct wined3d_context_vk *context_vk, const struct wined3d_render_pass_key_vk *key)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    VkAttachmentReference attachment_references[WINED3D_MAX_RENDER_TARGETS];
    VkAttachmentDescription attachments[WINED3D_MAX_RENDER_TARGETS + 1];
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    const struct wined3d_render_pass_attachment_vk *a;
    VkAttachmentReference ds_attachment_reference;
    VkAttachmentReference *ds_reference = NULL;
    unsigned int attachment_count, rt_count, i;
    VkAttachmentDescription *attachment;
    VkSubpassDescription sub_pass_desc;
    VkRenderPassCreateInfo pass_desc;
    uint32_t mask;
    VkResult vr;

    rt_count = 0;
    attachment_count = 0;
    mask = key->rt_mask & ((1u << WINED3D_MAX_RENDER_TARGETS) - 1);
    while (mask)
    {
        i = wined3d_bit_scan(&mask);
        a = &key->rt[i];

        attachment = &attachments[attachment_count];
        attachment->flags = 0;
        attachment->format = a->vk_format;
        attachment->samples = a->vk_samples;
        if (key->clear_flags & WINED3DCLEAR_TARGET)
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->initialLayout = a->vk_layout;
        attachment->finalLayout = a->vk_layout;

        attachment_references[i].attachment = attachment_count;
        attachment_references[i].layout = a->vk_layout;

        ++attachment_count;
        rt_count = i + 1;
    }

    mask = ~key->rt_mask & ((1u << rt_count) - 1);
    while (mask)
    {
        i = wined3d_bit_scan(&mask);
        attachment_references[i].attachment = VK_ATTACHMENT_UNUSED;
        attachment_references[i].layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    if (key->rt_mask & (1u << WINED3D_MAX_RENDER_TARGETS))
    {
        a = &key->ds;

        attachment = &attachments[attachment_count];
        attachment->flags = 0;
        attachment->format = a->vk_format;
        attachment->samples = a->vk_samples;
        if (key->clear_flags & WINED3DCLEAR_ZBUFFER)
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (key->clear_flags & WINED3DCLEAR_STENCIL)
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment->initialLayout = a->vk_layout;
        attachment->finalLayout = a->vk_layout;

        ds_reference = &ds_attachment_reference;
        ds_reference->attachment = attachment_count;
        ds_reference->layout = a->vk_layout;

        ++attachment_count;
    }

    sub_pass_desc.flags = 0;
    sub_pass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub_pass_desc.inputAttachmentCount = 0;
    sub_pass_desc.pInputAttachments = NULL;
    sub_pass_desc.colorAttachmentCount = rt_count;
    sub_pass_desc.pColorAttachments = attachment_references;
    sub_pass_desc.pResolveAttachments = NULL;
    sub_pass_desc.pDepthStencilAttachment = ds_reference;
    sub_pass_desc.preserveAttachmentCount = 0;
    sub_pass_desc.pPreserveAttachments = NULL;

    pass_desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_desc.pNext = NULL;
    pass_desc.flags = 0;
    pass_desc.attachmentCount = attachment_count;
    pass_desc.pAttachments = attachments;
    pass_desc.subpassCount = 1;
    pass_desc.pSubpasses = &sub_pass_desc;
    pass_desc.dependencyCount = 0;
    pass_desc.pDependencies = NULL;

    pass->key = *key;
    if ((vr = VK_CALL(vkCreateRenderPass(device_vk->vk_device,
            &pass_desc, NULL, &pass->vk_render_pass))) < 0)
    {
        WARN("Failed to create Vulkan render pass, vr %d.\n", vr);
        return false;
    }

    return true;
}

VkRenderPass wined3d_context_vk_get_render_pass(struct wined3d_context_vk *context_vk,
        const struct wined3d_fb_state *fb, unsigned int rt_count, bool depth_stencil, uint32_t clear_flags)
{
    struct wined3d_render_pass_key_vk key;
    struct wined3d_render_pass_vk *pass;
    struct wine_rb_entry *entry;

    wined3d_render_pass_key_vk_init(&key, fb, rt_count, depth_stencil, clear_flags);
    if ((entry = wine_rb_get(&context_vk->render_passes, &key)))
        return WINE_RB_ENTRY_VALUE(entry, struct wined3d_render_pass_vk, entry)->vk_render_pass;

    if (!(pass = heap_alloc(sizeof(*pass))))
        return VK_NULL_HANDLE;

    if (!wined3d_render_pass_vk_init(pass, context_vk, &key))
    {
        heap_free(pass);
        return VK_NULL_HANDLE;
    }

    if (wine_rb_put(&context_vk->render_passes, &pass->key, &pass->entry) == -1)
    {
        ERR("Failed to insert render pass.\n");
        wined3d_render_pass_vk_cleanup(pass, context_vk);
        heap_free(pass);
        return VK_NULL_HANDLE;
    }

    return pass->vk_render_pass;
}

static void wined3d_context_vk_destroy_render_pass(struct wine_rb_entry *entry, void *ctx)
{
    struct wined3d_render_pass_vk *pass = WINE_RB_ENTRY_VALUE(entry,
            struct wined3d_render_pass_vk, entry);

    wined3d_render_pass_vk_cleanup(pass, ctx);
    heap_free(pass);
}

void wined3d_context_vk_cleanup(struct wined3d_context_vk *context_vk)
{
    struct wined3d_command_buffer_vk *buffer = &context_vk->current_command_buffer;
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;

    if (buffer->vk_command_buffer)
    {
        VK_CALL(vkFreeCommandBuffers(device_vk->vk_device,
                context_vk->vk_command_pool, 1, &buffer->vk_command_buffer));
        buffer->vk_command_buffer = VK_NULL_HANDLE;
    }
    VK_CALL(vkDestroyCommandPool(device_vk->vk_device, context_vk->vk_command_pool, NULL));

    wined3d_context_vk_wait_command_buffer(context_vk, buffer->id - 1);
    context_vk->completed_command_buffer_id = buffer->id;
    wined3d_context_vk_cleanup_resources(context_vk);
    wine_rb_destroy(&context_vk->bo_slab_available, wined3d_context_vk_destroy_bo_slab, context_vk);
    heap_free(context_vk->submitted.buffers);
    heap_free(context_vk->retired.objects);

    wine_rb_destroy(&context_vk->render_passes, wined3d_context_vk_destroy_render_pass, context_vk);

    wined3d_context_cleanup(&context_vk->c);
}

VkCommandBuffer wined3d_context_vk_get_command_buffer(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkCommandBufferAllocateInfo command_buffer_info;
    struct wined3d_command_buffer_vk *buffer;
    VkCommandBufferBeginInfo begin_info;
    VkResult vr;

    TRACE("context_vk %p.\n", context_vk);

    buffer = &context_vk->current_command_buffer;
    if (buffer->vk_command_buffer)
    {
        TRACE("Returning existing command buffer %p with id 0x%s.\n",
                buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));
        return buffer->vk_command_buffer;
    }

    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.pNext = NULL;
    command_buffer_info.commandPool = context_vk->vk_command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;
    if ((vr = VK_CALL(vkAllocateCommandBuffers(device_vk->vk_device,
            &command_buffer_info, &buffer->vk_command_buffer))) < 0)
    {
        WARN("Failed to allocate Vulkan command buffer, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = NULL;
    if ((vr = VK_CALL(vkBeginCommandBuffer(buffer->vk_command_buffer, &begin_info))) < 0)
    {
        WARN("Failed to begin command buffer, vr %s.\n", wined3d_debug_vkresult(vr));
        VK_CALL(vkFreeCommandBuffers(device_vk->vk_device, context_vk->vk_command_pool,
                1, &buffer->vk_command_buffer));
        return buffer->vk_command_buffer = VK_NULL_HANDLE;
    }

    TRACE("Created new command buffer %p with id 0x%s.\n",
            buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));

    return buffer->vk_command_buffer;
}

void wined3d_context_vk_submit_command_buffer(struct wined3d_context_vk *context_vk,
        unsigned int wait_semaphore_count, const VkSemaphore *wait_semaphores, const VkPipelineStageFlags *wait_stages,
        unsigned int signal_semaphore_count, const VkSemaphore *signal_semaphores)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_command_buffer_vk *buffer;
    VkFenceCreateInfo fence_desc;
    VkSubmitInfo submit_info;
    VkResult vr;

    TRACE("context_vk %p, wait_semaphore_count %u, wait_semaphores %p, wait_stages %p,"
            "signal_semaphore_count %u, signal_semaphores %p.\n",
            context_vk, wait_semaphore_count, wait_semaphores, wait_stages,
            signal_semaphore_count, signal_semaphores);

    buffer = &context_vk->current_command_buffer;
    if (!buffer->vk_command_buffer)
        return;

    TRACE("Submitting command buffer %p with id 0x%s.\n",
            buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));

    VK_CALL(vkEndCommandBuffer(buffer->vk_command_buffer));

    fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_desc.pNext = NULL;
    fence_desc.flags = 0;
    if ((vr = VK_CALL(vkCreateFence(device_vk->vk_device, &fence_desc, NULL, &buffer->vk_fence))) < 0)
        ERR("Failed to create fence, vr %s.\n", wined3d_debug_vkresult(vr));

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = wait_semaphore_count;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer->vk_command_buffer;
    submit_info.signalSemaphoreCount = signal_semaphore_count;
    submit_info.pSignalSemaphores = signal_semaphores;

    if ((vr = VK_CALL(vkQueueSubmit(device_vk->vk_queue, 1, &submit_info, buffer->vk_fence))) < 0)
        ERR("Failed to submit command buffer %p, vr %s.\n",
                buffer->vk_command_buffer, wined3d_debug_vkresult(vr));

    if (!wined3d_array_reserve((void **)&context_vk->submitted.buffers, &context_vk->submitted.buffers_size,
            context_vk->submitted.buffer_count + 1, sizeof(*context_vk->submitted.buffers)))
        ERR("Failed to grow submitted command buffer array.\n");

    context_vk->submitted.buffers[context_vk->submitted.buffer_count++] = *buffer;

    buffer->vk_command_buffer = VK_NULL_HANDLE;
    /* We don't expect this to ever happen, but handle it anyway. */
    if (!++buffer->id)
    {
        wined3d_context_vk_wait_command_buffer(context_vk, buffer->id - 1);
        context_vk->completed_command_buffer_id = 0;
        buffer->id = 1;
    }
    wined3d_context_vk_cleanup_resources(context_vk);
}

void wined3d_context_vk_wait_command_buffer(struct wined3d_context_vk *context_vk, uint64_t id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    SIZE_T i;

    if (id <= context_vk->completed_command_buffer_id
            || id > context_vk->current_command_buffer.id) /* In case the buffer ID wrapped. */
        return;

    for (i = 0; i < context_vk->submitted.buffer_count; ++i)
    {
        if (context_vk->submitted.buffers[i].id != id)
            continue;

        VK_CALL(vkWaitForFences(device_vk->vk_device, 1,
                &context_vk->submitted.buffers[i].vk_fence, VK_TRUE, UINT64_MAX));
        wined3d_context_vk_cleanup_resources(context_vk);
        return;
    }

    ERR("Failed to find fence for command buffer with id 0x%s.\n", wine_dbgstr_longlong(id));
}

void wined3d_context_vk_image_barrier(struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
        VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout,
        VkImageLayout new_layout, VkImage image, VkImageAspectFlags aspect_mask)
{
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkImageMemoryBarrier barrier;

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = NULL;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstAccessMask = dst_access_mask;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VK_CALL(vkCmdPipelineBarrier(vk_command_buffer, src_stage_mask, dst_stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier));
}

static int wined3d_render_pass_vk_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_render_pass_key_vk *k = key;
    const struct wined3d_render_pass_vk *pass = WINE_RB_ENTRY_VALUE(entry,
            const struct wined3d_render_pass_vk, entry);

    return memcmp(k, &pass->key, sizeof(*k));
}

static int wined3d_bo_slab_vk_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_bo_slab_vk *slab = WINE_RB_ENTRY_VALUE(entry, const struct wined3d_bo_slab_vk, entry);
    const struct wined3d_bo_slab_vk_key *k = key;

    if (k->memory_type != slab->bo.memory_type)
        return k->memory_type - slab->bo.memory_type;
    if (k->usage != slab->bo.usage)
        return k->usage - slab->bo.usage;
    return k->size - slab->bo.size;
}

HRESULT wined3d_context_vk_init(struct wined3d_context_vk *context_vk, struct wined3d_swapchain *swapchain)
{
    VkCommandPoolCreateInfo command_pool_info;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_adapter_vk *adapter_vk;
    struct wined3d_device_vk *device_vk;
    VkResult vr;

    TRACE("context_vk %p, swapchain %p.\n", context_vk, swapchain);

    wined3d_context_init(&context_vk->c, swapchain);
    device_vk = wined3d_device_vk(swapchain->device);
    adapter_vk = wined3d_adapter_vk(device_vk->d.adapter);
    context_vk->vk_info = vk_info = &adapter_vk->vk_info;

    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    command_pool_info.queueFamilyIndex = device_vk->vk_queue_family_index;
    if ((vr = VK_CALL(vkCreateCommandPool(device_vk->vk_device,
            &command_pool_info, NULL, &context_vk->vk_command_pool))) < 0)
    {
        ERR("Failed to create Vulkan command pool, vr %s.\n", wined3d_debug_vkresult(vr));
        wined3d_context_cleanup(&context_vk->c);
        return E_FAIL;
    }
    context_vk->current_command_buffer.id = 1;

    wine_rb_init(&context_vk->render_passes, wined3d_render_pass_vk_compare);
    wine_rb_init(&context_vk->bo_slab_available, wined3d_bo_slab_vk_compare);

    return WINED3D_OK;
}
