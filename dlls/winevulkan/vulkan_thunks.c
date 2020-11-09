/* Automatically generated from Vulkan vk.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright (c) 2015-2020 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "config.h"
#include "wine/port.h"

#include "vulkan_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAcquireNextImageInfoKHR_win_to_host(const VkAcquireNextImageInfoKHR *in, VkAcquireNextImageInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->swapchain = in->swapchain;
    out->timeout = in->timeout;
    out->semaphore = in->semaphore;
    out->fence = in->fence;
    out->deviceMask = in->deviceMask;
}

static inline void convert_VkAcquireProfilingLockInfoKHR_win_to_host(const VkAcquireProfilingLockInfoKHR *in, VkAcquireProfilingLockInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->timeout = in->timeout;
}

static inline void convert_VkDescriptorSetAllocateInfo_win_to_host(const VkDescriptorSetAllocateInfo *in, VkDescriptorSetAllocateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->descriptorPool = in->descriptorPool;
    out->descriptorSetCount = in->descriptorSetCount;
    out->pSetLayouts = in->pSetLayouts;
}

static inline void convert_VkMemoryAllocateInfo_win_to_host(const VkMemoryAllocateInfo *in, VkMemoryAllocateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->allocationSize = in->allocationSize;
    out->memoryTypeIndex = in->memoryTypeIndex;
}

static inline VkCommandBufferInheritanceInfo_host *convert_VkCommandBufferInheritanceInfo_array_win_to_host(const VkCommandBufferInheritanceInfo *in, uint32_t count)
{
    VkCommandBufferInheritanceInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].renderPass = in[i].renderPass;
        out[i].subpass = in[i].subpass;
        out[i].framebuffer = in[i].framebuffer;
        out[i].occlusionQueryEnable = in[i].occlusionQueryEnable;
        out[i].queryFlags = in[i].queryFlags;
        out[i].pipelineStatistics = in[i].pipelineStatistics;
    }

    return out;
}

static inline void free_VkCommandBufferInheritanceInfo_array(VkCommandBufferInheritanceInfo_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkCommandBufferBeginInfo_win_to_host(const VkCommandBufferBeginInfo *in, VkCommandBufferBeginInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->pInheritanceInfo = convert_VkCommandBufferInheritanceInfo_array_win_to_host(in->pInheritanceInfo, 1);
}

static inline void free_VkCommandBufferBeginInfo(VkCommandBufferBeginInfo_host *in)
{
    free_VkCommandBufferInheritanceInfo_array((VkCommandBufferInheritanceInfo_host *)in->pInheritanceInfo, 1);
}

static inline VkBindAccelerationStructureMemoryInfoKHR_host *convert_VkBindAccelerationStructureMemoryInfoKHR_array_win_to_host(const VkBindAccelerationStructureMemoryInfoKHR *in, uint32_t count)
{
    VkBindAccelerationStructureMemoryInfoKHR_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].accelerationStructure = in[i].accelerationStructure;
        out[i].memory = in[i].memory;
        out[i].memoryOffset = in[i].memoryOffset;
        out[i].deviceIndexCount = in[i].deviceIndexCount;
        out[i].pDeviceIndices = in[i].pDeviceIndices;
    }

    return out;
}

static inline void free_VkBindAccelerationStructureMemoryInfoKHR_array(VkBindAccelerationStructureMemoryInfoKHR_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkBindBufferMemoryInfo_host *convert_VkBindBufferMemoryInfo_array_win_to_host(const VkBindBufferMemoryInfo *in, uint32_t count)
{
    VkBindBufferMemoryInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].buffer = in[i].buffer;
        out[i].memory = in[i].memory;
        out[i].memoryOffset = in[i].memoryOffset;
    }

    return out;
}

static inline void free_VkBindBufferMemoryInfo_array(VkBindBufferMemoryInfo_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkBindImageMemoryInfo_host *convert_VkBindImageMemoryInfo_array_win_to_host(const VkBindImageMemoryInfo *in, uint32_t count)
{
    VkBindImageMemoryInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].image = in[i].image;
        out[i].memory = in[i].memory;
        out[i].memoryOffset = in[i].memoryOffset;
    }

    return out;
}

static inline void free_VkBindImageMemoryInfo_array(VkBindImageMemoryInfo_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkConditionalRenderingBeginInfoEXT_win_to_host(const VkConditionalRenderingBeginInfoEXT *in, VkConditionalRenderingBeginInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
    out->offset = in->offset;
    out->flags = in->flags;
}

static inline void convert_VkRenderPassBeginInfo_win_to_host(const VkRenderPassBeginInfo *in, VkRenderPassBeginInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->renderPass = in->renderPass;
    out->framebuffer = in->framebuffer;
    out->renderArea = in->renderArea;
    out->clearValueCount = in->clearValueCount;
    out->pClearValues = in->pClearValues;
}

static inline void convert_VkBlitImageInfo2KHR_win_to_host(const VkBlitImageInfo2KHR *in, VkBlitImageInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcImage = in->srcImage;
    out->srcImageLayout = in->srcImageLayout;
    out->dstImage = in->dstImage;
    out->dstImageLayout = in->dstImageLayout;
    out->regionCount = in->regionCount;
    out->pRegions = in->pRegions;
    out->filter = in->filter;
}

static inline void convert_VkGeometryTrianglesNV_win_to_host(const VkGeometryTrianglesNV *in, VkGeometryTrianglesNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->vertexData = in->vertexData;
    out->vertexOffset = in->vertexOffset;
    out->vertexCount = in->vertexCount;
    out->vertexStride = in->vertexStride;
    out->vertexFormat = in->vertexFormat;
    out->indexData = in->indexData;
    out->indexOffset = in->indexOffset;
    out->indexCount = in->indexCount;
    out->indexType = in->indexType;
    out->transformData = in->transformData;
    out->transformOffset = in->transformOffset;
}

static inline void convert_VkGeometryAABBNV_win_to_host(const VkGeometryAABBNV *in, VkGeometryAABBNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->aabbData = in->aabbData;
    out->numAABBs = in->numAABBs;
    out->stride = in->stride;
    out->offset = in->offset;
}

static inline void convert_VkGeometryDataNV_win_to_host(const VkGeometryDataNV *in, VkGeometryDataNV_host *out)
{
    if (!in) return;

    convert_VkGeometryTrianglesNV_win_to_host(&in->triangles, &out->triangles);
    convert_VkGeometryAABBNV_win_to_host(&in->aabbs, &out->aabbs);
}

static inline VkGeometryNV_host *convert_VkGeometryNV_array_win_to_host(const VkGeometryNV *in, uint32_t count)
{
    VkGeometryNV_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].geometryType = in[i].geometryType;
        convert_VkGeometryDataNV_win_to_host(&in[i].geometry, &out[i].geometry);
        out[i].flags = in[i].flags;
    }

    return out;
}

static inline void free_VkGeometryNV_array(VkGeometryNV_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkAccelerationStructureInfoNV_win_to_host(const VkAccelerationStructureInfoNV *in, VkAccelerationStructureInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->flags = in->flags;
    out->instanceCount = in->instanceCount;
    out->geometryCount = in->geometryCount;
    out->pGeometries = convert_VkGeometryNV_array_win_to_host(in->pGeometries, in->geometryCount);
}

static inline void free_VkAccelerationStructureInfoNV(VkAccelerationStructureInfoNV_host *in)
{
    free_VkGeometryNV_array((VkGeometryNV_host *)in->pGeometries, in->geometryCount);
}

static inline VkBufferCopy_host *convert_VkBufferCopy_array_win_to_host(const VkBufferCopy *in, uint32_t count)
{
    VkBufferCopy_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].srcOffset = in[i].srcOffset;
        out[i].dstOffset = in[i].dstOffset;
        out[i].size = in[i].size;
    }

    return out;
}

static inline void free_VkBufferCopy_array(VkBufferCopy_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkBufferCopy2KHR_host *convert_VkBufferCopy2KHR_array_win_to_host(const VkBufferCopy2KHR *in, uint32_t count)
{
    VkBufferCopy2KHR_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].srcOffset = in[i].srcOffset;
        out[i].dstOffset = in[i].dstOffset;
        out[i].size = in[i].size;
    }

    return out;
}

static inline void free_VkBufferCopy2KHR_array(VkBufferCopy2KHR_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkCopyBufferInfo2KHR_win_to_host(const VkCopyBufferInfo2KHR *in, VkCopyBufferInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcBuffer = in->srcBuffer;
    out->dstBuffer = in->dstBuffer;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferCopy2KHR_array_win_to_host(in->pRegions, in->regionCount);
}

static inline void free_VkCopyBufferInfo2KHR(VkCopyBufferInfo2KHR_host *in)
{
    free_VkBufferCopy2KHR_array((VkBufferCopy2KHR_host *)in->pRegions, in->regionCount);
}

static inline VkBufferImageCopy_host *convert_VkBufferImageCopy_array_win_to_host(const VkBufferImageCopy *in, uint32_t count)
{
    VkBufferImageCopy_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].bufferOffset = in[i].bufferOffset;
        out[i].bufferRowLength = in[i].bufferRowLength;
        out[i].bufferImageHeight = in[i].bufferImageHeight;
        out[i].imageSubresource = in[i].imageSubresource;
        out[i].imageOffset = in[i].imageOffset;
        out[i].imageExtent = in[i].imageExtent;
    }

    return out;
}

static inline void free_VkBufferImageCopy_array(VkBufferImageCopy_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkBufferImageCopy2KHR_host *convert_VkBufferImageCopy2KHR_array_win_to_host(const VkBufferImageCopy2KHR *in, uint32_t count)
{
    VkBufferImageCopy2KHR_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].bufferOffset = in[i].bufferOffset;
        out[i].bufferRowLength = in[i].bufferRowLength;
        out[i].bufferImageHeight = in[i].bufferImageHeight;
        out[i].imageSubresource = in[i].imageSubresource;
        out[i].imageOffset = in[i].imageOffset;
        out[i].imageExtent = in[i].imageExtent;
    }

    return out;
}

static inline void free_VkBufferImageCopy2KHR_array(VkBufferImageCopy2KHR_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkCopyBufferToImageInfo2KHR_win_to_host(const VkCopyBufferToImageInfo2KHR *in, VkCopyBufferToImageInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcBuffer = in->srcBuffer;
    out->dstImage = in->dstImage;
    out->dstImageLayout = in->dstImageLayout;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferImageCopy2KHR_array_win_to_host(in->pRegions, in->regionCount);
}

static inline void free_VkCopyBufferToImageInfo2KHR(VkCopyBufferToImageInfo2KHR_host *in)
{
    free_VkBufferImageCopy2KHR_array((VkBufferImageCopy2KHR_host *)in->pRegions, in->regionCount);
}

static inline void convert_VkCopyImageInfo2KHR_win_to_host(const VkCopyImageInfo2KHR *in, VkCopyImageInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcImage = in->srcImage;
    out->srcImageLayout = in->srcImageLayout;
    out->dstImage = in->dstImage;
    out->dstImageLayout = in->dstImageLayout;
    out->regionCount = in->regionCount;
    out->pRegions = in->pRegions;
}

static inline void convert_VkCopyImageToBufferInfo2KHR_win_to_host(const VkCopyImageToBufferInfo2KHR *in, VkCopyImageToBufferInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcImage = in->srcImage;
    out->srcImageLayout = in->srcImageLayout;
    out->dstBuffer = in->dstBuffer;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferImageCopy2KHR_array_win_to_host(in->pRegions, in->regionCount);
}

static inline void free_VkCopyImageToBufferInfo2KHR(VkCopyImageToBufferInfo2KHR_host *in)
{
    free_VkBufferImageCopy2KHR_array((VkBufferImageCopy2KHR_host *)in->pRegions, in->regionCount);
}

static inline VkIndirectCommandsStreamNV_host *convert_VkIndirectCommandsStreamNV_array_win_to_host(const VkIndirectCommandsStreamNV *in, uint32_t count)
{
    VkIndirectCommandsStreamNV_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].offset = in[i].offset;
    }

    return out;
}

static inline void free_VkIndirectCommandsStreamNV_array(VkIndirectCommandsStreamNV_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkGeneratedCommandsInfoNV_win_to_host(const VkGeneratedCommandsInfoNV *in, VkGeneratedCommandsInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipelineBindPoint = in->pipelineBindPoint;
    out->pipeline = in->pipeline;
    out->indirectCommandsLayout = in->indirectCommandsLayout;
    out->streamCount = in->streamCount;
    out->pStreams = convert_VkIndirectCommandsStreamNV_array_win_to_host(in->pStreams, in->streamCount);
    out->sequencesCount = in->sequencesCount;
    out->preprocessBuffer = in->preprocessBuffer;
    out->preprocessOffset = in->preprocessOffset;
    out->preprocessSize = in->preprocessSize;
    out->sequencesCountBuffer = in->sequencesCountBuffer;
    out->sequencesCountOffset = in->sequencesCountOffset;
    out->sequencesIndexBuffer = in->sequencesIndexBuffer;
    out->sequencesIndexOffset = in->sequencesIndexOffset;
}

static inline void free_VkGeneratedCommandsInfoNV(VkGeneratedCommandsInfoNV_host *in)
{
    free_VkIndirectCommandsStreamNV_array((VkIndirectCommandsStreamNV_host *)in->pStreams, in->streamCount);
}

static inline VkBufferMemoryBarrier_host *convert_VkBufferMemoryBarrier_array_win_to_host(const VkBufferMemoryBarrier *in, uint32_t count)
{
    VkBufferMemoryBarrier_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].srcAccessMask = in[i].srcAccessMask;
        out[i].dstAccessMask = in[i].dstAccessMask;
        out[i].srcQueueFamilyIndex = in[i].srcQueueFamilyIndex;
        out[i].dstQueueFamilyIndex = in[i].dstQueueFamilyIndex;
        out[i].buffer = in[i].buffer;
        out[i].offset = in[i].offset;
        out[i].size = in[i].size;
    }

    return out;
}

static inline void free_VkBufferMemoryBarrier_array(VkBufferMemoryBarrier_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkImageMemoryBarrier_host *convert_VkImageMemoryBarrier_array_win_to_host(const VkImageMemoryBarrier *in, uint32_t count)
{
    VkImageMemoryBarrier_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].srcAccessMask = in[i].srcAccessMask;
        out[i].dstAccessMask = in[i].dstAccessMask;
        out[i].oldLayout = in[i].oldLayout;
        out[i].newLayout = in[i].newLayout;
        out[i].srcQueueFamilyIndex = in[i].srcQueueFamilyIndex;
        out[i].dstQueueFamilyIndex = in[i].dstQueueFamilyIndex;
        out[i].image = in[i].image;
        out[i].subresourceRange = in[i].subresourceRange;
    }

    return out;
}

static inline void free_VkImageMemoryBarrier_array(VkImageMemoryBarrier_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkDescriptorImageInfo_host *convert_VkDescriptorImageInfo_array_win_to_host(const VkDescriptorImageInfo *in, uint32_t count)
{
    VkDescriptorImageInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sampler = in[i].sampler;
        out[i].imageView = in[i].imageView;
        out[i].imageLayout = in[i].imageLayout;
    }

    return out;
}

static inline void free_VkDescriptorImageInfo_array(VkDescriptorImageInfo_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkDescriptorBufferInfo_host *convert_VkDescriptorBufferInfo_array_win_to_host(const VkDescriptorBufferInfo *in, uint32_t count)
{
    VkDescriptorBufferInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].offset = in[i].offset;
        out[i].range = in[i].range;
    }

    return out;
}

static inline void free_VkDescriptorBufferInfo_array(VkDescriptorBufferInfo_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkWriteDescriptorSet_host *convert_VkWriteDescriptorSet_array_win_to_host(const VkWriteDescriptorSet *in, uint32_t count)
{
    VkWriteDescriptorSet_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].dstSet = in[i].dstSet;
        out[i].dstBinding = in[i].dstBinding;
        out[i].dstArrayElement = in[i].dstArrayElement;
        out[i].descriptorCount = in[i].descriptorCount;
        out[i].descriptorType = in[i].descriptorType;
        out[i].pImageInfo = convert_VkDescriptorImageInfo_array_win_to_host(in[i].pImageInfo, in[i].descriptorCount);
        out[i].pBufferInfo = convert_VkDescriptorBufferInfo_array_win_to_host(in[i].pBufferInfo, in[i].descriptorCount);
        out[i].pTexelBufferView = in[i].pTexelBufferView;
    }

    return out;
}

static inline void free_VkWriteDescriptorSet_array(VkWriteDescriptorSet_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkDescriptorImageInfo_array((VkDescriptorImageInfo_host *)in[i].pImageInfo, in[i].descriptorCount);
        free_VkDescriptorBufferInfo_array((VkDescriptorBufferInfo_host *)in[i].pBufferInfo, in[i].descriptorCount);
    }
    heap_free(in);
}

static inline void convert_VkResolveImageInfo2KHR_win_to_host(const VkResolveImageInfo2KHR *in, VkResolveImageInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcImage = in->srcImage;
    out->srcImageLayout = in->srcImageLayout;
    out->dstImage = in->dstImage;
    out->dstImageLayout = in->dstImageLayout;
    out->regionCount = in->regionCount;
    out->pRegions = in->pRegions;
}

static inline void convert_VkPerformanceMarkerInfoINTEL_win_to_host(const VkPerformanceMarkerInfoINTEL *in, VkPerformanceMarkerInfoINTEL_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->marker = in->marker;
}

static inline void convert_VkPerformanceOverrideInfoINTEL_win_to_host(const VkPerformanceOverrideInfoINTEL *in, VkPerformanceOverrideInfoINTEL_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->enable = in->enable;
    out->parameter = in->parameter;
}

static inline void convert_VkAccelerationStructureCreateInfoNV_win_to_host(const VkAccelerationStructureCreateInfoNV *in, VkAccelerationStructureCreateInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->compactedSize = in->compactedSize;
    convert_VkAccelerationStructureInfoNV_win_to_host(&in->info, &out->info);
}

static inline void convert_VkBufferCreateInfo_win_to_host(const VkBufferCreateInfo *in, VkBufferCreateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->size = in->size;
    out->usage = in->usage;
    out->sharingMode = in->sharingMode;
    out->queueFamilyIndexCount = in->queueFamilyIndexCount;
    out->pQueueFamilyIndices = in->pQueueFamilyIndices;
}

static inline void convert_VkBufferViewCreateInfo_win_to_host(const VkBufferViewCreateInfo *in, VkBufferViewCreateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->buffer = in->buffer;
    out->format = in->format;
    out->offset = in->offset;
    out->range = in->range;
}

static inline void convert_VkPipelineShaderStageCreateInfo_win_to_host(const VkPipelineShaderStageCreateInfo *in, VkPipelineShaderStageCreateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->stage = in->stage;
    out->module = in->module;
    out->pName = in->pName;
    out->pSpecializationInfo = in->pSpecializationInfo;
}

static inline VkComputePipelineCreateInfo_host *convert_VkComputePipelineCreateInfo_array_win_to_host(const VkComputePipelineCreateInfo *in, uint32_t count)
{
    VkComputePipelineCreateInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        convert_VkPipelineShaderStageCreateInfo_win_to_host(&in[i].stage, &out[i].stage);
        out[i].layout = in[i].layout;
        out[i].basePipelineHandle = in[i].basePipelineHandle;
        out[i].basePipelineIndex = in[i].basePipelineIndex;
    }

    return out;
}

static inline void free_VkComputePipelineCreateInfo_array(VkComputePipelineCreateInfo_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkDescriptorUpdateTemplateCreateInfo_win_to_host(const VkDescriptorUpdateTemplateCreateInfo *in, VkDescriptorUpdateTemplateCreateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->descriptorUpdateEntryCount = in->descriptorUpdateEntryCount;
    out->pDescriptorUpdateEntries = in->pDescriptorUpdateEntries;
    out->templateType = in->templateType;
    out->descriptorSetLayout = in->descriptorSetLayout;
    out->pipelineBindPoint = in->pipelineBindPoint;
    out->pipelineLayout = in->pipelineLayout;
    out->set = in->set;
}

static inline void convert_VkFramebufferCreateInfo_win_to_host(const VkFramebufferCreateInfo *in, VkFramebufferCreateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->renderPass = in->renderPass;
    out->attachmentCount = in->attachmentCount;
    out->pAttachments = in->pAttachments;
    out->width = in->width;
    out->height = in->height;
    out->layers = in->layers;
}

static inline VkPipelineShaderStageCreateInfo_host *convert_VkPipelineShaderStageCreateInfo_array_win_to_host(const VkPipelineShaderStageCreateInfo *in, uint32_t count)
{
    VkPipelineShaderStageCreateInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].stage = in[i].stage;
        out[i].module = in[i].module;
        out[i].pName = in[i].pName;
        out[i].pSpecializationInfo = in[i].pSpecializationInfo;
    }

    return out;
}

static inline void free_VkPipelineShaderStageCreateInfo_array(VkPipelineShaderStageCreateInfo_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkGraphicsPipelineCreateInfo_host *convert_VkGraphicsPipelineCreateInfo_array_win_to_host(const VkGraphicsPipelineCreateInfo *in, uint32_t count)
{
    VkGraphicsPipelineCreateInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].stageCount = in[i].stageCount;
        out[i].pStages = convert_VkPipelineShaderStageCreateInfo_array_win_to_host(in[i].pStages, in[i].stageCount);
        out[i].pVertexInputState = in[i].pVertexInputState;
        out[i].pInputAssemblyState = in[i].pInputAssemblyState;
        out[i].pTessellationState = in[i].pTessellationState;
        out[i].pViewportState = in[i].pViewportState;
        out[i].pRasterizationState = in[i].pRasterizationState;
        out[i].pMultisampleState = in[i].pMultisampleState;
        out[i].pDepthStencilState = in[i].pDepthStencilState;
        out[i].pColorBlendState = in[i].pColorBlendState;
        out[i].pDynamicState = in[i].pDynamicState;
        out[i].layout = in[i].layout;
        out[i].renderPass = in[i].renderPass;
        out[i].subpass = in[i].subpass;
        out[i].basePipelineHandle = in[i].basePipelineHandle;
        out[i].basePipelineIndex = in[i].basePipelineIndex;
    }

    return out;
}

static inline void free_VkGraphicsPipelineCreateInfo_array(VkGraphicsPipelineCreateInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkPipelineShaderStageCreateInfo_array((VkPipelineShaderStageCreateInfo_host *)in[i].pStages, in[i].stageCount);
    }
    heap_free(in);
}

static inline void convert_VkImageViewCreateInfo_win_to_host(const VkImageViewCreateInfo *in, VkImageViewCreateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->image = in->image;
    out->viewType = in->viewType;
    out->format = in->format;
    out->components = in->components;
    out->subresourceRange = in->subresourceRange;
}

static inline VkIndirectCommandsLayoutTokenNV_host *convert_VkIndirectCommandsLayoutTokenNV_array_win_to_host(const VkIndirectCommandsLayoutTokenNV *in, uint32_t count)
{
    VkIndirectCommandsLayoutTokenNV_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].tokenType = in[i].tokenType;
        out[i].stream = in[i].stream;
        out[i].offset = in[i].offset;
        out[i].vertexBindingUnit = in[i].vertexBindingUnit;
        out[i].vertexDynamicStride = in[i].vertexDynamicStride;
        out[i].pushconstantPipelineLayout = in[i].pushconstantPipelineLayout;
        out[i].pushconstantShaderStageFlags = in[i].pushconstantShaderStageFlags;
        out[i].pushconstantOffset = in[i].pushconstantOffset;
        out[i].pushconstantSize = in[i].pushconstantSize;
        out[i].indirectStateFlags = in[i].indirectStateFlags;
        out[i].indexTypeCount = in[i].indexTypeCount;
        out[i].pIndexTypes = in[i].pIndexTypes;
        out[i].pIndexTypeValues = in[i].pIndexTypeValues;
    }

    return out;
}

static inline void free_VkIndirectCommandsLayoutTokenNV_array(VkIndirectCommandsLayoutTokenNV_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkIndirectCommandsLayoutCreateInfoNV_win_to_host(const VkIndirectCommandsLayoutCreateInfoNV *in, VkIndirectCommandsLayoutCreateInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->pipelineBindPoint = in->pipelineBindPoint;
    out->tokenCount = in->tokenCount;
    out->pTokens = convert_VkIndirectCommandsLayoutTokenNV_array_win_to_host(in->pTokens, in->tokenCount);
    out->streamCount = in->streamCount;
    out->pStreamStrides = in->pStreamStrides;
}

static inline void free_VkIndirectCommandsLayoutCreateInfoNV(VkIndirectCommandsLayoutCreateInfoNV_host *in)
{
    free_VkIndirectCommandsLayoutTokenNV_array((VkIndirectCommandsLayoutTokenNV_host *)in->pTokens, in->tokenCount);
}

static inline VkRayTracingPipelineCreateInfoNV_host *convert_VkRayTracingPipelineCreateInfoNV_array_win_to_host(const VkRayTracingPipelineCreateInfoNV *in, uint32_t count)
{
    VkRayTracingPipelineCreateInfoNV_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].stageCount = in[i].stageCount;
        out[i].pStages = convert_VkPipelineShaderStageCreateInfo_array_win_to_host(in[i].pStages, in[i].stageCount);
        out[i].groupCount = in[i].groupCount;
        out[i].pGroups = in[i].pGroups;
        out[i].maxRecursionDepth = in[i].maxRecursionDepth;
        out[i].layout = in[i].layout;
        out[i].basePipelineHandle = in[i].basePipelineHandle;
        out[i].basePipelineIndex = in[i].basePipelineIndex;
    }

    return out;
}

static inline void free_VkRayTracingPipelineCreateInfoNV_array(VkRayTracingPipelineCreateInfoNV_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkPipelineShaderStageCreateInfo_array((VkPipelineShaderStageCreateInfo_host *)in[i].pStages, in[i].stageCount);
    }
    heap_free(in);
}

static inline void convert_VkSwapchainCreateInfoKHR_win_to_host(const VkSwapchainCreateInfoKHR *in, VkSwapchainCreateInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->surface = in->surface;
    out->minImageCount = in->minImageCount;
    out->imageFormat = in->imageFormat;
    out->imageColorSpace = in->imageColorSpace;
    out->imageExtent = in->imageExtent;
    out->imageArrayLayers = in->imageArrayLayers;
    out->imageUsage = in->imageUsage;
    out->imageSharingMode = in->imageSharingMode;
    out->queueFamilyIndexCount = in->queueFamilyIndexCount;
    out->pQueueFamilyIndices = in->pQueueFamilyIndices;
    out->preTransform = in->preTransform;
    out->compositeAlpha = in->compositeAlpha;
    out->presentMode = in->presentMode;
    out->clipped = in->clipped;
    out->oldSwapchain = in->oldSwapchain;
}

static inline void convert_VkDebugMarkerObjectNameInfoEXT_win_to_host(const VkDebugMarkerObjectNameInfoEXT *in, VkDebugMarkerObjectNameInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->object = in->object;
    out->pObjectName = in->pObjectName;
}

static inline void convert_VkDebugMarkerObjectTagInfoEXT_win_to_host(const VkDebugMarkerObjectTagInfoEXT *in, VkDebugMarkerObjectTagInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->object = in->object;
    out->tagName = in->tagName;
    out->tagSize = in->tagSize;
    out->pTag = in->pTag;
}

static inline VkMappedMemoryRange_host *convert_VkMappedMemoryRange_array_win_to_host(const VkMappedMemoryRange *in, uint32_t count)
{
    VkMappedMemoryRange_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].memory = in[i].memory;
        out[i].offset = in[i].offset;
        out[i].size = in[i].size;
    }

    return out;
}

static inline void free_VkMappedMemoryRange_array(VkMappedMemoryRange_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkAccelerationStructureMemoryRequirementsInfoNV_win_to_host(const VkAccelerationStructureMemoryRequirementsInfoNV *in, VkAccelerationStructureMemoryRequirementsInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->accelerationStructure = in->accelerationStructure;
}

static inline void convert_VkMemoryRequirements_host_to_win(const VkMemoryRequirements_host *in, VkMemoryRequirements *out)
{
    if (!in) return;

    out->size = in->size;
    out->alignment = in->alignment;
    out->memoryTypeBits = in->memoryTypeBits;
}

static inline void convert_VkMemoryRequirements2KHR_win_to_host(const VkMemoryRequirements2KHR *in, VkMemoryRequirements2KHR_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}

static inline void convert_VkMemoryRequirements2KHR_host_to_win(const VkMemoryRequirements2KHR_host *in, VkMemoryRequirements2KHR *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkMemoryRequirements_host_to_win(&in->memoryRequirements, &out->memoryRequirements);
}

static inline void convert_VkBufferDeviceAddressInfo_win_to_host(const VkBufferDeviceAddressInfo *in, VkBufferDeviceAddressInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
}

static inline void convert_VkBufferMemoryRequirementsInfo2_win_to_host(const VkBufferMemoryRequirementsInfo2 *in, VkBufferMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
}

static inline void convert_VkMemoryRequirements2_win_to_host(const VkMemoryRequirements2 *in, VkMemoryRequirements2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}

static inline void convert_VkMemoryRequirements2_host_to_win(const VkMemoryRequirements2_host *in, VkMemoryRequirements2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkMemoryRequirements_host_to_win(&in->memoryRequirements, &out->memoryRequirements);
}

static inline void convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win_to_host(const VkDeviceMemoryOpaqueCaptureAddressInfo *in, VkDeviceMemoryOpaqueCaptureAddressInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->memory = in->memory;
}

static inline void convert_VkGeneratedCommandsMemoryRequirementsInfoNV_win_to_host(const VkGeneratedCommandsMemoryRequirementsInfoNV *in, VkGeneratedCommandsMemoryRequirementsInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipelineBindPoint = in->pipelineBindPoint;
    out->pipeline = in->pipeline;
    out->indirectCommandsLayout = in->indirectCommandsLayout;
    out->maxSequencesCount = in->maxSequencesCount;
}

static inline void convert_VkImageMemoryRequirementsInfo2_win_to_host(const VkImageMemoryRequirementsInfo2 *in, VkImageMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->image = in->image;
}

static inline void convert_VkImageSparseMemoryRequirementsInfo2_win_to_host(const VkImageSparseMemoryRequirementsInfo2 *in, VkImageSparseMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->image = in->image;
}

static inline void convert_VkSubresourceLayout_host_to_win(const VkSubresourceLayout_host *in, VkSubresourceLayout *out)
{
    if (!in) return;

    out->offset = in->offset;
    out->size = in->size;
    out->rowPitch = in->rowPitch;
    out->arrayPitch = in->arrayPitch;
    out->depthPitch = in->depthPitch;
}

static inline void convert_VkImageFormatProperties_host_to_win(const VkImageFormatProperties_host *in, VkImageFormatProperties *out)
{
    if (!in) return;

    out->maxExtent = in->maxExtent;
    out->maxMipLevels = in->maxMipLevels;
    out->maxArrayLayers = in->maxArrayLayers;
    out->sampleCounts = in->sampleCounts;
    out->maxResourceSize = in->maxResourceSize;
}

static inline void convert_VkImageFormatProperties2_win_to_host(const VkImageFormatProperties2 *in, VkImageFormatProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}

static inline void convert_VkImageFormatProperties2_host_to_win(const VkImageFormatProperties2_host *in, VkImageFormatProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkImageFormatProperties_host_to_win(&in->imageFormatProperties, &out->imageFormatProperties);
}

static inline void convert_VkMemoryHeap_static_array_host_to_win(const VkMemoryHeap_host *in, VkMemoryHeap *out, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        out[i].size = in[i].size;
        out[i].flags = in[i].flags;
    }
}

static inline void convert_VkPhysicalDeviceMemoryProperties_host_to_win(const VkPhysicalDeviceMemoryProperties_host *in, VkPhysicalDeviceMemoryProperties *out)
{
    if (!in) return;

    out->memoryTypeCount = in->memoryTypeCount;
    memcpy(out->memoryTypes, in->memoryTypes, VK_MAX_MEMORY_TYPES * sizeof(VkMemoryType));
    out->memoryHeapCount = in->memoryHeapCount;
    convert_VkMemoryHeap_static_array_host_to_win(in->memoryHeaps, out->memoryHeaps, VK_MAX_MEMORY_HEAPS);
}

static inline void convert_VkPhysicalDeviceMemoryProperties2_win_to_host(const VkPhysicalDeviceMemoryProperties2 *in, VkPhysicalDeviceMemoryProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}

static inline void convert_VkPhysicalDeviceMemoryProperties2_host_to_win(const VkPhysicalDeviceMemoryProperties2_host *in, VkPhysicalDeviceMemoryProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkPhysicalDeviceMemoryProperties_host_to_win(&in->memoryProperties, &out->memoryProperties);
}

static inline void convert_VkPhysicalDeviceLimits_host_to_win(const VkPhysicalDeviceLimits_host *in, VkPhysicalDeviceLimits *out)
{
    if (!in) return;

    out->maxImageDimension1D = in->maxImageDimension1D;
    out->maxImageDimension2D = in->maxImageDimension2D;
    out->maxImageDimension3D = in->maxImageDimension3D;
    out->maxImageDimensionCube = in->maxImageDimensionCube;
    out->maxImageArrayLayers = in->maxImageArrayLayers;
    out->maxTexelBufferElements = in->maxTexelBufferElements;
    out->maxUniformBufferRange = in->maxUniformBufferRange;
    out->maxStorageBufferRange = in->maxStorageBufferRange;
    out->maxPushConstantsSize = in->maxPushConstantsSize;
    out->maxMemoryAllocationCount = in->maxMemoryAllocationCount;
    out->maxSamplerAllocationCount = in->maxSamplerAllocationCount;
    out->bufferImageGranularity = in->bufferImageGranularity;
    out->sparseAddressSpaceSize = in->sparseAddressSpaceSize;
    out->maxBoundDescriptorSets = in->maxBoundDescriptorSets;
    out->maxPerStageDescriptorSamplers = in->maxPerStageDescriptorSamplers;
    out->maxPerStageDescriptorUniformBuffers = in->maxPerStageDescriptorUniformBuffers;
    out->maxPerStageDescriptorStorageBuffers = in->maxPerStageDescriptorStorageBuffers;
    out->maxPerStageDescriptorSampledImages = in->maxPerStageDescriptorSampledImages;
    out->maxPerStageDescriptorStorageImages = in->maxPerStageDescriptorStorageImages;
    out->maxPerStageDescriptorInputAttachments = in->maxPerStageDescriptorInputAttachments;
    out->maxPerStageResources = in->maxPerStageResources;
    out->maxDescriptorSetSamplers = in->maxDescriptorSetSamplers;
    out->maxDescriptorSetUniformBuffers = in->maxDescriptorSetUniformBuffers;
    out->maxDescriptorSetUniformBuffersDynamic = in->maxDescriptorSetUniformBuffersDynamic;
    out->maxDescriptorSetStorageBuffers = in->maxDescriptorSetStorageBuffers;
    out->maxDescriptorSetStorageBuffersDynamic = in->maxDescriptorSetStorageBuffersDynamic;
    out->maxDescriptorSetSampledImages = in->maxDescriptorSetSampledImages;
    out->maxDescriptorSetStorageImages = in->maxDescriptorSetStorageImages;
    out->maxDescriptorSetInputAttachments = in->maxDescriptorSetInputAttachments;
    out->maxVertexInputAttributes = in->maxVertexInputAttributes;
    out->maxVertexInputBindings = in->maxVertexInputBindings;
    out->maxVertexInputAttributeOffset = in->maxVertexInputAttributeOffset;
    out->maxVertexInputBindingStride = in->maxVertexInputBindingStride;
    out->maxVertexOutputComponents = in->maxVertexOutputComponents;
    out->maxTessellationGenerationLevel = in->maxTessellationGenerationLevel;
    out->maxTessellationPatchSize = in->maxTessellationPatchSize;
    out->maxTessellationControlPerVertexInputComponents = in->maxTessellationControlPerVertexInputComponents;
    out->maxTessellationControlPerVertexOutputComponents = in->maxTessellationControlPerVertexOutputComponents;
    out->maxTessellationControlPerPatchOutputComponents = in->maxTessellationControlPerPatchOutputComponents;
    out->maxTessellationControlTotalOutputComponents = in->maxTessellationControlTotalOutputComponents;
    out->maxTessellationEvaluationInputComponents = in->maxTessellationEvaluationInputComponents;
    out->maxTessellationEvaluationOutputComponents = in->maxTessellationEvaluationOutputComponents;
    out->maxGeometryShaderInvocations = in->maxGeometryShaderInvocations;
    out->maxGeometryInputComponents = in->maxGeometryInputComponents;
    out->maxGeometryOutputComponents = in->maxGeometryOutputComponents;
    out->maxGeometryOutputVertices = in->maxGeometryOutputVertices;
    out->maxGeometryTotalOutputComponents = in->maxGeometryTotalOutputComponents;
    out->maxFragmentInputComponents = in->maxFragmentInputComponents;
    out->maxFragmentOutputAttachments = in->maxFragmentOutputAttachments;
    out->maxFragmentDualSrcAttachments = in->maxFragmentDualSrcAttachments;
    out->maxFragmentCombinedOutputResources = in->maxFragmentCombinedOutputResources;
    out->maxComputeSharedMemorySize = in->maxComputeSharedMemorySize;
    memcpy(out->maxComputeWorkGroupCount, in->maxComputeWorkGroupCount, 3 * sizeof(uint32_t));
    out->maxComputeWorkGroupInvocations = in->maxComputeWorkGroupInvocations;
    memcpy(out->maxComputeWorkGroupSize, in->maxComputeWorkGroupSize, 3 * sizeof(uint32_t));
    out->subPixelPrecisionBits = in->subPixelPrecisionBits;
    out->subTexelPrecisionBits = in->subTexelPrecisionBits;
    out->mipmapPrecisionBits = in->mipmapPrecisionBits;
    out->maxDrawIndexedIndexValue = in->maxDrawIndexedIndexValue;
    out->maxDrawIndirectCount = in->maxDrawIndirectCount;
    out->maxSamplerLodBias = in->maxSamplerLodBias;
    out->maxSamplerAnisotropy = in->maxSamplerAnisotropy;
    out->maxViewports = in->maxViewports;
    memcpy(out->maxViewportDimensions, in->maxViewportDimensions, 2 * sizeof(uint32_t));
    memcpy(out->viewportBoundsRange, in->viewportBoundsRange, 2 * sizeof(float));
    out->viewportSubPixelBits = in->viewportSubPixelBits;
    out->minMemoryMapAlignment = in->minMemoryMapAlignment;
    out->minTexelBufferOffsetAlignment = in->minTexelBufferOffsetAlignment;
    out->minUniformBufferOffsetAlignment = in->minUniformBufferOffsetAlignment;
    out->minStorageBufferOffsetAlignment = in->minStorageBufferOffsetAlignment;
    out->minTexelOffset = in->minTexelOffset;
    out->maxTexelOffset = in->maxTexelOffset;
    out->minTexelGatherOffset = in->minTexelGatherOffset;
    out->maxTexelGatherOffset = in->maxTexelGatherOffset;
    out->minInterpolationOffset = in->minInterpolationOffset;
    out->maxInterpolationOffset = in->maxInterpolationOffset;
    out->subPixelInterpolationOffsetBits = in->subPixelInterpolationOffsetBits;
    out->maxFramebufferWidth = in->maxFramebufferWidth;
    out->maxFramebufferHeight = in->maxFramebufferHeight;
    out->maxFramebufferLayers = in->maxFramebufferLayers;
    out->framebufferColorSampleCounts = in->framebufferColorSampleCounts;
    out->framebufferDepthSampleCounts = in->framebufferDepthSampleCounts;
    out->framebufferStencilSampleCounts = in->framebufferStencilSampleCounts;
    out->framebufferNoAttachmentsSampleCounts = in->framebufferNoAttachmentsSampleCounts;
    out->maxColorAttachments = in->maxColorAttachments;
    out->sampledImageColorSampleCounts = in->sampledImageColorSampleCounts;
    out->sampledImageIntegerSampleCounts = in->sampledImageIntegerSampleCounts;
    out->sampledImageDepthSampleCounts = in->sampledImageDepthSampleCounts;
    out->sampledImageStencilSampleCounts = in->sampledImageStencilSampleCounts;
    out->storageImageSampleCounts = in->storageImageSampleCounts;
    out->maxSampleMaskWords = in->maxSampleMaskWords;
    out->timestampComputeAndGraphics = in->timestampComputeAndGraphics;
    out->timestampPeriod = in->timestampPeriod;
    out->maxClipDistances = in->maxClipDistances;
    out->maxCullDistances = in->maxCullDistances;
    out->maxCombinedClipAndCullDistances = in->maxCombinedClipAndCullDistances;
    out->discreteQueuePriorities = in->discreteQueuePriorities;
    memcpy(out->pointSizeRange, in->pointSizeRange, 2 * sizeof(float));
    memcpy(out->lineWidthRange, in->lineWidthRange, 2 * sizeof(float));
    out->pointSizeGranularity = in->pointSizeGranularity;
    out->lineWidthGranularity = in->lineWidthGranularity;
    out->strictLines = in->strictLines;
    out->standardSampleLocations = in->standardSampleLocations;
    out->optimalBufferCopyOffsetAlignment = in->optimalBufferCopyOffsetAlignment;
    out->optimalBufferCopyRowPitchAlignment = in->optimalBufferCopyRowPitchAlignment;
    out->nonCoherentAtomSize = in->nonCoherentAtomSize;
}

static inline void convert_VkPhysicalDeviceProperties_host_to_win(const VkPhysicalDeviceProperties_host *in, VkPhysicalDeviceProperties *out)
{
    if (!in) return;

    out->apiVersion = in->apiVersion;
    out->driverVersion = in->driverVersion;
    out->vendorID = in->vendorID;
    out->deviceID = in->deviceID;
    out->deviceType = in->deviceType;
    memcpy(out->deviceName, in->deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * sizeof(char));
    memcpy(out->pipelineCacheUUID, in->pipelineCacheUUID, VK_UUID_SIZE * sizeof(uint8_t));
    convert_VkPhysicalDeviceLimits_host_to_win(&in->limits, &out->limits);
    out->sparseProperties = in->sparseProperties;
}

static inline void convert_VkPhysicalDeviceProperties2_win_to_host(const VkPhysicalDeviceProperties2 *in, VkPhysicalDeviceProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}

static inline void convert_VkPhysicalDeviceProperties2_host_to_win(const VkPhysicalDeviceProperties2_host *in, VkPhysicalDeviceProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkPhysicalDeviceProperties_host_to_win(&in->properties, &out->properties);
}

static inline void convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(const VkPhysicalDeviceSurfaceInfo2KHR *in, VkPhysicalDeviceSurfaceInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->surface = in->surface;
}

static inline void convert_VkPipelineExecutableInfoKHR_win_to_host(const VkPipelineExecutableInfoKHR *in, VkPipelineExecutableInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
    out->executableIndex = in->executableIndex;
}

static inline void convert_VkPipelineInfoKHR_win_to_host(const VkPipelineInfoKHR *in, VkPipelineInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
}

static inline VkSparseMemoryBind_host *convert_VkSparseMemoryBind_array_win_to_host(const VkSparseMemoryBind *in, uint32_t count)
{
    VkSparseMemoryBind_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].resourceOffset = in[i].resourceOffset;
        out[i].size = in[i].size;
        out[i].memory = in[i].memory;
        out[i].memoryOffset = in[i].memoryOffset;
        out[i].flags = in[i].flags;
    }

    return out;
}

static inline void free_VkSparseMemoryBind_array(VkSparseMemoryBind_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkSparseBufferMemoryBindInfo_host *convert_VkSparseBufferMemoryBindInfo_array_win_to_host(const VkSparseBufferMemoryBindInfo *in, uint32_t count)
{
    VkSparseBufferMemoryBindInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseMemoryBind_array_win_to_host(in[i].pBinds, in[i].bindCount);
    }

    return out;
}

static inline void free_VkSparseBufferMemoryBindInfo_array(VkSparseBufferMemoryBindInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkSparseMemoryBind_array((VkSparseMemoryBind_host *)in[i].pBinds, in[i].bindCount);
    }
    heap_free(in);
}

static inline VkSparseImageOpaqueMemoryBindInfo_host *convert_VkSparseImageOpaqueMemoryBindInfo_array_win_to_host(const VkSparseImageOpaqueMemoryBindInfo *in, uint32_t count)
{
    VkSparseImageOpaqueMemoryBindInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].image = in[i].image;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseMemoryBind_array_win_to_host(in[i].pBinds, in[i].bindCount);
    }

    return out;
}

static inline void free_VkSparseImageOpaqueMemoryBindInfo_array(VkSparseImageOpaqueMemoryBindInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkSparseMemoryBind_array((VkSparseMemoryBind_host *)in[i].pBinds, in[i].bindCount);
    }
    heap_free(in);
}

static inline VkSparseImageMemoryBind_host *convert_VkSparseImageMemoryBind_array_win_to_host(const VkSparseImageMemoryBind *in, uint32_t count)
{
    VkSparseImageMemoryBind_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].subresource = in[i].subresource;
        out[i].offset = in[i].offset;
        out[i].extent = in[i].extent;
        out[i].memory = in[i].memory;
        out[i].memoryOffset = in[i].memoryOffset;
        out[i].flags = in[i].flags;
    }

    return out;
}

static inline void free_VkSparseImageMemoryBind_array(VkSparseImageMemoryBind_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline VkSparseImageMemoryBindInfo_host *convert_VkSparseImageMemoryBindInfo_array_win_to_host(const VkSparseImageMemoryBindInfo *in, uint32_t count)
{
    VkSparseImageMemoryBindInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].image = in[i].image;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseImageMemoryBind_array_win_to_host(in[i].pBinds, in[i].bindCount);
    }

    return out;
}

static inline void free_VkSparseImageMemoryBindInfo_array(VkSparseImageMemoryBindInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkSparseImageMemoryBind_array((VkSparseImageMemoryBind_host *)in[i].pBinds, in[i].bindCount);
    }
    heap_free(in);
}

static inline VkBindSparseInfo_host *convert_VkBindSparseInfo_array_win_to_host(const VkBindSparseInfo *in, uint32_t count)
{
    VkBindSparseInfo_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].waitSemaphoreCount = in[i].waitSemaphoreCount;
        out[i].pWaitSemaphores = in[i].pWaitSemaphores;
        out[i].bufferBindCount = in[i].bufferBindCount;
        out[i].pBufferBinds = convert_VkSparseBufferMemoryBindInfo_array_win_to_host(in[i].pBufferBinds, in[i].bufferBindCount);
        out[i].imageOpaqueBindCount = in[i].imageOpaqueBindCount;
        out[i].pImageOpaqueBinds = convert_VkSparseImageOpaqueMemoryBindInfo_array_win_to_host(in[i].pImageOpaqueBinds, in[i].imageOpaqueBindCount);
        out[i].imageBindCount = in[i].imageBindCount;
        out[i].pImageBinds = convert_VkSparseImageMemoryBindInfo_array_win_to_host(in[i].pImageBinds, in[i].imageBindCount);
        out[i].signalSemaphoreCount = in[i].signalSemaphoreCount;
        out[i].pSignalSemaphores = in[i].pSignalSemaphores;
    }

    return out;
}

static inline void free_VkBindSparseInfo_array(VkBindSparseInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkSparseBufferMemoryBindInfo_array((VkSparseBufferMemoryBindInfo_host *)in[i].pBufferBinds, in[i].bufferBindCount);
        free_VkSparseImageOpaqueMemoryBindInfo_array((VkSparseImageOpaqueMemoryBindInfo_host *)in[i].pImageOpaqueBinds, in[i].imageOpaqueBindCount);
        free_VkSparseImageMemoryBindInfo_array((VkSparseImageMemoryBindInfo_host *)in[i].pImageBinds, in[i].imageBindCount);
    }
    heap_free(in);
}

static inline void convert_VkDebugUtilsObjectNameInfoEXT_win_to_host(const VkDebugUtilsObjectNameInfoEXT *in, VkDebugUtilsObjectNameInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->objectHandle = in->objectHandle;
    out->pObjectName = in->pObjectName;
}

static inline void convert_VkDebugUtilsObjectTagInfoEXT_win_to_host(const VkDebugUtilsObjectTagInfoEXT *in, VkDebugUtilsObjectTagInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->objectHandle = in->objectHandle;
    out->tagName = in->tagName;
    out->tagSize = in->tagSize;
    out->pTag = in->pTag;
}

static inline void convert_VkSemaphoreSignalInfo_win_to_host(const VkSemaphoreSignalInfo *in, VkSemaphoreSignalInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->semaphore = in->semaphore;
    out->value = in->value;
}

static inline VkDebugUtilsObjectNameInfoEXT_host *convert_VkDebugUtilsObjectNameInfoEXT_array_win_to_host(const VkDebugUtilsObjectNameInfoEXT *in, uint32_t count)
{
    VkDebugUtilsObjectNameInfoEXT_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].objectType = in[i].objectType;
        out[i].objectHandle = in[i].objectHandle;
        out[i].pObjectName = in[i].pObjectName;
    }

    return out;
}

static inline void free_VkDebugUtilsObjectNameInfoEXT_array(VkDebugUtilsObjectNameInfoEXT_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

static inline void convert_VkDebugUtilsMessengerCallbackDataEXT_win_to_host(const VkDebugUtilsMessengerCallbackDataEXT *in, VkDebugUtilsMessengerCallbackDataEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->pMessageIdName = in->pMessageIdName;
    out->messageIdNumber = in->messageIdNumber;
    out->pMessage = in->pMessage;
    out->queueLabelCount = in->queueLabelCount;
    out->pQueueLabels = in->pQueueLabels;
    out->cmdBufLabelCount = in->cmdBufLabelCount;
    out->pCmdBufLabels = in->pCmdBufLabels;
    out->objectCount = in->objectCount;
    out->pObjects = convert_VkDebugUtilsObjectNameInfoEXT_array_win_to_host(in->pObjects, in->objectCount);
}

static inline void free_VkDebugUtilsMessengerCallbackDataEXT(VkDebugUtilsMessengerCallbackDataEXT_host *in)
{
    free_VkDebugUtilsObjectNameInfoEXT_array((VkDebugUtilsObjectNameInfoEXT_host *)in->pObjects, in->objectCount);
}

static inline VkCopyDescriptorSet_host *convert_VkCopyDescriptorSet_array_win_to_host(const VkCopyDescriptorSet *in, uint32_t count)
{
    VkCopyDescriptorSet_host *out;
    unsigned int i;

    if (!in) return NULL;

    out = heap_alloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].srcSet = in[i].srcSet;
        out[i].srcBinding = in[i].srcBinding;
        out[i].srcArrayElement = in[i].srcArrayElement;
        out[i].dstSet = in[i].dstSet;
        out[i].dstBinding = in[i].dstBinding;
        out[i].dstArrayElement = in[i].dstArrayElement;
        out[i].descriptorCount = in[i].descriptorCount;
    }

    return out;
}

static inline void free_VkCopyDescriptorSet_array(VkCopyDescriptorSet_host *in, uint32_t count)
{
    if (!in) return;

    heap_free(in);
}

#endif /* USE_STRUCT_CONVERSION */

