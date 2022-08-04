/* Automatically generated from Vulkan vk.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2015-2022 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdlib.h>

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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAcquireProfilingLockInfoKHR_win_to_host(const VkAcquireProfilingLockInfoKHR *in, VkAcquireProfilingLockInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->timeout = in->timeout;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDescriptorSetAllocateInfo_win_to_host(const VkDescriptorSetAllocateInfo *in, VkDescriptorSetAllocateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->descriptorPool = in->descriptorPool;
    out->descriptorSetCount = in->descriptorSetCount;
    out->pSetLayouts = in->pSetLayouts;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryAllocateInfo_win_to_host(const VkMemoryAllocateInfo *in, VkMemoryAllocateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->allocationSize = in->allocationSize;
    out->memoryTypeIndex = in->memoryTypeIndex;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkCommandBufferInheritanceInfo_host *convert_VkCommandBufferInheritanceInfo_array_win_to_host(const VkCommandBufferInheritanceInfo *in, uint32_t count)
{
    VkCommandBufferInheritanceInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkCommandBufferInheritanceInfo_array(VkCommandBufferInheritanceInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCommandBufferBeginInfo_win_to_host(const VkCommandBufferBeginInfo *in, VkCommandBufferBeginInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->pInheritanceInfo = convert_VkCommandBufferInheritanceInfo_array_win_to_host(in->pInheritanceInfo, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkCommandBufferBeginInfo(VkCommandBufferBeginInfo_host *in)
{
    free_VkCommandBufferInheritanceInfo_array((VkCommandBufferInheritanceInfo_host *)in->pInheritanceInfo, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBindAccelerationStructureMemoryInfoNV_host *convert_VkBindAccelerationStructureMemoryInfoNV_array_win_to_host(const VkBindAccelerationStructureMemoryInfoNV *in, uint32_t count)
{
    VkBindAccelerationStructureMemoryInfoNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBindAccelerationStructureMemoryInfoNV_array(VkBindAccelerationStructureMemoryInfoNV_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBindBufferMemoryInfo_host *convert_VkBindBufferMemoryInfo_array_win_to_host(const VkBindBufferMemoryInfo *in, uint32_t count)
{
    VkBindBufferMemoryInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBindBufferMemoryInfo_array(VkBindBufferMemoryInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBindImageMemoryInfo_host *convert_VkBindImageMemoryInfo_array_win_to_host(const VkBindImageMemoryInfo *in, uint32_t count)
{
    VkBindImageMemoryInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBindImageMemoryInfo_array(VkBindImageMemoryInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkAccelerationStructureBuildGeometryInfoKHR_host *convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win_to_host(const VkAccelerationStructureBuildGeometryInfoKHR *in, uint32_t count)
{
    VkAccelerationStructureBuildGeometryInfoKHR_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].type = in[i].type;
        out[i].flags = in[i].flags;
        out[i].mode = in[i].mode;
        out[i].srcAccelerationStructure = in[i].srcAccelerationStructure;
        out[i].dstAccelerationStructure = in[i].dstAccelerationStructure;
        out[i].geometryCount = in[i].geometryCount;
        out[i].pGeometries = in[i].pGeometries;
        out[i].ppGeometries = in[i].ppGeometries;
        out[i].scratchData = in[i].scratchData;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkAccelerationStructureBuildGeometryInfoKHR_array(VkAccelerationStructureBuildGeometryInfoKHR_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkConditionalRenderingBeginInfoEXT_win_to_host(const VkConditionalRenderingBeginInfoEXT *in, VkConditionalRenderingBeginInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
    out->offset = in->offset;
    out->flags = in->flags;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkRenderingAttachmentInfo_host *convert_VkRenderingAttachmentInfo_array_win_to_host(const VkRenderingAttachmentInfo *in, uint32_t count)
{
    VkRenderingAttachmentInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].imageView = in[i].imageView;
        out[i].imageLayout = in[i].imageLayout;
        out[i].resolveMode = in[i].resolveMode;
        out[i].resolveImageView = in[i].resolveImageView;
        out[i].resolveImageLayout = in[i].resolveImageLayout;
        out[i].loadOp = in[i].loadOp;
        out[i].storeOp = in[i].storeOp;
        out[i].clearValue = in[i].clearValue;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkRenderingAttachmentInfo_array(VkRenderingAttachmentInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkRenderingInfo_win_to_host(const VkRenderingInfo *in, VkRenderingInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->renderArea = in->renderArea;
    out->layerCount = in->layerCount;
    out->viewMask = in->viewMask;
    out->colorAttachmentCount = in->colorAttachmentCount;
    out->pColorAttachments = convert_VkRenderingAttachmentInfo_array_win_to_host(in->pColorAttachments, in->colorAttachmentCount);
    out->pDepthAttachment = convert_VkRenderingAttachmentInfo_array_win_to_host(in->pDepthAttachment, 1);
    out->pStencilAttachment = convert_VkRenderingAttachmentInfo_array_win_to_host(in->pStencilAttachment, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkRenderingInfo(VkRenderingInfo_host *in)
{
    free_VkRenderingAttachmentInfo_array((VkRenderingAttachmentInfo_host *)in->pColorAttachments, in->colorAttachmentCount);
    free_VkRenderingAttachmentInfo_array((VkRenderingAttachmentInfo_host *)in->pDepthAttachment, 1);
    free_VkRenderingAttachmentInfo_array((VkRenderingAttachmentInfo_host *)in->pStencilAttachment, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkBlitImageInfo2_win_to_host(const VkBlitImageInfo2 *in, VkBlitImageInfo2_host *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkGeometryDataNV_win_to_host(const VkGeometryDataNV *in, VkGeometryDataNV_host *out)
{
    if (!in) return;

    convert_VkGeometryTrianglesNV_win_to_host(&in->triangles, &out->triangles);
    convert_VkGeometryAABBNV_win_to_host(&in->aabbs, &out->aabbs);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkGeometryNV_host *convert_VkGeometryNV_array_win_to_host(const VkGeometryNV *in, uint32_t count)
{
    VkGeometryNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkGeometryNV_array(VkGeometryNV_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkAccelerationStructureInfoNV(VkAccelerationStructureInfoNV_host *in)
{
    free_VkGeometryNV_array((VkGeometryNV_host *)in->pGeometries, in->geometryCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyAccelerationStructureInfoKHR_win_to_host(const VkCopyAccelerationStructureInfoKHR *in, VkCopyAccelerationStructureInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->src = in->src;
    out->dst = in->dst;
    out->mode = in->mode;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyAccelerationStructureToMemoryInfoKHR_win_to_host(const VkCopyAccelerationStructureToMemoryInfoKHR *in, VkCopyAccelerationStructureToMemoryInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->src = in->src;
    out->dst = in->dst;
    out->mode = in->mode;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferCopy_host *convert_VkBufferCopy_array_win_to_host(const VkBufferCopy *in, uint32_t count)
{
    VkBufferCopy_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].srcOffset = in[i].srcOffset;
        out[i].dstOffset = in[i].dstOffset;
        out[i].size = in[i].size;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBufferCopy_array(VkBufferCopy_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferCopy2_host *convert_VkBufferCopy2_array_win_to_host(const VkBufferCopy2 *in, uint32_t count)
{
    VkBufferCopy2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBufferCopy2_array(VkBufferCopy2_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyBufferInfo2_win_to_host(const VkCopyBufferInfo2 *in, VkCopyBufferInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcBuffer = in->srcBuffer;
    out->dstBuffer = in->dstBuffer;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferCopy2_array_win_to_host(in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkCopyBufferInfo2(VkCopyBufferInfo2_host *in)
{
    free_VkBufferCopy2_array((VkBufferCopy2_host *)in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferImageCopy_host *convert_VkBufferImageCopy_array_win_to_host(const VkBufferImageCopy *in, uint32_t count)
{
    VkBufferImageCopy_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBufferImageCopy_array(VkBufferImageCopy_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferImageCopy2_host *convert_VkBufferImageCopy2_array_win_to_host(const VkBufferImageCopy2 *in, uint32_t count)
{
    VkBufferImageCopy2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBufferImageCopy2_array(VkBufferImageCopy2_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyBufferToImageInfo2_win_to_host(const VkCopyBufferToImageInfo2 *in, VkCopyBufferToImageInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcBuffer = in->srcBuffer;
    out->dstImage = in->dstImage;
    out->dstImageLayout = in->dstImageLayout;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferImageCopy2_array_win_to_host(in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkCopyBufferToImageInfo2(VkCopyBufferToImageInfo2_host *in)
{
    free_VkBufferImageCopy2_array((VkBufferImageCopy2_host *)in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyImageInfo2_win_to_host(const VkCopyImageInfo2 *in, VkCopyImageInfo2_host *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyImageToBufferInfo2_win_to_host(const VkCopyImageToBufferInfo2 *in, VkCopyImageToBufferInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcImage = in->srcImage;
    out->srcImageLayout = in->srcImageLayout;
    out->dstBuffer = in->dstBuffer;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferImageCopy2_array_win_to_host(in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkCopyImageToBufferInfo2(VkCopyImageToBufferInfo2_host *in)
{
    free_VkBufferImageCopy2_array((VkBufferImageCopy2_host *)in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyMemoryToAccelerationStructureInfoKHR_win_to_host(const VkCopyMemoryToAccelerationStructureInfoKHR *in, VkCopyMemoryToAccelerationStructureInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->src = in->src;
    out->dst = in->dst;
    out->mode = in->mode;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCuLaunchInfoNVX_win_to_host(const VkCuLaunchInfoNVX *in, VkCuLaunchInfoNVX_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->function = in->function;
    out->gridDimX = in->gridDimX;
    out->gridDimY = in->gridDimY;
    out->gridDimZ = in->gridDimZ;
    out->blockDimX = in->blockDimX;
    out->blockDimY = in->blockDimY;
    out->blockDimZ = in->blockDimZ;
    out->sharedMemBytes = in->sharedMemBytes;
    out->paramCount = in->paramCount;
    out->pParams = in->pParams;
    out->extraCount = in->extraCount;
    out->pExtras = in->pExtras;
}
#endif /* USE_STRUCT_CONVERSION */

static inline VkCommandBuffer *convert_VkCommandBuffer_array_win_to_host(const VkCommandBuffer *in, uint32_t count)
{
    VkCommandBuffer *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i] = in[i]->command_buffer;
    }

    return out;
}

static inline void free_VkCommandBuffer_array(VkCommandBuffer *in, uint32_t count)
{
    if (!in) return;

    free(in);
}

