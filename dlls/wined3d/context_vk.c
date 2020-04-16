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

BOOL wined3d_context_vk_create_bo(struct wined3d_context_vk *context_vk, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_type, struct wined3d_bo_vk *bo)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkMemoryRequirements memory_requirements;
    struct wined3d_adapter_vk *adapter_vk;
    VkMemoryAllocateInfo allocate_info;
    VkBufferCreateInfo create_info;
    VkResult vr;

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

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = wined3d_adapter_vk_get_memory_type_index(adapter_vk,
            memory_requirements.memoryTypeBits, memory_type);
    if (allocate_info.memoryTypeIndex == ~0u)
    {
        ERR("Failed to find suitable memory type.\n");
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }
    if ((vr = VK_CALL(vkAllocateMemory(device_vk->vk_device, &allocate_info, NULL, &bo->vk_memory))) < 0)
    {
        ERR("Failed to allocate buffer memory, vr %s.\n", wined3d_debug_vkresult(vr));
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }

    if ((vr = VK_CALL(vkBindBufferMemory(device_vk->vk_device, bo->vk_buffer, bo->vk_memory, 0))) < 0)
    {
        ERR("Failed to bind buffer memory, vr %s.\n", wined3d_debug_vkresult(vr));
        VK_CALL(vkFreeMemory(device_vk->vk_device, bo->vk_memory, NULL));
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }

    return TRUE;
}

void wined3d_context_vk_destroy_bo(struct wined3d_context_vk *context_vk, const struct wined3d_bo_vk *bo)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;

    VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
    VK_CALL(vkFreeMemory(device_vk->vk_device, bo->vk_memory, NULL));
}

static void wined3d_context_vk_cleanup_resources(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_command_buffer_vk *buffer;
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
    heap_free(context_vk->submitted.buffers);
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

void wined3d_context_vk_submit_command_buffer(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_command_buffer_vk *buffer;
    VkFenceCreateInfo fence_desc;
    VkSubmitInfo submit_info;
    VkResult vr;

    TRACE("context_vk %p.\n", context_vk);

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
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer->vk_command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

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

    if (id <= context_vk->completed_command_buffer_id)
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

    return WINED3D_OK;
}