VkResult convert_VkDeviceCreateInfo_struct_chain(const void *pNext, VkDeviceCreateInfo *out_struct)
{
    VkBaseOutStructure *out_header = (VkBaseOutStructure *)out_struct;
    const VkBaseInStructure *in_header;

    out_header->pNext = NULL;

    for (in_header = pNext; in_header; in_header = in_header->pNext)
    {
        switch (in_header->sType)
        {
        case VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO:
            break;

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV:
        {
            const VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV *in = (const VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV *)in_header;
            VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->deviceGeneratedCommands = in->deviceGeneratedCommands;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_DEVICE_PRIVATE_DATA_CREATE_INFO_EXT:
        {
            const VkDevicePrivateDataCreateInfoEXT *in = (const VkDevicePrivateDataCreateInfoEXT *)in_header;
            VkDevicePrivateDataCreateInfoEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->privateDataSlotRequestCount = in->privateDataSlotRequestCount;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES_EXT:
        {
            const VkPhysicalDevicePrivateDataFeaturesEXT *in = (const VkPhysicalDevicePrivateDataFeaturesEXT *)in_header;
            VkPhysicalDevicePrivateDataFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->privateData = in->privateData;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2:
        {
            const VkPhysicalDeviceFeatures2 *in = (const VkPhysicalDeviceFeatures2 *)in_header;
            VkPhysicalDeviceFeatures2 *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->features = in->features;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES:
        {
            const VkPhysicalDeviceVariablePointersFeatures *in = (const VkPhysicalDeviceVariablePointersFeatures *)in_header;
            VkPhysicalDeviceVariablePointersFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->variablePointersStorageBuffer = in->variablePointersStorageBuffer;
            out->variablePointers = in->variablePointers;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES:
        {
            const VkPhysicalDeviceMultiviewFeatures *in = (const VkPhysicalDeviceMultiviewFeatures *)in_header;
            VkPhysicalDeviceMultiviewFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->multiview = in->multiview;
            out->multiviewGeometryShader = in->multiviewGeometryShader;
            out->multiviewTessellationShader = in->multiviewTessellationShader;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO:
        {
            const VkDeviceGroupDeviceCreateInfo *in = (const VkDeviceGroupDeviceCreateInfo *)in_header;
            VkDeviceGroupDeviceCreateInfo *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->physicalDeviceCount = in->physicalDeviceCount;
            out->pPhysicalDevices = in->pPhysicalDevices;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
        {
            const VkPhysicalDevice16BitStorageFeatures *in = (const VkPhysicalDevice16BitStorageFeatures *)in_header;
            VkPhysicalDevice16BitStorageFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->storageBuffer16BitAccess = in->storageBuffer16BitAccess;
            out->uniformAndStorageBuffer16BitAccess = in->uniformAndStorageBuffer16BitAccess;
            out->storagePushConstant16 = in->storagePushConstant16;
            out->storageInputOutput16 = in->storageInputOutput16;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES:
        {
            const VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures *in = (const VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures *)in_header;
            VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderSubgroupExtendedTypes = in->shaderSubgroupExtendedTypes;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES:
        {
            const VkPhysicalDeviceSamplerYcbcrConversionFeatures *in = (const VkPhysicalDeviceSamplerYcbcrConversionFeatures *)in_header;
            VkPhysicalDeviceSamplerYcbcrConversionFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->samplerYcbcrConversion = in->samplerYcbcrConversion;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES:
        {
            const VkPhysicalDeviceProtectedMemoryFeatures *in = (const VkPhysicalDeviceProtectedMemoryFeatures *)in_header;
            VkPhysicalDeviceProtectedMemoryFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->protectedMemory = in->protectedMemory;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT:
        {
            const VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT *in = (const VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT *)in_header;
            VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->advancedBlendCoherentOperations = in->advancedBlendCoherentOperations;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT:
        {
            const VkPhysicalDeviceInlineUniformBlockFeaturesEXT *in = (const VkPhysicalDeviceInlineUniformBlockFeaturesEXT *)in_header;
            VkPhysicalDeviceInlineUniformBlockFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->inlineUniformBlock = in->inlineUniformBlock;
            out->descriptorBindingInlineUniformBlockUpdateAfterBind = in->descriptorBindingInlineUniformBlockUpdateAfterBind;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
        {
            const VkPhysicalDeviceShaderDrawParametersFeatures *in = (const VkPhysicalDeviceShaderDrawParametersFeatures *)in_header;
            VkPhysicalDeviceShaderDrawParametersFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderDrawParameters = in->shaderDrawParameters;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES:
        {
            const VkPhysicalDeviceShaderFloat16Int8Features *in = (const VkPhysicalDeviceShaderFloat16Int8Features *)in_header;
            VkPhysicalDeviceShaderFloat16Int8Features *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderFloat16 = in->shaderFloat16;
            out->shaderInt8 = in->shaderInt8;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES:
        {
            const VkPhysicalDeviceHostQueryResetFeatures *in = (const VkPhysicalDeviceHostQueryResetFeatures *)in_header;
            VkPhysicalDeviceHostQueryResetFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->hostQueryReset = in->hostQueryReset;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES:
        {
            const VkPhysicalDeviceDescriptorIndexingFeatures *in = (const VkPhysicalDeviceDescriptorIndexingFeatures *)in_header;
            VkPhysicalDeviceDescriptorIndexingFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderInputAttachmentArrayDynamicIndexing = in->shaderInputAttachmentArrayDynamicIndexing;
            out->shaderUniformTexelBufferArrayDynamicIndexing = in->shaderUniformTexelBufferArrayDynamicIndexing;
            out->shaderStorageTexelBufferArrayDynamicIndexing = in->shaderStorageTexelBufferArrayDynamicIndexing;
            out->shaderUniformBufferArrayNonUniformIndexing = in->shaderUniformBufferArrayNonUniformIndexing;
            out->shaderSampledImageArrayNonUniformIndexing = in->shaderSampledImageArrayNonUniformIndexing;
            out->shaderStorageBufferArrayNonUniformIndexing = in->shaderStorageBufferArrayNonUniformIndexing;
            out->shaderStorageImageArrayNonUniformIndexing = in->shaderStorageImageArrayNonUniformIndexing;
            out->shaderInputAttachmentArrayNonUniformIndexing = in->shaderInputAttachmentArrayNonUniformIndexing;
            out->shaderUniformTexelBufferArrayNonUniformIndexing = in->shaderUniformTexelBufferArrayNonUniformIndexing;
            out->shaderStorageTexelBufferArrayNonUniformIndexing = in->shaderStorageTexelBufferArrayNonUniformIndexing;
            out->descriptorBindingUniformBufferUpdateAfterBind = in->descriptorBindingUniformBufferUpdateAfterBind;
            out->descriptorBindingSampledImageUpdateAfterBind = in->descriptorBindingSampledImageUpdateAfterBind;
            out->descriptorBindingStorageImageUpdateAfterBind = in->descriptorBindingStorageImageUpdateAfterBind;
            out->descriptorBindingStorageBufferUpdateAfterBind = in->descriptorBindingStorageBufferUpdateAfterBind;
            out->descriptorBindingUniformTexelBufferUpdateAfterBind = in->descriptorBindingUniformTexelBufferUpdateAfterBind;
            out->descriptorBindingStorageTexelBufferUpdateAfterBind = in->descriptorBindingStorageTexelBufferUpdateAfterBind;
            out->descriptorBindingUpdateUnusedWhilePending = in->descriptorBindingUpdateUnusedWhilePending;
            out->descriptorBindingPartiallyBound = in->descriptorBindingPartiallyBound;
            out->descriptorBindingVariableDescriptorCount = in->descriptorBindingVariableDescriptorCount;
            out->runtimeDescriptorArray = in->runtimeDescriptorArray;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES:
        {
            const VkPhysicalDeviceTimelineSemaphoreFeatures *in = (const VkPhysicalDeviceTimelineSemaphoreFeatures *)in_header;
            VkPhysicalDeviceTimelineSemaphoreFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->timelineSemaphore = in->timelineSemaphore;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES:
        {
            const VkPhysicalDevice8BitStorageFeatures *in = (const VkPhysicalDevice8BitStorageFeatures *)in_header;
            VkPhysicalDevice8BitStorageFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->storageBuffer8BitAccess = in->storageBuffer8BitAccess;
            out->uniformAndStorageBuffer8BitAccess = in->uniformAndStorageBuffer8BitAccess;
            out->storagePushConstant8 = in->storagePushConstant8;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT:
        {
            const VkPhysicalDeviceConditionalRenderingFeaturesEXT *in = (const VkPhysicalDeviceConditionalRenderingFeaturesEXT *)in_header;
            VkPhysicalDeviceConditionalRenderingFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->conditionalRendering = in->conditionalRendering;
            out->inheritedConditionalRendering = in->inheritedConditionalRendering;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES:
        {
            const VkPhysicalDeviceVulkanMemoryModelFeatures *in = (const VkPhysicalDeviceVulkanMemoryModelFeatures *)in_header;
            VkPhysicalDeviceVulkanMemoryModelFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->vulkanMemoryModel = in->vulkanMemoryModel;
            out->vulkanMemoryModelDeviceScope = in->vulkanMemoryModelDeviceScope;
            out->vulkanMemoryModelAvailabilityVisibilityChains = in->vulkanMemoryModelAvailabilityVisibilityChains;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES:
        {
            const VkPhysicalDeviceShaderAtomicInt64Features *in = (const VkPhysicalDeviceShaderAtomicInt64Features *)in_header;
            VkPhysicalDeviceShaderAtomicInt64Features *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderBufferInt64Atomics = in->shaderBufferInt64Atomics;
            out->shaderSharedInt64Atomics = in->shaderSharedInt64Atomics;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT:
        {
            const VkPhysicalDeviceShaderAtomicFloatFeaturesEXT *in = (const VkPhysicalDeviceShaderAtomicFloatFeaturesEXT *)in_header;
            VkPhysicalDeviceShaderAtomicFloatFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderBufferFloat32Atomics = in->shaderBufferFloat32Atomics;
            out->shaderBufferFloat32AtomicAdd = in->shaderBufferFloat32AtomicAdd;
            out->shaderBufferFloat64Atomics = in->shaderBufferFloat64Atomics;
            out->shaderBufferFloat64AtomicAdd = in->shaderBufferFloat64AtomicAdd;
            out->shaderSharedFloat32Atomics = in->shaderSharedFloat32Atomics;
            out->shaderSharedFloat32AtomicAdd = in->shaderSharedFloat32AtomicAdd;
            out->shaderSharedFloat64Atomics = in->shaderSharedFloat64Atomics;
            out->shaderSharedFloat64AtomicAdd = in->shaderSharedFloat64AtomicAdd;
            out->shaderImageFloat32Atomics = in->shaderImageFloat32Atomics;
            out->shaderImageFloat32AtomicAdd = in->shaderImageFloat32AtomicAdd;
            out->sparseImageFloat32Atomics = in->sparseImageFloat32Atomics;
            out->sparseImageFloat32AtomicAdd = in->sparseImageFloat32AtomicAdd;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT:
        {
            const VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *in = (const VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *)in_header;
            VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->vertexAttributeInstanceRateDivisor = in->vertexAttributeInstanceRateDivisor;
            out->vertexAttributeInstanceRateZeroDivisor = in->vertexAttributeInstanceRateZeroDivisor;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT:
        {
            const VkPhysicalDeviceASTCDecodeFeaturesEXT *in = (const VkPhysicalDeviceASTCDecodeFeaturesEXT *)in_header;
            VkPhysicalDeviceASTCDecodeFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->decodeModeSharedExponent = in->decodeModeSharedExponent;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT:
        {
            const VkPhysicalDeviceTransformFeedbackFeaturesEXT *in = (const VkPhysicalDeviceTransformFeedbackFeaturesEXT *)in_header;
            VkPhysicalDeviceTransformFeedbackFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->transformFeedback = in->transformFeedback;
            out->geometryStreams = in->geometryStreams;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV:
        {
            const VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV *in = (const VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV *)in_header;
            VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->representativeFragmentTest = in->representativeFragmentTest;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV:
        {
            const VkPhysicalDeviceExclusiveScissorFeaturesNV *in = (const VkPhysicalDeviceExclusiveScissorFeaturesNV *)in_header;
            VkPhysicalDeviceExclusiveScissorFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->exclusiveScissor = in->exclusiveScissor;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV:
        {
            const VkPhysicalDeviceCornerSampledImageFeaturesNV *in = (const VkPhysicalDeviceCornerSampledImageFeaturesNV *)in_header;
            VkPhysicalDeviceCornerSampledImageFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->cornerSampledImage = in->cornerSampledImage;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV:
        {
            const VkPhysicalDeviceComputeShaderDerivativesFeaturesNV *in = (const VkPhysicalDeviceComputeShaderDerivativesFeaturesNV *)in_header;
            VkPhysicalDeviceComputeShaderDerivativesFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->computeDerivativeGroupQuads = in->computeDerivativeGroupQuads;
            out->computeDerivativeGroupLinear = in->computeDerivativeGroupLinear;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV:
        {
            const VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV *in = (const VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV *)in_header;
            VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentShaderBarycentric = in->fragmentShaderBarycentric;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV:
        {
            const VkPhysicalDeviceShaderImageFootprintFeaturesNV *in = (const VkPhysicalDeviceShaderImageFootprintFeaturesNV *)in_header;
            VkPhysicalDeviceShaderImageFootprintFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->imageFootprint = in->imageFootprint;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV:
        {
            const VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV *in = (const VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV *)in_header;
            VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->dedicatedAllocationImageAliasing = in->dedicatedAllocationImageAliasing;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV:
        {
            const VkPhysicalDeviceShadingRateImageFeaturesNV *in = (const VkPhysicalDeviceShadingRateImageFeaturesNV *)in_header;
            VkPhysicalDeviceShadingRateImageFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shadingRateImage = in->shadingRateImage;
            out->shadingRateCoarseSampleOrder = in->shadingRateCoarseSampleOrder;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV:
        {
            const VkPhysicalDeviceMeshShaderFeaturesNV *in = (const VkPhysicalDeviceMeshShaderFeaturesNV *)in_header;
            VkPhysicalDeviceMeshShaderFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->taskShader = in->taskShader;
            out->meshShader = in->meshShader;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD:
        {
            const VkDeviceMemoryOverallocationCreateInfoAMD *in = (const VkDeviceMemoryOverallocationCreateInfoAMD *)in_header;
            VkDeviceMemoryOverallocationCreateInfoAMD *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->overallocationBehavior = in->overallocationBehavior;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT:
        {
            const VkPhysicalDeviceFragmentDensityMapFeaturesEXT *in = (const VkPhysicalDeviceFragmentDensityMapFeaturesEXT *)in_header;
            VkPhysicalDeviceFragmentDensityMapFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentDensityMap = in->fragmentDensityMap;
            out->fragmentDensityMapDynamic = in->fragmentDensityMapDynamic;
            out->fragmentDensityMapNonSubsampledImages = in->fragmentDensityMapNonSubsampledImages;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT:
        {
            const VkPhysicalDeviceFragmentDensityMap2FeaturesEXT *in = (const VkPhysicalDeviceFragmentDensityMap2FeaturesEXT *)in_header;
            VkPhysicalDeviceFragmentDensityMap2FeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentDensityMapDeferred = in->fragmentDensityMapDeferred;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES:
        {
            const VkPhysicalDeviceScalarBlockLayoutFeatures *in = (const VkPhysicalDeviceScalarBlockLayoutFeatures *)in_header;
            VkPhysicalDeviceScalarBlockLayoutFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->scalarBlockLayout = in->scalarBlockLayout;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES:
        {
            const VkPhysicalDeviceUniformBufferStandardLayoutFeatures *in = (const VkPhysicalDeviceUniformBufferStandardLayoutFeatures *)in_header;
            VkPhysicalDeviceUniformBufferStandardLayoutFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->uniformBufferStandardLayout = in->uniformBufferStandardLayout;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT:
        {
            const VkPhysicalDeviceDepthClipEnableFeaturesEXT *in = (const VkPhysicalDeviceDepthClipEnableFeaturesEXT *)in_header;
            VkPhysicalDeviceDepthClipEnableFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->depthClipEnable = in->depthClipEnable;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT:
        {
            const VkPhysicalDeviceMemoryPriorityFeaturesEXT *in = (const VkPhysicalDeviceMemoryPriorityFeaturesEXT *)in_header;
            VkPhysicalDeviceMemoryPriorityFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->memoryPriority = in->memoryPriority;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES:
        {
            const VkPhysicalDeviceBufferDeviceAddressFeatures *in = (const VkPhysicalDeviceBufferDeviceAddressFeatures *)in_header;
            VkPhysicalDeviceBufferDeviceAddressFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->bufferDeviceAddress = in->bufferDeviceAddress;
            out->bufferDeviceAddressCaptureReplay = in->bufferDeviceAddressCaptureReplay;
            out->bufferDeviceAddressMultiDevice = in->bufferDeviceAddressMultiDevice;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT:
        {
            const VkPhysicalDeviceBufferDeviceAddressFeaturesEXT *in = (const VkPhysicalDeviceBufferDeviceAddressFeaturesEXT *)in_header;
            VkPhysicalDeviceBufferDeviceAddressFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->bufferDeviceAddress = in->bufferDeviceAddress;
            out->bufferDeviceAddressCaptureReplay = in->bufferDeviceAddressCaptureReplay;
            out->bufferDeviceAddressMultiDevice = in->bufferDeviceAddressMultiDevice;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES:
        {
            const VkPhysicalDeviceImagelessFramebufferFeatures *in = (const VkPhysicalDeviceImagelessFramebufferFeatures *)in_header;
            VkPhysicalDeviceImagelessFramebufferFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->imagelessFramebuffer = in->imagelessFramebuffer;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES_EXT:
        {
            const VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT *in = (const VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT *)in_header;
            VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->textureCompressionASTC_HDR = in->textureCompressionASTC_HDR;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV:
        {
            const VkPhysicalDeviceCooperativeMatrixFeaturesNV *in = (const VkPhysicalDeviceCooperativeMatrixFeaturesNV *)in_header;
            VkPhysicalDeviceCooperativeMatrixFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->cooperativeMatrix = in->cooperativeMatrix;
            out->cooperativeMatrixRobustBufferAccess = in->cooperativeMatrixRobustBufferAccess;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT:
        {
            const VkPhysicalDeviceYcbcrImageArraysFeaturesEXT *in = (const VkPhysicalDeviceYcbcrImageArraysFeaturesEXT *)in_header;
            VkPhysicalDeviceYcbcrImageArraysFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->ycbcrImageArrays = in->ycbcrImageArrays;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR:
        {
            const VkPhysicalDevicePerformanceQueryFeaturesKHR *in = (const VkPhysicalDevicePerformanceQueryFeaturesKHR *)in_header;
            VkPhysicalDevicePerformanceQueryFeaturesKHR *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->performanceCounterQueryPools = in->performanceCounterQueryPools;
            out->performanceCounterMultipleQueryPools = in->performanceCounterMultipleQueryPools;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV:
        {
            const VkPhysicalDeviceCoverageReductionModeFeaturesNV *in = (const VkPhysicalDeviceCoverageReductionModeFeaturesNV *)in_header;
            VkPhysicalDeviceCoverageReductionModeFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->coverageReductionMode = in->coverageReductionMode;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL:
        {
            const VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL *in = (const VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL *)in_header;
            VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderIntegerFunctions2 = in->shaderIntegerFunctions2;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR:
        {
            const VkPhysicalDeviceShaderClockFeaturesKHR *in = (const VkPhysicalDeviceShaderClockFeaturesKHR *)in_header;
            VkPhysicalDeviceShaderClockFeaturesKHR *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderSubgroupClock = in->shaderSubgroupClock;
            out->shaderDeviceClock = in->shaderDeviceClock;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT:
        {
            const VkPhysicalDeviceIndexTypeUint8FeaturesEXT *in = (const VkPhysicalDeviceIndexTypeUint8FeaturesEXT *)in_header;
            VkPhysicalDeviceIndexTypeUint8FeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->indexTypeUint8 = in->indexTypeUint8;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV:
        {
            const VkPhysicalDeviceShaderSMBuiltinsFeaturesNV *in = (const VkPhysicalDeviceShaderSMBuiltinsFeaturesNV *)in_header;
            VkPhysicalDeviceShaderSMBuiltinsFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderSMBuiltins = in->shaderSMBuiltins;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT:
        {
            const VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT *in = (const VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT *)in_header;
            VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentShaderSampleInterlock = in->fragmentShaderSampleInterlock;
            out->fragmentShaderPixelInterlock = in->fragmentShaderPixelInterlock;
            out->fragmentShaderShadingRateInterlock = in->fragmentShaderShadingRateInterlock;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES:
        {
            const VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures *in = (const VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures *)in_header;
            VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->separateDepthStencilLayouts = in->separateDepthStencilLayouts;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR:
        {
            const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *in = (const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *)in_header;
            VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelineExecutableInfo = in->pipelineExecutableInfo;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT:
        {
            const VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT *in = (const VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT *)in_header;
            VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderDemoteToHelperInvocation = in->shaderDemoteToHelperInvocation;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT:
        {
            const VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT *in = (const VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT *)in_header;
            VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->texelBufferAlignment = in->texelBufferAlignment;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT:
        {
            const VkPhysicalDeviceSubgroupSizeControlFeaturesEXT *in = (const VkPhysicalDeviceSubgroupSizeControlFeaturesEXT *)in_header;
            VkPhysicalDeviceSubgroupSizeControlFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->subgroupSizeControl = in->subgroupSizeControl;
            out->computeFullSubgroups = in->computeFullSubgroups;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT:
        {
            const VkPhysicalDeviceLineRasterizationFeaturesEXT *in = (const VkPhysicalDeviceLineRasterizationFeaturesEXT *)in_header;
            VkPhysicalDeviceLineRasterizationFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->rectangularLines = in->rectangularLines;
            out->bresenhamLines = in->bresenhamLines;
            out->smoothLines = in->smoothLines;
            out->stippledRectangularLines = in->stippledRectangularLines;
            out->stippledBresenhamLines = in->stippledBresenhamLines;
            out->stippledSmoothLines = in->stippledSmoothLines;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT:
        {
            const VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT *in = (const VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT *)in_header;
            VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelineCreationCacheControl = in->pipelineCreationCacheControl;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES:
        {
            const VkPhysicalDeviceVulkan11Features *in = (const VkPhysicalDeviceVulkan11Features *)in_header;
            VkPhysicalDeviceVulkan11Features *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->storageBuffer16BitAccess = in->storageBuffer16BitAccess;
            out->uniformAndStorageBuffer16BitAccess = in->uniformAndStorageBuffer16BitAccess;
            out->storagePushConstant16 = in->storagePushConstant16;
            out->storageInputOutput16 = in->storageInputOutput16;
            out->multiview = in->multiview;
            out->multiviewGeometryShader = in->multiviewGeometryShader;
            out->multiviewTessellationShader = in->multiviewTessellationShader;
            out->variablePointersStorageBuffer = in->variablePointersStorageBuffer;
            out->variablePointers = in->variablePointers;
            out->protectedMemory = in->protectedMemory;
            out->samplerYcbcrConversion = in->samplerYcbcrConversion;
            out->shaderDrawParameters = in->shaderDrawParameters;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES:
        {
            const VkPhysicalDeviceVulkan12Features *in = (const VkPhysicalDeviceVulkan12Features *)in_header;
            VkPhysicalDeviceVulkan12Features *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->samplerMirrorClampToEdge = in->samplerMirrorClampToEdge;
            out->drawIndirectCount = in->drawIndirectCount;
            out->storageBuffer8BitAccess = in->storageBuffer8BitAccess;
            out->uniformAndStorageBuffer8BitAccess = in->uniformAndStorageBuffer8BitAccess;
            out->storagePushConstant8 = in->storagePushConstant8;
            out->shaderBufferInt64Atomics = in->shaderBufferInt64Atomics;
            out->shaderSharedInt64Atomics = in->shaderSharedInt64Atomics;
            out->shaderFloat16 = in->shaderFloat16;
            out->shaderInt8 = in->shaderInt8;
            out->descriptorIndexing = in->descriptorIndexing;
            out->shaderInputAttachmentArrayDynamicIndexing = in->shaderInputAttachmentArrayDynamicIndexing;
            out->shaderUniformTexelBufferArrayDynamicIndexing = in->shaderUniformTexelBufferArrayDynamicIndexing;
            out->shaderStorageTexelBufferArrayDynamicIndexing = in->shaderStorageTexelBufferArrayDynamicIndexing;
            out->shaderUniformBufferArrayNonUniformIndexing = in->shaderUniformBufferArrayNonUniformIndexing;
            out->shaderSampledImageArrayNonUniformIndexing = in->shaderSampledImageArrayNonUniformIndexing;
            out->shaderStorageBufferArrayNonUniformIndexing = in->shaderStorageBufferArrayNonUniformIndexing;
            out->shaderStorageImageArrayNonUniformIndexing = in->shaderStorageImageArrayNonUniformIndexing;
            out->shaderInputAttachmentArrayNonUniformIndexing = in->shaderInputAttachmentArrayNonUniformIndexing;
            out->shaderUniformTexelBufferArrayNonUniformIndexing = in->shaderUniformTexelBufferArrayNonUniformIndexing;
            out->shaderStorageTexelBufferArrayNonUniformIndexing = in->shaderStorageTexelBufferArrayNonUniformIndexing;
            out->descriptorBindingUniformBufferUpdateAfterBind = in->descriptorBindingUniformBufferUpdateAfterBind;
            out->descriptorBindingSampledImageUpdateAfterBind = in->descriptorBindingSampledImageUpdateAfterBind;
            out->descriptorBindingStorageImageUpdateAfterBind = in->descriptorBindingStorageImageUpdateAfterBind;
            out->descriptorBindingStorageBufferUpdateAfterBind = in->descriptorBindingStorageBufferUpdateAfterBind;
            out->descriptorBindingUniformTexelBufferUpdateAfterBind = in->descriptorBindingUniformTexelBufferUpdateAfterBind;
            out->descriptorBindingStorageTexelBufferUpdateAfterBind = in->descriptorBindingStorageTexelBufferUpdateAfterBind;
            out->descriptorBindingUpdateUnusedWhilePending = in->descriptorBindingUpdateUnusedWhilePending;
            out->descriptorBindingPartiallyBound = in->descriptorBindingPartiallyBound;
            out->descriptorBindingVariableDescriptorCount = in->descriptorBindingVariableDescriptorCount;
            out->runtimeDescriptorArray = in->runtimeDescriptorArray;
            out->samplerFilterMinmax = in->samplerFilterMinmax;
            out->scalarBlockLayout = in->scalarBlockLayout;
            out->imagelessFramebuffer = in->imagelessFramebuffer;
            out->uniformBufferStandardLayout = in->uniformBufferStandardLayout;
            out->shaderSubgroupExtendedTypes = in->shaderSubgroupExtendedTypes;
            out->separateDepthStencilLayouts = in->separateDepthStencilLayouts;
            out->hostQueryReset = in->hostQueryReset;
            out->timelineSemaphore = in->timelineSemaphore;
            out->bufferDeviceAddress = in->bufferDeviceAddress;
            out->bufferDeviceAddressCaptureReplay = in->bufferDeviceAddressCaptureReplay;
            out->bufferDeviceAddressMultiDevice = in->bufferDeviceAddressMultiDevice;
            out->vulkanMemoryModel = in->vulkanMemoryModel;
            out->vulkanMemoryModelDeviceScope = in->vulkanMemoryModelDeviceScope;
            out->vulkanMemoryModelAvailabilityVisibilityChains = in->vulkanMemoryModelAvailabilityVisibilityChains;
            out->shaderOutputViewportIndex = in->shaderOutputViewportIndex;
            out->shaderOutputLayer = in->shaderOutputLayer;
            out->subgroupBroadcastDynamicId = in->subgroupBroadcastDynamicId;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD:
        {
            const VkPhysicalDeviceCoherentMemoryFeaturesAMD *in = (const VkPhysicalDeviceCoherentMemoryFeaturesAMD *)in_header;
            VkPhysicalDeviceCoherentMemoryFeaturesAMD *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->deviceCoherentMemory = in->deviceCoherentMemory;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT:
        {
            const VkPhysicalDeviceCustomBorderColorFeaturesEXT *in = (const VkPhysicalDeviceCustomBorderColorFeaturesEXT *)in_header;
            VkPhysicalDeviceCustomBorderColorFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->customBorderColors = in->customBorderColors;
            out->customBorderColorWithoutFormat = in->customBorderColorWithoutFormat;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT:
        {
            const VkPhysicalDeviceExtendedDynamicStateFeaturesEXT *in = (const VkPhysicalDeviceExtendedDynamicStateFeaturesEXT *)in_header;
            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->extendedDynamicState = in->extendedDynamicState;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV:
        {
            const VkPhysicalDeviceDiagnosticsConfigFeaturesNV *in = (const VkPhysicalDeviceDiagnosticsConfigFeaturesNV *)in_header;
            VkPhysicalDeviceDiagnosticsConfigFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->diagnosticsConfig = in->diagnosticsConfig;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV:
        {
            const VkDeviceDiagnosticsConfigCreateInfoNV *in = (const VkDeviceDiagnosticsConfigCreateInfoNV *)in_header;
            VkDeviceDiagnosticsConfigCreateInfoNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->flags = in->flags;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT:
        {
            const VkPhysicalDeviceRobustness2FeaturesEXT *in = (const VkPhysicalDeviceRobustness2FeaturesEXT *)in_header;
            VkPhysicalDeviceRobustness2FeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->robustBufferAccess2 = in->robustBufferAccess2;
            out->robustImageAccess2 = in->robustImageAccess2;
            out->nullDescriptor = in->nullDescriptor;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES_EXT:
        {
            const VkPhysicalDeviceImageRobustnessFeaturesEXT *in = (const VkPhysicalDeviceImageRobustnessFeaturesEXT *)in_header;
            VkPhysicalDeviceImageRobustnessFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->robustImageAccess = in->robustImageAccess;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT:
        {
            const VkPhysicalDevice4444FormatsFeaturesEXT *in = (const VkPhysicalDevice4444FormatsFeaturesEXT *)in_header;
            VkPhysicalDevice4444FormatsFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->formatA4R4G4B4 = in->formatA4R4G4B4;
            out->formatA4B4G4R4 = in->formatA4B4G4R4;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT:
        {
            const VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT *in = (const VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT *)in_header;
            VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderImageInt64Atomics = in->shaderImageInt64Atomics;
            out->sparseImageInt64Atomics = in->sparseImageInt64Atomics;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR:
        {
            const VkPhysicalDeviceFragmentShadingRateFeaturesKHR *in = (const VkPhysicalDeviceFragmentShadingRateFeaturesKHR *)in_header;
            VkPhysicalDeviceFragmentShadingRateFeaturesKHR *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelineFragmentShadingRate = in->pipelineFragmentShadingRate;
            out->primitiveFragmentShadingRate = in->primitiveFragmentShadingRate;
            out->attachmentFragmentShadingRate = in->attachmentFragmentShadingRate;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES_KHR:
        {
            const VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR *in = (const VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR *)in_header;
            VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderTerminateInvocation = in->shaderTerminateInvocation;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV:
        {
            const VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV *in = (const VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV *)in_header;
            VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentShadingRateEnums = in->fragmentShadingRateEnums;
            out->supersampleFragmentShadingRates = in->supersampleFragmentShadingRates;
            out->noInvocationFragmentShadingRates = in->noInvocationFragmentShadingRates;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        default:
            FIXME("Application requested a linked structure of type %u.\n", in_header->sType);
        }
    }

    return VK_SUCCESS;

out_of_memory:
    free_VkDeviceCreateInfo_struct_chain(out_struct);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

void free_VkDeviceCreateInfo_struct_chain(VkDeviceCreateInfo *s)
{
    VkBaseOutStructure *header = (void *)s->pNext;

    while (header)
    {
        void *prev = header;
        header = header->pNext;
        heap_free(prev);
    }

    s->pNext = NULL;
}

VkResult convert_VkInstanceCreateInfo_struct_chain(const void *pNext, VkInstanceCreateInfo *out_struct)
{
    VkBaseOutStructure *out_header = (VkBaseOutStructure *)out_struct;
    const VkBaseInStructure *in_header;

    out_header->pNext = NULL;

    for (in_header = pNext; in_header; in_header = in_header->pNext)
    {
        switch (in_header->sType)
        {
        case VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO:
        case VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO:
            break;

        case VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT:
        {
            const VkDebugReportCallbackCreateInfoEXT *in = (const VkDebugReportCallbackCreateInfoEXT *)in_header;
            VkDebugReportCallbackCreateInfoEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->flags = in->flags;
            out->pfnCallback = in->pfnCallback;
            out->pUserData = in->pUserData;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT:
        {
            const VkValidationFlagsEXT *in = (const VkValidationFlagsEXT *)in_header;
            VkValidationFlagsEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->disabledValidationCheckCount = in->disabledValidationCheckCount;
            out->pDisabledValidationChecks = in->pDisabledValidationChecks;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT:
        {
            const VkValidationFeaturesEXT *in = (const VkValidationFeaturesEXT *)in_header;
            VkValidationFeaturesEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->enabledValidationFeatureCount = in->enabledValidationFeatureCount;
            out->pEnabledValidationFeatures = in->pEnabledValidationFeatures;
            out->disabledValidationFeatureCount = in->disabledValidationFeatureCount;
            out->pDisabledValidationFeatures = in->pDisabledValidationFeatures;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT:
        {
            const VkDebugUtilsMessengerCreateInfoEXT *in = (const VkDebugUtilsMessengerCreateInfoEXT *)in_header;
            VkDebugUtilsMessengerCreateInfoEXT *out;

            if (!(out = heap_alloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->flags = in->flags;
            out->messageSeverity = in->messageSeverity;
            out->messageType = in->messageType;
            out->pfnUserCallback = in->pfnUserCallback;
            out->pUserData = in->pUserData;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        default:
            FIXME("Application requested a linked structure of type %u.\n", in_header->sType);
        }
    }

    return VK_SUCCESS;

out_of_memory:
    free_VkInstanceCreateInfo_struct_chain(out_struct);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

void free_VkInstanceCreateInfo_struct_chain(VkInstanceCreateInfo *s)
{
    VkBaseOutStructure *header = (void *)s->pNext;

    while (header)
    {
        void *prev = header;
        header = header->pNext;
        heap_free(prev);
    }

    s->pNext = NULL;
}

VkResult WINAPI wine_vkAcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAcquireNextImageInfoKHR_host pAcquireInfo_host;
    TRACE("%p, %p, %p\n", device, pAcquireInfo, pImageIndex);

    convert_VkAcquireNextImageInfoKHR_win_to_host(pAcquireInfo, &pAcquireInfo_host);
    result = device->funcs.p_vkAcquireNextImage2KHR(device->device, &pAcquireInfo_host, pImageIndex);

    return result;
#else
    TRACE("%p, %p, %p\n", device, pAcquireInfo, pImageIndex);
    return device->funcs.p_vkAcquireNextImage2KHR(device->device, pAcquireInfo, pImageIndex);
#endif
}

VkResult WINAPI wine_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %p\n", device, wine_dbgstr_longlong(swapchain), wine_dbgstr_longlong(timeout), wine_dbgstr_longlong(semaphore), wine_dbgstr_longlong(fence), pImageIndex);
    return device->funcs.p_vkAcquireNextImageKHR(device->device, swapchain, timeout, semaphore, fence, pImageIndex);
}

static VkResult WINAPI wine_vkAcquirePerformanceConfigurationINTEL(VkDevice device, const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo, VkPerformanceConfigurationINTEL *pConfiguration)
{
    TRACE("%p, %p, %p\n", device, pAcquireInfo, pConfiguration);
    return device->funcs.p_vkAcquirePerformanceConfigurationINTEL(device->device, pAcquireInfo, pConfiguration);
}

static VkResult WINAPI wine_vkAcquireProfilingLockKHR(VkDevice device, const VkAcquireProfilingLockInfoKHR *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAcquireProfilingLockInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkAcquireProfilingLockInfoKHR_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkAcquireProfilingLockKHR(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkAcquireProfilingLockKHR(device->device, pInfo);
#endif
}

VkResult WINAPI wine_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDescriptorSetAllocateInfo_host pAllocateInfo_host;
    TRACE("%p, %p, %p\n", device, pAllocateInfo, pDescriptorSets);

    convert_VkDescriptorSetAllocateInfo_win_to_host(pAllocateInfo, &pAllocateInfo_host);
    result = device->funcs.p_vkAllocateDescriptorSets(device->device, &pAllocateInfo_host, pDescriptorSets);

    return result;
#else
    TRACE("%p, %p, %p\n", device, pAllocateInfo, pDescriptorSets);
    return device->funcs.p_vkAllocateDescriptorSets(device->device, pAllocateInfo, pDescriptorSets);
#endif
}

VkResult WINAPI wine_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkMemoryAllocateInfo_host pAllocateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pAllocateInfo, pAllocator, pMemory);

    convert_VkMemoryAllocateInfo_win_to_host(pAllocateInfo, &pAllocateInfo_host);
    result = device->funcs.p_vkAllocateMemory(device->device, &pAllocateInfo_host, NULL, pMemory);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pAllocateInfo, pAllocator, pMemory);
    return device->funcs.p_vkAllocateMemory(device->device, pAllocateInfo, NULL, pMemory);
#endif
}

VkResult WINAPI wine_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkCommandBufferBeginInfo_host pBeginInfo_host;
    TRACE("%p, %p\n", commandBuffer, pBeginInfo);

    convert_VkCommandBufferBeginInfo_win_to_host(pBeginInfo, &pBeginInfo_host);
    result = commandBuffer->device->funcs.p_vkBeginCommandBuffer(commandBuffer->command_buffer, &pBeginInfo_host);

    free_VkCommandBufferBeginInfo(&pBeginInfo_host);
    return result;
#else
    TRACE("%p, %p\n", commandBuffer, pBeginInfo);
    return commandBuffer->device->funcs.p_vkBeginCommandBuffer(commandBuffer->command_buffer, pBeginInfo);
#endif
}

static VkResult WINAPI wine_vkBindAccelerationStructureMemoryNV(VkDevice device, uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoKHR *pBindInfos)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindAccelerationStructureMemoryInfoKHR_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);

    pBindInfos_host = convert_VkBindAccelerationStructureMemoryInfoKHR_array_win_to_host(pBindInfos, bindInfoCount);
    result = device->funcs.p_vkBindAccelerationStructureMemoryNV(device->device, bindInfoCount, pBindInfos_host);

    free_VkBindAccelerationStructureMemoryInfoKHR_array(pBindInfos_host, bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);
    return device->funcs.p_vkBindAccelerationStructureMemoryNV(device->device, bindInfoCount, pBindInfos);
#endif
}

VkResult WINAPI wine_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", device, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(memory), wine_dbgstr_longlong(memoryOffset));
    return device->funcs.p_vkBindBufferMemory(device->device, buffer, memory, memoryOffset);
}

VkResult WINAPI wine_vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindBufferMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);

    pBindInfos_host = convert_VkBindBufferMemoryInfo_array_win_to_host(pBindInfos, bindInfoCount);
    result = device->funcs.p_vkBindBufferMemory2(device->device, bindInfoCount, pBindInfos_host);

    free_VkBindBufferMemoryInfo_array(pBindInfos_host, bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);
    return device->funcs.p_vkBindBufferMemory2(device->device, bindInfoCount, pBindInfos);
#endif
}

static VkResult WINAPI wine_vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindBufferMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);

    pBindInfos_host = convert_VkBindBufferMemoryInfo_array_win_to_host(pBindInfos, bindInfoCount);
    result = device->funcs.p_vkBindBufferMemory2KHR(device->device, bindInfoCount, pBindInfos_host);

    free_VkBindBufferMemoryInfo_array(pBindInfos_host, bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);
    return device->funcs.p_vkBindBufferMemory2KHR(device->device, bindInfoCount, pBindInfos);
#endif
}

VkResult WINAPI wine_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", device, wine_dbgstr_longlong(image), wine_dbgstr_longlong(memory), wine_dbgstr_longlong(memoryOffset));
    return device->funcs.p_vkBindImageMemory(device->device, image, memory, memoryOffset);
}

VkResult WINAPI wine_vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindImageMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);

    pBindInfos_host = convert_VkBindImageMemoryInfo_array_win_to_host(pBindInfos, bindInfoCount);
    result = device->funcs.p_vkBindImageMemory2(device->device, bindInfoCount, pBindInfos_host);

    free_VkBindImageMemoryInfo_array(pBindInfos_host, bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);
    return device->funcs.p_vkBindImageMemory2(device->device, bindInfoCount, pBindInfos);
#endif
}

static VkResult WINAPI wine_vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindImageMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);

    pBindInfos_host = convert_VkBindImageMemoryInfo_array_win_to_host(pBindInfos, bindInfoCount);
    result = device->funcs.p_vkBindImageMemory2KHR(device->device, bindInfoCount, pBindInfos_host);

    free_VkBindImageMemoryInfo_array(pBindInfos_host, bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", device, bindInfoCount, pBindInfos);
    return device->funcs.p_vkBindImageMemory2KHR(device->device, bindInfoCount, pBindInfos);
#endif
}

static void WINAPI wine_vkCmdBeginConditionalRenderingEXT(VkCommandBuffer commandBuffer, const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin)
{
#if defined(USE_STRUCT_CONVERSION)
    VkConditionalRenderingBeginInfoEXT_host pConditionalRenderingBegin_host;
    TRACE("%p, %p\n", commandBuffer, pConditionalRenderingBegin);

    convert_VkConditionalRenderingBeginInfoEXT_win_to_host(pConditionalRenderingBegin, &pConditionalRenderingBegin_host);
    commandBuffer->device->funcs.p_vkCmdBeginConditionalRenderingEXT(commandBuffer->command_buffer, &pConditionalRenderingBegin_host);

#else
    TRACE("%p, %p\n", commandBuffer, pConditionalRenderingBegin);
    commandBuffer->device->funcs.p_vkCmdBeginConditionalRenderingEXT(commandBuffer->command_buffer, pConditionalRenderingBegin);
#endif
}

static void WINAPI wine_vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    TRACE("%p, %p\n", commandBuffer, pLabelInfo);
    commandBuffer->device->funcs.p_vkCmdBeginDebugUtilsLabelEXT(commandBuffer->command_buffer, pLabelInfo);
}

void WINAPI wine_vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    TRACE("%p, 0x%s, %u, %#x\n", commandBuffer, wine_dbgstr_longlong(queryPool), query, flags);
    commandBuffer->device->funcs.p_vkCmdBeginQuery(commandBuffer->command_buffer, queryPool, query, flags);
}

static void WINAPI wine_vkCmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index)
{
    TRACE("%p, 0x%s, %u, %#x, %u\n", commandBuffer, wine_dbgstr_longlong(queryPool), query, flags, index);
    commandBuffer->device->funcs.p_vkCmdBeginQueryIndexedEXT(commandBuffer->command_buffer, queryPool, query, flags, index);
}

void WINAPI wine_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
#if defined(USE_STRUCT_CONVERSION)
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %#x\n", commandBuffer, pRenderPassBegin, contents);

    convert_VkRenderPassBeginInfo_win_to_host(pRenderPassBegin, &pRenderPassBegin_host);
    commandBuffer->device->funcs.p_vkCmdBeginRenderPass(commandBuffer->command_buffer, &pRenderPassBegin_host, contents);

#else
    TRACE("%p, %p, %#x\n", commandBuffer, pRenderPassBegin, contents);
    commandBuffer->device->funcs.p_vkCmdBeginRenderPass(commandBuffer->command_buffer, pRenderPassBegin, contents);
#endif
}

void WINAPI wine_vkCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %p\n", commandBuffer, pRenderPassBegin, pSubpassBeginInfo);

    convert_VkRenderPassBeginInfo_win_to_host(pRenderPassBegin, &pRenderPassBegin_host);
    commandBuffer->device->funcs.p_vkCmdBeginRenderPass2(commandBuffer->command_buffer, &pRenderPassBegin_host, pSubpassBeginInfo);

#else
    TRACE("%p, %p, %p\n", commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
    commandBuffer->device->funcs.p_vkCmdBeginRenderPass2(commandBuffer->command_buffer, pRenderPassBegin, pSubpassBeginInfo);
#endif
}

static void WINAPI wine_vkCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %p\n", commandBuffer, pRenderPassBegin, pSubpassBeginInfo);

    convert_VkRenderPassBeginInfo_win_to_host(pRenderPassBegin, &pRenderPassBegin_host);
    commandBuffer->device->funcs.p_vkCmdBeginRenderPass2KHR(commandBuffer->command_buffer, &pRenderPassBegin_host, pSubpassBeginInfo);

#else
    TRACE("%p, %p, %p\n", commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
    commandBuffer->device->funcs.p_vkCmdBeginRenderPass2KHR(commandBuffer->command_buffer, pRenderPassBegin, pSubpassBeginInfo);
#endif
}

static void WINAPI wine_vkCmdBeginTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    TRACE("%p, %u, %u, %p, %p\n", commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
    commandBuffer->device->funcs.p_vkCmdBeginTransformFeedbackEXT(commandBuffer->command_buffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

void WINAPI wine_vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
{
    TRACE("%p, %#x, 0x%s, %u, %u, %p, %u, %p\n", commandBuffer, pipelineBindPoint, wine_dbgstr_longlong(layout), firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
    commandBuffer->device->funcs.p_vkCmdBindDescriptorSets(commandBuffer->command_buffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void WINAPI wine_vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    TRACE("%p, 0x%s, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), indexType);
    commandBuffer->device->funcs.p_vkCmdBindIndexBuffer(commandBuffer->command_buffer, buffer, offset, indexType);
}

void WINAPI wine_vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    TRACE("%p, %#x, 0x%s\n", commandBuffer, pipelineBindPoint, wine_dbgstr_longlong(pipeline));
    commandBuffer->device->funcs.p_vkCmdBindPipeline(commandBuffer->command_buffer, pipelineBindPoint, pipeline);
}

static void WINAPI wine_vkCmdBindPipelineShaderGroupNV(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline, uint32_t groupIndex)
{
    TRACE("%p, %#x, 0x%s, %u\n", commandBuffer, pipelineBindPoint, wine_dbgstr_longlong(pipeline), groupIndex);
    commandBuffer->device->funcs.p_vkCmdBindPipelineShaderGroupNV(commandBuffer->command_buffer, pipelineBindPoint, pipeline, groupIndex);
}

static void WINAPI wine_vkCmdBindShadingRateImageNV(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout)
{
    TRACE("%p, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(imageView), imageLayout);
    commandBuffer->device->funcs.p_vkCmdBindShadingRateImageNV(commandBuffer->command_buffer, imageView, imageLayout);
}

static void WINAPI wine_vkCmdBindTransformFeedbackBuffersEXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes)
{
    TRACE("%p, %u, %u, %p, %p, %p\n", commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes);
    commandBuffer->device->funcs.p_vkCmdBindTransformFeedbackBuffersEXT(commandBuffer->command_buffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes);
}

void WINAPI wine_vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
{
    TRACE("%p, %u, %u, %p, %p\n", commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
    commandBuffer->device->funcs.p_vkCmdBindVertexBuffers(commandBuffer->command_buffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

static void WINAPI wine_vkCmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides)
{
    TRACE("%p, %u, %u, %p, %p, %p, %p\n", commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
    commandBuffer->device->funcs.p_vkCmdBindVertexBuffers2EXT(commandBuffer->command_buffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}

void WINAPI wine_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter)
{
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p, %#x\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions, filter);
    commandBuffer->device->funcs.p_vkCmdBlitImage(commandBuffer->command_buffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

static void WINAPI wine_vkCmdBlitImage2KHR(VkCommandBuffer commandBuffer, const VkBlitImageInfo2KHR *pBlitImageInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBlitImageInfo2KHR_host pBlitImageInfo_host;
    TRACE("%p, %p\n", commandBuffer, pBlitImageInfo);

    convert_VkBlitImageInfo2KHR_win_to_host(pBlitImageInfo, &pBlitImageInfo_host);
    commandBuffer->device->funcs.p_vkCmdBlitImage2KHR(commandBuffer->command_buffer, &pBlitImageInfo_host);

#else
    TRACE("%p, %p\n", commandBuffer, pBlitImageInfo);
    commandBuffer->device->funcs.p_vkCmdBlitImage2KHR(commandBuffer->command_buffer, pBlitImageInfo);
#endif
}

static void WINAPI wine_vkCmdBuildAccelerationStructureNV(VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV *pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureKHR dst, VkAccelerationStructureKHR src, VkBuffer scratch, VkDeviceSize scratchOffset)
{
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureInfoNV_host pInfo_host;
    TRACE("%p, %p, 0x%s, 0x%s, %u, 0x%s, 0x%s, 0x%s, 0x%s\n", commandBuffer, pInfo, wine_dbgstr_longlong(instanceData), wine_dbgstr_longlong(instanceOffset), update, wine_dbgstr_longlong(dst), wine_dbgstr_longlong(src), wine_dbgstr_longlong(scratch), wine_dbgstr_longlong(scratchOffset));

    convert_VkAccelerationStructureInfoNV_win_to_host(pInfo, &pInfo_host);
    commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructureNV(commandBuffer->command_buffer, &pInfo_host, instanceData, instanceOffset, update, dst, src, scratch, scratchOffset);

    free_VkAccelerationStructureInfoNV(&pInfo_host);
#else
    TRACE("%p, %p, 0x%s, 0x%s, %u, 0x%s, 0x%s, 0x%s, 0x%s\n", commandBuffer, pInfo, wine_dbgstr_longlong(instanceData), wine_dbgstr_longlong(instanceOffset), update, wine_dbgstr_longlong(dst), wine_dbgstr_longlong(src), wine_dbgstr_longlong(scratch), wine_dbgstr_longlong(scratchOffset));
    commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructureNV(commandBuffer->command_buffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch, scratchOffset);
#endif
}

void WINAPI wine_vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects)
{
    TRACE("%p, %u, %p, %u, %p\n", commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
    commandBuffer->device->funcs.p_vkCmdClearAttachments(commandBuffer->command_buffer, attachmentCount, pAttachments, rectCount, pRects);
}

void WINAPI wine_vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", commandBuffer, wine_dbgstr_longlong(image), imageLayout, pColor, rangeCount, pRanges);
    commandBuffer->device->funcs.p_vkCmdClearColorImage(commandBuffer->command_buffer, image, imageLayout, pColor, rangeCount, pRanges);
}

void WINAPI wine_vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", commandBuffer, wine_dbgstr_longlong(image), imageLayout, pDepthStencil, rangeCount, pRanges);
    commandBuffer->device->funcs.p_vkCmdClearDepthStencilImage(commandBuffer->command_buffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

static void WINAPI wine_vkCmdCopyAccelerationStructureNV(VkCommandBuffer commandBuffer, VkAccelerationStructureKHR dst, VkAccelerationStructureKHR src, VkCopyAccelerationStructureModeKHR mode)
{
    TRACE("%p, 0x%s, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(dst), wine_dbgstr_longlong(src), mode);
    commandBuffer->device->funcs.p_vkCmdCopyAccelerationStructureNV(commandBuffer->command_buffer, dst, src, mode);
}

void WINAPI wine_vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBufferCopy_host *pRegions_host;
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcBuffer), wine_dbgstr_longlong(dstBuffer), regionCount, pRegions);

    pRegions_host = convert_VkBufferCopy_array_win_to_host(pRegions, regionCount);
    commandBuffer->device->funcs.p_vkCmdCopyBuffer(commandBuffer->command_buffer, srcBuffer, dstBuffer, regionCount, pRegions_host);

    free_VkBufferCopy_array(pRegions_host, regionCount);
#else
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcBuffer), wine_dbgstr_longlong(dstBuffer), regionCount, pRegions);
    commandBuffer->device->funcs.p_vkCmdCopyBuffer(commandBuffer->command_buffer, srcBuffer, dstBuffer, regionCount, pRegions);
#endif
}

static void WINAPI wine_vkCmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2KHR *pCopyBufferInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkCopyBufferInfo2KHR_host pCopyBufferInfo_host;
    TRACE("%p, %p\n", commandBuffer, pCopyBufferInfo);

    convert_VkCopyBufferInfo2KHR_win_to_host(pCopyBufferInfo, &pCopyBufferInfo_host);
    commandBuffer->device->funcs.p_vkCmdCopyBuffer2KHR(commandBuffer->command_buffer, &pCopyBufferInfo_host);

    free_VkCopyBufferInfo2KHR(&pCopyBufferInfo_host);
#else
    TRACE("%p, %p\n", commandBuffer, pCopyBufferInfo);
    commandBuffer->device->funcs.p_vkCmdCopyBuffer2KHR(commandBuffer->command_buffer, pCopyBufferInfo);
#endif
}

void WINAPI wine_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBufferImageCopy_host *pRegions_host;
    TRACE("%p, 0x%s, 0x%s, %#x, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcBuffer), wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions);

    pRegions_host = convert_VkBufferImageCopy_array_win_to_host(pRegions, regionCount);
    commandBuffer->device->funcs.p_vkCmdCopyBufferToImage(commandBuffer->command_buffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions_host);

    free_VkBufferImageCopy_array(pRegions_host, regionCount);
#else
    TRACE("%p, 0x%s, 0x%s, %#x, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcBuffer), wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions);
    commandBuffer->device->funcs.p_vkCmdCopyBufferToImage(commandBuffer->command_buffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
#endif
}

static void WINAPI wine_vkCmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2KHR *pCopyBufferToImageInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkCopyBufferToImageInfo2KHR_host pCopyBufferToImageInfo_host;
    TRACE("%p, %p\n", commandBuffer, pCopyBufferToImageInfo);

    convert_VkCopyBufferToImageInfo2KHR_win_to_host(pCopyBufferToImageInfo, &pCopyBufferToImageInfo_host);
    commandBuffer->device->funcs.p_vkCmdCopyBufferToImage2KHR(commandBuffer->command_buffer, &pCopyBufferToImageInfo_host);

    free_VkCopyBufferToImageInfo2KHR(&pCopyBufferToImageInfo_host);
#else
    TRACE("%p, %p\n", commandBuffer, pCopyBufferToImageInfo);
    commandBuffer->device->funcs.p_vkCmdCopyBufferToImage2KHR(commandBuffer->command_buffer, pCopyBufferToImageInfo);
#endif
}

void WINAPI wine_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions)
{
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions);
    commandBuffer->device->funcs.p_vkCmdCopyImage(commandBuffer->command_buffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

static void WINAPI wine_vkCmdCopyImage2KHR(VkCommandBuffer commandBuffer, const VkCopyImageInfo2KHR *pCopyImageInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkCopyImageInfo2KHR_host pCopyImageInfo_host;
    TRACE("%p, %p\n", commandBuffer, pCopyImageInfo);

    convert_VkCopyImageInfo2KHR_win_to_host(pCopyImageInfo, &pCopyImageInfo_host);
    commandBuffer->device->funcs.p_vkCmdCopyImage2KHR(commandBuffer->command_buffer, &pCopyImageInfo_host);

#else
    TRACE("%p, %p\n", commandBuffer, pCopyImageInfo);
    commandBuffer->device->funcs.p_vkCmdCopyImage2KHR(commandBuffer->command_buffer, pCopyImageInfo);
#endif
}

void WINAPI wine_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBufferImageCopy_host *pRegions_host;
    TRACE("%p, 0x%s, %#x, 0x%s, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstBuffer), regionCount, pRegions);

    pRegions_host = convert_VkBufferImageCopy_array_win_to_host(pRegions, regionCount);
    commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer(commandBuffer->command_buffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions_host);

    free_VkBufferImageCopy_array(pRegions_host, regionCount);
#else
    TRACE("%p, 0x%s, %#x, 0x%s, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstBuffer), regionCount, pRegions);
    commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer(commandBuffer->command_buffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
#endif
}

static void WINAPI wine_vkCmdCopyImageToBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2KHR *pCopyImageToBufferInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkCopyImageToBufferInfo2KHR_host pCopyImageToBufferInfo_host;
    TRACE("%p, %p\n", commandBuffer, pCopyImageToBufferInfo);

    convert_VkCopyImageToBufferInfo2KHR_win_to_host(pCopyImageToBufferInfo, &pCopyImageToBufferInfo_host);
    commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer2KHR(commandBuffer->command_buffer, &pCopyImageToBufferInfo_host);

    free_VkCopyImageToBufferInfo2KHR(&pCopyImageToBufferInfo_host);
#else
    TRACE("%p, %p\n", commandBuffer, pCopyImageToBufferInfo);
    commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer2KHR(commandBuffer->command_buffer, pCopyImageToBufferInfo);
#endif
}

void WINAPI wine_vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    TRACE("%p, 0x%s, %u, %u, 0x%s, 0x%s, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(queryPool), firstQuery, queryCount, wine_dbgstr_longlong(dstBuffer), wine_dbgstr_longlong(dstOffset), wine_dbgstr_longlong(stride), flags);
    commandBuffer->device->funcs.p_vkCmdCopyQueryPoolResults(commandBuffer->command_buffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

static void WINAPI wine_vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    TRACE("%p, %p\n", commandBuffer, pMarkerInfo);
    commandBuffer->device->funcs.p_vkCmdDebugMarkerBeginEXT(commandBuffer->command_buffer, pMarkerInfo);
}

static void WINAPI wine_vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
{
    TRACE("%p\n", commandBuffer);
    commandBuffer->device->funcs.p_vkCmdDebugMarkerEndEXT(commandBuffer->command_buffer);
}

static void WINAPI wine_vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    TRACE("%p, %p\n", commandBuffer, pMarkerInfo);
    commandBuffer->device->funcs.p_vkCmdDebugMarkerInsertEXT(commandBuffer->command_buffer, pMarkerInfo);
}

void WINAPI wine_vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    TRACE("%p, %u, %u, %u\n", commandBuffer, groupCountX, groupCountY, groupCountZ);
    commandBuffer->device->funcs.p_vkCmdDispatch(commandBuffer->command_buffer, groupCountX, groupCountY, groupCountZ);
}

void WINAPI wine_vkCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
    commandBuffer->device->funcs.p_vkCmdDispatchBase(commandBuffer->command_buffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

static void WINAPI wine_vkCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
    commandBuffer->device->funcs.p_vkCmdDispatchBaseKHR(commandBuffer->command_buffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

void WINAPI wine_vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    TRACE("%p, 0x%s, 0x%s\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset));
    commandBuffer->device->funcs.p_vkCmdDispatchIndirect(commandBuffer->command_buffer, buffer, offset);
}

void WINAPI wine_vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    TRACE("%p, %u, %u, %u, %u\n", commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    commandBuffer->device->funcs.p_vkCmdDraw(commandBuffer->command_buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void WINAPI wine_vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    TRACE("%p, %u, %u, %u, %d, %u\n", commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    commandBuffer->device->funcs.p_vkCmdDrawIndexed(commandBuffer->command_buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void WINAPI wine_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), drawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirect(commandBuffer->command_buffer, buffer, offset, drawCount, stride);
}

void WINAPI wine_vkCmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(countBuffer), wine_dbgstr_longlong(countBufferOffset), maxDrawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirectCount(commandBuffer->command_buffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

static void WINAPI wine_vkCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(countBuffer), wine_dbgstr_longlong(countBufferOffset), maxDrawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirectCountAMD(commandBuffer->command_buffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

static void WINAPI wine_vkCmdDrawIndexedIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(countBuffer), wine_dbgstr_longlong(countBufferOffset), maxDrawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirectCountKHR(commandBuffer->command_buffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void WINAPI wine_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), drawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndirect(commandBuffer->command_buffer, buffer, offset, drawCount, stride);
}

static void WINAPI wine_vkCmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride)
{
    TRACE("%p, %u, %u, 0x%s, 0x%s, %u, %u\n", commandBuffer, instanceCount, firstInstance, wine_dbgstr_longlong(counterBuffer), wine_dbgstr_longlong(counterBufferOffset), counterOffset, vertexStride);
    commandBuffer->device->funcs.p_vkCmdDrawIndirectByteCountEXT(commandBuffer->command_buffer, instanceCount, firstInstance, counterBuffer, counterBufferOffset, counterOffset, vertexStride);
}

void WINAPI wine_vkCmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(countBuffer), wine_dbgstr_longlong(countBufferOffset), maxDrawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndirectCount(commandBuffer->command_buffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

static void WINAPI wine_vkCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(countBuffer), wine_dbgstr_longlong(countBufferOffset), maxDrawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndirectCountAMD(commandBuffer->command_buffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

static void WINAPI wine_vkCmdDrawIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(countBuffer), wine_dbgstr_longlong(countBufferOffset), maxDrawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawIndirectCountKHR(commandBuffer->command_buffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

static void WINAPI wine_vkCmdDrawMeshTasksIndirectCountNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(countBuffer), wine_dbgstr_longlong(countBufferOffset), maxDrawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawMeshTasksIndirectCountNV(commandBuffer->command_buffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

static void WINAPI wine_vkCmdDrawMeshTasksIndirectNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), drawCount, stride);
    commandBuffer->device->funcs.p_vkCmdDrawMeshTasksIndirectNV(commandBuffer->command_buffer, buffer, offset, drawCount, stride);
}

static void WINAPI wine_vkCmdDrawMeshTasksNV(VkCommandBuffer commandBuffer, uint32_t taskCount, uint32_t firstTask)
{
    TRACE("%p, %u, %u\n", commandBuffer, taskCount, firstTask);
    commandBuffer->device->funcs.p_vkCmdDrawMeshTasksNV(commandBuffer->command_buffer, taskCount, firstTask);
}

static void WINAPI wine_vkCmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer)
{
    TRACE("%p\n", commandBuffer);
    commandBuffer->device->funcs.p_vkCmdEndConditionalRenderingEXT(commandBuffer->command_buffer);
}

static void WINAPI wine_vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
    TRACE("%p\n", commandBuffer);
    commandBuffer->device->funcs.p_vkCmdEndDebugUtilsLabelEXT(commandBuffer->command_buffer);
}

void WINAPI wine_vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    TRACE("%p, 0x%s, %u\n", commandBuffer, wine_dbgstr_longlong(queryPool), query);
    commandBuffer->device->funcs.p_vkCmdEndQuery(commandBuffer->command_buffer, queryPool, query);
}

static void WINAPI wine_vkCmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index)
{
    TRACE("%p, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(queryPool), query, index);
    commandBuffer->device->funcs.p_vkCmdEndQueryIndexedEXT(commandBuffer->command_buffer, queryPool, query, index);
}

void WINAPI wine_vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    TRACE("%p\n", commandBuffer);
    commandBuffer->device->funcs.p_vkCmdEndRenderPass(commandBuffer->command_buffer);
}

void WINAPI wine_vkCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    TRACE("%p, %p\n", commandBuffer, pSubpassEndInfo);
    commandBuffer->device->funcs.p_vkCmdEndRenderPass2(commandBuffer->command_buffer, pSubpassEndInfo);
}

static void WINAPI wine_vkCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    TRACE("%p, %p\n", commandBuffer, pSubpassEndInfo);
    commandBuffer->device->funcs.p_vkCmdEndRenderPass2KHR(commandBuffer->command_buffer, pSubpassEndInfo);
}

static void WINAPI wine_vkCmdEndTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    TRACE("%p, %u, %u, %p, %p\n", commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
    commandBuffer->device->funcs.p_vkCmdEndTransformFeedbackEXT(commandBuffer->command_buffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

static void WINAPI wine_vkCmdExecuteGeneratedCommandsNV(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkGeneratedCommandsInfoNV_host pGeneratedCommandsInfo_host;
    TRACE("%p, %u, %p\n", commandBuffer, isPreprocessed, pGeneratedCommandsInfo);

    convert_VkGeneratedCommandsInfoNV_win_to_host(pGeneratedCommandsInfo, &pGeneratedCommandsInfo_host);
    commandBuffer->device->funcs.p_vkCmdExecuteGeneratedCommandsNV(commandBuffer->command_buffer, isPreprocessed, &pGeneratedCommandsInfo_host);

    free_VkGeneratedCommandsInfoNV(&pGeneratedCommandsInfo_host);
#else
    TRACE("%p, %u, %p\n", commandBuffer, isPreprocessed, pGeneratedCommandsInfo);
    commandBuffer->device->funcs.p_vkCmdExecuteGeneratedCommandsNV(commandBuffer->command_buffer, isPreprocessed, pGeneratedCommandsInfo);
#endif
}

void WINAPI wine_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, %u\n", commandBuffer, wine_dbgstr_longlong(dstBuffer), wine_dbgstr_longlong(dstOffset), wine_dbgstr_longlong(size), data);
    commandBuffer->device->funcs.p_vkCmdFillBuffer(commandBuffer->command_buffer, dstBuffer, dstOffset, size, data);
}

static void WINAPI wine_vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    TRACE("%p, %p\n", commandBuffer, pLabelInfo);
    commandBuffer->device->funcs.p_vkCmdInsertDebugUtilsLabelEXT(commandBuffer->command_buffer, pLabelInfo);
}

void WINAPI wine_vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    TRACE("%p, %#x\n", commandBuffer, contents);
    commandBuffer->device->funcs.p_vkCmdNextSubpass(commandBuffer->command_buffer, contents);
}

void WINAPI wine_vkCmdNextSubpass2(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    TRACE("%p, %p, %p\n", commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
    commandBuffer->device->funcs.p_vkCmdNextSubpass2(commandBuffer->command_buffer, pSubpassBeginInfo, pSubpassEndInfo);
}

static void WINAPI wine_vkCmdNextSubpass2KHR(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    TRACE("%p, %p, %p\n", commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
    commandBuffer->device->funcs.p_vkCmdNextSubpass2KHR(commandBuffer->command_buffer, pSubpassBeginInfo, pSubpassEndInfo);
}

void WINAPI wine_vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryBarrier_host *pBufferMemoryBarriers_host;
    VkImageMemoryBarrier_host *pImageMemoryBarriers_host;
    TRACE("%p, %#x, %#x, %#x, %u, %p, %u, %p, %u, %p\n", commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

    pBufferMemoryBarriers_host = convert_VkBufferMemoryBarrier_array_win_to_host(pBufferMemoryBarriers, bufferMemoryBarrierCount);
    pImageMemoryBarriers_host = convert_VkImageMemoryBarrier_array_win_to_host(pImageMemoryBarriers, imageMemoryBarrierCount);
    commandBuffer->device->funcs.p_vkCmdPipelineBarrier(commandBuffer->command_buffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers_host, imageMemoryBarrierCount, pImageMemoryBarriers_host);

    free_VkBufferMemoryBarrier_array(pBufferMemoryBarriers_host, bufferMemoryBarrierCount);
    free_VkImageMemoryBarrier_array(pImageMemoryBarriers_host, imageMemoryBarrierCount);
#else
    TRACE("%p, %#x, %#x, %#x, %u, %p, %u, %p, %u, %p\n", commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    commandBuffer->device->funcs.p_vkCmdPipelineBarrier(commandBuffer->command_buffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
#endif
}

static void WINAPI wine_vkCmdPreprocessGeneratedCommandsNV(VkCommandBuffer commandBuffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkGeneratedCommandsInfoNV_host pGeneratedCommandsInfo_host;
    TRACE("%p, %p\n", commandBuffer, pGeneratedCommandsInfo);

    convert_VkGeneratedCommandsInfoNV_win_to_host(pGeneratedCommandsInfo, &pGeneratedCommandsInfo_host);
    commandBuffer->device->funcs.p_vkCmdPreprocessGeneratedCommandsNV(commandBuffer->command_buffer, &pGeneratedCommandsInfo_host);

    free_VkGeneratedCommandsInfoNV(&pGeneratedCommandsInfo_host);
#else
    TRACE("%p, %p\n", commandBuffer, pGeneratedCommandsInfo);
    commandBuffer->device->funcs.p_vkCmdPreprocessGeneratedCommandsNV(commandBuffer->command_buffer, pGeneratedCommandsInfo);
#endif
}

void WINAPI wine_vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues)
{
    TRACE("%p, 0x%s, %#x, %u, %u, %p\n", commandBuffer, wine_dbgstr_longlong(layout), stageFlags, offset, size, pValues);
    commandBuffer->device->funcs.p_vkCmdPushConstants(commandBuffer->command_buffer, layout, stageFlags, offset, size, pValues);
}

static void WINAPI wine_vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites)
{
#if defined(USE_STRUCT_CONVERSION)
    VkWriteDescriptorSet_host *pDescriptorWrites_host;
    TRACE("%p, %#x, 0x%s, %u, %u, %p\n", commandBuffer, pipelineBindPoint, wine_dbgstr_longlong(layout), set, descriptorWriteCount, pDescriptorWrites);

    pDescriptorWrites_host = convert_VkWriteDescriptorSet_array_win_to_host(pDescriptorWrites, descriptorWriteCount);
    commandBuffer->device->funcs.p_vkCmdPushDescriptorSetKHR(commandBuffer->command_buffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites_host);

    free_VkWriteDescriptorSet_array(pDescriptorWrites_host, descriptorWriteCount);
#else
    TRACE("%p, %#x, 0x%s, %u, %u, %p\n", commandBuffer, pipelineBindPoint, wine_dbgstr_longlong(layout), set, descriptorWriteCount, pDescriptorWrites);
    commandBuffer->device->funcs.p_vkCmdPushDescriptorSetKHR(commandBuffer->command_buffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
#endif
}

static void WINAPI wine_vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void *pData)
{
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", commandBuffer, wine_dbgstr_longlong(descriptorUpdateTemplate), wine_dbgstr_longlong(layout), set, pData);
    commandBuffer->device->funcs.p_vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer->command_buffer, descriptorUpdateTemplate, layout, set, pData);
}

void WINAPI wine_vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    TRACE("%p, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(event), stageMask);
    commandBuffer->device->funcs.p_vkCmdResetEvent(commandBuffer->command_buffer, event, stageMask);
}

void WINAPI wine_vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    TRACE("%p, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(queryPool), firstQuery, queryCount);
    commandBuffer->device->funcs.p_vkCmdResetQueryPool(commandBuffer->command_buffer, queryPool, firstQuery, queryCount);
}

void WINAPI wine_vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions)
{
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions);
    commandBuffer->device->funcs.p_vkCmdResolveImage(commandBuffer->command_buffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

static void WINAPI wine_vkCmdResolveImage2KHR(VkCommandBuffer commandBuffer, const VkResolveImageInfo2KHR *pResolveImageInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResolveImageInfo2KHR_host pResolveImageInfo_host;
    TRACE("%p, %p\n", commandBuffer, pResolveImageInfo);

    convert_VkResolveImageInfo2KHR_win_to_host(pResolveImageInfo, &pResolveImageInfo_host);
    commandBuffer->device->funcs.p_vkCmdResolveImage2KHR(commandBuffer->command_buffer, &pResolveImageInfo_host);

#else
    TRACE("%p, %p\n", commandBuffer, pResolveImageInfo);
    commandBuffer->device->funcs.p_vkCmdResolveImage2KHR(commandBuffer->command_buffer, pResolveImageInfo);
#endif
}

void WINAPI wine_vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    TRACE("%p, %p\n", commandBuffer, blendConstants);
    commandBuffer->device->funcs.p_vkCmdSetBlendConstants(commandBuffer->command_buffer, blendConstants);
}

static void WINAPI wine_vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    TRACE("%p, %p\n", commandBuffer, pCheckpointMarker);
    commandBuffer->device->funcs.p_vkCmdSetCheckpointNV(commandBuffer->command_buffer, pCheckpointMarker);
}

static void WINAPI wine_vkCmdSetCoarseSampleOrderNV(VkCommandBuffer commandBuffer, VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV *pCustomSampleOrders)
{
    TRACE("%p, %#x, %u, %p\n", commandBuffer, sampleOrderType, customSampleOrderCount, pCustomSampleOrders);
    commandBuffer->device->funcs.p_vkCmdSetCoarseSampleOrderNV(commandBuffer->command_buffer, sampleOrderType, customSampleOrderCount, pCustomSampleOrders);
}

static void WINAPI wine_vkCmdSetCullModeEXT(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
{
    TRACE("%p, %#x\n", commandBuffer, cullMode);
    commandBuffer->device->funcs.p_vkCmdSetCullModeEXT(commandBuffer->command_buffer, cullMode);
}

void WINAPI wine_vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    TRACE("%p, %f, %f, %f\n", commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
    commandBuffer->device->funcs.p_vkCmdSetDepthBias(commandBuffer->command_buffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void WINAPI wine_vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    TRACE("%p, %f, %f\n", commandBuffer, minDepthBounds, maxDepthBounds);
    commandBuffer->device->funcs.p_vkCmdSetDepthBounds(commandBuffer->command_buffer, minDepthBounds, maxDepthBounds);
}

static void WINAPI wine_vkCmdSetDepthBoundsTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable)
{
    TRACE("%p, %u\n", commandBuffer, depthBoundsTestEnable);
    commandBuffer->device->funcs.p_vkCmdSetDepthBoundsTestEnableEXT(commandBuffer->command_buffer, depthBoundsTestEnable);
}

static void WINAPI wine_vkCmdSetDepthCompareOpEXT(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp)
{
    TRACE("%p, %#x\n", commandBuffer, depthCompareOp);
    commandBuffer->device->funcs.p_vkCmdSetDepthCompareOpEXT(commandBuffer->command_buffer, depthCompareOp);
}

static void WINAPI wine_vkCmdSetDepthTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    TRACE("%p, %u\n", commandBuffer, depthTestEnable);
    commandBuffer->device->funcs.p_vkCmdSetDepthTestEnableEXT(commandBuffer->command_buffer, depthTestEnable);
}

static void WINAPI wine_vkCmdSetDepthWriteEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable)
{
    TRACE("%p, %u\n", commandBuffer, depthWriteEnable);
    commandBuffer->device->funcs.p_vkCmdSetDepthWriteEnableEXT(commandBuffer->command_buffer, depthWriteEnable);
}

void WINAPI wine_vkCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    TRACE("%p, %u\n", commandBuffer, deviceMask);
    commandBuffer->device->funcs.p_vkCmdSetDeviceMask(commandBuffer->command_buffer, deviceMask);
}

static void WINAPI wine_vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    TRACE("%p, %u\n", commandBuffer, deviceMask);
    commandBuffer->device->funcs.p_vkCmdSetDeviceMaskKHR(commandBuffer->command_buffer, deviceMask);
}

static void WINAPI wine_vkCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D *pDiscardRectangles)
{
    TRACE("%p, %u, %u, %p\n", commandBuffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
    commandBuffer->device->funcs.p_vkCmdSetDiscardRectangleEXT(commandBuffer->command_buffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
}

void WINAPI wine_vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    TRACE("%p, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(event), stageMask);
    commandBuffer->device->funcs.p_vkCmdSetEvent(commandBuffer->command_buffer, event, stageMask);
}

static void WINAPI wine_vkCmdSetExclusiveScissorNV(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D *pExclusiveScissors)
{
    TRACE("%p, %u, %u, %p\n", commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissors);
    commandBuffer->device->funcs.p_vkCmdSetExclusiveScissorNV(commandBuffer->command_buffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissors);
}

static void WINAPI wine_vkCmdSetFragmentShadingRateEnumNV(VkCommandBuffer commandBuffer, VkFragmentShadingRateNV shadingRate, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    TRACE("%p, %#x, %p\n", commandBuffer, shadingRate, combinerOps);
    commandBuffer->device->funcs.p_vkCmdSetFragmentShadingRateEnumNV(commandBuffer->command_buffer, shadingRate, combinerOps);
}

static void WINAPI wine_vkCmdSetFragmentShadingRateKHR(VkCommandBuffer commandBuffer, const VkExtent2D *pFragmentSize, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    TRACE("%p, %p, %p\n", commandBuffer, pFragmentSize, combinerOps);
    commandBuffer->device->funcs.p_vkCmdSetFragmentShadingRateKHR(commandBuffer->command_buffer, pFragmentSize, combinerOps);
}

static void WINAPI wine_vkCmdSetFrontFaceEXT(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
{
    TRACE("%p, %#x\n", commandBuffer, frontFace);
    commandBuffer->device->funcs.p_vkCmdSetFrontFaceEXT(commandBuffer->command_buffer, frontFace);
}

static void WINAPI wine_vkCmdSetLineStippleEXT(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern)
{
    TRACE("%p, %u, %u\n", commandBuffer, lineStippleFactor, lineStipplePattern);
    commandBuffer->device->funcs.p_vkCmdSetLineStippleEXT(commandBuffer->command_buffer, lineStippleFactor, lineStipplePattern);
}

void WINAPI wine_vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    TRACE("%p, %f\n", commandBuffer, lineWidth);
    commandBuffer->device->funcs.p_vkCmdSetLineWidth(commandBuffer->command_buffer, lineWidth);
}

static VkResult WINAPI wine_vkCmdSetPerformanceMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceMarkerInfoINTEL *pMarkerInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPerformanceMarkerInfoINTEL_host pMarkerInfo_host;
    TRACE("%p, %p\n", commandBuffer, pMarkerInfo);

    convert_VkPerformanceMarkerInfoINTEL_win_to_host(pMarkerInfo, &pMarkerInfo_host);
    result = commandBuffer->device->funcs.p_vkCmdSetPerformanceMarkerINTEL(commandBuffer->command_buffer, &pMarkerInfo_host);

    return result;
#else
    TRACE("%p, %p\n", commandBuffer, pMarkerInfo);
    return commandBuffer->device->funcs.p_vkCmdSetPerformanceMarkerINTEL(commandBuffer->command_buffer, pMarkerInfo);
#endif
}

static VkResult WINAPI wine_vkCmdSetPerformanceOverrideINTEL(VkCommandBuffer commandBuffer, const VkPerformanceOverrideInfoINTEL *pOverrideInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPerformanceOverrideInfoINTEL_host pOverrideInfo_host;
    TRACE("%p, %p\n", commandBuffer, pOverrideInfo);

    convert_VkPerformanceOverrideInfoINTEL_win_to_host(pOverrideInfo, &pOverrideInfo_host);
    result = commandBuffer->device->funcs.p_vkCmdSetPerformanceOverrideINTEL(commandBuffer->command_buffer, &pOverrideInfo_host);

    return result;
#else
    TRACE("%p, %p\n", commandBuffer, pOverrideInfo);
    return commandBuffer->device->funcs.p_vkCmdSetPerformanceOverrideINTEL(commandBuffer->command_buffer, pOverrideInfo);
#endif
}

static VkResult WINAPI wine_vkCmdSetPerformanceStreamMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo)
{
    TRACE("%p, %p\n", commandBuffer, pMarkerInfo);
    return commandBuffer->device->funcs.p_vkCmdSetPerformanceStreamMarkerINTEL(commandBuffer->command_buffer, pMarkerInfo);
}

static void WINAPI wine_vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
    TRACE("%p, %#x\n", commandBuffer, primitiveTopology);
    commandBuffer->device->funcs.p_vkCmdSetPrimitiveTopologyEXT(commandBuffer->command_buffer, primitiveTopology);
}

static void WINAPI wine_vkCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT *pSampleLocationsInfo)
{
    TRACE("%p, %p\n", commandBuffer, pSampleLocationsInfo);
    commandBuffer->device->funcs.p_vkCmdSetSampleLocationsEXT(commandBuffer->command_buffer, pSampleLocationsInfo);
}

void WINAPI wine_vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors)
{
    TRACE("%p, %u, %u, %p\n", commandBuffer, firstScissor, scissorCount, pScissors);
    commandBuffer->device->funcs.p_vkCmdSetScissor(commandBuffer->command_buffer, firstScissor, scissorCount, pScissors);
}

static void WINAPI wine_vkCmdSetScissorWithCountEXT(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors)
{
    TRACE("%p, %u, %p\n", commandBuffer, scissorCount, pScissors);
    commandBuffer->device->funcs.p_vkCmdSetScissorWithCountEXT(commandBuffer->command_buffer, scissorCount, pScissors);
}

void WINAPI wine_vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    TRACE("%p, %#x, %u\n", commandBuffer, faceMask, compareMask);
    commandBuffer->device->funcs.p_vkCmdSetStencilCompareMask(commandBuffer->command_buffer, faceMask, compareMask);
}

static void WINAPI wine_vkCmdSetStencilOpEXT(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
    TRACE("%p, %#x, %#x, %#x, %#x, %#x\n", commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
    commandBuffer->device->funcs.p_vkCmdSetStencilOpEXT(commandBuffer->command_buffer, faceMask, failOp, passOp, depthFailOp, compareOp);
}

void WINAPI wine_vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    TRACE("%p, %#x, %u\n", commandBuffer, faceMask, reference);
    commandBuffer->device->funcs.p_vkCmdSetStencilReference(commandBuffer->command_buffer, faceMask, reference);
}

static void WINAPI wine_vkCmdSetStencilTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable)
{
    TRACE("%p, %u\n", commandBuffer, stencilTestEnable);
    commandBuffer->device->funcs.p_vkCmdSetStencilTestEnableEXT(commandBuffer->command_buffer, stencilTestEnable);
}

void WINAPI wine_vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    TRACE("%p, %#x, %u\n", commandBuffer, faceMask, writeMask);
    commandBuffer->device->funcs.p_vkCmdSetStencilWriteMask(commandBuffer->command_buffer, faceMask, writeMask);
}

void WINAPI wine_vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports)
{
    TRACE("%p, %u, %u, %p\n", commandBuffer, firstViewport, viewportCount, pViewports);
    commandBuffer->device->funcs.p_vkCmdSetViewport(commandBuffer->command_buffer, firstViewport, viewportCount, pViewports);
}

static void WINAPI wine_vkCmdSetViewportShadingRatePaletteNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV *pShadingRatePalettes)
{
    TRACE("%p, %u, %u, %p\n", commandBuffer, firstViewport, viewportCount, pShadingRatePalettes);
    commandBuffer->device->funcs.p_vkCmdSetViewportShadingRatePaletteNV(commandBuffer->command_buffer, firstViewport, viewportCount, pShadingRatePalettes);
}

static void WINAPI wine_vkCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV *pViewportWScalings)
{
    TRACE("%p, %u, %u, %p\n", commandBuffer, firstViewport, viewportCount, pViewportWScalings);
    commandBuffer->device->funcs.p_vkCmdSetViewportWScalingNV(commandBuffer->command_buffer, firstViewport, viewportCount, pViewportWScalings);
}

static void WINAPI wine_vkCmdSetViewportWithCountEXT(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports)
{
    TRACE("%p, %u, %p\n", commandBuffer, viewportCount, pViewports);
    commandBuffer->device->funcs.p_vkCmdSetViewportWithCountEXT(commandBuffer->command_buffer, viewportCount, pViewports);
}

static void WINAPI wine_vkCmdTraceRaysNV(VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u, %u\n", commandBuffer, wine_dbgstr_longlong(raygenShaderBindingTableBuffer), wine_dbgstr_longlong(raygenShaderBindingOffset), wine_dbgstr_longlong(missShaderBindingTableBuffer), wine_dbgstr_longlong(missShaderBindingOffset), wine_dbgstr_longlong(missShaderBindingStride), wine_dbgstr_longlong(hitShaderBindingTableBuffer), wine_dbgstr_longlong(hitShaderBindingOffset), wine_dbgstr_longlong(hitShaderBindingStride), wine_dbgstr_longlong(callableShaderBindingTableBuffer), wine_dbgstr_longlong(callableShaderBindingOffset), wine_dbgstr_longlong(callableShaderBindingStride), width, height, depth);
    commandBuffer->device->funcs.p_vkCmdTraceRaysNV(commandBuffer->command_buffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset, missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride, hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride, callableShaderBindingTableBuffer, callableShaderBindingOffset, callableShaderBindingStride, width, height, depth);
}

void WINAPI wine_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, %p\n", commandBuffer, wine_dbgstr_longlong(dstBuffer), wine_dbgstr_longlong(dstOffset), wine_dbgstr_longlong(dataSize), pData);
    commandBuffer->device->funcs.p_vkCmdUpdateBuffer(commandBuffer->command_buffer, dstBuffer, dstOffset, dataSize, pData);
}

void WINAPI wine_vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryBarrier_host *pBufferMemoryBarriers_host;
    VkImageMemoryBarrier_host *pImageMemoryBarriers_host;
    TRACE("%p, %u, %p, %#x, %#x, %u, %p, %u, %p, %u, %p\n", commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);

    pBufferMemoryBarriers_host = convert_VkBufferMemoryBarrier_array_win_to_host(pBufferMemoryBarriers, bufferMemoryBarrierCount);
    pImageMemoryBarriers_host = convert_VkImageMemoryBarrier_array_win_to_host(pImageMemoryBarriers, imageMemoryBarrierCount);
    commandBuffer->device->funcs.p_vkCmdWaitEvents(commandBuffer->command_buffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers_host, imageMemoryBarrierCount, pImageMemoryBarriers_host);

    free_VkBufferMemoryBarrier_array(pBufferMemoryBarriers_host, bufferMemoryBarrierCount);
    free_VkImageMemoryBarrier_array(pImageMemoryBarriers_host, imageMemoryBarrierCount);
#else
    TRACE("%p, %u, %p, %#x, %#x, %u, %p, %u, %p, %u, %p\n", commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    commandBuffer->device->funcs.p_vkCmdWaitEvents(commandBuffer->command_buffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
#endif
}

static void WINAPI wine_vkCmdWriteAccelerationStructuresPropertiesNV(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, wine_dbgstr_longlong(queryPool), firstQuery);
    commandBuffer->device->funcs.p_vkCmdWriteAccelerationStructuresPropertiesNV(commandBuffer->command_buffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

static void WINAPI wine_vkCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    TRACE("%p, %#x, 0x%s, 0x%s, %u\n", commandBuffer, pipelineStage, wine_dbgstr_longlong(dstBuffer), wine_dbgstr_longlong(dstOffset), marker);
    commandBuffer->device->funcs.p_vkCmdWriteBufferMarkerAMD(commandBuffer->command_buffer, pipelineStage, dstBuffer, dstOffset, marker);
}

void WINAPI wine_vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    TRACE("%p, %#x, 0x%s, %u\n", commandBuffer, pipelineStage, wine_dbgstr_longlong(queryPool), query);
    commandBuffer->device->funcs.p_vkCmdWriteTimestamp(commandBuffer->command_buffer, pipelineStage, queryPool, query);
}

static VkResult WINAPI wine_vkCompileDeferredNV(VkDevice device, VkPipeline pipeline, uint32_t shader)
{
    TRACE("%p, 0x%s, %u\n", device, wine_dbgstr_longlong(pipeline), shader);
    return device->funcs.p_vkCompileDeferredNV(device->device, pipeline, shader);
}

static VkResult WINAPI wine_vkCreateAccelerationStructureNV(VkDevice device, const VkAccelerationStructureCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureNV *pAccelerationStructure)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAccelerationStructureCreateInfoNV_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pAccelerationStructure);

    convert_VkAccelerationStructureCreateInfoNV_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateAccelerationStructureNV(device->device, &pCreateInfo_host, NULL, pAccelerationStructure);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pAccelerationStructure);
    return device->funcs.p_vkCreateAccelerationStructureNV(device->device, pCreateInfo, NULL, pAccelerationStructure);
#endif
}

VkResult WINAPI wine_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBufferCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pBuffer);

    convert_VkBufferCreateInfo_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateBuffer(device->device, &pCreateInfo_host, NULL, pBuffer);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pBuffer);
    return device->funcs.p_vkCreateBuffer(device->device, pCreateInfo, NULL, pBuffer);
#endif
}

VkResult WINAPI wine_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBufferViewCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pView);

    convert_VkBufferViewCreateInfo_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateBufferView(device->device, &pCreateInfo_host, NULL, pView);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pView);
    return device->funcs.p_vkCreateBufferView(device->device, pCreateInfo, NULL, pView);
#endif
}

VkResult WINAPI wine_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkComputePipelineCreateInfo_host *pCreateInfos_host;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), createInfoCount, pCreateInfos, pAllocator, pPipelines);

    pCreateInfos_host = convert_VkComputePipelineCreateInfo_array_win_to_host(pCreateInfos, createInfoCount);
    result = device->funcs.p_vkCreateComputePipelines(device->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);

    free_VkComputePipelineCreateInfo_array(pCreateInfos_host, createInfoCount);
    return result;
#else
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return device->funcs.p_vkCreateComputePipelines(device->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
#endif
}

VkResult WINAPI wine_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pDescriptorPool);
    return device->funcs.p_vkCreateDescriptorPool(device->device, pCreateInfo, NULL, pDescriptorPool);
}

VkResult WINAPI wine_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSetLayout);
    return device->funcs.p_vkCreateDescriptorSetLayout(device->device, pCreateInfo, NULL, pSetLayout);
}

VkResult WINAPI wine_vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDescriptorUpdateTemplateCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);

    convert_VkDescriptorUpdateTemplateCreateInfo_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateDescriptorUpdateTemplate(device->device, &pCreateInfo_host, NULL, pDescriptorUpdateTemplate);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
    return device->funcs.p_vkCreateDescriptorUpdateTemplate(device->device, pCreateInfo, NULL, pDescriptorUpdateTemplate);
#endif
}

static VkResult WINAPI wine_vkCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDescriptorUpdateTemplateCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);

    convert_VkDescriptorUpdateTemplateCreateInfo_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateDescriptorUpdateTemplateKHR(device->device, &pCreateInfo_host, NULL, pDescriptorUpdateTemplate);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
    return device->funcs.p_vkCreateDescriptorUpdateTemplateKHR(device->device, pCreateInfo, NULL, pDescriptorUpdateTemplate);
#endif
}

VkResult WINAPI wine_vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pEvent);
    return device->funcs.p_vkCreateEvent(device->device, pCreateInfo, NULL, pEvent);
}

VkResult WINAPI wine_vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pFence);
    return device->funcs.p_vkCreateFence(device->device, pCreateInfo, NULL, pFence);
}

VkResult WINAPI wine_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkFramebufferCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pFramebuffer);

    convert_VkFramebufferCreateInfo_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateFramebuffer(device->device, &pCreateInfo_host, NULL, pFramebuffer);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pFramebuffer);
    return device->funcs.p_vkCreateFramebuffer(device->device, pCreateInfo, NULL, pFramebuffer);
#endif
}

VkResult WINAPI wine_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkGraphicsPipelineCreateInfo_host *pCreateInfos_host;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), createInfoCount, pCreateInfos, pAllocator, pPipelines);

    pCreateInfos_host = convert_VkGraphicsPipelineCreateInfo_array_win_to_host(pCreateInfos, createInfoCount);
    result = device->funcs.p_vkCreateGraphicsPipelines(device->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);

    free_VkGraphicsPipelineCreateInfo_array(pCreateInfos_host, createInfoCount);
    return result;
#else
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return device->funcs.p_vkCreateGraphicsPipelines(device->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
#endif
}

static VkResult WINAPI wine_vkCreateHeadlessSurfaceEXT(VkInstance instance, const VkHeadlessSurfaceCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
    TRACE("%p, %p, %p, %p\n", instance, pCreateInfo, pAllocator, pSurface);
    return instance->funcs.p_vkCreateHeadlessSurfaceEXT(instance->instance, pCreateInfo, NULL, pSurface);
}

VkResult WINAPI wine_vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pImage);
    return device->funcs.p_vkCreateImage(device->device, pCreateInfo, NULL, pImage);
}

VkResult WINAPI wine_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageViewCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pView);

    convert_VkImageViewCreateInfo_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateImageView(device->device, &pCreateInfo_host, NULL, pView);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pView);
    return device->funcs.p_vkCreateImageView(device->device, pCreateInfo, NULL, pView);
#endif
}

static VkResult WINAPI wine_vkCreateIndirectCommandsLayoutNV(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutNV *pIndirectCommandsLayout)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkIndirectCommandsLayoutCreateInfoNV_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pIndirectCommandsLayout);

    convert_VkIndirectCommandsLayoutCreateInfoNV_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateIndirectCommandsLayoutNV(device->device, &pCreateInfo_host, NULL, pIndirectCommandsLayout);

    free_VkIndirectCommandsLayoutCreateInfoNV(&pCreateInfo_host);
    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
    return device->funcs.p_vkCreateIndirectCommandsLayoutNV(device->device, pCreateInfo, NULL, pIndirectCommandsLayout);
#endif
}

VkResult WINAPI wine_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pPipelineCache);
    return device->funcs.p_vkCreatePipelineCache(device->device, pCreateInfo, NULL, pPipelineCache);
}

VkResult WINAPI wine_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pPipelineLayout);
    return device->funcs.p_vkCreatePipelineLayout(device->device, pCreateInfo, NULL, pPipelineLayout);
}

static VkResult WINAPI wine_vkCreatePrivateDataSlotEXT(VkDevice device, const VkPrivateDataSlotCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlotEXT *pPrivateDataSlot)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pPrivateDataSlot);
    return device->funcs.p_vkCreatePrivateDataSlotEXT(device->device, pCreateInfo, NULL, pPrivateDataSlot);
}

VkResult WINAPI wine_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pQueryPool);
    return device->funcs.p_vkCreateQueryPool(device->device, pCreateInfo, NULL, pQueryPool);
}

static VkResult WINAPI wine_vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkRayTracingPipelineCreateInfoNV_host *pCreateInfos_host;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), createInfoCount, pCreateInfos, pAllocator, pPipelines);

    pCreateInfos_host = convert_VkRayTracingPipelineCreateInfoNV_array_win_to_host(pCreateInfos, createInfoCount);
    result = device->funcs.p_vkCreateRayTracingPipelinesNV(device->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);

    free_VkRayTracingPipelineCreateInfoNV_array(pCreateInfos_host, createInfoCount);
    return result;
#else
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return device->funcs.p_vkCreateRayTracingPipelinesNV(device->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
#endif
}

VkResult WINAPI wine_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pRenderPass);
    return device->funcs.p_vkCreateRenderPass(device->device, pCreateInfo, NULL, pRenderPass);
}

VkResult WINAPI wine_vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pRenderPass);
    return device->funcs.p_vkCreateRenderPass2(device->device, pCreateInfo, NULL, pRenderPass);
}

static VkResult WINAPI wine_vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pRenderPass);
    return device->funcs.p_vkCreateRenderPass2KHR(device->device, pCreateInfo, NULL, pRenderPass);
}