#if defined(USE_STRUCT_CONVERSION)
static inline VkIndirectCommandsStreamNV_host *convert_VkIndirectCommandsStreamNV_array_win_to_host(const VkIndirectCommandsStreamNV *in, uint32_t count)
{
    VkIndirectCommandsStreamNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].offset = in[i].offset;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkIndirectCommandsStreamNV_array(VkIndirectCommandsStreamNV_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkGeneratedCommandsInfoNV(VkGeneratedCommandsInfoNV_host *in)
{
    free_VkIndirectCommandsStreamNV_array((VkIndirectCommandsStreamNV_host *)in->pStreams, in->streamCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferMemoryBarrier_host *convert_VkBufferMemoryBarrier_array_win_to_host(const VkBufferMemoryBarrier *in, uint32_t count)
{
    VkBufferMemoryBarrier_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBufferMemoryBarrier_array(VkBufferMemoryBarrier_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkImageMemoryBarrier_host *convert_VkImageMemoryBarrier_array_win_to_host(const VkImageMemoryBarrier *in, uint32_t count)
{
    VkImageMemoryBarrier_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkImageMemoryBarrier_array(VkImageMemoryBarrier_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferMemoryBarrier2_host *convert_VkBufferMemoryBarrier2_array_win_to_host(const VkBufferMemoryBarrier2 *in, uint32_t count)
{
    VkBufferMemoryBarrier2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].srcStageMask = in[i].srcStageMask;
        out[i].srcAccessMask = in[i].srcAccessMask;
        out[i].dstStageMask = in[i].dstStageMask;
        out[i].dstAccessMask = in[i].dstAccessMask;
        out[i].srcQueueFamilyIndex = in[i].srcQueueFamilyIndex;
        out[i].dstQueueFamilyIndex = in[i].dstQueueFamilyIndex;
        out[i].buffer = in[i].buffer;
        out[i].offset = in[i].offset;
        out[i].size = in[i].size;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBufferMemoryBarrier2_array(VkBufferMemoryBarrier2_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkImageMemoryBarrier2_host *convert_VkImageMemoryBarrier2_array_win_to_host(const VkImageMemoryBarrier2 *in, uint32_t count)
{
    VkImageMemoryBarrier2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].srcStageMask = in[i].srcStageMask;
        out[i].srcAccessMask = in[i].srcAccessMask;
        out[i].dstStageMask = in[i].dstStageMask;
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkImageMemoryBarrier2_array(VkImageMemoryBarrier2_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDependencyInfo_win_to_host(const VkDependencyInfo *in, VkDependencyInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->dependencyFlags = in->dependencyFlags;
    out->memoryBarrierCount = in->memoryBarrierCount;
    out->pMemoryBarriers = in->pMemoryBarriers;
    out->bufferMemoryBarrierCount = in->bufferMemoryBarrierCount;
    out->pBufferMemoryBarriers = convert_VkBufferMemoryBarrier2_array_win_to_host(in->pBufferMemoryBarriers, in->bufferMemoryBarrierCount);
    out->imageMemoryBarrierCount = in->imageMemoryBarrierCount;
    out->pImageMemoryBarriers = convert_VkImageMemoryBarrier2_array_win_to_host(in->pImageMemoryBarriers, in->imageMemoryBarrierCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkDependencyInfo(VkDependencyInfo_host *in)
{
    free_VkBufferMemoryBarrier2_array((VkBufferMemoryBarrier2_host *)in->pBufferMemoryBarriers, in->bufferMemoryBarrierCount);
    free_VkImageMemoryBarrier2_array((VkImageMemoryBarrier2_host *)in->pImageMemoryBarriers, in->imageMemoryBarrierCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDescriptorImageInfo_host *convert_VkDescriptorImageInfo_array_win_to_host(const VkDescriptorImageInfo *in, uint32_t count)
{
    VkDescriptorImageInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sampler = in[i].sampler;
        out[i].imageView = in[i].imageView;
        out[i].imageLayout = in[i].imageLayout;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkDescriptorImageInfo_array(VkDescriptorImageInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDescriptorBufferInfo_host *convert_VkDescriptorBufferInfo_array_win_to_host(const VkDescriptorBufferInfo *in, uint32_t count)
{
    VkDescriptorBufferInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].offset = in[i].offset;
        out[i].range = in[i].range;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkDescriptorBufferInfo_array(VkDescriptorBufferInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkWriteDescriptorSet_host *convert_VkWriteDescriptorSet_array_win_to_host(const VkWriteDescriptorSet *in, uint32_t count)
{
    VkWriteDescriptorSet_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkWriteDescriptorSet_array(VkWriteDescriptorSet_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkDescriptorImageInfo_array((VkDescriptorImageInfo_host *)in[i].pImageInfo, in[i].descriptorCount);
        free_VkDescriptorBufferInfo_array((VkDescriptorBufferInfo_host *)in[i].pBufferInfo, in[i].descriptorCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkResolveImageInfo2_win_to_host(const VkResolveImageInfo2 *in, VkResolveImageInfo2_host *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPerformanceMarkerInfoINTEL_win_to_host(const VkPerformanceMarkerInfoINTEL *in, VkPerformanceMarkerInfoINTEL_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->marker = in->marker;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPerformanceOverrideInfoINTEL_win_to_host(const VkPerformanceOverrideInfoINTEL *in, VkPerformanceOverrideInfoINTEL_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->enable = in->enable;
    out->parameter = in->parameter;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkStridedDeviceAddressRegionKHR_win_to_host(const VkStridedDeviceAddressRegionKHR *in, VkStridedDeviceAddressRegionKHR_host *out)
{
    if (!in) return;

    out->deviceAddress = in->deviceAddress;
    out->stride = in->stride;
    out->size = in->size;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDependencyInfo_host *convert_VkDependencyInfo_array_win_to_host(const VkDependencyInfo *in, uint32_t count)
{
    VkDependencyInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].dependencyFlags = in[i].dependencyFlags;
        out[i].memoryBarrierCount = in[i].memoryBarrierCount;
        out[i].pMemoryBarriers = in[i].pMemoryBarriers;
        out[i].bufferMemoryBarrierCount = in[i].bufferMemoryBarrierCount;
        out[i].pBufferMemoryBarriers = convert_VkBufferMemoryBarrier2_array_win_to_host(in[i].pBufferMemoryBarriers, in[i].bufferMemoryBarrierCount);
        out[i].imageMemoryBarrierCount = in[i].imageMemoryBarrierCount;
        out[i].pImageMemoryBarriers = convert_VkImageMemoryBarrier2_array_win_to_host(in[i].pImageMemoryBarriers, in[i].imageMemoryBarrierCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkDependencyInfo_array(VkDependencyInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkBufferMemoryBarrier2_array((VkBufferMemoryBarrier2_host *)in[i].pBufferMemoryBarriers, in[i].bufferMemoryBarrierCount);
        free_VkImageMemoryBarrier2_array((VkImageMemoryBarrier2_host *)in[i].pImageMemoryBarriers, in[i].imageMemoryBarrierCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureCreateInfoKHR_win_to_host(const VkAccelerationStructureCreateInfoKHR *in, VkAccelerationStructureCreateInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->createFlags = in->createFlags;
    out->buffer = in->buffer;
    out->offset = in->offset;
    out->size = in->size;
    out->type = in->type;
    out->deviceAddress = in->deviceAddress;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureCreateInfoNV_win_to_host(const VkAccelerationStructureCreateInfoNV *in, VkAccelerationStructureCreateInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->compactedSize = in->compactedSize;
    convert_VkAccelerationStructureInfoNV_win_to_host(&in->info, &out->info);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkComputePipelineCreateInfo_host *convert_VkComputePipelineCreateInfo_array_win_to_host(const VkComputePipelineCreateInfo *in, uint32_t count)
{
    VkComputePipelineCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkComputePipelineCreateInfo_array(VkComputePipelineCreateInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCuFunctionCreateInfoNVX_win_to_host(const VkCuFunctionCreateInfoNVX *in, VkCuFunctionCreateInfoNVX_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->module = in->module;
    out->pName = in->pName;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkPipelineShaderStageCreateInfo_host *convert_VkPipelineShaderStageCreateInfo_array_win_to_host(const VkPipelineShaderStageCreateInfo *in, uint32_t count)
{
    VkPipelineShaderStageCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkPipelineShaderStageCreateInfo_array(VkPipelineShaderStageCreateInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkGraphicsPipelineCreateInfo_host *convert_VkGraphicsPipelineCreateInfo_array_win_to_host(const VkGraphicsPipelineCreateInfo *in, uint32_t count)
{
    VkGraphicsPipelineCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkGraphicsPipelineCreateInfo_array(VkGraphicsPipelineCreateInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkPipelineShaderStageCreateInfo_array((VkPipelineShaderStageCreateInfo_host *)in[i].pStages, in[i].stageCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkIndirectCommandsLayoutTokenNV_host *convert_VkIndirectCommandsLayoutTokenNV_array_win_to_host(const VkIndirectCommandsLayoutTokenNV *in, uint32_t count)
{
    VkIndirectCommandsLayoutTokenNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkIndirectCommandsLayoutTokenNV_array(VkIndirectCommandsLayoutTokenNV_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkIndirectCommandsLayoutCreateInfoNV(VkIndirectCommandsLayoutCreateInfoNV_host *in)
{
    free_VkIndirectCommandsLayoutTokenNV_array((VkIndirectCommandsLayoutTokenNV_host *)in->pTokens, in->tokenCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkRayTracingPipelineCreateInfoKHR_host *convert_VkRayTracingPipelineCreateInfoKHR_array_win_to_host(const VkRayTracingPipelineCreateInfoKHR *in, uint32_t count)
{
    VkRayTracingPipelineCreateInfoKHR_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].stageCount = in[i].stageCount;
        out[i].pStages = convert_VkPipelineShaderStageCreateInfo_array_win_to_host(in[i].pStages, in[i].stageCount);
        out[i].groupCount = in[i].groupCount;
        out[i].pGroups = in[i].pGroups;
        out[i].maxPipelineRayRecursionDepth = in[i].maxPipelineRayRecursionDepth;
        out[i].pLibraryInfo = in[i].pLibraryInfo;
        out[i].pLibraryInterface = in[i].pLibraryInterface;
        out[i].pDynamicState = in[i].pDynamicState;
        out[i].layout = in[i].layout;
        out[i].basePipelineHandle = in[i].basePipelineHandle;
        out[i].basePipelineIndex = in[i].basePipelineIndex;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkRayTracingPipelineCreateInfoKHR_array(VkRayTracingPipelineCreateInfoKHR_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkPipelineShaderStageCreateInfo_array((VkPipelineShaderStageCreateInfo_host *)in[i].pStages, in[i].stageCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkRayTracingPipelineCreateInfoNV_host *convert_VkRayTracingPipelineCreateInfoNV_array_win_to_host(const VkRayTracingPipelineCreateInfoNV *in, uint32_t count)
{
    VkRayTracingPipelineCreateInfoNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkRayTracingPipelineCreateInfoNV_array(VkRayTracingPipelineCreateInfoNV_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkPipelineShaderStageCreateInfo_array((VkPipelineShaderStageCreateInfo_host *)in[i].pStages, in[i].stageCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSwapchainCreateInfoKHR_win_to_host(const VkSwapchainCreateInfoKHR *in, VkSwapchainCreateInfoKHR_host *out)
#else
static inline void convert_VkSwapchainCreateInfoKHR_win_to_host(const VkSwapchainCreateInfoKHR *in, VkSwapchainCreateInfoKHR *out)
#endif /* USE_STRUCT_CONVERSION */
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->surface = wine_surface_from_handle(in->surface)->driver_surface;
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

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugMarkerObjectNameInfoEXT_win_to_host(const VkDebugMarkerObjectNameInfoEXT *in, VkDebugMarkerObjectNameInfoEXT_host *out)
#else
static inline void convert_VkDebugMarkerObjectNameInfoEXT_win_to_host(const VkDebugMarkerObjectNameInfoEXT *in, VkDebugMarkerObjectNameInfoEXT *out)
#endif /* USE_STRUCT_CONVERSION */
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->object = wine_vk_unwrap_handle(in->objectType, in->object);
    out->pObjectName = in->pObjectName;
}

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugMarkerObjectTagInfoEXT_win_to_host(const VkDebugMarkerObjectTagInfoEXT *in, VkDebugMarkerObjectTagInfoEXT_host *out)
#else
static inline void convert_VkDebugMarkerObjectTagInfoEXT_win_to_host(const VkDebugMarkerObjectTagInfoEXT *in, VkDebugMarkerObjectTagInfoEXT *out)
#endif /* USE_STRUCT_CONVERSION */
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->object = wine_vk_unwrap_handle(in->objectType, in->object);
    out->tagName = in->tagName;
    out->tagSize = in->tagSize;
    out->pTag = in->pTag;
}

#if defined(USE_STRUCT_CONVERSION)
static inline VkMappedMemoryRange_host *convert_VkMappedMemoryRange_array_win_to_host(const VkMappedMemoryRange *in, uint32_t count)
{
    VkMappedMemoryRange_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkMappedMemoryRange_array(VkMappedMemoryRange_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureBuildGeometryInfoKHR_win_to_host(const VkAccelerationStructureBuildGeometryInfoKHR *in, VkAccelerationStructureBuildGeometryInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->flags = in->flags;
    out->mode = in->mode;
    out->srcAccelerationStructure = in->srcAccelerationStructure;
    out->dstAccelerationStructure = in->dstAccelerationStructure;
    out->geometryCount = in->geometryCount;
    out->pGeometries = in->pGeometries;
    out->ppGeometries = in->ppGeometries;
    out->scratchData = in->scratchData;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureBuildSizesInfoKHR_win_to_host(const VkAccelerationStructureBuildSizesInfoKHR *in, VkAccelerationStructureBuildSizesInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->accelerationStructureSize = in->accelerationStructureSize;
    out->updateScratchSize = in->updateScratchSize;
    out->buildScratchSize = in->buildScratchSize;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureDeviceAddressInfoKHR_win_to_host(const VkAccelerationStructureDeviceAddressInfoKHR *in, VkAccelerationStructureDeviceAddressInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->accelerationStructure = in->accelerationStructure;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureMemoryRequirementsInfoNV_win_to_host(const VkAccelerationStructureMemoryRequirementsInfoNV *in, VkAccelerationStructureMemoryRequirementsInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->accelerationStructure = in->accelerationStructure;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements_host_to_win(const VkMemoryRequirements_host *in, VkMemoryRequirements *out)
{
    if (!in) return;

    out->size = in->size;
    out->alignment = in->alignment;
    out->memoryTypeBits = in->memoryTypeBits;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2KHR_win_to_host(const VkMemoryRequirements2KHR *in, VkMemoryRequirements2KHR_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2KHR_host_to_win(const VkMemoryRequirements2KHR_host *in, VkMemoryRequirements2KHR *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkMemoryRequirements_host_to_win(&in->memoryRequirements, &out->memoryRequirements);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkBufferDeviceAddressInfo_win_to_host(const VkBufferDeviceAddressInfo *in, VkBufferDeviceAddressInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkBufferMemoryRequirementsInfo2_win_to_host(const VkBufferMemoryRequirementsInfo2 *in, VkBufferMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2_win_to_host(const VkMemoryRequirements2 *in, VkMemoryRequirements2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2_host_to_win(const VkMemoryRequirements2_host *in, VkMemoryRequirements2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkMemoryRequirements_host_to_win(&in->memoryRequirements, &out->memoryRequirements);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDescriptorSetBindingReferenceVALVE_win_to_host(const VkDescriptorSetBindingReferenceVALVE *in, VkDescriptorSetBindingReferenceVALVE_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->descriptorSetLayout = in->descriptorSetLayout;
    out->binding = in->binding;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferCreateInfo_host *convert_VkBufferCreateInfo_array_win_to_host(const VkBufferCreateInfo *in, uint32_t count)
{
    VkBufferCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].size = in[i].size;
        out[i].usage = in[i].usage;
        out[i].sharingMode = in[i].sharingMode;
        out[i].queueFamilyIndexCount = in[i].queueFamilyIndexCount;
        out[i].pQueueFamilyIndices = in[i].pQueueFamilyIndices;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkBufferCreateInfo_array(VkBufferCreateInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDeviceBufferMemoryRequirements_win_to_host(const VkDeviceBufferMemoryRequirements *in, VkDeviceBufferMemoryRequirements_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pCreateInfo = convert_VkBufferCreateInfo_array_win_to_host(in->pCreateInfo, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkDeviceBufferMemoryRequirements(VkDeviceBufferMemoryRequirements_host *in)
{
    free_VkBufferCreateInfo_array((VkBufferCreateInfo_host *)in->pCreateInfo, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win_to_host(const VkDeviceMemoryOpaqueCaptureAddressInfo *in, VkDeviceMemoryOpaqueCaptureAddressInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->memory = in->memory;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageMemoryRequirementsInfo2_win_to_host(const VkImageMemoryRequirementsInfo2 *in, VkImageMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->image = in->image;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageSparseMemoryRequirementsInfo2_win_to_host(const VkImageSparseMemoryRequirementsInfo2 *in, VkImageSparseMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->image = in->image;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSubresourceLayout_host_to_win(const VkSubresourceLayout_host *in, VkSubresourceLayout *out)
{
    if (!in) return;

    out->offset = in->offset;
    out->size = in->size;
    out->rowPitch = in->rowPitch;
    out->arrayPitch = in->arrayPitch;
    out->depthPitch = in->depthPitch;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSubresourceLayout2EXT_win_to_host(const VkSubresourceLayout2EXT *in, VkSubresourceLayout2EXT_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSubresourceLayout2EXT_host_to_win(const VkSubresourceLayout2EXT_host *in, VkSubresourceLayout2EXT *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkSubresourceLayout_host_to_win(&in->subresourceLayout, &out->subresourceLayout);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageViewAddressPropertiesNVX_win_to_host(const VkImageViewAddressPropertiesNVX *in, VkImageViewAddressPropertiesNVX_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageViewAddressPropertiesNVX_host_to_win(const VkImageViewAddressPropertiesNVX_host *in, VkImageViewAddressPropertiesNVX *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->deviceAddress = in->deviceAddress;
    out->size = in->size;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageViewHandleInfoNVX_win_to_host(const VkImageViewHandleInfoNVX *in, VkImageViewHandleInfoNVX_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->imageView = in->imageView;
    out->descriptorType = in->descriptorType;
    out->sampler = in->sampler;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryGetWin32HandleInfoKHR_win_to_host(const VkMemoryGetWin32HandleInfoKHR *in, VkMemoryGetWin32HandleInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->memory = in->memory;
    out->handleType = in->handleType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageFormatProperties_host_to_win(const VkImageFormatProperties_host *in, VkImageFormatProperties *out)
{
    if (!in) return;

    out->maxExtent = in->maxExtent;
    out->maxMipLevels = in->maxMipLevels;
    out->maxArrayLayers = in->maxArrayLayers;
    out->sampleCounts = in->sampleCounts;
    out->maxResourceSize = in->maxResourceSize;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageFormatProperties2_win_to_host(const VkImageFormatProperties2 *in, VkImageFormatProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageFormatProperties2_host_to_win(const VkImageFormatProperties2_host *in, VkImageFormatProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkImageFormatProperties_host_to_win(&in->imageFormatProperties, &out->imageFormatProperties);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION) */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceMemoryProperties_host_to_win(const VkPhysicalDeviceMemoryProperties_host *in, VkPhysicalDeviceMemoryProperties *out)
{
    if (!in) return;

    out->memoryTypeCount = in->memoryTypeCount;
    memcpy(out->memoryTypes, in->memoryTypes, VK_MAX_MEMORY_TYPES * sizeof(VkMemoryType));
    out->memoryHeapCount = in->memoryHeapCount;
    convert_VkMemoryHeap_static_array_host_to_win(in->memoryHeaps, out->memoryHeaps, VK_MAX_MEMORY_HEAPS);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceMemoryProperties2_win_to_host(const VkPhysicalDeviceMemoryProperties2 *in, VkPhysicalDeviceMemoryProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceMemoryProperties2_host_to_win(const VkPhysicalDeviceMemoryProperties2_host *in, VkPhysicalDeviceMemoryProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkPhysicalDeviceMemoryProperties_host_to_win(&in->memoryProperties, &out->memoryProperties);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceProperties2_win_to_host(const VkPhysicalDeviceProperties2 *in, VkPhysicalDeviceProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceProperties2_host_to_win(const VkPhysicalDeviceProperties2_host *in, VkPhysicalDeviceProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkPhysicalDeviceProperties_host_to_win(&in->properties, &out->properties);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(const VkPhysicalDeviceSurfaceInfo2KHR *in, VkPhysicalDeviceSurfaceInfo2KHR_host *out)
#else
static inline void convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(const VkPhysicalDeviceSurfaceInfo2KHR *in, VkPhysicalDeviceSurfaceInfo2KHR *out)
#endif /* USE_STRUCT_CONVERSION */
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->surface = wine_surface_from_handle(in->surface)->driver_surface;
}

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPipelineExecutableInfoKHR_win_to_host(const VkPipelineExecutableInfoKHR *in, VkPipelineExecutableInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
    out->executableIndex = in->executableIndex;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPipelineInfoKHR_win_to_host(const VkPipelineInfoKHR *in, VkPipelineInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPipelineInfoEXT_win_to_host(const VkPipelineInfoEXT *in, VkPipelineInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseMemoryBind_host *convert_VkSparseMemoryBind_array_win_to_host(const VkSparseMemoryBind *in, uint32_t count)
{
    VkSparseMemoryBind_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkSparseMemoryBind_array(VkSparseMemoryBind_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseBufferMemoryBindInfo_host *convert_VkSparseBufferMemoryBindInfo_array_win_to_host(const VkSparseBufferMemoryBindInfo *in, uint32_t count)
{
    VkSparseBufferMemoryBindInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseMemoryBind_array_win_to_host(in[i].pBinds, in[i].bindCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkSparseBufferMemoryBindInfo_array(VkSparseBufferMemoryBindInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkSparseMemoryBind_array((VkSparseMemoryBind_host *)in[i].pBinds, in[i].bindCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseImageOpaqueMemoryBindInfo_host *convert_VkSparseImageOpaqueMemoryBindInfo_array_win_to_host(const VkSparseImageOpaqueMemoryBindInfo *in, uint32_t count)
{
    VkSparseImageOpaqueMemoryBindInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].image = in[i].image;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseMemoryBind_array_win_to_host(in[i].pBinds, in[i].bindCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkSparseImageOpaqueMemoryBindInfo_array(VkSparseImageOpaqueMemoryBindInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkSparseMemoryBind_array((VkSparseMemoryBind_host *)in[i].pBinds, in[i].bindCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseImageMemoryBind_host *convert_VkSparseImageMemoryBind_array_win_to_host(const VkSparseImageMemoryBind *in, uint32_t count)
{
    VkSparseImageMemoryBind_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkSparseImageMemoryBind_array(VkSparseImageMemoryBind_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseImageMemoryBindInfo_host *convert_VkSparseImageMemoryBindInfo_array_win_to_host(const VkSparseImageMemoryBindInfo *in, uint32_t count)
{
    VkSparseImageMemoryBindInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].image = in[i].image;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseImageMemoryBind_array_win_to_host(in[i].pBinds, in[i].bindCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkSparseImageMemoryBindInfo_array(VkSparseImageMemoryBindInfo_host *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkSparseImageMemoryBind_array((VkSparseImageMemoryBind_host *)in[i].pBinds, in[i].bindCount);
    }
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBindSparseInfo_host *convert_VkBindSparseInfo_array_win_to_host(const VkBindSparseInfo *in, uint32_t count)
{
    VkBindSparseInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
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
    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

static inline VkSubmitInfo *convert_VkSubmitInfo_array_win_to_host(const VkSubmitInfo *in, uint32_t count)
{
    VkSubmitInfo *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].waitSemaphoreCount = in[i].waitSemaphoreCount;
        out[i].pWaitSemaphores = in[i].pWaitSemaphores;
        out[i].pWaitDstStageMask = in[i].pWaitDstStageMask;
        out[i].commandBufferCount = in[i].commandBufferCount;
        out[i].pCommandBuffers = convert_VkCommandBuffer_array_win_to_host(in[i].pCommandBuffers, in[i].commandBufferCount);
        out[i].signalSemaphoreCount = in[i].signalSemaphoreCount;
        out[i].pSignalSemaphores = in[i].pSignalSemaphores;
    }

    return out;
}

static inline void free_VkSubmitInfo_array(VkSubmitInfo *in, uint32_t count)
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
        free_VkCommandBuffer_array((VkCommandBuffer *)in[i].pCommandBuffers, in[i].commandBufferCount);
    }
    free(in);
}

#if defined(USE_STRUCT_CONVERSION)
static inline VkSemaphoreSubmitInfo_host *convert_VkSemaphoreSubmitInfo_array_win_to_host(const VkSemaphoreSubmitInfo *in, uint32_t count)
{
    VkSemaphoreSubmitInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].semaphore = in[i].semaphore;
        out[i].value = in[i].value;
        out[i].stageMask = in[i].stageMask;
        out[i].deviceIndex = in[i].deviceIndex;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkSemaphoreSubmitInfo_array(VkSemaphoreSubmitInfo_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

static inline VkCommandBufferSubmitInfo *convert_VkCommandBufferSubmitInfo_array_win_to_host(const VkCommandBufferSubmitInfo *in, uint32_t count)
{
    VkCommandBufferSubmitInfo *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].commandBuffer = in[i].commandBuffer->command_buffer;
        out[i].deviceMask = in[i].deviceMask;
    }

    return out;
}

static inline void free_VkCommandBufferSubmitInfo_array(VkCommandBufferSubmitInfo *in, uint32_t count)
{
    if (!in) return;

    free(in);
}

#if defined(USE_STRUCT_CONVERSION)
static inline VkSubmitInfo2_host *convert_VkSubmitInfo2_array_win_to_host(const VkSubmitInfo2 *in, uint32_t count)
{
    VkSubmitInfo2_host *out;
#else
static inline VkSubmitInfo2 *convert_VkSubmitInfo2_array_win_to_host(const VkSubmitInfo2 *in, uint32_t count)
{
    VkSubmitInfo2 *out;
#endif /* USE_STRUCT_CONVERSION */
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].waitSemaphoreInfoCount = in[i].waitSemaphoreInfoCount;
#if defined(USE_STRUCT_CONVERSION)
        out[i].pWaitSemaphoreInfos = convert_VkSemaphoreSubmitInfo_array_win_to_host(in[i].pWaitSemaphoreInfos, in[i].waitSemaphoreInfoCount);
#else
        out[i].pWaitSemaphoreInfos = in[i].pWaitSemaphoreInfos;
#endif /* USE_STRUCT_CONVERSION */
        out[i].commandBufferInfoCount = in[i].commandBufferInfoCount;
        out[i].pCommandBufferInfos = convert_VkCommandBufferSubmitInfo_array_win_to_host(in[i].pCommandBufferInfos, in[i].commandBufferInfoCount);
        out[i].signalSemaphoreInfoCount = in[i].signalSemaphoreInfoCount;
#if defined(USE_STRUCT_CONVERSION)
        out[i].pSignalSemaphoreInfos = convert_VkSemaphoreSubmitInfo_array_win_to_host(in[i].pSignalSemaphoreInfos, in[i].signalSemaphoreInfoCount);
#else
        out[i].pSignalSemaphoreInfos = in[i].pSignalSemaphoreInfos;
#endif /* USE_STRUCT_CONVERSION */
    }

    return out;
}

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkSubmitInfo2_array(VkSubmitInfo2_host *in, uint32_t count)
#else
static inline void free_VkSubmitInfo2_array(VkSubmitInfo2 *in, uint32_t count)
#endif /* USE_STRUCT_CONVERSION */
{
    unsigned int i;

    if (!in) return;

    for (i = 0; i < count; i++)
    {
#if defined(USE_STRUCT_CONVERSION)
        free_VkSemaphoreSubmitInfo_array((VkSemaphoreSubmitInfo_host *)in[i].pWaitSemaphoreInfos, in[i].waitSemaphoreInfoCount);
#endif /* USE_STRUCT_CONVERSION */
        free_VkCommandBufferSubmitInfo_array((VkCommandBufferSubmitInfo *)in[i].pCommandBufferInfos, in[i].commandBufferInfoCount);
#if defined(USE_STRUCT_CONVERSION)
        free_VkSemaphoreSubmitInfo_array((VkSemaphoreSubmitInfo_host *)in[i].pSignalSemaphoreInfos, in[i].signalSemaphoreInfoCount);
#endif /* USE_STRUCT_CONVERSION */
    }
    free(in);
}

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsObjectNameInfoEXT_win_to_host(const VkDebugUtilsObjectNameInfoEXT *in, VkDebugUtilsObjectNameInfoEXT_host *out)
#else
static inline void convert_VkDebugUtilsObjectNameInfoEXT_win_to_host(const VkDebugUtilsObjectNameInfoEXT *in, VkDebugUtilsObjectNameInfoEXT *out)
#endif /* USE_STRUCT_CONVERSION */
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->objectHandle = wine_vk_unwrap_handle(in->objectType, in->objectHandle);
    out->pObjectName = in->pObjectName;
}

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsObjectTagInfoEXT_win_to_host(const VkDebugUtilsObjectTagInfoEXT *in, VkDebugUtilsObjectTagInfoEXT_host *out)
#else
static inline void convert_VkDebugUtilsObjectTagInfoEXT_win_to_host(const VkDebugUtilsObjectTagInfoEXT *in, VkDebugUtilsObjectTagInfoEXT *out)
#endif /* USE_STRUCT_CONVERSION */
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->objectHandle = wine_vk_unwrap_handle(in->objectType, in->objectHandle);
    out->tagName = in->tagName;
    out->tagSize = in->tagSize;
    out->pTag = in->pTag;
}

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSemaphoreSignalInfo_win_to_host(const VkSemaphoreSignalInfo *in, VkSemaphoreSignalInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->semaphore = in->semaphore;
    out->value = in->value;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDebugUtilsObjectNameInfoEXT_host *convert_VkDebugUtilsObjectNameInfoEXT_array_win_to_host(const VkDebugUtilsObjectNameInfoEXT *in, uint32_t count)
{
    VkDebugUtilsObjectNameInfoEXT_host *out;
#else
static inline VkDebugUtilsObjectNameInfoEXT *convert_VkDebugUtilsObjectNameInfoEXT_array_win_to_host(const VkDebugUtilsObjectNameInfoEXT *in, uint32_t count)
{
    VkDebugUtilsObjectNameInfoEXT *out;
#endif /* USE_STRUCT_CONVERSION */
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].objectType = in[i].objectType;
        out[i].objectHandle = wine_vk_unwrap_handle(in[i].objectType, in[i].objectHandle);
        out[i].pObjectName = in[i].pObjectName;
    }

    return out;
}

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkDebugUtilsObjectNameInfoEXT_array(VkDebugUtilsObjectNameInfoEXT_host *in, uint32_t count)
#else
static inline void free_VkDebugUtilsObjectNameInfoEXT_array(VkDebugUtilsObjectNameInfoEXT *in, uint32_t count)
#endif /* USE_STRUCT_CONVERSION */
{
    if (!in) return;

    free(in);
}

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsMessengerCallbackDataEXT_win_to_host(const VkDebugUtilsMessengerCallbackDataEXT *in, VkDebugUtilsMessengerCallbackDataEXT_host *out)
#else
static inline void convert_VkDebugUtilsMessengerCallbackDataEXT_win_to_host(const VkDebugUtilsMessengerCallbackDataEXT *in, VkDebugUtilsMessengerCallbackDataEXT *out)
#endif /* USE_STRUCT_CONVERSION */
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

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkDebugUtilsMessengerCallbackDataEXT(VkDebugUtilsMessengerCallbackDataEXT_host *in)
#else
static inline void free_VkDebugUtilsMessengerCallbackDataEXT(VkDebugUtilsMessengerCallbackDataEXT *in)
#endif /* USE_STRUCT_CONVERSION */
{
#if defined(USE_STRUCT_CONVERSION)
    free_VkDebugUtilsObjectNameInfoEXT_array((VkDebugUtilsObjectNameInfoEXT_host *)in->pObjects, in->objectCount);
#else
    free_VkDebugUtilsObjectNameInfoEXT_array((VkDebugUtilsObjectNameInfoEXT *)in->pObjects, in->objectCount);
#endif /* USE_STRUCT_CONVERSION */
}

#if defined(USE_STRUCT_CONVERSION)
static inline VkCopyDescriptorSet_host *convert_VkCopyDescriptorSet_array_win_to_host(const VkCopyDescriptorSet *in, uint32_t count)
{
    VkCopyDescriptorSet_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void free_VkCopyDescriptorSet_array(VkCopyDescriptorSet_host *in, uint32_t count)
{
    if (!in) return;

    free(in);
}
#endif /* USE_STRUCT_CONVERSION */

static inline VkPhysicalDevice *convert_VkPhysicalDevice_array_win_to_host(const VkPhysicalDevice *in, uint32_t count)
{
    VkPhysicalDevice *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = malloc(count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i] = in[i]->phys_dev;
    }

    return out;
}

static inline void free_VkPhysicalDevice_array(VkPhysicalDevice *in, uint32_t count)
{
    if (!in) return;

    free(in);
}

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
            break;

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV:
        {
            const VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV *in = (const VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV *)in_header;
            VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->deviceGeneratedCommands = in->deviceGeneratedCommands;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_DEVICE_PRIVATE_DATA_CREATE_INFO:
        {
            const VkDevicePrivateDataCreateInfo *in = (const VkDevicePrivateDataCreateInfo *)in_header;
            VkDevicePrivateDataCreateInfo *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->privateDataSlotRequestCount = in->privateDataSlotRequestCount;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES:
        {
            const VkPhysicalDevicePrivateDataFeatures *in = (const VkPhysicalDevicePrivateDataFeatures *)in_header;
            VkPhysicalDevicePrivateDataFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->physicalDeviceCount = in->physicalDeviceCount;
            out->pPhysicalDevices = convert_VkPhysicalDevice_array_win_to_host(in->pPhysicalDevices, in->physicalDeviceCount);

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR:
        {
            const VkPhysicalDevicePresentIdFeaturesKHR *in = (const VkPhysicalDevicePresentIdFeaturesKHR *)in_header;
            VkPhysicalDevicePresentIdFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->presentId = in->presentId;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR:
        {
            const VkPhysicalDevicePresentWaitFeaturesKHR *in = (const VkPhysicalDevicePresentWaitFeaturesKHR *)in_header;
            VkPhysicalDevicePresentWaitFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->presentWait = in->presentWait;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES:
        {
            const VkPhysicalDevice16BitStorageFeatures *in = (const VkPhysicalDevice16BitStorageFeatures *)in_header;
            VkPhysicalDevice16BitStorageFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->advancedBlendCoherentOperations = in->advancedBlendCoherentOperations;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT:
        {
            const VkPhysicalDeviceMultiDrawFeaturesEXT *in = (const VkPhysicalDeviceMultiDrawFeaturesEXT *)in_header;
            VkPhysicalDeviceMultiDrawFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->multiDraw = in->multiDraw;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES:
        {
            const VkPhysicalDeviceInlineUniformBlockFeatures *in = (const VkPhysicalDeviceInlineUniformBlockFeatures *)in_header;
            VkPhysicalDeviceInlineUniformBlockFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->inlineUniformBlock = in->inlineUniformBlock;
            out->descriptorBindingInlineUniformBlockUpdateAfterBind = in->descriptorBindingInlineUniformBlockUpdateAfterBind;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES:
        {
            const VkPhysicalDeviceMaintenance4Features *in = (const VkPhysicalDeviceMaintenance4Features *)in_header;
            VkPhysicalDeviceMaintenance4Features *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->maintenance4 = in->maintenance4;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES:
        {
            const VkPhysicalDeviceShaderDrawParametersFeatures *in = (const VkPhysicalDeviceShaderDrawParametersFeatures *)in_header;
            VkPhysicalDeviceShaderDrawParametersFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->hostQueryReset = in->hostQueryReset;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_KHR:
        {
            const VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR *in = (const VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR *)in_header;
            VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->globalPriorityQuery = in->globalPriorityQuery;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES:
        {
            const VkPhysicalDeviceDescriptorIndexingFeatures *in = (const VkPhysicalDeviceDescriptorIndexingFeatures *)in_header;
            VkPhysicalDeviceDescriptorIndexingFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT:
        {
            const VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT *in = (const VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT *)in_header;
            VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderBufferFloat16Atomics = in->shaderBufferFloat16Atomics;
            out->shaderBufferFloat16AtomicAdd = in->shaderBufferFloat16AtomicAdd;
            out->shaderBufferFloat16AtomicMinMax = in->shaderBufferFloat16AtomicMinMax;
            out->shaderBufferFloat32AtomicMinMax = in->shaderBufferFloat32AtomicMinMax;
            out->shaderBufferFloat64AtomicMinMax = in->shaderBufferFloat64AtomicMinMax;
            out->shaderSharedFloat16Atomics = in->shaderSharedFloat16Atomics;
            out->shaderSharedFloat16AtomicAdd = in->shaderSharedFloat16AtomicAdd;
            out->shaderSharedFloat16AtomicMinMax = in->shaderSharedFloat16AtomicMinMax;
            out->shaderSharedFloat32AtomicMinMax = in->shaderSharedFloat32AtomicMinMax;
            out->shaderSharedFloat64AtomicMinMax = in->shaderSharedFloat64AtomicMinMax;
            out->shaderImageFloat32AtomicMinMax = in->shaderImageFloat32AtomicMinMax;
            out->sparseImageFloat32AtomicMinMax = in->sparseImageFloat32AtomicMinMax;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT:
        {
            const VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *in = (const VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *)in_header;
            VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->computeDerivativeGroupQuads = in->computeDerivativeGroupQuads;
            out->computeDerivativeGroupLinear = in->computeDerivativeGroupLinear;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV:
        {
            const VkPhysicalDeviceShaderImageFootprintFeaturesNV *in = (const VkPhysicalDeviceShaderImageFootprintFeaturesNV *)in_header;
            VkPhysicalDeviceShaderImageFootprintFeaturesNV *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shadingRateImage = in->shadingRateImage;
            out->shadingRateCoarseSampleOrder = in->shadingRateCoarseSampleOrder;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INVOCATION_MASK_FEATURES_HUAWEI:
        {
            const VkPhysicalDeviceInvocationMaskFeaturesHUAWEI *in = (const VkPhysicalDeviceInvocationMaskFeaturesHUAWEI *)in_header;
            VkPhysicalDeviceInvocationMaskFeaturesHUAWEI *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->invocationMask = in->invocationMask;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV:
        {
            const VkPhysicalDeviceMeshShaderFeaturesNV *in = (const VkPhysicalDeviceMeshShaderFeaturesNV *)in_header;
            VkPhysicalDeviceMeshShaderFeaturesNV *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->taskShader = in->taskShader;
            out->meshShader = in->meshShader;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR:
        {
            const VkPhysicalDeviceAccelerationStructureFeaturesKHR *in = (const VkPhysicalDeviceAccelerationStructureFeaturesKHR *)in_header;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->accelerationStructure = in->accelerationStructure;
            out->accelerationStructureCaptureReplay = in->accelerationStructureCaptureReplay;
            out->accelerationStructureIndirectBuild = in->accelerationStructureIndirectBuild;
            out->accelerationStructureHostCommands = in->accelerationStructureHostCommands;
            out->descriptorBindingAccelerationStructureUpdateAfterBind = in->descriptorBindingAccelerationStructureUpdateAfterBind;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR:
        {
            const VkPhysicalDeviceRayTracingPipelineFeaturesKHR *in = (const VkPhysicalDeviceRayTracingPipelineFeaturesKHR *)in_header;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->rayTracingPipeline = in->rayTracingPipeline;
            out->rayTracingPipelineShaderGroupHandleCaptureReplay = in->rayTracingPipelineShaderGroupHandleCaptureReplay;
            out->rayTracingPipelineShaderGroupHandleCaptureReplayMixed = in->rayTracingPipelineShaderGroupHandleCaptureReplayMixed;
            out->rayTracingPipelineTraceRaysIndirect = in->rayTracingPipelineTraceRaysIndirect;
            out->rayTraversalPrimitiveCulling = in->rayTraversalPrimitiveCulling;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR:
        {
            const VkPhysicalDeviceRayQueryFeaturesKHR *in = (const VkPhysicalDeviceRayQueryFeaturesKHR *)in_header;
            VkPhysicalDeviceRayQueryFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->rayQuery = in->rayQuery;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR:
        {
            const VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR *in = (const VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR *)in_header;
            VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->rayTracingMaintenance1 = in->rayTracingMaintenance1;
            out->rayTracingPipelineTraceRaysIndirect2 = in->rayTracingPipelineTraceRaysIndirect2;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD:
        {
            const VkDeviceMemoryOverallocationCreateInfoAMD *in = (const VkDeviceMemoryOverallocationCreateInfoAMD *)in_header;
            VkDeviceMemoryOverallocationCreateInfoAMD *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentDensityMapDeferred = in->fragmentDensityMapDeferred;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM:
        {
            const VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM *in = (const VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM *)in_header;
            VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentDensityMapOffset = in->fragmentDensityMapOffset;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES:
        {
            const VkPhysicalDeviceScalarBlockLayoutFeatures *in = (const VkPhysicalDeviceScalarBlockLayoutFeatures *)in_header;
            VkPhysicalDeviceScalarBlockLayoutFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->memoryPriority = in->memoryPriority;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT:
        {
            const VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT *in = (const VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT *)in_header;
            VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pageableDeviceLocalMemory = in->pageableDeviceLocalMemory;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES:
        {
            const VkPhysicalDeviceBufferDeviceAddressFeatures *in = (const VkPhysicalDeviceBufferDeviceAddressFeatures *)in_header;
            VkPhysicalDeviceBufferDeviceAddressFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->imagelessFramebuffer = in->imagelessFramebuffer;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES:
        {
            const VkPhysicalDeviceTextureCompressionASTCHDRFeatures *in = (const VkPhysicalDeviceTextureCompressionASTCHDRFeatures *)in_header;
            VkPhysicalDeviceTextureCompressionASTCHDRFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->separateDepthStencilLayouts = in->separateDepthStencilLayouts;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT:
        {
            const VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT *in = (const VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT *)in_header;
            VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->primitiveTopologyListRestart = in->primitiveTopologyListRestart;
            out->primitiveTopologyPatchListRestart = in->primitiveTopologyPatchListRestart;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR:
        {
            const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *in = (const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *)in_header;
            VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelineExecutableInfo = in->pipelineExecutableInfo;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES:
        {
            const VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures *in = (const VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures *)in_header;
            VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->texelBufferAlignment = in->texelBufferAlignment;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES:
        {
            const VkPhysicalDeviceSubgroupSizeControlFeatures *in = (const VkPhysicalDeviceSubgroupSizeControlFeatures *)in_header;
            VkPhysicalDeviceSubgroupSizeControlFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES:
        {
            const VkPhysicalDevicePipelineCreationCacheControlFeatures *in = (const VkPhysicalDevicePipelineCreationCacheControlFeatures *)in_header;
            VkPhysicalDevicePipelineCreationCacheControlFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES:
        {
            const VkPhysicalDeviceVulkan13Features *in = (const VkPhysicalDeviceVulkan13Features *)in_header;
            VkPhysicalDeviceVulkan13Features *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->robustImageAccess = in->robustImageAccess;
            out->inlineUniformBlock = in->inlineUniformBlock;
            out->descriptorBindingInlineUniformBlockUpdateAfterBind = in->descriptorBindingInlineUniformBlockUpdateAfterBind;
            out->pipelineCreationCacheControl = in->pipelineCreationCacheControl;
            out->privateData = in->privateData;
            out->shaderDemoteToHelperInvocation = in->shaderDemoteToHelperInvocation;
            out->shaderTerminateInvocation = in->shaderTerminateInvocation;
            out->subgroupSizeControl = in->subgroupSizeControl;
            out->computeFullSubgroups = in->computeFullSubgroups;
            out->synchronization2 = in->synchronization2;
            out->textureCompressionASTC_HDR = in->textureCompressionASTC_HDR;
            out->shaderZeroInitializeWorkgroupMemory = in->shaderZeroInitializeWorkgroupMemory;
            out->dynamicRendering = in->dynamicRendering;
            out->shaderIntegerDotProduct = in->shaderIntegerDotProduct;
            out->maintenance4 = in->maintenance4;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD:
        {
            const VkPhysicalDeviceCoherentMemoryFeaturesAMD *in = (const VkPhysicalDeviceCoherentMemoryFeaturesAMD *)in_header;
            VkPhysicalDeviceCoherentMemoryFeaturesAMD *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->customBorderColors = in->customBorderColors;
            out->customBorderColorWithoutFormat = in->customBorderColorWithoutFormat;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT:
        {
            const VkPhysicalDeviceBorderColorSwizzleFeaturesEXT *in = (const VkPhysicalDeviceBorderColorSwizzleFeaturesEXT *)in_header;
            VkPhysicalDeviceBorderColorSwizzleFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->borderColorSwizzle = in->borderColorSwizzle;
            out->borderColorSwizzleFromImage = in->borderColorSwizzleFromImage;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT:
        {
            const VkPhysicalDeviceExtendedDynamicStateFeaturesEXT *in = (const VkPhysicalDeviceExtendedDynamicStateFeaturesEXT *)in_header;
            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->extendedDynamicState = in->extendedDynamicState;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT:
        {
            const VkPhysicalDeviceExtendedDynamicState2FeaturesEXT *in = (const VkPhysicalDeviceExtendedDynamicState2FeaturesEXT *)in_header;
            VkPhysicalDeviceExtendedDynamicState2FeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->extendedDynamicState2 = in->extendedDynamicState2;
            out->extendedDynamicState2LogicOp = in->extendedDynamicState2LogicOp;
            out->extendedDynamicState2PatchControlPoints = in->extendedDynamicState2PatchControlPoints;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV:
        {
            const VkPhysicalDeviceDiagnosticsConfigFeaturesNV *in = (const VkPhysicalDeviceDiagnosticsConfigFeaturesNV *)in_header;
            VkPhysicalDeviceDiagnosticsConfigFeaturesNV *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->flags = in->flags;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES:
        {
            const VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures *in = (const VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures *)in_header;
            VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderZeroInitializeWorkgroupMemory = in->shaderZeroInitializeWorkgroupMemory;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR:
        {
            const VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR *in = (const VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR *)in_header;
            VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderSubgroupUniformControlFlow = in->shaderSubgroupUniformControlFlow;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT:
        {
            const VkPhysicalDeviceRobustness2FeaturesEXT *in = (const VkPhysicalDeviceRobustness2FeaturesEXT *)in_header;
            VkPhysicalDeviceRobustness2FeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->robustBufferAccess2 = in->robustBufferAccess2;
            out->robustImageAccess2 = in->robustImageAccess2;
            out->nullDescriptor = in->nullDescriptor;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES:
        {
            const VkPhysicalDeviceImageRobustnessFeatures *in = (const VkPhysicalDeviceImageRobustnessFeatures *)in_header;
            VkPhysicalDeviceImageRobustnessFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->robustImageAccess = in->robustImageAccess;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR:
        {
            const VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR *in = (const VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR *)in_header;
            VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->workgroupMemoryExplicitLayout = in->workgroupMemoryExplicitLayout;
            out->workgroupMemoryExplicitLayoutScalarBlockLayout = in->workgroupMemoryExplicitLayoutScalarBlockLayout;
            out->workgroupMemoryExplicitLayout8BitAccess = in->workgroupMemoryExplicitLayout8BitAccess;
            out->workgroupMemoryExplicitLayout16BitAccess = in->workgroupMemoryExplicitLayout16BitAccess;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT:
        {
            const VkPhysicalDevice4444FormatsFeaturesEXT *in = (const VkPhysicalDevice4444FormatsFeaturesEXT *)in_header;
            VkPhysicalDevice4444FormatsFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->formatA4R4G4B4 = in->formatA4R4G4B4;
            out->formatA4B4G4R4 = in->formatA4B4G4R4;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI:
        {
            const VkPhysicalDeviceSubpassShadingFeaturesHUAWEI *in = (const VkPhysicalDeviceSubpassShadingFeaturesHUAWEI *)in_header;
            VkPhysicalDeviceSubpassShadingFeaturesHUAWEI *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->subpassShading = in->subpassShading;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT:
        {
            const VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT *in = (const VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT *)in_header;
            VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelineFragmentShadingRate = in->pipelineFragmentShadingRate;
            out->primitiveFragmentShadingRate = in->primitiveFragmentShadingRate;
            out->attachmentFragmentShadingRate = in->attachmentFragmentShadingRate;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES:
        {
            const VkPhysicalDeviceShaderTerminateInvocationFeatures *in = (const VkPhysicalDeviceShaderTerminateInvocationFeatures *)in_header;
            VkPhysicalDeviceShaderTerminateInvocationFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentShadingRateEnums = in->fragmentShadingRateEnums;
            out->supersampleFragmentShadingRates = in->supersampleFragmentShadingRates;
            out->noInvocationFragmentShadingRates = in->noInvocationFragmentShadingRates;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT:
        {
            const VkPhysicalDeviceImage2DViewOf3DFeaturesEXT *in = (const VkPhysicalDeviceImage2DViewOf3DFeaturesEXT *)in_header;
            VkPhysicalDeviceImage2DViewOf3DFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->image2DViewOf3D = in->image2DViewOf3D;
            out->sampler2DViewOf3D = in->sampler2DViewOf3D;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE:
        {
            const VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE *in = (const VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE *)in_header;
            VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->mutableDescriptorType = in->mutableDescriptorType;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT:
        {
            const VkPhysicalDeviceDepthClipControlFeaturesEXT *in = (const VkPhysicalDeviceDepthClipControlFeaturesEXT *)in_header;
            VkPhysicalDeviceDepthClipControlFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->depthClipControl = in->depthClipControl;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT:
        {
            const VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT *in = (const VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT *)in_header;
            VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->vertexInputDynamicState = in->vertexInputDynamicState;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT:
        {
            const VkPhysicalDeviceColorWriteEnableFeaturesEXT *in = (const VkPhysicalDeviceColorWriteEnableFeaturesEXT *)in_header;
            VkPhysicalDeviceColorWriteEnableFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->colorWriteEnable = in->colorWriteEnable;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES:
        {
            const VkPhysicalDeviceSynchronization2Features *in = (const VkPhysicalDeviceSynchronization2Features *)in_header;
            VkPhysicalDeviceSynchronization2Features *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->synchronization2 = in->synchronization2;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT:
        {
            const VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT *in = (const VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT *)in_header;
            VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->primitivesGeneratedQuery = in->primitivesGeneratedQuery;
            out->primitivesGeneratedQueryWithRasterizerDiscard = in->primitivesGeneratedQueryWithRasterizerDiscard;
            out->primitivesGeneratedQueryWithNonZeroStreams = in->primitivesGeneratedQueryWithNonZeroStreams;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT:
        {
            const VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT *in = (const VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT *)in_header;
            VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->multisampledRenderToSingleSampled = in->multisampledRenderToSingleSampled;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV:
        {
            const VkPhysicalDeviceInheritedViewportScissorFeaturesNV *in = (const VkPhysicalDeviceInheritedViewportScissorFeaturesNV *)in_header;
            VkPhysicalDeviceInheritedViewportScissorFeaturesNV *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->inheritedViewportScissor2D = in->inheritedViewportScissor2D;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT:
        {
            const VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT *in = (const VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT *)in_header;
            VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->ycbcr2plane444Formats = in->ycbcr2plane444Formats;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT:
        {
            const VkPhysicalDeviceProvokingVertexFeaturesEXT *in = (const VkPhysicalDeviceProvokingVertexFeaturesEXT *)in_header;
            VkPhysicalDeviceProvokingVertexFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->provokingVertexLast = in->provokingVertexLast;
            out->transformFeedbackPreservesProvokingVertex = in->transformFeedbackPreservesProvokingVertex;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES:
        {
            const VkPhysicalDeviceShaderIntegerDotProductFeatures *in = (const VkPhysicalDeviceShaderIntegerDotProductFeatures *)in_header;
            VkPhysicalDeviceShaderIntegerDotProductFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderIntegerDotProduct = in->shaderIntegerDotProduct;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR:
        {
            const VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR *in = (const VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR *)in_header;
            VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->fragmentShaderBarycentric = in->fragmentShaderBarycentric;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV:
        {
            const VkPhysicalDeviceRayTracingMotionBlurFeaturesNV *in = (const VkPhysicalDeviceRayTracingMotionBlurFeaturesNV *)in_header;
            VkPhysicalDeviceRayTracingMotionBlurFeaturesNV *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->rayTracingMotionBlur = in->rayTracingMotionBlur;
            out->rayTracingMotionBlurPipelineTraceRaysIndirect = in->rayTracingMotionBlurPipelineTraceRaysIndirect;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT:
        {
            const VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT *in = (const VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT *)in_header;
            VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->formatRgba10x6WithoutYCbCrSampler = in->formatRgba10x6WithoutYCbCrSampler;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES:
        {
            const VkPhysicalDeviceDynamicRenderingFeatures *in = (const VkPhysicalDeviceDynamicRenderingFeatures *)in_header;
            VkPhysicalDeviceDynamicRenderingFeatures *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->dynamicRendering = in->dynamicRendering;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT:
        {
            const VkPhysicalDeviceImageViewMinLodFeaturesEXT *in = (const VkPhysicalDeviceImageViewMinLodFeaturesEXT *)in_header;
            VkPhysicalDeviceImageViewMinLodFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->minLod = in->minLod;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM:
        {
            const VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM *in = (const VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM *)in_header;
            VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->rasterizationOrderColorAttachmentAccess = in->rasterizationOrderColorAttachmentAccess;
            out->rasterizationOrderDepthAttachmentAccess = in->rasterizationOrderDepthAttachmentAccess;
            out->rasterizationOrderStencilAttachmentAccess = in->rasterizationOrderStencilAttachmentAccess;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV:
        {
            const VkPhysicalDeviceLinearColorAttachmentFeaturesNV *in = (const VkPhysicalDeviceLinearColorAttachmentFeaturesNV *)in_header;
            VkPhysicalDeviceLinearColorAttachmentFeaturesNV *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->linearColorAttachment = in->linearColorAttachment;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT:
        {
            const VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT *in = (const VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT *)in_header;
            VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->graphicsPipelineLibrary = in->graphicsPipelineLibrary;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_SET_HOST_MAPPING_FEATURES_VALVE:
        {
            const VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE *in = (const VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE *)in_header;
            VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->descriptorSetHostMapping = in->descriptorSetHostMapping;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT:
        {
            const VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT *in = (const VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT *)in_header;
            VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderModuleIdentifier = in->shaderModuleIdentifier;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_FEATURES_EXT:
        {
            const VkPhysicalDeviceImageCompressionControlFeaturesEXT *in = (const VkPhysicalDeviceImageCompressionControlFeaturesEXT *)in_header;
            VkPhysicalDeviceImageCompressionControlFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->imageCompressionControl = in->imageCompressionControl;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_FEATURES_EXT:
        {
            const VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT *in = (const VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT *)in_header;
            VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->imageCompressionControlSwapchain = in->imageCompressionControlSwapchain;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT:
        {
            const VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT *in = (const VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT *)in_header;
            VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->subpassMergeFeedback = in->subpassMergeFeedback;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROPERTIES_FEATURES_EXT:
        {
            const VkPhysicalDevicePipelinePropertiesFeaturesEXT *in = (const VkPhysicalDevicePipelinePropertiesFeaturesEXT *)in_header;
            VkPhysicalDevicePipelinePropertiesFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelinePropertiesIdentifier = in->pipelinePropertiesIdentifier;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EARLY_AND_LATE_FRAGMENT_TESTS_FEATURES_AMD:
        {
            const VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD *in = (const VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD *)in_header;
            VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->shaderEarlyAndLateFragmentTests = in->shaderEarlyAndLateFragmentTests;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT:
        {
            const VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT *in = (const VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT *)in_header;
            VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->nonSeamlessCubeMap = in->nonSeamlessCubeMap;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES_EXT:
        {
            const VkPhysicalDevicePipelineRobustnessFeaturesEXT *in = (const VkPhysicalDevicePipelineRobustnessFeaturesEXT *)in_header;
            VkPhysicalDevicePipelineRobustnessFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelineRobustness = in->pipelineRobustness;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_FEATURES_QCOM:
        {
            const VkPhysicalDeviceImageProcessingFeaturesQCOM *in = (const VkPhysicalDeviceImageProcessingFeaturesQCOM *)in_header;
            VkPhysicalDeviceImageProcessingFeaturesQCOM *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->textureSampleWeighted = in->textureSampleWeighted;
            out->textureBoxFilter = in->textureBoxFilter;
            out->textureBlockMatch = in->textureBlockMatch;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_PROPERTIES_FEATURES_QCOM:
        {
            const VkPhysicalDeviceTilePropertiesFeaturesQCOM *in = (const VkPhysicalDeviceTilePropertiesFeaturesQCOM *)in_header;
            VkPhysicalDeviceTilePropertiesFeaturesQCOM *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->tileProperties = in->tileProperties;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT:
        {
            const VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT *in = (const VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT *)in_header;
            VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

            out->sType = in->sType;
            out->pNext = NULL;
            out->attachmentFeedbackLoopLayout = in->attachmentFeedbackLoopLayout;

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

        switch (header->sType)
        {
            case VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO:
            {
                VkDeviceGroupDeviceCreateInfo *structure = (VkDeviceGroupDeviceCreateInfo *) header;
                free_VkPhysicalDevice_array((VkPhysicalDevice *)structure->pPhysicalDevices, structure->physicalDeviceCount);
                break;
            }
            default:
                break;
        }
        header = header->pNext;
        free(prev);
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
        case VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO:
            break;

        case VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT:
        {
            const VkDebugReportCallbackCreateInfoEXT *in = (const VkDebugReportCallbackCreateInfoEXT *)in_header;
            VkDebugReportCallbackCreateInfoEXT *out;

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

            if (!(out = malloc(sizeof(*out)))) goto out_of_memory;

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

        switch (header->sType)
        {
            default:
                break;
        }
        header = header->pNext;
        free(prev);
    }

    s->pNext = NULL;
}

static NTSTATUS wine_vkAcquireNextImage2KHR(void *args)
{
    struct vkAcquireNextImage2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAcquireNextImageInfoKHR_host pAcquireInfo_host;
    TRACE("%p, %p, %p\n", params->device, params->pAcquireInfo, params->pImageIndex);

    convert_VkAcquireNextImageInfoKHR_win_to_host(params->pAcquireInfo, &pAcquireInfo_host);
    result = params->device->funcs.p_vkAcquireNextImage2KHR(params->device->device, &pAcquireInfo_host, params->pImageIndex);

    return result;
#else
    TRACE("%p, %p, %p\n", params->device, params->pAcquireInfo, params->pImageIndex);
    return params->device->funcs.p_vkAcquireNextImage2KHR(params->device->device, params->pAcquireInfo, params->pImageIndex);
#endif
}

static NTSTATUS wine_vkAcquireNextImageKHR(void *args)
{
    struct vkAcquireNextImageKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), wine_dbgstr_longlong(params->timeout), wine_dbgstr_longlong(params->semaphore), wine_dbgstr_longlong(params->fence), params->pImageIndex);
    return params->device->funcs.p_vkAcquireNextImageKHR(params->device->device, params->swapchain, params->timeout, params->semaphore, params->fence, params->pImageIndex);
}

static NTSTATUS wine_vkAcquirePerformanceConfigurationINTEL(void *args)
{
    struct vkAcquirePerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAcquireInfo, params->pConfiguration);
    return params->device->funcs.p_vkAcquirePerformanceConfigurationINTEL(params->device->device, params->pAcquireInfo, params->pConfiguration);
}

static NTSTATUS wine_vkAcquireProfilingLockKHR(void *args)
{
    struct vkAcquireProfilingLockKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAcquireProfilingLockInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkAcquireProfilingLockInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    result = params->device->funcs.p_vkAcquireProfilingLockKHR(params->device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    return params->device->funcs.p_vkAcquireProfilingLockKHR(params->device->device, params->pInfo);
#endif
}

static NTSTATUS wine_vkAllocateDescriptorSets(void *args)
{
    struct vkAllocateDescriptorSets_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDescriptorSetAllocateInfo_host pAllocateInfo_host;
    TRACE("%p, %p, %p\n", params->device, params->pAllocateInfo, params->pDescriptorSets);

    convert_VkDescriptorSetAllocateInfo_win_to_host(params->pAllocateInfo, &pAllocateInfo_host);
    result = params->device->funcs.p_vkAllocateDescriptorSets(params->device->device, &pAllocateInfo_host, params->pDescriptorSets);

    return result;
#else
    TRACE("%p, %p, %p\n", params->device, params->pAllocateInfo, params->pDescriptorSets);
    return params->device->funcs.p_vkAllocateDescriptorSets(params->device->device, params->pAllocateInfo, params->pDescriptorSets);
#endif
}

static NTSTATUS wine_vkAllocateMemory(void *args)
{
    struct vkAllocateMemory_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkMemoryAllocateInfo_host pAllocateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pAllocateInfo, params->pAllocator, params->pMemory);

    convert_VkMemoryAllocateInfo_win_to_host(params->pAllocateInfo, &pAllocateInfo_host);
    result = params->device->funcs.p_vkAllocateMemory(params->device->device, &pAllocateInfo_host, NULL, params->pMemory);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pAllocateInfo, params->pAllocator, params->pMemory);
    return params->device->funcs.p_vkAllocateMemory(params->device->device, params->pAllocateInfo, NULL, params->pMemory);
#endif
}

static NTSTATUS wine_vkBeginCommandBuffer(void *args)
{
    struct vkBeginCommandBuffer_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkCommandBufferBeginInfo_host pBeginInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pBeginInfo);

    convert_VkCommandBufferBeginInfo_win_to_host(params->pBeginInfo, &pBeginInfo_host);
    result = params->commandBuffer->device->funcs.p_vkBeginCommandBuffer(params->commandBuffer->command_buffer, &pBeginInfo_host);

    free_VkCommandBufferBeginInfo(&pBeginInfo_host);
    return result;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pBeginInfo);
    return params->commandBuffer->device->funcs.p_vkBeginCommandBuffer(params->commandBuffer->command_buffer, params->pBeginInfo);
#endif
}

static NTSTATUS wine_vkBindAccelerationStructureMemoryNV(void *args)
{
    struct vkBindAccelerationStructureMemoryNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindAccelerationStructureMemoryInfoNV_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    pBindInfos_host = convert_VkBindAccelerationStructureMemoryInfoNV_array_win_to_host(params->pBindInfos, params->bindInfoCount);
    result = params->device->funcs.p_vkBindAccelerationStructureMemoryNV(params->device->device, params->bindInfoCount, pBindInfos_host);

    free_VkBindAccelerationStructureMemoryInfoNV_array(pBindInfos_host, params->bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);
    return params->device->funcs.p_vkBindAccelerationStructureMemoryNV(params->device->device, params->bindInfoCount, params->pBindInfos);
#endif
}

static NTSTATUS wine_vkBindBufferMemory(void *args)
{
    struct vkBindBufferMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->memoryOffset));
    return params->device->funcs.p_vkBindBufferMemory(params->device->device, params->buffer, params->memory, params->memoryOffset);
}

static NTSTATUS wine_vkBindBufferMemory2(void *args)
{
    struct vkBindBufferMemory2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindBufferMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    pBindInfos_host = convert_VkBindBufferMemoryInfo_array_win_to_host(params->pBindInfos, params->bindInfoCount);
    result = params->device->funcs.p_vkBindBufferMemory2(params->device->device, params->bindInfoCount, pBindInfos_host);

    free_VkBindBufferMemoryInfo_array(pBindInfos_host, params->bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);
    return params->device->funcs.p_vkBindBufferMemory2(params->device->device, params->bindInfoCount, params->pBindInfos);
#endif
}

static NTSTATUS wine_vkBindBufferMemory2KHR(void *args)
{
    struct vkBindBufferMemory2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindBufferMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    pBindInfos_host = convert_VkBindBufferMemoryInfo_array_win_to_host(params->pBindInfos, params->bindInfoCount);
    result = params->device->funcs.p_vkBindBufferMemory2KHR(params->device->device, params->bindInfoCount, pBindInfos_host);

    free_VkBindBufferMemoryInfo_array(pBindInfos_host, params->bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);
    return params->device->funcs.p_vkBindBufferMemory2KHR(params->device->device, params->bindInfoCount, params->pBindInfos);
#endif
}

static NTSTATUS wine_vkBindImageMemory(void *args)
{
    struct vkBindImageMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->image), wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->memoryOffset));
    return params->device->funcs.p_vkBindImageMemory(params->device->device, params->image, params->memory, params->memoryOffset);
}

static NTSTATUS wine_vkBindImageMemory2(void *args)
{
    struct vkBindImageMemory2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindImageMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    pBindInfos_host = convert_VkBindImageMemoryInfo_array_win_to_host(params->pBindInfos, params->bindInfoCount);
    result = params->device->funcs.p_vkBindImageMemory2(params->device->device, params->bindInfoCount, pBindInfos_host);

    free_VkBindImageMemoryInfo_array(pBindInfos_host, params->bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);
    return params->device->funcs.p_vkBindImageMemory2(params->device->device, params->bindInfoCount, params->pBindInfos);
#endif
}

static NTSTATUS wine_vkBindImageMemory2KHR(void *args)
{
    struct vkBindImageMemory2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindImageMemoryInfo_host *pBindInfos_host;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    pBindInfos_host = convert_VkBindImageMemoryInfo_array_win_to_host(params->pBindInfos, params->bindInfoCount);
    result = params->device->funcs.p_vkBindImageMemory2KHR(params->device->device, params->bindInfoCount, pBindInfos_host);

    free_VkBindImageMemoryInfo_array(pBindInfos_host, params->bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);
    return params->device->funcs.p_vkBindImageMemory2KHR(params->device->device, params->bindInfoCount, params->pBindInfos);
#endif
}

static NTSTATUS wine_vkBuildAccelerationStructuresKHR(void *args)
{
    struct vkBuildAccelerationStructuresKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAccelerationStructureBuildGeometryInfoKHR_host *pInfos_host;
    TRACE("%p, 0x%s, %u, %p, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->infoCount, params->pInfos, params->ppBuildRangeInfos);

    pInfos_host = convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win_to_host(params->pInfos, params->infoCount);
    result = params->device->funcs.p_vkBuildAccelerationStructuresKHR(params->device->device, params->deferredOperation, params->infoCount, pInfos_host, params->ppBuildRangeInfos);

    free_VkAccelerationStructureBuildGeometryInfoKHR_array(pInfos_host, params->infoCount);
    return result;
#else
    TRACE("%p, 0x%s, %u, %p, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->infoCount, params->pInfos, params->ppBuildRangeInfos);
    return params->device->funcs.p_vkBuildAccelerationStructuresKHR(params->device->device, params->deferredOperation, params->infoCount, params->pInfos, params->ppBuildRangeInfos);
#endif
}

static NTSTATUS wine_vkCmdBeginConditionalRenderingEXT(void *args)
{
    struct vkCmdBeginConditionalRenderingEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkConditionalRenderingBeginInfoEXT_host pConditionalRenderingBegin_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pConditionalRenderingBegin);

    convert_VkConditionalRenderingBeginInfoEXT_win_to_host(params->pConditionalRenderingBegin, &pConditionalRenderingBegin_host);
    params->commandBuffer->device->funcs.p_vkCmdBeginConditionalRenderingEXT(params->commandBuffer->command_buffer, &pConditionalRenderingBegin_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pConditionalRenderingBegin);
    params->commandBuffer->device->funcs.p_vkCmdBeginConditionalRenderingEXT(params->commandBuffer->command_buffer, params->pConditionalRenderingBegin);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBeginDebugUtilsLabelEXT(void *args)
{
    struct vkCmdBeginDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pLabelInfo);
    params->commandBuffer->device->funcs.p_vkCmdBeginDebugUtilsLabelEXT(params->commandBuffer->command_buffer, params->pLabelInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBeginQuery(void *args)
{
    struct vkCmdBeginQuery_params *params = args;
    TRACE("%p, 0x%s, %u, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->flags);
    params->commandBuffer->device->funcs.p_vkCmdBeginQuery(params->commandBuffer->command_buffer, params->queryPool, params->query, params->flags);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBeginQueryIndexedEXT(void *args)
{
    struct vkCmdBeginQueryIndexedEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %#x, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->flags, params->index);
    params->commandBuffer->device->funcs.p_vkCmdBeginQueryIndexedEXT(params->commandBuffer->command_buffer, params->queryPool, params->query, params->flags, params->index);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBeginRenderPass(void *args)
{
    struct vkCmdBeginRenderPass_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %#x\n", params->commandBuffer, params->pRenderPassBegin, params->contents);

    convert_VkRenderPassBeginInfo_win_to_host(params->pRenderPassBegin, &pRenderPassBegin_host);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderPass(params->commandBuffer->command_buffer, &pRenderPassBegin_host, params->contents);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %#x\n", params->commandBuffer, params->pRenderPassBegin, params->contents);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderPass(params->commandBuffer->command_buffer, params->pRenderPassBegin, params->contents);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBeginRenderPass2(void *args)
{
    struct vkCmdBeginRenderPass2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);

    convert_VkRenderPassBeginInfo_win_to_host(params->pRenderPassBegin, &pRenderPassBegin_host);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderPass2(params->commandBuffer->command_buffer, &pRenderPassBegin_host, params->pSubpassBeginInfo);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderPass2(params->commandBuffer->command_buffer, params->pRenderPassBegin, params->pSubpassBeginInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBeginRenderPass2KHR(void *args)
{
    struct vkCmdBeginRenderPass2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);

    convert_VkRenderPassBeginInfo_win_to_host(params->pRenderPassBegin, &pRenderPassBegin_host);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderPass2KHR(params->commandBuffer->command_buffer, &pRenderPassBegin_host, params->pSubpassBeginInfo);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderPass2KHR(params->commandBuffer->command_buffer, params->pRenderPassBegin, params->pSubpassBeginInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBeginRendering(void *args)
{
    struct vkCmdBeginRendering_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkRenderingInfo_host pRenderingInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);

    convert_VkRenderingInfo_win_to_host(params->pRenderingInfo, &pRenderingInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdBeginRendering(params->commandBuffer->command_buffer, &pRenderingInfo_host);

    free_VkRenderingInfo(&pRenderingInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);
    params->commandBuffer->device->funcs.p_vkCmdBeginRendering(params->commandBuffer->command_buffer, params->pRenderingInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBeginRenderingKHR(void *args)
{
    struct vkCmdBeginRenderingKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkRenderingInfo_host pRenderingInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);

    convert_VkRenderingInfo_win_to_host(params->pRenderingInfo, &pRenderingInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderingKHR(params->commandBuffer->command_buffer, &pRenderingInfo_host);

    free_VkRenderingInfo(&pRenderingInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);
    params->commandBuffer->device->funcs.p_vkCmdBeginRenderingKHR(params->commandBuffer->command_buffer, params->pRenderingInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBeginTransformFeedbackEXT(void *args)
{
    struct vkCmdBeginTransformFeedbackEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    params->commandBuffer->device->funcs.p_vkCmdBeginTransformFeedbackEXT(params->commandBuffer->command_buffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindDescriptorSets(void *args)
{
    struct vkCmdBindDescriptorSets_params *params = args;
    TRACE("%p, %#x, 0x%s, %u, %u, %p, %u, %p\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->layout), params->firstSet, params->descriptorSetCount, params->pDescriptorSets, params->dynamicOffsetCount, params->pDynamicOffsets);
    params->commandBuffer->device->funcs.p_vkCmdBindDescriptorSets(params->commandBuffer->command_buffer, params->pipelineBindPoint, params->layout, params->firstSet, params->descriptorSetCount, params->pDescriptorSets, params->dynamicOffsetCount, params->pDynamicOffsets);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindIndexBuffer(void *args)
{
    struct vkCmdBindIndexBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->indexType);
    params->commandBuffer->device->funcs.p_vkCmdBindIndexBuffer(params->commandBuffer->command_buffer, params->buffer, params->offset, params->indexType);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindInvocationMaskHUAWEI(void *args)
{
    struct vkCmdBindInvocationMaskHUAWEI_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->imageView), params->imageLayout);
    params->commandBuffer->device->funcs.p_vkCmdBindInvocationMaskHUAWEI(params->commandBuffer->command_buffer, params->imageView, params->imageLayout);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindPipeline(void *args)
{
    struct vkCmdBindPipeline_params *params = args;
    TRACE("%p, %#x, 0x%s\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->pipeline));
    params->commandBuffer->device->funcs.p_vkCmdBindPipeline(params->commandBuffer->command_buffer, params->pipelineBindPoint, params->pipeline);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindPipelineShaderGroupNV(void *args)
{
    struct vkCmdBindPipelineShaderGroupNV_params *params = args;
    TRACE("%p, %#x, 0x%s, %u\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->pipeline), params->groupIndex);
    params->commandBuffer->device->funcs.p_vkCmdBindPipelineShaderGroupNV(params->commandBuffer->command_buffer, params->pipelineBindPoint, params->pipeline, params->groupIndex);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindShadingRateImageNV(void *args)
{
    struct vkCmdBindShadingRateImageNV_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->imageView), params->imageLayout);
    params->commandBuffer->device->funcs.p_vkCmdBindShadingRateImageNV(params->commandBuffer->command_buffer, params->imageView, params->imageLayout);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindTransformFeedbackBuffersEXT(void *args)
{
    struct vkCmdBindTransformFeedbackBuffersEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes);
    params->commandBuffer->device->funcs.p_vkCmdBindTransformFeedbackBuffersEXT(params->commandBuffer->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindVertexBuffers(void *args)
{
    struct vkCmdBindVertexBuffers_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets);
    params->commandBuffer->device->funcs.p_vkCmdBindVertexBuffers(params->commandBuffer->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindVertexBuffers2(void *args)
{
    struct vkCmdBindVertexBuffers2_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    params->commandBuffer->device->funcs.p_vkCmdBindVertexBuffers2(params->commandBuffer->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBindVertexBuffers2EXT(void *args)
{
    struct vkCmdBindVertexBuffers2EXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    params->commandBuffer->device->funcs.p_vkCmdBindVertexBuffers2EXT(params->commandBuffer->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBlitImage(void *args)
{
    struct vkCmdBlitImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions, params->filter);
    params->commandBuffer->device->funcs.p_vkCmdBlitImage(params->commandBuffer->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions, params->filter);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdBlitImage2(void *args)
{
    struct vkCmdBlitImage2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBlitImageInfo2_host pBlitImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);

    convert_VkBlitImageInfo2_win_to_host(params->pBlitImageInfo, &pBlitImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdBlitImage2(params->commandBuffer->command_buffer, &pBlitImageInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdBlitImage2(params->commandBuffer->command_buffer, params->pBlitImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBlitImage2KHR(void *args)
{
    struct vkCmdBlitImage2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBlitImageInfo2_host pBlitImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);

    convert_VkBlitImageInfo2_win_to_host(params->pBlitImageInfo, &pBlitImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdBlitImage2KHR(params->commandBuffer->command_buffer, &pBlitImageInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdBlitImage2KHR(params->commandBuffer->command_buffer, params->pBlitImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBuildAccelerationStructureNV(void *args)
{
    struct vkCmdBuildAccelerationStructureNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureInfoNV_host pInfo_host;
    TRACE("%p, %p, 0x%s, 0x%s, %u, 0x%s, 0x%s, 0x%s, 0x%s\n", params->commandBuffer, params->pInfo, wine_dbgstr_longlong(params->instanceData), wine_dbgstr_longlong(params->instanceOffset), params->update, wine_dbgstr_longlong(params->dst), wine_dbgstr_longlong(params->src), wine_dbgstr_longlong(params->scratch), wine_dbgstr_longlong(params->scratchOffset));

    convert_VkAccelerationStructureInfoNV_win_to_host(params->pInfo, &pInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructureNV(params->commandBuffer->command_buffer, &pInfo_host, params->instanceData, params->instanceOffset, params->update, params->dst, params->src, params->scratch, params->scratchOffset);

    free_VkAccelerationStructureInfoNV(&pInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, 0x%s, 0x%s, %u, 0x%s, 0x%s, 0x%s, 0x%s\n", params->commandBuffer, params->pInfo, wine_dbgstr_longlong(params->instanceData), wine_dbgstr_longlong(params->instanceOffset), params->update, wine_dbgstr_longlong(params->dst), wine_dbgstr_longlong(params->src), wine_dbgstr_longlong(params->scratch), wine_dbgstr_longlong(params->scratchOffset));
    params->commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructureNV(params->commandBuffer->command_buffer, params->pInfo, params->instanceData, params->instanceOffset, params->update, params->dst, params->src, params->scratch, params->scratchOffset);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBuildAccelerationStructuresIndirectKHR(void *args)
{
    struct vkCmdBuildAccelerationStructuresIndirectKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureBuildGeometryInfoKHR_host *pInfos_host;
    TRACE("%p, %u, %p, %p, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);

    pInfos_host = convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win_to_host(params->pInfos, params->infoCount);
    params->commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructuresIndirectKHR(params->commandBuffer->command_buffer, params->infoCount, pInfos_host, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);

    free_VkAccelerationStructureBuildGeometryInfoKHR_array(pInfos_host, params->infoCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %u, %p, %p, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);
    params->commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructuresIndirectKHR(params->commandBuffer->command_buffer, params->infoCount, params->pInfos, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdBuildAccelerationStructuresKHR(void *args)
{
    struct vkCmdBuildAccelerationStructuresKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureBuildGeometryInfoKHR_host *pInfos_host;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->ppBuildRangeInfos);

    pInfos_host = convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win_to_host(params->pInfos, params->infoCount);
    params->commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructuresKHR(params->commandBuffer->command_buffer, params->infoCount, pInfos_host, params->ppBuildRangeInfos);

    free_VkAccelerationStructureBuildGeometryInfoKHR_array(pInfos_host, params->infoCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->ppBuildRangeInfos);
    params->commandBuffer->device->funcs.p_vkCmdBuildAccelerationStructuresKHR(params->commandBuffer->command_buffer, params->infoCount, params->pInfos, params->ppBuildRangeInfos);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdClearAttachments(void *args)
{
    struct vkCmdClearAttachments_params *params = args;
    TRACE("%p, %u, %p, %u, %p\n", params->commandBuffer, params->attachmentCount, params->pAttachments, params->rectCount, params->pRects);
    params->commandBuffer->device->funcs.p_vkCmdClearAttachments(params->commandBuffer->command_buffer, params->attachmentCount, params->pAttachments, params->rectCount, params->pRects);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdClearColorImage(void *args)
{
    struct vkCmdClearColorImage_params *params = args;
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->image), params->imageLayout, params->pColor, params->rangeCount, params->pRanges);
    params->commandBuffer->device->funcs.p_vkCmdClearColorImage(params->commandBuffer->command_buffer, params->image, params->imageLayout, params->pColor, params->rangeCount, params->pRanges);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdClearDepthStencilImage(void *args)
{
    struct vkCmdClearDepthStencilImage_params *params = args;
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->image), params->imageLayout, params->pDepthStencil, params->rangeCount, params->pRanges);
    params->commandBuffer->device->funcs.p_vkCmdClearDepthStencilImage(params->commandBuffer->command_buffer, params->image, params->imageLayout, params->pDepthStencil, params->rangeCount, params->pRanges);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdCopyAccelerationStructureKHR(void *args)
{
    struct vkCmdCopyAccelerationStructureKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyAccelerationStructureInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyAccelerationStructureKHR(params->commandBuffer->command_buffer, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyAccelerationStructureKHR(params->commandBuffer->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyAccelerationStructureNV(void *args)
{
    struct vkCmdCopyAccelerationStructureNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->dst), wine_dbgstr_longlong(params->src), params->mode);
    params->commandBuffer->device->funcs.p_vkCmdCopyAccelerationStructureNV(params->commandBuffer->command_buffer, params->dst, params->src, params->mode);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdCopyAccelerationStructureToMemoryKHR(void *args)
{
    struct vkCmdCopyAccelerationStructureToMemoryKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyAccelerationStructureToMemoryInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyAccelerationStructureToMemoryInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyAccelerationStructureToMemoryKHR(params->commandBuffer->command_buffer, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyAccelerationStructureToMemoryKHR(params->commandBuffer->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyBuffer(void *args)
{
    struct vkCmdCopyBuffer_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferCopy_host *pRegions_host;
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);

    pRegions_host = convert_VkBufferCopy_array_win_to_host(params->pRegions, params->regionCount);
    params->commandBuffer->device->funcs.p_vkCmdCopyBuffer(params->commandBuffer->command_buffer, params->srcBuffer, params->dstBuffer, params->regionCount, pRegions_host);

    free_VkBufferCopy_array(pRegions_host, params->regionCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);
    params->commandBuffer->device->funcs.p_vkCmdCopyBuffer(params->commandBuffer->command_buffer, params->srcBuffer, params->dstBuffer, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyBuffer2(void *args)
{
    struct vkCmdCopyBuffer2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyBufferInfo2_host pCopyBufferInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);

    convert_VkCopyBufferInfo2_win_to_host(params->pCopyBufferInfo, &pCopyBufferInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyBuffer2(params->commandBuffer->command_buffer, &pCopyBufferInfo_host);

    free_VkCopyBufferInfo2(&pCopyBufferInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyBuffer2(params->commandBuffer->command_buffer, params->pCopyBufferInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyBuffer2KHR(void *args)
{
    struct vkCmdCopyBuffer2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyBufferInfo2_host pCopyBufferInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);

    convert_VkCopyBufferInfo2_win_to_host(params->pCopyBufferInfo, &pCopyBufferInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyBuffer2KHR(params->commandBuffer->command_buffer, &pCopyBufferInfo_host);

    free_VkCopyBufferInfo2(&pCopyBufferInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyBuffer2KHR(params->commandBuffer->command_buffer, params->pCopyBufferInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyBufferToImage(void *args)
{
    struct vkCmdCopyBufferToImage_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferImageCopy_host *pRegions_host;
    TRACE("%p, 0x%s, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);

    pRegions_host = convert_VkBufferImageCopy_array_win_to_host(params->pRegions, params->regionCount);
    params->commandBuffer->device->funcs.p_vkCmdCopyBufferToImage(params->commandBuffer->command_buffer, params->srcBuffer, params->dstImage, params->dstImageLayout, params->regionCount, pRegions_host);

    free_VkBufferImageCopy_array(pRegions_host, params->regionCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);
    params->commandBuffer->device->funcs.p_vkCmdCopyBufferToImage(params->commandBuffer->command_buffer, params->srcBuffer, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyBufferToImage2(void *args)
{
    struct vkCmdCopyBufferToImage2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyBufferToImageInfo2_host pCopyBufferToImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);

    convert_VkCopyBufferToImageInfo2_win_to_host(params->pCopyBufferToImageInfo, &pCopyBufferToImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyBufferToImage2(params->commandBuffer->command_buffer, &pCopyBufferToImageInfo_host);

    free_VkCopyBufferToImageInfo2(&pCopyBufferToImageInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyBufferToImage2(params->commandBuffer->command_buffer, params->pCopyBufferToImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyBufferToImage2KHR(void *args)
{
    struct vkCmdCopyBufferToImage2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyBufferToImageInfo2_host pCopyBufferToImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);

    convert_VkCopyBufferToImageInfo2_win_to_host(params->pCopyBufferToImageInfo, &pCopyBufferToImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyBufferToImage2KHR(params->commandBuffer->command_buffer, &pCopyBufferToImageInfo_host);

    free_VkCopyBufferToImageInfo2(&pCopyBufferToImageInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyBufferToImage2KHR(params->commandBuffer->command_buffer, params->pCopyBufferToImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyImage(void *args)
{
    struct vkCmdCopyImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);
    params->commandBuffer->device->funcs.p_vkCmdCopyImage(params->commandBuffer->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdCopyImage2(void *args)
{
    struct vkCmdCopyImage2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyImageInfo2_host pCopyImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);

    convert_VkCopyImageInfo2_win_to_host(params->pCopyImageInfo, &pCopyImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyImage2(params->commandBuffer->command_buffer, &pCopyImageInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyImage2(params->commandBuffer->command_buffer, params->pCopyImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyImage2KHR(void *args)
{
    struct vkCmdCopyImage2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyImageInfo2_host pCopyImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);

    convert_VkCopyImageInfo2_win_to_host(params->pCopyImageInfo, &pCopyImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyImage2KHR(params->commandBuffer->command_buffer, &pCopyImageInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyImage2KHR(params->commandBuffer->command_buffer, params->pCopyImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyImageToBuffer(void *args)
{
    struct vkCmdCopyImageToBuffer_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferImageCopy_host *pRegions_host;
    TRACE("%p, 0x%s, %#x, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);

    pRegions_host = convert_VkBufferImageCopy_array_win_to_host(params->pRegions, params->regionCount);
    params->commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer(params->commandBuffer->command_buffer, params->srcImage, params->srcImageLayout, params->dstBuffer, params->regionCount, pRegions_host);

    free_VkBufferImageCopy_array(pRegions_host, params->regionCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, %#x, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);
    params->commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer(params->commandBuffer->command_buffer, params->srcImage, params->srcImageLayout, params->dstBuffer, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyImageToBuffer2(void *args)
{
    struct vkCmdCopyImageToBuffer2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyImageToBufferInfo2_host pCopyImageToBufferInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);

    convert_VkCopyImageToBufferInfo2_win_to_host(params->pCopyImageToBufferInfo, &pCopyImageToBufferInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer2(params->commandBuffer->command_buffer, &pCopyImageToBufferInfo_host);

    free_VkCopyImageToBufferInfo2(&pCopyImageToBufferInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer2(params->commandBuffer->command_buffer, params->pCopyImageToBufferInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyImageToBuffer2KHR(void *args)
{
    struct vkCmdCopyImageToBuffer2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyImageToBufferInfo2_host pCopyImageToBufferInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);

    convert_VkCopyImageToBufferInfo2_win_to_host(params->pCopyImageToBufferInfo, &pCopyImageToBufferInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer2KHR(params->commandBuffer->command_buffer, &pCopyImageToBufferInfo_host);

    free_VkCopyImageToBufferInfo2(&pCopyImageToBufferInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyImageToBuffer2KHR(params->commandBuffer->command_buffer, params->pCopyImageToBufferInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyMemoryToAccelerationStructureKHR(void *args)
{
    struct vkCmdCopyMemoryToAccelerationStructureKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCopyMemoryToAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyMemoryToAccelerationStructureInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCopyMemoryToAccelerationStructureKHR(params->commandBuffer->command_buffer, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);
    params->commandBuffer->device->funcs.p_vkCmdCopyMemoryToAccelerationStructureKHR(params->commandBuffer->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdCopyQueryPoolResults(void *args)
{
    struct vkCmdCopyQueryPoolResults_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->stride), params->flags);
    params->commandBuffer->device->funcs.p_vkCmdCopyQueryPoolResults(params->commandBuffer->command_buffer, params->queryPool, params->firstQuery, params->queryCount, params->dstBuffer, params->dstOffset, params->stride, params->flags);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdCuLaunchKernelNVX(void *args)
{
    struct vkCmdCuLaunchKernelNVX_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkCuLaunchInfoNVX_host pLaunchInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pLaunchInfo);

    convert_VkCuLaunchInfoNVX_win_to_host(params->pLaunchInfo, &pLaunchInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdCuLaunchKernelNVX(params->commandBuffer->command_buffer, &pLaunchInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pLaunchInfo);
    params->commandBuffer->device->funcs.p_vkCmdCuLaunchKernelNVX(params->commandBuffer->command_buffer, params->pLaunchInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdDebugMarkerBeginEXT(void *args)
{
    struct vkCmdDebugMarkerBeginEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);
    params->commandBuffer->device->funcs.p_vkCmdDebugMarkerBeginEXT(params->commandBuffer->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDebugMarkerEndEXT(void *args)
{
    struct vkCmdDebugMarkerEndEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    params->commandBuffer->device->funcs.p_vkCmdDebugMarkerEndEXT(params->commandBuffer->command_buffer);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDebugMarkerInsertEXT(void *args)
{
    struct vkCmdDebugMarkerInsertEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);
    params->commandBuffer->device->funcs.p_vkCmdDebugMarkerInsertEXT(params->commandBuffer->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDispatch(void *args)
{
    struct vkCmdDispatch_params *params = args;
    TRACE("%p, %u, %u, %u\n", params->commandBuffer, params->groupCountX, params->groupCountY, params->groupCountZ);
    params->commandBuffer->device->funcs.p_vkCmdDispatch(params->commandBuffer->command_buffer, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDispatchBase(void *args)
{
    struct vkCmdDispatchBase_params *params = args;
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", params->commandBuffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    params->commandBuffer->device->funcs.p_vkCmdDispatchBase(params->commandBuffer->command_buffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDispatchBaseKHR(void *args)
{
    struct vkCmdDispatchBaseKHR_params *params = args;
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", params->commandBuffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    params->commandBuffer->device->funcs.p_vkCmdDispatchBaseKHR(params->commandBuffer->command_buffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDispatchIndirect(void *args)
{
    struct vkCmdDispatchIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset));
    params->commandBuffer->device->funcs.p_vkCmdDispatchIndirect(params->commandBuffer->command_buffer, params->buffer, params->offset);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDraw(void *args)
{
    struct vkCmdDraw_params *params = args;
    TRACE("%p, %u, %u, %u, %u\n", params->commandBuffer, params->vertexCount, params->instanceCount, params->firstVertex, params->firstInstance);
    params->commandBuffer->device->funcs.p_vkCmdDraw(params->commandBuffer->command_buffer, params->vertexCount, params->instanceCount, params->firstVertex, params->firstInstance);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndexed(void *args)
{
    struct vkCmdDrawIndexed_params *params = args;
    TRACE("%p, %u, %u, %u, %d, %u\n", params->commandBuffer, params->indexCount, params->instanceCount, params->firstIndex, params->vertexOffset, params->firstInstance);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndexed(params->commandBuffer->command_buffer, params->indexCount, params->instanceCount, params->firstIndex, params->vertexOffset, params->firstInstance);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndexedIndirect(void *args)
{
    struct vkCmdDrawIndexedIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirect(params->commandBuffer->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndexedIndirectCount(void *args)
{
    struct vkCmdDrawIndexedIndirectCount_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirectCount(params->commandBuffer->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndexedIndirectCountAMD(void *args)
{
    struct vkCmdDrawIndexedIndirectCountAMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirectCountAMD(params->commandBuffer->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndexedIndirectCountKHR(void *args)
{
    struct vkCmdDrawIndexedIndirectCountKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndexedIndirectCountKHR(params->commandBuffer->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndirect(void *args)
{
    struct vkCmdDrawIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndirect(params->commandBuffer->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndirectByteCountEXT(void *args)
{
    struct vkCmdDrawIndirectByteCountEXT_params *params = args;
    TRACE("%p, %u, %u, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, params->instanceCount, params->firstInstance, wine_dbgstr_longlong(params->counterBuffer), wine_dbgstr_longlong(params->counterBufferOffset), params->counterOffset, params->vertexStride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndirectByteCountEXT(params->commandBuffer->command_buffer, params->instanceCount, params->firstInstance, params->counterBuffer, params->counterBufferOffset, params->counterOffset, params->vertexStride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndirectCount(void *args)
{
    struct vkCmdDrawIndirectCount_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndirectCount(params->commandBuffer->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndirectCountAMD(void *args)
{
    struct vkCmdDrawIndirectCountAMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndirectCountAMD(params->commandBuffer->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawIndirectCountKHR(void *args)
{
    struct vkCmdDrawIndirectCountKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawIndirectCountKHR(params->commandBuffer->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawMeshTasksIndirectCountNV(void *args)
{
    struct vkCmdDrawMeshTasksIndirectCountNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawMeshTasksIndirectCountNV(params->commandBuffer->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawMeshTasksIndirectNV(void *args)
{
    struct vkCmdDrawMeshTasksIndirectNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawMeshTasksIndirectNV(params->commandBuffer->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawMeshTasksNV(void *args)
{
    struct vkCmdDrawMeshTasksNV_params *params = args;
    TRACE("%p, %u, %u\n", params->commandBuffer, params->taskCount, params->firstTask);
    params->commandBuffer->device->funcs.p_vkCmdDrawMeshTasksNV(params->commandBuffer->command_buffer, params->taskCount, params->firstTask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawMultiEXT(void *args)
{
    struct vkCmdDrawMultiEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %u, %u\n", params->commandBuffer, params->drawCount, params->pVertexInfo, params->instanceCount, params->firstInstance, params->stride);
    params->commandBuffer->device->funcs.p_vkCmdDrawMultiEXT(params->commandBuffer->command_buffer, params->drawCount, params->pVertexInfo, params->instanceCount, params->firstInstance, params->stride);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdDrawMultiIndexedEXT(void *args)
{
    struct vkCmdDrawMultiIndexedEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %u, %u, %p\n", params->commandBuffer, params->drawCount, params->pIndexInfo, params->instanceCount, params->firstInstance, params->stride, params->pVertexOffset);
    params->commandBuffer->device->funcs.p_vkCmdDrawMultiIndexedEXT(params->commandBuffer->command_buffer, params->drawCount, params->pIndexInfo, params->instanceCount, params->firstInstance, params->stride, params->pVertexOffset);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndConditionalRenderingEXT(void *args)
{
    struct vkCmdEndConditionalRenderingEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    params->commandBuffer->device->funcs.p_vkCmdEndConditionalRenderingEXT(params->commandBuffer->command_buffer);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndDebugUtilsLabelEXT(void *args)
{
    struct vkCmdEndDebugUtilsLabelEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    params->commandBuffer->device->funcs.p_vkCmdEndDebugUtilsLabelEXT(params->commandBuffer->command_buffer);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndQuery(void *args)
{
    struct vkCmdEndQuery_params *params = args;
    TRACE("%p, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query);
    params->commandBuffer->device->funcs.p_vkCmdEndQuery(params->commandBuffer->command_buffer, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndQueryIndexedEXT(void *args)
{
    struct vkCmdEndQueryIndexedEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->index);
    params->commandBuffer->device->funcs.p_vkCmdEndQueryIndexedEXT(params->commandBuffer->command_buffer, params->queryPool, params->query, params->index);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndRenderPass(void *args)
{
    struct vkCmdEndRenderPass_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    params->commandBuffer->device->funcs.p_vkCmdEndRenderPass(params->commandBuffer->command_buffer);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndRenderPass2(void *args)
{
    struct vkCmdEndRenderPass2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSubpassEndInfo);
    params->commandBuffer->device->funcs.p_vkCmdEndRenderPass2(params->commandBuffer->command_buffer, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndRenderPass2KHR(void *args)
{
    struct vkCmdEndRenderPass2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSubpassEndInfo);
    params->commandBuffer->device->funcs.p_vkCmdEndRenderPass2KHR(params->commandBuffer->command_buffer, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndRendering(void *args)
{
    struct vkCmdEndRendering_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    params->commandBuffer->device->funcs.p_vkCmdEndRendering(params->commandBuffer->command_buffer);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndRenderingKHR(void *args)
{
    struct vkCmdEndRenderingKHR_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    params->commandBuffer->device->funcs.p_vkCmdEndRenderingKHR(params->commandBuffer->command_buffer);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdEndTransformFeedbackEXT(void *args)
{
    struct vkCmdEndTransformFeedbackEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    params->commandBuffer->device->funcs.p_vkCmdEndTransformFeedbackEXT(params->commandBuffer->command_buffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdExecuteCommands(void *args)
{
    struct vkCmdExecuteCommands_params *params = args;
    VkCommandBuffer *pCommandBuffers_host;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->commandBufferCount, params->pCommandBuffers);

    pCommandBuffers_host = convert_VkCommandBuffer_array_win_to_host(params->pCommandBuffers, params->commandBufferCount);
    params->commandBuffer->device->funcs.p_vkCmdExecuteCommands(params->commandBuffer->command_buffer, params->commandBufferCount, pCommandBuffers_host);

    free_VkCommandBuffer_array(pCommandBuffers_host, params->commandBufferCount);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdExecuteGeneratedCommandsNV(void *args)
{
    struct vkCmdExecuteGeneratedCommandsNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkGeneratedCommandsInfoNV_host pGeneratedCommandsInfo_host;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->isPreprocessed, params->pGeneratedCommandsInfo);

    convert_VkGeneratedCommandsInfoNV_win_to_host(params->pGeneratedCommandsInfo, &pGeneratedCommandsInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdExecuteGeneratedCommandsNV(params->commandBuffer->command_buffer, params->isPreprocessed, &pGeneratedCommandsInfo_host);

    free_VkGeneratedCommandsInfoNV(&pGeneratedCommandsInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %u, %p\n", params->commandBuffer, params->isPreprocessed, params->pGeneratedCommandsInfo);
    params->commandBuffer->device->funcs.p_vkCmdExecuteGeneratedCommandsNV(params->commandBuffer->command_buffer, params->isPreprocessed, params->pGeneratedCommandsInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdFillBuffer(void *args)
{
    struct vkCmdFillBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->size), params->data);
    params->commandBuffer->device->funcs.p_vkCmdFillBuffer(params->commandBuffer->command_buffer, params->dstBuffer, params->dstOffset, params->size, params->data);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdInsertDebugUtilsLabelEXT(void *args)
{
    struct vkCmdInsertDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pLabelInfo);
    params->commandBuffer->device->funcs.p_vkCmdInsertDebugUtilsLabelEXT(params->commandBuffer->command_buffer, params->pLabelInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdNextSubpass(void *args)
{
    struct vkCmdNextSubpass_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->contents);
    params->commandBuffer->device->funcs.p_vkCmdNextSubpass(params->commandBuffer->command_buffer, params->contents);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdNextSubpass2(void *args)
{
    struct vkCmdNextSubpass2_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    params->commandBuffer->device->funcs.p_vkCmdNextSubpass2(params->commandBuffer->command_buffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdNextSubpass2KHR(void *args)
{
    struct vkCmdNextSubpass2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    params->commandBuffer->device->funcs.p_vkCmdNextSubpass2KHR(params->commandBuffer->command_buffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdPipelineBarrier(void *args)
{
    struct vkCmdPipelineBarrier_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryBarrier_host *pBufferMemoryBarriers_host;
    VkImageMemoryBarrier_host *pImageMemoryBarriers_host;
    TRACE("%p, %#x, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);

    pBufferMemoryBarriers_host = convert_VkBufferMemoryBarrier_array_win_to_host(params->pBufferMemoryBarriers, params->bufferMemoryBarrierCount);
    pImageMemoryBarriers_host = convert_VkImageMemoryBarrier_array_win_to_host(params->pImageMemoryBarriers, params->imageMemoryBarrierCount);
    params->commandBuffer->device->funcs.p_vkCmdPipelineBarrier(params->commandBuffer->command_buffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, pBufferMemoryBarriers_host, params->imageMemoryBarrierCount, pImageMemoryBarriers_host);

    free_VkBufferMemoryBarrier_array(pBufferMemoryBarriers_host, params->bufferMemoryBarrierCount);
    free_VkImageMemoryBarrier_array(pImageMemoryBarriers_host, params->imageMemoryBarrierCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %#x, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);
    params->commandBuffer->device->funcs.p_vkCmdPipelineBarrier(params->commandBuffer->command_buffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdPipelineBarrier2(void *args)
{
    struct vkCmdPipelineBarrier2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDependencyInfo_host pDependencyInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);

    convert_VkDependencyInfo_win_to_host(params->pDependencyInfo, &pDependencyInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdPipelineBarrier2(params->commandBuffer->command_buffer, &pDependencyInfo_host);

    free_VkDependencyInfo(&pDependencyInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);
    params->commandBuffer->device->funcs.p_vkCmdPipelineBarrier2(params->commandBuffer->command_buffer, params->pDependencyInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdPipelineBarrier2KHR(void *args)
{
    struct vkCmdPipelineBarrier2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDependencyInfo_host pDependencyInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);

    convert_VkDependencyInfo_win_to_host(params->pDependencyInfo, &pDependencyInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdPipelineBarrier2KHR(params->commandBuffer->command_buffer, &pDependencyInfo_host);

    free_VkDependencyInfo(&pDependencyInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);
    params->commandBuffer->device->funcs.p_vkCmdPipelineBarrier2KHR(params->commandBuffer->command_buffer, params->pDependencyInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdPreprocessGeneratedCommandsNV(void *args)
{
    struct vkCmdPreprocessGeneratedCommandsNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkGeneratedCommandsInfoNV_host pGeneratedCommandsInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pGeneratedCommandsInfo);

    convert_VkGeneratedCommandsInfoNV_win_to_host(params->pGeneratedCommandsInfo, &pGeneratedCommandsInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdPreprocessGeneratedCommandsNV(params->commandBuffer->command_buffer, &pGeneratedCommandsInfo_host);

    free_VkGeneratedCommandsInfoNV(&pGeneratedCommandsInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pGeneratedCommandsInfo);
    params->commandBuffer->device->funcs.p_vkCmdPreprocessGeneratedCommandsNV(params->commandBuffer->command_buffer, params->pGeneratedCommandsInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdPushConstants(void *args)
{
    struct vkCmdPushConstants_params *params = args;
    TRACE("%p, 0x%s, %#x, %u, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->layout), params->stageFlags, params->offset, params->size, params->pValues);
    params->commandBuffer->device->funcs.p_vkCmdPushConstants(params->commandBuffer->command_buffer, params->layout, params->stageFlags, params->offset, params->size, params->pValues);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdPushDescriptorSetKHR(void *args)
{
    struct vkCmdPushDescriptorSetKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkWriteDescriptorSet_host *pDescriptorWrites_host;
    TRACE("%p, %#x, 0x%s, %u, %u, %p\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->layout), params->set, params->descriptorWriteCount, params->pDescriptorWrites);

    pDescriptorWrites_host = convert_VkWriteDescriptorSet_array_win_to_host(params->pDescriptorWrites, params->descriptorWriteCount);
    params->commandBuffer->device->funcs.p_vkCmdPushDescriptorSetKHR(params->commandBuffer->command_buffer, params->pipelineBindPoint, params->layout, params->set, params->descriptorWriteCount, pDescriptorWrites_host);

    free_VkWriteDescriptorSet_array(pDescriptorWrites_host, params->descriptorWriteCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %#x, 0x%s, %u, %u, %p\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->layout), params->set, params->descriptorWriteCount, params->pDescriptorWrites);
    params->commandBuffer->device->funcs.p_vkCmdPushDescriptorSetKHR(params->commandBuffer->command_buffer, params->pipelineBindPoint, params->layout, params->set, params->descriptorWriteCount, params->pDescriptorWrites);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdPushDescriptorSetWithTemplateKHR(void *args)
{
    struct vkCmdPushDescriptorSetWithTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->descriptorUpdateTemplate), wine_dbgstr_longlong(params->layout), params->set, params->pData);
    params->commandBuffer->device->funcs.p_vkCmdPushDescriptorSetWithTemplateKHR(params->commandBuffer->command_buffer, params->descriptorUpdateTemplate, params->layout, params->set, params->pData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdResetEvent(void *args)
{
    struct vkCmdResetEvent_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->stageMask);
    params->commandBuffer->device->funcs.p_vkCmdResetEvent(params->commandBuffer->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdResetEvent2(void *args)
{
    struct vkCmdResetEvent2_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->event), wine_dbgstr_longlong(params->stageMask));
    params->commandBuffer->device->funcs.p_vkCmdResetEvent2(params->commandBuffer->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdResetEvent2KHR(void *args)
{
    struct vkCmdResetEvent2KHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->event), wine_dbgstr_longlong(params->stageMask));
    params->commandBuffer->device->funcs.p_vkCmdResetEvent2KHR(params->commandBuffer->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdResetQueryPool(void *args)
{
    struct vkCmdResetQueryPool_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);
    params->commandBuffer->device->funcs.p_vkCmdResetQueryPool(params->commandBuffer->command_buffer, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdResolveImage(void *args)
{
    struct vkCmdResolveImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);
    params->commandBuffer->device->funcs.p_vkCmdResolveImage(params->commandBuffer->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdResolveImage2(void *args)
{
    struct vkCmdResolveImage2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResolveImageInfo2_host pResolveImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);

    convert_VkResolveImageInfo2_win_to_host(params->pResolveImageInfo, &pResolveImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdResolveImage2(params->commandBuffer->command_buffer, &pResolveImageInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdResolveImage2(params->commandBuffer->command_buffer, params->pResolveImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdResolveImage2KHR(void *args)
{
    struct vkCmdResolveImage2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResolveImageInfo2_host pResolveImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);

    convert_VkResolveImageInfo2_win_to_host(params->pResolveImageInfo, &pResolveImageInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdResolveImage2KHR(params->commandBuffer->command_buffer, &pResolveImageInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);
    params->commandBuffer->device->funcs.p_vkCmdResolveImage2KHR(params->commandBuffer->command_buffer, params->pResolveImageInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdSetBlendConstants(void *args)
{
    struct vkCmdSetBlendConstants_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->blendConstants);
    params->commandBuffer->device->funcs.p_vkCmdSetBlendConstants(params->commandBuffer->command_buffer, params->blendConstants);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetCheckpointNV(void *args)
{
    struct vkCmdSetCheckpointNV_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCheckpointMarker);
    params->commandBuffer->device->funcs.p_vkCmdSetCheckpointNV(params->commandBuffer->command_buffer, params->pCheckpointMarker);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetCoarseSampleOrderNV(void *args)
{
    struct vkCmdSetCoarseSampleOrderNV_params *params = args;
    TRACE("%p, %#x, %u, %p\n", params->commandBuffer, params->sampleOrderType, params->customSampleOrderCount, params->pCustomSampleOrders);
    params->commandBuffer->device->funcs.p_vkCmdSetCoarseSampleOrderNV(params->commandBuffer->command_buffer, params->sampleOrderType, params->customSampleOrderCount, params->pCustomSampleOrders);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetColorWriteEnableEXT(void *args)
{
    struct vkCmdSetColorWriteEnableEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->attachmentCount, params->pColorWriteEnables);
    params->commandBuffer->device->funcs.p_vkCmdSetColorWriteEnableEXT(params->commandBuffer->command_buffer, params->attachmentCount, params->pColorWriteEnables);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetCullMode(void *args)
{
    struct vkCmdSetCullMode_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->cullMode);
    params->commandBuffer->device->funcs.p_vkCmdSetCullMode(params->commandBuffer->command_buffer, params->cullMode);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetCullModeEXT(void *args)
{
    struct vkCmdSetCullModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->cullMode);
    params->commandBuffer->device->funcs.p_vkCmdSetCullModeEXT(params->commandBuffer->command_buffer, params->cullMode);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthBias(void *args)
{
    struct vkCmdSetDepthBias_params *params = args;
    TRACE("%p, %f, %f, %f\n", params->commandBuffer, params->depthBiasConstantFactor, params->depthBiasClamp, params->depthBiasSlopeFactor);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthBias(params->commandBuffer->command_buffer, params->depthBiasConstantFactor, params->depthBiasClamp, params->depthBiasSlopeFactor);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthBiasEnable(void *args)
{
    struct vkCmdSetDepthBiasEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBiasEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthBiasEnable(params->commandBuffer->command_buffer, params->depthBiasEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthBiasEnableEXT(void *args)
{
    struct vkCmdSetDepthBiasEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBiasEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthBiasEnableEXT(params->commandBuffer->command_buffer, params->depthBiasEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthBounds(void *args)
{
    struct vkCmdSetDepthBounds_params *params = args;
    TRACE("%p, %f, %f\n", params->commandBuffer, params->minDepthBounds, params->maxDepthBounds);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthBounds(params->commandBuffer->command_buffer, params->minDepthBounds, params->maxDepthBounds);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthBoundsTestEnable(void *args)
{
    struct vkCmdSetDepthBoundsTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBoundsTestEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthBoundsTestEnable(params->commandBuffer->command_buffer, params->depthBoundsTestEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthBoundsTestEnableEXT(void *args)
{
    struct vkCmdSetDepthBoundsTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBoundsTestEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthBoundsTestEnableEXT(params->commandBuffer->command_buffer, params->depthBoundsTestEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthCompareOp(void *args)
{
    struct vkCmdSetDepthCompareOp_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->depthCompareOp);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthCompareOp(params->commandBuffer->command_buffer, params->depthCompareOp);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthCompareOpEXT(void *args)
{
    struct vkCmdSetDepthCompareOpEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->depthCompareOp);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthCompareOpEXT(params->commandBuffer->command_buffer, params->depthCompareOp);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthTestEnable(void *args)
{
    struct vkCmdSetDepthTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthTestEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthTestEnable(params->commandBuffer->command_buffer, params->depthTestEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthTestEnableEXT(void *args)
{
    struct vkCmdSetDepthTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthTestEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthTestEnableEXT(params->commandBuffer->command_buffer, params->depthTestEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthWriteEnable(void *args)
{
    struct vkCmdSetDepthWriteEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthWriteEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthWriteEnable(params->commandBuffer->command_buffer, params->depthWriteEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDepthWriteEnableEXT(void *args)
{
    struct vkCmdSetDepthWriteEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthWriteEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetDepthWriteEnableEXT(params->commandBuffer->command_buffer, params->depthWriteEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDeviceMask(void *args)
{
    struct vkCmdSetDeviceMask_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->deviceMask);
    params->commandBuffer->device->funcs.p_vkCmdSetDeviceMask(params->commandBuffer->command_buffer, params->deviceMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDeviceMaskKHR(void *args)
{
    struct vkCmdSetDeviceMaskKHR_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->deviceMask);
    params->commandBuffer->device->funcs.p_vkCmdSetDeviceMaskKHR(params->commandBuffer->command_buffer, params->deviceMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetDiscardRectangleEXT(void *args)
{
    struct vkCmdSetDiscardRectangleEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstDiscardRectangle, params->discardRectangleCount, params->pDiscardRectangles);
    params->commandBuffer->device->funcs.p_vkCmdSetDiscardRectangleEXT(params->commandBuffer->command_buffer, params->firstDiscardRectangle, params->discardRectangleCount, params->pDiscardRectangles);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetEvent(void *args)
{
    struct vkCmdSetEvent_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->stageMask);
    params->commandBuffer->device->funcs.p_vkCmdSetEvent(params->commandBuffer->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetEvent2(void *args)
{
    struct vkCmdSetEvent2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDependencyInfo_host pDependencyInfo_host;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);

    convert_VkDependencyInfo_win_to_host(params->pDependencyInfo, &pDependencyInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdSetEvent2(params->commandBuffer->command_buffer, params->event, &pDependencyInfo_host);

    free_VkDependencyInfo(&pDependencyInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);
    params->commandBuffer->device->funcs.p_vkCmdSetEvent2(params->commandBuffer->command_buffer, params->event, params->pDependencyInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdSetEvent2KHR(void *args)
{
    struct vkCmdSetEvent2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDependencyInfo_host pDependencyInfo_host;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);

    convert_VkDependencyInfo_win_to_host(params->pDependencyInfo, &pDependencyInfo_host);
    params->commandBuffer->device->funcs.p_vkCmdSetEvent2KHR(params->commandBuffer->command_buffer, params->event, &pDependencyInfo_host);

    free_VkDependencyInfo(&pDependencyInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);
    params->commandBuffer->device->funcs.p_vkCmdSetEvent2KHR(params->commandBuffer->command_buffer, params->event, params->pDependencyInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdSetExclusiveScissorNV(void *args)
{
    struct vkCmdSetExclusiveScissorNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstExclusiveScissor, params->exclusiveScissorCount, params->pExclusiveScissors);
    params->commandBuffer->device->funcs.p_vkCmdSetExclusiveScissorNV(params->commandBuffer->command_buffer, params->firstExclusiveScissor, params->exclusiveScissorCount, params->pExclusiveScissors);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetFragmentShadingRateEnumNV(void *args)
{
    struct vkCmdSetFragmentShadingRateEnumNV_params *params = args;
    TRACE("%p, %#x, %p\n", params->commandBuffer, params->shadingRate, params->combinerOps);
    params->commandBuffer->device->funcs.p_vkCmdSetFragmentShadingRateEnumNV(params->commandBuffer->command_buffer, params->shadingRate, params->combinerOps);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetFragmentShadingRateKHR(void *args)
{
    struct vkCmdSetFragmentShadingRateKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pFragmentSize, params->combinerOps);
    params->commandBuffer->device->funcs.p_vkCmdSetFragmentShadingRateKHR(params->commandBuffer->command_buffer, params->pFragmentSize, params->combinerOps);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetFrontFace(void *args)
{
    struct vkCmdSetFrontFace_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->frontFace);
    params->commandBuffer->device->funcs.p_vkCmdSetFrontFace(params->commandBuffer->command_buffer, params->frontFace);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetFrontFaceEXT(void *args)
{
    struct vkCmdSetFrontFaceEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->frontFace);
    params->commandBuffer->device->funcs.p_vkCmdSetFrontFaceEXT(params->commandBuffer->command_buffer, params->frontFace);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetLineStippleEXT(void *args)
{
    struct vkCmdSetLineStippleEXT_params *params = args;
    TRACE("%p, %u, %u\n", params->commandBuffer, params->lineStippleFactor, params->lineStipplePattern);
    params->commandBuffer->device->funcs.p_vkCmdSetLineStippleEXT(params->commandBuffer->command_buffer, params->lineStippleFactor, params->lineStipplePattern);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetLineWidth(void *args)
{
    struct vkCmdSetLineWidth_params *params = args;
    TRACE("%p, %f\n", params->commandBuffer, params->lineWidth);
    params->commandBuffer->device->funcs.p_vkCmdSetLineWidth(params->commandBuffer->command_buffer, params->lineWidth);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetLogicOpEXT(void *args)
{
    struct vkCmdSetLogicOpEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->logicOp);
    params->commandBuffer->device->funcs.p_vkCmdSetLogicOpEXT(params->commandBuffer->command_buffer, params->logicOp);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetPatchControlPointsEXT(void *args)
{
    struct vkCmdSetPatchControlPointsEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->patchControlPoints);
    params->commandBuffer->device->funcs.p_vkCmdSetPatchControlPointsEXT(params->commandBuffer->command_buffer, params->patchControlPoints);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetPerformanceMarkerINTEL(void *args)
{
    struct vkCmdSetPerformanceMarkerINTEL_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPerformanceMarkerInfoINTEL_host pMarkerInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    convert_VkPerformanceMarkerInfoINTEL_win_to_host(params->pMarkerInfo, &pMarkerInfo_host);
    result = params->commandBuffer->device->funcs.p_vkCmdSetPerformanceMarkerINTEL(params->commandBuffer->command_buffer, &pMarkerInfo_host);

    return result;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);
    return params->commandBuffer->device->funcs.p_vkCmdSetPerformanceMarkerINTEL(params->commandBuffer->command_buffer, params->pMarkerInfo);
#endif
}

static NTSTATUS wine_vkCmdSetPerformanceOverrideINTEL(void *args)
{
    struct vkCmdSetPerformanceOverrideINTEL_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPerformanceOverrideInfoINTEL_host pOverrideInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pOverrideInfo);

    convert_VkPerformanceOverrideInfoINTEL_win_to_host(params->pOverrideInfo, &pOverrideInfo_host);
    result = params->commandBuffer->device->funcs.p_vkCmdSetPerformanceOverrideINTEL(params->commandBuffer->command_buffer, &pOverrideInfo_host);

    return result;
#else
    TRACE("%p, %p\n", params->commandBuffer, params->pOverrideInfo);
    return params->commandBuffer->device->funcs.p_vkCmdSetPerformanceOverrideINTEL(params->commandBuffer->command_buffer, params->pOverrideInfo);
#endif
}

static NTSTATUS wine_vkCmdSetPerformanceStreamMarkerINTEL(void *args)
{
    struct vkCmdSetPerformanceStreamMarkerINTEL_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);
    return params->commandBuffer->device->funcs.p_vkCmdSetPerformanceStreamMarkerINTEL(params->commandBuffer->command_buffer, params->pMarkerInfo);
}

static NTSTATUS wine_vkCmdSetPrimitiveRestartEnable(void *args)
{
    struct vkCmdSetPrimitiveRestartEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->primitiveRestartEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetPrimitiveRestartEnable(params->commandBuffer->command_buffer, params->primitiveRestartEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetPrimitiveRestartEnableEXT(void *args)
{
    struct vkCmdSetPrimitiveRestartEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->primitiveRestartEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetPrimitiveRestartEnableEXT(params->commandBuffer->command_buffer, params->primitiveRestartEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetPrimitiveTopology(void *args)
{
    struct vkCmdSetPrimitiveTopology_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->primitiveTopology);
    params->commandBuffer->device->funcs.p_vkCmdSetPrimitiveTopology(params->commandBuffer->command_buffer, params->primitiveTopology);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetPrimitiveTopologyEXT(void *args)
{
    struct vkCmdSetPrimitiveTopologyEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->primitiveTopology);
    params->commandBuffer->device->funcs.p_vkCmdSetPrimitiveTopologyEXT(params->commandBuffer->command_buffer, params->primitiveTopology);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetRasterizerDiscardEnable(void *args)
{
    struct vkCmdSetRasterizerDiscardEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizerDiscardEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetRasterizerDiscardEnable(params->commandBuffer->command_buffer, params->rasterizerDiscardEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetRasterizerDiscardEnableEXT(void *args)
{
    struct vkCmdSetRasterizerDiscardEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizerDiscardEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetRasterizerDiscardEnableEXT(params->commandBuffer->command_buffer, params->rasterizerDiscardEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetRayTracingPipelineStackSizeKHR(void *args)
{
    struct vkCmdSetRayTracingPipelineStackSizeKHR_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->pipelineStackSize);
    params->commandBuffer->device->funcs.p_vkCmdSetRayTracingPipelineStackSizeKHR(params->commandBuffer->command_buffer, params->pipelineStackSize);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetSampleLocationsEXT(void *args)
{
    struct vkCmdSetSampleLocationsEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSampleLocationsInfo);
    params->commandBuffer->device->funcs.p_vkCmdSetSampleLocationsEXT(params->commandBuffer->command_buffer, params->pSampleLocationsInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetScissor(void *args)
{
    struct vkCmdSetScissor_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstScissor, params->scissorCount, params->pScissors);
    params->commandBuffer->device->funcs.p_vkCmdSetScissor(params->commandBuffer->command_buffer, params->firstScissor, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetScissorWithCount(void *args)
{
    struct vkCmdSetScissorWithCount_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->scissorCount, params->pScissors);
    params->commandBuffer->device->funcs.p_vkCmdSetScissorWithCount(params->commandBuffer->command_buffer, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetScissorWithCountEXT(void *args)
{
    struct vkCmdSetScissorWithCountEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->scissorCount, params->pScissors);
    params->commandBuffer->device->funcs.p_vkCmdSetScissorWithCountEXT(params->commandBuffer->command_buffer, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetStencilCompareMask(void *args)
{
    struct vkCmdSetStencilCompareMask_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->compareMask);
    params->commandBuffer->device->funcs.p_vkCmdSetStencilCompareMask(params->commandBuffer->command_buffer, params->faceMask, params->compareMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetStencilOp(void *args)
{
    struct vkCmdSetStencilOp_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x\n", params->commandBuffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    params->commandBuffer->device->funcs.p_vkCmdSetStencilOp(params->commandBuffer->command_buffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetStencilOpEXT(void *args)
{
    struct vkCmdSetStencilOpEXT_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x\n", params->commandBuffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    params->commandBuffer->device->funcs.p_vkCmdSetStencilOpEXT(params->commandBuffer->command_buffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetStencilReference(void *args)
{
    struct vkCmdSetStencilReference_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->reference);
    params->commandBuffer->device->funcs.p_vkCmdSetStencilReference(params->commandBuffer->command_buffer, params->faceMask, params->reference);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetStencilTestEnable(void *args)
{
    struct vkCmdSetStencilTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stencilTestEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetStencilTestEnable(params->commandBuffer->command_buffer, params->stencilTestEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetStencilTestEnableEXT(void *args)
{
    struct vkCmdSetStencilTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stencilTestEnable);
    params->commandBuffer->device->funcs.p_vkCmdSetStencilTestEnableEXT(params->commandBuffer->command_buffer, params->stencilTestEnable);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetStencilWriteMask(void *args)
{
    struct vkCmdSetStencilWriteMask_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->writeMask);
    params->commandBuffer->device->funcs.p_vkCmdSetStencilWriteMask(params->commandBuffer->command_buffer, params->faceMask, params->writeMask);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetVertexInputEXT(void *args)
{
    struct vkCmdSetVertexInputEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %p\n", params->commandBuffer, params->vertexBindingDescriptionCount, params->pVertexBindingDescriptions, params->vertexAttributeDescriptionCount, params->pVertexAttributeDescriptions);
    params->commandBuffer->device->funcs.p_vkCmdSetVertexInputEXT(params->commandBuffer->command_buffer, params->vertexBindingDescriptionCount, params->pVertexBindingDescriptions, params->vertexAttributeDescriptionCount, params->pVertexAttributeDescriptions);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetViewport(void *args)
{
    struct vkCmdSetViewport_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewports);
    params->commandBuffer->device->funcs.p_vkCmdSetViewport(params->commandBuffer->command_buffer, params->firstViewport, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetViewportShadingRatePaletteNV(void *args)
{
    struct vkCmdSetViewportShadingRatePaletteNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pShadingRatePalettes);
    params->commandBuffer->device->funcs.p_vkCmdSetViewportShadingRatePaletteNV(params->commandBuffer->command_buffer, params->firstViewport, params->viewportCount, params->pShadingRatePalettes);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetViewportWScalingNV(void *args)
{
    struct vkCmdSetViewportWScalingNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewportWScalings);
    params->commandBuffer->device->funcs.p_vkCmdSetViewportWScalingNV(params->commandBuffer->command_buffer, params->firstViewport, params->viewportCount, params->pViewportWScalings);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetViewportWithCount(void *args)
{
    struct vkCmdSetViewportWithCount_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->viewportCount, params->pViewports);
    params->commandBuffer->device->funcs.p_vkCmdSetViewportWithCount(params->commandBuffer->command_buffer, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSetViewportWithCountEXT(void *args)
{
    struct vkCmdSetViewportWithCountEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->viewportCount, params->pViewports);
    params->commandBuffer->device->funcs.p_vkCmdSetViewportWithCountEXT(params->commandBuffer->command_buffer, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdSubpassShadingHUAWEI(void *args)
{
    struct vkCmdSubpassShadingHUAWEI_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    params->commandBuffer->device->funcs.p_vkCmdSubpassShadingHUAWEI(params->commandBuffer->command_buffer);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdTraceRaysIndirect2KHR(void *args)
{
    struct vkCmdTraceRaysIndirect2KHR_params *params = args;
    TRACE("%p, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->indirectDeviceAddress));
    params->commandBuffer->device->funcs.p_vkCmdTraceRaysIndirect2KHR(params->commandBuffer->command_buffer, params->indirectDeviceAddress);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdTraceRaysIndirectKHR(void *args)
{
    struct vkCmdTraceRaysIndirectKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkStridedDeviceAddressRegionKHR_host pRaygenShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pMissShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pHitShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pCallableShaderBindingTable_host;
    TRACE("%p, %p, %p, %p, %p, 0x%s\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, wine_dbgstr_longlong(params->indirectDeviceAddress));

    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pRaygenShaderBindingTable, &pRaygenShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pMissShaderBindingTable, &pMissShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pHitShaderBindingTable, &pHitShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pCallableShaderBindingTable, &pCallableShaderBindingTable_host);
    params->commandBuffer->device->funcs.p_vkCmdTraceRaysIndirectKHR(params->commandBuffer->command_buffer, &pRaygenShaderBindingTable_host, &pMissShaderBindingTable_host, &pHitShaderBindingTable_host, &pCallableShaderBindingTable_host, params->indirectDeviceAddress);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p, %p, %p, 0x%s\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, wine_dbgstr_longlong(params->indirectDeviceAddress));
    params->commandBuffer->device->funcs.p_vkCmdTraceRaysIndirectKHR(params->commandBuffer->command_buffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->indirectDeviceAddress);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdTraceRaysKHR(void *args)
{
    struct vkCmdTraceRaysKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkStridedDeviceAddressRegionKHR_host pRaygenShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pMissShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pHitShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pCallableShaderBindingTable_host;
    TRACE("%p, %p, %p, %p, %p, %u, %u, %u\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->width, params->height, params->depth);

    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pRaygenShaderBindingTable, &pRaygenShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pMissShaderBindingTable, &pMissShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pHitShaderBindingTable, &pHitShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win_to_host(params->pCallableShaderBindingTable, &pCallableShaderBindingTable_host);
    params->commandBuffer->device->funcs.p_vkCmdTraceRaysKHR(params->commandBuffer->command_buffer, &pRaygenShaderBindingTable_host, &pMissShaderBindingTable_host, &pHitShaderBindingTable_host, &pCallableShaderBindingTable_host, params->width, params->height, params->depth);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p, %p, %p, %u, %u, %u\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->width, params->height, params->depth);
    params->commandBuffer->device->funcs.p_vkCmdTraceRaysKHR(params->commandBuffer->command_buffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->width, params->height, params->depth);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdTraceRaysNV(void *args)
{
    struct vkCmdTraceRaysNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->raygenShaderBindingTableBuffer), wine_dbgstr_longlong(params->raygenShaderBindingOffset), wine_dbgstr_longlong(params->missShaderBindingTableBuffer), wine_dbgstr_longlong(params->missShaderBindingOffset), wine_dbgstr_longlong(params->missShaderBindingStride), wine_dbgstr_longlong(params->hitShaderBindingTableBuffer), wine_dbgstr_longlong(params->hitShaderBindingOffset), wine_dbgstr_longlong(params->hitShaderBindingStride), wine_dbgstr_longlong(params->callableShaderBindingTableBuffer), wine_dbgstr_longlong(params->callableShaderBindingOffset), wine_dbgstr_longlong(params->callableShaderBindingStride), params->width, params->height, params->depth);
    params->commandBuffer->device->funcs.p_vkCmdTraceRaysNV(params->commandBuffer->command_buffer, params->raygenShaderBindingTableBuffer, params->raygenShaderBindingOffset, params->missShaderBindingTableBuffer, params->missShaderBindingOffset, params->missShaderBindingStride, params->hitShaderBindingTableBuffer, params->hitShaderBindingOffset, params->hitShaderBindingStride, params->callableShaderBindingTableBuffer, params->callableShaderBindingOffset, params->callableShaderBindingStride, params->width, params->height, params->depth);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdUpdateBuffer(void *args)
{
    struct vkCmdUpdateBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->dataSize), params->pData);
    params->commandBuffer->device->funcs.p_vkCmdUpdateBuffer(params->commandBuffer->command_buffer, params->dstBuffer, params->dstOffset, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdWaitEvents(void *args)
{
    struct vkCmdWaitEvents_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryBarrier_host *pBufferMemoryBarriers_host;
    VkImageMemoryBarrier_host *pImageMemoryBarriers_host;
    TRACE("%p, %u, %p, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);

    pBufferMemoryBarriers_host = convert_VkBufferMemoryBarrier_array_win_to_host(params->pBufferMemoryBarriers, params->bufferMemoryBarrierCount);
    pImageMemoryBarriers_host = convert_VkImageMemoryBarrier_array_win_to_host(params->pImageMemoryBarriers, params->imageMemoryBarrierCount);
    params->commandBuffer->device->funcs.p_vkCmdWaitEvents(params->commandBuffer->command_buffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, pBufferMemoryBarriers_host, params->imageMemoryBarrierCount, pImageMemoryBarriers_host);

    free_VkBufferMemoryBarrier_array(pBufferMemoryBarriers_host, params->bufferMemoryBarrierCount);
    free_VkImageMemoryBarrier_array(pImageMemoryBarriers_host, params->imageMemoryBarrierCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %u, %p, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);
    params->commandBuffer->device->funcs.p_vkCmdWaitEvents(params->commandBuffer->command_buffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdWaitEvents2(void *args)
{
    struct vkCmdWaitEvents2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDependencyInfo_host *pDependencyInfos_host;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);

    pDependencyInfos_host = convert_VkDependencyInfo_array_win_to_host(params->pDependencyInfos, params->eventCount);
    params->commandBuffer->device->funcs.p_vkCmdWaitEvents2(params->commandBuffer->command_buffer, params->eventCount, params->pEvents, pDependencyInfos_host);

    free_VkDependencyInfo_array(pDependencyInfos_host, params->eventCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);
    params->commandBuffer->device->funcs.p_vkCmdWaitEvents2(params->commandBuffer->command_buffer, params->eventCount, params->pEvents, params->pDependencyInfos);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdWaitEvents2KHR(void *args)
{
    struct vkCmdWaitEvents2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDependencyInfo_host *pDependencyInfos_host;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);

    pDependencyInfos_host = convert_VkDependencyInfo_array_win_to_host(params->pDependencyInfos, params->eventCount);
    params->commandBuffer->device->funcs.p_vkCmdWaitEvents2KHR(params->commandBuffer->command_buffer, params->eventCount, params->pEvents, pDependencyInfos_host);

    free_VkDependencyInfo_array(pDependencyInfos_host, params->eventCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);
    params->commandBuffer->device->funcs.p_vkCmdWaitEvents2KHR(params->commandBuffer->command_buffer, params->eventCount, params->pEvents, params->pDependencyInfos);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkCmdWriteAccelerationStructuresPropertiesKHR(void *args)
{
    struct vkCmdWriteAccelerationStructuresPropertiesKHR_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);
    params->commandBuffer->device->funcs.p_vkCmdWriteAccelerationStructuresPropertiesKHR(params->commandBuffer->command_buffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdWriteAccelerationStructuresPropertiesNV(void *args)
{
    struct vkCmdWriteAccelerationStructuresPropertiesNV_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);
    params->commandBuffer->device->funcs.p_vkCmdWriteAccelerationStructuresPropertiesNV(params->commandBuffer->command_buffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdWriteBufferMarker2AMD(void *args)
{
    struct vkCmdWriteBufferMarker2AMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), params->marker);
    params->commandBuffer->device->funcs.p_vkCmdWriteBufferMarker2AMD(params->commandBuffer->command_buffer, params->stage, params->dstBuffer, params->dstOffset, params->marker);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdWriteBufferMarkerAMD(void *args)
{
    struct vkCmdWriteBufferMarkerAMD_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %u\n", params->commandBuffer, params->pipelineStage, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), params->marker);
    params->commandBuffer->device->funcs.p_vkCmdWriteBufferMarkerAMD(params->commandBuffer->command_buffer, params->pipelineStage, params->dstBuffer, params->dstOffset, params->marker);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdWriteTimestamp(void *args)
{
    struct vkCmdWriteTimestamp_params *params = args;
    TRACE("%p, %#x, 0x%s, %u\n", params->commandBuffer, params->pipelineStage, wine_dbgstr_longlong(params->queryPool), params->query);
    params->commandBuffer->device->funcs.p_vkCmdWriteTimestamp(params->commandBuffer->command_buffer, params->pipelineStage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdWriteTimestamp2(void *args)
{
    struct vkCmdWriteTimestamp2_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->queryPool), params->query);
    params->commandBuffer->device->funcs.p_vkCmdWriteTimestamp2(params->commandBuffer->command_buffer, params->stage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCmdWriteTimestamp2KHR(void *args)
{
    struct vkCmdWriteTimestamp2KHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->queryPool), params->query);
    params->commandBuffer->device->funcs.p_vkCmdWriteTimestamp2KHR(params->commandBuffer->command_buffer, params->stage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkCompileDeferredNV(void *args)
{
    struct vkCompileDeferredNV_params *params = args;
    TRACE("%p, 0x%s, %u\n", params->device, wine_dbgstr_longlong(params->pipeline), params->shader);
    return params->device->funcs.p_vkCompileDeferredNV(params->device->device, params->pipeline, params->shader);
}

static NTSTATUS wine_vkCopyAccelerationStructureKHR(void *args)
{
    struct vkCopyAccelerationStructureKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkCopyAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyAccelerationStructureInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    result = params->device->funcs.p_vkCopyAccelerationStructureKHR(params->device->device, params->deferredOperation, &pInfo_host);

    return result;
#else
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);
    return params->device->funcs.p_vkCopyAccelerationStructureKHR(params->device->device, params->deferredOperation, params->pInfo);
#endif
}

static NTSTATUS wine_vkCopyAccelerationStructureToMemoryKHR(void *args)
{
    struct vkCopyAccelerationStructureToMemoryKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkCopyAccelerationStructureToMemoryInfoKHR_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyAccelerationStructureToMemoryInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    result = params->device->funcs.p_vkCopyAccelerationStructureToMemoryKHR(params->device->device, params->deferredOperation, &pInfo_host);

    return result;
#else
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);
    return params->device->funcs.p_vkCopyAccelerationStructureToMemoryKHR(params->device->device, params->deferredOperation, params->pInfo);
#endif
}

static NTSTATUS wine_vkCopyMemoryToAccelerationStructureKHR(void *args)
{
    struct vkCopyMemoryToAccelerationStructureKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkCopyMemoryToAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyMemoryToAccelerationStructureInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    result = params->device->funcs.p_vkCopyMemoryToAccelerationStructureKHR(params->device->device, params->deferredOperation, &pInfo_host);

    return result;
#else
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);
    return params->device->funcs.p_vkCopyMemoryToAccelerationStructureKHR(params->device->device, params->deferredOperation, params->pInfo);
#endif
}

static NTSTATUS wine_vkCreateAccelerationStructureKHR(void *args)
{
    struct vkCreateAccelerationStructureKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAccelerationStructureCreateInfoKHR_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);

    convert_VkAccelerationStructureCreateInfoKHR_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateAccelerationStructureKHR(params->device->device, &pCreateInfo_host, NULL, params->pAccelerationStructure);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);
    return params->device->funcs.p_vkCreateAccelerationStructureKHR(params->device->device, params->pCreateInfo, NULL, params->pAccelerationStructure);
#endif
}

static NTSTATUS wine_vkCreateAccelerationStructureNV(void *args)
{
    struct vkCreateAccelerationStructureNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkAccelerationStructureCreateInfoNV_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);

    convert_VkAccelerationStructureCreateInfoNV_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateAccelerationStructureNV(params->device->device, &pCreateInfo_host, NULL, params->pAccelerationStructure);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);
    return params->device->funcs.p_vkCreateAccelerationStructureNV(params->device->device, params->pCreateInfo, NULL, params->pAccelerationStructure);
#endif
}

static NTSTATUS wine_vkCreateBuffer(void *args)
{
    struct vkCreateBuffer_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBufferCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pBuffer);

    convert_VkBufferCreateInfo_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateBuffer(params->device->device, &pCreateInfo_host, NULL, params->pBuffer);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pBuffer);
    return params->device->funcs.p_vkCreateBuffer(params->device->device, params->pCreateInfo, NULL, params->pBuffer);
#endif
}

static NTSTATUS wine_vkCreateBufferView(void *args)
{
    struct vkCreateBufferView_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBufferViewCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);

    convert_VkBufferViewCreateInfo_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateBufferView(params->device->device, &pCreateInfo_host, NULL, params->pView);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);
    return params->device->funcs.p_vkCreateBufferView(params->device->device, params->pCreateInfo, NULL, params->pView);
#endif
}

VkResult thunk_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkComputePipelineCreateInfo_host *pCreateInfos_host;
    pCreateInfos_host = convert_VkComputePipelineCreateInfo_array_win_to_host(pCreateInfos, createInfoCount);
    result = device->funcs.p_vkCreateComputePipelines(device->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);

    free_VkComputePipelineCreateInfo_array(pCreateInfos_host, createInfoCount);
    return result;
#else
    return device->funcs.p_vkCreateComputePipelines(device->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
#endif
}

static NTSTATUS wine_vkCreateCuFunctionNVX(void *args)
{
    struct vkCreateCuFunctionNVX_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkCuFunctionCreateInfoNVX_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFunction);

    convert_VkCuFunctionCreateInfoNVX_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateCuFunctionNVX(params->device->device, &pCreateInfo_host, NULL, params->pFunction);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFunction);
    return params->device->funcs.p_vkCreateCuFunctionNVX(params->device->device, params->pCreateInfo, NULL, params->pFunction);
#endif
}

static NTSTATUS wine_vkCreateCuModuleNVX(void *args)
{
    struct vkCreateCuModuleNVX_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pModule);
    return params->device->funcs.p_vkCreateCuModuleNVX(params->device->device, params->pCreateInfo, NULL, params->pModule);
}

static NTSTATUS wine_vkCreateDeferredOperationKHR(void *args)
{
    struct vkCreateDeferredOperationKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAllocator, params->pDeferredOperation);
    return params->device->funcs.p_vkCreateDeferredOperationKHR(params->device->device, NULL, params->pDeferredOperation);
}

static NTSTATUS wine_vkCreateDescriptorPool(void *args)
{
    struct vkCreateDescriptorPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorPool);
    return params->device->funcs.p_vkCreateDescriptorPool(params->device->device, params->pCreateInfo, NULL, params->pDescriptorPool);
}

static NTSTATUS wine_vkCreateDescriptorSetLayout(void *args)
{
    struct vkCreateDescriptorSetLayout_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSetLayout);
    return params->device->funcs.p_vkCreateDescriptorSetLayout(params->device->device, params->pCreateInfo, NULL, params->pSetLayout);
}

static NTSTATUS wine_vkCreateDescriptorUpdateTemplate(void *args)
{
    struct vkCreateDescriptorUpdateTemplate_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDescriptorUpdateTemplateCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);

    convert_VkDescriptorUpdateTemplateCreateInfo_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateDescriptorUpdateTemplate(params->device->device, &pCreateInfo_host, NULL, params->pDescriptorUpdateTemplate);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);
    return params->device->funcs.p_vkCreateDescriptorUpdateTemplate(params->device->device, params->pCreateInfo, NULL, params->pDescriptorUpdateTemplate);
#endif
}

static NTSTATUS wine_vkCreateDescriptorUpdateTemplateKHR(void *args)
{
    struct vkCreateDescriptorUpdateTemplateKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDescriptorUpdateTemplateCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);

    convert_VkDescriptorUpdateTemplateCreateInfo_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateDescriptorUpdateTemplateKHR(params->device->device, &pCreateInfo_host, NULL, params->pDescriptorUpdateTemplate);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);
    return params->device->funcs.p_vkCreateDescriptorUpdateTemplateKHR(params->device->device, params->pCreateInfo, NULL, params->pDescriptorUpdateTemplate);
#endif
}

static NTSTATUS wine_vkCreateEvent(void *args)
{
    struct vkCreateEvent_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pEvent);
    return params->device->funcs.p_vkCreateEvent(params->device->device, params->pCreateInfo, NULL, params->pEvent);
}

static NTSTATUS wine_vkCreateFence(void *args)
{
    struct vkCreateFence_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFence);
    return params->device->funcs.p_vkCreateFence(params->device->device, params->pCreateInfo, NULL, params->pFence);
}

static NTSTATUS wine_vkCreateFramebuffer(void *args)
{
    struct vkCreateFramebuffer_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkFramebufferCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFramebuffer);

    convert_VkFramebufferCreateInfo_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateFramebuffer(params->device->device, &pCreateInfo_host, NULL, params->pFramebuffer);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFramebuffer);
    return params->device->funcs.p_vkCreateFramebuffer(params->device->device, params->pCreateInfo, NULL, params->pFramebuffer);
#endif
}

VkResult thunk_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkGraphicsPipelineCreateInfo_host *pCreateInfos_host;
    pCreateInfos_host = convert_VkGraphicsPipelineCreateInfo_array_win_to_host(pCreateInfos, createInfoCount);
    result = device->funcs.p_vkCreateGraphicsPipelines(device->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);

    free_VkGraphicsPipelineCreateInfo_array(pCreateInfos_host, createInfoCount);
    return result;
#else
    return device->funcs.p_vkCreateGraphicsPipelines(device->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
#endif
}

static NTSTATUS wine_vkCreateImage(void *args)
{
    struct vkCreateImage_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pImage);
    return params->device->funcs.p_vkCreateImage(params->device->device, params->pCreateInfo, NULL, params->pImage);
}

static NTSTATUS wine_vkCreateImageView(void *args)
{
    struct vkCreateImageView_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageViewCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);

    convert_VkImageViewCreateInfo_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateImageView(params->device->device, &pCreateInfo_host, NULL, params->pView);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);
    return params->device->funcs.p_vkCreateImageView(params->device->device, params->pCreateInfo, NULL, params->pView);
#endif
}

static NTSTATUS wine_vkCreateIndirectCommandsLayoutNV(void *args)
{
    struct vkCreateIndirectCommandsLayoutNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkIndirectCommandsLayoutCreateInfoNV_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pIndirectCommandsLayout);

    convert_VkIndirectCommandsLayoutCreateInfoNV_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateIndirectCommandsLayoutNV(params->device->device, &pCreateInfo_host, NULL, params->pIndirectCommandsLayout);

    free_VkIndirectCommandsLayoutCreateInfoNV(&pCreateInfo_host);
    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pIndirectCommandsLayout);
    return params->device->funcs.p_vkCreateIndirectCommandsLayoutNV(params->device->device, params->pCreateInfo, NULL, params->pIndirectCommandsLayout);
#endif
}

static NTSTATUS wine_vkCreatePipelineCache(void *args)
{
    struct vkCreatePipelineCache_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPipelineCache);
    return params->device->funcs.p_vkCreatePipelineCache(params->device->device, params->pCreateInfo, NULL, params->pPipelineCache);
}

static NTSTATUS wine_vkCreatePipelineLayout(void *args)
{
    struct vkCreatePipelineLayout_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPipelineLayout);
    return params->device->funcs.p_vkCreatePipelineLayout(params->device->device, params->pCreateInfo, NULL, params->pPipelineLayout);
}

static NTSTATUS wine_vkCreatePrivateDataSlot(void *args)
{
    struct vkCreatePrivateDataSlot_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPrivateDataSlot);
    return params->device->funcs.p_vkCreatePrivateDataSlot(params->device->device, params->pCreateInfo, NULL, params->pPrivateDataSlot);
}

static NTSTATUS wine_vkCreatePrivateDataSlotEXT(void *args)
{
    struct vkCreatePrivateDataSlotEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPrivateDataSlot);
    return params->device->funcs.p_vkCreatePrivateDataSlotEXT(params->device->device, params->pCreateInfo, NULL, params->pPrivateDataSlot);
}

static NTSTATUS wine_vkCreateQueryPool(void *args)
{
    struct vkCreateQueryPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pQueryPool);
    return params->device->funcs.p_vkCreateQueryPool(params->device->device, params->pCreateInfo, NULL, params->pQueryPool);
}

VkResult thunk_vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkRayTracingPipelineCreateInfoKHR_host *pCreateInfos_host;
    pCreateInfos_host = convert_VkRayTracingPipelineCreateInfoKHR_array_win_to_host(pCreateInfos, createInfoCount);
    result = device->funcs.p_vkCreateRayTracingPipelinesKHR(device->device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);

    free_VkRayTracingPipelineCreateInfoKHR_array(pCreateInfos_host, createInfoCount);
    return result;
#else
    return device->funcs.p_vkCreateRayTracingPipelinesKHR(device->device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
#endif
}

VkResult thunk_vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkRayTracingPipelineCreateInfoNV_host *pCreateInfos_host;
    pCreateInfos_host = convert_VkRayTracingPipelineCreateInfoNV_array_win_to_host(pCreateInfos, createInfoCount);
    result = device->funcs.p_vkCreateRayTracingPipelinesNV(device->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);

    free_VkRayTracingPipelineCreateInfoNV_array(pCreateInfos_host, createInfoCount);
    return result;
#else
    return device->funcs.p_vkCreateRayTracingPipelinesNV(device->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
#endif
}

static NTSTATUS wine_vkCreateRenderPass(void *args)
{
    struct vkCreateRenderPass_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);
    return params->device->funcs.p_vkCreateRenderPass(params->device->device, params->pCreateInfo, NULL, params->pRenderPass);
}

static NTSTATUS wine_vkCreateRenderPass2(void *args)
{
    struct vkCreateRenderPass2_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);
    return params->device->funcs.p_vkCreateRenderPass2(params->device->device, params->pCreateInfo, NULL, params->pRenderPass);
}

static NTSTATUS wine_vkCreateRenderPass2KHR(void *args)
{
    struct vkCreateRenderPass2KHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);
    return params->device->funcs.p_vkCreateRenderPass2KHR(params->device->device, params->pCreateInfo, NULL, params->pRenderPass);
}

static NTSTATUS wine_vkCreateSampler(void *args)
{
    struct vkCreateSampler_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSampler);
    return params->device->funcs.p_vkCreateSampler(params->device->device, params->pCreateInfo, NULL, params->pSampler);
}

static NTSTATUS wine_vkCreateSamplerYcbcrConversion(void *args)
{
    struct vkCreateSamplerYcbcrConversion_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pYcbcrConversion);
    return params->device->funcs.p_vkCreateSamplerYcbcrConversion(params->device->device, params->pCreateInfo, NULL, params->pYcbcrConversion);
}

static NTSTATUS wine_vkCreateSamplerYcbcrConversionKHR(void *args)
{
    struct vkCreateSamplerYcbcrConversionKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pYcbcrConversion);
    return params->device->funcs.p_vkCreateSamplerYcbcrConversionKHR(params->device->device, params->pCreateInfo, NULL, params->pYcbcrConversion);
}

static NTSTATUS wine_vkCreateSemaphore(void *args)
{
    struct vkCreateSemaphore_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSemaphore);
    return params->device->funcs.p_vkCreateSemaphore(params->device->device, params->pCreateInfo, NULL, params->pSemaphore);
}

static NTSTATUS wine_vkCreateShaderModule(void *args)
{
    struct vkCreateShaderModule_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pShaderModule);
    return params->device->funcs.p_vkCreateShaderModule(params->device->device, params->pCreateInfo, NULL, params->pShaderModule);
}

static NTSTATUS wine_vkCreateSwapchainKHR(void *args)
{
    struct vkCreateSwapchainKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSwapchainCreateInfoKHR_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSwapchain);

    convert_VkSwapchainCreateInfoKHR_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateSwapchainKHR(params->device->device, &pCreateInfo_host, NULL, params->pSwapchain);

    return result;
#else
    VkResult result;
    VkSwapchainCreateInfoKHR pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSwapchain);

    convert_VkSwapchainCreateInfoKHR_win_to_host(params->pCreateInfo, &pCreateInfo_host);
    result = params->device->funcs.p_vkCreateSwapchainKHR(params->device->device, &pCreateInfo_host, NULL, params->pSwapchain);

    return result;
#endif
}

static NTSTATUS wine_vkCreateValidationCacheEXT(void *args)
{
    struct vkCreateValidationCacheEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pValidationCache);
    return params->device->funcs.p_vkCreateValidationCacheEXT(params->device->device, params->pCreateInfo, NULL, params->pValidationCache);
}

static NTSTATUS wine_vkDebugMarkerSetObjectNameEXT(void *args)
{
    struct vkDebugMarkerSetObjectNameEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugMarkerObjectNameInfoEXT_host pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugMarkerObjectNameInfoEXT_win_to_host(params->pNameInfo, &pNameInfo_host);
    result = params->device->funcs.p_vkDebugMarkerSetObjectNameEXT(params->device->device, &pNameInfo_host);

    return result;
#else
    VkResult result;
    VkDebugMarkerObjectNameInfoEXT pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugMarkerObjectNameInfoEXT_win_to_host(params->pNameInfo, &pNameInfo_host);
    result = params->device->funcs.p_vkDebugMarkerSetObjectNameEXT(params->device->device, &pNameInfo_host);

    return result;
#endif
}

static NTSTATUS wine_vkDebugMarkerSetObjectTagEXT(void *args)
{
    struct vkDebugMarkerSetObjectTagEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugMarkerObjectTagInfoEXT_host pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugMarkerObjectTagInfoEXT_win_to_host(params->pTagInfo, &pTagInfo_host);
    result = params->device->funcs.p_vkDebugMarkerSetObjectTagEXT(params->device->device, &pTagInfo_host);

    return result;
#else
    VkResult result;
    VkDebugMarkerObjectTagInfoEXT pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugMarkerObjectTagInfoEXT_win_to_host(params->pTagInfo, &pTagInfo_host);
    result = params->device->funcs.p_vkDebugMarkerSetObjectTagEXT(params->device->device, &pTagInfo_host);

    return result;
#endif
}

static NTSTATUS wine_vkDebugReportMessageEXT(void *args)
{
    struct vkDebugReportMessageEXT_params *params = args;
    TRACE("%p, %#x, %#x, 0x%s, 0x%s, %d, %p, %p\n", params->instance, params->flags, params->objectType, wine_dbgstr_longlong(params->object), wine_dbgstr_longlong(params->location), params->messageCode, params->pLayerPrefix, params->pMessage);
    params->instance->funcs.p_vkDebugReportMessageEXT(params->instance->instance, params->flags, params->objectType, wine_vk_unwrap_handle(params->objectType, params->object), params->location, params->messageCode, params->pLayerPrefix, params->pMessage);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDeferredOperationJoinKHR(void *args)
{
    struct vkDeferredOperationJoinKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));
    return params->device->funcs.p_vkDeferredOperationJoinKHR(params->device->device, params->operation);
}

static NTSTATUS wine_vkDestroyAccelerationStructureKHR(void *args)
{
    struct vkDestroyAccelerationStructureKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), params->pAllocator);
    params->device->funcs.p_vkDestroyAccelerationStructureKHR(params->device->device, params->accelerationStructure, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyAccelerationStructureNV(void *args)
{
    struct vkDestroyAccelerationStructureNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), params->pAllocator);
    params->device->funcs.p_vkDestroyAccelerationStructureNV(params->device->device, params->accelerationStructure, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyBuffer(void *args)
{
    struct vkDestroyBuffer_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->buffer), params->pAllocator);
    params->device->funcs.p_vkDestroyBuffer(params->device->device, params->buffer, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyBufferView(void *args)
{
    struct vkDestroyBufferView_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->bufferView), params->pAllocator);
    params->device->funcs.p_vkDestroyBufferView(params->device->device, params->bufferView, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyCuFunctionNVX(void *args)
{
    struct vkDestroyCuFunctionNVX_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->function), params->pAllocator);
    params->device->funcs.p_vkDestroyCuFunctionNVX(params->device->device, params->function, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyCuModuleNVX(void *args)
{
    struct vkDestroyCuModuleNVX_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->module), params->pAllocator);
    params->device->funcs.p_vkDestroyCuModuleNVX(params->device->device, params->module, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyDeferredOperationKHR(void *args)
{
    struct vkDestroyDeferredOperationKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->operation), params->pAllocator);
    params->device->funcs.p_vkDestroyDeferredOperationKHR(params->device->device, params->operation, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyDescriptorPool(void *args)
{
    struct vkDestroyDescriptorPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->pAllocator);
    params->device->funcs.p_vkDestroyDescriptorPool(params->device->device, params->descriptorPool, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyDescriptorSetLayout(void *args)
{
    struct vkDestroyDescriptorSetLayout_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSetLayout), params->pAllocator);
    params->device->funcs.p_vkDestroyDescriptorSetLayout(params->device->device, params->descriptorSetLayout, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyDescriptorUpdateTemplate(void *args)
{
    struct vkDestroyDescriptorUpdateTemplate_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pAllocator);
    params->device->funcs.p_vkDestroyDescriptorUpdateTemplate(params->device->device, params->descriptorUpdateTemplate, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyDescriptorUpdateTemplateKHR(void *args)
{
    struct vkDestroyDescriptorUpdateTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pAllocator);
    params->device->funcs.p_vkDestroyDescriptorUpdateTemplateKHR(params->device->device, params->descriptorUpdateTemplate, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyEvent(void *args)
{
    struct vkDestroyEvent_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->event), params->pAllocator);
    params->device->funcs.p_vkDestroyEvent(params->device->device, params->event, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyFence(void *args)
{
    struct vkDestroyFence_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->fence), params->pAllocator);
    params->device->funcs.p_vkDestroyFence(params->device->device, params->fence, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyFramebuffer(void *args)
{
    struct vkDestroyFramebuffer_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->framebuffer), params->pAllocator);
    params->device->funcs.p_vkDestroyFramebuffer(params->device->device, params->framebuffer, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyImage(void *args)
{
    struct vkDestroyImage_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pAllocator);
    params->device->funcs.p_vkDestroyImage(params->device->device, params->image, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyImageView(void *args)
{
    struct vkDestroyImageView_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->imageView), params->pAllocator);
    params->device->funcs.p_vkDestroyImageView(params->device->device, params->imageView, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyIndirectCommandsLayoutNV(void *args)
{
    struct vkDestroyIndirectCommandsLayoutNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->indirectCommandsLayout), params->pAllocator);
    params->device->funcs.p_vkDestroyIndirectCommandsLayoutNV(params->device->device, params->indirectCommandsLayout, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyPipeline(void *args)
{
    struct vkDestroyPipeline_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->pAllocator);
    params->device->funcs.p_vkDestroyPipeline(params->device->device, params->pipeline, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyPipelineCache(void *args)
{
    struct vkDestroyPipelineCache_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->pAllocator);
    params->device->funcs.p_vkDestroyPipelineCache(params->device->device, params->pipelineCache, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyPipelineLayout(void *args)
{
    struct vkDestroyPipelineLayout_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipelineLayout), params->pAllocator);
    params->device->funcs.p_vkDestroyPipelineLayout(params->device->device, params->pipelineLayout, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyPrivateDataSlot(void *args)
{
    struct vkDestroyPrivateDataSlot_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->privateDataSlot), params->pAllocator);
    params->device->funcs.p_vkDestroyPrivateDataSlot(params->device->device, params->privateDataSlot, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyPrivateDataSlotEXT(void *args)
{
    struct vkDestroyPrivateDataSlotEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->privateDataSlot), params->pAllocator);
    params->device->funcs.p_vkDestroyPrivateDataSlotEXT(params->device->device, params->privateDataSlot, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyQueryPool(void *args)
{
    struct vkDestroyQueryPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->queryPool), params->pAllocator);
    params->device->funcs.p_vkDestroyQueryPool(params->device->device, params->queryPool, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyRenderPass(void *args)
{
    struct vkDestroyRenderPass_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderPass), params->pAllocator);
    params->device->funcs.p_vkDestroyRenderPass(params->device->device, params->renderPass, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroySampler(void *args)
{
    struct vkDestroySampler_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->sampler), params->pAllocator);
    params->device->funcs.p_vkDestroySampler(params->device->device, params->sampler, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroySamplerYcbcrConversion(void *args)
{
    struct vkDestroySamplerYcbcrConversion_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->ycbcrConversion), params->pAllocator);
    params->device->funcs.p_vkDestroySamplerYcbcrConversion(params->device->device, params->ycbcrConversion, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroySamplerYcbcrConversionKHR(void *args)
{
    struct vkDestroySamplerYcbcrConversionKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->ycbcrConversion), params->pAllocator);
    params->device->funcs.p_vkDestroySamplerYcbcrConversionKHR(params->device->device, params->ycbcrConversion, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroySemaphore(void *args)
{
    struct vkDestroySemaphore_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pAllocator);
    params->device->funcs.p_vkDestroySemaphore(params->device->device, params->semaphore, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyShaderModule(void *args)
{
    struct vkDestroyShaderModule_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->shaderModule), params->pAllocator);
    params->device->funcs.p_vkDestroyShaderModule(params->device->device, params->shaderModule, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroySwapchainKHR(void *args)
{
    struct vkDestroySwapchainKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), params->pAllocator);
    params->device->funcs.p_vkDestroySwapchainKHR(params->device->device, params->swapchain, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDestroyValidationCacheEXT(void *args)
{
    struct vkDestroyValidationCacheEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->validationCache), params->pAllocator);
    params->device->funcs.p_vkDestroyValidationCacheEXT(params->device->device, params->validationCache, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkDeviceWaitIdle(void *args)
{
    struct vkDeviceWaitIdle_params *params = args;
    TRACE("%p\n", params->device);
    return params->device->funcs.p_vkDeviceWaitIdle(params->device->device);
}

static NTSTATUS wine_vkEndCommandBuffer(void *args)
{
    struct vkEndCommandBuffer_params *params = args;
    TRACE("%p\n", params->commandBuffer);
    return params->commandBuffer->device->funcs.p_vkEndCommandBuffer(params->commandBuffer->command_buffer);
}

static NTSTATUS wine_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(void *args)
{
    struct vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR_params *params = args;
    TRACE("%p, %u, %p, %p, %p\n", params->physicalDevice, params->queueFamilyIndex, params->pCounterCount, params->pCounters, params->pCounterDescriptions);
    return params->physicalDevice->instance->funcs.p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(params->physicalDevice->phys_dev, params->queueFamilyIndex, params->pCounterCount, params->pCounters, params->pCounterDescriptions);
}

static NTSTATUS wine_vkFlushMappedMemoryRanges(void *args)
{
    struct vkFlushMappedMemoryRanges_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkMappedMemoryRange_host *pMemoryRanges_host;
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);

    pMemoryRanges_host = convert_VkMappedMemoryRange_array_win_to_host(params->pMemoryRanges, params->memoryRangeCount);
    result = params->device->funcs.p_vkFlushMappedMemoryRanges(params->device->device, params->memoryRangeCount, pMemoryRanges_host);

    free_VkMappedMemoryRange_array(pMemoryRanges_host, params->memoryRangeCount);
    return result;
#else
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);
    return params->device->funcs.p_vkFlushMappedMemoryRanges(params->device->device, params->memoryRangeCount, params->pMemoryRanges);
#endif
}

static NTSTATUS wine_vkFreeDescriptorSets(void *args)
{
    struct vkFreeDescriptorSets_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->descriptorSetCount, params->pDescriptorSets);
    return params->device->funcs.p_vkFreeDescriptorSets(params->device->device, params->descriptorPool, params->descriptorSetCount, params->pDescriptorSets);
}

static NTSTATUS wine_vkFreeMemory(void *args)
{
    struct vkFreeMemory_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->memory), params->pAllocator);
    params->device->funcs.p_vkFreeMemory(params->device->device, params->memory, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetAccelerationStructureBuildSizesKHR(void *args)
{
    struct vkGetAccelerationStructureBuildSizesKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureBuildGeometryInfoKHR_host pBuildInfo_host;
    VkAccelerationStructureBuildSizesInfoKHR_host pSizeInfo_host;
    TRACE("%p, %#x, %p, %p, %p\n", params->device, params->buildType, params->pBuildInfo, params->pMaxPrimitiveCounts, params->pSizeInfo);

    convert_VkAccelerationStructureBuildGeometryInfoKHR_win_to_host(params->pBuildInfo, &pBuildInfo_host);
    convert_VkAccelerationStructureBuildSizesInfoKHR_win_to_host(params->pSizeInfo, &pSizeInfo_host);
    params->device->funcs.p_vkGetAccelerationStructureBuildSizesKHR(params->device->device, params->buildType, &pBuildInfo_host, params->pMaxPrimitiveCounts, &pSizeInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %#x, %p, %p, %p\n", params->device, params->buildType, params->pBuildInfo, params->pMaxPrimitiveCounts, params->pSizeInfo);
    params->device->funcs.p_vkGetAccelerationStructureBuildSizesKHR(params->device->device, params->buildType, params->pBuildInfo, params->pMaxPrimitiveCounts, params->pSizeInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetAccelerationStructureDeviceAddressKHR(void *args)
{
    struct vkGetAccelerationStructureDeviceAddressKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureDeviceAddressInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkAccelerationStructureDeviceAddressInfoKHR_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetAccelerationStructureDeviceAddressKHR(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetAccelerationStructureDeviceAddressKHR(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetAccelerationStructureHandleNV(void *args)
{
    struct vkGetAccelerationStructureHandleNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), wine_dbgstr_longlong(params->dataSize), params->pData);
    return params->device->funcs.p_vkGetAccelerationStructureHandleNV(params->device->device, params->accelerationStructure, params->dataSize, params->pData);
}

static NTSTATUS wine_vkGetAccelerationStructureMemoryRequirementsNV(void *args)
{
    struct vkGetAccelerationStructureMemoryRequirementsNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkAccelerationStructureMemoryRequirementsInfoNV_host pInfo_host;
    VkMemoryRequirements2KHR_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkAccelerationStructureMemoryRequirementsInfoNV_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2KHR_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetAccelerationStructureMemoryRequirementsNV(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2KHR_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetAccelerationStructureMemoryRequirementsNV(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferDeviceAddress(void *args)
{
    struct vkGetBufferDeviceAddress_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetBufferDeviceAddress(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetBufferDeviceAddress(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferDeviceAddressEXT(void *args)
{
    struct vkGetBufferDeviceAddressEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetBufferDeviceAddressEXT(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetBufferDeviceAddressEXT(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferDeviceAddressKHR(void *args)
{
    struct vkGetBufferDeviceAddressKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetBufferDeviceAddressKHR(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetBufferDeviceAddressKHR(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferMemoryRequirements(void *args)
{
    struct vkGetBufferMemoryRequirements_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkMemoryRequirements_host pMemoryRequirements_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->buffer), params->pMemoryRequirements);

    params->device->funcs.p_vkGetBufferMemoryRequirements(params->device->device, params->buffer, &pMemoryRequirements_host);

    convert_VkMemoryRequirements_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->buffer), params->pMemoryRequirements);
    params->device->funcs.p_vkGetBufferMemoryRequirements(params->device->device, params->buffer, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferMemoryRequirements2(void *args)
{
    struct vkGetBufferMemoryRequirements2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkBufferMemoryRequirementsInfo2_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetBufferMemoryRequirements2(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetBufferMemoryRequirements2(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferMemoryRequirements2KHR(void *args)
{
    struct vkGetBufferMemoryRequirements2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkBufferMemoryRequirementsInfo2_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetBufferMemoryRequirements2KHR(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetBufferMemoryRequirements2KHR(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferOpaqueCaptureAddress(void *args)
{
    struct vkGetBufferOpaqueCaptureAddress_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetBufferOpaqueCaptureAddress(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetBufferOpaqueCaptureAddress(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetBufferOpaqueCaptureAddressKHR(void *args)
{
    struct vkGetBufferOpaqueCaptureAddressKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetBufferOpaqueCaptureAddressKHR(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetBufferOpaqueCaptureAddressKHR(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDeferredOperationMaxConcurrencyKHR(void *args)
{
    struct vkGetDeferredOperationMaxConcurrencyKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));
    return params->device->funcs.p_vkGetDeferredOperationMaxConcurrencyKHR(params->device->device, params->operation);
}

static NTSTATUS wine_vkGetDeferredOperationResultKHR(void *args)
{
    struct vkGetDeferredOperationResultKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));
    return params->device->funcs.p_vkGetDeferredOperationResultKHR(params->device->device, params->operation);
}

static NTSTATUS wine_vkGetDescriptorSetHostMappingVALVE(void *args)
{
    struct vkGetDescriptorSetHostMappingVALVE_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), params->ppData);
    params->device->funcs.p_vkGetDescriptorSetHostMappingVALVE(params->device->device, params->descriptorSet, params->ppData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDescriptorSetLayoutHostMappingInfoVALVE(void *args)
{
    struct vkGetDescriptorSetLayoutHostMappingInfoVALVE_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDescriptorSetBindingReferenceVALVE_host pBindingReference_host;
    TRACE("%p, %p, %p\n", params->device, params->pBindingReference, params->pHostMapping);

    convert_VkDescriptorSetBindingReferenceVALVE_win_to_host(params->pBindingReference, &pBindingReference_host);
    params->device->funcs.p_vkGetDescriptorSetLayoutHostMappingInfoVALVE(params->device->device, &pBindingReference_host, params->pHostMapping);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pBindingReference, params->pHostMapping);
    params->device->funcs.p_vkGetDescriptorSetLayoutHostMappingInfoVALVE(params->device->device, params->pBindingReference, params->pHostMapping);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDescriptorSetLayoutSupport(void *args)
{
    struct vkGetDescriptorSetLayoutSupport_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pSupport);
    params->device->funcs.p_vkGetDescriptorSetLayoutSupport(params->device->device, params->pCreateInfo, params->pSupport);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDescriptorSetLayoutSupportKHR(void *args)
{
    struct vkGetDescriptorSetLayoutSupportKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pSupport);
    params->device->funcs.p_vkGetDescriptorSetLayoutSupportKHR(params->device->device, params->pCreateInfo, params->pSupport);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDeviceAccelerationStructureCompatibilityKHR(void *args)
{
    struct vkGetDeviceAccelerationStructureCompatibilityKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pVersionInfo, params->pCompatibility);
    params->device->funcs.p_vkGetDeviceAccelerationStructureCompatibilityKHR(params->device->device, params->pVersionInfo, params->pCompatibility);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDeviceBufferMemoryRequirements(void *args)
{
    struct vkGetDeviceBufferMemoryRequirements_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDeviceBufferMemoryRequirements_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkDeviceBufferMemoryRequirements_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetDeviceBufferMemoryRequirements(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    free_VkDeviceBufferMemoryRequirements(&pInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetDeviceBufferMemoryRequirements(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDeviceBufferMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceBufferMemoryRequirementsKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDeviceBufferMemoryRequirements_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkDeviceBufferMemoryRequirements_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetDeviceBufferMemoryRequirementsKHR(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    free_VkDeviceBufferMemoryRequirements(&pInfo_host);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetDeviceBufferMemoryRequirementsKHR(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDeviceGroupPeerMemoryFeatures(void *args)
{
    struct vkGetDeviceGroupPeerMemoryFeatures_params *params = args;
    TRACE("%p, %u, %u, %u, %p\n", params->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    params->device->funcs.p_vkGetDeviceGroupPeerMemoryFeatures(params->device->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDeviceGroupPeerMemoryFeaturesKHR(void *args)
{
    struct vkGetDeviceGroupPeerMemoryFeaturesKHR_params *params = args;
    TRACE("%p, %u, %u, %u, %p\n", params->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    params->device->funcs.p_vkGetDeviceGroupPeerMemoryFeaturesKHR(params->device->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDeviceGroupPresentCapabilitiesKHR(void *args)
{
    struct vkGetDeviceGroupPresentCapabilitiesKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pDeviceGroupPresentCapabilities);
    return params->device->funcs.p_vkGetDeviceGroupPresentCapabilitiesKHR(params->device->device, params->pDeviceGroupPresentCapabilities);
}

static NTSTATUS wine_vkGetDeviceGroupSurfacePresentModesKHR(void *args)
{
    struct vkGetDeviceGroupSurfacePresentModesKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->surface), params->pModes);
    return params->device->funcs.p_vkGetDeviceGroupSurfacePresentModesKHR(params->device->device, wine_surface_from_handle(params->surface)->driver_surface, params->pModes);
}

static NTSTATUS wine_vkGetDeviceImageMemoryRequirements(void *args)
{
    struct vkGetDeviceImageMemoryRequirements_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetDeviceImageMemoryRequirements(params->device->device, params->pInfo, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetDeviceImageMemoryRequirements(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDeviceImageMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceImageMemoryRequirementsKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetDeviceImageMemoryRequirementsKHR(params->device->device, params->pInfo, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetDeviceImageMemoryRequirementsKHR(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDeviceImageSparseMemoryRequirements(void *args)
{
    struct vkGetDeviceImageSparseMemoryRequirements_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    params->device->funcs.p_vkGetDeviceImageSparseMemoryRequirements(params->device->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDeviceImageSparseMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceImageSparseMemoryRequirementsKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    params->device->funcs.p_vkGetDeviceImageSparseMemoryRequirementsKHR(params->device->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDeviceMemoryCommitment(void *args)
{
    struct vkGetDeviceMemoryCommitment_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->memory), params->pCommittedMemoryInBytes);
    params->device->funcs.p_vkGetDeviceMemoryCommitment(params->device->device, params->memory, params->pCommittedMemoryInBytes);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetDeviceMemoryOpaqueCaptureAddress(void *args)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddress_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDeviceMemoryOpaqueCaptureAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddress(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddress(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDeviceMemoryOpaqueCaptureAddressKHR(void *args)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddressKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDeviceMemoryOpaqueCaptureAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win_to_host(params->pInfo, &pInfo_host);
    params->result = params->device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(params->device->device, &pInfo_host);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    params->result = params->device->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(params->device->device, params->pInfo);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(void *args)
{
    struct vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderpass), params->pMaxWorkgroupSize);
    return params->device->funcs.p_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(params->device->device, params->renderpass, params->pMaxWorkgroupSize);
}

static NTSTATUS wine_vkGetDynamicRenderingTilePropertiesQCOM(void *args)
{
    struct vkGetDynamicRenderingTilePropertiesQCOM_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkRenderingInfo_host pRenderingInfo_host;
    TRACE("%p, %p, %p\n", params->device, params->pRenderingInfo, params->pProperties);

    convert_VkRenderingInfo_win_to_host(params->pRenderingInfo, &pRenderingInfo_host);
    result = params->device->funcs.p_vkGetDynamicRenderingTilePropertiesQCOM(params->device->device, &pRenderingInfo_host, params->pProperties);

    free_VkRenderingInfo(&pRenderingInfo_host);
    return result;
#else
    TRACE("%p, %p, %p\n", params->device, params->pRenderingInfo, params->pProperties);
    return params->device->funcs.p_vkGetDynamicRenderingTilePropertiesQCOM(params->device->device, params->pRenderingInfo, params->pProperties);
#endif
}

static NTSTATUS wine_vkGetEventStatus(void *args)
{
    struct vkGetEventStatus_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));
    return params->device->funcs.p_vkGetEventStatus(params->device->device, params->event);
}

static NTSTATUS wine_vkGetFenceStatus(void *args)
{
    struct vkGetFenceStatus_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->fence));
    return params->device->funcs.p_vkGetFenceStatus(params->device->device, params->fence);
}

static NTSTATUS wine_vkGetFramebufferTilePropertiesQCOM(void *args)
{
    struct vkGetFramebufferTilePropertiesQCOM_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->framebuffer), params->pPropertiesCount, params->pProperties);
    return params->device->funcs.p_vkGetFramebufferTilePropertiesQCOM(params->device->device, params->framebuffer, params->pPropertiesCount, params->pProperties);
}

static NTSTATUS wine_vkGetGeneratedCommandsMemoryRequirementsNV(void *args)
{
    struct vkGetGeneratedCommandsMemoryRequirementsNV_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkGeneratedCommandsMemoryRequirementsInfoNV_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkGeneratedCommandsMemoryRequirementsInfoNV_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetGeneratedCommandsMemoryRequirementsNV(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetGeneratedCommandsMemoryRequirementsNV(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageMemoryRequirements(void *args)
{
    struct vkGetImageMemoryRequirements_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkMemoryRequirements_host pMemoryRequirements_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pMemoryRequirements);

    params->device->funcs.p_vkGetImageMemoryRequirements(params->device->device, params->image, &pMemoryRequirements_host);

    convert_VkMemoryRequirements_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pMemoryRequirements);
    params->device->funcs.p_vkGetImageMemoryRequirements(params->device->device, params->image, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageMemoryRequirements2(void *args)
{
    struct vkGetImageMemoryRequirements2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkImageMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkImageMemoryRequirementsInfo2_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetImageMemoryRequirements2(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetImageMemoryRequirements2(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageMemoryRequirements2KHR(void *args)
{
    struct vkGetImageMemoryRequirements2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkImageMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkImageMemoryRequirementsInfo2_win_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    params->device->funcs.p_vkGetImageMemoryRequirements2KHR(params->device->device, &pInfo_host, &pMemoryRequirements_host);

    convert_VkMemoryRequirements2_host_to_win(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);
    params->device->funcs.p_vkGetImageMemoryRequirements2KHR(params->device->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageSparseMemoryRequirements(void *args)
{
    struct vkGetImageSparseMemoryRequirements_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    params->device->funcs.p_vkGetImageSparseMemoryRequirements(params->device->device, params->image, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetImageSparseMemoryRequirements2(void *args)
{
    struct vkGetImageSparseMemoryRequirements2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkImageSparseMemoryRequirementsInfo2_host pInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    convert_VkImageSparseMemoryRequirementsInfo2_win_to_host(params->pInfo, &pInfo_host);
    params->device->funcs.p_vkGetImageSparseMemoryRequirements2(params->device->device, &pInfo_host, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    params->device->funcs.p_vkGetImageSparseMemoryRequirements2(params->device->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageSparseMemoryRequirements2KHR(void *args)
{
    struct vkGetImageSparseMemoryRequirements2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkImageSparseMemoryRequirementsInfo2_host pInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    convert_VkImageSparseMemoryRequirementsInfo2_win_to_host(params->pInfo, &pInfo_host);
    params->device->funcs.p_vkGetImageSparseMemoryRequirements2KHR(params->device->device, &pInfo_host, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    return STATUS_SUCCESS;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    params->device->funcs.p_vkGetImageSparseMemoryRequirements2KHR(params->device->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageSubresourceLayout(void *args)
{
    struct vkGetImageSubresourceLayout_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkSubresourceLayout_host pLayout_host;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);

    params->device->funcs.p_vkGetImageSubresourceLayout(params->device->device, params->image, params->pSubresource, &pLayout_host);

    convert_VkSubresourceLayout_host_to_win(&pLayout_host, params->pLayout);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);
    params->device->funcs.p_vkGetImageSubresourceLayout(params->device->device, params->image, params->pSubresource, params->pLayout);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageSubresourceLayout2EXT(void *args)
{
    struct vkGetImageSubresourceLayout2EXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkSubresourceLayout2EXT_host pLayout_host;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);

    convert_VkSubresourceLayout2EXT_win_to_host(params->pLayout, &pLayout_host);
    params->device->funcs.p_vkGetImageSubresourceLayout2EXT(params->device->device, params->image, params->pSubresource, &pLayout_host);

    convert_VkSubresourceLayout2EXT_host_to_win(&pLayout_host, params->pLayout);
    return STATUS_SUCCESS;
#else
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);
    params->device->funcs.p_vkGetImageSubresourceLayout2EXT(params->device->device, params->image, params->pSubresource, params->pLayout);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetImageViewAddressNVX(void *args)
{
    struct vkGetImageViewAddressNVX_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageViewAddressPropertiesNVX_host pProperties_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->imageView), params->pProperties);

    convert_VkImageViewAddressPropertiesNVX_win_to_host(params->pProperties, &pProperties_host);
    result = params->device->funcs.p_vkGetImageViewAddressNVX(params->device->device, params->imageView, &pProperties_host);

    convert_VkImageViewAddressPropertiesNVX_host_to_win(&pProperties_host, params->pProperties);
    return result;
#else
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->imageView), params->pProperties);
    return params->device->funcs.p_vkGetImageViewAddressNVX(params->device->device, params->imageView, params->pProperties);
#endif
}

static NTSTATUS wine_vkGetImageViewHandleNVX(void *args)
{
    struct vkGetImageViewHandleNVX_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    uint32_t result;
    VkImageViewHandleInfoNVX_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkImageViewHandleInfoNVX_win_to_host(params->pInfo, &pInfo_host);
    result = params->device->funcs.p_vkGetImageViewHandleNVX(params->device->device, &pInfo_host);

    return result;
#else
    TRACE("%p, %p\n", params->device, params->pInfo);
    return params->device->funcs.p_vkGetImageViewHandleNVX(params->device->device, params->pInfo);
#endif
}

static NTSTATUS wine_vkGetMemoryHostPointerPropertiesEXT(void *args)
{
    struct vkGetMemoryHostPointerPropertiesEXT_params *params = args;
    TRACE("%p, %#x, %p, %p\n", params->device, params->handleType, params->pHostPointer, params->pMemoryHostPointerProperties);
    return params->device->funcs.p_vkGetMemoryHostPointerPropertiesEXT(params->device->device, params->handleType, params->pHostPointer, params->pMemoryHostPointerProperties);
}

static NTSTATUS wine_vkGetPerformanceParameterINTEL(void *args)
{
    struct vkGetPerformanceParameterINTEL_params *params = args;
    TRACE("%p, %#x, %p\n", params->device, params->parameter, params->pValue);
    return params->device->funcs.p_vkGetPerformanceParameterINTEL(params->device->device, params->parameter, params->pValue);
}

static NTSTATUS wine_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(void *args)
{
    struct vkGetPhysicalDeviceCooperativeMatrixPropertiesNV_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPropertyCount, params->pProperties);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(params->physicalDevice->phys_dev, params->pPropertyCount, params->pProperties);
}

static NTSTATUS wine_vkGetPhysicalDeviceFeatures(void *args)
{
    struct vkGetPhysicalDeviceFeatures_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFeatures(params->physicalDevice->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceFeatures2(void *args)
{
    struct vkGetPhysicalDeviceFeatures2_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFeatures2(params->physicalDevice->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceFeatures2KHR(void *args)
{
    struct vkGetPhysicalDeviceFeatures2KHR_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFeatures2KHR(params->physicalDevice->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFormatProperties(params->physicalDevice->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties2_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2(params->physicalDevice->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties2KHR_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2KHR(params->physicalDevice->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceFragmentShadingRatesKHR(void *args)
{
    struct vkGetPhysicalDeviceFragmentShadingRatesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pFragmentShadingRateCount, params->pFragmentShadingRates);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFragmentShadingRatesKHR(params->physicalDevice->phys_dev, params->pFragmentShadingRateCount, params->pFragmentShadingRates);
}

static NTSTATUS wine_vkGetPhysicalDeviceImageFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceImageFormatProperties_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageFormatProperties_host pImageFormatProperties_host;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p\n", params->physicalDevice, params->format, params->type, params->tiling, params->usage, params->flags, params->pImageFormatProperties);

    result = params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(params->physicalDevice->phys_dev, params->format, params->type, params->tiling, params->usage, params->flags, &pImageFormatProperties_host);

    convert_VkImageFormatProperties_host_to_win(&pImageFormatProperties_host, params->pImageFormatProperties);
    return result;
#else
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p\n", params->physicalDevice, params->format, params->type, params->tiling, params->usage, params->flags, params->pImageFormatProperties);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(params->physicalDevice->phys_dev, params->format, params->type, params->tiling, params->usage, params->flags, params->pImageFormatProperties);
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

static NTSTATUS wine_vkGetPhysicalDeviceMemoryProperties(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceMemoryProperties_host pMemoryProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties(params->physicalDevice->phys_dev, &pMemoryProperties_host);

    convert_VkPhysicalDeviceMemoryProperties_host_to_win(&pMemoryProperties_host, params->pMemoryProperties);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties(params->physicalDevice->phys_dev, params->pMemoryProperties);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetPhysicalDeviceMemoryProperties2(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceMemoryProperties2_host pMemoryProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    convert_VkPhysicalDeviceMemoryProperties2_win_to_host(params->pMemoryProperties, &pMemoryProperties_host);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2(params->physicalDevice->phys_dev, &pMemoryProperties_host);

    convert_VkPhysicalDeviceMemoryProperties2_host_to_win(&pMemoryProperties_host, params->pMemoryProperties);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2(params->physicalDevice->phys_dev, params->pMemoryProperties);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetPhysicalDeviceMemoryProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceMemoryProperties2_host pMemoryProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    convert_VkPhysicalDeviceMemoryProperties2_win_to_host(params->pMemoryProperties, &pMemoryProperties_host);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2KHR(params->physicalDevice->phys_dev, &pMemoryProperties_host);

    convert_VkPhysicalDeviceMemoryProperties2_host_to_win(&pMemoryProperties_host, params->pMemoryProperties);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2KHR(params->physicalDevice->phys_dev, params->pMemoryProperties);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetPhysicalDeviceMultisamplePropertiesEXT(void *args)
{
    struct vkGetPhysicalDeviceMultisamplePropertiesEXT_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->samples, params->pMultisampleProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceMultisamplePropertiesEXT(params->physicalDevice->phys_dev, params->samples, params->pMultisampleProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDevicePresentRectanglesKHR(void *args)
{
    struct vkGetPhysicalDevicePresentRectanglesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pRectCount, params->pRects);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDevicePresentRectanglesKHR(params->physicalDevice->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pRectCount, params->pRects);
}

static NTSTATUS wine_vkGetPhysicalDeviceProperties(void *args)
{
    struct vkGetPhysicalDeviceProperties_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceProperties_host pProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties(params->physicalDevice->phys_dev, &pProperties_host);

    convert_VkPhysicalDeviceProperties_host_to_win(&pProperties_host, params->pProperties);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties(params->physicalDevice->phys_dev, params->pProperties);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetPhysicalDeviceProperties2(void *args)
{
    struct vkGetPhysicalDeviceProperties2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceProperties2_host pProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    convert_VkPhysicalDeviceProperties2_win_to_host(params->pProperties, &pProperties_host);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2(params->physicalDevice->phys_dev, &pProperties_host);

    convert_VkPhysicalDeviceProperties2_host_to_win(&pProperties_host, params->pProperties);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2(params->physicalDevice->phys_dev, params->pProperties);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetPhysicalDeviceProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceProperties2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkPhysicalDeviceProperties2_host pProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    convert_VkPhysicalDeviceProperties2_win_to_host(params->pProperties, &pProperties_host);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2KHR(params->physicalDevice->phys_dev, &pProperties_host);

    convert_VkPhysicalDeviceProperties2_host_to_win(&pProperties_host, params->pProperties);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceProperties2KHR(params->physicalDevice->phys_dev, params->pProperties);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPerformanceQueryCreateInfo, params->pNumPasses);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(params->physicalDevice->phys_dev, params->pPerformanceQueryCreateInfo, params->pNumPasses);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceQueueFamilyProperties(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties(params->physicalDevice->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceQueueFamilyProperties2(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2(params->physicalDevice->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceQueueFamilyProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2KHR(params->physicalDevice->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceSparseImageFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p, %p\n", params->physicalDevice, params->format, params->type, params->samples, params->usage, params->tiling, params->pPropertyCount, params->pProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties(params->physicalDevice->phys_dev, params->format, params->type, params->samples, params->usage, params->tiling, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceSparseImageFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2(params->physicalDevice->phys_dev, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2KHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(params->physicalDevice->phys_dev, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(void *args)
{
    struct vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pCombinationCount, params->pCombinations);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(params->physicalDevice->phys_dev, params->pCombinationCount, params->pCombinations);
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
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR pSurfaceInfo_host;
    convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(pSurfaceInfo, &pSurfaceInfo_host);
    result = physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice->phys_dev, &pSurfaceInfo_host, pSurfaceCapabilities);

    return result;
#endif
}

VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->phys_dev, wine_surface_from_handle(surface)->driver_surface, pSurfaceCapabilities);
}

static NTSTATUS wine_vkGetPhysicalDeviceSurfaceFormats2KHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceFormats2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR_host pSurfaceInfo_host;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pSurfaceInfo, params->pSurfaceFormatCount, params->pSurfaceFormats);

    convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(params->pSurfaceInfo, &pSurfaceInfo_host);
    result = params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR(params->physicalDevice->phys_dev, &pSurfaceInfo_host, params->pSurfaceFormatCount, params->pSurfaceFormats);

    return result;
#else
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR pSurfaceInfo_host;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pSurfaceInfo, params->pSurfaceFormatCount, params->pSurfaceFormats);

    convert_VkPhysicalDeviceSurfaceInfo2KHR_win_to_host(params->pSurfaceInfo, &pSurfaceInfo_host);
    result = params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR(params->physicalDevice->phys_dev, &pSurfaceInfo_host, params->pSurfaceFormatCount, params->pSurfaceFormats);

    return result;
#endif
}

static NTSTATUS wine_vkGetPhysicalDeviceSurfaceFormatsKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceFormatsKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pSurfaceFormatCount, params->pSurfaceFormats);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormatsKHR(params->physicalDevice->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pSurfaceFormatCount, params->pSurfaceFormats);
}

static NTSTATUS wine_vkGetPhysicalDeviceSurfacePresentModesKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfacePresentModesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pPresentModeCount, params->pPresentModes);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfacePresentModesKHR(params->physicalDevice->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pPresentModeCount, params->pPresentModes);
}

static NTSTATUS wine_vkGetPhysicalDeviceSurfaceSupportKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceSupportKHR_params *params = args;
    TRACE("%p, %u, 0x%s, %p\n", params->physicalDevice, params->queueFamilyIndex, wine_dbgstr_longlong(params->surface), params->pSupported);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSurfaceSupportKHR(params->physicalDevice->phys_dev, params->queueFamilyIndex, wine_surface_from_handle(params->surface)->driver_surface, params->pSupported);
}

static NTSTATUS wine_vkGetPhysicalDeviceToolProperties(void *args)
{
    struct vkGetPhysicalDeviceToolProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pToolCount, params->pToolProperties);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceToolProperties(params->physicalDevice->phys_dev, params->pToolCount, params->pToolProperties);
}

static NTSTATUS wine_vkGetPhysicalDeviceToolPropertiesEXT(void *args)
{
    struct vkGetPhysicalDeviceToolPropertiesEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pToolCount, params->pToolProperties);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceToolPropertiesEXT(params->physicalDevice->phys_dev, params->pToolCount, params->pToolProperties);
}

static NTSTATUS wine_vkGetPhysicalDeviceWin32PresentationSupportKHR(void *args)
{
    struct vkGetPhysicalDeviceWin32PresentationSupportKHR_params *params = args;
    TRACE("%p, %u\n", params->physicalDevice, params->queueFamilyIndex);
    return params->physicalDevice->instance->funcs.p_vkGetPhysicalDeviceWin32PresentationSupportKHR(params->physicalDevice->phys_dev, params->queueFamilyIndex);
}

static NTSTATUS wine_vkGetPipelineCacheData(void *args)
{
    struct vkGetPipelineCacheData_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->pDataSize, params->pData);
    return params->device->funcs.p_vkGetPipelineCacheData(params->device->device, params->pipelineCache, params->pDataSize, params->pData);
}

static NTSTATUS wine_vkGetPipelineExecutableInternalRepresentationsKHR(void *args)
{
    struct vkGetPipelineExecutableInternalRepresentationsKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPipelineExecutableInfoKHR_host pExecutableInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pInternalRepresentationCount, params->pInternalRepresentations);

    convert_VkPipelineExecutableInfoKHR_win_to_host(params->pExecutableInfo, &pExecutableInfo_host);
    result = params->device->funcs.p_vkGetPipelineExecutableInternalRepresentationsKHR(params->device->device, &pExecutableInfo_host, params->pInternalRepresentationCount, params->pInternalRepresentations);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pInternalRepresentationCount, params->pInternalRepresentations);
    return params->device->funcs.p_vkGetPipelineExecutableInternalRepresentationsKHR(params->device->device, params->pExecutableInfo, params->pInternalRepresentationCount, params->pInternalRepresentations);
#endif
}

static NTSTATUS wine_vkGetPipelineExecutablePropertiesKHR(void *args)
{
    struct vkGetPipelineExecutablePropertiesKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPipelineInfoKHR_host pPipelineInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pPipelineInfo, params->pExecutableCount, params->pProperties);

    convert_VkPipelineInfoKHR_win_to_host(params->pPipelineInfo, &pPipelineInfo_host);
    result = params->device->funcs.p_vkGetPipelineExecutablePropertiesKHR(params->device->device, &pPipelineInfo_host, params->pExecutableCount, params->pProperties);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pPipelineInfo, params->pExecutableCount, params->pProperties);
    return params->device->funcs.p_vkGetPipelineExecutablePropertiesKHR(params->device->device, params->pPipelineInfo, params->pExecutableCount, params->pProperties);
#endif
}

static NTSTATUS wine_vkGetPipelineExecutableStatisticsKHR(void *args)
{
    struct vkGetPipelineExecutableStatisticsKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPipelineExecutableInfoKHR_host pExecutableInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pStatisticCount, params->pStatistics);

    convert_VkPipelineExecutableInfoKHR_win_to_host(params->pExecutableInfo, &pExecutableInfo_host);
    result = params->device->funcs.p_vkGetPipelineExecutableStatisticsKHR(params->device->device, &pExecutableInfo_host, params->pStatisticCount, params->pStatistics);

    return result;
#else
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pStatisticCount, params->pStatistics);
    return params->device->funcs.p_vkGetPipelineExecutableStatisticsKHR(params->device->device, params->pExecutableInfo, params->pStatisticCount, params->pStatistics);
#endif
}

static NTSTATUS wine_vkGetPipelinePropertiesEXT(void *args)
{
    struct vkGetPipelinePropertiesEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkPipelineInfoEXT_host pPipelineInfo_host;
    TRACE("%p, %p, %p\n", params->device, params->pPipelineInfo, params->pPipelineProperties);

    convert_VkPipelineInfoEXT_win_to_host(params->pPipelineInfo, &pPipelineInfo_host);
    result = params->device->funcs.p_vkGetPipelinePropertiesEXT(params->device->device, &pPipelineInfo_host, params->pPipelineProperties);

    return result;
#else
    TRACE("%p, %p, %p\n", params->device, params->pPipelineInfo, params->pPipelineProperties);
    return params->device->funcs.p_vkGetPipelinePropertiesEXT(params->device->device, params->pPipelineInfo, params->pPipelineProperties);
#endif
}

static NTSTATUS wine_vkGetPrivateData(void *args)
{
    struct vkGetPrivateData_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %p\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), params->pData);
    params->device->funcs.p_vkGetPrivateData(params->device->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->pData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetPrivateDataEXT(void *args)
{
    struct vkGetPrivateDataEXT_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %p\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), params->pData);
    params->device->funcs.p_vkGetPrivateDataEXT(params->device->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->pData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetQueryPoolResults(void *args)
{
    struct vkGetQueryPoolResults_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride), params->flags);
    return params->device->funcs.p_vkGetQueryPoolResults(params->device->device, params->queryPool, params->firstQuery, params->queryCount, params->dataSize, params->pData, params->stride, params->flags);
}

static NTSTATUS wine_vkGetQueueCheckpointData2NV(void *args)
{
    struct vkGetQueueCheckpointData2NV_params *params = args;
    TRACE("%p, %p, %p\n", params->queue, params->pCheckpointDataCount, params->pCheckpointData);
    params->queue->device->funcs.p_vkGetQueueCheckpointData2NV(params->queue->queue, params->pCheckpointDataCount, params->pCheckpointData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetQueueCheckpointDataNV(void *args)
{
    struct vkGetQueueCheckpointDataNV_params *params = args;
    TRACE("%p, %p, %p\n", params->queue, params->pCheckpointDataCount, params->pCheckpointData);
    params->queue->device->funcs.p_vkGetQueueCheckpointDataNV(params->queue->queue, params->pCheckpointDataCount, params->pCheckpointData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(void *args)
{
    struct vkGetRayTracingCaptureReplayShaderGroupHandlesKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);
    return params->device->funcs.p_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(params->device->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
}

static NTSTATUS wine_vkGetRayTracingShaderGroupHandlesKHR(void *args)
{
    struct vkGetRayTracingShaderGroupHandlesKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);
    return params->device->funcs.p_vkGetRayTracingShaderGroupHandlesKHR(params->device->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
}

static NTSTATUS wine_vkGetRayTracingShaderGroupHandlesNV(void *args)
{
    struct vkGetRayTracingShaderGroupHandlesNV_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);
    return params->device->funcs.p_vkGetRayTracingShaderGroupHandlesNV(params->device->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
}

static NTSTATUS wine_vkGetRayTracingShaderGroupStackSizeKHR(void *args)
{
    struct vkGetRayTracingShaderGroupStackSizeKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %#x\n", params->device, wine_dbgstr_longlong(params->pipeline), params->group, params->groupShader);
    return params->device->funcs.p_vkGetRayTracingShaderGroupStackSizeKHR(params->device->device, params->pipeline, params->group, params->groupShader);
}

static NTSTATUS wine_vkGetRenderAreaGranularity(void *args)
{
    struct vkGetRenderAreaGranularity_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderPass), params->pGranularity);
    params->device->funcs.p_vkGetRenderAreaGranularity(params->device->device, params->renderPass, params->pGranularity);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetSemaphoreCounterValue(void *args)
{
    struct vkGetSemaphoreCounterValue_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pValue);
    return params->device->funcs.p_vkGetSemaphoreCounterValue(params->device->device, params->semaphore, params->pValue);
}

static NTSTATUS wine_vkGetSemaphoreCounterValueKHR(void *args)
{
    struct vkGetSemaphoreCounterValueKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pValue);
    return params->device->funcs.p_vkGetSemaphoreCounterValueKHR(params->device->device, params->semaphore, params->pValue);
}

static NTSTATUS wine_vkGetShaderInfoAMD(void *args)
{
    struct vkGetShaderInfoAMD_params *params = args;
    TRACE("%p, 0x%s, %#x, %#x, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->shaderStage, params->infoType, params->pInfoSize, params->pInfo);
    return params->device->funcs.p_vkGetShaderInfoAMD(params->device->device, params->pipeline, params->shaderStage, params->infoType, params->pInfoSize, params->pInfo);
}

static NTSTATUS wine_vkGetShaderModuleCreateInfoIdentifierEXT(void *args)
{
    struct vkGetShaderModuleCreateInfoIdentifierEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pIdentifier);
    params->device->funcs.p_vkGetShaderModuleCreateInfoIdentifierEXT(params->device->device, params->pCreateInfo, params->pIdentifier);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetShaderModuleIdentifierEXT(void *args)
{
    struct vkGetShaderModuleIdentifierEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->shaderModule), params->pIdentifier);
    params->device->funcs.p_vkGetShaderModuleIdentifierEXT(params->device->device, params->shaderModule, params->pIdentifier);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkGetSwapchainImagesKHR(void *args)
{
    struct vkGetSwapchainImagesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), params->pSwapchainImageCount, params->pSwapchainImages);
    return params->device->funcs.p_vkGetSwapchainImagesKHR(params->device->device, params->swapchain, params->pSwapchainImageCount, params->pSwapchainImages);
}

static NTSTATUS wine_vkGetValidationCacheDataEXT(void *args)
{
    struct vkGetValidationCacheDataEXT_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->validationCache), params->pDataSize, params->pData);
    return params->device->funcs.p_vkGetValidationCacheDataEXT(params->device->device, params->validationCache, params->pDataSize, params->pData);
}

static NTSTATUS wine_vkInitializePerformanceApiINTEL(void *args)
{
    struct vkInitializePerformanceApiINTEL_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInitializeInfo);
    return params->device->funcs.p_vkInitializePerformanceApiINTEL(params->device->device, params->pInitializeInfo);
}

static NTSTATUS wine_vkInvalidateMappedMemoryRanges(void *args)
{
    struct vkInvalidateMappedMemoryRanges_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkMappedMemoryRange_host *pMemoryRanges_host;
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);

    pMemoryRanges_host = convert_VkMappedMemoryRange_array_win_to_host(params->pMemoryRanges, params->memoryRangeCount);
    result = params->device->funcs.p_vkInvalidateMappedMemoryRanges(params->device->device, params->memoryRangeCount, pMemoryRanges_host);

    free_VkMappedMemoryRange_array(pMemoryRanges_host, params->memoryRangeCount);
    return result;
#else
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);
    return params->device->funcs.p_vkInvalidateMappedMemoryRanges(params->device->device, params->memoryRangeCount, params->pMemoryRanges);
#endif
}

static NTSTATUS wine_vkMapMemory(void *args)
{
    struct vkMapMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %#x, %p\n", params->device, wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->size), params->flags, params->ppData);
    return params->device->funcs.p_vkMapMemory(params->device->device, params->memory, params->offset, params->size, params->flags, params->ppData);
}

static NTSTATUS wine_vkMergePipelineCaches(void *args)
{
    struct vkMergePipelineCaches_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->dstCache), params->srcCacheCount, params->pSrcCaches);
    return params->device->funcs.p_vkMergePipelineCaches(params->device->device, params->dstCache, params->srcCacheCount, params->pSrcCaches);
}

static NTSTATUS wine_vkMergeValidationCachesEXT(void *args)
{
    struct vkMergeValidationCachesEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->dstCache), params->srcCacheCount, params->pSrcCaches);
    return params->device->funcs.p_vkMergeValidationCachesEXT(params->device->device, params->dstCache, params->srcCacheCount, params->pSrcCaches);
}

static NTSTATUS wine_vkQueueBeginDebugUtilsLabelEXT(void *args)
{
    struct vkQueueBeginDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pLabelInfo);
    params->queue->device->funcs.p_vkQueueBeginDebugUtilsLabelEXT(params->queue->queue, params->pLabelInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkQueueBindSparse(void *args)
{
    struct vkQueueBindSparse_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkBindSparseInfo_host *pBindInfo_host;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->bindInfoCount, params->pBindInfo, wine_dbgstr_longlong(params->fence));

    pBindInfo_host = convert_VkBindSparseInfo_array_win_to_host(params->pBindInfo, params->bindInfoCount);
    result = params->queue->device->funcs.p_vkQueueBindSparse(params->queue->queue, params->bindInfoCount, pBindInfo_host, params->fence);

    free_VkBindSparseInfo_array(pBindInfo_host, params->bindInfoCount);
    return result;
#else
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->bindInfoCount, params->pBindInfo, wine_dbgstr_longlong(params->fence));
    return params->queue->device->funcs.p_vkQueueBindSparse(params->queue->queue, params->bindInfoCount, params->pBindInfo, params->fence);
#endif
}

static NTSTATUS wine_vkQueueEndDebugUtilsLabelEXT(void *args)
{
    struct vkQueueEndDebugUtilsLabelEXT_params *params = args;
    TRACE("%p\n", params->queue);
    params->queue->device->funcs.p_vkQueueEndDebugUtilsLabelEXT(params->queue->queue);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkQueueInsertDebugUtilsLabelEXT(void *args)
{
    struct vkQueueInsertDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pLabelInfo);
    params->queue->device->funcs.p_vkQueueInsertDebugUtilsLabelEXT(params->queue->queue, params->pLabelInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkQueuePresentKHR(void *args)
{
    struct vkQueuePresentKHR_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pPresentInfo);
    return params->queue->device->funcs.p_vkQueuePresentKHR(params->queue->queue, params->pPresentInfo);
}

static NTSTATUS wine_vkQueueSetPerformanceConfigurationINTEL(void *args)
{
    struct vkQueueSetPerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, 0x%s\n", params->queue, wine_dbgstr_longlong(params->configuration));
    return params->queue->device->funcs.p_vkQueueSetPerformanceConfigurationINTEL(params->queue->queue, params->configuration);
}

static NTSTATUS wine_vkQueueSubmit(void *args)
{
    struct vkQueueSubmit_params *params = args;
    VkResult result;
    VkSubmitInfo *pSubmits_host;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    pSubmits_host = convert_VkSubmitInfo_array_win_to_host(params->pSubmits, params->submitCount);
    result = params->queue->device->funcs.p_vkQueueSubmit(params->queue->queue, params->submitCount, pSubmits_host, params->fence);

    free_VkSubmitInfo_array(pSubmits_host, params->submitCount);
    return result;
}

static NTSTATUS wine_vkQueueSubmit2(void *args)
{
    struct vkQueueSubmit2_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSubmitInfo2_host *pSubmits_host;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    pSubmits_host = convert_VkSubmitInfo2_array_win_to_host(params->pSubmits, params->submitCount);
    result = params->queue->device->funcs.p_vkQueueSubmit2(params->queue->queue, params->submitCount, pSubmits_host, params->fence);

    free_VkSubmitInfo2_array(pSubmits_host, params->submitCount);
    return result;
#else
    VkResult result;
    VkSubmitInfo2 *pSubmits_host;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    pSubmits_host = convert_VkSubmitInfo2_array_win_to_host(params->pSubmits, params->submitCount);
    result = params->queue->device->funcs.p_vkQueueSubmit2(params->queue->queue, params->submitCount, pSubmits_host, params->fence);

    free_VkSubmitInfo2_array(pSubmits_host, params->submitCount);
    return result;
#endif
}

static NTSTATUS wine_vkQueueSubmit2KHR(void *args)
{
    struct vkQueueSubmit2KHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSubmitInfo2_host *pSubmits_host;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    pSubmits_host = convert_VkSubmitInfo2_array_win_to_host(params->pSubmits, params->submitCount);
    result = params->queue->device->funcs.p_vkQueueSubmit2KHR(params->queue->queue, params->submitCount, pSubmits_host, params->fence);

    free_VkSubmitInfo2_array(pSubmits_host, params->submitCount);
    return result;
#else
    VkResult result;
    VkSubmitInfo2 *pSubmits_host;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    pSubmits_host = convert_VkSubmitInfo2_array_win_to_host(params->pSubmits, params->submitCount);
    result = params->queue->device->funcs.p_vkQueueSubmit2KHR(params->queue->queue, params->submitCount, pSubmits_host, params->fence);

    free_VkSubmitInfo2_array(pSubmits_host, params->submitCount);
    return result;
#endif
}

static NTSTATUS wine_vkQueueWaitIdle(void *args)
{
    struct vkQueueWaitIdle_params *params = args;
    TRACE("%p\n", params->queue);
    return params->queue->device->funcs.p_vkQueueWaitIdle(params->queue->queue);
}

static NTSTATUS wine_vkReleasePerformanceConfigurationINTEL(void *args)
{
    struct vkReleasePerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->configuration));
    return params->device->funcs.p_vkReleasePerformanceConfigurationINTEL(params->device->device, params->configuration);
}

static NTSTATUS wine_vkReleaseProfilingLockKHR(void *args)
{
    struct vkReleaseProfilingLockKHR_params *params = args;
    TRACE("%p\n", params->device);
    params->device->funcs.p_vkReleaseProfilingLockKHR(params->device->device);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkResetCommandBuffer(void *args)
{
    struct vkResetCommandBuffer_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->flags);
    return params->commandBuffer->device->funcs.p_vkResetCommandBuffer(params->commandBuffer->command_buffer, params->flags);
}

static NTSTATUS wine_vkResetCommandPool(void *args)
{
    struct vkResetCommandPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);
    return params->device->funcs.p_vkResetCommandPool(params->device->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
}

static NTSTATUS wine_vkResetDescriptorPool(void *args)
{
    struct vkResetDescriptorPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->flags);
    return params->device->funcs.p_vkResetDescriptorPool(params->device->device, params->descriptorPool, params->flags);
}

static NTSTATUS wine_vkResetEvent(void *args)
{
    struct vkResetEvent_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));
    return params->device->funcs.p_vkResetEvent(params->device->device, params->event);
}

static NTSTATUS wine_vkResetFences(void *args)
{
    struct vkResetFences_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->fenceCount, params->pFences);
    return params->device->funcs.p_vkResetFences(params->device->device, params->fenceCount, params->pFences);
}

static NTSTATUS wine_vkResetQueryPool(void *args)
{
    struct vkResetQueryPool_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);
    params->device->funcs.p_vkResetQueryPool(params->device->device, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkResetQueryPoolEXT(void *args)
{
    struct vkResetQueryPoolEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);
    params->device->funcs.p_vkResetQueryPoolEXT(params->device->device, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkSetDebugUtilsObjectNameEXT(void *args)
{
    struct vkSetDebugUtilsObjectNameEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugUtilsObjectNameInfoEXT_host pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugUtilsObjectNameInfoEXT_win_to_host(params->pNameInfo, &pNameInfo_host);
    result = params->device->funcs.p_vkSetDebugUtilsObjectNameEXT(params->device->device, &pNameInfo_host);

    return result;
#else
    VkResult result;
    VkDebugUtilsObjectNameInfoEXT pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugUtilsObjectNameInfoEXT_win_to_host(params->pNameInfo, &pNameInfo_host);
    result = params->device->funcs.p_vkSetDebugUtilsObjectNameEXT(params->device->device, &pNameInfo_host);

    return result;
#endif
}

static NTSTATUS wine_vkSetDebugUtilsObjectTagEXT(void *args)
{
    struct vkSetDebugUtilsObjectTagEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkDebugUtilsObjectTagInfoEXT_host pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugUtilsObjectTagInfoEXT_win_to_host(params->pTagInfo, &pTagInfo_host);
    result = params->device->funcs.p_vkSetDebugUtilsObjectTagEXT(params->device->device, &pTagInfo_host);

    return result;
#else
    VkResult result;
    VkDebugUtilsObjectTagInfoEXT pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugUtilsObjectTagInfoEXT_win_to_host(params->pTagInfo, &pTagInfo_host);
    result = params->device->funcs.p_vkSetDebugUtilsObjectTagEXT(params->device->device, &pTagInfo_host);

    return result;
#endif
}

static NTSTATUS wine_vkSetDeviceMemoryPriorityEXT(void *args)
{
    struct vkSetDeviceMemoryPriorityEXT_params *params = args;
    TRACE("%p, 0x%s, %f\n", params->device, wine_dbgstr_longlong(params->memory), params->priority);
    params->device->funcs.p_vkSetDeviceMemoryPriorityEXT(params->device->device, params->memory, params->priority);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkSetEvent(void *args)
{
    struct vkSetEvent_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));
    return params->device->funcs.p_vkSetEvent(params->device->device, params->event);
}

static NTSTATUS wine_vkSetPrivateData(void *args)
{
    struct vkSetPrivateData_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, 0x%s\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), wine_dbgstr_longlong(params->data));
    return params->device->funcs.p_vkSetPrivateData(params->device->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->data);
}

static NTSTATUS wine_vkSetPrivateDataEXT(void *args)
{
    struct vkSetPrivateDataEXT_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, 0x%s\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), wine_dbgstr_longlong(params->data));
    return params->device->funcs.p_vkSetPrivateDataEXT(params->device->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->data);
}

static NTSTATUS wine_vkSignalSemaphore(void *args)
{
    struct vkSignalSemaphore_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSemaphoreSignalInfo_host pSignalInfo_host;
    TRACE("%p, %p\n", params->device, params->pSignalInfo);

    convert_VkSemaphoreSignalInfo_win_to_host(params->pSignalInfo, &pSignalInfo_host);
    result = params->device->funcs.p_vkSignalSemaphore(params->device->device, &pSignalInfo_host);

    return result;
#else
    TRACE("%p, %p\n", params->device, params->pSignalInfo);
    return params->device->funcs.p_vkSignalSemaphore(params->device->device, params->pSignalInfo);
#endif
}

static NTSTATUS wine_vkSignalSemaphoreKHR(void *args)
{
    struct vkSignalSemaphoreKHR_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkSemaphoreSignalInfo_host pSignalInfo_host;
    TRACE("%p, %p\n", params->device, params->pSignalInfo);

    convert_VkSemaphoreSignalInfo_win_to_host(params->pSignalInfo, &pSignalInfo_host);
    result = params->device->funcs.p_vkSignalSemaphoreKHR(params->device->device, &pSignalInfo_host);

    return result;
#else
    TRACE("%p, %p\n", params->device, params->pSignalInfo);
    return params->device->funcs.p_vkSignalSemaphoreKHR(params->device->device, params->pSignalInfo);
#endif
}

static NTSTATUS wine_vkSubmitDebugUtilsMessageEXT(void *args)
{
    struct vkSubmitDebugUtilsMessageEXT_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkDebugUtilsMessengerCallbackDataEXT_host pCallbackData_host;
    TRACE("%p, %#x, %#x, %p\n", params->instance, params->messageSeverity, params->messageTypes, params->pCallbackData);

    convert_VkDebugUtilsMessengerCallbackDataEXT_win_to_host(params->pCallbackData, &pCallbackData_host);
    params->instance->funcs.p_vkSubmitDebugUtilsMessageEXT(params->instance->instance, params->messageSeverity, params->messageTypes, &pCallbackData_host);

    free_VkDebugUtilsMessengerCallbackDataEXT(&pCallbackData_host);
    return STATUS_SUCCESS;
#else
    VkDebugUtilsMessengerCallbackDataEXT pCallbackData_host;
    TRACE("%p, %#x, %#x, %p\n", params->instance, params->messageSeverity, params->messageTypes, params->pCallbackData);

    convert_VkDebugUtilsMessengerCallbackDataEXT_win_to_host(params->pCallbackData, &pCallbackData_host);
    params->instance->funcs.p_vkSubmitDebugUtilsMessageEXT(params->instance->instance, params->messageSeverity, params->messageTypes, &pCallbackData_host);

    free_VkDebugUtilsMessengerCallbackDataEXT(&pCallbackData_host);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkTrimCommandPool(void *args)
{
    struct vkTrimCommandPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);
    params->device->funcs.p_vkTrimCommandPool(params->device->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkTrimCommandPoolKHR(void *args)
{
    struct vkTrimCommandPoolKHR_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);
    params->device->funcs.p_vkTrimCommandPoolKHR(params->device->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkUninitializePerformanceApiINTEL(void *args)
{
    struct vkUninitializePerformanceApiINTEL_params *params = args;
    TRACE("%p\n", params->device);
    params->device->funcs.p_vkUninitializePerformanceApiINTEL(params->device->device);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkUnmapMemory(void *args)
{
    struct vkUnmapMemory_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->memory));
    params->device->funcs.p_vkUnmapMemory(params->device->device, params->memory);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkUpdateDescriptorSetWithTemplate(void *args)
{
    struct vkUpdateDescriptorSetWithTemplate_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pData);
    params->device->funcs.p_vkUpdateDescriptorSetWithTemplate(params->device->device, params->descriptorSet, params->descriptorUpdateTemplate, params->pData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkUpdateDescriptorSetWithTemplateKHR(void *args)
{
    struct vkUpdateDescriptorSetWithTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pData);
    params->device->funcs.p_vkUpdateDescriptorSetWithTemplateKHR(params->device->device, params->descriptorSet, params->descriptorUpdateTemplate, params->pData);
    return STATUS_SUCCESS;
}

static NTSTATUS wine_vkUpdateDescriptorSets(void *args)
{
    struct vkUpdateDescriptorSets_params *params = args;
#if defined(USE_STRUCT_CONVERSION)
    VkWriteDescriptorSet_host *pDescriptorWrites_host;
    VkCopyDescriptorSet_host *pDescriptorCopies_host;
    TRACE("%p, %u, %p, %u, %p\n", params->device, params->descriptorWriteCount, params->pDescriptorWrites, params->descriptorCopyCount, params->pDescriptorCopies);

    pDescriptorWrites_host = convert_VkWriteDescriptorSet_array_win_to_host(params->pDescriptorWrites, params->descriptorWriteCount);
    pDescriptorCopies_host = convert_VkCopyDescriptorSet_array_win_to_host(params->pDescriptorCopies, params->descriptorCopyCount);
    params->device->funcs.p_vkUpdateDescriptorSets(params->device->device, params->descriptorWriteCount, pDescriptorWrites_host, params->descriptorCopyCount, pDescriptorCopies_host);

    free_VkWriteDescriptorSet_array(pDescriptorWrites_host, params->descriptorWriteCount);
    free_VkCopyDescriptorSet_array(pDescriptorCopies_host, params->descriptorCopyCount);
    return STATUS_SUCCESS;
#else
    TRACE("%p, %u, %p, %u, %p\n", params->device, params->descriptorWriteCount, params->pDescriptorWrites, params->descriptorCopyCount, params->pDescriptorCopies);
    params->device->funcs.p_vkUpdateDescriptorSets(params->device->device, params->descriptorWriteCount, params->pDescriptorWrites, params->descriptorCopyCount, params->pDescriptorCopies);
    return STATUS_SUCCESS;
#endif
}

static NTSTATUS wine_vkWaitForFences(void *args)
{
    struct vkWaitForFences_params *params = args;
    TRACE("%p, %u, %p, %u, 0x%s\n", params->device, params->fenceCount, params->pFences, params->waitAll, wine_dbgstr_longlong(params->timeout));
    return params->device->funcs.p_vkWaitForFences(params->device->device, params->fenceCount, params->pFences, params->waitAll, params->timeout);
}

static NTSTATUS wine_vkWaitForPresentKHR(void *args)
{
    struct vkWaitForPresentKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->swapchain), wine_dbgstr_longlong(params->presentId), wine_dbgstr_longlong(params->timeout));
    return params->device->funcs.p_vkWaitForPresentKHR(params->device->device, params->swapchain, params->presentId, params->timeout);
}

static NTSTATUS wine_vkWaitSemaphores(void *args)
{
    struct vkWaitSemaphores_params *params = args;
    TRACE("%p, %p, 0x%s\n", params->device, params->pWaitInfo, wine_dbgstr_longlong(params->timeout));
    return params->device->funcs.p_vkWaitSemaphores(params->device->device, params->pWaitInfo, params->timeout);
}

static NTSTATUS wine_vkWaitSemaphoresKHR(void *args)
{
    struct vkWaitSemaphoresKHR_params *params = args;
    TRACE("%p, %p, 0x%s\n", params->device, params->pWaitInfo, wine_dbgstr_longlong(params->timeout));
    return params->device->funcs.p_vkWaitSemaphoresKHR(params->device->device, params->pWaitInfo, params->timeout);
}

static NTSTATUS wine_vkWriteAccelerationStructuresPropertiesKHR(void *args)
{
    struct vkWriteAccelerationStructuresPropertiesKHR_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %p, 0x%s\n", params->device, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride));
    return params->device->funcs.p_vkWriteAccelerationStructuresPropertiesKHR(params->device->device, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->dataSize, params->pData, params->stride);
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
    "VK_AMD_shader_early_and_late_fragment_tests",
    "VK_AMD_shader_explicit_vertex_parameter",
    "VK_AMD_shader_fragment_mask",
    "VK_AMD_shader_image_load_store_lod",
    "VK_AMD_shader_info",
    "VK_AMD_shader_trinary_minmax",
    "VK_AMD_texture_gather_bias_lod",
    "VK_ARM_rasterization_order_attachment_access",
    "VK_EXT_4444_formats",
    "VK_EXT_astc_decode_mode",
    "VK_EXT_attachment_feedback_loop_layout",
    "VK_EXT_blend_operation_advanced",
    "VK_EXT_border_color_swizzle",
    "VK_EXT_buffer_device_address",
    "VK_EXT_calibrated_timestamps",
    "VK_EXT_color_write_enable",
    "VK_EXT_conditional_rendering",
    "VK_EXT_conservative_rasterization",
    "VK_EXT_custom_border_color",
    "VK_EXT_debug_marker",
    "VK_EXT_depth_clip_control",
    "VK_EXT_depth_clip_enable",
    "VK_EXT_depth_range_unrestricted",
    "VK_EXT_descriptor_indexing",
    "VK_EXT_discard_rectangles",
    "VK_EXT_extended_dynamic_state",
    "VK_EXT_extended_dynamic_state2",
    "VK_EXT_external_memory_host",
    "VK_EXT_filter_cubic",
    "VK_EXT_fragment_density_map",
    "VK_EXT_fragment_density_map2",
    "VK_EXT_fragment_shader_interlock",
    "VK_EXT_global_priority",
    "VK_EXT_global_priority_query",
    "VK_EXT_graphics_pipeline_library",
    "VK_EXT_host_query_reset",
    "VK_EXT_image_2d_view_of_3d",
    "VK_EXT_image_compression_control",
    "VK_EXT_image_compression_control_swapchain",
    "VK_EXT_image_robustness",
    "VK_EXT_image_view_min_lod",
    "VK_EXT_index_type_uint8",
    "VK_EXT_inline_uniform_block",
    "VK_EXT_line_rasterization",
    "VK_EXT_load_store_op_none",
    "VK_EXT_memory_budget",
    "VK_EXT_memory_priority",
    "VK_EXT_multi_draw",
    "VK_EXT_multisampled_render_to_single_sampled",
    "VK_EXT_non_seamless_cube_map",
    "VK_EXT_pageable_device_local_memory",
    "VK_EXT_pci_bus_info",
    "VK_EXT_pipeline_creation_cache_control",
    "VK_EXT_pipeline_creation_feedback",
    "VK_EXT_pipeline_properties",
    "VK_EXT_pipeline_robustness",
    "VK_EXT_post_depth_coverage",
    "VK_EXT_primitive_topology_list_restart",
    "VK_EXT_primitives_generated_query",
    "VK_EXT_private_data",
    "VK_EXT_provoking_vertex",
    "VK_EXT_queue_family_foreign",
    "VK_EXT_rgba10x6_formats",
    "VK_EXT_robustness2",
    "VK_EXT_sample_locations",
    "VK_EXT_sampler_filter_minmax",
    "VK_EXT_scalar_block_layout",
    "VK_EXT_separate_stencil_usage",
    "VK_EXT_shader_atomic_float",
    "VK_EXT_shader_atomic_float2",
    "VK_EXT_shader_demote_to_helper_invocation",
    "VK_EXT_shader_image_atomic_int64",
    "VK_EXT_shader_module_identifier",
    "VK_EXT_shader_stencil_export",
    "VK_EXT_shader_subgroup_ballot",
    "VK_EXT_shader_subgroup_vote",
    "VK_EXT_shader_viewport_index_layer",
    "VK_EXT_subgroup_size_control",
    "VK_EXT_subpass_merge_feedback",
    "VK_EXT_texel_buffer_alignment",
    "VK_EXT_texture_compression_astc_hdr",
    "VK_EXT_tooling_info",
    "VK_EXT_transform_feedback",
    "VK_EXT_validation_cache",
    "VK_EXT_vertex_attribute_divisor",
    "VK_EXT_vertex_input_dynamic_state",
    "VK_EXT_ycbcr_2plane_444_formats",
    "VK_EXT_ycbcr_image_arrays",
    "VK_GOOGLE_decorate_string",
    "VK_GOOGLE_hlsl_functionality1",
    "VK_GOOGLE_user_type",
    "VK_HUAWEI_invocation_mask",
    "VK_HUAWEI_subpass_shading",
    "VK_IMG_filter_cubic",
    "VK_IMG_format_pvrtc",
    "VK_INTEL_performance_query",
    "VK_INTEL_shader_integer_functions2",
    "VK_KHR_16bit_storage",
    "VK_KHR_8bit_storage",
    "VK_KHR_acceleration_structure",
    "VK_KHR_bind_memory2",
    "VK_KHR_buffer_device_address",
    "VK_KHR_copy_commands2",
    "VK_KHR_create_renderpass2",
    "VK_KHR_dedicated_allocation",
    "VK_KHR_deferred_host_operations",
    "VK_KHR_depth_stencil_resolve",
    "VK_KHR_descriptor_update_template",
    "VK_KHR_device_group",
    "VK_KHR_draw_indirect_count",
    "VK_KHR_driver_properties",
    "VK_KHR_dynamic_rendering",
    "VK_KHR_external_fence",
    "VK_KHR_external_memory",
    "VK_KHR_external_semaphore",
    "VK_KHR_format_feature_flags2",
    "VK_KHR_fragment_shader_barycentric",
    "VK_KHR_fragment_shading_rate",
    "VK_KHR_get_memory_requirements2",
    "VK_KHR_global_priority",
    "VK_KHR_image_format_list",
    "VK_KHR_imageless_framebuffer",
    "VK_KHR_incremental_present",
    "VK_KHR_maintenance1",
    "VK_KHR_maintenance2",
    "VK_KHR_maintenance3",
    "VK_KHR_maintenance4",
    "VK_KHR_multiview",
    "VK_KHR_performance_query",
    "VK_KHR_pipeline_executable_properties",
    "VK_KHR_pipeline_library",
    "VK_KHR_present_id",
    "VK_KHR_present_wait",
    "VK_KHR_push_descriptor",
    "VK_KHR_ray_query",
    "VK_KHR_ray_tracing_maintenance1",
    "VK_KHR_ray_tracing_pipeline",
    "VK_KHR_relaxed_block_layout",
    "VK_KHR_sampler_mirror_clamp_to_edge",
    "VK_KHR_sampler_ycbcr_conversion",
    "VK_KHR_separate_depth_stencil_layouts",
    "VK_KHR_shader_atomic_int64",
    "VK_KHR_shader_clock",
    "VK_KHR_shader_draw_parameters",
    "VK_KHR_shader_float16_int8",
    "VK_KHR_shader_float_controls",
    "VK_KHR_shader_integer_dot_product",
    "VK_KHR_shader_non_semantic_info",
    "VK_KHR_shader_subgroup_extended_types",
    "VK_KHR_shader_subgroup_uniform_control_flow",
    "VK_KHR_shader_terminate_invocation",
    "VK_KHR_spirv_1_4",
    "VK_KHR_storage_buffer_storage_class",
    "VK_KHR_swapchain",
    "VK_KHR_swapchain_mutable_format",
    "VK_KHR_synchronization2",
    "VK_KHR_timeline_semaphore",
    "VK_KHR_uniform_buffer_standard_layout",
    "VK_KHR_variable_pointers",
    "VK_KHR_vulkan_memory_model",
    "VK_KHR_workgroup_memory_explicit_layout",
    "VK_KHR_zero_initialize_workgroup_memory",
    "VK_NVX_binary_import",
    "VK_NVX_image_view_handle",
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
    "VK_NV_inherited_viewport_scissor",
    "VK_NV_linear_color_attachment",
    "VK_NV_mesh_shader",
    "VK_NV_ray_tracing",
    "VK_NV_ray_tracing_motion_blur",
    "VK_NV_representative_fragment_test",
    "VK_NV_sample_mask_override_coverage",
    "VK_NV_scissor_exclusive",
    "VK_NV_shader_image_footprint",
    "VK_NV_shader_sm_builtins",
    "VK_NV_shader_subgroup_partitioned",
    "VK_NV_shading_rate_image",
    "VK_NV_viewport_array2",
    "VK_NV_viewport_swizzle",
    "VK_QCOM_fragment_density_map_offset",
    "VK_QCOM_image_processing",
    "VK_QCOM_render_pass_shader_resolve",
    "VK_QCOM_render_pass_store_ops",
    "VK_QCOM_render_pass_transform",
    "VK_QCOM_rotated_copy_commands",
    "VK_QCOM_tile_properties",
    "VK_VALVE_descriptor_set_host_mapping",
    "VK_VALVE_mutable_descriptor_type",
};

static const char * const vk_instance_extensions[] =
{
    "VK_EXT_debug_report",
    "VK_EXT_debug_utils",
    "VK_EXT_swapchain_colorspace",
    "VK_EXT_validation_features",
    "VK_EXT_validation_flags",
    "VK_KHR_device_group_creation",
    "VK_KHR_external_fence_capabilities",
    "VK_KHR_external_memory_capabilities",
    "VK_KHR_external_semaphore_capabilities",
    "VK_KHR_get_physical_device_properties2",
    "VK_KHR_get_surface_capabilities2",
    "VK_KHR_portability_enumeration",
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
        type == VK_OBJECT_TYPE_QUEUE ||
        type == VK_OBJECT_TYPE_SURFACE_KHR;
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
    case VK_OBJECT_TYPE_SURFACE_KHR:
        return (uint64_t) wine_surface_from_handle(handle)->surface;
    default:
       return handle;
    }
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    init_vulkan,
    wine_vkAcquireNextImage2KHR,
    wine_vkAcquireNextImageKHR,
    wine_vkAcquirePerformanceConfigurationINTEL,
    wine_vkAcquireProfilingLockKHR,
    wine_vkAllocateCommandBuffers,
    wine_vkAllocateDescriptorSets,
    wine_vkAllocateMemory,
    wine_vkBeginCommandBuffer,
    wine_vkBindAccelerationStructureMemoryNV,
    wine_vkBindBufferMemory,
    wine_vkBindBufferMemory2,
    wine_vkBindBufferMemory2KHR,
    wine_vkBindImageMemory,
    wine_vkBindImageMemory2,
    wine_vkBindImageMemory2KHR,
    wine_vkBuildAccelerationStructuresKHR,
    wine_vkCmdBeginConditionalRenderingEXT,
    wine_vkCmdBeginDebugUtilsLabelEXT,
    wine_vkCmdBeginQuery,
    wine_vkCmdBeginQueryIndexedEXT,
    wine_vkCmdBeginRenderPass,
    wine_vkCmdBeginRenderPass2,
    wine_vkCmdBeginRenderPass2KHR,
    wine_vkCmdBeginRendering,
    wine_vkCmdBeginRenderingKHR,
    wine_vkCmdBeginTransformFeedbackEXT,
    wine_vkCmdBindDescriptorSets,
    wine_vkCmdBindIndexBuffer,
    wine_vkCmdBindInvocationMaskHUAWEI,
    wine_vkCmdBindPipeline,
    wine_vkCmdBindPipelineShaderGroupNV,
    wine_vkCmdBindShadingRateImageNV,
    wine_vkCmdBindTransformFeedbackBuffersEXT,
    wine_vkCmdBindVertexBuffers,
    wine_vkCmdBindVertexBuffers2,
    wine_vkCmdBindVertexBuffers2EXT,
    wine_vkCmdBlitImage,
    wine_vkCmdBlitImage2,
    wine_vkCmdBlitImage2KHR,
    wine_vkCmdBuildAccelerationStructureNV,
    wine_vkCmdBuildAccelerationStructuresIndirectKHR,
    wine_vkCmdBuildAccelerationStructuresKHR,
    wine_vkCmdClearAttachments,
    wine_vkCmdClearColorImage,
    wine_vkCmdClearDepthStencilImage,
    wine_vkCmdCopyAccelerationStructureKHR,
    wine_vkCmdCopyAccelerationStructureNV,
    wine_vkCmdCopyAccelerationStructureToMemoryKHR,
    wine_vkCmdCopyBuffer,
    wine_vkCmdCopyBuffer2,
    wine_vkCmdCopyBuffer2KHR,
    wine_vkCmdCopyBufferToImage,
    wine_vkCmdCopyBufferToImage2,
    wine_vkCmdCopyBufferToImage2KHR,
    wine_vkCmdCopyImage,
    wine_vkCmdCopyImage2,
    wine_vkCmdCopyImage2KHR,
    wine_vkCmdCopyImageToBuffer,
    wine_vkCmdCopyImageToBuffer2,
    wine_vkCmdCopyImageToBuffer2KHR,
    wine_vkCmdCopyMemoryToAccelerationStructureKHR,
    wine_vkCmdCopyQueryPoolResults,
    wine_vkCmdCuLaunchKernelNVX,
    wine_vkCmdDebugMarkerBeginEXT,
    wine_vkCmdDebugMarkerEndEXT,
    wine_vkCmdDebugMarkerInsertEXT,
    wine_vkCmdDispatch,
    wine_vkCmdDispatchBase,
    wine_vkCmdDispatchBaseKHR,
    wine_vkCmdDispatchIndirect,
    wine_vkCmdDraw,
    wine_vkCmdDrawIndexed,
    wine_vkCmdDrawIndexedIndirect,
    wine_vkCmdDrawIndexedIndirectCount,
    wine_vkCmdDrawIndexedIndirectCountAMD,
    wine_vkCmdDrawIndexedIndirectCountKHR,
    wine_vkCmdDrawIndirect,
    wine_vkCmdDrawIndirectByteCountEXT,
    wine_vkCmdDrawIndirectCount,
    wine_vkCmdDrawIndirectCountAMD,
    wine_vkCmdDrawIndirectCountKHR,
    wine_vkCmdDrawMeshTasksIndirectCountNV,
    wine_vkCmdDrawMeshTasksIndirectNV,
    wine_vkCmdDrawMeshTasksNV,
    wine_vkCmdDrawMultiEXT,
    wine_vkCmdDrawMultiIndexedEXT,
    wine_vkCmdEndConditionalRenderingEXT,
    wine_vkCmdEndDebugUtilsLabelEXT,
    wine_vkCmdEndQuery,
    wine_vkCmdEndQueryIndexedEXT,
    wine_vkCmdEndRenderPass,
    wine_vkCmdEndRenderPass2,
    wine_vkCmdEndRenderPass2KHR,
    wine_vkCmdEndRendering,
    wine_vkCmdEndRenderingKHR,
    wine_vkCmdEndTransformFeedbackEXT,
    wine_vkCmdExecuteCommands,
    wine_vkCmdExecuteGeneratedCommandsNV,
    wine_vkCmdFillBuffer,
    wine_vkCmdInsertDebugUtilsLabelEXT,
    wine_vkCmdNextSubpass,
    wine_vkCmdNextSubpass2,
    wine_vkCmdNextSubpass2KHR,
    wine_vkCmdPipelineBarrier,
    wine_vkCmdPipelineBarrier2,
    wine_vkCmdPipelineBarrier2KHR,
    wine_vkCmdPreprocessGeneratedCommandsNV,
    wine_vkCmdPushConstants,
    wine_vkCmdPushDescriptorSetKHR,
    wine_vkCmdPushDescriptorSetWithTemplateKHR,
    wine_vkCmdResetEvent,
    wine_vkCmdResetEvent2,
    wine_vkCmdResetEvent2KHR,
    wine_vkCmdResetQueryPool,
    wine_vkCmdResolveImage,
    wine_vkCmdResolveImage2,
    wine_vkCmdResolveImage2KHR,
    wine_vkCmdSetBlendConstants,
    wine_vkCmdSetCheckpointNV,
    wine_vkCmdSetCoarseSampleOrderNV,
    wine_vkCmdSetColorWriteEnableEXT,
    wine_vkCmdSetCullMode,
    wine_vkCmdSetCullModeEXT,
    wine_vkCmdSetDepthBias,
    wine_vkCmdSetDepthBiasEnable,
    wine_vkCmdSetDepthBiasEnableEXT,
    wine_vkCmdSetDepthBounds,
    wine_vkCmdSetDepthBoundsTestEnable,
    wine_vkCmdSetDepthBoundsTestEnableEXT,
    wine_vkCmdSetDepthCompareOp,
    wine_vkCmdSetDepthCompareOpEXT,
    wine_vkCmdSetDepthTestEnable,
    wine_vkCmdSetDepthTestEnableEXT,
    wine_vkCmdSetDepthWriteEnable,
    wine_vkCmdSetDepthWriteEnableEXT,
    wine_vkCmdSetDeviceMask,
    wine_vkCmdSetDeviceMaskKHR,
    wine_vkCmdSetDiscardRectangleEXT,
    wine_vkCmdSetEvent,
    wine_vkCmdSetEvent2,
    wine_vkCmdSetEvent2KHR,
    wine_vkCmdSetExclusiveScissorNV,
    wine_vkCmdSetFragmentShadingRateEnumNV,
    wine_vkCmdSetFragmentShadingRateKHR,
    wine_vkCmdSetFrontFace,
    wine_vkCmdSetFrontFaceEXT,
    wine_vkCmdSetLineStippleEXT,
    wine_vkCmdSetLineWidth,
    wine_vkCmdSetLogicOpEXT,
    wine_vkCmdSetPatchControlPointsEXT,
    wine_vkCmdSetPerformanceMarkerINTEL,
    wine_vkCmdSetPerformanceOverrideINTEL,
    wine_vkCmdSetPerformanceStreamMarkerINTEL,
    wine_vkCmdSetPrimitiveRestartEnable,
    wine_vkCmdSetPrimitiveRestartEnableEXT,
    wine_vkCmdSetPrimitiveTopology,
    wine_vkCmdSetPrimitiveTopologyEXT,
    wine_vkCmdSetRasterizerDiscardEnable,
    wine_vkCmdSetRasterizerDiscardEnableEXT,
    wine_vkCmdSetRayTracingPipelineStackSizeKHR,
    wine_vkCmdSetSampleLocationsEXT,
    wine_vkCmdSetScissor,
    wine_vkCmdSetScissorWithCount,
    wine_vkCmdSetScissorWithCountEXT,
    wine_vkCmdSetStencilCompareMask,
    wine_vkCmdSetStencilOp,
    wine_vkCmdSetStencilOpEXT,
    wine_vkCmdSetStencilReference,
    wine_vkCmdSetStencilTestEnable,
    wine_vkCmdSetStencilTestEnableEXT,
    wine_vkCmdSetStencilWriteMask,
    wine_vkCmdSetVertexInputEXT,
    wine_vkCmdSetViewport,
    wine_vkCmdSetViewportShadingRatePaletteNV,
    wine_vkCmdSetViewportWScalingNV,
    wine_vkCmdSetViewportWithCount,
    wine_vkCmdSetViewportWithCountEXT,
    wine_vkCmdSubpassShadingHUAWEI,
    wine_vkCmdTraceRaysIndirect2KHR,
    wine_vkCmdTraceRaysIndirectKHR,
    wine_vkCmdTraceRaysKHR,
    wine_vkCmdTraceRaysNV,
    wine_vkCmdUpdateBuffer,
    wine_vkCmdWaitEvents,
    wine_vkCmdWaitEvents2,
    wine_vkCmdWaitEvents2KHR,
    wine_vkCmdWriteAccelerationStructuresPropertiesKHR,
    wine_vkCmdWriteAccelerationStructuresPropertiesNV,
    wine_vkCmdWriteBufferMarker2AMD,
    wine_vkCmdWriteBufferMarkerAMD,
    wine_vkCmdWriteTimestamp,
    wine_vkCmdWriteTimestamp2,
    wine_vkCmdWriteTimestamp2KHR,
    wine_vkCompileDeferredNV,
    wine_vkCopyAccelerationStructureKHR,
    wine_vkCopyAccelerationStructureToMemoryKHR,
    wine_vkCopyMemoryToAccelerationStructureKHR,
    wine_vkCreateAccelerationStructureKHR,
    wine_vkCreateAccelerationStructureNV,
    wine_vkCreateBuffer,
    wine_vkCreateBufferView,
    wine_vkCreateCommandPool,
    wine_vkCreateComputePipelines,
    wine_vkCreateCuFunctionNVX,
    wine_vkCreateCuModuleNVX,
    wine_vkCreateDebugReportCallbackEXT,
    wine_vkCreateDebugUtilsMessengerEXT,
    wine_vkCreateDeferredOperationKHR,
    wine_vkCreateDescriptorPool,
    wine_vkCreateDescriptorSetLayout,
    wine_vkCreateDescriptorUpdateTemplate,
    wine_vkCreateDescriptorUpdateTemplateKHR,
    wine_vkCreateDevice,
    wine_vkCreateEvent,
    wine_vkCreateFence,
    wine_vkCreateFramebuffer,
    wine_vkCreateGraphicsPipelines,
    wine_vkCreateImage,
    wine_vkCreateImageView,
    wine_vkCreateIndirectCommandsLayoutNV,
    wine_vkCreateInstance,
    wine_vkCreatePipelineCache,
    wine_vkCreatePipelineLayout,
    wine_vkCreatePrivateDataSlot,
    wine_vkCreatePrivateDataSlotEXT,
    wine_vkCreateQueryPool,
    wine_vkCreateRayTracingPipelinesKHR,
    wine_vkCreateRayTracingPipelinesNV,
    wine_vkCreateRenderPass,
    wine_vkCreateRenderPass2,
    wine_vkCreateRenderPass2KHR,
    wine_vkCreateSampler,
    wine_vkCreateSamplerYcbcrConversion,
    wine_vkCreateSamplerYcbcrConversionKHR,
    wine_vkCreateSemaphore,
    wine_vkCreateShaderModule,
    wine_vkCreateSwapchainKHR,
    wine_vkCreateValidationCacheEXT,
    wine_vkCreateWin32SurfaceKHR,
    wine_vkDebugMarkerSetObjectNameEXT,
    wine_vkDebugMarkerSetObjectTagEXT,
    wine_vkDebugReportMessageEXT,
    wine_vkDeferredOperationJoinKHR,
    wine_vkDestroyAccelerationStructureKHR,
    wine_vkDestroyAccelerationStructureNV,
    wine_vkDestroyBuffer,
    wine_vkDestroyBufferView,
    wine_vkDestroyCommandPool,
    wine_vkDestroyCuFunctionNVX,
    wine_vkDestroyCuModuleNVX,
    wine_vkDestroyDebugReportCallbackEXT,
    wine_vkDestroyDebugUtilsMessengerEXT,
    wine_vkDestroyDeferredOperationKHR,
    wine_vkDestroyDescriptorPool,
    wine_vkDestroyDescriptorSetLayout,
    wine_vkDestroyDescriptorUpdateTemplate,
    wine_vkDestroyDescriptorUpdateTemplateKHR,
    wine_vkDestroyDevice,
    wine_vkDestroyEvent,
    wine_vkDestroyFence,
    wine_vkDestroyFramebuffer,
    wine_vkDestroyImage,
    wine_vkDestroyImageView,
    wine_vkDestroyIndirectCommandsLayoutNV,
    wine_vkDestroyInstance,
    wine_vkDestroyPipeline,
    wine_vkDestroyPipelineCache,
    wine_vkDestroyPipelineLayout,
    wine_vkDestroyPrivateDataSlot,
    wine_vkDestroyPrivateDataSlotEXT,
    wine_vkDestroyQueryPool,
    wine_vkDestroyRenderPass,
    wine_vkDestroySampler,
    wine_vkDestroySamplerYcbcrConversion,
    wine_vkDestroySamplerYcbcrConversionKHR,
    wine_vkDestroySemaphore,
    wine_vkDestroyShaderModule,
    wine_vkDestroySurfaceKHR,
    wine_vkDestroySwapchainKHR,
    wine_vkDestroyValidationCacheEXT,
    wine_vkDeviceWaitIdle,
    wine_vkEndCommandBuffer,
    wine_vkEnumerateDeviceExtensionProperties,
    wine_vkEnumerateDeviceLayerProperties,
    wine_vkEnumerateInstanceExtensionProperties,
    wine_vkEnumerateInstanceVersion,
    wine_vkEnumeratePhysicalDeviceGroups,
    wine_vkEnumeratePhysicalDeviceGroupsKHR,
    wine_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR,
    wine_vkEnumeratePhysicalDevices,
    wine_vkFlushMappedMemoryRanges,
    wine_vkFreeCommandBuffers,
    wine_vkFreeDescriptorSets,
    wine_vkFreeMemory,
    wine_vkGetAccelerationStructureBuildSizesKHR,
    wine_vkGetAccelerationStructureDeviceAddressKHR,
    wine_vkGetAccelerationStructureHandleNV,
    wine_vkGetAccelerationStructureMemoryRequirementsNV,
    wine_vkGetBufferDeviceAddress,
    wine_vkGetBufferDeviceAddressEXT,
    wine_vkGetBufferDeviceAddressKHR,
    wine_vkGetBufferMemoryRequirements,
    wine_vkGetBufferMemoryRequirements2,
    wine_vkGetBufferMemoryRequirements2KHR,
    wine_vkGetBufferOpaqueCaptureAddress,
    wine_vkGetBufferOpaqueCaptureAddressKHR,
    wine_vkGetCalibratedTimestampsEXT,
    wine_vkGetDeferredOperationMaxConcurrencyKHR,
    wine_vkGetDeferredOperationResultKHR,
    wine_vkGetDescriptorSetHostMappingVALVE,
    wine_vkGetDescriptorSetLayoutHostMappingInfoVALVE,
    wine_vkGetDescriptorSetLayoutSupport,
    wine_vkGetDescriptorSetLayoutSupportKHR,
    wine_vkGetDeviceAccelerationStructureCompatibilityKHR,
    wine_vkGetDeviceBufferMemoryRequirements,
    wine_vkGetDeviceBufferMemoryRequirementsKHR,
    wine_vkGetDeviceGroupPeerMemoryFeatures,
    wine_vkGetDeviceGroupPeerMemoryFeaturesKHR,
    wine_vkGetDeviceGroupPresentCapabilitiesKHR,
    wine_vkGetDeviceGroupSurfacePresentModesKHR,
    wine_vkGetDeviceImageMemoryRequirements,
    wine_vkGetDeviceImageMemoryRequirementsKHR,
    wine_vkGetDeviceImageSparseMemoryRequirements,
    wine_vkGetDeviceImageSparseMemoryRequirementsKHR,
    wine_vkGetDeviceMemoryCommitment,
    wine_vkGetDeviceMemoryOpaqueCaptureAddress,
    wine_vkGetDeviceMemoryOpaqueCaptureAddressKHR,
    wine_vkGetDeviceQueue,
    wine_vkGetDeviceQueue2,
    wine_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI,
    wine_vkGetDynamicRenderingTilePropertiesQCOM,
    wine_vkGetEventStatus,
    wine_vkGetFenceStatus,
    wine_vkGetFramebufferTilePropertiesQCOM,
    wine_vkGetGeneratedCommandsMemoryRequirementsNV,
    wine_vkGetImageMemoryRequirements,
    wine_vkGetImageMemoryRequirements2,
    wine_vkGetImageMemoryRequirements2KHR,
    wine_vkGetImageSparseMemoryRequirements,
    wine_vkGetImageSparseMemoryRequirements2,
    wine_vkGetImageSparseMemoryRequirements2KHR,
    wine_vkGetImageSubresourceLayout,
    wine_vkGetImageSubresourceLayout2EXT,
    wine_vkGetImageViewAddressNVX,
    wine_vkGetImageViewHandleNVX,
    wine_vkGetMemoryHostPointerPropertiesEXT,
    wine_vkGetPerformanceParameterINTEL,
    wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
    wine_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV,
    wine_vkGetPhysicalDeviceExternalBufferProperties,
    wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR,
    wine_vkGetPhysicalDeviceExternalFenceProperties,
    wine_vkGetPhysicalDeviceExternalFencePropertiesKHR,
    wine_vkGetPhysicalDeviceExternalSemaphoreProperties,
    wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR,
    wine_vkGetPhysicalDeviceFeatures,
    wine_vkGetPhysicalDeviceFeatures2,
    wine_vkGetPhysicalDeviceFeatures2KHR,
    wine_vkGetPhysicalDeviceFormatProperties,
    wine_vkGetPhysicalDeviceFormatProperties2,
    wine_vkGetPhysicalDeviceFormatProperties2KHR,
    wine_vkGetPhysicalDeviceFragmentShadingRatesKHR,
    wine_vkGetPhysicalDeviceImageFormatProperties,
    wine_vkGetPhysicalDeviceImageFormatProperties2,
    wine_vkGetPhysicalDeviceImageFormatProperties2KHR,
    wine_vkGetPhysicalDeviceMemoryProperties,
    wine_vkGetPhysicalDeviceMemoryProperties2,
    wine_vkGetPhysicalDeviceMemoryProperties2KHR,
    wine_vkGetPhysicalDeviceMultisamplePropertiesEXT,
    wine_vkGetPhysicalDevicePresentRectanglesKHR,
    wine_vkGetPhysicalDeviceProperties,
    wine_vkGetPhysicalDeviceProperties2,
    wine_vkGetPhysicalDeviceProperties2KHR,
    wine_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR,
    wine_vkGetPhysicalDeviceQueueFamilyProperties,
    wine_vkGetPhysicalDeviceQueueFamilyProperties2,
    wine_vkGetPhysicalDeviceQueueFamilyProperties2KHR,
    wine_vkGetPhysicalDeviceSparseImageFormatProperties,
    wine_vkGetPhysicalDeviceSparseImageFormatProperties2,
    wine_vkGetPhysicalDeviceSparseImageFormatProperties2KHR,
    wine_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV,
    wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR,
    wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
    wine_vkGetPhysicalDeviceSurfaceFormats2KHR,
    wine_vkGetPhysicalDeviceSurfaceFormatsKHR,
    wine_vkGetPhysicalDeviceSurfacePresentModesKHR,
    wine_vkGetPhysicalDeviceSurfaceSupportKHR,
    wine_vkGetPhysicalDeviceToolProperties,
    wine_vkGetPhysicalDeviceToolPropertiesEXT,
    wine_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    wine_vkGetPipelineCacheData,
    wine_vkGetPipelineExecutableInternalRepresentationsKHR,
    wine_vkGetPipelineExecutablePropertiesKHR,
    wine_vkGetPipelineExecutableStatisticsKHR,
    wine_vkGetPipelinePropertiesEXT,
    wine_vkGetPrivateData,
    wine_vkGetPrivateDataEXT,
    wine_vkGetQueryPoolResults,
    wine_vkGetQueueCheckpointData2NV,
    wine_vkGetQueueCheckpointDataNV,
    wine_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR,
    wine_vkGetRayTracingShaderGroupHandlesKHR,
    wine_vkGetRayTracingShaderGroupHandlesNV,
    wine_vkGetRayTracingShaderGroupStackSizeKHR,
    wine_vkGetRenderAreaGranularity,
    wine_vkGetSemaphoreCounterValue,
    wine_vkGetSemaphoreCounterValueKHR,
    wine_vkGetShaderInfoAMD,
    wine_vkGetShaderModuleCreateInfoIdentifierEXT,
    wine_vkGetShaderModuleIdentifierEXT,
    wine_vkGetSwapchainImagesKHR,
    wine_vkGetValidationCacheDataEXT,
    wine_vkInitializePerformanceApiINTEL,
    wine_vkInvalidateMappedMemoryRanges,
    wine_vkMapMemory,
    wine_vkMergePipelineCaches,
    wine_vkMergeValidationCachesEXT,
    wine_vkQueueBeginDebugUtilsLabelEXT,
    wine_vkQueueBindSparse,
    wine_vkQueueEndDebugUtilsLabelEXT,
    wine_vkQueueInsertDebugUtilsLabelEXT,
    wine_vkQueuePresentKHR,
    wine_vkQueueSetPerformanceConfigurationINTEL,
    wine_vkQueueSubmit,
    wine_vkQueueSubmit2,
    wine_vkQueueSubmit2KHR,
    wine_vkQueueWaitIdle,
    wine_vkReleasePerformanceConfigurationINTEL,
    wine_vkReleaseProfilingLockKHR,
    wine_vkResetCommandBuffer,
    wine_vkResetCommandPool,
    wine_vkResetDescriptorPool,
    wine_vkResetEvent,
    wine_vkResetFences,
    wine_vkResetQueryPool,
    wine_vkResetQueryPoolEXT,
    wine_vkSetDebugUtilsObjectNameEXT,
    wine_vkSetDebugUtilsObjectTagEXT,
    wine_vkSetDeviceMemoryPriorityEXT,
    wine_vkSetEvent,
    wine_vkSetPrivateData,
    wine_vkSetPrivateDataEXT,
    wine_vkSignalSemaphore,
    wine_vkSignalSemaphoreKHR,
    wine_vkSubmitDebugUtilsMessageEXT,
    wine_vkTrimCommandPool,
    wine_vkTrimCommandPoolKHR,
    wine_vkUninitializePerformanceApiINTEL,
    wine_vkUnmapMemory,
    wine_vkUpdateDescriptorSetWithTemplate,
    wine_vkUpdateDescriptorSetWithTemplateKHR,
    wine_vkUpdateDescriptorSets,
    wine_vkWaitForFences,
    wine_vkWaitForPresentKHR,
    wine_vkWaitSemaphores,
    wine_vkWaitSemaphoresKHR,
    wine_vkWriteAccelerationStructuresPropertiesKHR,
};
C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_count);

static NTSTATUS WINAPI wine_vk_call(enum unix_call code, void *params)
{
    return __wine_unix_call_funcs[code](params);
}

const struct unix_funcs loader_funcs =
{
    wine_vk_call,
    wine_vk_is_available_instance_function,
    wine_vk_is_available_device_function,
};