VkResult WINAPI wine_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSampler);
    return device->funcs.p_vkCreateSampler(device->device, pCreateInfo, NULL, pSampler);
}

VkResult WINAPI wine_vkCreateSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pYcbcrConversion);
    return device->funcs.p_vkCreateSamplerYcbcrConversion(device->device, pCreateInfo, NULL, pYcbcrConversion);
}

static VkResult WINAPI wine_vkCreateSamplerYcbcrConversionKHR(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pYcbcrConversion);
    return device->funcs.p_vkCreateSamplerYcbcrConversionKHR(device->device, pCreateInfo, NULL, pYcbcrConversion);
}

VkResult WINAPI wine_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSemaphore);
    return device->funcs.p_vkCreateSemaphore(device->device, pCreateInfo, NULL, pSemaphore);
}

VkResult WINAPI wine_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pShaderModule);
    return device->funcs.p_vkCreateShaderModule(device->device, pCreateInfo, NULL, pShaderModule);
}

VkResult WINAPI wine_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSwapchainCreateInfoKHR_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSwapchain);

    convert_VkSwapchainCreateInfoKHR_win_to_host(pCreateInfo, &pCreateInfo_host);
    result = device->funcs.p_vkCreateSwapchainKHR(device->device, &pCreateInfo_host, NULL, pSwapchain);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSwapchain);
    return device->funcs.p_vkCreateSwapchainKHR(device->device, pCreateInfo, NULL, pSwapchain);
#endif
}

static VkResult WINAPI wine_vkCreateValidationCacheEXT(VkDevice device, const VkValidationCacheCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkValidationCacheEXT *pValidationCache)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pValidationCache);
    return device->funcs.p_vkCreateValidationCacheEXT(device->device, pCreateInfo, NULL, pValidationCache);
}

VkResult WINAPI wine_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
    TRACE("%p, %p, %p, %p\n", instance, pCreateInfo, pAllocator, pSurface);
    return instance->funcs.p_vkCreateWin32SurfaceKHR(instance->instance, pCreateInfo, NULL, pSurface);
}

VkResult thunk_vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugMarkerObjectNameInfoEXT_host pNameInfo_host;
    convert_VkDebugMarkerObjectNameInfoEXT_win_to_host(pNameInfo, &pNameInfo_host);
    result = device->funcs.p_vkDebugMarkerSetObjectNameEXT(device->device, &pNameInfo_host);

    return result;
#else
    return device->funcs.p_vkDebugMarkerSetObjectNameEXT(device->device, pNameInfo);
#endif
}

VkResult thunk_vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugMarkerObjectTagInfoEXT_host pTagInfo_host;
    convert_VkDebugMarkerObjectTagInfoEXT_win_to_host(pTagInfo, &pTagInfo_host);
    result = device->funcs.p_vkDebugMarkerSetObjectTagEXT(device->device, &pTagInfo_host);

    return result;
#else
    return device->funcs.p_vkDebugMarkerSetObjectTagEXT(device->device, pTagInfo);
#endif
}

static void WINAPI wine_vkDestroyAccelerationStructureNV(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(accelerationStructure), pAllocator);
    device->funcs.p_vkDestroyAccelerationStructureNV(device->device, accelerationStructure, NULL);
}

void WINAPI wine_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(buffer), pAllocator);
    device->funcs.p_vkDestroyBuffer(device->device, buffer, NULL);
}

void WINAPI wine_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(bufferView), pAllocator);
    device->funcs.p_vkDestroyBufferView(device->device, bufferView, NULL);
}

void WINAPI wine_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorPool), pAllocator);
    device->funcs.p_vkDestroyDescriptorPool(device->device, descriptorPool, NULL);
}

void WINAPI wine_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorSetLayout), pAllocator);
    device->funcs.p_vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, NULL);
}

void WINAPI wine_vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorUpdateTemplate), pAllocator);
    device->funcs.p_vkDestroyDescriptorUpdateTemplate(device->device, descriptorUpdateTemplate, NULL);
}

static void WINAPI wine_vkDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorUpdateTemplate), pAllocator);
    device->funcs.p_vkDestroyDescriptorUpdateTemplateKHR(device->device, descriptorUpdateTemplate, NULL);
}

void WINAPI wine_vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(event), pAllocator);
    device->funcs.p_vkDestroyEvent(device->device, event, NULL);
}

void WINAPI wine_vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(fence), pAllocator);
    device->funcs.p_vkDestroyFence(device->device, fence, NULL);
}

void WINAPI wine_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(framebuffer), pAllocator);
    device->funcs.p_vkDestroyFramebuffer(device->device, framebuffer, NULL);
}

void WINAPI wine_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(image), pAllocator);
    device->funcs.p_vkDestroyImage(device->device, image, NULL);
}

void WINAPI wine_vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(imageView), pAllocator);
    device->funcs.p_vkDestroyImageView(device->device, imageView, NULL);
}

static void WINAPI wine_vkDestroyIndirectCommandsLayoutNV(VkDevice device, VkIndirectCommandsLayoutNV indirectCommandsLayout, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(indirectCommandsLayout), pAllocator);
    device->funcs.p_vkDestroyIndirectCommandsLayoutNV(device->device, indirectCommandsLayout, NULL);
}

void WINAPI wine_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(pipeline), pAllocator);
    device->funcs.p_vkDestroyPipeline(device->device, pipeline, NULL);
}

void WINAPI wine_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(pipelineCache), pAllocator);
    device->funcs.p_vkDestroyPipelineCache(device->device, pipelineCache, NULL);
}

void WINAPI wine_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(pipelineLayout), pAllocator);
    device->funcs.p_vkDestroyPipelineLayout(device->device, pipelineLayout, NULL);
}

static void WINAPI wine_vkDestroyPrivateDataSlotEXT(VkDevice device, VkPrivateDataSlotEXT privateDataSlot, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(privateDataSlot), pAllocator);
    device->funcs.p_vkDestroyPrivateDataSlotEXT(device->device, privateDataSlot, NULL);
}

void WINAPI wine_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(queryPool), pAllocator);
    device->funcs.p_vkDestroyQueryPool(device->device, queryPool, NULL);
}

void WINAPI wine_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(renderPass), pAllocator);
    device->funcs.p_vkDestroyRenderPass(device->device, renderPass, NULL);
}

void WINAPI wine_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(sampler), pAllocator);
    device->funcs.p_vkDestroySampler(device->device, sampler, NULL);
}

void WINAPI wine_vkDestroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(ycbcrConversion), pAllocator);
    device->funcs.p_vkDestroySamplerYcbcrConversion(device->device, ycbcrConversion, NULL);
}

static void WINAPI wine_vkDestroySamplerYcbcrConversionKHR(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(ycbcrConversion), pAllocator);
    device->funcs.p_vkDestroySamplerYcbcrConversionKHR(device->device, ycbcrConversion, NULL);
}

void WINAPI wine_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(semaphore), pAllocator);
    device->funcs.p_vkDestroySemaphore(device->device, semaphore, NULL);
}

void WINAPI wine_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(shaderModule), pAllocator);
    device->funcs.p_vkDestroyShaderModule(device->device, shaderModule, NULL);
}

void WINAPI wine_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", instance, wine_dbgstr_longlong(surface), pAllocator);
    instance->funcs.p_vkDestroySurfaceKHR(instance->instance, surface, NULL);
}

void WINAPI wine_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(swapchain), pAllocator);
    device->funcs.p_vkDestroySwapchainKHR(device->device, swapchain, NULL);
}

static void WINAPI wine_vkDestroyValidationCacheEXT(VkDevice device, VkValidationCacheEXT validationCache, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(validationCache), pAllocator);
    device->funcs.p_vkDestroyValidationCacheEXT(device->device, validationCache, NULL);
}

VkResult WINAPI wine_vkDeviceWaitIdle(VkDevice device)
{
    TRACE("%p\n", device);
    return device->funcs.p_vkDeviceWaitIdle(device->device);
}

VkResult WINAPI wine_vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    TRACE("%p\n", commandBuffer);
    return commandBuffer->device->funcs.p_vkEndCommandBuffer(commandBuffer->command_buffer);
}

VkResult WINAPI wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pPropertyCount, pProperties);
    return physicalDevice->instance->funcs.p_vkEnumerateDeviceLayerProperties(physicalDevice->phys_dev, pPropertyCount, pProperties);
}

static VkResult WINAPI wine_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pCounterCount, VkPerformanceCounterKHR *pCounters, VkPerformanceCounterDescriptionKHR *pCounterDescriptions)
{
    TRACE("%p, %u, %p, %p, %p\n", physicalDevice, queueFamilyIndex, pCounterCount, pCounters, pCounterDescriptions);
    return physicalDevice->instance->funcs.p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(physicalDevice->phys_dev, queueFamilyIndex, pCounterCount, pCounters, pCounterDescriptions);
}

VkResult WINAPI wine_vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkMappedMemoryRange_host *pMemoryRanges_host;
    TRACE("%p, %u, %p\n", device, memoryRangeCount, pMemoryRanges);

    pMemoryRanges_host = convert_VkMappedMemoryRange_array_win_to_host(pMemoryRanges, memoryRangeCount);
    result = device->funcs.p_vkFlushMappedMemoryRanges(device->device, memoryRangeCount, pMemoryRanges_host);

    free_VkMappedMemoryRange_array(pMemoryRanges_host, memoryRangeCount);
    return result;
#else
    TRACE("%p, %u, %p\n", device, memoryRangeCount, pMemoryRanges);
    return device->funcs.p_vkFlushMappedMemoryRanges(device->device, memoryRangeCount, pMemoryRanges);
#endif
}

VkResult WINAPI wine_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets)
{
    TRACE("%p, 0x%s, %u, %p\n", device, wine_dbgstr_longlong(descriptorPool), descriptorSetCount, pDescriptorSets);
    return device->funcs.p_vkFreeDescriptorSets(device->device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void WINAPI wine_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(memory), pAllocator);
    device->funcs.p_vkFreeMemory(device->device, memory, NULL);
}

static VkResult WINAPI wine_vkGetAccelerationStructureHandleNV(VkDevice device, VkAccelerationStructureKHR accelerationStructure, size_t dataSize, void *pData)
{
    TRACE("%p, 0x%s, 0x%s, %p\n", device, wine_dbgstr_longlong(accelerationStructure), wine_dbgstr_longlong(dataSize), pData);
    return device->funcs.p_vkGetAccelerationStructureHandleNV(device->device, accelerationStructure, dataSize, pData);
}

static void WINAPI wine_vkGetAccelerationStructureMemoryRequirementsNV(VkDevice device, const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2KHR *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureMemoryRequirementsInfoNV_host pInfo_host;
    VkMemoryRequirements2KHR_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);

    convert_VkAccelerationStructureMemoryRequirementsInfoNV_win_to_host(pInfo, &pInfo_host);
    convert_VkMemoryRequirements2KHR_win_to_host(pMemoryRequirements, &pMemoryRequirements_host);
    device->funcs.p_vkGetAccelerationStructureMemoryRequirementsNV(device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2KHR_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);
    device->funcs.p_vkGetAccelerationStructureMemoryRequirementsNV(device->device, pInfo, pMemoryRequirements);
#endif
}

VkDeviceAddress WINAPI wine_vkGetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkDeviceAddress result;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkGetBufferDeviceAddress(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkGetBufferDeviceAddress(device->device, pInfo);
#endif
}

static VkDeviceAddress WINAPI wine_vkGetBufferDeviceAddressEXT(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkDeviceAddress result;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkGetBufferDeviceAddressEXT(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkGetBufferDeviceAddressEXT(device->device, pInfo);
#endif
}

static VkDeviceAddress WINAPI wine_vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkDeviceAddress result;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkGetBufferDeviceAddressKHR(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkGetBufferDeviceAddressKHR(device->device, pInfo);
#endif
}

void WINAPI wine_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkMemoryRequirements_host pMemoryRequirements_host;
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(buffer), pMemoryRequirements);

    device->funcs.p_vkGetBufferMemoryRequirements(device->device, buffer, &pMemoryRequirements_host);

    convert_VkMemoryRequirements_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(buffer), pMemoryRequirements);
    device->funcs.p_vkGetBufferMemoryRequirements(device->device, buffer, pMemoryRequirements);
#endif
}

void WINAPI wine_vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);

    convert_VkBufferMemoryRequirementsInfo2_win_to_host(pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(pMemoryRequirements, &pMemoryRequirements_host);
    device->funcs.p_vkGetBufferMemoryRequirements2(device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);
    device->funcs.p_vkGetBufferMemoryRequirements2(device->device, pInfo, pMemoryRequirements);
#endif
}

static void WINAPI wine_vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);

    convert_VkBufferMemoryRequirementsInfo2_win_to_host(pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(pMemoryRequirements, &pMemoryRequirements_host);
    device->funcs.p_vkGetBufferMemoryRequirements2KHR(device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);
    device->funcs.p_vkGetBufferMemoryRequirements2KHR(device->device, pInfo, pMemoryRequirements);
#endif
}

uint64_t WINAPI wine_vkGetBufferOpaqueCaptureAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    uint64_t result;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkGetBufferOpaqueCaptureAddress(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkGetBufferOpaqueCaptureAddress(device->device, pInfo);
#endif
}

static uint64_t WINAPI wine_vkGetBufferOpaqueCaptureAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    uint64_t result;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkGetBufferOpaqueCaptureAddressKHR(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkGetBufferOpaqueCaptureAddressKHR(device->device, pInfo);
#endif
}

void WINAPI wine_vkGetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    TRACE("%p, %p, %p\n", device, pCreateInfo, pSupport);
    device->funcs.p_vkGetDescriptorSetLayoutSupport(device->device, pCreateInfo, pSupport);
}

static void WINAPI wine_vkGetDescriptorSetLayoutSupportKHR(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    TRACE("%p, %p, %p\n", device, pCreateInfo, pSupport);
    device->funcs.p_vkGetDescriptorSetLayoutSupportKHR(device->device, pCreateInfo, pSupport);
}

void WINAPI wine_vkGetDeviceGroupPeerMemoryFeatures(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    TRACE("%p, %u, %u, %u, %p\n", device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
    device->funcs.p_vkGetDeviceGroupPeerMemoryFeatures(device->device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

static void WINAPI wine_vkGetDeviceGroupPeerMemoryFeaturesKHR(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    TRACE("%p, %u, %u, %u, %p\n", device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
    device->funcs.p_vkGetDeviceGroupPeerMemoryFeaturesKHR(device->device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

VkResult WINAPI wine_vkGetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pDeviceGroupPresentCapabilities)
{
    TRACE("%p, %p\n", device, pDeviceGroupPresentCapabilities);
    return device->funcs.p_vkGetDeviceGroupPresentCapabilitiesKHR(device->device, pDeviceGroupPresentCapabilities);
}

VkResult WINAPI wine_vkGetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(surface), pModes);
    return device->funcs.p_vkGetDeviceGroupSurfacePresentModesKHR(device->device, surface, pModes);
}

void WINAPI wine_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(memory), pCommittedMemoryInBytes);
    device->funcs.p_vkGetDeviceMemoryCommitment(device->device, memory, pCommittedMemoryInBytes);
}

uint64_t WINAPI wine_vkGetDeviceMemoryOpaqueCaptureAddress(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    uint64_t result;
    VkDeviceMemoryOpaqueCaptureAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddress(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddress(device->device, pInfo);
#endif
}

static uint64_t WINAPI wine_vkGetDeviceMemoryOpaqueCaptureAddressKHR(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    uint64_t result;
    VkDeviceMemoryOpaqueCaptureAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", device, pInfo);

    convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win_to_host(pInfo, &pInfo_host);
    result = device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pInfo);
    return device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(device->device, pInfo);
#endif
}

VkResult WINAPI wine_vkGetEventStatus(VkDevice device, VkEvent event)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(event));
    return device->funcs.p_vkGetEventStatus(device->device, event);
}

VkResult WINAPI wine_vkGetFenceStatus(VkDevice device, VkFence fence)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(fence));
    return device->funcs.p_vkGetFenceStatus(device->device, fence);
}

static void WINAPI wine_vkGetGeneratedCommandsMemoryRequirementsNV(VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkGeneratedCommandsMemoryRequirementsInfoNV_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);

    convert_VkGeneratedCommandsMemoryRequirementsInfoNV_win_to_host(pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(pMemoryRequirements, &pMemoryRequirements_host);
    device->funcs.p_vkGetGeneratedCommandsMemoryRequirementsNV(device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);
    device->funcs.p_vkGetGeneratedCommandsMemoryRequirementsNV(device->device, pInfo, pMemoryRequirements);
#endif
}

void WINAPI wine_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkMemoryRequirements_host pMemoryRequirements_host;
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(image), pMemoryRequirements);

    device->funcs.p_vkGetImageMemoryRequirements(device->device, image, &pMemoryRequirements_host);

    convert_VkMemoryRequirements_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(image), pMemoryRequirements);
    device->funcs.p_vkGetImageMemoryRequirements(device->device, image, pMemoryRequirements);
#endif
}

void WINAPI wine_vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkImageMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);

    convert_VkImageMemoryRequirementsInfo2_win_to_host(pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(pMemoryRequirements, &pMemoryRequirements_host);
    device->funcs.p_vkGetImageMemoryRequirements2(device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);
    device->funcs.p_vkGetImageMemoryRequirements2(device->device, pInfo, pMemoryRequirements);
#endif
}

static void WINAPI wine_vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkImageMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);

    convert_VkImageMemoryRequirementsInfo2_win_to_host(pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(pMemoryRequirements, &pMemoryRequirements_host);
    device->funcs.p_vkGetImageMemoryRequirements2KHR(device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, pMemoryRequirements);
#else
    TRACE("%p, %p, %p\n", device, pInfo, pMemoryRequirements);
    device->funcs.p_vkGetImageMemoryRequirements2KHR(device->device, pInfo, pMemoryRequirements);
#endif
}

void WINAPI wine_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements *pSparseMemoryRequirements)
{
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(image), pSparseMemoryRequirementCount, pSparseMemoryRequirements);
    device->funcs.p_vkGetImageSparseMemoryRequirements(device->device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

void WINAPI wine_vkGetImageSparseMemoryRequirements2(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkImageSparseMemoryRequirementsInfo2_host pInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);

    convert_VkImageSparseMemoryRequirementsInfo2_win_to_host(pInfo, &pInfo_host);
    device->funcs.p_vkGetImageSparseMemoryRequirements2(device->device, &pInfo_host, pSparseMemoryRequirementCount, pSparseMemoryRequirements);

#else
    TRACE("%p, %p, %p, %p\n", device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
    device->funcs.p_vkGetImageSparseMemoryRequirements2(device->device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
#endif
}

static void WINAPI wine_vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
#if defined(USE_STRUCT_CONVERSION)
    VkImageSparseMemoryRequirementsInfo2_host pInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);

    convert_VkImageSparseMemoryRequirementsInfo2_win_to_host(pInfo, &pInfo_host);
    device->funcs.p_vkGetImageSparseMemoryRequirements2KHR(device->device, &pInfo_host, pSparseMemoryRequirementCount, pSparseMemoryRequirements);

#else
    TRACE("%p, %p, %p, %p\n", device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
    device->funcs.p_vkGetImageSparseMemoryRequirements2KHR(device->device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
#endif
}

void WINAPI wine_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout)
{
#if defined(USE_STRUCT_CONVERSION)
    VkSubresourceLayout_host pLayout_host;
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(image), pSubresource, pLayout);

    device->funcs.p_vkGetImageSubresourceLayout(device->device, image, pSubresource, &pLayout_host);

    convert_VkSubresourceLayout_host_to_win(&pLayout_host, pLayout);
#else
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(image), pSubresource, pLayout);
    device->funcs.p_vkGetImageSubresourceLayout(device->device, image, pSubresource, pLayout);
#endif
}

static VkResult WINAPI wine_vkGetMemoryHostPointerPropertiesEXT(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, const void *pHostPointer, VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties)
{
    TRACE("%p, %#x, %p, %p\n", device, handleType, pHostPointer, pMemoryHostPointerProperties);
    return device->funcs.p_vkGetMemoryHostPointerPropertiesEXT(device->device, handleType, pHostPointer, pMemoryHostPointerProperties);
}

static VkResult WINAPI wine_vkGetPerformanceParameterINTEL(VkDevice device, VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL *pValue)
{
    TRACE("%p, %#x, %p\n", device, parameter, pValue);
    return device->funcs.p_vkGetPerformanceParameterINTEL(device->device, parameter, pValue);
}

static VkResult WINAPI wine_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixPropertiesNV *pProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pPropertyCount, pProperties);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(physicalDevice->phys_dev, pPropertyCount, pProperties);
}

void WINAPI wine_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
    TRACE("%p, %p\n", physicalDevice, pFeatures);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFeatures(physicalDevice->phys_dev, pFeatures);
}

void WINAPI wine_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    TRACE("%p, %p\n", physicalDevice, pFeatures);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFeatures2(physicalDevice->phys_dev, pFeatures);
}

static void WINAPI wine_vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    TRACE("%p, %p\n", physicalDevice, pFeatures);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFeatures2KHR(physicalDevice->phys_dev, pFeatures);
}

void WINAPI wine_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties)
{
    TRACE("%p, %#x, %p\n", physicalDevice, format, pFormatProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFormatProperties(physicalDevice->phys_dev, format, pFormatProperties);
}

void WINAPI wine_vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    TRACE("%p, %#x, %p\n", physicalDevice, format, pFormatProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2(physicalDevice->phys_dev, format, pFormatProperties);
}

static void WINAPI wine_vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    TRACE("%p, %#x, %p\n", physicalDevice, format, pFormatProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2KHR(physicalDevice->phys_dev, format, pFormatProperties);
}

static VkResult WINAPI wine_vkGetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t *pFragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR *pFragmentShadingRates)
{
    TRACE("%p, %p, %p\n", physicalDevice, pFragmentShadingRateCount, pFragmentShadingRates);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFragmentShadingRatesKHR(physicalDevice->phys_dev, pFragmentShadingRateCount, pFragmentShadingRates);
}

VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageFormatProperties_host pImageFormatProperties_host;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p\n", physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);

    result = physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(physicalDevice->phys_dev, format, type, tiling, usage, flags, &pImageFormatProperties_host);

    convert_VkImageFormatProperties_host_to_win(&pImageFormatProperties_host, pImageFormatProperties);
    return result;
#else
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p\n", physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(physicalDevice->phys_dev, format, type, tiling, usage, flags, pImageFormatProperties);
#endif
}

VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageFormatProperties2_host pImageFormatProperties_host;
    convert_VkImageFormatProperties2_win_to_host(pImageFormatProperties, &pImageFormatProperties_host);
    result = physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2(physicalDevice->phys_dev, pImageFormatInfo, &pImageFormatProperties_host);

    convert_VkImageFormatProperties2_host_to_win(&pImageFormatProperties_host, pImageFormatProperties);
    return result;
#else
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2(physicalDevice->phys_dev, pImageFormatInfo, pImageFormatProperties);
#endif
}

VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageFormatProperties2_host pImageFormatProperties_host;
    convert_VkImageFormatProperties2_win_to_host(pImageFormatProperties, &pImageFormatProperties_host);
    result = physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2KHR(physicalDevice->phys_dev, pImageFormatInfo, &pImageFormatProperties_host);

    convert_VkImageFormatProperties2_host_to_win(&pImageFormatProperties_host, pImageFormatProperties);
    return result;
#else
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2KHR(physicalDevice->phys_dev, pImageFormatInfo, pImageFormatProperties);
#endif
}

void WINAPI wine_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceMemoryProperties_host pMemoryProperties_host;
    TRACE("%p, %p\n", physicalDevice, pMemoryProperties);

    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties(physicalDevice->phys_dev, &pMemoryProperties_host);

    convert_VkPhysicalDeviceMemoryProperties_host_to_win(&pMemoryProperties_host, pMemoryProperties);
#else
    TRACE("%p, %p\n", physicalDevice, pMemoryProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties(physicalDevice->phys_dev, pMemoryProperties);
#endif
}

void WINAPI wine_vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceMemoryProperties2_host pMemoryProperties_host;
    TRACE("%p, %p\n", physicalDevice, pMemoryProperties);

    convert_VkPhysicalDeviceMemoryProperties2_win_to_host(pMemoryProperties, &pMemoryProperties_host);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2(physicalDevice->phys_dev, &pMemoryProperties_host);

    convert_VkPhysicalDeviceMemoryProperties2_host_to_win(&pMemoryProperties_host, pMemoryProperties);
#else
    TRACE("%p, %p\n", physicalDevice, pMemoryProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2(physicalDevice->phys_dev, pMemoryProperties);
#endif
}

static void WINAPI wine_vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceMemoryProperties2_host pMemoryProperties_host;
    TRACE("%p, %p\n", physicalDevice, pMemoryProperties);

    convert_VkPhysicalDeviceMemoryProperties2_win_to_host(pMemoryProperties, &pMemoryProperties_host);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice->phys_dev, &pMemoryProperties_host);

    convert_VkPhysicalDeviceMemoryProperties2_host_to_win(&pMemoryProperties_host, pMemoryProperties);
#else
    TRACE("%p, %p\n", physicalDevice, pMemoryProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice->phys_dev, pMemoryProperties);
#endif
}

static void WINAPI wine_vkGetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT *pMultisampleProperties)
{
    TRACE("%p, %#x, %p\n", physicalDevice, samples, pMultisampleProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMultisamplePropertiesEXT(physicalDevice->phys_dev, samples, pMultisampleProperties);
}

VkResult WINAPI wine_vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects)
{
    TRACE("%p, 0x%s, %p, %p\n", physicalDevice, wine_dbgstr_longlong(surface), pRectCount, pRects);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDevicePresentRectanglesKHR(physicalDevice->phys_dev, surface, pRectCount, pRects);
}

void WINAPI wine_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceProperties_host pProperties_host;
    TRACE("%p, %p\n", physicalDevice, pProperties);

    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties(physicalDevice->phys_dev, &pProperties_host);

    convert_VkPhysicalDeviceProperties_host_to_win(&pProperties_host, pProperties);
#else
    TRACE("%p, %p\n", physicalDevice, pProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties(physicalDevice->phys_dev, pProperties);
#endif
}

void thunk_vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceProperties2_host pProperties_host;
    convert_VkPhysicalDeviceProperties2_win_to_host(pProperties, &pProperties_host);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2(physicalDevice->phys_dev, &pProperties_host);

    convert_VkPhysicalDeviceProperties2_host_to_win(&pProperties_host, pProperties);
#else
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2(physicalDevice->phys_dev, pProperties);
#endif
}

void thunk_vkGetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceProperties2_host pProperties_host;
    convert_VkPhysicalDeviceProperties2_win_to_host(pProperties, &pProperties_host);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2KHR(physicalDevice->phys_dev, &pProperties_host);

    convert_VkPhysicalDeviceProperties2_host_to_win(&pProperties_host, pProperties);
#else
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2KHR(physicalDevice->phys_dev, pProperties);
#endif
}

static void WINAPI wine_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(VkPhysicalDevice physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo, uint32_t *pNumPasses)
{
    TRACE("%p, %p, %p\n", physicalDevice, pPerformanceQueryCreateInfo, pNumPasses);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(physicalDevice->phys_dev, pPerformanceQueryCreateInfo, pNumPasses);
}

void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice->phys_dev, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice->phys_dev, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

static void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice->phys_dev, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties)
{
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p, %p\n", physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice->phys_dev, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    TRACE("%p, %p, %p, %p\n", physicalDevice, pFormatInfo, pPropertyCount, pProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2(physicalDevice->phys_dev, pFormatInfo, pPropertyCount, pProperties);
}

static void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    TRACE("%p, %p, %p, %p\n", physicalDevice, pFormatInfo, pPropertyCount, pProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(physicalDevice->phys_dev, pFormatInfo, pPropertyCount, pProperties);
}

static VkResult WINAPI wine_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(VkPhysicalDevice physicalDevice, uint32_t *pCombinationCount, VkFramebufferMixedSamplesCombinationNV *pCombinations)
{
    TRACE("%p, %p, %p\n", physicalDevice, pCombinationCount, pCombinations);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(physicalDevice->phys_dev, pCombinationCount, pCombinations);
}

VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR_host pSurfaceInfo_host;
    convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(pSurfaceInfo, &pSurfaceInfo_host);
    result = physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice->phys_dev, &pSurfaceInfo_host, pSurfaceCapabilities);

    return result;
#else
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice->phys_dev, pSurfaceInfo, pSurfaceCapabilities);
#endif
}

VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->phys_dev, surface, pSurfaceCapabilities);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR_host pSurfaceInfo_host;
    TRACE("%p, %p, %p, %p\n", physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);

    convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(pSurfaceInfo, &pSurfaceInfo_host);
    result = physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice->phys_dev, &pSurfaceInfo_host, pSurfaceFormatCount, pSurfaceFormats);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice->phys_dev, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
#endif
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats)
{
    TRACE("%p, 0x%s, %p, %p\n", physicalDevice, wine_dbgstr_longlong(surface), pSurfaceFormatCount, pSurfaceFormats);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->phys_dev, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes)
{
    TRACE("%p, 0x%s, %p, %p\n", physicalDevice, wine_dbgstr_longlong(surface), pPresentModeCount, pPresentModes);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->phys_dev, surface, pPresentModeCount, pPresentModes);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported)
{
    TRACE("%p, %u, 0x%s, %p\n", physicalDevice, queueFamilyIndex, wine_dbgstr_longlong(surface), pSupported);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->phys_dev, queueFamilyIndex, surface, pSupported);
}

static VkResult WINAPI wine_vkGetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolPropertiesEXT *pToolProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pToolCount, pToolProperties);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceToolPropertiesEXT(physicalDevice->phys_dev, pToolCount, pToolProperties);
}

VkBool32 WINAPI wine_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
{
    TRACE("%p, %u\n", physicalDevice, queueFamilyIndex);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice->phys_dev, queueFamilyIndex);
}

VkResult WINAPI wine_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData)
{
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), pDataSize, pData);
    return device->funcs.p_vkGetPipelineCacheData(device->device, pipelineCache, pDataSize, pData);
}

static VkResult WINAPI wine_vkGetPipelineExecutableInternalRepresentationsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPipelineExecutableInfoKHR_host pExecutableInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations);

    convert_VkPipelineExecutableInfoKHR_win_to_host(pExecutableInfo, &pExecutableInfo_host);
    result = device->funcs.p_vkGetPipelineExecutableInternalRepresentationsKHR(device->device, &pExecutableInfo_host, pInternalRepresentationCount, pInternalRepresentations);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations);
    return device->funcs.p_vkGetPipelineExecutableInternalRepresentationsKHR(device->device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations);
#endif
}

static VkResult WINAPI wine_vkGetPipelineExecutablePropertiesKHR(VkDevice device, const VkPipelineInfoKHR *pPipelineInfo, uint32_t *pExecutableCount, VkPipelineExecutablePropertiesKHR *pProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPipelineInfoKHR_host pPipelineInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pPipelineInfo, pExecutableCount, pProperties);

    convert_VkPipelineInfoKHR_win_to_host(pPipelineInfo, &pPipelineInfo_host);
    result = device->funcs.p_vkGetPipelineExecutablePropertiesKHR(device->device, &pPipelineInfo_host, pExecutableCount, pProperties);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pPipelineInfo, pExecutableCount, pProperties);
    return device->funcs.p_vkGetPipelineExecutablePropertiesKHR(device->device, pPipelineInfo, pExecutableCount, pProperties);
#endif
}

static VkResult WINAPI wine_vkGetPipelineExecutableStatisticsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pStatisticCount, VkPipelineExecutableStatisticKHR *pStatistics)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPipelineExecutableInfoKHR_host pExecutableInfo_host;
    TRACE("%p, %p, %p, %p\n", device, pExecutableInfo, pStatisticCount, pStatistics);

    convert_VkPipelineExecutableInfoKHR_win_to_host(pExecutableInfo, &pExecutableInfo_host);
    result = device->funcs.p_vkGetPipelineExecutableStatisticsKHR(device->device, &pExecutableInfo_host, pStatisticCount, pStatistics);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", device, pExecutableInfo, pStatisticCount, pStatistics);
    return device->funcs.p_vkGetPipelineExecutableStatisticsKHR(device->device, pExecutableInfo, pStatisticCount, pStatistics);
#endif
}

VkResult WINAPI wine_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p, 0x%s, %#x\n", device, wine_dbgstr_longlong(queryPool), firstQuery, queryCount, wine_dbgstr_longlong(dataSize), pData, wine_dbgstr_longlong(stride), flags);
    return device->funcs.p_vkGetQueryPoolResults(device->device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

static void WINAPI wine_vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    TRACE("%p, %p, %p\n", queue, pCheckpointDataCount, pCheckpointData);
    queue->device->funcs.p_vkGetQueueCheckpointDataNV(queue->queue, pCheckpointDataCount, pCheckpointData);
}

static VkResult WINAPI wine_vkGetRayTracingShaderGroupHandlesNV(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", device, wine_dbgstr_longlong(pipeline), firstGroup, groupCount, wine_dbgstr_longlong(dataSize), pData);
    return device->funcs.p_vkGetRayTracingShaderGroupHandlesNV(device->device, pipeline, firstGroup, groupCount, dataSize, pData);
}

void WINAPI wine_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(renderPass), pGranularity);
    device->funcs.p_vkGetRenderAreaGranularity(device->device, renderPass, pGranularity);
}

VkResult WINAPI wine_vkGetSemaphoreCounterValue(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(semaphore), pValue);
    return device->funcs.p_vkGetSemaphoreCounterValue(device->device, semaphore, pValue);
}

static VkResult WINAPI wine_vkGetSemaphoreCounterValueKHR(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(semaphore), pValue);
    return device->funcs.p_vkGetSemaphoreCounterValueKHR(device->device, semaphore, pValue);
}

static VkResult WINAPI wine_vkGetShaderInfoAMD(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t *pInfoSize, void *pInfo)
{
    TRACE("%p, 0x%s, %#x, %#x, %p, %p\n", device, wine_dbgstr_longlong(pipeline), shaderStage, infoType, pInfoSize, pInfo);
    return device->funcs.p_vkGetShaderInfoAMD(device->device, pipeline, shaderStage, infoType, pInfoSize, pInfo);
}

VkResult WINAPI wine_vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages)
{
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(swapchain), pSwapchainImageCount, pSwapchainImages);
    return device->funcs.p_vkGetSwapchainImagesKHR(device->device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

static VkResult WINAPI wine_vkGetValidationCacheDataEXT(VkDevice device, VkValidationCacheEXT validationCache, size_t *pDataSize, void *pData)
{
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(validationCache), pDataSize, pData);
    return device->funcs.p_vkGetValidationCacheDataEXT(device->device, validationCache, pDataSize, pData);
}

static VkResult WINAPI wine_vkInitializePerformanceApiINTEL(VkDevice device, const VkInitializePerformanceApiInfoINTEL *pInitializeInfo)
{
    TRACE("%p, %p\n", device, pInitializeInfo);
    return device->funcs.p_vkInitializePerformanceApiINTEL(device->device, pInitializeInfo);
}

VkResult WINAPI wine_vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkMappedMemoryRange_host *pMemoryRanges_host;
    TRACE("%p, %u, %p\n", device, memoryRangeCount, pMemoryRanges);

    pMemoryRanges_host = convert_VkMappedMemoryRange_array_win_to_host(pMemoryRanges, memoryRangeCount);
    result = device->funcs.p_vkInvalidateMappedMemoryRanges(device->device, memoryRangeCount, pMemoryRanges_host);

    free_VkMappedMemoryRange_array(pMemoryRanges_host, memoryRangeCount);
    return result;
#else
    TRACE("%p, %u, %p\n", device, memoryRangeCount, pMemoryRanges);
    return device->funcs.p_vkInvalidateMappedMemoryRanges(device->device, memoryRangeCount, pMemoryRanges);
#endif
}

VkResult WINAPI wine_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, %#x, %p\n", device, wine_dbgstr_longlong(memory), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(size), flags, ppData);
    return device->funcs.p_vkMapMemory(device->device, memory, offset, size, flags, ppData);
}

VkResult WINAPI wine_vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
    TRACE("%p, 0x%s, %u, %p\n", device, wine_dbgstr_longlong(dstCache), srcCacheCount, pSrcCaches);
    return device->funcs.p_vkMergePipelineCaches(device->device, dstCache, srcCacheCount, pSrcCaches);
}

static VkResult WINAPI wine_vkMergeValidationCachesEXT(VkDevice device, VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT *pSrcCaches)
{
    TRACE("%p, 0x%s, %u, %p\n", device, wine_dbgstr_longlong(dstCache), srcCacheCount, pSrcCaches);
    return device->funcs.p_vkMergeValidationCachesEXT(device->device, dstCache, srcCacheCount, pSrcCaches);
}

static void WINAPI wine_vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    TRACE("%p, %p\n", queue, pLabelInfo);
    queue->device->funcs.p_vkQueueBeginDebugUtilsLabelEXT(queue->queue, pLabelInfo);
}

VkResult WINAPI wine_vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindSparseInfo_host *pBindInfo_host;
    TRACE("%p, %u, %p, 0x%s\n", queue, bindInfoCount, pBindInfo, wine_dbgstr_longlong(fence));

    pBindInfo_host = convert_VkBindSparseInfo_array_win_to_host(pBindInfo, bindInfoCount);
    result = queue->device->funcs.p_vkQueueBindSparse(queue->queue, bindInfoCount, pBindInfo_host, fence);

    free_VkBindSparseInfo_array(pBindInfo_host, bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p, 0x%s\n", queue, bindInfoCount, pBindInfo, wine_dbgstr_longlong(fence));
    return queue->device->funcs.p_vkQueueBindSparse(queue->queue, bindInfoCount, pBindInfo, fence);
#endif
}

static void WINAPI wine_vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
{
    TRACE("%p\n", queue);
    queue->device->funcs.p_vkQueueEndDebugUtilsLabelEXT(queue->queue);
}

static void WINAPI wine_vkQueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    TRACE("%p, %p\n", queue, pLabelInfo);
    queue->device->funcs.p_vkQueueInsertDebugUtilsLabelEXT(queue->queue, pLabelInfo);
}

VkResult WINAPI wine_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    TRACE("%p, %p\n", queue, pPresentInfo);
    return queue->device->funcs.p_vkQueuePresentKHR(queue->queue, pPresentInfo);
}

static VkResult WINAPI wine_vkQueueSetPerformanceConfigurationINTEL(VkQueue queue, VkPerformanceConfigurationINTEL configuration)
{
    TRACE("%p, 0x%s\n", queue, wine_dbgstr_longlong(configuration));
    return queue->device->funcs.p_vkQueueSetPerformanceConfigurationINTEL(queue->queue, configuration);
}

VkResult WINAPI wine_vkQueueWaitIdle(VkQueue queue)
{
    TRACE("%p\n", queue);
    return queue->device->funcs.p_vkQueueWaitIdle(queue->queue);
}

static VkResult WINAPI wine_vkReleasePerformanceConfigurationINTEL(VkDevice device, VkPerformanceConfigurationINTEL configuration)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(configuration));
    return device->funcs.p_vkReleasePerformanceConfigurationINTEL(device->device, configuration);
}

static void WINAPI wine_vkReleaseProfilingLockKHR(VkDevice device)
{
    TRACE("%p\n", device);
    device->funcs.p_vkReleaseProfilingLockKHR(device->device);
}

VkResult WINAPI wine_vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    TRACE("%p, %#x\n", commandBuffer, flags);
    return commandBuffer->device->funcs.p_vkResetCommandBuffer(commandBuffer->command_buffer, flags);
}

VkResult WINAPI wine_vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    TRACE("%p, 0x%s, %#x\n", device, wine_dbgstr_longlong(commandPool), flags);
    return device->funcs.p_vkResetCommandPool(device->device, wine_cmd_pool_from_handle(commandPool)->command_pool, flags);
}

VkResult WINAPI wine_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    TRACE("%p, 0x%s, %#x\n", device, wine_dbgstr_longlong(descriptorPool), flags);
    return device->funcs.p_vkResetDescriptorPool(device->device, descriptorPool, flags);
}

VkResult WINAPI wine_vkResetEvent(VkDevice device, VkEvent event)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(event));
    return device->funcs.p_vkResetEvent(device->device, event);
}

VkResult WINAPI wine_vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
{
    TRACE("%p, %u, %p\n", device, fenceCount, pFences);
    return device->funcs.p_vkResetFences(device->device, fenceCount, pFences);
}

void WINAPI wine_vkResetQueryPool(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    TRACE("%p, 0x%s, %u, %u\n", device, wine_dbgstr_longlong(queryPool), firstQuery, queryCount);
    device->funcs.p_vkResetQueryPool(device->device, queryPool, firstQuery, queryCount);
}

static void WINAPI wine_vkResetQueryPoolEXT(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    TRACE("%p, 0x%s, %u, %u\n", device, wine_dbgstr_longlong(queryPool), firstQuery, queryCount);
    device->funcs.p_vkResetQueryPoolEXT(device->device, queryPool, firstQuery, queryCount);
}

VkResult thunk_vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugUtilsObjectNameInfoEXT_host pNameInfo_host;
    convert_VkDebugUtilsObjectNameInfoEXT_win_to_host(pNameInfo, &pNameInfo_host);
    result = device->funcs.p_vkSetDebugUtilsObjectNameEXT(device->device, &pNameInfo_host);

    return result;
#else
    return device->funcs.p_vkSetDebugUtilsObjectNameEXT(device->device, pNameInfo);
#endif
}

VkResult thunk_vkSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugUtilsObjectTagInfoEXT_host pTagInfo_host;
    convert_VkDebugUtilsObjectTagInfoEXT_win_to_host(pTagInfo, &pTagInfo_host);
    result = device->funcs.p_vkSetDebugUtilsObjectTagEXT(device->device, &pTagInfo_host);

    return result;
#else
    return device->funcs.p_vkSetDebugUtilsObjectTagEXT(device->device, pTagInfo);
#endif
}

VkResult WINAPI wine_vkSetEvent(VkDevice device, VkEvent event)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(event));
    return device->funcs.p_vkSetEvent(device->device, event);
}

VkResult WINAPI wine_vkSignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSemaphoreSignalInfo_host pSignalInfo_host;
    TRACE("%p, %p\n", device, pSignalInfo);

    convert_VkSemaphoreSignalInfo_win_to_host(pSignalInfo, &pSignalInfo_host);
    result = device->funcs.p_vkSignalSemaphore(device->device, &pSignalInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pSignalInfo);
    return device->funcs.p_vkSignalSemaphore(device->device, pSignalInfo);
#endif
}

static VkResult WINAPI wine_vkSignalSemaphoreKHR(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSemaphoreSignalInfo_host pSignalInfo_host;
    TRACE("%p, %p\n", device, pSignalInfo);

    convert_VkSemaphoreSignalInfo_win_to_host(pSignalInfo, &pSignalInfo_host);
    result = device->funcs.p_vkSignalSemaphoreKHR(device->device, &pSignalInfo_host);

    return result;
#else
    TRACE("%p, %p\n", device, pSignalInfo);
    return device->funcs.p_vkSignalSemaphoreKHR(device->device, pSignalInfo);
#endif
}

void thunk_vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
{
#if defined(USE_STRUCT_CONVERSION)
    VkDebugUtilsMessengerCallbackDataEXT_host pCallbackData_host;
    convert_VkDebugUtilsMessengerCallbackDataEXT_win_to_host(pCallbackData, &pCallbackData_host);
    instance->funcs.p_vkSubmitDebugUtilsMessageEXT(instance->instance, messageSeverity, messageTypes, &pCallbackData_host);

    free_VkDebugUtilsMessengerCallbackDataEXT(&pCallbackData_host);
#else
    instance->funcs.p_vkSubmitDebugUtilsMessageEXT(instance->instance, messageSeverity, messageTypes, pCallbackData);
#endif
}

void WINAPI wine_vkTrimCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    TRACE("%p, 0x%s, %#x\n", device, wine_dbgstr_longlong(commandPool), flags);
    device->funcs.p_vkTrimCommandPool(device->device, wine_cmd_pool_from_handle(commandPool)->command_pool, flags);
}

static void WINAPI wine_vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    TRACE("%p, 0x%s, %#x\n", device, wine_dbgstr_longlong(commandPool), flags);
    device->funcs.p_vkTrimCommandPoolKHR(device->device, wine_cmd_pool_from_handle(commandPool)->command_pool, flags);
}

static void WINAPI wine_vkUninitializePerformanceApiINTEL(VkDevice device)
{
    TRACE("%p\n", device);
    device->funcs.p_vkUninitializePerformanceApiINTEL(device->device);
}

void WINAPI wine_vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(memory));
    device->funcs.p_vkUnmapMemory(device->device, memory);
}

void WINAPI wine_vkUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    TRACE("%p, 0x%s, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorSet), wine_dbgstr_longlong(descriptorUpdateTemplate), pData);
    device->funcs.p_vkUpdateDescriptorSetWithTemplate(device->device, descriptorSet, descriptorUpdateTemplate, pData);
}

static void WINAPI wine_vkUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    TRACE("%p, 0x%s, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorSet), wine_dbgstr_longlong(descriptorUpdateTemplate), pData);
    device->funcs.p_vkUpdateDescriptorSetWithTemplateKHR(device->device, descriptorSet, descriptorUpdateTemplate, pData);
}

void WINAPI wine_vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies)
{
#if defined(USE_STRUCT_CONVERSION)
    VkWriteDescriptorSet_host *pDescriptorWrites_host;
    VkCopyDescriptorSet_host *pDescriptorCopies_host;
    TRACE("%p, %u, %p, %u, %p\n", device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);

    pDescriptorWrites_host = convert_VkWriteDescriptorSet_array_win_to_host(pDescriptorWrites, descriptorWriteCount);
    pDescriptorCopies_host = convert_VkCopyDescriptorSet_array_win_to_host(pDescriptorCopies, descriptorCopyCount);
    device->funcs.p_vkUpdateDescriptorSets(device->device, descriptorWriteCount, pDescriptorWrites_host, descriptorCopyCount, pDescriptorCopies_host);

    free_VkWriteDescriptorSet_array(pDescriptorWrites_host, descriptorWriteCount);
    free_VkCopyDescriptorSet_array(pDescriptorCopies_host, descriptorCopyCount);
#else
    TRACE("%p, %u, %p, %u, %p\n", device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    device->funcs.p_vkUpdateDescriptorSets(device->device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
#endif
}

VkResult WINAPI wine_vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout)
{
    TRACE("%p, %u, %p, %u, 0x%s\n", device, fenceCount, pFences, waitAll, wine_dbgstr_longlong(timeout));
    return device->funcs.p_vkWaitForFences(device->device, fenceCount, pFences, waitAll, timeout);
}

VkResult WINAPI wine_vkWaitSemaphores(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    TRACE("%p, %p, 0x%s\n", device, pWaitInfo, wine_dbgstr_longlong(timeout));
    return device->funcs.p_vkWaitSemaphores(device->device, pWaitInfo, timeout);
}

static VkResult WINAPI wine_vkWaitSemaphoresKHR(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    TRACE("%p, %p, 0x%s\n", device, pWaitInfo, wine_dbgstr_longlong(timeout));
    return device->funcs.p_vkWaitSemaphoresKHR(device->device, pWaitInfo, timeout);
}

static const struct vulkan_func vk_device_dispatch_table[] =
{
    {"vkAcquireNextImage2KHR", &wine_vkAcquireNextImage2KHR},
    {"vkAcquireNextImageKHR", &wine_vkAcquireNextImageKHR},
    {"vkAcquirePerformanceConfigurationINTEL", &wine_vkAcquirePerformanceConfigurationINTEL},
    {"vkAcquireProfilingLockKHR", &wine_vkAcquireProfilingLockKHR},
    {"vkAllocateCommandBuffers", &wine_vkAllocateCommandBuffers},
    {"vkAllocateDescriptorSets", &wine_vkAllocateDescriptorSets},
    {"vkAllocateMemory", &wine_vkAllocateMemory},
    {"vkBeginCommandBuffer", &wine_vkBeginCommandBuffer},
    {"vkBindAccelerationStructureMemoryNV", &wine_vkBindAccelerationStructureMemoryNV},
    {"vkBindBufferMemory", &wine_vkBindBufferMemory},
    {"vkBindBufferMemory2", &wine_vkBindBufferMemory2},
    {"vkBindBufferMemory2KHR", &wine_vkBindBufferMemory2KHR},
    {"vkBindImageMemory", &wine_vkBindImageMemory},
    {"vkBindImageMemory2", &wine_vkBindImageMemory2},
    {"vkBindImageMemory2KHR", &wine_vkBindImageMemory2KHR},
    {"vkCmdBeginConditionalRenderingEXT", &wine_vkCmdBeginConditionalRenderingEXT},
    {"vkCmdBeginDebugUtilsLabelEXT", &wine_vkCmdBeginDebugUtilsLabelEXT},
    {"vkCmdBeginQuery", &wine_vkCmdBeginQuery},
    {"vkCmdBeginQueryIndexedEXT", &wine_vkCmdBeginQueryIndexedEXT},
    {"vkCmdBeginRenderPass", &wine_vkCmdBeginRenderPass},
    {"vkCmdBeginRenderPass2", &wine_vkCmdBeginRenderPass2},
    {"vkCmdBeginRenderPass2KHR", &wine_vkCmdBeginRenderPass2KHR},
    {"vkCmdBeginTransformFeedbackEXT", &wine_vkCmdBeginTransformFeedbackEXT},
    {"vkCmdBindDescriptorSets", &wine_vkCmdBindDescriptorSets},
    {"vkCmdBindIndexBuffer", &wine_vkCmdBindIndexBuffer},
    {"vkCmdBindPipeline", &wine_vkCmdBindPipeline},
    {"vkCmdBindPipelineShaderGroupNV", &wine_vkCmdBindPipelineShaderGroupNV},
    {"vkCmdBindShadingRateImageNV", &wine_vkCmdBindShadingRateImageNV},
    {"vkCmdBindTransformFeedbackBuffersEXT", &wine_vkCmdBindTransformFeedbackBuffersEXT},
    {"vkCmdBindVertexBuffers", &wine_vkCmdBindVertexBuffers},
    {"vkCmdBindVertexBuffers2EXT", &wine_vkCmdBindVertexBuffers2EXT},
    {"vkCmdBlitImage", &wine_vkCmdBlitImage},
    {"vkCmdBlitImage2KHR", &wine_vkCmdBlitImage2KHR},
    {"vkCmdBuildAccelerationStructureNV", &wine_vkCmdBuildAccelerationStructureNV},
    {"vkCmdClearAttachments", &wine_vkCmdClearAttachments},
    {"vkCmdClearColorImage", &wine_vkCmdClearColorImage},
    {"vkCmdClearDepthStencilImage", &wine_vkCmdClearDepthStencilImage},
    {"vkCmdCopyAccelerationStructureNV", &wine_vkCmdCopyAccelerationStructureNV},
    {"vkCmdCopyBuffer", &wine_vkCmdCopyBuffer},
    {"vkCmdCopyBuffer2KHR", &wine_vkCmdCopyBuffer2KHR},
    {"vkCmdCopyBufferToImage", &wine_vkCmdCopyBufferToImage},
    {"vkCmdCopyBufferToImage2KHR", &wine_vkCmdCopyBufferToImage2KHR},
    {"vkCmdCopyImage", &wine_vkCmdCopyImage},
    {"vkCmdCopyImage2KHR", &wine_vkCmdCopyImage2KHR},
    {"vkCmdCopyImageToBuffer", &wine_vkCmdCopyImageToBuffer},
    {"vkCmdCopyImageToBuffer2KHR", &wine_vkCmdCopyImageToBuffer2KHR},
    {"vkCmdCopyQueryPoolResults", &wine_vkCmdCopyQueryPoolResults},
    {"vkCmdDebugMarkerBeginEXT", &wine_vkCmdDebugMarkerBeginEXT},
    {"vkCmdDebugMarkerEndEXT", &wine_vkCmdDebugMarkerEndEXT},
    {"vkCmdDebugMarkerInsertEXT", &wine_vkCmdDebugMarkerInsertEXT},
    {"vkCmdDispatch", &wine_vkCmdDispatch},
    {"vkCmdDispatchBase", &wine_vkCmdDispatchBase},
    {"vkCmdDispatchBaseKHR", &wine_vkCmdDispatchBaseKHR},
    {"vkCmdDispatchIndirect", &wine_vkCmdDispatchIndirect},
    {"vkCmdDraw", &wine_vkCmdDraw},
    {"vkCmdDrawIndexed", &wine_vkCmdDrawIndexed},
    {"vkCmdDrawIndexedIndirect", &wine_vkCmdDrawIndexedIndirect},
    {"vkCmdDrawIndexedIndirectCount", &wine_vkCmdDrawIndexedIndirectCount},
    {"vkCmdDrawIndexedIndirectCountAMD", &wine_vkCmdDrawIndexedIndirectCountAMD},
    {"vkCmdDrawIndexedIndirectCountKHR", &wine_vkCmdDrawIndexedIndirectCountKHR},
    {"vkCmdDrawIndirect", &wine_vkCmdDrawIndirect},
    {"vkCmdDrawIndirectByteCountEXT", &wine_vkCmdDrawIndirectByteCountEXT},
    {"vkCmdDrawIndirectCount", &wine_vkCmdDrawIndirectCount},
    {"vkCmdDrawIndirectCountAMD", &wine_vkCmdDrawIndirectCountAMD},
    {"vkCmdDrawIndirectCountKHR", &wine_vkCmdDrawIndirectCountKHR},
    {"vkCmdDrawMeshTasksIndirectCountNV", &wine_vkCmdDrawMeshTasksIndirectCountNV},
    {"vkCmdDrawMeshTasksIndirectNV", &wine_vkCmdDrawMeshTasksIndirectNV},
    {"vkCmdDrawMeshTasksNV", &wine_vkCmdDrawMeshTasksNV},
    {"vkCmdEndConditionalRenderingEXT", &wine_vkCmdEndConditionalRenderingEXT},
    {"vkCmdEndDebugUtilsLabelEXT", &wine_vkCmdEndDebugUtilsLabelEXT},
    {"vkCmdEndQuery", &wine_vkCmdEndQuery},
    {"vkCmdEndQueryIndexedEXT", &wine_vkCmdEndQueryIndexedEXT},
    {"vkCmdEndRenderPass", &wine_vkCmdEndRenderPass},
    {"vkCmdEndRenderPass2", &wine_vkCmdEndRenderPass2},
    {"vkCmdEndRenderPass2KHR", &wine_vkCmdEndRenderPass2KHR},
    {"vkCmdEndTransformFeedbackEXT", &wine_vkCmdEndTransformFeedbackEXT},
    {"vkCmdExecuteCommands", &wine_vkCmdExecuteCommands},
    {"vkCmdExecuteGeneratedCommandsNV", &wine_vkCmdExecuteGeneratedCommandsNV},
    {"vkCmdFillBuffer", &wine_vkCmdFillBuffer},
    {"vkCmdInsertDebugUtilsLabelEXT", &wine_vkCmdInsertDebugUtilsLabelEXT},
    {"vkCmdNextSubpass", &wine_vkCmdNextSubpass},
    {"vkCmdNextSubpass2", &wine_vkCmdNextSubpass2},
    {"vkCmdNextSubpass2KHR", &wine_vkCmdNextSubpass2KHR},
    {"vkCmdPipelineBarrier", &wine_vkCmdPipelineBarrier},
    {"vkCmdPreprocessGeneratedCommandsNV", &wine_vkCmdPreprocessGeneratedCommandsNV},
    {"vkCmdPushConstants", &wine_vkCmdPushConstants},
    {"vkCmdPushDescriptorSetKHR", &wine_vkCmdPushDescriptorSetKHR},
    {"vkCmdPushDescriptorSetWithTemplateKHR", &wine_vkCmdPushDescriptorSetWithTemplateKHR},
    {"vkCmdResetEvent", &wine_vkCmdResetEvent},
    {"vkCmdResetQueryPool", &wine_vkCmdResetQueryPool},
    {"vkCmdResolveImage", &wine_vkCmdResolveImage},
    {"vkCmdResolveImage2KHR", &wine_vkCmdResolveImage2KHR},
    {"vkCmdSetBlendConstants", &wine_vkCmdSetBlendConstants},
    {"vkCmdSetCheckpointNV", &wine_vkCmdSetCheckpointNV},
    {"vkCmdSetCoarseSampleOrderNV", &wine_vkCmdSetCoarseSampleOrderNV},
    {"vkCmdSetCullModeEXT", &wine_vkCmdSetCullModeEXT},
    {"vkCmdSetDepthBias", &wine_vkCmdSetDepthBias},
    {"vkCmdSetDepthBounds", &wine_vkCmdSetDepthBounds},
    {"vkCmdSetDepthBoundsTestEnableEXT", &wine_vkCmdSetDepthBoundsTestEnableEXT},
    {"vkCmdSetDepthCompareOpEXT", &wine_vkCmdSetDepthCompareOpEXT},
    {"vkCmdSetDepthTestEnableEXT", &wine_vkCmdSetDepthTestEnableEXT},
    {"vkCmdSetDepthWriteEnableEXT", &wine_vkCmdSetDepthWriteEnableEXT},
    {"vkCmdSetDeviceMask", &wine_vkCmdSetDeviceMask},
    {"vkCmdSetDeviceMaskKHR", &wine_vkCmdSetDeviceMaskKHR},
    {"vkCmdSetDiscardRectangleEXT", &wine_vkCmdSetDiscardRectangleEXT},
    {"vkCmdSetEvent", &wine_vkCmdSetEvent},
    {"vkCmdSetExclusiveScissorNV", &wine_vkCmdSetExclusiveScissorNV},
    {"vkCmdSetFragmentShadingRateEnumNV", &wine_vkCmdSetFragmentShadingRateEnumNV},
    {"vkCmdSetFragmentShadingRateKHR", &wine_vkCmdSetFragmentShadingRateKHR},
    {"vkCmdSetFrontFaceEXT", &wine_vkCmdSetFrontFaceEXT},
    {"vkCmdSetLineStippleEXT", &wine_vkCmdSetLineStippleEXT},
    {"vkCmdSetLineWidth", &wine_vkCmdSetLineWidth},
    {"vkCmdSetPerformanceMarkerINTEL", &wine_vkCmdSetPerformanceMarkerINTEL},
    {"vkCmdSetPerformanceOverrideINTEL", &wine_vkCmdSetPerformanceOverrideINTEL},
    {"vkCmdSetPerformanceStreamMarkerINTEL", &wine_vkCmdSetPerformanceStreamMarkerINTEL},
    {"vkCmdSetPrimitiveTopologyEXT", &wine_vkCmdSetPrimitiveTopologyEXT},
    {"vkCmdSetSampleLocationsEXT", &wine_vkCmdSetSampleLocationsEXT},
    {"vkCmdSetScissor", &wine_vkCmdSetScissor},
    {"vkCmdSetScissorWithCountEXT", &wine_vkCmdSetScissorWithCountEXT},
    {"vkCmdSetStencilCompareMask", &wine_vkCmdSetStencilCompareMask},
    {"vkCmdSetStencilOpEXT", &wine_vkCmdSetStencilOpEXT},
    {"vkCmdSetStencilReference", &wine_vkCmdSetStencilReference},
    {"vkCmdSetStencilTestEnableEXT", &wine_vkCmdSetStencilTestEnableEXT},
    {"vkCmdSetStencilWriteMask", &wine_vkCmdSetStencilWriteMask},
    {"vkCmdSetViewport", &wine_vkCmdSetViewport},
    {"vkCmdSetViewportShadingRatePaletteNV", &wine_vkCmdSetViewportShadingRatePaletteNV},
    {"vkCmdSetViewportWScalingNV", &wine_vkCmdSetViewportWScalingNV},
    {"vkCmdSetViewportWithCountEXT", &wine_vkCmdSetViewportWithCountEXT},
    {"vkCmdTraceRaysNV", &wine_vkCmdTraceRaysNV},
    {"vkCmdUpdateBuffer", &wine_vkCmdUpdateBuffer},
    {"vkCmdWaitEvents", &wine_vkCmdWaitEvents},
    {"vkCmdWriteAccelerationStructuresPropertiesNV", &wine_vkCmdWriteAccelerationStructuresPropertiesNV},
    {"vkCmdWriteBufferMarkerAMD", &wine_vkCmdWriteBufferMarkerAMD},
    {"vkCmdWriteTimestamp", &wine_vkCmdWriteTimestamp},
    {"vkCompileDeferredNV", &wine_vkCompileDeferredNV},
    {"vkCreateAccelerationStructureNV", &wine_vkCreateAccelerationStructureNV},
    {"vkCreateBuffer", &wine_vkCreateBuffer},
    {"vkCreateBufferView", &wine_vkCreateBufferView},
    {"vkCreateCommandPool", &wine_vkCreateCommandPool},
    {"vkCreateComputePipelines", &wine_vkCreateComputePipelines},
    {"vkCreateDescriptorPool", &wine_vkCreateDescriptorPool},
    {"vkCreateDescriptorSetLayout", &wine_vkCreateDescriptorSetLayout},
    {"vkCreateDescriptorUpdateTemplate", &wine_vkCreateDescriptorUpdateTemplate},
    {"vkCreateDescriptorUpdateTemplateKHR", &wine_vkCreateDescriptorUpdateTemplateKHR},
    {"vkCreateEvent", &wine_vkCreateEvent},
    {"vkCreateFence", &wine_vkCreateFence},
    {"vkCreateFramebuffer", &wine_vkCreateFramebuffer},
    {"vkCreateGraphicsPipelines", &wine_vkCreateGraphicsPipelines},
    {"vkCreateImage", &wine_vkCreateImage},
    {"vkCreateImageView", &wine_vkCreateImageView},
    {"vkCreateIndirectCommandsLayoutNV", &wine_vkCreateIndirectCommandsLayoutNV},
    {"vkCreatePipelineCache", &wine_vkCreatePipelineCache},
    {"vkCreatePipelineLayout", &wine_vkCreatePipelineLayout},
    {"vkCreatePrivateDataSlotEXT", &wine_vkCreatePrivateDataSlotEXT},
    {"vkCreateQueryPool", &wine_vkCreateQueryPool},
    {"vkCreateRayTracingPipelinesNV", &wine_vkCreateRayTracingPipelinesNV},
    {"vkCreateRenderPass", &wine_vkCreateRenderPass},
    {"vkCreateRenderPass2", &wine_vkCreateRenderPass2},
    {"vkCreateRenderPass2KHR", &wine_vkCreateRenderPass2KHR},
    {"vkCreateSampler", &wine_vkCreateSampler},
    {"vkCreateSamplerYcbcrConversion", &wine_vkCreateSamplerYcbcrConversion},
    {"vkCreateSamplerYcbcrConversionKHR", &wine_vkCreateSamplerYcbcrConversionKHR},
    {"vkCreateSemaphore", &wine_vkCreateSemaphore},
    {"vkCreateShaderModule", &wine_vkCreateShaderModule},
    {"vkCreateSwapchainKHR", &wine_vkCreateSwapchainKHR},
    {"vkCreateValidationCacheEXT", &wine_vkCreateValidationCacheEXT},
    {"vkDebugMarkerSetObjectNameEXT", &wine_vkDebugMarkerSetObjectNameEXT},
    {"vkDebugMarkerSetObjectTagEXT", &wine_vkDebugMarkerSetObjectTagEXT},
    {"vkDestroyAccelerationStructureNV", &wine_vkDestroyAccelerationStructureNV},
    {"vkDestroyBuffer", &wine_vkDestroyBuffer},
    {"vkDestroyBufferView", &wine_vkDestroyBufferView},
    {"vkDestroyCommandPool", &wine_vkDestroyCommandPool},
    {"vkDestroyDescriptorPool", &wine_vkDestroyDescriptorPool},
    {"vkDestroyDescriptorSetLayout", &wine_vkDestroyDescriptorSetLayout},
    {"vkDestroyDescriptorUpdateTemplate", &wine_vkDestroyDescriptorUpdateTemplate},
    {"vkDestroyDescriptorUpdateTemplateKHR", &wine_vkDestroyDescriptorUpdateTemplateKHR},
    {"vkDestroyDevice", &wine_vkDestroyDevice},
    {"vkDestroyEvent", &wine_vkDestroyEvent},
    {"vkDestroyFence", &wine_vkDestroyFence},
    {"vkDestroyFramebuffer", &wine_vkDestroyFramebuffer},
    {"vkDestroyImage", &wine_vkDestroyImage},
    {"vkDestroyImageView", &wine_vkDestroyImageView},
    {"vkDestroyIndirectCommandsLayoutNV", &wine_vkDestroyIndirectCommandsLayoutNV},
    {"vkDestroyPipeline", &wine_vkDestroyPipeline},
    {"vkDestroyPipelineCache", &wine_vkDestroyPipelineCache},
    {"vkDestroyPipelineLayout", &wine_vkDestroyPipelineLayout},
    {"vkDestroyPrivateDataSlotEXT", &wine_vkDestroyPrivateDataSlotEXT},
    {"vkDestroyQueryPool", &wine_vkDestroyQueryPool},
    {"vkDestroyRenderPass", &wine_vkDestroyRenderPass},
    {"vkDestroySampler", &wine_vkDestroySampler},
    {"vkDestroySamplerYcbcrConversion", &wine_vkDestroySamplerYcbcrConversion},
    {"vkDestroySamplerYcbcrConversionKHR", &wine_vkDestroySamplerYcbcrConversionKHR},
    {"vkDestroySemaphore", &wine_vkDestroySemaphore},
    {"vkDestroyShaderModule", &wine_vkDestroyShaderModule},
    {"vkDestroySwapchainKHR", &wine_vkDestroySwapchainKHR},
    {"vkDestroyValidationCacheEXT", &wine_vkDestroyValidationCacheEXT},
    {"vkDeviceWaitIdle", &wine_vkDeviceWaitIdle},
    {"vkEndCommandBuffer", &wine_vkEndCommandBuffer},
    {"vkFlushMappedMemoryRanges", &wine_vkFlushMappedMemoryRanges},
    {"vkFreeCommandBuffers", &wine_vkFreeCommandBuffers},
    {"vkFreeDescriptorSets", &wine_vkFreeDescriptorSets},
    {"vkFreeMemory", &wine_vkFreeMemory},
    {"vkGetAccelerationStructureHandleNV", &wine_vkGetAccelerationStructureHandleNV},
    {"vkGetAccelerationStructureMemoryRequirementsNV", &wine_vkGetAccelerationStructureMemoryRequirementsNV},
    {"vkGetBufferDeviceAddress", &wine_vkGetBufferDeviceAddress},
    {"vkGetBufferDeviceAddressEXT", &wine_vkGetBufferDeviceAddressEXT},
    {"vkGetBufferDeviceAddressKHR", &wine_vkGetBufferDeviceAddressKHR},
    {"vkGetBufferMemoryRequirements", &wine_vkGetBufferMemoryRequirements},
    {"vkGetBufferMemoryRequirements2", &wine_vkGetBufferMemoryRequirements2},
    {"vkGetBufferMemoryRequirements2KHR", &wine_vkGetBufferMemoryRequirements2KHR},
    {"vkGetBufferOpaqueCaptureAddress", &wine_vkGetBufferOpaqueCaptureAddress},
    {"vkGetBufferOpaqueCaptureAddressKHR", &wine_vkGetBufferOpaqueCaptureAddressKHR},
    {"vkGetCalibratedTimestampsEXT", &wine_vkGetCalibratedTimestampsEXT},
    {"vkGetDescriptorSetLayoutSupport", &wine_vkGetDescriptorSetLayoutSupport},
    {"vkGetDescriptorSetLayoutSupportKHR", &wine_vkGetDescriptorSetLayoutSupportKHR},
    {"vkGetDeviceGroupPeerMemoryFeatures", &wine_vkGetDeviceGroupPeerMemoryFeatures},
    {"vkGetDeviceGroupPeerMemoryFeaturesKHR", &wine_vkGetDeviceGroupPeerMemoryFeaturesKHR},
    {"vkGetDeviceGroupPresentCapabilitiesKHR", &wine_vkGetDeviceGroupPresentCapabilitiesKHR},
    {"vkGetDeviceGroupSurfacePresentModesKHR", &wine_vkGetDeviceGroupSurfacePresentModesKHR},
    {"vkGetDeviceMemoryCommitment", &wine_vkGetDeviceMemoryCommitment},
    {"vkGetDeviceMemoryOpaqueCaptureAddress", &wine_vkGetDeviceMemoryOpaqueCaptureAddress},
    {"vkGetDeviceMemoryOpaqueCaptureAddressKHR", &wine_vkGetDeviceMemoryOpaqueCaptureAddressKHR},
    {"vkGetDeviceProcAddr", &wine_vkGetDeviceProcAddr},
    {"vkGetDeviceQueue", &wine_vkGetDeviceQueue},
    {"vkGetDeviceQueue2", &wine_vkGetDeviceQueue2},
    {"vkGetEventStatus", &wine_vkGetEventStatus},
    {"vkGetFenceStatus", &wine_vkGetFenceStatus},
    {"vkGetGeneratedCommandsMemoryRequirementsNV", &wine_vkGetGeneratedCommandsMemoryRequirementsNV},
    {"vkGetImageMemoryRequirements", &wine_vkGetImageMemoryRequirements},
    {"vkGetImageMemoryRequirements2", &wine_vkGetImageMemoryRequirements2},
    {"vkGetImageMemoryRequirements2KHR", &wine_vkGetImageMemoryRequirements2KHR},
    {"vkGetImageSparseMemoryRequirements", &wine_vkGetImageSparseMemoryRequirements},
    {"vkGetImageSparseMemoryRequirements2", &wine_vkGetImageSparseMemoryRequirements2},
    {"vkGetImageSparseMemoryRequirements2KHR", &wine_vkGetImageSparseMemoryRequirements2KHR},
    {"vkGetImageSubresourceLayout", &wine_vkGetImageSubresourceLayout},
    {"vkGetMemoryHostPointerPropertiesEXT", &wine_vkGetMemoryHostPointerPropertiesEXT},
    {"vkGetPerformanceParameterINTEL", &wine_vkGetPerformanceParameterINTEL},
    {"vkGetPipelineCacheData", &wine_vkGetPipelineCacheData},
    {"vkGetPipelineExecutableInternalRepresentationsKHR", &wine_vkGetPipelineExecutableInternalRepresentationsKHR},
    {"vkGetPipelineExecutablePropertiesKHR", &wine_vkGetPipelineExecutablePropertiesKHR},
    {"vkGetPipelineExecutableStatisticsKHR", &wine_vkGetPipelineExecutableStatisticsKHR},
    {"vkGetPrivateDataEXT", &wine_vkGetPrivateDataEXT},
    {"vkGetQueryPoolResults", &wine_vkGetQueryPoolResults},
    {"vkGetQueueCheckpointDataNV", &wine_vkGetQueueCheckpointDataNV},
    {"vkGetRayTracingShaderGroupHandlesNV", &wine_vkGetRayTracingShaderGroupHandlesNV},
    {"vkGetRenderAreaGranularity", &wine_vkGetRenderAreaGranularity},
    {"vkGetSemaphoreCounterValue", &wine_vkGetSemaphoreCounterValue},
    {"vkGetSemaphoreCounterValueKHR", &wine_vkGetSemaphoreCounterValueKHR},
    {"vkGetShaderInfoAMD", &wine_vkGetShaderInfoAMD},
    {"vkGetSwapchainImagesKHR", &wine_vkGetSwapchainImagesKHR},
    {"vkGetValidationCacheDataEXT", &wine_vkGetValidationCacheDataEXT},
    {"vkInitializePerformanceApiINTEL", &wine_vkInitializePerformanceApiINTEL},
    {"vkInvalidateMappedMemoryRanges", &wine_vkInvalidateMappedMemoryRanges},
    {"vkMapMemory", &wine_vkMapMemory},
    {"vkMergePipelineCaches", &wine_vkMergePipelineCaches},
    {"vkMergeValidationCachesEXT", &wine_vkMergeValidationCachesEXT},
    {"vkQueueBeginDebugUtilsLabelEXT", &wine_vkQueueBeginDebugUtilsLabelEXT},
    {"vkQueueBindSparse", &wine_vkQueueBindSparse},
    {"vkQueueEndDebugUtilsLabelEXT", &wine_vkQueueEndDebugUtilsLabelEXT},
    {"vkQueueInsertDebugUtilsLabelEXT", &wine_vkQueueInsertDebugUtilsLabelEXT},
    {"vkQueuePresentKHR", &wine_vkQueuePresentKHR},
    {"vkQueueSetPerformanceConfigurationINTEL", &wine_vkQueueSetPerformanceConfigurationINTEL},
    {"vkQueueSubmit", &wine_vkQueueSubmit},
    {"vkQueueWaitIdle", &wine_vkQueueWaitIdle},
    {"vkReleasePerformanceConfigurationINTEL", &wine_vkReleasePerformanceConfigurationINTEL},
    {"vkReleaseProfilingLockKHR", &wine_vkReleaseProfilingLockKHR},
    {"vkResetCommandBuffer", &wine_vkResetCommandBuffer},
    {"vkResetCommandPool", &wine_vkResetCommandPool},
    {"vkResetDescriptorPool", &wine_vkResetDescriptorPool},
    {"vkResetEvent", &wine_vkResetEvent},
    {"vkResetFences", &wine_vkResetFences},
    {"vkResetQueryPool", &wine_vkResetQueryPool},
    {"vkResetQueryPoolEXT", &wine_vkResetQueryPoolEXT},
    {"vkSetDebugUtilsObjectNameEXT", &wine_vkSetDebugUtilsObjectNameEXT},
    {"vkSetDebugUtilsObjectTagEXT", &wine_vkSetDebugUtilsObjectTagEXT},
    {"vkSetEvent", &wine_vkSetEvent},
    {"vkSetPrivateDataEXT", &wine_vkSetPrivateDataEXT},
    {"vkSignalSemaphore", &wine_vkSignalSemaphore},
    {"vkSignalSemaphoreKHR", &wine_vkSignalSemaphoreKHR},
    {"vkTrimCommandPool", &wine_vkTrimCommandPool},
    {"vkTrimCommandPoolKHR", &wine_vkTrimCommandPoolKHR},
    {"vkUninitializePerformanceApiINTEL", &wine_vkUninitializePerformanceApiINTEL},
    {"vkUnmapMemory", &wine_vkUnmapMemory},
    {"vkUpdateDescriptorSetWithTemplate", &wine_vkUpdateDescriptorSetWithTemplate},
    {"vkUpdateDescriptorSetWithTemplateKHR", &wine_vkUpdateDescriptorSetWithTemplateKHR},
    {"vkUpdateDescriptorSets", &wine_vkUpdateDescriptorSets},
    {"vkWaitForFences", &wine_vkWaitForFences},
    {"vkWaitSemaphores", &wine_vkWaitSemaphores},
    {"vkWaitSemaphoresKHR", &wine_vkWaitSemaphoresKHR},
};

static const struct vulkan_func vk_instance_dispatch_table[] =
{
    {"vkCreateDebugReportCallbackEXT", &wine_vkCreateDebugReportCallbackEXT},
    {"vkCreateDebugUtilsMessengerEXT", &wine_vkCreateDebugUtilsMessengerEXT},
    {"vkCreateDevice", &wine_vkCreateDevice},
    {"vkCreateHeadlessSurfaceEXT", &wine_vkCreateHeadlessSurfaceEXT},
    {"vkCreateWin32SurfaceKHR", &wine_vkCreateWin32SurfaceKHR},
    {"vkDebugReportMessageEXT", &wine_vkDebugReportMessageEXT},
    {"vkDestroyDebugReportCallbackEXT", &wine_vkDestroyDebugReportCallbackEXT},
    {"vkDestroyDebugUtilsMessengerEXT", &wine_vkDestroyDebugUtilsMessengerEXT},
    {"vkDestroyInstance", &wine_vkDestroyInstance},
    {"vkDestroySurfaceKHR", &wine_vkDestroySurfaceKHR},
    {"vkEnumerateDeviceExtensionProperties", &wine_vkEnumerateDeviceExtensionProperties},
    {"vkEnumerateDeviceLayerProperties", &wine_vkEnumerateDeviceLayerProperties},
    {"vkEnumeratePhysicalDeviceGroups", &wine_vkEnumeratePhysicalDeviceGroups},
    {"vkEnumeratePhysicalDeviceGroupsKHR", &wine_vkEnumeratePhysicalDeviceGroupsKHR},
    {"vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR", &wine_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR},
    {"vkEnumeratePhysicalDevices", &wine_vkEnumeratePhysicalDevices},
    {"vkGetPhysicalDeviceCalibrateableTimeDomainsEXT", &wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT},
    {"vkGetPhysicalDeviceCooperativeMatrixPropertiesNV", &wine_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV},
    {"vkGetPhysicalDeviceExternalBufferProperties", &wine_vkGetPhysicalDeviceExternalBufferProperties},
    {"vkGetPhysicalDeviceExternalBufferPropertiesKHR", &wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR},
    {"vkGetPhysicalDeviceExternalFenceProperties", &wine_vkGetPhysicalDeviceExternalFenceProperties},
    {"vkGetPhysicalDeviceExternalFencePropertiesKHR", &wine_vkGetPhysicalDeviceExternalFencePropertiesKHR},
    {"vkGetPhysicalDeviceExternalSemaphoreProperties", &wine_vkGetPhysicalDeviceExternalSemaphoreProperties},
    {"vkGetPhysicalDeviceExternalSemaphorePropertiesKHR", &wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR},
    {"vkGetPhysicalDeviceFeatures", &wine_vkGetPhysicalDeviceFeatures},
    {"vkGetPhysicalDeviceFeatures2", &wine_vkGetPhysicalDeviceFeatures2},
    {"vkGetPhysicalDeviceFeatures2KHR", &wine_vkGetPhysicalDeviceFeatures2KHR},
    {"vkGetPhysicalDeviceFormatProperties", &wine_vkGetPhysicalDeviceFormatProperties},
    {"vkGetPhysicalDeviceFormatProperties2", &wine_vkGetPhysicalDeviceFormatProperties2},
    {"vkGetPhysicalDeviceFormatProperties2KHR", &wine_vkGetPhysicalDeviceFormatProperties2KHR},
    {"vkGetPhysicalDeviceFragmentShadingRatesKHR", &wine_vkGetPhysicalDeviceFragmentShadingRatesKHR},
    {"vkGetPhysicalDeviceImageFormatProperties", &wine_vkGetPhysicalDeviceImageFormatProperties},
    {"vkGetPhysicalDeviceImageFormatProperties2", &wine_vkGetPhysicalDeviceImageFormatProperties2},
    {"vkGetPhysicalDeviceImageFormatProperties2KHR", &wine_vkGetPhysicalDeviceImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceMemoryProperties", &wine_vkGetPhysicalDeviceMemoryProperties},
    {"vkGetPhysicalDeviceMemoryProperties2", &wine_vkGetPhysicalDeviceMemoryProperties2},
    {"vkGetPhysicalDeviceMemoryProperties2KHR", &wine_vkGetPhysicalDeviceMemoryProperties2KHR},
    {"vkGetPhysicalDeviceMultisamplePropertiesEXT", &wine_vkGetPhysicalDeviceMultisamplePropertiesEXT},
    {"vkGetPhysicalDevicePresentRectanglesKHR", &wine_vkGetPhysicalDevicePresentRectanglesKHR},
    {"vkGetPhysicalDeviceProperties", &wine_vkGetPhysicalDeviceProperties},
    {"vkGetPhysicalDeviceProperties2", &wine_vkGetPhysicalDeviceProperties2},
    {"vkGetPhysicalDeviceProperties2KHR", &wine_vkGetPhysicalDeviceProperties2KHR},
    {"vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR", &wine_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR},
    {"vkGetPhysicalDeviceQueueFamilyProperties", &wine_vkGetPhysicalDeviceQueueFamilyProperties},
    {"vkGetPhysicalDeviceQueueFamilyProperties2", &wine_vkGetPhysicalDeviceQueueFamilyProperties2},
    {"vkGetPhysicalDeviceQueueFamilyProperties2KHR", &wine_vkGetPhysicalDeviceQueueFamilyProperties2KHR},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", &wine_vkGetPhysicalDeviceSparseImageFormatProperties},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2", &wine_vkGetPhysicalDeviceSparseImageFormatProperties2},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2KHR", &wine_vkGetPhysicalDeviceSparseImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV", &wine_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV},
    {"vkGetPhysicalDeviceSurfaceCapabilities2KHR", &wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR},
    {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", &wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR},
    {"vkGetPhysicalDeviceSurfaceFormats2KHR", &wine_vkGetPhysicalDeviceSurfaceFormats2KHR},
    {"vkGetPhysicalDeviceSurfaceFormatsKHR", &wine_vkGetPhysicalDeviceSurfaceFormatsKHR},
    {"vkGetPhysicalDeviceSurfacePresentModesKHR", &wine_vkGetPhysicalDeviceSurfacePresentModesKHR},
    {"vkGetPhysicalDeviceSurfaceSupportKHR", &wine_vkGetPhysicalDeviceSurfaceSupportKHR},
    {"vkGetPhysicalDeviceToolPropertiesEXT", &wine_vkGetPhysicalDeviceToolPropertiesEXT},
    {"vkGetPhysicalDeviceWin32PresentationSupportKHR", &wine_vkGetPhysicalDeviceWin32PresentationSupportKHR},
    {"vkSubmitDebugUtilsMessageEXT", &wine_vkSubmitDebugUtilsMessageEXT},
};

void *wine_vk_get_device_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_device_dispatch_table); i++)
    {
        if (strcmp(vk_device_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found name=%s in device table\n", debugstr_a(name));
            return vk_device_dispatch_table[i].func;
        }
    }
    return NULL;
}

void *wine_vk_get_instance_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_instance_dispatch_table); i++)
    {
        if (strcmp(vk_instance_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found name=%s in instance table\n", debugstr_a(name));
            return vk_instance_dispatch_table[i].func;
        }
    }
    return NULL;
}

static const char * const vk_device_extensions[] =
{
    "VK_AMD_buffer_marker",
    "VK_AMD_device_coherent_memory",
    "VK_AMD_draw_indirect_count",
    "VK_AMD_gcn_shader",
    "VK_AMD_gpu_shader_half_float",
    "VK_AMD_gpu_shader_int16",
    "VK_AMD_memory_overallocation_behavior",
    "VK_AMD_mixed_attachment_samples",
    "VK_AMD_negative_viewport_height",
    "VK_AMD_pipeline_compiler_control",
    "VK_AMD_rasterization_order",
    "VK_AMD_shader_ballot",
    "VK_AMD_shader_core_properties",
    "VK_AMD_shader_core_properties2",
    "VK_AMD_shader_explicit_vertex_parameter",
    "VK_AMD_shader_fragment_mask",
    "VK_AMD_shader_image_load_store_lod",
    "VK_AMD_shader_info",
    "VK_AMD_shader_trinary_minmax",
    "VK_AMD_texture_gather_bias_lod",
    "VK_EXT_4444_formats",
    "VK_EXT_astc_decode_mode",
    "VK_EXT_blend_operation_advanced",
    "VK_EXT_buffer_device_address",
    "VK_EXT_calibrated_timestamps",
    "VK_EXT_conditional_rendering",
    "VK_EXT_conservative_rasterization",
    "VK_EXT_custom_border_color",
    "VK_EXT_debug_marker",
    "VK_EXT_depth_clip_enable",
    "VK_EXT_depth_range_unrestricted",
    "VK_EXT_descriptor_indexing",
    "VK_EXT_discard_rectangles",
    "VK_EXT_extended_dynamic_state",
    "VK_EXT_external_memory_host",
    "VK_EXT_filter_cubic",
    "VK_EXT_fragment_density_map",
    "VK_EXT_fragment_density_map2",
    "VK_EXT_fragment_shader_interlock",
    "VK_EXT_global_priority",
    "VK_EXT_host_query_reset",
    "VK_EXT_image_robustness",
    "VK_EXT_index_type_uint8",
    "VK_EXT_inline_uniform_block",
    "VK_EXT_line_rasterization",
    "VK_EXT_memory_budget",
    "VK_EXT_memory_priority",
    "VK_EXT_pci_bus_info",
    "VK_EXT_pipeline_creation_cache_control",
    "VK_EXT_post_depth_coverage",
    "VK_EXT_private_data",
    "VK_EXT_queue_family_foreign",
    "VK_EXT_robustness2",
    "VK_EXT_sample_locations",
    "VK_EXT_sampler_filter_minmax",
    "VK_EXT_scalar_block_layout",
    "VK_EXT_separate_stencil_usage",
    "VK_EXT_shader_atomic_float",
    "VK_EXT_shader_demote_to_helper_invocation",
    "VK_EXT_shader_image_atomic_int64",
    "VK_EXT_shader_stencil_export",
    "VK_EXT_shader_subgroup_ballot",
    "VK_EXT_shader_subgroup_vote",
    "VK_EXT_shader_viewport_index_layer",
    "VK_EXT_subgroup_size_control",
    "VK_EXT_texel_buffer_alignment",
    "VK_EXT_texture_compression_astc_hdr",
    "VK_EXT_tooling_info",
    "VK_EXT_transform_feedback",
    "VK_EXT_validation_cache",
    "VK_EXT_vertex_attribute_divisor",
    "VK_EXT_ycbcr_image_arrays",
    "VK_GOOGLE_decorate_string",
    "VK_GOOGLE_hlsl_functionality1",
    "VK_GOOGLE_user_type",
    "VK_IMG_filter_cubic",
    "VK_IMG_format_pvrtc",
    "VK_INTEL_performance_query",
    "VK_INTEL_shader_integer_functions2",
    "VK_KHR_16bit_storage",
    "VK_KHR_8bit_storage",
    "VK_KHR_bind_memory2",
    "VK_KHR_buffer_device_address",
    "VK_KHR_copy_commands2",
    "VK_KHR_create_renderpass2",
    "VK_KHR_dedicated_allocation",
    "VK_KHR_depth_stencil_resolve",
    "VK_KHR_descriptor_update_template",
    "VK_KHR_device_group",
    "VK_KHR_draw_indirect_count",
    "VK_KHR_driver_properties",
    "VK_KHR_external_fence",
    "VK_KHR_external_memory",
    "VK_KHR_external_semaphore",
    "VK_KHR_fragment_shading_rate",
    "VK_KHR_get_memory_requirements2",
    "VK_KHR_image_format_list",
    "VK_KHR_imageless_framebuffer",
    "VK_KHR_incremental_present",
    "VK_KHR_maintenance1",
    "VK_KHR_maintenance2",
    "VK_KHR_maintenance3",
    "VK_KHR_multiview",
    "VK_KHR_performance_query",
    "VK_KHR_pipeline_executable_properties",
    "VK_KHR_push_descriptor",
    "VK_KHR_relaxed_block_layout",
    "VK_KHR_sampler_mirror_clamp_to_edge",
    "VK_KHR_sampler_ycbcr_conversion",
    "VK_KHR_separate_depth_stencil_layouts",
    "VK_KHR_shader_atomic_int64",
    "VK_KHR_shader_clock",
    "VK_KHR_shader_draw_parameters",
    "VK_KHR_shader_float16_int8",
    "VK_KHR_shader_float_controls",
    "VK_KHR_shader_non_semantic_info",
    "VK_KHR_shader_subgroup_extended_types",
    "VK_KHR_shader_terminate_invocation",
    "VK_KHR_spirv_1_4",
    "VK_KHR_storage_buffer_storage_class",
    "VK_KHR_swapchain",
    "VK_KHR_swapchain_mutable_format",
    "VK_KHR_timeline_semaphore",
    "VK_KHR_uniform_buffer_standard_layout",
    "VK_KHR_variable_pointers",
    "VK_KHR_vulkan_memory_model",
    "VK_NV_clip_space_w_scaling",
    "VK_NV_compute_shader_derivatives",
    "VK_NV_cooperative_matrix",
    "VK_NV_corner_sampled_image",
    "VK_NV_coverage_reduction_mode",
    "VK_NV_dedicated_allocation",
    "VK_NV_dedicated_allocation_image_aliasing",
    "VK_NV_device_diagnostic_checkpoints",
    "VK_NV_device_diagnostics_config",
    "VK_NV_device_generated_commands",
    "VK_NV_fill_rectangle",
    "VK_NV_fragment_coverage_to_color",
    "VK_NV_fragment_shader_barycentric",
    "VK_NV_fragment_shading_rate_enums",
    "VK_NV_framebuffer_mixed_samples",
    "VK_NV_geometry_shader_passthrough",
    "VK_NV_glsl_shader",
    "VK_NV_mesh_shader",
    "VK_NV_ray_tracing",
    "VK_NV_representative_fragment_test",
    "VK_NV_sample_mask_override_coverage",
    "VK_NV_scissor_exclusive",
    "VK_NV_shader_image_footprint",
    "VK_NV_shader_sm_builtins",
    "VK_NV_shader_subgroup_partitioned",
    "VK_NV_shading_rate_image",
    "VK_NV_viewport_array2",
    "VK_NV_viewport_swizzle",
    "VK_QCOM_render_pass_shader_resolve",
    "VK_QCOM_render_pass_store_ops",
    "VK_QCOM_render_pass_transform",
    "VK_QCOM_rotated_copy_commands",
};

static const char * const vk_instance_extensions[] =
{
    "VK_EXT_debug_report",
    "VK_EXT_debug_utils",
    "VK_EXT_headless_surface",
    "VK_EXT_swapchain_colorspace",
    "VK_EXT_validation_features",
    "VK_EXT_validation_flags",
    "VK_KHR_device_group_creation",
    "VK_KHR_external_fence_capabilities",
    "VK_KHR_external_memory_capabilities",
    "VK_KHR_external_semaphore_capabilities",
    "VK_KHR_get_physical_device_properties2",
    "VK_KHR_get_surface_capabilities2",
    "VK_KHR_surface",
    "VK_KHR_win32_surface",
};

BOOL wine_vk_device_extension_supported(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_device_extensions); i++)
    {
        if (strcmp(vk_device_extensions[i], name) == 0)
            return TRUE;
    }
    return FALSE;
}

BOOL wine_vk_instance_extension_supported(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_instance_extensions); i++)
    {
        if (strcmp(vk_instance_extensions[i], name) == 0)
            return TRUE;
    }
    return FALSE;
}

BOOL wine_vk_is_type_wrapped(VkObjectType type)
{
    return FALSE ||
        type == VK_OBJECT_TYPE_COMMAND_BUFFER ||
        type == VK_OBJECT_TYPE_COMMAND_POOL ||
        type == VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT ||
        type == VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT ||
        type == VK_OBJECT_TYPE_DEVICE ||
        type == VK_OBJECT_TYPE_INSTANCE ||
        type == VK_OBJECT_TYPE_PHYSICAL_DEVICE ||
        type == VK_OBJECT_TYPE_QUEUE;
}

uint64_t wine_vk_unwrap_handle(VkObjectType type, uint64_t handle)
{
    switch(type)
    {
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
        return (uint64_t) (uintptr_t) ((VkCommandBuffer) (uintptr_t) handle)->command_buffer;
    case VK_OBJECT_TYPE_COMMAND_POOL:
        return (uint64_t) wine_cmd_pool_from_handle(handle)->command_pool;
    case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
        return (uint64_t) wine_debug_report_callback_from_handle(handle)->debug_callback;
    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
        return (uint64_t) wine_debug_utils_messenger_from_handle(handle)->debug_messenger;
    case VK_OBJECT_TYPE_DEVICE:
        return (uint64_t) (uintptr_t) ((VkDevice) (uintptr_t) handle)->device;
    case VK_OBJECT_TYPE_INSTANCE:
        return (uint64_t) (uintptr_t) ((VkInstance) (uintptr_t) handle)->instance;
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
        return (uint64_t) (uintptr_t) ((VkPhysicalDevice) (uintptr_t) handle)->phys_dev;
    case VK_OBJECT_TYPE_QUEUE:
        return (uint64_t) (uintptr_t) ((VkQueue) (uintptr_t) handle)->queue;
    default:
       return handle;
    }
}
