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
static inline void convert_VkAcquireNextImageInfoKHR_win32_to_host(const VkAcquireNextImageInfoKHR *in, VkAcquireNextImageInfoKHR_host *out)
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
static inline void convert_VkAcquireProfilingLockInfoKHR_win32_to_host(const VkAcquireProfilingLockInfoKHR *in, VkAcquireProfilingLockInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->timeout = in->timeout;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDescriptorSetAllocateInfo_win32_to_host(const VkDescriptorSetAllocateInfo *in, VkDescriptorSetAllocateInfo_host *out)
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
static inline void convert_VkMemoryAllocateInfo_win32_to_host(const VkMemoryAllocateInfo *in, VkMemoryAllocateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->allocationSize = in->allocationSize;
    out->memoryTypeIndex = in->memoryTypeIndex;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkCommandBufferInheritanceInfo_host *convert_VkCommandBufferInheritanceInfo_array_win32_to_host(struct conversion_context *ctx, const VkCommandBufferInheritanceInfo *in, uint32_t count)
{
    VkCommandBufferInheritanceInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkCommandBufferBeginInfo_win32_to_host(struct conversion_context *ctx, const VkCommandBufferBeginInfo *in, VkCommandBufferBeginInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->pInheritanceInfo = convert_VkCommandBufferInheritanceInfo_array_win32_to_host(ctx, in->pInheritanceInfo, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBindAccelerationStructureMemoryInfoNV_host *convert_VkBindAccelerationStructureMemoryInfoNV_array_win32_to_host(struct conversion_context *ctx, const VkBindAccelerationStructureMemoryInfoNV *in, uint32_t count)
{
    VkBindAccelerationStructureMemoryInfoNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkBindBufferMemoryInfo_host *convert_VkBindBufferMemoryInfo_array_win32_to_host(struct conversion_context *ctx, const VkBindBufferMemoryInfo *in, uint32_t count)
{
    VkBindBufferMemoryInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkBindImageMemoryInfo_host *convert_VkBindImageMemoryInfo_array_win32_to_host(struct conversion_context *ctx, const VkBindImageMemoryInfo *in, uint32_t count)
{
    VkBindImageMemoryInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkAccelerationStructureBuildGeometryInfoKHR_host *convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win32_to_host(struct conversion_context *ctx, const VkAccelerationStructureBuildGeometryInfoKHR *in, uint32_t count)
{
    VkAccelerationStructureBuildGeometryInfoKHR_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkMicromapBuildInfoEXT_host *convert_VkMicromapBuildInfoEXT_array_win32_to_host(struct conversion_context *ctx, const VkMicromapBuildInfoEXT *in, uint32_t count)
{
    VkMicromapBuildInfoEXT_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].type = in[i].type;
        out[i].flags = in[i].flags;
        out[i].mode = in[i].mode;
        out[i].dstMicromap = in[i].dstMicromap;
        out[i].usageCountsCount = in[i].usageCountsCount;
        out[i].pUsageCounts = in[i].pUsageCounts;
        out[i].ppUsageCounts = in[i].ppUsageCounts;
        out[i].data = in[i].data;
        out[i].scratchData = in[i].scratchData;
        out[i].triangleArray = in[i].triangleArray;
        out[i].triangleArrayStride = in[i].triangleArrayStride;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkConditionalRenderingBeginInfoEXT_win32_to_host(const VkConditionalRenderingBeginInfoEXT *in, VkConditionalRenderingBeginInfoEXT_host *out)
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
static inline void convert_VkRenderPassBeginInfo_win32_to_host(const VkRenderPassBeginInfo *in, VkRenderPassBeginInfo_host *out)
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
static inline VkRenderingAttachmentInfo_host *convert_VkRenderingAttachmentInfo_array_win32_to_host(struct conversion_context *ctx, const VkRenderingAttachmentInfo *in, uint32_t count)
{
    VkRenderingAttachmentInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkRenderingInfo_win32_to_host(struct conversion_context *ctx, const VkRenderingInfo *in, VkRenderingInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->renderArea = in->renderArea;
    out->layerCount = in->layerCount;
    out->viewMask = in->viewMask;
    out->colorAttachmentCount = in->colorAttachmentCount;
    out->pColorAttachments = convert_VkRenderingAttachmentInfo_array_win32_to_host(ctx, in->pColorAttachments, in->colorAttachmentCount);
    out->pDepthAttachment = convert_VkRenderingAttachmentInfo_array_win32_to_host(ctx, in->pDepthAttachment, 1);
    out->pStencilAttachment = convert_VkRenderingAttachmentInfo_array_win32_to_host(ctx, in->pStencilAttachment, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkBlitImageInfo2_win32_to_host(const VkBlitImageInfo2 *in, VkBlitImageInfo2_host *out)
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
static inline void convert_VkGeometryTrianglesNV_win32_to_host(const VkGeometryTrianglesNV *in, VkGeometryTrianglesNV_host *out)
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
static inline void convert_VkGeometryAABBNV_win32_to_host(const VkGeometryAABBNV *in, VkGeometryAABBNV_host *out)
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
static inline void convert_VkGeometryDataNV_win32_to_host(const VkGeometryDataNV *in, VkGeometryDataNV_host *out)
{
    if (!in) return;

    convert_VkGeometryTrianglesNV_win32_to_host(&in->triangles, &out->triangles);
    convert_VkGeometryAABBNV_win32_to_host(&in->aabbs, &out->aabbs);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkGeometryNV_host *convert_VkGeometryNV_array_win32_to_host(struct conversion_context *ctx, const VkGeometryNV *in, uint32_t count)
{
    VkGeometryNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].geometryType = in[i].geometryType;
        convert_VkGeometryDataNV_win32_to_host(&in[i].geometry, &out[i].geometry);
        out[i].flags = in[i].flags;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureInfoNV_win32_to_host(struct conversion_context *ctx, const VkAccelerationStructureInfoNV *in, VkAccelerationStructureInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->flags = in->flags;
    out->instanceCount = in->instanceCount;
    out->geometryCount = in->geometryCount;
    out->pGeometries = convert_VkGeometryNV_array_win32_to_host(ctx, in->pGeometries, in->geometryCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyAccelerationStructureInfoKHR_win32_to_host(const VkCopyAccelerationStructureInfoKHR *in, VkCopyAccelerationStructureInfoKHR_host *out)
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
static inline void convert_VkCopyAccelerationStructureToMemoryInfoKHR_win32_to_host(const VkCopyAccelerationStructureToMemoryInfoKHR *in, VkCopyAccelerationStructureToMemoryInfoKHR_host *out)
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
static inline VkBufferCopy_host *convert_VkBufferCopy_array_win32_to_host(struct conversion_context *ctx, const VkBufferCopy *in, uint32_t count)
{
    VkBufferCopy_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkBufferCopy2_host *convert_VkBufferCopy2_array_win32_to_host(struct conversion_context *ctx, const VkBufferCopy2 *in, uint32_t count)
{
    VkBufferCopy2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkCopyBufferInfo2_win32_to_host(struct conversion_context *ctx, const VkCopyBufferInfo2 *in, VkCopyBufferInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcBuffer = in->srcBuffer;
    out->dstBuffer = in->dstBuffer;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferCopy2_array_win32_to_host(ctx, in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferImageCopy_host *convert_VkBufferImageCopy_array_win32_to_host(struct conversion_context *ctx, const VkBufferImageCopy *in, uint32_t count)
{
    VkBufferImageCopy_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkBufferImageCopy2_host *convert_VkBufferImageCopy2_array_win32_to_host(struct conversion_context *ctx, const VkBufferImageCopy2 *in, uint32_t count)
{
    VkBufferImageCopy2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkCopyBufferToImageInfo2_win32_to_host(struct conversion_context *ctx, const VkCopyBufferToImageInfo2 *in, VkCopyBufferToImageInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcBuffer = in->srcBuffer;
    out->dstImage = in->dstImage;
    out->dstImageLayout = in->dstImageLayout;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferImageCopy2_array_win32_to_host(ctx, in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyImageInfo2_win32_to_host(const VkCopyImageInfo2 *in, VkCopyImageInfo2_host *out)
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
static inline void convert_VkCopyImageToBufferInfo2_win32_to_host(struct conversion_context *ctx, const VkCopyImageToBufferInfo2 *in, VkCopyImageToBufferInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->srcImage = in->srcImage;
    out->srcImageLayout = in->srcImageLayout;
    out->dstBuffer = in->dstBuffer;
    out->regionCount = in->regionCount;
    out->pRegions = convert_VkBufferImageCopy2_array_win32_to_host(ctx, in->pRegions, in->regionCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCopyMemoryToAccelerationStructureInfoKHR_win32_to_host(const VkCopyMemoryToAccelerationStructureInfoKHR *in, VkCopyMemoryToAccelerationStructureInfoKHR_host *out)
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
static inline void convert_VkCopyMemoryToMicromapInfoEXT_win32_to_host(const VkCopyMemoryToMicromapInfoEXT *in, VkCopyMemoryToMicromapInfoEXT_host *out)
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
static inline void convert_VkCopyMicromapInfoEXT_win32_to_host(const VkCopyMicromapInfoEXT *in, VkCopyMicromapInfoEXT_host *out)
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
static inline void convert_VkCopyMicromapToMemoryInfoEXT_win32_to_host(const VkCopyMicromapToMemoryInfoEXT *in, VkCopyMicromapToMemoryInfoEXT_host *out)
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
static inline void convert_VkCuLaunchInfoNVX_win32_to_host(const VkCuLaunchInfoNVX *in, VkCuLaunchInfoNVX_host *out)
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

#if !defined(USE_STRUCT_CONVERSION)
static inline VkCommandBuffer *convert_VkCommandBuffer_array_win64_to_host(struct conversion_context *ctx, const VkCommandBuffer *in, uint32_t count)
{
    VkCommandBuffer *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i] = wine_cmd_buffer_from_handle(in[i])->command_buffer;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkCommandBuffer *convert_VkCommandBuffer_array_win32_to_host(struct conversion_context *ctx, const VkCommandBuffer *in, uint32_t count)
{
    VkCommandBuffer *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i] = wine_cmd_buffer_from_handle(in[i])->command_buffer;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkIndirectCommandsStreamNV_host *convert_VkIndirectCommandsStreamNV_array_win32_to_host(struct conversion_context *ctx, const VkIndirectCommandsStreamNV *in, uint32_t count)
{
    VkIndirectCommandsStreamNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].offset = in[i].offset;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkGeneratedCommandsInfoNV_win32_to_host(struct conversion_context *ctx, const VkGeneratedCommandsInfoNV *in, VkGeneratedCommandsInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipelineBindPoint = in->pipelineBindPoint;
    out->pipeline = in->pipeline;
    out->indirectCommandsLayout = in->indirectCommandsLayout;
    out->streamCount = in->streamCount;
    out->pStreams = convert_VkIndirectCommandsStreamNV_array_win32_to_host(ctx, in->pStreams, in->streamCount);
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
static inline VkBufferMemoryBarrier_host *convert_VkBufferMemoryBarrier_array_win32_to_host(struct conversion_context *ctx, const VkBufferMemoryBarrier *in, uint32_t count)
{
    VkBufferMemoryBarrier_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkImageMemoryBarrier_host *convert_VkImageMemoryBarrier_array_win32_to_host(struct conversion_context *ctx, const VkImageMemoryBarrier *in, uint32_t count)
{
    VkImageMemoryBarrier_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkBufferMemoryBarrier2_host *convert_VkBufferMemoryBarrier2_array_win32_to_host(struct conversion_context *ctx, const VkBufferMemoryBarrier2 *in, uint32_t count)
{
    VkBufferMemoryBarrier2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkImageMemoryBarrier2_host *convert_VkImageMemoryBarrier2_array_win32_to_host(struct conversion_context *ctx, const VkImageMemoryBarrier2 *in, uint32_t count)
{
    VkImageMemoryBarrier2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkDependencyInfo_win32_to_host(struct conversion_context *ctx, const VkDependencyInfo *in, VkDependencyInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->dependencyFlags = in->dependencyFlags;
    out->memoryBarrierCount = in->memoryBarrierCount;
    out->pMemoryBarriers = in->pMemoryBarriers;
    out->bufferMemoryBarrierCount = in->bufferMemoryBarrierCount;
    out->pBufferMemoryBarriers = convert_VkBufferMemoryBarrier2_array_win32_to_host(ctx, in->pBufferMemoryBarriers, in->bufferMemoryBarrierCount);
    out->imageMemoryBarrierCount = in->imageMemoryBarrierCount;
    out->pImageMemoryBarriers = convert_VkImageMemoryBarrier2_array_win32_to_host(ctx, in->pImageMemoryBarriers, in->imageMemoryBarrierCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDescriptorImageInfo_host *convert_VkDescriptorImageInfo_array_win32_to_host(struct conversion_context *ctx, const VkDescriptorImageInfo *in, uint32_t count)
{
    VkDescriptorImageInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkDescriptorBufferInfo_host *convert_VkDescriptorBufferInfo_array_win32_to_host(struct conversion_context *ctx, const VkDescriptorBufferInfo *in, uint32_t count)
{
    VkDescriptorBufferInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkWriteDescriptorSet_host *convert_VkWriteDescriptorSet_array_win32_to_host(struct conversion_context *ctx, const VkWriteDescriptorSet *in, uint32_t count)
{
    VkWriteDescriptorSet_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].dstSet = in[i].dstSet;
        out[i].dstBinding = in[i].dstBinding;
        out[i].dstArrayElement = in[i].dstArrayElement;
        out[i].descriptorCount = in[i].descriptorCount;
        out[i].descriptorType = in[i].descriptorType;
        out[i].pImageInfo = convert_VkDescriptorImageInfo_array_win32_to_host(ctx, in[i].pImageInfo, in[i].descriptorCount);
        out[i].pBufferInfo = convert_VkDescriptorBufferInfo_array_win32_to_host(ctx, in[i].pBufferInfo, in[i].descriptorCount);
        out[i].pTexelBufferView = in[i].pTexelBufferView;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkResolveImageInfo2_win32_to_host(const VkResolveImageInfo2 *in, VkResolveImageInfo2_host *out)
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
static inline void convert_VkPerformanceMarkerInfoINTEL_win32_to_host(const VkPerformanceMarkerInfoINTEL *in, VkPerformanceMarkerInfoINTEL_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->marker = in->marker;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPerformanceOverrideInfoINTEL_win32_to_host(const VkPerformanceOverrideInfoINTEL *in, VkPerformanceOverrideInfoINTEL_host *out)
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
static inline void convert_VkStridedDeviceAddressRegionKHR_win32_to_host(const VkStridedDeviceAddressRegionKHR *in, VkStridedDeviceAddressRegionKHR_host *out)
{
    if (!in) return;

    out->deviceAddress = in->deviceAddress;
    out->stride = in->stride;
    out->size = in->size;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDependencyInfo_host *convert_VkDependencyInfo_array_win32_to_host(struct conversion_context *ctx, const VkDependencyInfo *in, uint32_t count)
{
    VkDependencyInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].dependencyFlags = in[i].dependencyFlags;
        out[i].memoryBarrierCount = in[i].memoryBarrierCount;
        out[i].pMemoryBarriers = in[i].pMemoryBarriers;
        out[i].bufferMemoryBarrierCount = in[i].bufferMemoryBarrierCount;
        out[i].pBufferMemoryBarriers = convert_VkBufferMemoryBarrier2_array_win32_to_host(ctx, in[i].pBufferMemoryBarriers, in[i].bufferMemoryBarrierCount);
        out[i].imageMemoryBarrierCount = in[i].imageMemoryBarrierCount;
        out[i].pImageMemoryBarriers = convert_VkImageMemoryBarrier2_array_win32_to_host(ctx, in[i].pImageMemoryBarriers, in[i].imageMemoryBarrierCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureCreateInfoKHR_win32_to_host(const VkAccelerationStructureCreateInfoKHR *in, VkAccelerationStructureCreateInfoKHR_host *out)
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
static inline void convert_VkAccelerationStructureCreateInfoNV_win32_to_host(struct conversion_context *ctx, const VkAccelerationStructureCreateInfoNV *in, VkAccelerationStructureCreateInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->compactedSize = in->compactedSize;
    convert_VkAccelerationStructureInfoNV_win32_to_host(ctx, &in->info, &out->info);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkBufferCreateInfo_win32_to_host(const VkBufferCreateInfo *in, VkBufferCreateInfo_host *out)
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
static inline void convert_VkBufferViewCreateInfo_win32_to_host(const VkBufferViewCreateInfo *in, VkBufferViewCreateInfo_host *out)
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
static inline void convert_VkPipelineShaderStageCreateInfo_win32_to_host(const VkPipelineShaderStageCreateInfo *in, VkPipelineShaderStageCreateInfo_host *out)
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
static inline VkComputePipelineCreateInfo_host *convert_VkComputePipelineCreateInfo_array_win32_to_host(struct conversion_context *ctx, const VkComputePipelineCreateInfo *in, uint32_t count)
{
    VkComputePipelineCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        convert_VkPipelineShaderStageCreateInfo_win32_to_host(&in[i].stage, &out[i].stage);
        out[i].layout = in[i].layout;
        out[i].basePipelineHandle = in[i].basePipelineHandle;
        out[i].basePipelineIndex = in[i].basePipelineIndex;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCuFunctionCreateInfoNVX_win32_to_host(const VkCuFunctionCreateInfoNVX *in, VkCuFunctionCreateInfoNVX_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->module = in->module;
    out->pName = in->pName;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDescriptorUpdateTemplateCreateInfo_win32_to_host(const VkDescriptorUpdateTemplateCreateInfo *in, VkDescriptorUpdateTemplateCreateInfo_host *out)
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
static inline void convert_VkFramebufferCreateInfo_win32_to_host(const VkFramebufferCreateInfo *in, VkFramebufferCreateInfo_host *out)
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
static inline VkPipelineShaderStageCreateInfo_host *convert_VkPipelineShaderStageCreateInfo_array_win32_to_host(struct conversion_context *ctx, const VkPipelineShaderStageCreateInfo *in, uint32_t count)
{
    VkPipelineShaderStageCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkGraphicsPipelineCreateInfo_host *convert_VkGraphicsPipelineCreateInfo_array_win32_to_host(struct conversion_context *ctx, const VkGraphicsPipelineCreateInfo *in, uint32_t count)
{
    VkGraphicsPipelineCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].stageCount = in[i].stageCount;
        out[i].pStages = convert_VkPipelineShaderStageCreateInfo_array_win32_to_host(ctx, in[i].pStages, in[i].stageCount);
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
static inline void convert_VkImageViewCreateInfo_win32_to_host(const VkImageViewCreateInfo *in, VkImageViewCreateInfo_host *out)
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
static inline VkIndirectCommandsLayoutTokenNV_host *convert_VkIndirectCommandsLayoutTokenNV_array_win32_to_host(struct conversion_context *ctx, const VkIndirectCommandsLayoutTokenNV *in, uint32_t count)
{
    VkIndirectCommandsLayoutTokenNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkIndirectCommandsLayoutCreateInfoNV_win32_to_host(struct conversion_context *ctx, const VkIndirectCommandsLayoutCreateInfoNV *in, VkIndirectCommandsLayoutCreateInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->flags = in->flags;
    out->pipelineBindPoint = in->pipelineBindPoint;
    out->tokenCount = in->tokenCount;
    out->pTokens = convert_VkIndirectCommandsLayoutTokenNV_array_win32_to_host(ctx, in->pTokens, in->tokenCount);
    out->streamCount = in->streamCount;
    out->pStreamStrides = in->pStreamStrides;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMicromapCreateInfoEXT_win32_to_host(const VkMicromapCreateInfoEXT *in, VkMicromapCreateInfoEXT_host *out)
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
static inline VkRayTracingPipelineCreateInfoKHR_host *convert_VkRayTracingPipelineCreateInfoKHR_array_win32_to_host(struct conversion_context *ctx, const VkRayTracingPipelineCreateInfoKHR *in, uint32_t count)
{
    VkRayTracingPipelineCreateInfoKHR_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].stageCount = in[i].stageCount;
        out[i].pStages = convert_VkPipelineShaderStageCreateInfo_array_win32_to_host(ctx, in[i].pStages, in[i].stageCount);
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
static inline VkRayTracingPipelineCreateInfoNV_host *convert_VkRayTracingPipelineCreateInfoNV_array_win32_to_host(struct conversion_context *ctx, const VkRayTracingPipelineCreateInfoNV *in, uint32_t count)
{
    VkRayTracingPipelineCreateInfoNV_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].stageCount = in[i].stageCount;
        out[i].pStages = convert_VkPipelineShaderStageCreateInfo_array_win32_to_host(ctx, in[i].pStages, in[i].stageCount);
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

#if !defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSwapchainCreateInfoKHR_win64_to_host(const VkSwapchainCreateInfoKHR *in, VkSwapchainCreateInfoKHR *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSwapchainCreateInfoKHR_win32_to_host(const VkSwapchainCreateInfoKHR *in, VkSwapchainCreateInfoKHR_host *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugMarkerObjectNameInfoEXT_win64_to_host(const VkDebugMarkerObjectNameInfoEXT *in, VkDebugMarkerObjectNameInfoEXT *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->object = wine_vk_unwrap_handle(in->objectType, in->object);
    out->pObjectName = in->pObjectName;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugMarkerObjectNameInfoEXT_win32_to_host(const VkDebugMarkerObjectNameInfoEXT *in, VkDebugMarkerObjectNameInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->object = wine_vk_unwrap_handle(in->objectType, in->object);
    out->pObjectName = in->pObjectName;
}
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugMarkerObjectTagInfoEXT_win64_to_host(const VkDebugMarkerObjectTagInfoEXT *in, VkDebugMarkerObjectTagInfoEXT *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugMarkerObjectTagInfoEXT_win32_to_host(const VkDebugMarkerObjectTagInfoEXT *in, VkDebugMarkerObjectTagInfoEXT_host *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkMappedMemoryRange_host *convert_VkMappedMemoryRange_array_win32_to_host(struct conversion_context *ctx, const VkMappedMemoryRange *in, uint32_t count)
{
    VkMappedMemoryRange_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkAccelerationStructureBuildGeometryInfoKHR_win32_to_host(const VkAccelerationStructureBuildGeometryInfoKHR *in, VkAccelerationStructureBuildGeometryInfoKHR_host *out)
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
static inline void convert_VkAccelerationStructureBuildSizesInfoKHR_win32_to_host(const VkAccelerationStructureBuildSizesInfoKHR *in, VkAccelerationStructureBuildSizesInfoKHR_host *out)
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
static inline void convert_VkAccelerationStructureDeviceAddressInfoKHR_win32_to_host(const VkAccelerationStructureDeviceAddressInfoKHR *in, VkAccelerationStructureDeviceAddressInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->accelerationStructure = in->accelerationStructure;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkAccelerationStructureMemoryRequirementsInfoNV_win32_to_host(const VkAccelerationStructureMemoryRequirementsInfoNV *in, VkAccelerationStructureMemoryRequirementsInfoNV_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->accelerationStructure = in->accelerationStructure;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements_host_to_win32(const VkMemoryRequirements_host *in, VkMemoryRequirements *out)
{
    if (!in) return;

    out->size = in->size;
    out->alignment = in->alignment;
    out->memoryTypeBits = in->memoryTypeBits;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2KHR_win32_to_host(const VkMemoryRequirements2KHR *in, VkMemoryRequirements2KHR_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2KHR_host_to_win32(const VkMemoryRequirements2KHR_host *in, VkMemoryRequirements2KHR *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkMemoryRequirements_host_to_win32(&in->memoryRequirements, &out->memoryRequirements);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkBufferDeviceAddressInfo_win32_to_host(const VkBufferDeviceAddressInfo *in, VkBufferDeviceAddressInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkBufferMemoryRequirementsInfo2_win32_to_host(const VkBufferMemoryRequirementsInfo2 *in, VkBufferMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->buffer = in->buffer;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2_win32_to_host(const VkMemoryRequirements2 *in, VkMemoryRequirements2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryRequirements2_host_to_win32(const VkMemoryRequirements2_host *in, VkMemoryRequirements2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkMemoryRequirements_host_to_win32(&in->memoryRequirements, &out->memoryRequirements);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDescriptorSetBindingReferenceVALVE_win32_to_host(const VkDescriptorSetBindingReferenceVALVE *in, VkDescriptorSetBindingReferenceVALVE_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->descriptorSetLayout = in->descriptorSetLayout;
    out->binding = in->binding;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBufferCreateInfo_host *convert_VkBufferCreateInfo_array_win32_to_host(struct conversion_context *ctx, const VkBufferCreateInfo *in, uint32_t count)
{
    VkBufferCreateInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline void convert_VkDeviceBufferMemoryRequirements_win32_to_host(struct conversion_context *ctx, const VkDeviceBufferMemoryRequirements *in, VkDeviceBufferMemoryRequirements_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pCreateInfo = convert_VkBufferCreateInfo_array_win32_to_host(ctx, in->pCreateInfo, 1);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDeviceFaultCountsEXT_win32_to_host(const VkDeviceFaultCountsEXT *in, VkDeviceFaultCountsEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->addressInfoCount = in->addressInfoCount;
    out->vendorInfoCount = in->vendorInfoCount;
    out->vendorBinarySize = in->vendorBinarySize;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDeviceFaultAddressInfoEXT_host *convert_VkDeviceFaultAddressInfoEXT_array_win32_to_host(struct conversion_context *ctx, const VkDeviceFaultAddressInfoEXT *in, uint32_t count)
{
    VkDeviceFaultAddressInfoEXT_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].addressType = in[i].addressType;
        out[i].reportedAddress = in[i].reportedAddress;
        out[i].addressPrecision = in[i].addressPrecision;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDeviceFaultVendorInfoEXT_host *convert_VkDeviceFaultVendorInfoEXT_array_win32_to_host(struct conversion_context *ctx, const VkDeviceFaultVendorInfoEXT *in, uint32_t count)
{
    VkDeviceFaultVendorInfoEXT_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        memcpy(out[i].description, in[i].description, VK_MAX_DESCRIPTION_SIZE * sizeof(char));
        out[i].vendorFaultCode = in[i].vendorFaultCode;
        out[i].vendorFaultData = in[i].vendorFaultData;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDeviceFaultInfoEXT_win32_to_host(struct conversion_context *ctx, const VkDeviceFaultInfoEXT *in, VkDeviceFaultInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    memcpy(out->description, in->description, VK_MAX_DESCRIPTION_SIZE * sizeof(char));
    out->pAddressInfos = convert_VkDeviceFaultAddressInfoEXT_array_win32_to_host(ctx, in->pAddressInfos, 1);
    out->pVendorInfos = convert_VkDeviceFaultVendorInfoEXT_array_win32_to_host(ctx, in->pVendorInfos, 1);
    out->pVendorBinaryData = in->pVendorBinaryData;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win32_to_host(const VkDeviceMemoryOpaqueCaptureAddressInfo *in, VkDeviceMemoryOpaqueCaptureAddressInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->memory = in->memory;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkGeneratedCommandsMemoryRequirementsInfoNV_win32_to_host(const VkGeneratedCommandsMemoryRequirementsInfoNV *in, VkGeneratedCommandsMemoryRequirementsInfoNV_host *out)
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
static inline void convert_VkImageMemoryRequirementsInfo2_win32_to_host(const VkImageMemoryRequirementsInfo2 *in, VkImageMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->image = in->image;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageSparseMemoryRequirementsInfo2_win32_to_host(const VkImageSparseMemoryRequirementsInfo2 *in, VkImageSparseMemoryRequirementsInfo2_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->image = in->image;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSubresourceLayout_host_to_win32(const VkSubresourceLayout_host *in, VkSubresourceLayout *out)
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
static inline void convert_VkSubresourceLayout2EXT_win32_to_host(const VkSubresourceLayout2EXT *in, VkSubresourceLayout2EXT_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSubresourceLayout2EXT_host_to_win32(const VkSubresourceLayout2EXT_host *in, VkSubresourceLayout2EXT *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkSubresourceLayout_host_to_win32(&in->subresourceLayout, &out->subresourceLayout);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageViewAddressPropertiesNVX_win32_to_host(const VkImageViewAddressPropertiesNVX *in, VkImageViewAddressPropertiesNVX_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageViewAddressPropertiesNVX_host_to_win32(const VkImageViewAddressPropertiesNVX_host *in, VkImageViewAddressPropertiesNVX *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->deviceAddress = in->deviceAddress;
    out->size = in->size;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageViewHandleInfoNVX_win32_to_host(const VkImageViewHandleInfoNVX *in, VkImageViewHandleInfoNVX_host *out)
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
static inline void convert_VkMemoryGetWin32HandleInfoKHR_win32_to_host(const VkMemoryGetWin32HandleInfoKHR *in, VkMemoryGetWin32HandleInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->memory = in->memory;
    out->handleType = in->handleType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMicromapBuildInfoEXT_win32_to_host(const VkMicromapBuildInfoEXT *in, VkMicromapBuildInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->type = in->type;
    out->flags = in->flags;
    out->mode = in->mode;
    out->dstMicromap = in->dstMicromap;
    out->usageCountsCount = in->usageCountsCount;
    out->pUsageCounts = in->pUsageCounts;
    out->ppUsageCounts = in->ppUsageCounts;
    out->data = in->data;
    out->scratchData = in->scratchData;
    out->triangleArray = in->triangleArray;
    out->triangleArrayStride = in->triangleArrayStride;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMicromapBuildSizesInfoEXT_win32_to_host(const VkMicromapBuildSizesInfoEXT *in, VkMicromapBuildSizesInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->micromapSize = in->micromapSize;
    out->buildScratchSize = in->buildScratchSize;
    out->discardable = in->discardable;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageFormatProperties_host_to_win32(const VkImageFormatProperties_host *in, VkImageFormatProperties *out)
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
static inline void convert_VkImageFormatProperties2_win32_to_host(const VkImageFormatProperties2 *in, VkImageFormatProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkImageFormatProperties2_host_to_win32(const VkImageFormatProperties2_host *in, VkImageFormatProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkImageFormatProperties_host_to_win32(&in->imageFormatProperties, &out->imageFormatProperties);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkMemoryHeap_static_array_host_to_win32(const VkMemoryHeap_host *in, VkMemoryHeap *out, uint32_t count)
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
static inline void convert_VkPhysicalDeviceMemoryProperties_host_to_win32(const VkPhysicalDeviceMemoryProperties_host *in, VkPhysicalDeviceMemoryProperties *out)
{
    if (!in) return;

    out->memoryTypeCount = in->memoryTypeCount;
    memcpy(out->memoryTypes, in->memoryTypes, VK_MAX_MEMORY_TYPES * sizeof(VkMemoryType));
    out->memoryHeapCount = in->memoryHeapCount;
    convert_VkMemoryHeap_static_array_host_to_win32(in->memoryHeaps, out->memoryHeaps, VK_MAX_MEMORY_HEAPS);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceMemoryProperties2_win32_to_host(const VkPhysicalDeviceMemoryProperties2 *in, VkPhysicalDeviceMemoryProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceMemoryProperties2_host_to_win32(const VkPhysicalDeviceMemoryProperties2_host *in, VkPhysicalDeviceMemoryProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkPhysicalDeviceMemoryProperties_host_to_win32(&in->memoryProperties, &out->memoryProperties);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceLimits_host_to_win32(const VkPhysicalDeviceLimits_host *in, VkPhysicalDeviceLimits *out)
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
static inline void convert_VkPhysicalDeviceProperties_host_to_win32(const VkPhysicalDeviceProperties_host *in, VkPhysicalDeviceProperties *out)
{
    if (!in) return;

    out->apiVersion = in->apiVersion;
    out->driverVersion = in->driverVersion;
    out->vendorID = in->vendorID;
    out->deviceID = in->deviceID;
    out->deviceType = in->deviceType;
    memcpy(out->deviceName, in->deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * sizeof(char));
    memcpy(out->pipelineCacheUUID, in->pipelineCacheUUID, VK_UUID_SIZE * sizeof(uint8_t));
    convert_VkPhysicalDeviceLimits_host_to_win32(&in->limits, &out->limits);
    out->sparseProperties = in->sparseProperties;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceProperties2_win32_to_host(const VkPhysicalDeviceProperties2 *in, VkPhysicalDeviceProperties2_host *out)
{
    if (!in) return;

    out->pNext = in->pNext;
    out->sType = in->sType;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceProperties2_host_to_win32(const VkPhysicalDeviceProperties2_host *in, VkPhysicalDeviceProperties2 *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    convert_VkPhysicalDeviceProperties_host_to_win32(&in->properties, &out->properties);
}
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceSurfaceInfo2KHR_win64_to_host(const VkPhysicalDeviceSurfaceInfo2KHR *in, VkPhysicalDeviceSurfaceInfo2KHR *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->surface = wine_surface_from_handle(in->surface)->driver_surface;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPhysicalDeviceSurfaceInfo2KHR_win32_to_host(const VkPhysicalDeviceSurfaceInfo2KHR *in, VkPhysicalDeviceSurfaceInfo2KHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->surface = wine_surface_from_handle(in->surface)->driver_surface;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPipelineExecutableInfoKHR_win32_to_host(const VkPipelineExecutableInfoKHR *in, VkPipelineExecutableInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
    out->executableIndex = in->executableIndex;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPipelineInfoKHR_win32_to_host(const VkPipelineInfoKHR *in, VkPipelineInfoKHR_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkPipelineInfoEXT_win32_to_host(const VkPipelineInfoEXT *in, VkPipelineInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->pipeline = in->pipeline;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseMemoryBind_host *convert_VkSparseMemoryBind_array_win32_to_host(struct conversion_context *ctx, const VkSparseMemoryBind *in, uint32_t count)
{
    VkSparseMemoryBind_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkSparseBufferMemoryBindInfo_host *convert_VkSparseBufferMemoryBindInfo_array_win32_to_host(struct conversion_context *ctx, const VkSparseBufferMemoryBindInfo *in, uint32_t count)
{
    VkSparseBufferMemoryBindInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].buffer = in[i].buffer;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseMemoryBind_array_win32_to_host(ctx, in[i].pBinds, in[i].bindCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseImageOpaqueMemoryBindInfo_host *convert_VkSparseImageOpaqueMemoryBindInfo_array_win32_to_host(struct conversion_context *ctx, const VkSparseImageOpaqueMemoryBindInfo *in, uint32_t count)
{
    VkSparseImageOpaqueMemoryBindInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].image = in[i].image;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseMemoryBind_array_win32_to_host(ctx, in[i].pBinds, in[i].bindCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSparseImageMemoryBind_host *convert_VkSparseImageMemoryBind_array_win32_to_host(struct conversion_context *ctx, const VkSparseImageMemoryBind *in, uint32_t count)
{
    VkSparseImageMemoryBind_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
static inline VkSparseImageMemoryBindInfo_host *convert_VkSparseImageMemoryBindInfo_array_win32_to_host(struct conversion_context *ctx, const VkSparseImageMemoryBindInfo *in, uint32_t count)
{
    VkSparseImageMemoryBindInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].image = in[i].image;
        out[i].bindCount = in[i].bindCount;
        out[i].pBinds = convert_VkSparseImageMemoryBind_array_win32_to_host(ctx, in[i].pBinds, in[i].bindCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkBindSparseInfo_host *convert_VkBindSparseInfo_array_win32_to_host(struct conversion_context *ctx, const VkBindSparseInfo *in, uint32_t count)
{
    VkBindSparseInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].waitSemaphoreCount = in[i].waitSemaphoreCount;
        out[i].pWaitSemaphores = in[i].pWaitSemaphores;
        out[i].bufferBindCount = in[i].bufferBindCount;
        out[i].pBufferBinds = convert_VkSparseBufferMemoryBindInfo_array_win32_to_host(ctx, in[i].pBufferBinds, in[i].bufferBindCount);
        out[i].imageOpaqueBindCount = in[i].imageOpaqueBindCount;
        out[i].pImageOpaqueBinds = convert_VkSparseImageOpaqueMemoryBindInfo_array_win32_to_host(ctx, in[i].pImageOpaqueBinds, in[i].imageOpaqueBindCount);
        out[i].imageBindCount = in[i].imageBindCount;
        out[i].pImageBinds = convert_VkSparseImageMemoryBindInfo_array_win32_to_host(ctx, in[i].pImageBinds, in[i].imageBindCount);
        out[i].signalSemaphoreCount = in[i].signalSemaphoreCount;
        out[i].pSignalSemaphores = in[i].pSignalSemaphores;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline VkSubmitInfo *convert_VkSubmitInfo_array_win64_to_host(struct conversion_context *ctx, const VkSubmitInfo *in, uint32_t count)
{
    VkSubmitInfo *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].waitSemaphoreCount = in[i].waitSemaphoreCount;
        out[i].pWaitSemaphores = in[i].pWaitSemaphores;
        out[i].pWaitDstStageMask = in[i].pWaitDstStageMask;
        out[i].commandBufferCount = in[i].commandBufferCount;
        out[i].pCommandBuffers = convert_VkCommandBuffer_array_win64_to_host(ctx, in[i].pCommandBuffers, in[i].commandBufferCount);
        out[i].signalSemaphoreCount = in[i].signalSemaphoreCount;
        out[i].pSignalSemaphores = in[i].pSignalSemaphores;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSubmitInfo *convert_VkSubmitInfo_array_win32_to_host(struct conversion_context *ctx, const VkSubmitInfo *in, uint32_t count)
{
    VkSubmitInfo *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].waitSemaphoreCount = in[i].waitSemaphoreCount;
        out[i].pWaitSemaphores = in[i].pWaitSemaphores;
        out[i].pWaitDstStageMask = in[i].pWaitDstStageMask;
        out[i].commandBufferCount = in[i].commandBufferCount;
        out[i].pCommandBuffers = convert_VkCommandBuffer_array_win32_to_host(ctx, in[i].pCommandBuffers, in[i].commandBufferCount);
        out[i].signalSemaphoreCount = in[i].signalSemaphoreCount;
        out[i].pSignalSemaphores = in[i].pSignalSemaphores;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSemaphoreSubmitInfo_host *convert_VkSemaphoreSubmitInfo_array_win32_to_host(struct conversion_context *ctx, const VkSemaphoreSubmitInfo *in, uint32_t count)
{
    VkSemaphoreSubmitInfo_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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

#if !defined(USE_STRUCT_CONVERSION)
static inline VkCommandBufferSubmitInfo *convert_VkCommandBufferSubmitInfo_array_win64_to_host(struct conversion_context *ctx, const VkCommandBufferSubmitInfo *in, uint32_t count)
{
    VkCommandBufferSubmitInfo *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].commandBuffer = wine_cmd_buffer_from_handle(in[i].commandBuffer)->command_buffer;
        out[i].deviceMask = in[i].deviceMask;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkCommandBufferSubmitInfo *convert_VkCommandBufferSubmitInfo_array_win32_to_host(struct conversion_context *ctx, const VkCommandBufferSubmitInfo *in, uint32_t count)
{
    VkCommandBufferSubmitInfo *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].commandBuffer = wine_cmd_buffer_from_handle(in[i].commandBuffer)->command_buffer;
        out[i].deviceMask = in[i].deviceMask;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline VkSubmitInfo2 *convert_VkSubmitInfo2_array_win64_to_host(struct conversion_context *ctx, const VkSubmitInfo2 *in, uint32_t count)
{
    VkSubmitInfo2 *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].waitSemaphoreInfoCount = in[i].waitSemaphoreInfoCount;
        out[i].pWaitSemaphoreInfos = in[i].pWaitSemaphoreInfos;
        out[i].commandBufferInfoCount = in[i].commandBufferInfoCount;
        out[i].pCommandBufferInfos = convert_VkCommandBufferSubmitInfo_array_win64_to_host(ctx, in[i].pCommandBufferInfos, in[i].commandBufferInfoCount);
        out[i].signalSemaphoreInfoCount = in[i].signalSemaphoreInfoCount;
        out[i].pSignalSemaphoreInfos = in[i].pSignalSemaphoreInfos;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkSubmitInfo2_host *convert_VkSubmitInfo2_array_win32_to_host(struct conversion_context *ctx, const VkSubmitInfo2 *in, uint32_t count)
{
    VkSubmitInfo2_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i].sType = in[i].sType;
        out[i].pNext = in[i].pNext;
        out[i].flags = in[i].flags;
        out[i].waitSemaphoreInfoCount = in[i].waitSemaphoreInfoCount;
        out[i].pWaitSemaphoreInfos = convert_VkSemaphoreSubmitInfo_array_win32_to_host(ctx, in[i].pWaitSemaphoreInfos, in[i].waitSemaphoreInfoCount);
        out[i].commandBufferInfoCount = in[i].commandBufferInfoCount;
        out[i].pCommandBufferInfos = convert_VkCommandBufferSubmitInfo_array_win32_to_host(ctx, in[i].pCommandBufferInfos, in[i].commandBufferInfoCount);
        out[i].signalSemaphoreInfoCount = in[i].signalSemaphoreInfoCount;
        out[i].pSignalSemaphoreInfos = convert_VkSemaphoreSubmitInfo_array_win32_to_host(ctx, in[i].pSignalSemaphoreInfos, in[i].signalSemaphoreInfoCount);
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsObjectNameInfoEXT_win64_to_host(const VkDebugUtilsObjectNameInfoEXT *in, VkDebugUtilsObjectNameInfoEXT *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->objectHandle = wine_vk_unwrap_handle(in->objectType, in->objectHandle);
    out->pObjectName = in->pObjectName;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsObjectNameInfoEXT_win32_to_host(const VkDebugUtilsObjectNameInfoEXT *in, VkDebugUtilsObjectNameInfoEXT_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->objectType = in->objectType;
    out->objectHandle = wine_vk_unwrap_handle(in->objectType, in->objectHandle);
    out->pObjectName = in->pObjectName;
}
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsObjectTagInfoEXT_win64_to_host(const VkDebugUtilsObjectTagInfoEXT *in, VkDebugUtilsObjectTagInfoEXT *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsObjectTagInfoEXT_win32_to_host(const VkDebugUtilsObjectTagInfoEXT *in, VkDebugUtilsObjectTagInfoEXT_host *out)
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkSemaphoreSignalInfo_win32_to_host(const VkSemaphoreSignalInfo *in, VkSemaphoreSignalInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->semaphore = in->semaphore;
    out->value = in->value;
}
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline VkDebugUtilsObjectNameInfoEXT *convert_VkDebugUtilsObjectNameInfoEXT_array_win64_to_host(struct conversion_context *ctx, const VkDebugUtilsObjectNameInfoEXT *in, uint32_t count)
{
    VkDebugUtilsObjectNameInfoEXT *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkDebugUtilsObjectNameInfoEXT_host *convert_VkDebugUtilsObjectNameInfoEXT_array_win32_to_host(struct conversion_context *ctx, const VkDebugUtilsObjectNameInfoEXT *in, uint32_t count)
{
    VkDebugUtilsObjectNameInfoEXT_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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
#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsMessengerCallbackDataEXT_win64_to_host(struct conversion_context *ctx, const VkDebugUtilsMessengerCallbackDataEXT *in, VkDebugUtilsMessengerCallbackDataEXT *out)
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
    out->pObjects = convert_VkDebugUtilsObjectNameInfoEXT_array_win64_to_host(ctx, in->pObjects, in->objectCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkDebugUtilsMessengerCallbackDataEXT_win32_to_host(struct conversion_context *ctx, const VkDebugUtilsMessengerCallbackDataEXT *in, VkDebugUtilsMessengerCallbackDataEXT_host *out)
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
    out->pObjects = convert_VkDebugUtilsObjectNameInfoEXT_array_win32_to_host(ctx, in->pObjects, in->objectCount);
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkCopyDescriptorSet_host *convert_VkCopyDescriptorSet_array_win32_to_host(struct conversion_context *ctx, const VkCopyDescriptorSet *in, uint32_t count)
{
    VkCopyDescriptorSet_host *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
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

#if !defined(USE_STRUCT_CONVERSION)
static inline VkPhysicalDevice *convert_VkPhysicalDevice_array_win64_to_host(struct conversion_context *ctx, const VkPhysicalDevice *in, uint32_t count)
{
    VkPhysicalDevice *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i] = wine_phys_dev_from_handle(in[i])->phys_dev;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

#if defined(USE_STRUCT_CONVERSION)
static inline VkPhysicalDevice *convert_VkPhysicalDevice_array_win32_to_host(struct conversion_context *ctx, const VkPhysicalDevice *in, uint32_t count)
{
    VkPhysicalDevice *out;
    unsigned int i;

    if (!in || !count) return NULL;

    out = conversion_context_alloc(ctx, count * sizeof(*out));
    for (i = 0; i < count; i++)
    {
        out[i] = wine_phys_dev_from_handle(in[i])->phys_dev;
    }

    return out;
}
#endif /* USE_STRUCT_CONVERSION */

VkResult convert_VkDeviceCreateInfo_struct_chain(struct conversion_context *ctx, const void *pNext, VkDeviceCreateInfo *out_struct)
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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->physicalDeviceCount = in->physicalDeviceCount;
#if defined(USE_STRUCT_CONVERSION)
            out->pPhysicalDevices = convert_VkPhysicalDevice_array_win32_to_host(ctx, in->pPhysicalDevices, in->physicalDeviceCount);
#else
            out->pPhysicalDevices = convert_VkPhysicalDevice_array_win64_to_host(ctx, in->pPhysicalDevices, in->physicalDeviceCount);
#endif /* USE_STRUCT_CONVERSION */

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR:
        {
            const VkPhysicalDevicePresentIdFeaturesKHR *in = (const VkPhysicalDevicePresentIdFeaturesKHR *)in_header;
            VkPhysicalDevicePresentIdFeaturesKHR *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->taskShader = in->taskShader;
            out->meshShader = in->meshShader;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT:
        {
            const VkPhysicalDeviceMeshShaderFeaturesEXT *in = (const VkPhysicalDeviceMeshShaderFeaturesEXT *)in_header;
            VkPhysicalDeviceMeshShaderFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->taskShader = in->taskShader;
            out->meshShader = in->meshShader;
            out->multiviewMeshShader = in->multiviewMeshShader;
            out->primitiveFragmentShadingRateMeshShader = in->primitiveFragmentShadingRateMeshShader;
            out->meshShaderQueries = in->meshShaderQueries;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR:
        {
            const VkPhysicalDeviceAccelerationStructureFeaturesKHR *in = (const VkPhysicalDeviceAccelerationStructureFeaturesKHR *)in_header;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->ycbcrImageArrays = in->ycbcrImageArrays;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_BARRIER_FEATURES_NV:
        {
            const VkPhysicalDevicePresentBarrierFeaturesNV *in = (const VkPhysicalDevicePresentBarrierFeaturesNV *)in_header;
            VkPhysicalDevicePresentBarrierFeaturesNV *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->presentBarrier = in->presentBarrier;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR:
        {
            const VkPhysicalDevicePerformanceQueryFeaturesKHR *in = (const VkPhysicalDevicePerformanceQueryFeaturesKHR *)in_header;
            VkPhysicalDevicePerformanceQueryFeaturesKHR *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->extendedDynamicState2 = in->extendedDynamicState2;
            out->extendedDynamicState2LogicOp = in->extendedDynamicState2LogicOp;
            out->extendedDynamicState2PatchControlPoints = in->extendedDynamicState2PatchControlPoints;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT:
        {
            const VkPhysicalDeviceExtendedDynamicState3FeaturesEXT *in = (const VkPhysicalDeviceExtendedDynamicState3FeaturesEXT *)in_header;
            VkPhysicalDeviceExtendedDynamicState3FeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->extendedDynamicState3TessellationDomainOrigin = in->extendedDynamicState3TessellationDomainOrigin;
            out->extendedDynamicState3DepthClampEnable = in->extendedDynamicState3DepthClampEnable;
            out->extendedDynamicState3PolygonMode = in->extendedDynamicState3PolygonMode;
            out->extendedDynamicState3RasterizationSamples = in->extendedDynamicState3RasterizationSamples;
            out->extendedDynamicState3SampleMask = in->extendedDynamicState3SampleMask;
            out->extendedDynamicState3AlphaToCoverageEnable = in->extendedDynamicState3AlphaToCoverageEnable;
            out->extendedDynamicState3AlphaToOneEnable = in->extendedDynamicState3AlphaToOneEnable;
            out->extendedDynamicState3LogicOpEnable = in->extendedDynamicState3LogicOpEnable;
            out->extendedDynamicState3ColorBlendEnable = in->extendedDynamicState3ColorBlendEnable;
            out->extendedDynamicState3ColorBlendEquation = in->extendedDynamicState3ColorBlendEquation;
            out->extendedDynamicState3ColorWriteMask = in->extendedDynamicState3ColorWriteMask;
            out->extendedDynamicState3RasterizationStream = in->extendedDynamicState3RasterizationStream;
            out->extendedDynamicState3ConservativeRasterizationMode = in->extendedDynamicState3ConservativeRasterizationMode;
            out->extendedDynamicState3ExtraPrimitiveOverestimationSize = in->extendedDynamicState3ExtraPrimitiveOverestimationSize;
            out->extendedDynamicState3DepthClipEnable = in->extendedDynamicState3DepthClipEnable;
            out->extendedDynamicState3SampleLocationsEnable = in->extendedDynamicState3SampleLocationsEnable;
            out->extendedDynamicState3ColorBlendAdvanced = in->extendedDynamicState3ColorBlendAdvanced;
            out->extendedDynamicState3ProvokingVertexMode = in->extendedDynamicState3ProvokingVertexMode;
            out->extendedDynamicState3LineRasterizationMode = in->extendedDynamicState3LineRasterizationMode;
            out->extendedDynamicState3LineStippleEnable = in->extendedDynamicState3LineStippleEnable;
            out->extendedDynamicState3DepthClipNegativeOneToOne = in->extendedDynamicState3DepthClipNegativeOneToOne;
            out->extendedDynamicState3ViewportWScalingEnable = in->extendedDynamicState3ViewportWScalingEnable;
            out->extendedDynamicState3ViewportSwizzle = in->extendedDynamicState3ViewportSwizzle;
            out->extendedDynamicState3CoverageToColorEnable = in->extendedDynamicState3CoverageToColorEnable;
            out->extendedDynamicState3CoverageToColorLocation = in->extendedDynamicState3CoverageToColorLocation;
            out->extendedDynamicState3CoverageModulationMode = in->extendedDynamicState3CoverageModulationMode;
            out->extendedDynamicState3CoverageModulationTableEnable = in->extendedDynamicState3CoverageModulationTableEnable;
            out->extendedDynamicState3CoverageModulationTable = in->extendedDynamicState3CoverageModulationTable;
            out->extendedDynamicState3CoverageReductionMode = in->extendedDynamicState3CoverageReductionMode;
            out->extendedDynamicState3RepresentativeFragmentTestEnable = in->extendedDynamicState3RepresentativeFragmentTestEnable;
            out->extendedDynamicState3ShadingRateImageEnable = in->extendedDynamicState3ShadingRateImageEnable;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV:
        {
            const VkPhysicalDeviceDiagnosticsConfigFeaturesNV *in = (const VkPhysicalDeviceDiagnosticsConfigFeaturesNV *)in_header;
            VkPhysicalDeviceDiagnosticsConfigFeaturesNV *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->image2DViewOf3D = in->image2DViewOf3D;
            out->sampler2DViewOf3D = in->sampler2DViewOf3D;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT:
        {
            const VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT *in = (const VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT *)in_header;
            VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->primitivesGeneratedQuery = in->primitivesGeneratedQuery;
            out->primitivesGeneratedQueryWithRasterizerDiscard = in->primitivesGeneratedQueryWithRasterizerDiscard;
            out->primitivesGeneratedQueryWithNonZeroStreams = in->primitivesGeneratedQueryWithNonZeroStreams;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LEGACY_DITHERING_FEATURES_EXT:
        {
            const VkPhysicalDeviceLegacyDitheringFeaturesEXT *in = (const VkPhysicalDeviceLegacyDitheringFeaturesEXT *)in_header;
            VkPhysicalDeviceLegacyDitheringFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->legacyDithering = in->legacyDithering;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT:
        {
            const VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT *in = (const VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT *)in_header;
            VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->multisampledRenderToSingleSampled = in->multisampledRenderToSingleSampled;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROTECTED_ACCESS_FEATURES_EXT:
        {
            const VkPhysicalDevicePipelineProtectedAccessFeaturesEXT *in = (const VkPhysicalDevicePipelineProtectedAccessFeaturesEXT *)in_header;
            VkPhysicalDevicePipelineProtectedAccessFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->pipelineProtectedAccess = in->pipelineProtectedAccess;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV:
        {
            const VkPhysicalDeviceInheritedViewportScissorFeaturesNV *in = (const VkPhysicalDeviceInheritedViewportScissorFeaturesNV *)in_header;
            VkPhysicalDeviceInheritedViewportScissorFeaturesNV *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->minLod = in->minLod;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT:
        {
            const VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT *in = (const VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT *)in_header;
            VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->subpassMergeFeedback = in->subpassMergeFeedback;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT:
        {
            const VkPhysicalDeviceOpacityMicromapFeaturesEXT *in = (const VkPhysicalDeviceOpacityMicromapFeaturesEXT *)in_header;
            VkPhysicalDeviceOpacityMicromapFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->micromap = in->micromap;
            out->micromapCaptureReplay = in->micromapCaptureReplay;
            out->micromapHostCommands = in->micromapHostCommands;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROPERTIES_FEATURES_EXT:
        {
            const VkPhysicalDevicePipelinePropertiesFeaturesEXT *in = (const VkPhysicalDevicePipelinePropertiesFeaturesEXT *)in_header;
            VkPhysicalDevicePipelinePropertiesFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->attachmentFeedbackLoopLayout = in->attachmentFeedbackLoopLayout;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_EXT:
        {
            const VkPhysicalDeviceDepthClampZeroOneFeaturesEXT *in = (const VkPhysicalDeviceDepthClampZeroOneFeaturesEXT *)in_header;
            VkPhysicalDeviceDepthClampZeroOneFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->depthClampZeroOne = in->depthClampZeroOne;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ADDRESS_BINDING_REPORT_FEATURES_EXT:
        {
            const VkPhysicalDeviceAddressBindingReportFeaturesEXT *in = (const VkPhysicalDeviceAddressBindingReportFeaturesEXT *)in_header;
            VkPhysicalDeviceAddressBindingReportFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->reportAddressBinding = in->reportAddressBinding;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_FEATURES_NV:
        {
            const VkPhysicalDeviceOpticalFlowFeaturesNV *in = (const VkPhysicalDeviceOpticalFlowFeaturesNV *)in_header;
            VkPhysicalDeviceOpticalFlowFeaturesNV *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->opticalFlow = in->opticalFlow;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT:
        {
            const VkPhysicalDeviceFaultFeaturesEXT *in = (const VkPhysicalDeviceFaultFeaturesEXT *)in_header;
            VkPhysicalDeviceFaultFeaturesEXT *out;

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

            out->sType = in->sType;
            out->pNext = NULL;
            out->deviceFault = in->deviceFault;
            out->deviceFaultVendorBinary = in->deviceFaultVendorBinary;

            out_header->pNext = (VkBaseOutStructure *)out;
            out_header = out_header->pNext;
            break;
        }

        default:
            FIXME("Application requested a linked structure of type %u.\n", in_header->sType);
        }
    }

    return VK_SUCCESS;
}

VkResult convert_VkInstanceCreateInfo_struct_chain(struct conversion_context *ctx, const void *pNext, VkInstanceCreateInfo *out_struct)
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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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

            if (!(out = conversion_context_alloc(ctx, sizeof(*out)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

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
}

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkAcquireNextImage2KHR(void *args)
{
    struct vkAcquireNextImage2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAcquireInfo, params->pImageIndex);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquireNextImage2KHR(wine_device_from_handle(params->device)->device, params->pAcquireInfo, params->pImageIndex);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkAcquireNextImage2KHR(void *args)
{
    struct vkAcquireNextImage2KHR_params *params = args;
    VkAcquireNextImageInfoKHR_host pAcquireInfo_host;
    TRACE("%p, %p, %p\n", params->device, params->pAcquireInfo, params->pImageIndex);

    convert_VkAcquireNextImageInfoKHR_win32_to_host(params->pAcquireInfo, &pAcquireInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquireNextImage2KHR(wine_device_from_handle(params->device)->device, &pAcquireInfo_host, params->pImageIndex);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkAcquireNextImageKHR(void *args)
{
    struct vkAcquireNextImageKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), wine_dbgstr_longlong(params->timeout), wine_dbgstr_longlong(params->semaphore), wine_dbgstr_longlong(params->fence), params->pImageIndex);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquireNextImageKHR(wine_device_from_handle(params->device)->device, params->swapchain, params->timeout, params->semaphore, params->fence, params->pImageIndex);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkAcquireNextImageKHR(void *args)
{
    struct vkAcquireNextImageKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), wine_dbgstr_longlong(params->timeout), wine_dbgstr_longlong(params->semaphore), wine_dbgstr_longlong(params->fence), params->pImageIndex);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquireNextImageKHR(wine_device_from_handle(params->device)->device, params->swapchain, params->timeout, params->semaphore, params->fence, params->pImageIndex);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkAcquirePerformanceConfigurationINTEL(void *args)
{
    struct vkAcquirePerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAcquireInfo, params->pConfiguration);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquirePerformanceConfigurationINTEL(wine_device_from_handle(params->device)->device, params->pAcquireInfo, params->pConfiguration);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkAcquirePerformanceConfigurationINTEL(void *args)
{
    struct vkAcquirePerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAcquireInfo, params->pConfiguration);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquirePerformanceConfigurationINTEL(wine_device_from_handle(params->device)->device, params->pAcquireInfo, params->pConfiguration);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkAcquireProfilingLockKHR(void *args)
{
    struct vkAcquireProfilingLockKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquireProfilingLockKHR(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkAcquireProfilingLockKHR(void *args)
{
    struct vkAcquireProfilingLockKHR_params *params = args;
    VkAcquireProfilingLockInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkAcquireProfilingLockInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkAcquireProfilingLockKHR(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkAllocateCommandBuffers(void *args)
{
    struct vkAllocateCommandBuffers_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAllocateInfo, params->pCommandBuffers);

    params->result = wine_vkAllocateCommandBuffers(params->device, params->pAllocateInfo, params->pCommandBuffers);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkAllocateCommandBuffers(void *args)
{
    struct vkAllocateCommandBuffers_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAllocateInfo, params->pCommandBuffers);

    params->result = wine_vkAllocateCommandBuffers(params->device, params->pAllocateInfo, params->pCommandBuffers);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkAllocateDescriptorSets(void *args)
{
    struct vkAllocateDescriptorSets_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAllocateInfo, params->pDescriptorSets);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAllocateDescriptorSets(wine_device_from_handle(params->device)->device, params->pAllocateInfo, params->pDescriptorSets);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkAllocateDescriptorSets(void *args)
{
    struct vkAllocateDescriptorSets_params *params = args;
    VkDescriptorSetAllocateInfo_host pAllocateInfo_host;
    TRACE("%p, %p, %p\n", params->device, params->pAllocateInfo, params->pDescriptorSets);

    convert_VkDescriptorSetAllocateInfo_win32_to_host(params->pAllocateInfo, &pAllocateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkAllocateDescriptorSets(wine_device_from_handle(params->device)->device, &pAllocateInfo_host, params->pDescriptorSets);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkAllocateMemory(void *args)
{
    struct vkAllocateMemory_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pAllocateInfo, params->pAllocator, params->pMemory);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkAllocateMemory(wine_device_from_handle(params->device)->device, params->pAllocateInfo, NULL, params->pMemory);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkAllocateMemory(void *args)
{
    struct vkAllocateMemory_params *params = args;
    VkMemoryAllocateInfo_host pAllocateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pAllocateInfo, params->pAllocator, params->pMemory);

    convert_VkMemoryAllocateInfo_win32_to_host(params->pAllocateInfo, &pAllocateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkAllocateMemory(wine_device_from_handle(params->device)->device, &pAllocateInfo_host, NULL, params->pMemory);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBeginCommandBuffer(void *args)
{
    struct vkBeginCommandBuffer_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pBeginInfo);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkBeginCommandBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pBeginInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBeginCommandBuffer(void *args)
{
    struct vkBeginCommandBuffer_params *params = args;
    VkCommandBufferBeginInfo_host pBeginInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pBeginInfo);

    init_conversion_context(&ctx);
    convert_VkCommandBufferBeginInfo_win32_to_host(&ctx, params->pBeginInfo, &pBeginInfo_host);
    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkBeginCommandBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pBeginInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindAccelerationStructureMemoryNV(void *args)
{
    struct vkBindAccelerationStructureMemoryNV_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindAccelerationStructureMemoryNV(wine_device_from_handle(params->device)->device, params->bindInfoCount, params->pBindInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindAccelerationStructureMemoryNV(void *args)
{
    struct vkBindAccelerationStructureMemoryNV_params *params = args;
    VkBindAccelerationStructureMemoryInfoNV_host *pBindInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    init_conversion_context(&ctx);
    pBindInfos_host = convert_VkBindAccelerationStructureMemoryInfoNV_array_win32_to_host(&ctx, params->pBindInfos, params->bindInfoCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindAccelerationStructureMemoryNV(wine_device_from_handle(params->device)->device, params->bindInfoCount, pBindInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindBufferMemory(void *args)
{
    struct vkBindBufferMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->memoryOffset));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindBufferMemory(wine_device_from_handle(params->device)->device, params->buffer, params->memory, params->memoryOffset);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindBufferMemory(void *args)
{
    struct vkBindBufferMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->memoryOffset));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindBufferMemory(wine_device_from_handle(params->device)->device, params->buffer, params->memory, params->memoryOffset);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindBufferMemory2(void *args)
{
    struct vkBindBufferMemory2_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindBufferMemory2(wine_device_from_handle(params->device)->device, params->bindInfoCount, params->pBindInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindBufferMemory2(void *args)
{
    struct vkBindBufferMemory2_params *params = args;
    VkBindBufferMemoryInfo_host *pBindInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    init_conversion_context(&ctx);
    pBindInfos_host = convert_VkBindBufferMemoryInfo_array_win32_to_host(&ctx, params->pBindInfos, params->bindInfoCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindBufferMemory2(wine_device_from_handle(params->device)->device, params->bindInfoCount, pBindInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindBufferMemory2KHR(void *args)
{
    struct vkBindBufferMemory2KHR_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindBufferMemory2KHR(wine_device_from_handle(params->device)->device, params->bindInfoCount, params->pBindInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindBufferMemory2KHR(void *args)
{
    struct vkBindBufferMemory2KHR_params *params = args;
    VkBindBufferMemoryInfo_host *pBindInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    init_conversion_context(&ctx);
    pBindInfos_host = convert_VkBindBufferMemoryInfo_array_win32_to_host(&ctx, params->pBindInfos, params->bindInfoCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindBufferMemory2KHR(wine_device_from_handle(params->device)->device, params->bindInfoCount, pBindInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindImageMemory(void *args)
{
    struct vkBindImageMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->image), wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->memoryOffset));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindImageMemory(wine_device_from_handle(params->device)->device, params->image, params->memory, params->memoryOffset);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindImageMemory(void *args)
{
    struct vkBindImageMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->image), wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->memoryOffset));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindImageMemory(wine_device_from_handle(params->device)->device, params->image, params->memory, params->memoryOffset);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindImageMemory2(void *args)
{
    struct vkBindImageMemory2_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindImageMemory2(wine_device_from_handle(params->device)->device, params->bindInfoCount, params->pBindInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindImageMemory2(void *args)
{
    struct vkBindImageMemory2_params *params = args;
    VkBindImageMemoryInfo_host *pBindInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    init_conversion_context(&ctx);
    pBindInfos_host = convert_VkBindImageMemoryInfo_array_win32_to_host(&ctx, params->pBindInfos, params->bindInfoCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindImageMemory2(wine_device_from_handle(params->device)->device, params->bindInfoCount, pBindInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindImageMemory2KHR(void *args)
{
    struct vkBindImageMemory2KHR_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindImageMemory2KHR(wine_device_from_handle(params->device)->device, params->bindInfoCount, params->pBindInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindImageMemory2KHR(void *args)
{
    struct vkBindImageMemory2KHR_params *params = args;
    VkBindImageMemoryInfo_host *pBindInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->device, params->bindInfoCount, params->pBindInfos);

    init_conversion_context(&ctx);
    pBindInfos_host = convert_VkBindImageMemoryInfo_array_win32_to_host(&ctx, params->pBindInfos, params->bindInfoCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindImageMemory2KHR(wine_device_from_handle(params->device)->device, params->bindInfoCount, pBindInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBindOpticalFlowSessionImageNV(void *args)
{
    struct vkBindOpticalFlowSessionImageNV_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->session), params->bindingPoint, wine_dbgstr_longlong(params->view), params->layout);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindOpticalFlowSessionImageNV(wine_device_from_handle(params->device)->device, params->session, params->bindingPoint, params->view, params->layout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBindOpticalFlowSessionImageNV(void *args)
{
    struct vkBindOpticalFlowSessionImageNV_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->session), params->bindingPoint, wine_dbgstr_longlong(params->view), params->layout);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBindOpticalFlowSessionImageNV(wine_device_from_handle(params->device)->device, params->session, params->bindingPoint, params->view, params->layout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBuildAccelerationStructuresKHR(void *args)
{
    struct vkBuildAccelerationStructuresKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %p, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->infoCount, params->pInfos, params->ppBuildRangeInfos);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBuildAccelerationStructuresKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, params->infoCount, params->pInfos, params->ppBuildRangeInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBuildAccelerationStructuresKHR(void *args)
{
    struct vkBuildAccelerationStructuresKHR_params *params = args;
    VkAccelerationStructureBuildGeometryInfoKHR_host *pInfos_host;
    struct conversion_context ctx;
    TRACE("%p, 0x%s, %u, %p, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->infoCount, params->pInfos, params->ppBuildRangeInfos);

    init_conversion_context(&ctx);
    pInfos_host = convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win32_to_host(&ctx, params->pInfos, params->infoCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkBuildAccelerationStructuresKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, params->infoCount, pInfos_host, params->ppBuildRangeInfos);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkBuildMicromapsEXT(void *args)
{
    struct vkBuildMicromapsEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->infoCount, params->pInfos);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkBuildMicromapsEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, params->infoCount, params->pInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkBuildMicromapsEXT(void *args)
{
    struct vkBuildMicromapsEXT_params *params = args;
    VkMicromapBuildInfoEXT_host *pInfos_host;
    struct conversion_context ctx;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->infoCount, params->pInfos);

    init_conversion_context(&ctx);
    pInfos_host = convert_VkMicromapBuildInfoEXT_array_win32_to_host(&ctx, params->pInfos, params->infoCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkBuildMicromapsEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, params->infoCount, pInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginConditionalRenderingEXT(void *args)
{
    struct vkCmdBeginConditionalRenderingEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pConditionalRenderingBegin);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginConditionalRenderingEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pConditionalRenderingBegin);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginConditionalRenderingEXT(void *args)
{
    struct vkCmdBeginConditionalRenderingEXT_params *params = args;
    VkConditionalRenderingBeginInfoEXT_host pConditionalRenderingBegin_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pConditionalRenderingBegin);

    convert_VkConditionalRenderingBeginInfoEXT_win32_to_host(params->pConditionalRenderingBegin, &pConditionalRenderingBegin_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginConditionalRenderingEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pConditionalRenderingBegin_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginDebugUtilsLabelEXT(void *args)
{
    struct vkCmdBeginDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pLabelInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginDebugUtilsLabelEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginDebugUtilsLabelEXT(void *args)
{
    struct vkCmdBeginDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pLabelInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginDebugUtilsLabelEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginQuery(void *args)
{
    struct vkCmdBeginQuery_params *params = args;
    TRACE("%p, 0x%s, %u, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->flags);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginQuery(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginQuery(void *args)
{
    struct vkCmdBeginQuery_params *params = args;
    TRACE("%p, 0x%s, %u, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->flags);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginQuery(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginQueryIndexedEXT(void *args)
{
    struct vkCmdBeginQueryIndexedEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %#x, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->flags, params->index);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginQueryIndexedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query, params->flags, params->index);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginQueryIndexedEXT(void *args)
{
    struct vkCmdBeginQueryIndexedEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %#x, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->flags, params->index);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginQueryIndexedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query, params->flags, params->index);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginRenderPass(void *args)
{
    struct vkCmdBeginRenderPass_params *params = args;
    TRACE("%p, %p, %#x\n", params->commandBuffer, params->pRenderPassBegin, params->contents);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderPass(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pRenderPassBegin, params->contents);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginRenderPass(void *args)
{
    struct vkCmdBeginRenderPass_params *params = args;
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %#x\n", params->commandBuffer, params->pRenderPassBegin, params->contents);

    convert_VkRenderPassBeginInfo_win32_to_host(params->pRenderPassBegin, &pRenderPassBegin_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderPass(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pRenderPassBegin_host, params->contents);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginRenderPass2(void *args)
{
    struct vkCmdBeginRenderPass2_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderPass2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pRenderPassBegin, params->pSubpassBeginInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginRenderPass2(void *args)
{
    struct vkCmdBeginRenderPass2_params *params = args;
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);

    convert_VkRenderPassBeginInfo_win32_to_host(params->pRenderPassBegin, &pRenderPassBegin_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderPass2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pRenderPassBegin_host, params->pSubpassBeginInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginRenderPass2KHR(void *args)
{
    struct vkCmdBeginRenderPass2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderPass2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pRenderPassBegin, params->pSubpassBeginInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginRenderPass2KHR(void *args)
{
    struct vkCmdBeginRenderPass2KHR_params *params = args;
    VkRenderPassBeginInfo_host pRenderPassBegin_host;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pRenderPassBegin, params->pSubpassBeginInfo);

    convert_VkRenderPassBeginInfo_win32_to_host(params->pRenderPassBegin, &pRenderPassBegin_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderPass2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pRenderPassBegin_host, params->pSubpassBeginInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginRendering(void *args)
{
    struct vkCmdBeginRendering_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRendering(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pRenderingInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginRendering(void *args)
{
    struct vkCmdBeginRendering_params *params = args;
    VkRenderingInfo_host pRenderingInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);

    init_conversion_context(&ctx);
    convert_VkRenderingInfo_win32_to_host(&ctx, params->pRenderingInfo, &pRenderingInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRendering(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pRenderingInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginRenderingKHR(void *args)
{
    struct vkCmdBeginRenderingKHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderingKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pRenderingInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginRenderingKHR(void *args)
{
    struct vkCmdBeginRenderingKHR_params *params = args;
    VkRenderingInfo_host pRenderingInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pRenderingInfo);

    init_conversion_context(&ctx);
    convert_VkRenderingInfo_win32_to_host(&ctx, params->pRenderingInfo, &pRenderingInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginRenderingKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pRenderingInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBeginTransformFeedbackEXT(void *args)
{
    struct vkCmdBeginTransformFeedbackEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginTransformFeedbackEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBeginTransformFeedbackEXT(void *args)
{
    struct vkCmdBeginTransformFeedbackEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBeginTransformFeedbackEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindDescriptorSets(void *args)
{
    struct vkCmdBindDescriptorSets_params *params = args;
    TRACE("%p, %#x, 0x%s, %u, %u, %p, %u, %p\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->layout), params->firstSet, params->descriptorSetCount, params->pDescriptorSets, params->dynamicOffsetCount, params->pDynamicOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindDescriptorSets(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->layout, params->firstSet, params->descriptorSetCount, params->pDescriptorSets, params->dynamicOffsetCount, params->pDynamicOffsets);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindDescriptorSets(void *args)
{
    struct vkCmdBindDescriptorSets_params *params = args;
    TRACE("%p, %#x, 0x%s, %u, %u, %p, %u, %p\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->layout), params->firstSet, params->descriptorSetCount, params->pDescriptorSets, params->dynamicOffsetCount, params->pDynamicOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindDescriptorSets(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->layout, params->firstSet, params->descriptorSetCount, params->pDescriptorSets, params->dynamicOffsetCount, params->pDynamicOffsets);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindIndexBuffer(void *args)
{
    struct vkCmdBindIndexBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->indexType);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindIndexBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->indexType);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindIndexBuffer(void *args)
{
    struct vkCmdBindIndexBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->indexType);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindIndexBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->indexType);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindInvocationMaskHUAWEI(void *args)
{
    struct vkCmdBindInvocationMaskHUAWEI_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->imageView), params->imageLayout);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindInvocationMaskHUAWEI(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->imageView, params->imageLayout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindInvocationMaskHUAWEI(void *args)
{
    struct vkCmdBindInvocationMaskHUAWEI_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->imageView), params->imageLayout);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindInvocationMaskHUAWEI(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->imageView, params->imageLayout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindPipeline(void *args)
{
    struct vkCmdBindPipeline_params *params = args;
    TRACE("%p, %#x, 0x%s\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->pipeline));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindPipeline(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->pipeline);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindPipeline(void *args)
{
    struct vkCmdBindPipeline_params *params = args;
    TRACE("%p, %#x, 0x%s\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->pipeline));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindPipeline(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->pipeline);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindPipelineShaderGroupNV(void *args)
{
    struct vkCmdBindPipelineShaderGroupNV_params *params = args;
    TRACE("%p, %#x, 0x%s, %u\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->pipeline), params->groupIndex);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindPipelineShaderGroupNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->pipeline, params->groupIndex);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindPipelineShaderGroupNV(void *args)
{
    struct vkCmdBindPipelineShaderGroupNV_params *params = args;
    TRACE("%p, %#x, 0x%s, %u\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->pipeline), params->groupIndex);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindPipelineShaderGroupNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->pipeline, params->groupIndex);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindShadingRateImageNV(void *args)
{
    struct vkCmdBindShadingRateImageNV_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->imageView), params->imageLayout);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindShadingRateImageNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->imageView, params->imageLayout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindShadingRateImageNV(void *args)
{
    struct vkCmdBindShadingRateImageNV_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->imageView), params->imageLayout);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindShadingRateImageNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->imageView, params->imageLayout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindTransformFeedbackBuffersEXT(void *args)
{
    struct vkCmdBindTransformFeedbackBuffersEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindTransformFeedbackBuffersEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindTransformFeedbackBuffersEXT(void *args)
{
    struct vkCmdBindTransformFeedbackBuffersEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindTransformFeedbackBuffersEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindVertexBuffers(void *args)
{
    struct vkCmdBindVertexBuffers_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindVertexBuffers(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindVertexBuffers(void *args)
{
    struct vkCmdBindVertexBuffers_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindVertexBuffers(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindVertexBuffers2(void *args)
{
    struct vkCmdBindVertexBuffers2_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindVertexBuffers2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindVertexBuffers2(void *args)
{
    struct vkCmdBindVertexBuffers2_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindVertexBuffers2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBindVertexBuffers2EXT(void *args)
{
    struct vkCmdBindVertexBuffers2EXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindVertexBuffers2EXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBindVertexBuffers2EXT(void *args)
{
    struct vkCmdBindVertexBuffers2EXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p, %p, %p\n", params->commandBuffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBindVertexBuffers2EXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstBinding, params->bindingCount, params->pBuffers, params->pOffsets, params->pSizes, params->pStrides);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBlitImage(void *args)
{
    struct vkCmdBlitImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions, params->filter);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBlitImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions, params->filter);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBlitImage(void *args)
{
    struct vkCmdBlitImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions, params->filter);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBlitImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions, params->filter);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBlitImage2(void *args)
{
    struct vkCmdBlitImage2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBlitImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pBlitImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBlitImage2(void *args)
{
    struct vkCmdBlitImage2_params *params = args;
    VkBlitImageInfo2_host pBlitImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);

    convert_VkBlitImageInfo2_win32_to_host(params->pBlitImageInfo, &pBlitImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBlitImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pBlitImageInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBlitImage2KHR(void *args)
{
    struct vkCmdBlitImage2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBlitImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pBlitImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBlitImage2KHR(void *args)
{
    struct vkCmdBlitImage2KHR_params *params = args;
    VkBlitImageInfo2_host pBlitImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pBlitImageInfo);

    convert_VkBlitImageInfo2_win32_to_host(params->pBlitImageInfo, &pBlitImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBlitImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pBlitImageInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBuildAccelerationStructureNV(void *args)
{
    struct vkCmdBuildAccelerationStructureNV_params *params = args;
    TRACE("%p, %p, 0x%s, 0x%s, %u, 0x%s, 0x%s, 0x%s, 0x%s\n", params->commandBuffer, params->pInfo, wine_dbgstr_longlong(params->instanceData), wine_dbgstr_longlong(params->instanceOffset), params->update, wine_dbgstr_longlong(params->dst), wine_dbgstr_longlong(params->src), wine_dbgstr_longlong(params->scratch), wine_dbgstr_longlong(params->scratchOffset));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildAccelerationStructureNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pInfo, params->instanceData, params->instanceOffset, params->update, params->dst, params->src, params->scratch, params->scratchOffset);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBuildAccelerationStructureNV(void *args)
{
    struct vkCmdBuildAccelerationStructureNV_params *params = args;
    VkAccelerationStructureInfoNV_host pInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p, 0x%s, 0x%s, %u, 0x%s, 0x%s, 0x%s, 0x%s\n", params->commandBuffer, params->pInfo, wine_dbgstr_longlong(params->instanceData), wine_dbgstr_longlong(params->instanceOffset), params->update, wine_dbgstr_longlong(params->dst), wine_dbgstr_longlong(params->src), wine_dbgstr_longlong(params->scratch), wine_dbgstr_longlong(params->scratchOffset));

    init_conversion_context(&ctx);
    convert_VkAccelerationStructureInfoNV_win32_to_host(&ctx, params->pInfo, &pInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildAccelerationStructureNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pInfo_host, params->instanceData, params->instanceOffset, params->update, params->dst, params->src, params->scratch, params->scratchOffset);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBuildAccelerationStructuresIndirectKHR(void *args)
{
    struct vkCmdBuildAccelerationStructuresIndirectKHR_params *params = args;
    TRACE("%p, %u, %p, %p, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildAccelerationStructuresIndirectKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->infoCount, params->pInfos, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBuildAccelerationStructuresIndirectKHR(void *args)
{
    struct vkCmdBuildAccelerationStructuresIndirectKHR_params *params = args;
    VkAccelerationStructureBuildGeometryInfoKHR_host *pInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, %p, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);

    init_conversion_context(&ctx);
    pInfos_host = convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win32_to_host(&ctx, params->pInfos, params->infoCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildAccelerationStructuresIndirectKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->infoCount, pInfos_host, params->pIndirectDeviceAddresses, params->pIndirectStrides, params->ppMaxPrimitiveCounts);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBuildAccelerationStructuresKHR(void *args)
{
    struct vkCmdBuildAccelerationStructuresKHR_params *params = args;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->ppBuildRangeInfos);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildAccelerationStructuresKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->infoCount, params->pInfos, params->ppBuildRangeInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBuildAccelerationStructuresKHR(void *args)
{
    struct vkCmdBuildAccelerationStructuresKHR_params *params = args;
    VkAccelerationStructureBuildGeometryInfoKHR_host *pInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->infoCount, params->pInfos, params->ppBuildRangeInfos);

    init_conversion_context(&ctx);
    pInfos_host = convert_VkAccelerationStructureBuildGeometryInfoKHR_array_win32_to_host(&ctx, params->pInfos, params->infoCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildAccelerationStructuresKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->infoCount, pInfos_host, params->ppBuildRangeInfos);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdBuildMicromapsEXT(void *args)
{
    struct vkCmdBuildMicromapsEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->infoCount, params->pInfos);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildMicromapsEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->infoCount, params->pInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdBuildMicromapsEXT(void *args)
{
    struct vkCmdBuildMicromapsEXT_params *params = args;
    VkMicromapBuildInfoEXT_host *pInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->infoCount, params->pInfos);

    init_conversion_context(&ctx);
    pInfos_host = convert_VkMicromapBuildInfoEXT_array_win32_to_host(&ctx, params->pInfos, params->infoCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdBuildMicromapsEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->infoCount, pInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdClearAttachments(void *args)
{
    struct vkCmdClearAttachments_params *params = args;
    TRACE("%p, %u, %p, %u, %p\n", params->commandBuffer, params->attachmentCount, params->pAttachments, params->rectCount, params->pRects);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdClearAttachments(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->attachmentCount, params->pAttachments, params->rectCount, params->pRects);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdClearAttachments(void *args)
{
    struct vkCmdClearAttachments_params *params = args;
    TRACE("%p, %u, %p, %u, %p\n", params->commandBuffer, params->attachmentCount, params->pAttachments, params->rectCount, params->pRects);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdClearAttachments(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->attachmentCount, params->pAttachments, params->rectCount, params->pRects);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdClearColorImage(void *args)
{
    struct vkCmdClearColorImage_params *params = args;
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->image), params->imageLayout, params->pColor, params->rangeCount, params->pRanges);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdClearColorImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->image, params->imageLayout, params->pColor, params->rangeCount, params->pRanges);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdClearColorImage(void *args)
{
    struct vkCmdClearColorImage_params *params = args;
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->image), params->imageLayout, params->pColor, params->rangeCount, params->pRanges);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdClearColorImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->image, params->imageLayout, params->pColor, params->rangeCount, params->pRanges);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdClearDepthStencilImage(void *args)
{
    struct vkCmdClearDepthStencilImage_params *params = args;
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->image), params->imageLayout, params->pDepthStencil, params->rangeCount, params->pRanges);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdClearDepthStencilImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->image, params->imageLayout, params->pDepthStencil, params->rangeCount, params->pRanges);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdClearDepthStencilImage(void *args)
{
    struct vkCmdClearDepthStencilImage_params *params = args;
    TRACE("%p, 0x%s, %#x, %p, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->image), params->imageLayout, params->pDepthStencil, params->rangeCount, params->pRanges);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdClearDepthStencilImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->image, params->imageLayout, params->pDepthStencil, params->rangeCount, params->pRanges);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyAccelerationStructureKHR(void *args)
{
    struct vkCmdCopyAccelerationStructureKHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyAccelerationStructureKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyAccelerationStructureKHR(void *args)
{
    struct vkCmdCopyAccelerationStructureKHR_params *params = args;
    VkCopyAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyAccelerationStructureInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyAccelerationStructureKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyAccelerationStructureNV(void *args)
{
    struct vkCmdCopyAccelerationStructureNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->dst), wine_dbgstr_longlong(params->src), params->mode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyAccelerationStructureNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->dst, params->src, params->mode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyAccelerationStructureNV(void *args)
{
    struct vkCmdCopyAccelerationStructureNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->dst), wine_dbgstr_longlong(params->src), params->mode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyAccelerationStructureNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->dst, params->src, params->mode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyAccelerationStructureToMemoryKHR(void *args)
{
    struct vkCmdCopyAccelerationStructureToMemoryKHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyAccelerationStructureToMemoryKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyAccelerationStructureToMemoryKHR(void *args)
{
    struct vkCmdCopyAccelerationStructureToMemoryKHR_params *params = args;
    VkCopyAccelerationStructureToMemoryInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyAccelerationStructureToMemoryInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyAccelerationStructureToMemoryKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyBuffer(void *args)
{
    struct vkCmdCopyBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcBuffer, params->dstBuffer, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyBuffer(void *args)
{
    struct vkCmdCopyBuffer_params *params = args;
    VkBufferCopy_host *pRegions_host;
    struct conversion_context ctx;
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);

    init_conversion_context(&ctx);
    pRegions_host = convert_VkBufferCopy_array_win32_to_host(&ctx, params->pRegions, params->regionCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcBuffer, params->dstBuffer, params->regionCount, pRegions_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyBuffer2(void *args)
{
    struct vkCmdCopyBuffer2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBuffer2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyBufferInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyBuffer2(void *args)
{
    struct vkCmdCopyBuffer2_params *params = args;
    VkCopyBufferInfo2_host pCopyBufferInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);

    init_conversion_context(&ctx);
    convert_VkCopyBufferInfo2_win32_to_host(&ctx, params->pCopyBufferInfo, &pCopyBufferInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBuffer2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyBufferInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyBuffer2KHR(void *args)
{
    struct vkCmdCopyBuffer2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBuffer2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyBufferInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyBuffer2KHR(void *args)
{
    struct vkCmdCopyBuffer2KHR_params *params = args;
    VkCopyBufferInfo2_host pCopyBufferInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferInfo);

    init_conversion_context(&ctx);
    convert_VkCopyBufferInfo2_win32_to_host(&ctx, params->pCopyBufferInfo, &pCopyBufferInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBuffer2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyBufferInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyBufferToImage(void *args)
{
    struct vkCmdCopyBufferToImage_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBufferToImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcBuffer, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyBufferToImage(void *args)
{
    struct vkCmdCopyBufferToImage_params *params = args;
    VkBufferImageCopy_host *pRegions_host;
    struct conversion_context ctx;
    TRACE("%p, 0x%s, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcBuffer), wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);

    init_conversion_context(&ctx);
    pRegions_host = convert_VkBufferImageCopy_array_win32_to_host(&ctx, params->pRegions, params->regionCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBufferToImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcBuffer, params->dstImage, params->dstImageLayout, params->regionCount, pRegions_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyBufferToImage2(void *args)
{
    struct vkCmdCopyBufferToImage2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBufferToImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyBufferToImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyBufferToImage2(void *args)
{
    struct vkCmdCopyBufferToImage2_params *params = args;
    VkCopyBufferToImageInfo2_host pCopyBufferToImageInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);

    init_conversion_context(&ctx);
    convert_VkCopyBufferToImageInfo2_win32_to_host(&ctx, params->pCopyBufferToImageInfo, &pCopyBufferToImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBufferToImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyBufferToImageInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyBufferToImage2KHR(void *args)
{
    struct vkCmdCopyBufferToImage2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBufferToImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyBufferToImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyBufferToImage2KHR(void *args)
{
    struct vkCmdCopyBufferToImage2KHR_params *params = args;
    VkCopyBufferToImageInfo2_host pCopyBufferToImageInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyBufferToImageInfo);

    init_conversion_context(&ctx);
    convert_VkCopyBufferToImageInfo2_win32_to_host(&ctx, params->pCopyBufferToImageInfo, &pCopyBufferToImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyBufferToImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyBufferToImageInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyImage(void *args)
{
    struct vkCmdCopyImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyImage(void *args)
{
    struct vkCmdCopyImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyImage2(void *args)
{
    struct vkCmdCopyImage2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyImage2(void *args)
{
    struct vkCmdCopyImage2_params *params = args;
    VkCopyImageInfo2_host pCopyImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);

    convert_VkCopyImageInfo2_win32_to_host(params->pCopyImageInfo, &pCopyImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyImageInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyImage2KHR(void *args)
{
    struct vkCmdCopyImage2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyImage2KHR(void *args)
{
    struct vkCmdCopyImage2KHR_params *params = args;
    VkCopyImageInfo2_host pCopyImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageInfo);

    convert_VkCopyImageInfo2_win32_to_host(params->pCopyImageInfo, &pCopyImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyImageInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyImageToBuffer(void *args)
{
    struct vkCmdCopyImageToBuffer_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImageToBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstBuffer, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyImageToBuffer(void *args)
{
    struct vkCmdCopyImageToBuffer_params *params = args;
    VkBufferImageCopy_host *pRegions_host;
    struct conversion_context ctx;
    TRACE("%p, 0x%s, %#x, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstBuffer), params->regionCount, params->pRegions);

    init_conversion_context(&ctx);
    pRegions_host = convert_VkBufferImageCopy_array_win32_to_host(&ctx, params->pRegions, params->regionCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImageToBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstBuffer, params->regionCount, pRegions_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyImageToBuffer2(void *args)
{
    struct vkCmdCopyImageToBuffer2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImageToBuffer2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyImageToBufferInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyImageToBuffer2(void *args)
{
    struct vkCmdCopyImageToBuffer2_params *params = args;
    VkCopyImageToBufferInfo2_host pCopyImageToBufferInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);

    init_conversion_context(&ctx);
    convert_VkCopyImageToBufferInfo2_win32_to_host(&ctx, params->pCopyImageToBufferInfo, &pCopyImageToBufferInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImageToBuffer2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyImageToBufferInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyImageToBuffer2KHR(void *args)
{
    struct vkCmdCopyImageToBuffer2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImageToBuffer2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCopyImageToBufferInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyImageToBuffer2KHR(void *args)
{
    struct vkCmdCopyImageToBuffer2KHR_params *params = args;
    VkCopyImageToBufferInfo2_host pCopyImageToBufferInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pCopyImageToBufferInfo);

    init_conversion_context(&ctx);
    convert_VkCopyImageToBufferInfo2_win32_to_host(&ctx, params->pCopyImageToBufferInfo, &pCopyImageToBufferInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyImageToBuffer2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pCopyImageToBufferInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyMemoryToAccelerationStructureKHR(void *args)
{
    struct vkCmdCopyMemoryToAccelerationStructureKHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMemoryToAccelerationStructureKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyMemoryToAccelerationStructureKHR(void *args)
{
    struct vkCmdCopyMemoryToAccelerationStructureKHR_params *params = args;
    VkCopyMemoryToAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyMemoryToAccelerationStructureInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMemoryToAccelerationStructureKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyMemoryToMicromapEXT(void *args)
{
    struct vkCmdCopyMemoryToMicromapEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMemoryToMicromapEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyMemoryToMicromapEXT(void *args)
{
    struct vkCmdCopyMemoryToMicromapEXT_params *params = args;
    VkCopyMemoryToMicromapInfoEXT_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyMemoryToMicromapInfoEXT_win32_to_host(params->pInfo, &pInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMemoryToMicromapEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyMicromapEXT(void *args)
{
    struct vkCmdCopyMicromapEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMicromapEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyMicromapEXT(void *args)
{
    struct vkCmdCopyMicromapEXT_params *params = args;
    VkCopyMicromapInfoEXT_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyMicromapInfoEXT_win32_to_host(params->pInfo, &pInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMicromapEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyMicromapToMemoryEXT(void *args)
{
    struct vkCmdCopyMicromapToMemoryEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMicromapToMemoryEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyMicromapToMemoryEXT(void *args)
{
    struct vkCmdCopyMicromapToMemoryEXT_params *params = args;
    VkCopyMicromapToMemoryInfoEXT_host pInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pInfo);

    convert_VkCopyMicromapToMemoryInfoEXT_win32_to_host(params->pInfo, &pInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyMicromapToMemoryEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCopyQueryPoolResults(void *args)
{
    struct vkCmdCopyQueryPoolResults_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->stride), params->flags);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyQueryPoolResults(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->firstQuery, params->queryCount, params->dstBuffer, params->dstOffset, params->stride, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCopyQueryPoolResults(void *args)
{
    struct vkCmdCopyQueryPoolResults_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, 0x%s, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->stride), params->flags);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCopyQueryPoolResults(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->firstQuery, params->queryCount, params->dstBuffer, params->dstOffset, params->stride, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdCuLaunchKernelNVX(void *args)
{
    struct vkCmdCuLaunchKernelNVX_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pLaunchInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCuLaunchKernelNVX(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pLaunchInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdCuLaunchKernelNVX(void *args)
{
    struct vkCmdCuLaunchKernelNVX_params *params = args;
    VkCuLaunchInfoNVX_host pLaunchInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pLaunchInfo);

    convert_VkCuLaunchInfoNVX_win32_to_host(params->pLaunchInfo, &pLaunchInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdCuLaunchKernelNVX(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pLaunchInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDebugMarkerBeginEXT(void *args)
{
    struct vkCmdDebugMarkerBeginEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDebugMarkerBeginEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDebugMarkerBeginEXT(void *args)
{
    struct vkCmdDebugMarkerBeginEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDebugMarkerBeginEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDebugMarkerEndEXT(void *args)
{
    struct vkCmdDebugMarkerEndEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDebugMarkerEndEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDebugMarkerEndEXT(void *args)
{
    struct vkCmdDebugMarkerEndEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDebugMarkerEndEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDebugMarkerInsertEXT(void *args)
{
    struct vkCmdDebugMarkerInsertEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDebugMarkerInsertEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDebugMarkerInsertEXT(void *args)
{
    struct vkCmdDebugMarkerInsertEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDebugMarkerInsertEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDispatch(void *args)
{
    struct vkCmdDispatch_params *params = args;
    TRACE("%p, %u, %u, %u\n", params->commandBuffer, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatch(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDispatch(void *args)
{
    struct vkCmdDispatch_params *params = args;
    TRACE("%p, %u, %u, %u\n", params->commandBuffer, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatch(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDispatchBase(void *args)
{
    struct vkCmdDispatchBase_params *params = args;
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", params->commandBuffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatchBase(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDispatchBase(void *args)
{
    struct vkCmdDispatchBase_params *params = args;
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", params->commandBuffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatchBase(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDispatchBaseKHR(void *args)
{
    struct vkCmdDispatchBaseKHR_params *params = args;
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", params->commandBuffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatchBaseKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDispatchBaseKHR(void *args)
{
    struct vkCmdDispatchBaseKHR_params *params = args;
    TRACE("%p, %u, %u, %u, %u, %u, %u\n", params->commandBuffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatchBaseKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->baseGroupX, params->baseGroupY, params->baseGroupZ, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDispatchIndirect(void *args)
{
    struct vkCmdDispatchIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatchIndirect(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDispatchIndirect(void *args)
{
    struct vkCmdDispatchIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDispatchIndirect(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDraw(void *args)
{
    struct vkCmdDraw_params *params = args;
    TRACE("%p, %u, %u, %u, %u\n", params->commandBuffer, params->vertexCount, params->instanceCount, params->firstVertex, params->firstInstance);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDraw(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->vertexCount, params->instanceCount, params->firstVertex, params->firstInstance);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDraw(void *args)
{
    struct vkCmdDraw_params *params = args;
    TRACE("%p, %u, %u, %u, %u\n", params->commandBuffer, params->vertexCount, params->instanceCount, params->firstVertex, params->firstInstance);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDraw(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->vertexCount, params->instanceCount, params->firstVertex, params->firstInstance);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndexed(void *args)
{
    struct vkCmdDrawIndexed_params *params = args;
    TRACE("%p, %u, %u, %u, %d, %u\n", params->commandBuffer, params->indexCount, params->instanceCount, params->firstIndex, params->vertexOffset, params->firstInstance);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexed(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->indexCount, params->instanceCount, params->firstIndex, params->vertexOffset, params->firstInstance);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndexed(void *args)
{
    struct vkCmdDrawIndexed_params *params = args;
    TRACE("%p, %u, %u, %u, %d, %u\n", params->commandBuffer, params->indexCount, params->instanceCount, params->firstIndex, params->vertexOffset, params->firstInstance);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexed(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->indexCount, params->instanceCount, params->firstIndex, params->vertexOffset, params->firstInstance);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndexedIndirect(void *args)
{
    struct vkCmdDrawIndexedIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirect(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndexedIndirect(void *args)
{
    struct vkCmdDrawIndexedIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirect(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndexedIndirectCount(void *args)
{
    struct vkCmdDrawIndexedIndirectCount_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirectCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndexedIndirectCount(void *args)
{
    struct vkCmdDrawIndexedIndirectCount_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirectCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndexedIndirectCountAMD(void *args)
{
    struct vkCmdDrawIndexedIndirectCountAMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirectCountAMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndexedIndirectCountAMD(void *args)
{
    struct vkCmdDrawIndexedIndirectCountAMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirectCountAMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndexedIndirectCountKHR(void *args)
{
    struct vkCmdDrawIndexedIndirectCountKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirectCountKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndexedIndirectCountKHR(void *args)
{
    struct vkCmdDrawIndexedIndirectCountKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndexedIndirectCountKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndirect(void *args)
{
    struct vkCmdDrawIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirect(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndirect(void *args)
{
    struct vkCmdDrawIndirect_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirect(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndirectByteCountEXT(void *args)
{
    struct vkCmdDrawIndirectByteCountEXT_params *params = args;
    TRACE("%p, %u, %u, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, params->instanceCount, params->firstInstance, wine_dbgstr_longlong(params->counterBuffer), wine_dbgstr_longlong(params->counterBufferOffset), params->counterOffset, params->vertexStride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectByteCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->instanceCount, params->firstInstance, params->counterBuffer, params->counterBufferOffset, params->counterOffset, params->vertexStride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndirectByteCountEXT(void *args)
{
    struct vkCmdDrawIndirectByteCountEXT_params *params = args;
    TRACE("%p, %u, %u, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, params->instanceCount, params->firstInstance, wine_dbgstr_longlong(params->counterBuffer), wine_dbgstr_longlong(params->counterBufferOffset), params->counterOffset, params->vertexStride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectByteCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->instanceCount, params->firstInstance, params->counterBuffer, params->counterBufferOffset, params->counterOffset, params->vertexStride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndirectCount(void *args)
{
    struct vkCmdDrawIndirectCount_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndirectCount(void *args)
{
    struct vkCmdDrawIndirectCount_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndirectCountAMD(void *args)
{
    struct vkCmdDrawIndirectCountAMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectCountAMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndirectCountAMD(void *args)
{
    struct vkCmdDrawIndirectCountAMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectCountAMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawIndirectCountKHR(void *args)
{
    struct vkCmdDrawIndirectCountKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectCountKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawIndirectCountKHR(void *args)
{
    struct vkCmdDrawIndirectCountKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawIndirectCountKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMeshTasksEXT(void *args)
{
    struct vkCmdDrawMeshTasksEXT_params *params = args;
    TRACE("%p, %u, %u, %u\n", params->commandBuffer, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMeshTasksEXT(void *args)
{
    struct vkCmdDrawMeshTasksEXT_params *params = args;
    TRACE("%p, %u, %u, %u\n", params->commandBuffer, params->groupCountX, params->groupCountY, params->groupCountZ);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->groupCountX, params->groupCountY, params->groupCountZ);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMeshTasksIndirectCountEXT(void *args)
{
    struct vkCmdDrawMeshTasksIndirectCountEXT_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMeshTasksIndirectCountEXT(void *args)
{
    struct vkCmdDrawMeshTasksIndirectCountEXT_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMeshTasksIndirectCountNV(void *args)
{
    struct vkCmdDrawMeshTasksIndirectCountNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectCountNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMeshTasksIndirectCountNV(void *args)
{
    struct vkCmdDrawMeshTasksIndirectCountNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->countBuffer), wine_dbgstr_longlong(params->countBufferOffset), params->maxDrawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectCountNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->countBuffer, params->countBufferOffset, params->maxDrawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMeshTasksIndirectEXT(void *args)
{
    struct vkCmdDrawMeshTasksIndirectEXT_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMeshTasksIndirectEXT(void *args)
{
    struct vkCmdDrawMeshTasksIndirectEXT_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMeshTasksIndirectNV(void *args)
{
    struct vkCmdDrawMeshTasksIndirectNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMeshTasksIndirectNV(void *args)
{
    struct vkCmdDrawMeshTasksIndirectNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->buffer), wine_dbgstr_longlong(params->offset), params->drawCount, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksIndirectNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->buffer, params->offset, params->drawCount, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMeshTasksNV(void *args)
{
    struct vkCmdDrawMeshTasksNV_params *params = args;
    TRACE("%p, %u, %u\n", params->commandBuffer, params->taskCount, params->firstTask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->taskCount, params->firstTask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMeshTasksNV(void *args)
{
    struct vkCmdDrawMeshTasksNV_params *params = args;
    TRACE("%p, %u, %u\n", params->commandBuffer, params->taskCount, params->firstTask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMeshTasksNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->taskCount, params->firstTask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMultiEXT(void *args)
{
    struct vkCmdDrawMultiEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %u, %u\n", params->commandBuffer, params->drawCount, params->pVertexInfo, params->instanceCount, params->firstInstance, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMultiEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->drawCount, params->pVertexInfo, params->instanceCount, params->firstInstance, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMultiEXT(void *args)
{
    struct vkCmdDrawMultiEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %u, %u\n", params->commandBuffer, params->drawCount, params->pVertexInfo, params->instanceCount, params->firstInstance, params->stride);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMultiEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->drawCount, params->pVertexInfo, params->instanceCount, params->firstInstance, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdDrawMultiIndexedEXT(void *args)
{
    struct vkCmdDrawMultiIndexedEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %u, %u, %p\n", params->commandBuffer, params->drawCount, params->pIndexInfo, params->instanceCount, params->firstInstance, params->stride, params->pVertexOffset);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMultiIndexedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->drawCount, params->pIndexInfo, params->instanceCount, params->firstInstance, params->stride, params->pVertexOffset);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdDrawMultiIndexedEXT(void *args)
{
    struct vkCmdDrawMultiIndexedEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %u, %u, %p\n", params->commandBuffer, params->drawCount, params->pIndexInfo, params->instanceCount, params->firstInstance, params->stride, params->pVertexOffset);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdDrawMultiIndexedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->drawCount, params->pIndexInfo, params->instanceCount, params->firstInstance, params->stride, params->pVertexOffset);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndConditionalRenderingEXT(void *args)
{
    struct vkCmdEndConditionalRenderingEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndConditionalRenderingEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndConditionalRenderingEXT(void *args)
{
    struct vkCmdEndConditionalRenderingEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndConditionalRenderingEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndDebugUtilsLabelEXT(void *args)
{
    struct vkCmdEndDebugUtilsLabelEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndDebugUtilsLabelEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndDebugUtilsLabelEXT(void *args)
{
    struct vkCmdEndDebugUtilsLabelEXT_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndDebugUtilsLabelEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndQuery(void *args)
{
    struct vkCmdEndQuery_params *params = args;
    TRACE("%p, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndQuery(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndQuery(void *args)
{
    struct vkCmdEndQuery_params *params = args;
    TRACE("%p, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndQuery(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndQueryIndexedEXT(void *args)
{
    struct vkCmdEndQueryIndexedEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->index);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndQueryIndexedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query, params->index);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndQueryIndexedEXT(void *args)
{
    struct vkCmdEndQueryIndexedEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->query, params->index);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndQueryIndexedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->query, params->index);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndRenderPass(void *args)
{
    struct vkCmdEndRenderPass_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderPass(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndRenderPass(void *args)
{
    struct vkCmdEndRenderPass_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderPass(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndRenderPass2(void *args)
{
    struct vkCmdEndRenderPass2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderPass2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndRenderPass2(void *args)
{
    struct vkCmdEndRenderPass2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderPass2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndRenderPass2KHR(void *args)
{
    struct vkCmdEndRenderPass2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderPass2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndRenderPass2KHR(void *args)
{
    struct vkCmdEndRenderPass2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderPass2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndRendering(void *args)
{
    struct vkCmdEndRendering_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRendering(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndRendering(void *args)
{
    struct vkCmdEndRendering_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRendering(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndRenderingKHR(void *args)
{
    struct vkCmdEndRenderingKHR_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderingKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndRenderingKHR(void *args)
{
    struct vkCmdEndRenderingKHR_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndRenderingKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdEndTransformFeedbackEXT(void *args)
{
    struct vkCmdEndTransformFeedbackEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndTransformFeedbackEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdEndTransformFeedbackEXT(void *args)
{
    struct vkCmdEndTransformFeedbackEXT_params *params = args;
    TRACE("%p, %u, %u, %p, %p\n", params->commandBuffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdEndTransformFeedbackEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstCounterBuffer, params->counterBufferCount, params->pCounterBuffers, params->pCounterBufferOffsets);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdExecuteCommands(void *args)
{
    struct vkCmdExecuteCommands_params *params = args;
    VkCommandBuffer *pCommandBuffers_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->commandBufferCount, params->pCommandBuffers);

    init_conversion_context(&ctx);
    pCommandBuffers_host = convert_VkCommandBuffer_array_win64_to_host(&ctx, params->pCommandBuffers, params->commandBufferCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdExecuteCommands(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->commandBufferCount, pCommandBuffers_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdExecuteCommands(void *args)
{
    struct vkCmdExecuteCommands_params *params = args;
    VkCommandBuffer *pCommandBuffers_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->commandBufferCount, params->pCommandBuffers);

    init_conversion_context(&ctx);
    pCommandBuffers_host = convert_VkCommandBuffer_array_win32_to_host(&ctx, params->pCommandBuffers, params->commandBufferCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdExecuteCommands(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->commandBufferCount, pCommandBuffers_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdExecuteGeneratedCommandsNV(void *args)
{
    struct vkCmdExecuteGeneratedCommandsNV_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->isPreprocessed, params->pGeneratedCommandsInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdExecuteGeneratedCommandsNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->isPreprocessed, params->pGeneratedCommandsInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdExecuteGeneratedCommandsNV(void *args)
{
    struct vkCmdExecuteGeneratedCommandsNV_params *params = args;
    VkGeneratedCommandsInfoNV_host pGeneratedCommandsInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->isPreprocessed, params->pGeneratedCommandsInfo);

    init_conversion_context(&ctx);
    convert_VkGeneratedCommandsInfoNV_win32_to_host(&ctx, params->pGeneratedCommandsInfo, &pGeneratedCommandsInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdExecuteGeneratedCommandsNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->isPreprocessed, &pGeneratedCommandsInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdFillBuffer(void *args)
{
    struct vkCmdFillBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->size), params->data);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdFillBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->dstBuffer, params->dstOffset, params->size, params->data);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdFillBuffer(void *args)
{
    struct vkCmdFillBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->size), params->data);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdFillBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->dstBuffer, params->dstOffset, params->size, params->data);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdInsertDebugUtilsLabelEXT(void *args)
{
    struct vkCmdInsertDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pLabelInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdInsertDebugUtilsLabelEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdInsertDebugUtilsLabelEXT(void *args)
{
    struct vkCmdInsertDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pLabelInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdInsertDebugUtilsLabelEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdNextSubpass(void *args)
{
    struct vkCmdNextSubpass_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->contents);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdNextSubpass(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->contents);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdNextSubpass(void *args)
{
    struct vkCmdNextSubpass_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->contents);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdNextSubpass(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->contents);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdNextSubpass2(void *args)
{
    struct vkCmdNextSubpass2_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdNextSubpass2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdNextSubpass2(void *args)
{
    struct vkCmdNextSubpass2_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdNextSubpass2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdNextSubpass2KHR(void *args)
{
    struct vkCmdNextSubpass2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdNextSubpass2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdNextSubpass2KHR(void *args)
{
    struct vkCmdNextSubpass2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdNextSubpass2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSubpassBeginInfo, params->pSubpassEndInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdOpticalFlowExecuteNV(void *args)
{
    struct vkCmdOpticalFlowExecuteNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->session), params->pExecuteInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdOpticalFlowExecuteNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->session, params->pExecuteInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdOpticalFlowExecuteNV(void *args)
{
    struct vkCmdOpticalFlowExecuteNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->session), params->pExecuteInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdOpticalFlowExecuteNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->session, params->pExecuteInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdPipelineBarrier(void *args)
{
    struct vkCmdPipelineBarrier_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPipelineBarrier(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdPipelineBarrier(void *args)
{
    struct vkCmdPipelineBarrier_params *params = args;
    VkBufferMemoryBarrier_host *pBufferMemoryBarriers_host;
    VkImageMemoryBarrier_host *pImageMemoryBarriers_host;
    struct conversion_context ctx;
    TRACE("%p, %#x, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);

    init_conversion_context(&ctx);
    pBufferMemoryBarriers_host = convert_VkBufferMemoryBarrier_array_win32_to_host(&ctx, params->pBufferMemoryBarriers, params->bufferMemoryBarrierCount);
    pImageMemoryBarriers_host = convert_VkImageMemoryBarrier_array_win32_to_host(&ctx, params->pImageMemoryBarriers, params->imageMemoryBarrierCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPipelineBarrier(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcStageMask, params->dstStageMask, params->dependencyFlags, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, pBufferMemoryBarriers_host, params->imageMemoryBarrierCount, pImageMemoryBarriers_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdPipelineBarrier2(void *args)
{
    struct vkCmdPipelineBarrier2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPipelineBarrier2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pDependencyInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdPipelineBarrier2(void *args)
{
    struct vkCmdPipelineBarrier2_params *params = args;
    VkDependencyInfo_host pDependencyInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);

    init_conversion_context(&ctx);
    convert_VkDependencyInfo_win32_to_host(&ctx, params->pDependencyInfo, &pDependencyInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPipelineBarrier2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pDependencyInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdPipelineBarrier2KHR(void *args)
{
    struct vkCmdPipelineBarrier2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPipelineBarrier2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pDependencyInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdPipelineBarrier2KHR(void *args)
{
    struct vkCmdPipelineBarrier2KHR_params *params = args;
    VkDependencyInfo_host pDependencyInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pDependencyInfo);

    init_conversion_context(&ctx);
    convert_VkDependencyInfo_win32_to_host(&ctx, params->pDependencyInfo, &pDependencyInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPipelineBarrier2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pDependencyInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdPreprocessGeneratedCommandsNV(void *args)
{
    struct vkCmdPreprocessGeneratedCommandsNV_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pGeneratedCommandsInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPreprocessGeneratedCommandsNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pGeneratedCommandsInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdPreprocessGeneratedCommandsNV(void *args)
{
    struct vkCmdPreprocessGeneratedCommandsNV_params *params = args;
    VkGeneratedCommandsInfoNV_host pGeneratedCommandsInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p\n", params->commandBuffer, params->pGeneratedCommandsInfo);

    init_conversion_context(&ctx);
    convert_VkGeneratedCommandsInfoNV_win32_to_host(&ctx, params->pGeneratedCommandsInfo, &pGeneratedCommandsInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPreprocessGeneratedCommandsNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pGeneratedCommandsInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdPushConstants(void *args)
{
    struct vkCmdPushConstants_params *params = args;
    TRACE("%p, 0x%s, %#x, %u, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->layout), params->stageFlags, params->offset, params->size, params->pValues);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPushConstants(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->layout, params->stageFlags, params->offset, params->size, params->pValues);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdPushConstants(void *args)
{
    struct vkCmdPushConstants_params *params = args;
    TRACE("%p, 0x%s, %#x, %u, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->layout), params->stageFlags, params->offset, params->size, params->pValues);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPushConstants(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->layout, params->stageFlags, params->offset, params->size, params->pValues);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdPushDescriptorSetKHR(void *args)
{
    struct vkCmdPushDescriptorSetKHR_params *params = args;
    TRACE("%p, %#x, 0x%s, %u, %u, %p\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->layout), params->set, params->descriptorWriteCount, params->pDescriptorWrites);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPushDescriptorSetKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->layout, params->set, params->descriptorWriteCount, params->pDescriptorWrites);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdPushDescriptorSetKHR(void *args)
{
    struct vkCmdPushDescriptorSetKHR_params *params = args;
    VkWriteDescriptorSet_host *pDescriptorWrites_host;
    struct conversion_context ctx;
    TRACE("%p, %#x, 0x%s, %u, %u, %p\n", params->commandBuffer, params->pipelineBindPoint, wine_dbgstr_longlong(params->layout), params->set, params->descriptorWriteCount, params->pDescriptorWrites);

    init_conversion_context(&ctx);
    pDescriptorWrites_host = convert_VkWriteDescriptorSet_array_win32_to_host(&ctx, params->pDescriptorWrites, params->descriptorWriteCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPushDescriptorSetKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineBindPoint, params->layout, params->set, params->descriptorWriteCount, pDescriptorWrites_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdPushDescriptorSetWithTemplateKHR(void *args)
{
    struct vkCmdPushDescriptorSetWithTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->descriptorUpdateTemplate), wine_dbgstr_longlong(params->layout), params->set, params->pData);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPushDescriptorSetWithTemplateKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->descriptorUpdateTemplate, params->layout, params->set, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdPushDescriptorSetWithTemplateKHR(void *args)
{
    struct vkCmdPushDescriptorSetWithTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->descriptorUpdateTemplate), wine_dbgstr_longlong(params->layout), params->set, params->pData);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdPushDescriptorSetWithTemplateKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->descriptorUpdateTemplate, params->layout, params->set, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdResetEvent(void *args)
{
    struct vkCmdResetEvent_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->stageMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetEvent(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdResetEvent(void *args)
{
    struct vkCmdResetEvent_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->stageMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetEvent(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdResetEvent2(void *args)
{
    struct vkCmdResetEvent2_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->event), wine_dbgstr_longlong(params->stageMask));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetEvent2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdResetEvent2(void *args)
{
    struct vkCmdResetEvent2_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->event), wine_dbgstr_longlong(params->stageMask));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetEvent2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdResetEvent2KHR(void *args)
{
    struct vkCmdResetEvent2KHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->event), wine_dbgstr_longlong(params->stageMask));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetEvent2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdResetEvent2KHR(void *args)
{
    struct vkCmdResetEvent2KHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->event), wine_dbgstr_longlong(params->stageMask));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetEvent2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdResetQueryPool(void *args)
{
    struct vkCmdResetQueryPool_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetQueryPool(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdResetQueryPool(void *args)
{
    struct vkCmdResetQueryPool_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResetQueryPool(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdResolveImage(void *args)
{
    struct vkCmdResolveImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResolveImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdResolveImage(void *args)
{
    struct vkCmdResolveImage_params *params = args;
    TRACE("%p, 0x%s, %#x, 0x%s, %#x, %u, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->srcImage), params->srcImageLayout, wine_dbgstr_longlong(params->dstImage), params->dstImageLayout, params->regionCount, params->pRegions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResolveImage(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->srcImage, params->srcImageLayout, params->dstImage, params->dstImageLayout, params->regionCount, params->pRegions);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdResolveImage2(void *args)
{
    struct vkCmdResolveImage2_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResolveImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pResolveImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdResolveImage2(void *args)
{
    struct vkCmdResolveImage2_params *params = args;
    VkResolveImageInfo2_host pResolveImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);

    convert_VkResolveImageInfo2_win32_to_host(params->pResolveImageInfo, &pResolveImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResolveImage2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pResolveImageInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdResolveImage2KHR(void *args)
{
    struct vkCmdResolveImage2KHR_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResolveImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pResolveImageInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdResolveImage2KHR(void *args)
{
    struct vkCmdResolveImage2KHR_params *params = args;
    VkResolveImageInfo2_host pResolveImageInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pResolveImageInfo);

    convert_VkResolveImageInfo2_win32_to_host(params->pResolveImageInfo, &pResolveImageInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdResolveImage2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pResolveImageInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetAlphaToCoverageEnableEXT(void *args)
{
    struct vkCmdSetAlphaToCoverageEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->alphaToCoverageEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetAlphaToCoverageEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->alphaToCoverageEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetAlphaToCoverageEnableEXT(void *args)
{
    struct vkCmdSetAlphaToCoverageEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->alphaToCoverageEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetAlphaToCoverageEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->alphaToCoverageEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetAlphaToOneEnableEXT(void *args)
{
    struct vkCmdSetAlphaToOneEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->alphaToOneEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetAlphaToOneEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->alphaToOneEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetAlphaToOneEnableEXT(void *args)
{
    struct vkCmdSetAlphaToOneEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->alphaToOneEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetAlphaToOneEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->alphaToOneEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetBlendConstants(void *args)
{
    struct vkCmdSetBlendConstants_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->blendConstants);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetBlendConstants(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->blendConstants);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetBlendConstants(void *args)
{
    struct vkCmdSetBlendConstants_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->blendConstants);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetBlendConstants(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->blendConstants);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCheckpointNV(void *args)
{
    struct vkCmdSetCheckpointNV_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCheckpointMarker);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCheckpointNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCheckpointMarker);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCheckpointNV(void *args)
{
    struct vkCmdSetCheckpointNV_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pCheckpointMarker);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCheckpointNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pCheckpointMarker);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCoarseSampleOrderNV(void *args)
{
    struct vkCmdSetCoarseSampleOrderNV_params *params = args;
    TRACE("%p, %#x, %u, %p\n", params->commandBuffer, params->sampleOrderType, params->customSampleOrderCount, params->pCustomSampleOrders);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoarseSampleOrderNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->sampleOrderType, params->customSampleOrderCount, params->pCustomSampleOrders);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCoarseSampleOrderNV(void *args)
{
    struct vkCmdSetCoarseSampleOrderNV_params *params = args;
    TRACE("%p, %#x, %u, %p\n", params->commandBuffer, params->sampleOrderType, params->customSampleOrderCount, params->pCustomSampleOrders);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoarseSampleOrderNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->sampleOrderType, params->customSampleOrderCount, params->pCustomSampleOrders);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetColorBlendAdvancedEXT(void *args)
{
    struct vkCmdSetColorBlendAdvancedEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorBlendAdvanced);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorBlendAdvancedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorBlendAdvanced);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetColorBlendAdvancedEXT(void *args)
{
    struct vkCmdSetColorBlendAdvancedEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorBlendAdvanced);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorBlendAdvancedEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorBlendAdvanced);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetColorBlendEnableEXT(void *args)
{
    struct vkCmdSetColorBlendEnableEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEnables);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorBlendEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEnables);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetColorBlendEnableEXT(void *args)
{
    struct vkCmdSetColorBlendEnableEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEnables);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorBlendEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEnables);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetColorBlendEquationEXT(void *args)
{
    struct vkCmdSetColorBlendEquationEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEquations);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorBlendEquationEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEquations);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetColorBlendEquationEXT(void *args)
{
    struct vkCmdSetColorBlendEquationEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEquations);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorBlendEquationEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorBlendEquations);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetColorWriteEnableEXT(void *args)
{
    struct vkCmdSetColorWriteEnableEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->attachmentCount, params->pColorWriteEnables);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorWriteEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->attachmentCount, params->pColorWriteEnables);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetColorWriteEnableEXT(void *args)
{
    struct vkCmdSetColorWriteEnableEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->attachmentCount, params->pColorWriteEnables);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorWriteEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->attachmentCount, params->pColorWriteEnables);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetColorWriteMaskEXT(void *args)
{
    struct vkCmdSetColorWriteMaskEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorWriteMasks);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorWriteMaskEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorWriteMasks);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetColorWriteMaskEXT(void *args)
{
    struct vkCmdSetColorWriteMaskEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstAttachment, params->attachmentCount, params->pColorWriteMasks);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetColorWriteMaskEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstAttachment, params->attachmentCount, params->pColorWriteMasks);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetConservativeRasterizationModeEXT(void *args)
{
    struct vkCmdSetConservativeRasterizationModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->conservativeRasterizationMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetConservativeRasterizationModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->conservativeRasterizationMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetConservativeRasterizationModeEXT(void *args)
{
    struct vkCmdSetConservativeRasterizationModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->conservativeRasterizationMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetConservativeRasterizationModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->conservativeRasterizationMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCoverageModulationModeNV(void *args)
{
    struct vkCmdSetCoverageModulationModeNV_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->coverageModulationMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageModulationModeNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageModulationMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCoverageModulationModeNV(void *args)
{
    struct vkCmdSetCoverageModulationModeNV_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->coverageModulationMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageModulationModeNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageModulationMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCoverageModulationTableEnableNV(void *args)
{
    struct vkCmdSetCoverageModulationTableEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->coverageModulationTableEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageModulationTableEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageModulationTableEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCoverageModulationTableEnableNV(void *args)
{
    struct vkCmdSetCoverageModulationTableEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->coverageModulationTableEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageModulationTableEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageModulationTableEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCoverageModulationTableNV(void *args)
{
    struct vkCmdSetCoverageModulationTableNV_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->coverageModulationTableCount, params->pCoverageModulationTable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageModulationTableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageModulationTableCount, params->pCoverageModulationTable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCoverageModulationTableNV(void *args)
{
    struct vkCmdSetCoverageModulationTableNV_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->coverageModulationTableCount, params->pCoverageModulationTable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageModulationTableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageModulationTableCount, params->pCoverageModulationTable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCoverageReductionModeNV(void *args)
{
    struct vkCmdSetCoverageReductionModeNV_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->coverageReductionMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageReductionModeNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageReductionMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCoverageReductionModeNV(void *args)
{
    struct vkCmdSetCoverageReductionModeNV_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->coverageReductionMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageReductionModeNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageReductionMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCoverageToColorEnableNV(void *args)
{
    struct vkCmdSetCoverageToColorEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->coverageToColorEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageToColorEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageToColorEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCoverageToColorEnableNV(void *args)
{
    struct vkCmdSetCoverageToColorEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->coverageToColorEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageToColorEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageToColorEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCoverageToColorLocationNV(void *args)
{
    struct vkCmdSetCoverageToColorLocationNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->coverageToColorLocation);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageToColorLocationNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageToColorLocation);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCoverageToColorLocationNV(void *args)
{
    struct vkCmdSetCoverageToColorLocationNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->coverageToColorLocation);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCoverageToColorLocationNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->coverageToColorLocation);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCullMode(void *args)
{
    struct vkCmdSetCullMode_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->cullMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCullMode(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->cullMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCullMode(void *args)
{
    struct vkCmdSetCullMode_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->cullMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCullMode(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->cullMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetCullModeEXT(void *args)
{
    struct vkCmdSetCullModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->cullMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCullModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->cullMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetCullModeEXT(void *args)
{
    struct vkCmdSetCullModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->cullMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetCullModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->cullMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthBias(void *args)
{
    struct vkCmdSetDepthBias_params *params = args;
    TRACE("%p, %f, %f, %f\n", params->commandBuffer, params->depthBiasConstantFactor, params->depthBiasClamp, params->depthBiasSlopeFactor);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBias(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBiasConstantFactor, params->depthBiasClamp, params->depthBiasSlopeFactor);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthBias(void *args)
{
    struct vkCmdSetDepthBias_params *params = args;
    TRACE("%p, %f, %f, %f\n", params->commandBuffer, params->depthBiasConstantFactor, params->depthBiasClamp, params->depthBiasSlopeFactor);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBias(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBiasConstantFactor, params->depthBiasClamp, params->depthBiasSlopeFactor);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthBiasEnable(void *args)
{
    struct vkCmdSetDepthBiasEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBiasEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBiasEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBiasEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthBiasEnable(void *args)
{
    struct vkCmdSetDepthBiasEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBiasEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBiasEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBiasEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthBiasEnableEXT(void *args)
{
    struct vkCmdSetDepthBiasEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBiasEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBiasEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBiasEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthBiasEnableEXT(void *args)
{
    struct vkCmdSetDepthBiasEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBiasEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBiasEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBiasEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthBounds(void *args)
{
    struct vkCmdSetDepthBounds_params *params = args;
    TRACE("%p, %f, %f\n", params->commandBuffer, params->minDepthBounds, params->maxDepthBounds);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBounds(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->minDepthBounds, params->maxDepthBounds);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthBounds(void *args)
{
    struct vkCmdSetDepthBounds_params *params = args;
    TRACE("%p, %f, %f\n", params->commandBuffer, params->minDepthBounds, params->maxDepthBounds);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBounds(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->minDepthBounds, params->maxDepthBounds);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthBoundsTestEnable(void *args)
{
    struct vkCmdSetDepthBoundsTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBoundsTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBoundsTestEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBoundsTestEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthBoundsTestEnable(void *args)
{
    struct vkCmdSetDepthBoundsTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBoundsTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBoundsTestEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBoundsTestEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthBoundsTestEnableEXT(void *args)
{
    struct vkCmdSetDepthBoundsTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBoundsTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBoundsTestEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBoundsTestEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthBoundsTestEnableEXT(void *args)
{
    struct vkCmdSetDepthBoundsTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthBoundsTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthBoundsTestEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthBoundsTestEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthClampEnableEXT(void *args)
{
    struct vkCmdSetDepthClampEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthClampEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthClampEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthClampEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthClampEnableEXT(void *args)
{
    struct vkCmdSetDepthClampEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthClampEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthClampEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthClampEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthClipEnableEXT(void *args)
{
    struct vkCmdSetDepthClipEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthClipEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthClipEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthClipEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthClipEnableEXT(void *args)
{
    struct vkCmdSetDepthClipEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthClipEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthClipEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthClipEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthClipNegativeOneToOneEXT(void *args)
{
    struct vkCmdSetDepthClipNegativeOneToOneEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->negativeOneToOne);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthClipNegativeOneToOneEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->negativeOneToOne);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthClipNegativeOneToOneEXT(void *args)
{
    struct vkCmdSetDepthClipNegativeOneToOneEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->negativeOneToOne);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthClipNegativeOneToOneEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->negativeOneToOne);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthCompareOp(void *args)
{
    struct vkCmdSetDepthCompareOp_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->depthCompareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthCompareOp(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthCompareOp);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthCompareOp(void *args)
{
    struct vkCmdSetDepthCompareOp_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->depthCompareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthCompareOp(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthCompareOp);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthCompareOpEXT(void *args)
{
    struct vkCmdSetDepthCompareOpEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->depthCompareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthCompareOpEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthCompareOp);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthCompareOpEXT(void *args)
{
    struct vkCmdSetDepthCompareOpEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->depthCompareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthCompareOpEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthCompareOp);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthTestEnable(void *args)
{
    struct vkCmdSetDepthTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthTestEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthTestEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthTestEnable(void *args)
{
    struct vkCmdSetDepthTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthTestEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthTestEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthTestEnableEXT(void *args)
{
    struct vkCmdSetDepthTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthTestEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthTestEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthTestEnableEXT(void *args)
{
    struct vkCmdSetDepthTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthTestEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthTestEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthWriteEnable(void *args)
{
    struct vkCmdSetDepthWriteEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthWriteEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthWriteEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthWriteEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthWriteEnable(void *args)
{
    struct vkCmdSetDepthWriteEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthWriteEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthWriteEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthWriteEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDepthWriteEnableEXT(void *args)
{
    struct vkCmdSetDepthWriteEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthWriteEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthWriteEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthWriteEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDepthWriteEnableEXT(void *args)
{
    struct vkCmdSetDepthWriteEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->depthWriteEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDepthWriteEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->depthWriteEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDeviceMask(void *args)
{
    struct vkCmdSetDeviceMask_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->deviceMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDeviceMask(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->deviceMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDeviceMask(void *args)
{
    struct vkCmdSetDeviceMask_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->deviceMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDeviceMask(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->deviceMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDeviceMaskKHR(void *args)
{
    struct vkCmdSetDeviceMaskKHR_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->deviceMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDeviceMaskKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->deviceMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDeviceMaskKHR(void *args)
{
    struct vkCmdSetDeviceMaskKHR_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->deviceMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDeviceMaskKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->deviceMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetDiscardRectangleEXT(void *args)
{
    struct vkCmdSetDiscardRectangleEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstDiscardRectangle, params->discardRectangleCount, params->pDiscardRectangles);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDiscardRectangleEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstDiscardRectangle, params->discardRectangleCount, params->pDiscardRectangles);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetDiscardRectangleEXT(void *args)
{
    struct vkCmdSetDiscardRectangleEXT_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstDiscardRectangle, params->discardRectangleCount, params->pDiscardRectangles);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetDiscardRectangleEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstDiscardRectangle, params->discardRectangleCount, params->pDiscardRectangles);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetEvent(void *args)
{
    struct vkCmdSetEvent_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->stageMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetEvent(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetEvent(void *args)
{
    struct vkCmdSetEvent_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->stageMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetEvent(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->stageMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetEvent2(void *args)
{
    struct vkCmdSetEvent2_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetEvent2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->pDependencyInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetEvent2(void *args)
{
    struct vkCmdSetEvent2_params *params = args;
    VkDependencyInfo_host pDependencyInfo_host;
    struct conversion_context ctx;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);

    init_conversion_context(&ctx);
    convert_VkDependencyInfo_win32_to_host(&ctx, params->pDependencyInfo, &pDependencyInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetEvent2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, &pDependencyInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetEvent2KHR(void *args)
{
    struct vkCmdSetEvent2KHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetEvent2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, params->pDependencyInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetEvent2KHR(void *args)
{
    struct vkCmdSetEvent2KHR_params *params = args;
    VkDependencyInfo_host pDependencyInfo_host;
    struct conversion_context ctx;
    TRACE("%p, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->event), params->pDependencyInfo);

    init_conversion_context(&ctx);
    convert_VkDependencyInfo_win32_to_host(&ctx, params->pDependencyInfo, &pDependencyInfo_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetEvent2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->event, &pDependencyInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetExclusiveScissorNV(void *args)
{
    struct vkCmdSetExclusiveScissorNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstExclusiveScissor, params->exclusiveScissorCount, params->pExclusiveScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetExclusiveScissorNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstExclusiveScissor, params->exclusiveScissorCount, params->pExclusiveScissors);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetExclusiveScissorNV(void *args)
{
    struct vkCmdSetExclusiveScissorNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstExclusiveScissor, params->exclusiveScissorCount, params->pExclusiveScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetExclusiveScissorNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstExclusiveScissor, params->exclusiveScissorCount, params->pExclusiveScissors);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetExtraPrimitiveOverestimationSizeEXT(void *args)
{
    struct vkCmdSetExtraPrimitiveOverestimationSizeEXT_params *params = args;
    TRACE("%p, %f\n", params->commandBuffer, params->extraPrimitiveOverestimationSize);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetExtraPrimitiveOverestimationSizeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->extraPrimitiveOverestimationSize);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetExtraPrimitiveOverestimationSizeEXT(void *args)
{
    struct vkCmdSetExtraPrimitiveOverestimationSizeEXT_params *params = args;
    TRACE("%p, %f\n", params->commandBuffer, params->extraPrimitiveOverestimationSize);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetExtraPrimitiveOverestimationSizeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->extraPrimitiveOverestimationSize);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetFragmentShadingRateEnumNV(void *args)
{
    struct vkCmdSetFragmentShadingRateEnumNV_params *params = args;
    TRACE("%p, %#x, %p\n", params->commandBuffer, params->shadingRate, params->combinerOps);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFragmentShadingRateEnumNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->shadingRate, params->combinerOps);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetFragmentShadingRateEnumNV(void *args)
{
    struct vkCmdSetFragmentShadingRateEnumNV_params *params = args;
    TRACE("%p, %#x, %p\n", params->commandBuffer, params->shadingRate, params->combinerOps);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFragmentShadingRateEnumNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->shadingRate, params->combinerOps);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetFragmentShadingRateKHR(void *args)
{
    struct vkCmdSetFragmentShadingRateKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pFragmentSize, params->combinerOps);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFragmentShadingRateKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pFragmentSize, params->combinerOps);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetFragmentShadingRateKHR(void *args)
{
    struct vkCmdSetFragmentShadingRateKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->commandBuffer, params->pFragmentSize, params->combinerOps);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFragmentShadingRateKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pFragmentSize, params->combinerOps);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetFrontFace(void *args)
{
    struct vkCmdSetFrontFace_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->frontFace);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFrontFace(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->frontFace);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetFrontFace(void *args)
{
    struct vkCmdSetFrontFace_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->frontFace);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFrontFace(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->frontFace);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetFrontFaceEXT(void *args)
{
    struct vkCmdSetFrontFaceEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->frontFace);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFrontFaceEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->frontFace);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetFrontFaceEXT(void *args)
{
    struct vkCmdSetFrontFaceEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->frontFace);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetFrontFaceEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->frontFace);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetLineRasterizationModeEXT(void *args)
{
    struct vkCmdSetLineRasterizationModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->lineRasterizationMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineRasterizationModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->lineRasterizationMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetLineRasterizationModeEXT(void *args)
{
    struct vkCmdSetLineRasterizationModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->lineRasterizationMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineRasterizationModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->lineRasterizationMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetLineStippleEXT(void *args)
{
    struct vkCmdSetLineStippleEXT_params *params = args;
    TRACE("%p, %u, %u\n", params->commandBuffer, params->lineStippleFactor, params->lineStipplePattern);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineStippleEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->lineStippleFactor, params->lineStipplePattern);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetLineStippleEXT(void *args)
{
    struct vkCmdSetLineStippleEXT_params *params = args;
    TRACE("%p, %u, %u\n", params->commandBuffer, params->lineStippleFactor, params->lineStipplePattern);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineStippleEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->lineStippleFactor, params->lineStipplePattern);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetLineStippleEnableEXT(void *args)
{
    struct vkCmdSetLineStippleEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stippledLineEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineStippleEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stippledLineEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetLineStippleEnableEXT(void *args)
{
    struct vkCmdSetLineStippleEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stippledLineEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineStippleEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stippledLineEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetLineWidth(void *args)
{
    struct vkCmdSetLineWidth_params *params = args;
    TRACE("%p, %f\n", params->commandBuffer, params->lineWidth);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineWidth(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->lineWidth);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetLineWidth(void *args)
{
    struct vkCmdSetLineWidth_params *params = args;
    TRACE("%p, %f\n", params->commandBuffer, params->lineWidth);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLineWidth(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->lineWidth);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetLogicOpEXT(void *args)
{
    struct vkCmdSetLogicOpEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->logicOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLogicOpEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->logicOp);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetLogicOpEXT(void *args)
{
    struct vkCmdSetLogicOpEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->logicOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLogicOpEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->logicOp);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetLogicOpEnableEXT(void *args)
{
    struct vkCmdSetLogicOpEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->logicOpEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLogicOpEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->logicOpEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetLogicOpEnableEXT(void *args)
{
    struct vkCmdSetLogicOpEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->logicOpEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetLogicOpEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->logicOpEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPatchControlPointsEXT(void *args)
{
    struct vkCmdSetPatchControlPointsEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->patchControlPoints);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPatchControlPointsEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->patchControlPoints);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPatchControlPointsEXT(void *args)
{
    struct vkCmdSetPatchControlPointsEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->patchControlPoints);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPatchControlPointsEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->patchControlPoints);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPerformanceMarkerINTEL(void *args)
{
    struct vkCmdSetPerformanceMarkerINTEL_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPerformanceMarkerINTEL(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPerformanceMarkerINTEL(void *args)
{
    struct vkCmdSetPerformanceMarkerINTEL_params *params = args;
    VkPerformanceMarkerInfoINTEL_host pMarkerInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    convert_VkPerformanceMarkerInfoINTEL_win32_to_host(params->pMarkerInfo, &pMarkerInfo_host);
    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPerformanceMarkerINTEL(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pMarkerInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPerformanceOverrideINTEL(void *args)
{
    struct vkCmdSetPerformanceOverrideINTEL_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pOverrideInfo);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPerformanceOverrideINTEL(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pOverrideInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPerformanceOverrideINTEL(void *args)
{
    struct vkCmdSetPerformanceOverrideINTEL_params *params = args;
    VkPerformanceOverrideInfoINTEL_host pOverrideInfo_host;
    TRACE("%p, %p\n", params->commandBuffer, params->pOverrideInfo);

    convert_VkPerformanceOverrideInfoINTEL_win32_to_host(params->pOverrideInfo, &pOverrideInfo_host);
    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPerformanceOverrideINTEL(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pOverrideInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPerformanceStreamMarkerINTEL(void *args)
{
    struct vkCmdSetPerformanceStreamMarkerINTEL_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPerformanceStreamMarkerINTEL(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPerformanceStreamMarkerINTEL(void *args)
{
    struct vkCmdSetPerformanceStreamMarkerINTEL_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pMarkerInfo);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPerformanceStreamMarkerINTEL(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pMarkerInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPolygonModeEXT(void *args)
{
    struct vkCmdSetPolygonModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->polygonMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPolygonModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->polygonMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPolygonModeEXT(void *args)
{
    struct vkCmdSetPolygonModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->polygonMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPolygonModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->polygonMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPrimitiveRestartEnable(void *args)
{
    struct vkCmdSetPrimitiveRestartEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->primitiveRestartEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveRestartEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveRestartEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPrimitiveRestartEnable(void *args)
{
    struct vkCmdSetPrimitiveRestartEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->primitiveRestartEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveRestartEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveRestartEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPrimitiveRestartEnableEXT(void *args)
{
    struct vkCmdSetPrimitiveRestartEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->primitiveRestartEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveRestartEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveRestartEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPrimitiveRestartEnableEXT(void *args)
{
    struct vkCmdSetPrimitiveRestartEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->primitiveRestartEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveRestartEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveRestartEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPrimitiveTopology(void *args)
{
    struct vkCmdSetPrimitiveTopology_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->primitiveTopology);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveTopology(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveTopology);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPrimitiveTopology(void *args)
{
    struct vkCmdSetPrimitiveTopology_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->primitiveTopology);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveTopology(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveTopology);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetPrimitiveTopologyEXT(void *args)
{
    struct vkCmdSetPrimitiveTopologyEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->primitiveTopology);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveTopologyEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveTopology);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetPrimitiveTopologyEXT(void *args)
{
    struct vkCmdSetPrimitiveTopologyEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->primitiveTopology);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetPrimitiveTopologyEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->primitiveTopology);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetProvokingVertexModeEXT(void *args)
{
    struct vkCmdSetProvokingVertexModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->provokingVertexMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetProvokingVertexModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->provokingVertexMode);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetProvokingVertexModeEXT(void *args)
{
    struct vkCmdSetProvokingVertexModeEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->provokingVertexMode);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetProvokingVertexModeEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->provokingVertexMode);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetRasterizationSamplesEXT(void *args)
{
    struct vkCmdSetRasterizationSamplesEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->rasterizationSamples);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizationSamplesEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizationSamples);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetRasterizationSamplesEXT(void *args)
{
    struct vkCmdSetRasterizationSamplesEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->rasterizationSamples);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizationSamplesEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizationSamples);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetRasterizationStreamEXT(void *args)
{
    struct vkCmdSetRasterizationStreamEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizationStream);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizationStreamEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizationStream);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetRasterizationStreamEXT(void *args)
{
    struct vkCmdSetRasterizationStreamEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizationStream);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizationStreamEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizationStream);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetRasterizerDiscardEnable(void *args)
{
    struct vkCmdSetRasterizerDiscardEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizerDiscardEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizerDiscardEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizerDiscardEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetRasterizerDiscardEnable(void *args)
{
    struct vkCmdSetRasterizerDiscardEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizerDiscardEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizerDiscardEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizerDiscardEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetRasterizerDiscardEnableEXT(void *args)
{
    struct vkCmdSetRasterizerDiscardEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizerDiscardEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizerDiscardEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizerDiscardEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetRasterizerDiscardEnableEXT(void *args)
{
    struct vkCmdSetRasterizerDiscardEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->rasterizerDiscardEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRasterizerDiscardEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->rasterizerDiscardEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetRayTracingPipelineStackSizeKHR(void *args)
{
    struct vkCmdSetRayTracingPipelineStackSizeKHR_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->pipelineStackSize);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRayTracingPipelineStackSizeKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineStackSize);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetRayTracingPipelineStackSizeKHR(void *args)
{
    struct vkCmdSetRayTracingPipelineStackSizeKHR_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->pipelineStackSize);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRayTracingPipelineStackSizeKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineStackSize);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetRepresentativeFragmentTestEnableNV(void *args)
{
    struct vkCmdSetRepresentativeFragmentTestEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->representativeFragmentTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRepresentativeFragmentTestEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->representativeFragmentTestEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetRepresentativeFragmentTestEnableNV(void *args)
{
    struct vkCmdSetRepresentativeFragmentTestEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->representativeFragmentTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetRepresentativeFragmentTestEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->representativeFragmentTestEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetSampleLocationsEXT(void *args)
{
    struct vkCmdSetSampleLocationsEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSampleLocationsInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetSampleLocationsEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSampleLocationsInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetSampleLocationsEXT(void *args)
{
    struct vkCmdSetSampleLocationsEXT_params *params = args;
    TRACE("%p, %p\n", params->commandBuffer, params->pSampleLocationsInfo);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetSampleLocationsEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pSampleLocationsInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetSampleLocationsEnableEXT(void *args)
{
    struct vkCmdSetSampleLocationsEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->sampleLocationsEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetSampleLocationsEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->sampleLocationsEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetSampleLocationsEnableEXT(void *args)
{
    struct vkCmdSetSampleLocationsEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->sampleLocationsEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetSampleLocationsEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->sampleLocationsEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetSampleMaskEXT(void *args)
{
    struct vkCmdSetSampleMaskEXT_params *params = args;
    TRACE("%p, %#x, %p\n", params->commandBuffer, params->samples, params->pSampleMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetSampleMaskEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->samples, params->pSampleMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetSampleMaskEXT(void *args)
{
    struct vkCmdSetSampleMaskEXT_params *params = args;
    TRACE("%p, %#x, %p\n", params->commandBuffer, params->samples, params->pSampleMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetSampleMaskEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->samples, params->pSampleMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetScissor(void *args)
{
    struct vkCmdSetScissor_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstScissor, params->scissorCount, params->pScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetScissor(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstScissor, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetScissor(void *args)
{
    struct vkCmdSetScissor_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstScissor, params->scissorCount, params->pScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetScissor(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstScissor, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetScissorWithCount(void *args)
{
    struct vkCmdSetScissorWithCount_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->scissorCount, params->pScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetScissorWithCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetScissorWithCount(void *args)
{
    struct vkCmdSetScissorWithCount_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->scissorCount, params->pScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetScissorWithCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetScissorWithCountEXT(void *args)
{
    struct vkCmdSetScissorWithCountEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->scissorCount, params->pScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetScissorWithCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetScissorWithCountEXT(void *args)
{
    struct vkCmdSetScissorWithCountEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->scissorCount, params->pScissors);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetScissorWithCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->scissorCount, params->pScissors);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetShadingRateImageEnableNV(void *args)
{
    struct vkCmdSetShadingRateImageEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->shadingRateImageEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetShadingRateImageEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->shadingRateImageEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetShadingRateImageEnableNV(void *args)
{
    struct vkCmdSetShadingRateImageEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->shadingRateImageEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetShadingRateImageEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->shadingRateImageEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetStencilCompareMask(void *args)
{
    struct vkCmdSetStencilCompareMask_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->compareMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilCompareMask(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->compareMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetStencilCompareMask(void *args)
{
    struct vkCmdSetStencilCompareMask_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->compareMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilCompareMask(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->compareMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetStencilOp(void *args)
{
    struct vkCmdSetStencilOp_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x\n", params->commandBuffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilOp(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetStencilOp(void *args)
{
    struct vkCmdSetStencilOp_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x\n", params->commandBuffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilOp(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetStencilOpEXT(void *args)
{
    struct vkCmdSetStencilOpEXT_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x\n", params->commandBuffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilOpEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetStencilOpEXT(void *args)
{
    struct vkCmdSetStencilOpEXT_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x\n", params->commandBuffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilOpEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->failOp, params->passOp, params->depthFailOp, params->compareOp);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetStencilReference(void *args)
{
    struct vkCmdSetStencilReference_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->reference);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilReference(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->reference);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetStencilReference(void *args)
{
    struct vkCmdSetStencilReference_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->reference);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilReference(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->reference);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetStencilTestEnable(void *args)
{
    struct vkCmdSetStencilTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stencilTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilTestEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stencilTestEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetStencilTestEnable(void *args)
{
    struct vkCmdSetStencilTestEnable_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stencilTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilTestEnable(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stencilTestEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetStencilTestEnableEXT(void *args)
{
    struct vkCmdSetStencilTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stencilTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilTestEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stencilTestEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetStencilTestEnableEXT(void *args)
{
    struct vkCmdSetStencilTestEnableEXT_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->stencilTestEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilTestEnableEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stencilTestEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetStencilWriteMask(void *args)
{
    struct vkCmdSetStencilWriteMask_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->writeMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilWriteMask(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->writeMask);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetStencilWriteMask(void *args)
{
    struct vkCmdSetStencilWriteMask_params *params = args;
    TRACE("%p, %#x, %u\n", params->commandBuffer, params->faceMask, params->writeMask);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetStencilWriteMask(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->faceMask, params->writeMask);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetTessellationDomainOriginEXT(void *args)
{
    struct vkCmdSetTessellationDomainOriginEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->domainOrigin);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetTessellationDomainOriginEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->domainOrigin);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetTessellationDomainOriginEXT(void *args)
{
    struct vkCmdSetTessellationDomainOriginEXT_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->domainOrigin);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetTessellationDomainOriginEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->domainOrigin);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetVertexInputEXT(void *args)
{
    struct vkCmdSetVertexInputEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %p\n", params->commandBuffer, params->vertexBindingDescriptionCount, params->pVertexBindingDescriptions, params->vertexAttributeDescriptionCount, params->pVertexAttributeDescriptions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetVertexInputEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->vertexBindingDescriptionCount, params->pVertexBindingDescriptions, params->vertexAttributeDescriptionCount, params->pVertexAttributeDescriptions);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetVertexInputEXT(void *args)
{
    struct vkCmdSetVertexInputEXT_params *params = args;
    TRACE("%p, %u, %p, %u, %p\n", params->commandBuffer, params->vertexBindingDescriptionCount, params->pVertexBindingDescriptions, params->vertexAttributeDescriptionCount, params->pVertexAttributeDescriptions);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetVertexInputEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->vertexBindingDescriptionCount, params->pVertexBindingDescriptions, params->vertexAttributeDescriptionCount, params->pVertexAttributeDescriptions);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetViewport(void *args)
{
    struct vkCmdSetViewport_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewports);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewport(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetViewport(void *args)
{
    struct vkCmdSetViewport_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewports);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewport(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetViewportShadingRatePaletteNV(void *args)
{
    struct vkCmdSetViewportShadingRatePaletteNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pShadingRatePalettes);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportShadingRatePaletteNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pShadingRatePalettes);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetViewportShadingRatePaletteNV(void *args)
{
    struct vkCmdSetViewportShadingRatePaletteNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pShadingRatePalettes);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportShadingRatePaletteNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pShadingRatePalettes);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetViewportSwizzleNV(void *args)
{
    struct vkCmdSetViewportSwizzleNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewportSwizzles);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportSwizzleNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pViewportSwizzles);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetViewportSwizzleNV(void *args)
{
    struct vkCmdSetViewportSwizzleNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewportSwizzles);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportSwizzleNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pViewportSwizzles);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetViewportWScalingEnableNV(void *args)
{
    struct vkCmdSetViewportWScalingEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->viewportWScalingEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWScalingEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->viewportWScalingEnable);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetViewportWScalingEnableNV(void *args)
{
    struct vkCmdSetViewportWScalingEnableNV_params *params = args;
    TRACE("%p, %u\n", params->commandBuffer, params->viewportWScalingEnable);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWScalingEnableNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->viewportWScalingEnable);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetViewportWScalingNV(void *args)
{
    struct vkCmdSetViewportWScalingNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewportWScalings);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWScalingNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pViewportWScalings);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetViewportWScalingNV(void *args)
{
    struct vkCmdSetViewportWScalingNV_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->commandBuffer, params->firstViewport, params->viewportCount, params->pViewportWScalings);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWScalingNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->firstViewport, params->viewportCount, params->pViewportWScalings);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetViewportWithCount(void *args)
{
    struct vkCmdSetViewportWithCount_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->viewportCount, params->pViewports);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWithCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetViewportWithCount(void *args)
{
    struct vkCmdSetViewportWithCount_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->viewportCount, params->pViewports);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWithCount(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSetViewportWithCountEXT(void *args)
{
    struct vkCmdSetViewportWithCountEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->viewportCount, params->pViewports);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWithCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSetViewportWithCountEXT(void *args)
{
    struct vkCmdSetViewportWithCountEXT_params *params = args;
    TRACE("%p, %u, %p\n", params->commandBuffer, params->viewportCount, params->pViewports);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSetViewportWithCountEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->viewportCount, params->pViewports);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdSubpassShadingHUAWEI(void *args)
{
    struct vkCmdSubpassShadingHUAWEI_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSubpassShadingHUAWEI(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdSubpassShadingHUAWEI(void *args)
{
    struct vkCmdSubpassShadingHUAWEI_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdSubpassShadingHUAWEI(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdTraceRaysIndirect2KHR(void *args)
{
    struct vkCmdTraceRaysIndirect2KHR_params *params = args;
    TRACE("%p, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->indirectDeviceAddress));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysIndirect2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->indirectDeviceAddress);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdTraceRaysIndirect2KHR(void *args)
{
    struct vkCmdTraceRaysIndirect2KHR_params *params = args;
    TRACE("%p, 0x%s\n", params->commandBuffer, wine_dbgstr_longlong(params->indirectDeviceAddress));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysIndirect2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->indirectDeviceAddress);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdTraceRaysIndirectKHR(void *args)
{
    struct vkCmdTraceRaysIndirectKHR_params *params = args;
    TRACE("%p, %p, %p, %p, %p, 0x%s\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, wine_dbgstr_longlong(params->indirectDeviceAddress));

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysIndirectKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->indirectDeviceAddress);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdTraceRaysIndirectKHR(void *args)
{
    struct vkCmdTraceRaysIndirectKHR_params *params = args;
    VkStridedDeviceAddressRegionKHR_host pRaygenShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pMissShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pHitShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pCallableShaderBindingTable_host;
    TRACE("%p, %p, %p, %p, %p, 0x%s\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, wine_dbgstr_longlong(params->indirectDeviceAddress));

    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pRaygenShaderBindingTable, &pRaygenShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pMissShaderBindingTable, &pMissShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pHitShaderBindingTable, &pHitShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pCallableShaderBindingTable, &pCallableShaderBindingTable_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysIndirectKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pRaygenShaderBindingTable_host, &pMissShaderBindingTable_host, &pHitShaderBindingTable_host, &pCallableShaderBindingTable_host, params->indirectDeviceAddress);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdTraceRaysKHR(void *args)
{
    struct vkCmdTraceRaysKHR_params *params = args;
    TRACE("%p, %p, %p, %p, %p, %u, %u, %u\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->width, params->height, params->depth);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->width, params->height, params->depth);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdTraceRaysKHR(void *args)
{
    struct vkCmdTraceRaysKHR_params *params = args;
    VkStridedDeviceAddressRegionKHR_host pRaygenShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pMissShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pHitShaderBindingTable_host;
    VkStridedDeviceAddressRegionKHR_host pCallableShaderBindingTable_host;
    TRACE("%p, %p, %p, %p, %p, %u, %u, %u\n", params->commandBuffer, params->pRaygenShaderBindingTable, params->pMissShaderBindingTable, params->pHitShaderBindingTable, params->pCallableShaderBindingTable, params->width, params->height, params->depth);

    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pRaygenShaderBindingTable, &pRaygenShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pMissShaderBindingTable, &pMissShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pHitShaderBindingTable, &pHitShaderBindingTable_host);
    convert_VkStridedDeviceAddressRegionKHR_win32_to_host(params->pCallableShaderBindingTable, &pCallableShaderBindingTable_host);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, &pRaygenShaderBindingTable_host, &pMissShaderBindingTable_host, &pHitShaderBindingTable_host, &pCallableShaderBindingTable_host, params->width, params->height, params->depth);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdTraceRaysNV(void *args)
{
    struct vkCmdTraceRaysNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->raygenShaderBindingTableBuffer), wine_dbgstr_longlong(params->raygenShaderBindingOffset), wine_dbgstr_longlong(params->missShaderBindingTableBuffer), wine_dbgstr_longlong(params->missShaderBindingOffset), wine_dbgstr_longlong(params->missShaderBindingStride), wine_dbgstr_longlong(params->hitShaderBindingTableBuffer), wine_dbgstr_longlong(params->hitShaderBindingOffset), wine_dbgstr_longlong(params->hitShaderBindingStride), wine_dbgstr_longlong(params->callableShaderBindingTableBuffer), wine_dbgstr_longlong(params->callableShaderBindingOffset), wine_dbgstr_longlong(params->callableShaderBindingStride), params->width, params->height, params->depth);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->raygenShaderBindingTableBuffer, params->raygenShaderBindingOffset, params->missShaderBindingTableBuffer, params->missShaderBindingOffset, params->missShaderBindingStride, params->hitShaderBindingTableBuffer, params->hitShaderBindingOffset, params->hitShaderBindingStride, params->callableShaderBindingTableBuffer, params->callableShaderBindingOffset, params->callableShaderBindingStride, params->width, params->height, params->depth);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdTraceRaysNV(void *args)
{
    struct vkCmdTraceRaysNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, %u, %u, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->raygenShaderBindingTableBuffer), wine_dbgstr_longlong(params->raygenShaderBindingOffset), wine_dbgstr_longlong(params->missShaderBindingTableBuffer), wine_dbgstr_longlong(params->missShaderBindingOffset), wine_dbgstr_longlong(params->missShaderBindingStride), wine_dbgstr_longlong(params->hitShaderBindingTableBuffer), wine_dbgstr_longlong(params->hitShaderBindingOffset), wine_dbgstr_longlong(params->hitShaderBindingStride), wine_dbgstr_longlong(params->callableShaderBindingTableBuffer), wine_dbgstr_longlong(params->callableShaderBindingOffset), wine_dbgstr_longlong(params->callableShaderBindingStride), params->width, params->height, params->depth);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdTraceRaysNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->raygenShaderBindingTableBuffer, params->raygenShaderBindingOffset, params->missShaderBindingTableBuffer, params->missShaderBindingOffset, params->missShaderBindingStride, params->hitShaderBindingTableBuffer, params->hitShaderBindingOffset, params->hitShaderBindingStride, params->callableShaderBindingTableBuffer, params->callableShaderBindingOffset, params->callableShaderBindingStride, params->width, params->height, params->depth);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdUpdateBuffer(void *args)
{
    struct vkCmdUpdateBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->dataSize), params->pData);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdUpdateBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->dstBuffer, params->dstOffset, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdUpdateBuffer(void *args)
{
    struct vkCmdUpdateBuffer_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %p\n", params->commandBuffer, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), wine_dbgstr_longlong(params->dataSize), params->pData);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdUpdateBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->dstBuffer, params->dstOffset, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWaitEvents(void *args)
{
    struct vkCmdWaitEvents_params *params = args;
    TRACE("%p, %u, %p, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWaitEvents(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWaitEvents(void *args)
{
    struct vkCmdWaitEvents_params *params = args;
    VkBufferMemoryBarrier_host *pBufferMemoryBarriers_host;
    VkImageMemoryBarrier_host *pImageMemoryBarriers_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, %#x, %#x, %u, %p, %u, %p, %u, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, params->pBufferMemoryBarriers, params->imageMemoryBarrierCount, params->pImageMemoryBarriers);

    init_conversion_context(&ctx);
    pBufferMemoryBarriers_host = convert_VkBufferMemoryBarrier_array_win32_to_host(&ctx, params->pBufferMemoryBarriers, params->bufferMemoryBarrierCount);
    pImageMemoryBarriers_host = convert_VkImageMemoryBarrier_array_win32_to_host(&ctx, params->pImageMemoryBarriers, params->imageMemoryBarrierCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWaitEvents(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->eventCount, params->pEvents, params->srcStageMask, params->dstStageMask, params->memoryBarrierCount, params->pMemoryBarriers, params->bufferMemoryBarrierCount, pBufferMemoryBarriers_host, params->imageMemoryBarrierCount, pImageMemoryBarriers_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWaitEvents2(void *args)
{
    struct vkCmdWaitEvents2_params *params = args;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWaitEvents2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->eventCount, params->pEvents, params->pDependencyInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWaitEvents2(void *args)
{
    struct vkCmdWaitEvents2_params *params = args;
    VkDependencyInfo_host *pDependencyInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);

    init_conversion_context(&ctx);
    pDependencyInfos_host = convert_VkDependencyInfo_array_win32_to_host(&ctx, params->pDependencyInfos, params->eventCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWaitEvents2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->eventCount, params->pEvents, pDependencyInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWaitEvents2KHR(void *args)
{
    struct vkCmdWaitEvents2KHR_params *params = args;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWaitEvents2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->eventCount, params->pEvents, params->pDependencyInfos);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWaitEvents2KHR(void *args)
{
    struct vkCmdWaitEvents2KHR_params *params = args;
    VkDependencyInfo_host *pDependencyInfos_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, %p\n", params->commandBuffer, params->eventCount, params->pEvents, params->pDependencyInfos);

    init_conversion_context(&ctx);
    pDependencyInfos_host = convert_VkDependencyInfo_array_win32_to_host(&ctx, params->pDependencyInfos, params->eventCount);
    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWaitEvents2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->eventCount, params->pEvents, pDependencyInfos_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteAccelerationStructuresPropertiesKHR(void *args)
{
    struct vkCmdWriteAccelerationStructuresPropertiesKHR_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteAccelerationStructuresPropertiesKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteAccelerationStructuresPropertiesKHR(void *args)
{
    struct vkCmdWriteAccelerationStructuresPropertiesKHR_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteAccelerationStructuresPropertiesKHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteAccelerationStructuresPropertiesNV(void *args)
{
    struct vkCmdWriteAccelerationStructuresPropertiesNV_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteAccelerationStructuresPropertiesNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteAccelerationStructuresPropertiesNV(void *args)
{
    struct vkCmdWriteAccelerationStructuresPropertiesNV_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteAccelerationStructuresPropertiesNV(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteBufferMarker2AMD(void *args)
{
    struct vkCmdWriteBufferMarker2AMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), params->marker);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteBufferMarker2AMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stage, params->dstBuffer, params->dstOffset, params->marker);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteBufferMarker2AMD(void *args)
{
    struct vkCmdWriteBufferMarker2AMD_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), params->marker);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteBufferMarker2AMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stage, params->dstBuffer, params->dstOffset, params->marker);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteBufferMarkerAMD(void *args)
{
    struct vkCmdWriteBufferMarkerAMD_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %u\n", params->commandBuffer, params->pipelineStage, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), params->marker);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteBufferMarkerAMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineStage, params->dstBuffer, params->dstOffset, params->marker);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteBufferMarkerAMD(void *args)
{
    struct vkCmdWriteBufferMarkerAMD_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %u\n", params->commandBuffer, params->pipelineStage, wine_dbgstr_longlong(params->dstBuffer), wine_dbgstr_longlong(params->dstOffset), params->marker);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteBufferMarkerAMD(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineStage, params->dstBuffer, params->dstOffset, params->marker);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteMicromapsPropertiesEXT(void *args)
{
    struct vkCmdWriteMicromapsPropertiesEXT_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->micromapCount, params->pMicromaps, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteMicromapsPropertiesEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->micromapCount, params->pMicromaps, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteMicromapsPropertiesEXT(void *args)
{
    struct vkCmdWriteMicromapsPropertiesEXT_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %u\n", params->commandBuffer, params->micromapCount, params->pMicromaps, params->queryType, wine_dbgstr_longlong(params->queryPool), params->firstQuery);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteMicromapsPropertiesEXT(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->micromapCount, params->pMicromaps, params->queryType, params->queryPool, params->firstQuery);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteTimestamp(void *args)
{
    struct vkCmdWriteTimestamp_params *params = args;
    TRACE("%p, %#x, 0x%s, %u\n", params->commandBuffer, params->pipelineStage, wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteTimestamp(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineStage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteTimestamp(void *args)
{
    struct vkCmdWriteTimestamp_params *params = args;
    TRACE("%p, %#x, 0x%s, %u\n", params->commandBuffer, params->pipelineStage, wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteTimestamp(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->pipelineStage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteTimestamp2(void *args)
{
    struct vkCmdWriteTimestamp2_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteTimestamp2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteTimestamp2(void *args)
{
    struct vkCmdWriteTimestamp2_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteTimestamp2(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCmdWriteTimestamp2KHR(void *args)
{
    struct vkCmdWriteTimestamp2KHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteTimestamp2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCmdWriteTimestamp2KHR(void *args)
{
    struct vkCmdWriteTimestamp2KHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u\n", params->commandBuffer, wine_dbgstr_longlong(params->stage), wine_dbgstr_longlong(params->queryPool), params->query);

    wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkCmdWriteTimestamp2KHR(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->stage, params->queryPool, params->query);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCompileDeferredNV(void *args)
{
    struct vkCompileDeferredNV_params *params = args;
    TRACE("%p, 0x%s, %u\n", params->device, wine_dbgstr_longlong(params->pipeline), params->shader);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCompileDeferredNV(wine_device_from_handle(params->device)->device, params->pipeline, params->shader);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCompileDeferredNV(void *args)
{
    struct vkCompileDeferredNV_params *params = args;
    TRACE("%p, 0x%s, %u\n", params->device, wine_dbgstr_longlong(params->pipeline), params->shader);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCompileDeferredNV(wine_device_from_handle(params->device)->device, params->pipeline, params->shader);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCopyAccelerationStructureKHR(void *args)
{
    struct vkCopyAccelerationStructureKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyAccelerationStructureKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCopyAccelerationStructureKHR(void *args)
{
    struct vkCopyAccelerationStructureKHR_params *params = args;
    VkCopyAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyAccelerationStructureInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyAccelerationStructureKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCopyAccelerationStructureToMemoryKHR(void *args)
{
    struct vkCopyAccelerationStructureToMemoryKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyAccelerationStructureToMemoryKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCopyAccelerationStructureToMemoryKHR(void *args)
{
    struct vkCopyAccelerationStructureToMemoryKHR_params *params = args;
    VkCopyAccelerationStructureToMemoryInfoKHR_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyAccelerationStructureToMemoryInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyAccelerationStructureToMemoryKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCopyMemoryToAccelerationStructureKHR(void *args)
{
    struct vkCopyMemoryToAccelerationStructureKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMemoryToAccelerationStructureKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCopyMemoryToAccelerationStructureKHR(void *args)
{
    struct vkCopyMemoryToAccelerationStructureKHR_params *params = args;
    VkCopyMemoryToAccelerationStructureInfoKHR_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyMemoryToAccelerationStructureInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMemoryToAccelerationStructureKHR(wine_device_from_handle(params->device)->device, params->deferredOperation, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCopyMemoryToMicromapEXT(void *args)
{
    struct vkCopyMemoryToMicromapEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMemoryToMicromapEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCopyMemoryToMicromapEXT(void *args)
{
    struct vkCopyMemoryToMicromapEXT_params *params = args;
    VkCopyMemoryToMicromapInfoEXT_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyMemoryToMicromapInfoEXT_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMemoryToMicromapEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCopyMicromapEXT(void *args)
{
    struct vkCopyMicromapEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMicromapEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCopyMicromapEXT(void *args)
{
    struct vkCopyMicromapEXT_params *params = args;
    VkCopyMicromapInfoEXT_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyMicromapInfoEXT_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMicromapEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCopyMicromapToMemoryEXT(void *args)
{
    struct vkCopyMicromapToMemoryEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMicromapToMemoryEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCopyMicromapToMemoryEXT(void *args)
{
    struct vkCopyMicromapToMemoryEXT_params *params = args;
    VkCopyMicromapToMemoryInfoEXT_host pInfo_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), params->pInfo);

    convert_VkCopyMicromapToMemoryInfoEXT_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCopyMicromapToMemoryEXT(wine_device_from_handle(params->device)->device, params->deferredOperation, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateAccelerationStructureKHR(void *args)
{
    struct vkCreateAccelerationStructureKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateAccelerationStructureKHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pAccelerationStructure);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateAccelerationStructureKHR(void *args)
{
    struct vkCreateAccelerationStructureKHR_params *params = args;
    VkAccelerationStructureCreateInfoKHR_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);

    convert_VkAccelerationStructureCreateInfoKHR_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateAccelerationStructureKHR(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pAccelerationStructure);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateAccelerationStructureNV(void *args)
{
    struct vkCreateAccelerationStructureNV_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateAccelerationStructureNV(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pAccelerationStructure);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateAccelerationStructureNV(void *args)
{
    struct vkCreateAccelerationStructureNV_params *params = args;
    VkAccelerationStructureCreateInfoNV_host pCreateInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pAccelerationStructure);

    init_conversion_context(&ctx);
    convert_VkAccelerationStructureCreateInfoNV_win32_to_host(&ctx, params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateAccelerationStructureNV(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pAccelerationStructure);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateBuffer(void *args)
{
    struct vkCreateBuffer_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pBuffer);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateBuffer(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pBuffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateBuffer(void *args)
{
    struct vkCreateBuffer_params *params = args;
    VkBufferCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pBuffer);

    convert_VkBufferCreateInfo_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateBuffer(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pBuffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateBufferView(void *args)
{
    struct vkCreateBufferView_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateBufferView(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pView);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateBufferView(void *args)
{
    struct vkCreateBufferView_params *params = args;
    VkBufferViewCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);

    convert_VkBufferViewCreateInfo_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateBufferView(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pView);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateCommandPool(void *args)
{
    struct vkCreateCommandPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pCommandPool);

    params->result = wine_vkCreateCommandPool(params->device, params->pCreateInfo, params->pAllocator, params->pCommandPool, params->client_ptr);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateCommandPool(void *args)
{
    struct vkCreateCommandPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pCommandPool);

    params->result = wine_vkCreateCommandPool(params->device, params->pCreateInfo, params->pAllocator, params->pCommandPool, params->client_ptr);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    result = wine_device_from_handle(device)->funcs.p_vkCreateComputePipelines(wine_device_from_handle(device)->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
    return result;
}

static NTSTATUS thunk64_vkCreateComputePipelines(void *args)
{
    struct vkCreateComputePipelines_params *params = args;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateComputePipelines(params->device, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    VkComputePipelineCreateInfo_host *pCreateInfos_host;
    struct conversion_context ctx;
    init_conversion_context(&ctx);
    pCreateInfos_host = convert_VkComputePipelineCreateInfo_array_win32_to_host(&ctx, pCreateInfos, createInfoCount);
    result = wine_device_from_handle(device)->funcs.p_vkCreateComputePipelines(wine_device_from_handle(device)->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);
    free_conversion_context(&ctx);
    return result;
}

static NTSTATUS thunk32_vkCreateComputePipelines(void *args)
{
    struct vkCreateComputePipelines_params *params = args;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateComputePipelines(params->device, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateCuFunctionNVX(void *args)
{
    struct vkCreateCuFunctionNVX_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFunction);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateCuFunctionNVX(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pFunction);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateCuFunctionNVX(void *args)
{
    struct vkCreateCuFunctionNVX_params *params = args;
    VkCuFunctionCreateInfoNVX_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFunction);

    convert_VkCuFunctionCreateInfoNVX_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateCuFunctionNVX(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pFunction);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateCuModuleNVX(void *args)
{
    struct vkCreateCuModuleNVX_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pModule);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateCuModuleNVX(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pModule);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateCuModuleNVX(void *args)
{
    struct vkCreateCuModuleNVX_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pModule);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateCuModuleNVX(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pModule);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDebugReportCallbackEXT(void *args)
{
    struct vkCreateDebugReportCallbackEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->instance, params->pCreateInfo, params->pAllocator, params->pCallback);

    params->result = wine_vkCreateDebugReportCallbackEXT(params->instance, params->pCreateInfo, params->pAllocator, params->pCallback);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDebugReportCallbackEXT(void *args)
{
    struct vkCreateDebugReportCallbackEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->instance, params->pCreateInfo, params->pAllocator, params->pCallback);

    params->result = wine_vkCreateDebugReportCallbackEXT(params->instance, params->pCreateInfo, params->pAllocator, params->pCallback);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDebugUtilsMessengerEXT(void *args)
{
    struct vkCreateDebugUtilsMessengerEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->instance, params->pCreateInfo, params->pAllocator, params->pMessenger);

    params->result = wine_vkCreateDebugUtilsMessengerEXT(params->instance, params->pCreateInfo, params->pAllocator, params->pMessenger);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDebugUtilsMessengerEXT(void *args)
{
    struct vkCreateDebugUtilsMessengerEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->instance, params->pCreateInfo, params->pAllocator, params->pMessenger);

    params->result = wine_vkCreateDebugUtilsMessengerEXT(params->instance, params->pCreateInfo, params->pAllocator, params->pMessenger);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDeferredOperationKHR(void *args)
{
    struct vkCreateDeferredOperationKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAllocator, params->pDeferredOperation);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDeferredOperationKHR(wine_device_from_handle(params->device)->device, NULL, params->pDeferredOperation);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDeferredOperationKHR(void *args)
{
    struct vkCreateDeferredOperationKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pAllocator, params->pDeferredOperation);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDeferredOperationKHR(wine_device_from_handle(params->device)->device, NULL, params->pDeferredOperation);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDescriptorPool(void *args)
{
    struct vkCreateDescriptorPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorPool);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorPool(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pDescriptorPool);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDescriptorPool(void *args)
{
    struct vkCreateDescriptorPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorPool);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorPool(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pDescriptorPool);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDescriptorSetLayout(void *args)
{
    struct vkCreateDescriptorSetLayout_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSetLayout);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorSetLayout(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSetLayout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDescriptorSetLayout(void *args)
{
    struct vkCreateDescriptorSetLayout_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSetLayout);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorSetLayout(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSetLayout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDescriptorUpdateTemplate(void *args)
{
    struct vkCreateDescriptorUpdateTemplate_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorUpdateTemplate(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pDescriptorUpdateTemplate);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDescriptorUpdateTemplate(void *args)
{
    struct vkCreateDescriptorUpdateTemplate_params *params = args;
    VkDescriptorUpdateTemplateCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);

    convert_VkDescriptorUpdateTemplateCreateInfo_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorUpdateTemplate(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pDescriptorUpdateTemplate);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDescriptorUpdateTemplateKHR(void *args)
{
    struct vkCreateDescriptorUpdateTemplateKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorUpdateTemplateKHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pDescriptorUpdateTemplate);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDescriptorUpdateTemplateKHR(void *args)
{
    struct vkCreateDescriptorUpdateTemplateKHR_params *params = args;
    VkDescriptorUpdateTemplateCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pDescriptorUpdateTemplate);

    convert_VkDescriptorUpdateTemplateCreateInfo_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateDescriptorUpdateTemplateKHR(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pDescriptorUpdateTemplate);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateDevice(void *args)
{
    struct vkCreateDevice_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pCreateInfo, params->pAllocator, params->pDevice);

    params->result = wine_vkCreateDevice(params->physicalDevice, params->pCreateInfo, params->pAllocator, params->pDevice, params->client_ptr);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateDevice(void *args)
{
    struct vkCreateDevice_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pCreateInfo, params->pAllocator, params->pDevice);

    params->result = wine_vkCreateDevice(params->physicalDevice, params->pCreateInfo, params->pAllocator, params->pDevice, params->client_ptr);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateEvent(void *args)
{
    struct vkCreateEvent_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pEvent);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateEvent(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pEvent);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateEvent(void *args)
{
    struct vkCreateEvent_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pEvent);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateEvent(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pEvent);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateFence(void *args)
{
    struct vkCreateFence_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFence);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateFence(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pFence);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateFence(void *args)
{
    struct vkCreateFence_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFence);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateFence(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pFence);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateFramebuffer(void *args)
{
    struct vkCreateFramebuffer_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFramebuffer);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateFramebuffer(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pFramebuffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateFramebuffer(void *args)
{
    struct vkCreateFramebuffer_params *params = args;
    VkFramebufferCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pFramebuffer);

    convert_VkFramebufferCreateInfo_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateFramebuffer(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pFramebuffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    result = wine_device_from_handle(device)->funcs.p_vkCreateGraphicsPipelines(wine_device_from_handle(device)->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
    return result;
}

static NTSTATUS thunk64_vkCreateGraphicsPipelines(void *args)
{
    struct vkCreateGraphicsPipelines_params *params = args;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateGraphicsPipelines(params->device, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    VkGraphicsPipelineCreateInfo_host *pCreateInfos_host;
    struct conversion_context ctx;
    init_conversion_context(&ctx);
    pCreateInfos_host = convert_VkGraphicsPipelineCreateInfo_array_win32_to_host(&ctx, pCreateInfos, createInfoCount);
    result = wine_device_from_handle(device)->funcs.p_vkCreateGraphicsPipelines(wine_device_from_handle(device)->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);
    free_conversion_context(&ctx);
    return result;
}

static NTSTATUS thunk32_vkCreateGraphicsPipelines(void *args)
{
    struct vkCreateGraphicsPipelines_params *params = args;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateGraphicsPipelines(params->device, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateImage(void *args)
{
    struct vkCreateImage_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pImage);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateImage(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pImage);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateImage(void *args)
{
    struct vkCreateImage_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pImage);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateImage(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pImage);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateImageView(void *args)
{
    struct vkCreateImageView_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateImageView(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pView);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateImageView(void *args)
{
    struct vkCreateImageView_params *params = args;
    VkImageViewCreateInfo_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pView);

    convert_VkImageViewCreateInfo_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateImageView(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pView);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateIndirectCommandsLayoutNV(void *args)
{
    struct vkCreateIndirectCommandsLayoutNV_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pIndirectCommandsLayout);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateIndirectCommandsLayoutNV(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pIndirectCommandsLayout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateIndirectCommandsLayoutNV(void *args)
{
    struct vkCreateIndirectCommandsLayoutNV_params *params = args;
    VkIndirectCommandsLayoutCreateInfoNV_host pCreateInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pIndirectCommandsLayout);

    init_conversion_context(&ctx);
    convert_VkIndirectCommandsLayoutCreateInfoNV_win32_to_host(&ctx, params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateIndirectCommandsLayoutNV(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pIndirectCommandsLayout);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateInstance(void *args)
{
    struct vkCreateInstance_params *params = args;
    TRACE("%p, %p, %p\n", params->pCreateInfo, params->pAllocator, params->pInstance);

    params->result = wine_vkCreateInstance(params->pCreateInfo, params->pAllocator, params->pInstance, params->client_ptr);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateInstance(void *args)
{
    struct vkCreateInstance_params *params = args;
    TRACE("%p, %p, %p\n", params->pCreateInfo, params->pAllocator, params->pInstance);

    params->result = wine_vkCreateInstance(params->pCreateInfo, params->pAllocator, params->pInstance, params->client_ptr);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateMicromapEXT(void *args)
{
    struct vkCreateMicromapEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pMicromap);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateMicromapEXT(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pMicromap);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateMicromapEXT(void *args)
{
    struct vkCreateMicromapEXT_params *params = args;
    VkMicromapCreateInfoEXT_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pMicromap);

    convert_VkMicromapCreateInfoEXT_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateMicromapEXT(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pMicromap);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateOpticalFlowSessionNV(void *args)
{
    struct vkCreateOpticalFlowSessionNV_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSession);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateOpticalFlowSessionNV(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSession);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateOpticalFlowSessionNV(void *args)
{
    struct vkCreateOpticalFlowSessionNV_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSession);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateOpticalFlowSessionNV(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSession);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreatePipelineCache(void *args)
{
    struct vkCreatePipelineCache_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPipelineCache);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePipelineCache(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPipelineCache);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreatePipelineCache(void *args)
{
    struct vkCreatePipelineCache_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPipelineCache);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePipelineCache(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPipelineCache);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreatePipelineLayout(void *args)
{
    struct vkCreatePipelineLayout_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPipelineLayout);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePipelineLayout(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPipelineLayout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreatePipelineLayout(void *args)
{
    struct vkCreatePipelineLayout_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPipelineLayout);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePipelineLayout(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPipelineLayout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreatePrivateDataSlot(void *args)
{
    struct vkCreatePrivateDataSlot_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPrivateDataSlot);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePrivateDataSlot(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPrivateDataSlot);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreatePrivateDataSlot(void *args)
{
    struct vkCreatePrivateDataSlot_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPrivateDataSlot);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePrivateDataSlot(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPrivateDataSlot);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreatePrivateDataSlotEXT(void *args)
{
    struct vkCreatePrivateDataSlotEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPrivateDataSlot);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePrivateDataSlotEXT(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPrivateDataSlot);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreatePrivateDataSlotEXT(void *args)
{
    struct vkCreatePrivateDataSlotEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pPrivateDataSlot);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreatePrivateDataSlotEXT(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pPrivateDataSlot);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateQueryPool(void *args)
{
    struct vkCreateQueryPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pQueryPool);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateQueryPool(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pQueryPool);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateQueryPool(void *args)
{
    struct vkCreateQueryPool_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pQueryPool);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateQueryPool(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pQueryPool);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    result = wine_device_from_handle(device)->funcs.p_vkCreateRayTracingPipelinesKHR(wine_device_from_handle(device)->device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
    return result;
}

static NTSTATUS thunk64_vkCreateRayTracingPipelinesKHR(void *args)
{
    struct vkCreateRayTracingPipelinesKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateRayTracingPipelinesKHR(params->device, params->deferredOperation, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    VkRayTracingPipelineCreateInfoKHR_host *pCreateInfos_host;
    struct conversion_context ctx;
    init_conversion_context(&ctx);
    pCreateInfos_host = convert_VkRayTracingPipelineCreateInfoKHR_array_win32_to_host(&ctx, pCreateInfos, createInfoCount);
    result = wine_device_from_handle(device)->funcs.p_vkCreateRayTracingPipelinesKHR(wine_device_from_handle(device)->device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);
    free_conversion_context(&ctx);
    return result;
}

static NTSTATUS thunk32_vkCreateRayTracingPipelinesKHR(void *args)
{
    struct vkCreateRayTracingPipelinesKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->deferredOperation), wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateRayTracingPipelinesKHR(params->device, params->deferredOperation, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    result = wine_device_from_handle(device)->funcs.p_vkCreateRayTracingPipelinesNV(wine_device_from_handle(device)->device, pipelineCache, createInfoCount, pCreateInfos, NULL, pPipelines);
    return result;
}

static NTSTATUS thunk64_vkCreateRayTracingPipelinesNV(void *args)
{
    struct vkCreateRayTracingPipelinesNV_params *params = args;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateRayTracingPipelinesNV(params->device, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    VkResult result;
    VkRayTracingPipelineCreateInfoNV_host *pCreateInfos_host;
    struct conversion_context ctx;
    init_conversion_context(&ctx);
    pCreateInfos_host = convert_VkRayTracingPipelineCreateInfoNV_array_win32_to_host(&ctx, pCreateInfos, createInfoCount);
    result = wine_device_from_handle(device)->funcs.p_vkCreateRayTracingPipelinesNV(wine_device_from_handle(device)->device, pipelineCache, createInfoCount, pCreateInfos_host, NULL, pPipelines);
    free_conversion_context(&ctx);
    return result;
}

static NTSTATUS thunk32_vkCreateRayTracingPipelinesNV(void *args)
{
    struct vkCreateRayTracingPipelinesNV_params *params = args;
    TRACE("%p, 0x%s, %u, %p, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);

    params->result = wine_vkCreateRayTracingPipelinesNV(params->device, params->pipelineCache, params->createInfoCount, params->pCreateInfos, params->pAllocator, params->pPipelines);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateRenderPass(void *args)
{
    struct vkCreateRenderPass_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateRenderPass(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pRenderPass);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateRenderPass(void *args)
{
    struct vkCreateRenderPass_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateRenderPass(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pRenderPass);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateRenderPass2(void *args)
{
    struct vkCreateRenderPass2_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateRenderPass2(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pRenderPass);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateRenderPass2(void *args)
{
    struct vkCreateRenderPass2_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateRenderPass2(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pRenderPass);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateRenderPass2KHR(void *args)
{
    struct vkCreateRenderPass2KHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateRenderPass2KHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pRenderPass);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateRenderPass2KHR(void *args)
{
    struct vkCreateRenderPass2KHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pRenderPass);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateRenderPass2KHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pRenderPass);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateSampler(void *args)
{
    struct vkCreateSampler_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSampler);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSampler(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSampler);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateSampler(void *args)
{
    struct vkCreateSampler_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSampler);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSampler(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSampler);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateSamplerYcbcrConversion(void *args)
{
    struct vkCreateSamplerYcbcrConversion_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pYcbcrConversion);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSamplerYcbcrConversion(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pYcbcrConversion);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateSamplerYcbcrConversion(void *args)
{
    struct vkCreateSamplerYcbcrConversion_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pYcbcrConversion);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSamplerYcbcrConversion(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pYcbcrConversion);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateSamplerYcbcrConversionKHR(void *args)
{
    struct vkCreateSamplerYcbcrConversionKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pYcbcrConversion);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSamplerYcbcrConversionKHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pYcbcrConversion);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateSamplerYcbcrConversionKHR(void *args)
{
    struct vkCreateSamplerYcbcrConversionKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pYcbcrConversion);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSamplerYcbcrConversionKHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pYcbcrConversion);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateSemaphore(void *args)
{
    struct vkCreateSemaphore_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSemaphore);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSemaphore(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSemaphore);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateSemaphore(void *args)
{
    struct vkCreateSemaphore_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSemaphore);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSemaphore(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pSemaphore);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateShaderModule(void *args)
{
    struct vkCreateShaderModule_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pShaderModule);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateShaderModule(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pShaderModule);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateShaderModule(void *args)
{
    struct vkCreateShaderModule_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pShaderModule);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateShaderModule(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pShaderModule);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateSwapchainKHR(void *args)
{
    struct vkCreateSwapchainKHR_params *params = args;
    VkSwapchainCreateInfoKHR pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSwapchain);

    convert_VkSwapchainCreateInfoKHR_win64_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSwapchainKHR(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pSwapchain);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateSwapchainKHR(void *args)
{
    struct vkCreateSwapchainKHR_params *params = args;
    VkSwapchainCreateInfoKHR_host pCreateInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pSwapchain);

    convert_VkSwapchainCreateInfoKHR_win32_to_host(params->pCreateInfo, &pCreateInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateSwapchainKHR(wine_device_from_handle(params->device)->device, &pCreateInfo_host, NULL, params->pSwapchain);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateValidationCacheEXT(void *args)
{
    struct vkCreateValidationCacheEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pValidationCache);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateValidationCacheEXT(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pValidationCache);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateValidationCacheEXT(void *args)
{
    struct vkCreateValidationCacheEXT_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pCreateInfo, params->pAllocator, params->pValidationCache);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkCreateValidationCacheEXT(wine_device_from_handle(params->device)->device, params->pCreateInfo, NULL, params->pValidationCache);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkCreateWin32SurfaceKHR(void *args)
{
    struct vkCreateWin32SurfaceKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->instance, params->pCreateInfo, params->pAllocator, params->pSurface);

    params->result = wine_vkCreateWin32SurfaceKHR(params->instance, params->pCreateInfo, params->pAllocator, params->pSurface);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkCreateWin32SurfaceKHR(void *args)
{
    struct vkCreateWin32SurfaceKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->instance, params->pCreateInfo, params->pAllocator, params->pSurface);

    params->result = wine_vkCreateWin32SurfaceKHR(params->instance, params->pCreateInfo, params->pAllocator, params->pSurface);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDebugMarkerSetObjectNameEXT(void *args)
{
    struct vkDebugMarkerSetObjectNameEXT_params *params = args;
    VkDebugMarkerObjectNameInfoEXT pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugMarkerObjectNameInfoEXT_win64_to_host(params->pNameInfo, &pNameInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkDebugMarkerSetObjectNameEXT(wine_device_from_handle(params->device)->device, &pNameInfo_host);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDebugMarkerSetObjectNameEXT(void *args)
{
    struct vkDebugMarkerSetObjectNameEXT_params *params = args;
    VkDebugMarkerObjectNameInfoEXT_host pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugMarkerObjectNameInfoEXT_win32_to_host(params->pNameInfo, &pNameInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkDebugMarkerSetObjectNameEXT(wine_device_from_handle(params->device)->device, &pNameInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDebugMarkerSetObjectTagEXT(void *args)
{
    struct vkDebugMarkerSetObjectTagEXT_params *params = args;
    VkDebugMarkerObjectTagInfoEXT pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugMarkerObjectTagInfoEXT_win64_to_host(params->pTagInfo, &pTagInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkDebugMarkerSetObjectTagEXT(wine_device_from_handle(params->device)->device, &pTagInfo_host);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDebugMarkerSetObjectTagEXT(void *args)
{
    struct vkDebugMarkerSetObjectTagEXT_params *params = args;
    VkDebugMarkerObjectTagInfoEXT_host pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugMarkerObjectTagInfoEXT_win32_to_host(params->pTagInfo, &pTagInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkDebugMarkerSetObjectTagEXT(wine_device_from_handle(params->device)->device, &pTagInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDebugReportMessageEXT(void *args)
{
    struct vkDebugReportMessageEXT_params *params = args;
    TRACE("%p, %#x, %#x, 0x%s, 0x%s, %d, %p, %p\n", params->instance, params->flags, params->objectType, wine_dbgstr_longlong(params->object), wine_dbgstr_longlong(params->location), params->messageCode, params->pLayerPrefix, params->pMessage);

    wine_instance_from_handle(params->instance)->funcs.p_vkDebugReportMessageEXT(wine_instance_from_handle(params->instance)->instance, params->flags, params->objectType, wine_vk_unwrap_handle(params->objectType, params->object), params->location, params->messageCode, params->pLayerPrefix, params->pMessage);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDebugReportMessageEXT(void *args)
{
    struct vkDebugReportMessageEXT_params *params = args;
    TRACE("%p, %#x, %#x, 0x%s, 0x%s, %d, %p, %p\n", params->instance, params->flags, params->objectType, wine_dbgstr_longlong(params->object), wine_dbgstr_longlong(params->location), params->messageCode, params->pLayerPrefix, params->pMessage);

    wine_instance_from_handle(params->instance)->funcs.p_vkDebugReportMessageEXT(wine_instance_from_handle(params->instance)->instance, params->flags, params->objectType, wine_vk_unwrap_handle(params->objectType, params->object), params->location, params->messageCode, params->pLayerPrefix, params->pMessage);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDeferredOperationJoinKHR(void *args)
{
    struct vkDeferredOperationJoinKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkDeferredOperationJoinKHR(wine_device_from_handle(params->device)->device, params->operation);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDeferredOperationJoinKHR(void *args)
{
    struct vkDeferredOperationJoinKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkDeferredOperationJoinKHR(wine_device_from_handle(params->device)->device, params->operation);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyAccelerationStructureKHR(void *args)
{
    struct vkDestroyAccelerationStructureKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyAccelerationStructureKHR(wine_device_from_handle(params->device)->device, params->accelerationStructure, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyAccelerationStructureKHR(void *args)
{
    struct vkDestroyAccelerationStructureKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyAccelerationStructureKHR(wine_device_from_handle(params->device)->device, params->accelerationStructure, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyAccelerationStructureNV(void *args)
{
    struct vkDestroyAccelerationStructureNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyAccelerationStructureNV(wine_device_from_handle(params->device)->device, params->accelerationStructure, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyAccelerationStructureNV(void *args)
{
    struct vkDestroyAccelerationStructureNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyAccelerationStructureNV(wine_device_from_handle(params->device)->device, params->accelerationStructure, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyBuffer(void *args)
{
    struct vkDestroyBuffer_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->buffer), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyBuffer(wine_device_from_handle(params->device)->device, params->buffer, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyBuffer(void *args)
{
    struct vkDestroyBuffer_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->buffer), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyBuffer(wine_device_from_handle(params->device)->device, params->buffer, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyBufferView(void *args)
{
    struct vkDestroyBufferView_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->bufferView), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyBufferView(wine_device_from_handle(params->device)->device, params->bufferView, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyBufferView(void *args)
{
    struct vkDestroyBufferView_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->bufferView), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyBufferView(wine_device_from_handle(params->device)->device, params->bufferView, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyCommandPool(void *args)
{
    struct vkDestroyCommandPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->commandPool), params->pAllocator);

    wine_vkDestroyCommandPool(params->device, params->commandPool, params->pAllocator);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyCommandPool(void *args)
{
    struct vkDestroyCommandPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->commandPool), params->pAllocator);

    wine_vkDestroyCommandPool(params->device, params->commandPool, params->pAllocator);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyCuFunctionNVX(void *args)
{
    struct vkDestroyCuFunctionNVX_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->function), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyCuFunctionNVX(wine_device_from_handle(params->device)->device, params->function, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyCuFunctionNVX(void *args)
{
    struct vkDestroyCuFunctionNVX_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->function), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyCuFunctionNVX(wine_device_from_handle(params->device)->device, params->function, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyCuModuleNVX(void *args)
{
    struct vkDestroyCuModuleNVX_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->module), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyCuModuleNVX(wine_device_from_handle(params->device)->device, params->module, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyCuModuleNVX(void *args)
{
    struct vkDestroyCuModuleNVX_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->module), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyCuModuleNVX(wine_device_from_handle(params->device)->device, params->module, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDebugReportCallbackEXT(void *args)
{
    struct vkDestroyDebugReportCallbackEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->instance, wine_dbgstr_longlong(params->callback), params->pAllocator);

    wine_vkDestroyDebugReportCallbackEXT(params->instance, params->callback, params->pAllocator);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDebugReportCallbackEXT(void *args)
{
    struct vkDestroyDebugReportCallbackEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->instance, wine_dbgstr_longlong(params->callback), params->pAllocator);

    wine_vkDestroyDebugReportCallbackEXT(params->instance, params->callback, params->pAllocator);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDebugUtilsMessengerEXT(void *args)
{
    struct vkDestroyDebugUtilsMessengerEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->instance, wine_dbgstr_longlong(params->messenger), params->pAllocator);

    wine_vkDestroyDebugUtilsMessengerEXT(params->instance, params->messenger, params->pAllocator);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDebugUtilsMessengerEXT(void *args)
{
    struct vkDestroyDebugUtilsMessengerEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->instance, wine_dbgstr_longlong(params->messenger), params->pAllocator);

    wine_vkDestroyDebugUtilsMessengerEXT(params->instance, params->messenger, params->pAllocator);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDeferredOperationKHR(void *args)
{
    struct vkDestroyDeferredOperationKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->operation), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDeferredOperationKHR(wine_device_from_handle(params->device)->device, params->operation, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDeferredOperationKHR(void *args)
{
    struct vkDestroyDeferredOperationKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->operation), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDeferredOperationKHR(wine_device_from_handle(params->device)->device, params->operation, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDescriptorPool(void *args)
{
    struct vkDestroyDescriptorPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorPool(wine_device_from_handle(params->device)->device, params->descriptorPool, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDescriptorPool(void *args)
{
    struct vkDestroyDescriptorPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorPool(wine_device_from_handle(params->device)->device, params->descriptorPool, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDescriptorSetLayout(void *args)
{
    struct vkDestroyDescriptorSetLayout_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSetLayout), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorSetLayout(wine_device_from_handle(params->device)->device, params->descriptorSetLayout, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDescriptorSetLayout(void *args)
{
    struct vkDestroyDescriptorSetLayout_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSetLayout), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorSetLayout(wine_device_from_handle(params->device)->device, params->descriptorSetLayout, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDescriptorUpdateTemplate(void *args)
{
    struct vkDestroyDescriptorUpdateTemplate_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorUpdateTemplate(wine_device_from_handle(params->device)->device, params->descriptorUpdateTemplate, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDescriptorUpdateTemplate(void *args)
{
    struct vkDestroyDescriptorUpdateTemplate_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorUpdateTemplate(wine_device_from_handle(params->device)->device, params->descriptorUpdateTemplate, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDescriptorUpdateTemplateKHR(void *args)
{
    struct vkDestroyDescriptorUpdateTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorUpdateTemplateKHR(wine_device_from_handle(params->device)->device, params->descriptorUpdateTemplate, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDescriptorUpdateTemplateKHR(void *args)
{
    struct vkDestroyDescriptorUpdateTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyDescriptorUpdateTemplateKHR(wine_device_from_handle(params->device)->device, params->descriptorUpdateTemplate, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyDevice(void *args)
{
    struct vkDestroyDevice_params *params = args;
    TRACE("%p, %p\n", params->device, params->pAllocator);

    wine_vkDestroyDevice(params->device, params->pAllocator);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyDevice(void *args)
{
    struct vkDestroyDevice_params *params = args;
    TRACE("%p, %p\n", params->device, params->pAllocator);

    wine_vkDestroyDevice(params->device, params->pAllocator);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyEvent(void *args)
{
    struct vkDestroyEvent_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->event), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyEvent(wine_device_from_handle(params->device)->device, params->event, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyEvent(void *args)
{
    struct vkDestroyEvent_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->event), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyEvent(wine_device_from_handle(params->device)->device, params->event, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyFence(void *args)
{
    struct vkDestroyFence_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->fence), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyFence(wine_device_from_handle(params->device)->device, params->fence, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyFence(void *args)
{
    struct vkDestroyFence_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->fence), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyFence(wine_device_from_handle(params->device)->device, params->fence, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyFramebuffer(void *args)
{
    struct vkDestroyFramebuffer_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->framebuffer), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyFramebuffer(wine_device_from_handle(params->device)->device, params->framebuffer, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyFramebuffer(void *args)
{
    struct vkDestroyFramebuffer_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->framebuffer), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyFramebuffer(wine_device_from_handle(params->device)->device, params->framebuffer, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyImage(void *args)
{
    struct vkDestroyImage_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyImage(wine_device_from_handle(params->device)->device, params->image, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyImage(void *args)
{
    struct vkDestroyImage_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyImage(wine_device_from_handle(params->device)->device, params->image, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyImageView(void *args)
{
    struct vkDestroyImageView_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->imageView), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyImageView(wine_device_from_handle(params->device)->device, params->imageView, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyImageView(void *args)
{
    struct vkDestroyImageView_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->imageView), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyImageView(wine_device_from_handle(params->device)->device, params->imageView, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyIndirectCommandsLayoutNV(void *args)
{
    struct vkDestroyIndirectCommandsLayoutNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->indirectCommandsLayout), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyIndirectCommandsLayoutNV(wine_device_from_handle(params->device)->device, params->indirectCommandsLayout, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyIndirectCommandsLayoutNV(void *args)
{
    struct vkDestroyIndirectCommandsLayoutNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->indirectCommandsLayout), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyIndirectCommandsLayoutNV(wine_device_from_handle(params->device)->device, params->indirectCommandsLayout, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyInstance(void *args)
{
    struct vkDestroyInstance_params *params = args;
    TRACE("%p, %p\n", params->instance, params->pAllocator);

    wine_vkDestroyInstance(params->instance, params->pAllocator);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyInstance(void *args)
{
    struct vkDestroyInstance_params *params = args;
    TRACE("%p, %p\n", params->instance, params->pAllocator);

    wine_vkDestroyInstance(params->instance, params->pAllocator);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyMicromapEXT(void *args)
{
    struct vkDestroyMicromapEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->micromap), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyMicromapEXT(wine_device_from_handle(params->device)->device, params->micromap, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyMicromapEXT(void *args)
{
    struct vkDestroyMicromapEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->micromap), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyMicromapEXT(wine_device_from_handle(params->device)->device, params->micromap, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyOpticalFlowSessionNV(void *args)
{
    struct vkDestroyOpticalFlowSessionNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->session), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyOpticalFlowSessionNV(wine_device_from_handle(params->device)->device, params->session, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyOpticalFlowSessionNV(void *args)
{
    struct vkDestroyOpticalFlowSessionNV_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->session), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyOpticalFlowSessionNV(wine_device_from_handle(params->device)->device, params->session, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyPipeline(void *args)
{
    struct vkDestroyPipeline_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPipeline(wine_device_from_handle(params->device)->device, params->pipeline, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyPipeline(void *args)
{
    struct vkDestroyPipeline_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPipeline(wine_device_from_handle(params->device)->device, params->pipeline, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyPipelineCache(void *args)
{
    struct vkDestroyPipelineCache_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPipelineCache(wine_device_from_handle(params->device)->device, params->pipelineCache, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyPipelineCache(void *args)
{
    struct vkDestroyPipelineCache_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPipelineCache(wine_device_from_handle(params->device)->device, params->pipelineCache, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyPipelineLayout(void *args)
{
    struct vkDestroyPipelineLayout_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipelineLayout), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPipelineLayout(wine_device_from_handle(params->device)->device, params->pipelineLayout, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyPipelineLayout(void *args)
{
    struct vkDestroyPipelineLayout_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipelineLayout), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPipelineLayout(wine_device_from_handle(params->device)->device, params->pipelineLayout, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyPrivateDataSlot(void *args)
{
    struct vkDestroyPrivateDataSlot_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->privateDataSlot), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPrivateDataSlot(wine_device_from_handle(params->device)->device, params->privateDataSlot, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyPrivateDataSlot(void *args)
{
    struct vkDestroyPrivateDataSlot_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->privateDataSlot), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPrivateDataSlot(wine_device_from_handle(params->device)->device, params->privateDataSlot, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyPrivateDataSlotEXT(void *args)
{
    struct vkDestroyPrivateDataSlotEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->privateDataSlot), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPrivateDataSlotEXT(wine_device_from_handle(params->device)->device, params->privateDataSlot, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyPrivateDataSlotEXT(void *args)
{
    struct vkDestroyPrivateDataSlotEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->privateDataSlot), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyPrivateDataSlotEXT(wine_device_from_handle(params->device)->device, params->privateDataSlot, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyQueryPool(void *args)
{
    struct vkDestroyQueryPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->queryPool), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyQueryPool(wine_device_from_handle(params->device)->device, params->queryPool, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyQueryPool(void *args)
{
    struct vkDestroyQueryPool_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->queryPool), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyQueryPool(wine_device_from_handle(params->device)->device, params->queryPool, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyRenderPass(void *args)
{
    struct vkDestroyRenderPass_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderPass), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyRenderPass(wine_device_from_handle(params->device)->device, params->renderPass, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyRenderPass(void *args)
{
    struct vkDestroyRenderPass_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderPass), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyRenderPass(wine_device_from_handle(params->device)->device, params->renderPass, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroySampler(void *args)
{
    struct vkDestroySampler_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->sampler), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySampler(wine_device_from_handle(params->device)->device, params->sampler, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroySampler(void *args)
{
    struct vkDestroySampler_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->sampler), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySampler(wine_device_from_handle(params->device)->device, params->sampler, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroySamplerYcbcrConversion(void *args)
{
    struct vkDestroySamplerYcbcrConversion_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->ycbcrConversion), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySamplerYcbcrConversion(wine_device_from_handle(params->device)->device, params->ycbcrConversion, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroySamplerYcbcrConversion(void *args)
{
    struct vkDestroySamplerYcbcrConversion_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->ycbcrConversion), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySamplerYcbcrConversion(wine_device_from_handle(params->device)->device, params->ycbcrConversion, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroySamplerYcbcrConversionKHR(void *args)
{
    struct vkDestroySamplerYcbcrConversionKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->ycbcrConversion), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySamplerYcbcrConversionKHR(wine_device_from_handle(params->device)->device, params->ycbcrConversion, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroySamplerYcbcrConversionKHR(void *args)
{
    struct vkDestroySamplerYcbcrConversionKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->ycbcrConversion), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySamplerYcbcrConversionKHR(wine_device_from_handle(params->device)->device, params->ycbcrConversion, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroySemaphore(void *args)
{
    struct vkDestroySemaphore_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySemaphore(wine_device_from_handle(params->device)->device, params->semaphore, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroySemaphore(void *args)
{
    struct vkDestroySemaphore_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySemaphore(wine_device_from_handle(params->device)->device, params->semaphore, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyShaderModule(void *args)
{
    struct vkDestroyShaderModule_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->shaderModule), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyShaderModule(wine_device_from_handle(params->device)->device, params->shaderModule, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyShaderModule(void *args)
{
    struct vkDestroyShaderModule_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->shaderModule), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyShaderModule(wine_device_from_handle(params->device)->device, params->shaderModule, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroySurfaceKHR(void *args)
{
    struct vkDestroySurfaceKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->instance, wine_dbgstr_longlong(params->surface), params->pAllocator);

    wine_vkDestroySurfaceKHR(params->instance, params->surface, params->pAllocator);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroySurfaceKHR(void *args)
{
    struct vkDestroySurfaceKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->instance, wine_dbgstr_longlong(params->surface), params->pAllocator);

    wine_vkDestroySurfaceKHR(params->instance, params->surface, params->pAllocator);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroySwapchainKHR(void *args)
{
    struct vkDestroySwapchainKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySwapchainKHR(wine_device_from_handle(params->device)->device, params->swapchain, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroySwapchainKHR(void *args)
{
    struct vkDestroySwapchainKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroySwapchainKHR(wine_device_from_handle(params->device)->device, params->swapchain, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDestroyValidationCacheEXT(void *args)
{
    struct vkDestroyValidationCacheEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->validationCache), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyValidationCacheEXT(wine_device_from_handle(params->device)->device, params->validationCache, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDestroyValidationCacheEXT(void *args)
{
    struct vkDestroyValidationCacheEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->validationCache), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkDestroyValidationCacheEXT(wine_device_from_handle(params->device)->device, params->validationCache, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkDeviceWaitIdle(void *args)
{
    struct vkDeviceWaitIdle_params *params = args;
    TRACE("%p\n", params->device);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkDeviceWaitIdle(wine_device_from_handle(params->device)->device);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkDeviceWaitIdle(void *args)
{
    struct vkDeviceWaitIdle_params *params = args;
    TRACE("%p\n", params->device);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkDeviceWaitIdle(wine_device_from_handle(params->device)->device);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEndCommandBuffer(void *args)
{
    struct vkEndCommandBuffer_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkEndCommandBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEndCommandBuffer(void *args)
{
    struct vkEndCommandBuffer_params *params = args;
    TRACE("%p\n", params->commandBuffer);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkEndCommandBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumerateDeviceExtensionProperties(void *args)
{
    struct vkEnumerateDeviceExtensionProperties_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pLayerName, params->pPropertyCount, params->pProperties);

    params->result = wine_vkEnumerateDeviceExtensionProperties(params->physicalDevice, params->pLayerName, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumerateDeviceExtensionProperties(void *args)
{
    struct vkEnumerateDeviceExtensionProperties_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pLayerName, params->pPropertyCount, params->pProperties);

    params->result = wine_vkEnumerateDeviceExtensionProperties(params->physicalDevice, params->pLayerName, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumerateDeviceLayerProperties(void *args)
{
    struct vkEnumerateDeviceLayerProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPropertyCount, params->pProperties);

    params->result = wine_vkEnumerateDeviceLayerProperties(params->physicalDevice, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumerateDeviceLayerProperties(void *args)
{
    struct vkEnumerateDeviceLayerProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPropertyCount, params->pProperties);

    params->result = wine_vkEnumerateDeviceLayerProperties(params->physicalDevice, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumerateInstanceExtensionProperties(void *args)
{
    struct vkEnumerateInstanceExtensionProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->pLayerName, params->pPropertyCount, params->pProperties);

    params->result = wine_vkEnumerateInstanceExtensionProperties(params->pLayerName, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumerateInstanceExtensionProperties(void *args)
{
    struct vkEnumerateInstanceExtensionProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->pLayerName, params->pPropertyCount, params->pProperties);

    params->result = wine_vkEnumerateInstanceExtensionProperties(params->pLayerName, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumerateInstanceVersion(void *args)
{
    struct vkEnumerateInstanceVersion_params *params = args;
    TRACE("%p\n", params->pApiVersion);

    params->result = wine_vkEnumerateInstanceVersion(params->pApiVersion);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumerateInstanceVersion(void *args)
{
    struct vkEnumerateInstanceVersion_params *params = args;
    TRACE("%p\n", params->pApiVersion);

    params->result = wine_vkEnumerateInstanceVersion(params->pApiVersion);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumeratePhysicalDeviceGroups(void *args)
{
    struct vkEnumeratePhysicalDeviceGroups_params *params = args;
    TRACE("%p, %p, %p\n", params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);

    params->result = wine_vkEnumeratePhysicalDeviceGroups(params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumeratePhysicalDeviceGroups(void *args)
{
    struct vkEnumeratePhysicalDeviceGroups_params *params = args;
    TRACE("%p, %p, %p\n", params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);

    params->result = wine_vkEnumeratePhysicalDeviceGroups(params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumeratePhysicalDeviceGroupsKHR(void *args)
{
    struct vkEnumeratePhysicalDeviceGroupsKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);

    params->result = wine_vkEnumeratePhysicalDeviceGroupsKHR(params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumeratePhysicalDeviceGroupsKHR(void *args)
{
    struct vkEnumeratePhysicalDeviceGroupsKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);

    params->result = wine_vkEnumeratePhysicalDeviceGroupsKHR(params->instance, params->pPhysicalDeviceGroupCount, params->pPhysicalDeviceGroupProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(void *args)
{
    struct vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR_params *params = args;
    TRACE("%p, %u, %p, %p, %p\n", params->physicalDevice, params->queueFamilyIndex, params->pCounterCount, params->pCounters, params->pCounterDescriptions);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->queueFamilyIndex, params->pCounterCount, params->pCounters, params->pCounterDescriptions);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(void *args)
{
    struct vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR_params *params = args;
    TRACE("%p, %u, %p, %p, %p\n", params->physicalDevice, params->queueFamilyIndex, params->pCounterCount, params->pCounters, params->pCounterDescriptions);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->queueFamilyIndex, params->pCounterCount, params->pCounters, params->pCounterDescriptions);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkEnumeratePhysicalDevices(void *args)
{
    struct vkEnumeratePhysicalDevices_params *params = args;
    TRACE("%p, %p, %p\n", params->instance, params->pPhysicalDeviceCount, params->pPhysicalDevices);

    params->result = wine_vkEnumeratePhysicalDevices(params->instance, params->pPhysicalDeviceCount, params->pPhysicalDevices);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkEnumeratePhysicalDevices(void *args)
{
    struct vkEnumeratePhysicalDevices_params *params = args;
    TRACE("%p, %p, %p\n", params->instance, params->pPhysicalDeviceCount, params->pPhysicalDevices);

    params->result = wine_vkEnumeratePhysicalDevices(params->instance, params->pPhysicalDeviceCount, params->pPhysicalDevices);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkFlushMappedMemoryRanges(void *args)
{
    struct vkFlushMappedMemoryRanges_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkFlushMappedMemoryRanges(wine_device_from_handle(params->device)->device, params->memoryRangeCount, params->pMemoryRanges);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkFlushMappedMemoryRanges(void *args)
{
    struct vkFlushMappedMemoryRanges_params *params = args;
    VkMappedMemoryRange_host *pMemoryRanges_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);

    init_conversion_context(&ctx);
    pMemoryRanges_host = convert_VkMappedMemoryRange_array_win32_to_host(&ctx, params->pMemoryRanges, params->memoryRangeCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkFlushMappedMemoryRanges(wine_device_from_handle(params->device)->device, params->memoryRangeCount, pMemoryRanges_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkFreeCommandBuffers(void *args)
{
    struct vkFreeCommandBuffers_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->commandPool), params->commandBufferCount, params->pCommandBuffers);

    wine_vkFreeCommandBuffers(params->device, params->commandPool, params->commandBufferCount, params->pCommandBuffers);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkFreeCommandBuffers(void *args)
{
    struct vkFreeCommandBuffers_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->commandPool), params->commandBufferCount, params->pCommandBuffers);

    wine_vkFreeCommandBuffers(params->device, params->commandPool, params->commandBufferCount, params->pCommandBuffers);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkFreeDescriptorSets(void *args)
{
    struct vkFreeDescriptorSets_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->descriptorSetCount, params->pDescriptorSets);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkFreeDescriptorSets(wine_device_from_handle(params->device)->device, params->descriptorPool, params->descriptorSetCount, params->pDescriptorSets);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkFreeDescriptorSets(void *args)
{
    struct vkFreeDescriptorSets_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->descriptorSetCount, params->pDescriptorSets);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkFreeDescriptorSets(wine_device_from_handle(params->device)->device, params->descriptorPool, params->descriptorSetCount, params->pDescriptorSets);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkFreeMemory(void *args)
{
    struct vkFreeMemory_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->memory), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkFreeMemory(wine_device_from_handle(params->device)->device, params->memory, NULL);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkFreeMemory(void *args)
{
    struct vkFreeMemory_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->memory), params->pAllocator);

    wine_device_from_handle(params->device)->funcs.p_vkFreeMemory(wine_device_from_handle(params->device)->device, params->memory, NULL);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetAccelerationStructureBuildSizesKHR(void *args)
{
    struct vkGetAccelerationStructureBuildSizesKHR_params *params = args;
    TRACE("%p, %#x, %p, %p, %p\n", params->device, params->buildType, params->pBuildInfo, params->pMaxPrimitiveCounts, params->pSizeInfo);

    wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureBuildSizesKHR(wine_device_from_handle(params->device)->device, params->buildType, params->pBuildInfo, params->pMaxPrimitiveCounts, params->pSizeInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetAccelerationStructureBuildSizesKHR(void *args)
{
    struct vkGetAccelerationStructureBuildSizesKHR_params *params = args;
    VkAccelerationStructureBuildGeometryInfoKHR_host pBuildInfo_host;
    VkAccelerationStructureBuildSizesInfoKHR_host pSizeInfo_host;
    TRACE("%p, %#x, %p, %p, %p\n", params->device, params->buildType, params->pBuildInfo, params->pMaxPrimitiveCounts, params->pSizeInfo);

    convert_VkAccelerationStructureBuildGeometryInfoKHR_win32_to_host(params->pBuildInfo, &pBuildInfo_host);
    convert_VkAccelerationStructureBuildSizesInfoKHR_win32_to_host(params->pSizeInfo, &pSizeInfo_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureBuildSizesKHR(wine_device_from_handle(params->device)->device, params->buildType, &pBuildInfo_host, params->pMaxPrimitiveCounts, &pSizeInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetAccelerationStructureDeviceAddressKHR(void *args)
{
    struct vkGetAccelerationStructureDeviceAddressKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureDeviceAddressKHR(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetAccelerationStructureDeviceAddressKHR(void *args)
{
    struct vkGetAccelerationStructureDeviceAddressKHR_params *params = args;
    VkAccelerationStructureDeviceAddressInfoKHR_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkAccelerationStructureDeviceAddressInfoKHR_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureDeviceAddressKHR(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetAccelerationStructureHandleNV(void *args)
{
    struct vkGetAccelerationStructureHandleNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureHandleNV(wine_device_from_handle(params->device)->device, params->accelerationStructure, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetAccelerationStructureHandleNV(void *args)
{
    struct vkGetAccelerationStructureHandleNV_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->accelerationStructure), wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureHandleNV(wine_device_from_handle(params->device)->device, params->accelerationStructure, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetAccelerationStructureMemoryRequirementsNV(void *args)
{
    struct vkGetAccelerationStructureMemoryRequirementsNV_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureMemoryRequirementsNV(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetAccelerationStructureMemoryRequirementsNV(void *args)
{
    struct vkGetAccelerationStructureMemoryRequirementsNV_params *params = args;
    VkAccelerationStructureMemoryRequirementsInfoNV_host pInfo_host;
    VkMemoryRequirements2KHR_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkAccelerationStructureMemoryRequirementsInfoNV_win32_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2KHR_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetAccelerationStructureMemoryRequirementsNV(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2KHR_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferDeviceAddress(void *args)
{
    struct vkGetBufferDeviceAddress_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferDeviceAddress(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferDeviceAddress(void *args)
{
    struct vkGetBufferDeviceAddress_params *params = args;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferDeviceAddress(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferDeviceAddressEXT(void *args)
{
    struct vkGetBufferDeviceAddressEXT_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferDeviceAddressEXT(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferDeviceAddressEXT(void *args)
{
    struct vkGetBufferDeviceAddressEXT_params *params = args;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferDeviceAddressEXT(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferDeviceAddressKHR(void *args)
{
    struct vkGetBufferDeviceAddressKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferDeviceAddressKHR(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferDeviceAddressKHR(void *args)
{
    struct vkGetBufferDeviceAddressKHR_params *params = args;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferDeviceAddressKHR(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferMemoryRequirements(void *args)
{
    struct vkGetBufferMemoryRequirements_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->buffer), params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetBufferMemoryRequirements(wine_device_from_handle(params->device)->device, params->buffer, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferMemoryRequirements(void *args)
{
    struct vkGetBufferMemoryRequirements_params *params = args;
    VkMemoryRequirements_host pMemoryRequirements_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->buffer), params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetBufferMemoryRequirements(wine_device_from_handle(params->device)->device, params->buffer, &pMemoryRequirements_host);
    convert_VkMemoryRequirements_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferMemoryRequirements2(void *args)
{
    struct vkGetBufferMemoryRequirements2_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetBufferMemoryRequirements2(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferMemoryRequirements2(void *args)
{
    struct vkGetBufferMemoryRequirements2_params *params = args;
    VkBufferMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkBufferMemoryRequirementsInfo2_win32_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetBufferMemoryRequirements2(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferMemoryRequirements2KHR(void *args)
{
    struct vkGetBufferMemoryRequirements2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetBufferMemoryRequirements2KHR(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferMemoryRequirements2KHR(void *args)
{
    struct vkGetBufferMemoryRequirements2KHR_params *params = args;
    VkBufferMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkBufferMemoryRequirementsInfo2_win32_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetBufferMemoryRequirements2KHR(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferOpaqueCaptureAddress(void *args)
{
    struct vkGetBufferOpaqueCaptureAddress_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferOpaqueCaptureAddress(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferOpaqueCaptureAddress(void *args)
{
    struct vkGetBufferOpaqueCaptureAddress_params *params = args;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferOpaqueCaptureAddress(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetBufferOpaqueCaptureAddressKHR(void *args)
{
    struct vkGetBufferOpaqueCaptureAddressKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferOpaqueCaptureAddressKHR(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetBufferOpaqueCaptureAddressKHR(void *args)
{
    struct vkGetBufferOpaqueCaptureAddressKHR_params *params = args;
    VkBufferDeviceAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkBufferDeviceAddressInfo_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetBufferOpaqueCaptureAddressKHR(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetCalibratedTimestampsEXT(void *args)
{
    struct vkGetCalibratedTimestampsEXT_params *params = args;
    TRACE("%p, %u, %p, %p, %p\n", params->device, params->timestampCount, params->pTimestampInfos, params->pTimestamps, params->pMaxDeviation);

    params->result = wine_vkGetCalibratedTimestampsEXT(params->device, params->timestampCount, params->pTimestampInfos, params->pTimestamps, params->pMaxDeviation);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetCalibratedTimestampsEXT(void *args)
{
    struct vkGetCalibratedTimestampsEXT_params *params = args;
    TRACE("%p, %u, %p, %p, %p\n", params->device, params->timestampCount, params->pTimestampInfos, params->pTimestamps, params->pMaxDeviation);

    params->result = wine_vkGetCalibratedTimestampsEXT(params->device, params->timestampCount, params->pTimestampInfos, params->pTimestamps, params->pMaxDeviation);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeferredOperationMaxConcurrencyKHR(void *args)
{
    struct vkGetDeferredOperationMaxConcurrencyKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeferredOperationMaxConcurrencyKHR(wine_device_from_handle(params->device)->device, params->operation);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeferredOperationMaxConcurrencyKHR(void *args)
{
    struct vkGetDeferredOperationMaxConcurrencyKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeferredOperationMaxConcurrencyKHR(wine_device_from_handle(params->device)->device, params->operation);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeferredOperationResultKHR(void *args)
{
    struct vkGetDeferredOperationResultKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeferredOperationResultKHR(wine_device_from_handle(params->device)->device, params->operation);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeferredOperationResultKHR(void *args)
{
    struct vkGetDeferredOperationResultKHR_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->operation));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeferredOperationResultKHR(wine_device_from_handle(params->device)->device, params->operation);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDescriptorSetHostMappingVALVE(void *args)
{
    struct vkGetDescriptorSetHostMappingVALVE_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), params->ppData);

    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetHostMappingVALVE(wine_device_from_handle(params->device)->device, params->descriptorSet, params->ppData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDescriptorSetHostMappingVALVE(void *args)
{
    struct vkGetDescriptorSetHostMappingVALVE_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), params->ppData);

    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetHostMappingVALVE(wine_device_from_handle(params->device)->device, params->descriptorSet, params->ppData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDescriptorSetLayoutHostMappingInfoVALVE(void *args)
{
    struct vkGetDescriptorSetLayoutHostMappingInfoVALVE_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pBindingReference, params->pHostMapping);

    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetLayoutHostMappingInfoVALVE(wine_device_from_handle(params->device)->device, params->pBindingReference, params->pHostMapping);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDescriptorSetLayoutHostMappingInfoVALVE(void *args)
{
    struct vkGetDescriptorSetLayoutHostMappingInfoVALVE_params *params = args;
    VkDescriptorSetBindingReferenceVALVE_host pBindingReference_host;
    TRACE("%p, %p, %p\n", params->device, params->pBindingReference, params->pHostMapping);

    convert_VkDescriptorSetBindingReferenceVALVE_win32_to_host(params->pBindingReference, &pBindingReference_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetLayoutHostMappingInfoVALVE(wine_device_from_handle(params->device)->device, &pBindingReference_host, params->pHostMapping);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDescriptorSetLayoutSupport(void *args)
{
    struct vkGetDescriptorSetLayoutSupport_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pSupport);

    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetLayoutSupport(wine_device_from_handle(params->device)->device, params->pCreateInfo, params->pSupport);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDescriptorSetLayoutSupport(void *args)
{
    struct vkGetDescriptorSetLayoutSupport_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pSupport);

    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetLayoutSupport(wine_device_from_handle(params->device)->device, params->pCreateInfo, params->pSupport);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDescriptorSetLayoutSupportKHR(void *args)
{
    struct vkGetDescriptorSetLayoutSupportKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pSupport);

    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetLayoutSupportKHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, params->pSupport);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDescriptorSetLayoutSupportKHR(void *args)
{
    struct vkGetDescriptorSetLayoutSupportKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pSupport);

    wine_device_from_handle(params->device)->funcs.p_vkGetDescriptorSetLayoutSupportKHR(wine_device_from_handle(params->device)->device, params->pCreateInfo, params->pSupport);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceAccelerationStructureCompatibilityKHR(void *args)
{
    struct vkGetDeviceAccelerationStructureCompatibilityKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pVersionInfo, params->pCompatibility);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceAccelerationStructureCompatibilityKHR(wine_device_from_handle(params->device)->device, params->pVersionInfo, params->pCompatibility);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceAccelerationStructureCompatibilityKHR(void *args)
{
    struct vkGetDeviceAccelerationStructureCompatibilityKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pVersionInfo, params->pCompatibility);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceAccelerationStructureCompatibilityKHR(wine_device_from_handle(params->device)->device, params->pVersionInfo, params->pCompatibility);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceBufferMemoryRequirements(void *args)
{
    struct vkGetDeviceBufferMemoryRequirements_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceBufferMemoryRequirements(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceBufferMemoryRequirements(void *args)
{
    struct vkGetDeviceBufferMemoryRequirements_params *params = args;
    VkDeviceBufferMemoryRequirements_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    struct conversion_context ctx;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    init_conversion_context(&ctx);
    convert_VkDeviceBufferMemoryRequirements_win32_to_host(&ctx, params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceBufferMemoryRequirements(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceBufferMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceBufferMemoryRequirementsKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceBufferMemoryRequirementsKHR(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceBufferMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceBufferMemoryRequirementsKHR_params *params = args;
    VkDeviceBufferMemoryRequirements_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    struct conversion_context ctx;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    init_conversion_context(&ctx);
    convert_VkDeviceBufferMemoryRequirements_win32_to_host(&ctx, params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceBufferMemoryRequirementsKHR(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceFaultInfoEXT(void *args)
{
    struct vkGetDeviceFaultInfoEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pFaultCounts, params->pFaultInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceFaultInfoEXT(wine_device_from_handle(params->device)->device, params->pFaultCounts, params->pFaultInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceFaultInfoEXT(void *args)
{
    struct vkGetDeviceFaultInfoEXT_params *params = args;
    VkDeviceFaultCountsEXT_host pFaultCounts_host;
    VkDeviceFaultInfoEXT_host pFaultInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p, %p\n", params->device, params->pFaultCounts, params->pFaultInfo);

    init_conversion_context(&ctx);
    convert_VkDeviceFaultCountsEXT_win32_to_host(params->pFaultCounts, &pFaultCounts_host);
    convert_VkDeviceFaultInfoEXT_win32_to_host(&ctx, params->pFaultInfo, &pFaultInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceFaultInfoEXT(wine_device_from_handle(params->device)->device, &pFaultCounts_host, &pFaultInfo_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceGroupPeerMemoryFeatures(void *args)
{
    struct vkGetDeviceGroupPeerMemoryFeatures_params *params = args;
    TRACE("%p, %u, %u, %u, %p\n", params->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupPeerMemoryFeatures(wine_device_from_handle(params->device)->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceGroupPeerMemoryFeatures(void *args)
{
    struct vkGetDeviceGroupPeerMemoryFeatures_params *params = args;
    TRACE("%p, %u, %u, %u, %p\n", params->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupPeerMemoryFeatures(wine_device_from_handle(params->device)->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceGroupPeerMemoryFeaturesKHR(void *args)
{
    struct vkGetDeviceGroupPeerMemoryFeaturesKHR_params *params = args;
    TRACE("%p, %u, %u, %u, %p\n", params->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupPeerMemoryFeaturesKHR(wine_device_from_handle(params->device)->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceGroupPeerMemoryFeaturesKHR(void *args)
{
    struct vkGetDeviceGroupPeerMemoryFeaturesKHR_params *params = args;
    TRACE("%p, %u, %u, %u, %p\n", params->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupPeerMemoryFeaturesKHR(wine_device_from_handle(params->device)->device, params->heapIndex, params->localDeviceIndex, params->remoteDeviceIndex, params->pPeerMemoryFeatures);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceGroupPresentCapabilitiesKHR(void *args)
{
    struct vkGetDeviceGroupPresentCapabilitiesKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pDeviceGroupPresentCapabilities);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupPresentCapabilitiesKHR(wine_device_from_handle(params->device)->device, params->pDeviceGroupPresentCapabilities);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceGroupPresentCapabilitiesKHR(void *args)
{
    struct vkGetDeviceGroupPresentCapabilitiesKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pDeviceGroupPresentCapabilities);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupPresentCapabilitiesKHR(wine_device_from_handle(params->device)->device, params->pDeviceGroupPresentCapabilities);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceGroupSurfacePresentModesKHR(void *args)
{
    struct vkGetDeviceGroupSurfacePresentModesKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->surface), params->pModes);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupSurfacePresentModesKHR(wine_device_from_handle(params->device)->device, wine_surface_from_handle(params->surface)->driver_surface, params->pModes);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceGroupSurfacePresentModesKHR(void *args)
{
    struct vkGetDeviceGroupSurfacePresentModesKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->surface), params->pModes);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceGroupSurfacePresentModesKHR(wine_device_from_handle(params->device)->device, wine_surface_from_handle(params->surface)->driver_surface, params->pModes);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceImageMemoryRequirements(void *args)
{
    struct vkGetDeviceImageMemoryRequirements_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageMemoryRequirements(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceImageMemoryRequirements(void *args)
{
    struct vkGetDeviceImageMemoryRequirements_params *params = args;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageMemoryRequirements(wine_device_from_handle(params->device)->device, params->pInfo, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceImageMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceImageMemoryRequirementsKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageMemoryRequirementsKHR(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceImageMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceImageMemoryRequirementsKHR_params *params = args;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageMemoryRequirementsKHR(wine_device_from_handle(params->device)->device, params->pInfo, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceImageSparseMemoryRequirements(void *args)
{
    struct vkGetDeviceImageSparseMemoryRequirements_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageSparseMemoryRequirements(wine_device_from_handle(params->device)->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceImageSparseMemoryRequirements(void *args)
{
    struct vkGetDeviceImageSparseMemoryRequirements_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageSparseMemoryRequirements(wine_device_from_handle(params->device)->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceImageSparseMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceImageSparseMemoryRequirementsKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageSparseMemoryRequirementsKHR(wine_device_from_handle(params->device)->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceImageSparseMemoryRequirementsKHR(void *args)
{
    struct vkGetDeviceImageSparseMemoryRequirementsKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceImageSparseMemoryRequirementsKHR(wine_device_from_handle(params->device)->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceMemoryCommitment(void *args)
{
    struct vkGetDeviceMemoryCommitment_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->memory), params->pCommittedMemoryInBytes);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMemoryCommitment(wine_device_from_handle(params->device)->device, params->memory, params->pCommittedMemoryInBytes);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceMemoryCommitment(void *args)
{
    struct vkGetDeviceMemoryCommitment_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->memory), params->pCommittedMemoryInBytes);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMemoryCommitment(wine_device_from_handle(params->device)->device, params->memory, params->pCommittedMemoryInBytes);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceMemoryOpaqueCaptureAddress(void *args)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddress_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddress(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceMemoryOpaqueCaptureAddress(void *args)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddress_params *params = args;
    VkDeviceMemoryOpaqueCaptureAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddress(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceMemoryOpaqueCaptureAddressKHR(void *args)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddressKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceMemoryOpaqueCaptureAddressKHR(void *args)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddressKHR_params *params = args;
    VkDeviceMemoryOpaqueCaptureAddressInfo_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkDeviceMemoryOpaqueCaptureAddressInfo_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMemoryOpaqueCaptureAddressKHR(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceMicromapCompatibilityEXT(void *args)
{
    struct vkGetDeviceMicromapCompatibilityEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pVersionInfo, params->pCompatibility);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMicromapCompatibilityEXT(wine_device_from_handle(params->device)->device, params->pVersionInfo, params->pCompatibility);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceMicromapCompatibilityEXT(void *args)
{
    struct vkGetDeviceMicromapCompatibilityEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pVersionInfo, params->pCompatibility);

    wine_device_from_handle(params->device)->funcs.p_vkGetDeviceMicromapCompatibilityEXT(wine_device_from_handle(params->device)->device, params->pVersionInfo, params->pCompatibility);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceQueue(void *args)
{
    struct vkGetDeviceQueue_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->device, params->queueFamilyIndex, params->queueIndex, params->pQueue);

    wine_vkGetDeviceQueue(params->device, params->queueFamilyIndex, params->queueIndex, params->pQueue);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceQueue(void *args)
{
    struct vkGetDeviceQueue_params *params = args;
    TRACE("%p, %u, %u, %p\n", params->device, params->queueFamilyIndex, params->queueIndex, params->pQueue);

    wine_vkGetDeviceQueue(params->device, params->queueFamilyIndex, params->queueIndex, params->pQueue);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceQueue2(void *args)
{
    struct vkGetDeviceQueue2_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pQueueInfo, params->pQueue);

    wine_vkGetDeviceQueue2(params->device, params->pQueueInfo, params->pQueue);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceQueue2(void *args)
{
    struct vkGetDeviceQueue2_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pQueueInfo, params->pQueue);

    wine_vkGetDeviceQueue2(params->device, params->pQueueInfo, params->pQueue);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(void *args)
{
    struct vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderpass), params->pMaxWorkgroupSize);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(wine_device_from_handle(params->device)->device, params->renderpass, params->pMaxWorkgroupSize);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(void *args)
{
    struct vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderpass), params->pMaxWorkgroupSize);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(wine_device_from_handle(params->device)->device, params->renderpass, params->pMaxWorkgroupSize);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetDynamicRenderingTilePropertiesQCOM(void *args)
{
    struct vkGetDynamicRenderingTilePropertiesQCOM_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pRenderingInfo, params->pProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDynamicRenderingTilePropertiesQCOM(wine_device_from_handle(params->device)->device, params->pRenderingInfo, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetDynamicRenderingTilePropertiesQCOM(void *args)
{
    struct vkGetDynamicRenderingTilePropertiesQCOM_params *params = args;
    VkRenderingInfo_host pRenderingInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %p, %p\n", params->device, params->pRenderingInfo, params->pProperties);

    init_conversion_context(&ctx);
    convert_VkRenderingInfo_win32_to_host(&ctx, params->pRenderingInfo, &pRenderingInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetDynamicRenderingTilePropertiesQCOM(wine_device_from_handle(params->device)->device, &pRenderingInfo_host, params->pProperties);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetEventStatus(void *args)
{
    struct vkGetEventStatus_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetEventStatus(wine_device_from_handle(params->device)->device, params->event);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetEventStatus(void *args)
{
    struct vkGetEventStatus_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetEventStatus(wine_device_from_handle(params->device)->device, params->event);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetFenceStatus(void *args)
{
    struct vkGetFenceStatus_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->fence));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetFenceStatus(wine_device_from_handle(params->device)->device, params->fence);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetFenceStatus(void *args)
{
    struct vkGetFenceStatus_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->fence));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetFenceStatus(wine_device_from_handle(params->device)->device, params->fence);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetFramebufferTilePropertiesQCOM(void *args)
{
    struct vkGetFramebufferTilePropertiesQCOM_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->framebuffer), params->pPropertiesCount, params->pProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetFramebufferTilePropertiesQCOM(wine_device_from_handle(params->device)->device, params->framebuffer, params->pPropertiesCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetFramebufferTilePropertiesQCOM(void *args)
{
    struct vkGetFramebufferTilePropertiesQCOM_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->framebuffer), params->pPropertiesCount, params->pProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetFramebufferTilePropertiesQCOM(wine_device_from_handle(params->device)->device, params->framebuffer, params->pPropertiesCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetGeneratedCommandsMemoryRequirementsNV(void *args)
{
    struct vkGetGeneratedCommandsMemoryRequirementsNV_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetGeneratedCommandsMemoryRequirementsNV(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetGeneratedCommandsMemoryRequirementsNV(void *args)
{
    struct vkGetGeneratedCommandsMemoryRequirementsNV_params *params = args;
    VkGeneratedCommandsMemoryRequirementsInfoNV_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkGeneratedCommandsMemoryRequirementsInfoNV_win32_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetGeneratedCommandsMemoryRequirementsNV(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageMemoryRequirements(void *args)
{
    struct vkGetImageMemoryRequirements_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageMemoryRequirements(wine_device_from_handle(params->device)->device, params->image, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageMemoryRequirements(void *args)
{
    struct vkGetImageMemoryRequirements_params *params = args;
    VkMemoryRequirements_host pMemoryRequirements_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageMemoryRequirements(wine_device_from_handle(params->device)->device, params->image, &pMemoryRequirements_host);
    convert_VkMemoryRequirements_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageMemoryRequirements2(void *args)
{
    struct vkGetImageMemoryRequirements2_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageMemoryRequirements2(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageMemoryRequirements2(void *args)
{
    struct vkGetImageMemoryRequirements2_params *params = args;
    VkImageMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkImageMemoryRequirementsInfo2_win32_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetImageMemoryRequirements2(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageMemoryRequirements2KHR(void *args)
{
    struct vkGetImageMemoryRequirements2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageMemoryRequirements2KHR(wine_device_from_handle(params->device)->device, params->pInfo, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageMemoryRequirements2KHR(void *args)
{
    struct vkGetImageMemoryRequirements2KHR_params *params = args;
    VkImageMemoryRequirementsInfo2_host pInfo_host;
    VkMemoryRequirements2_host pMemoryRequirements_host;
    TRACE("%p, %p, %p\n", params->device, params->pInfo, params->pMemoryRequirements);

    convert_VkImageMemoryRequirementsInfo2_win32_to_host(params->pInfo, &pInfo_host);
    convert_VkMemoryRequirements2_win32_to_host(params->pMemoryRequirements, &pMemoryRequirements_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetImageMemoryRequirements2KHR(wine_device_from_handle(params->device)->device, &pInfo_host, &pMemoryRequirements_host);
    convert_VkMemoryRequirements2_host_to_win32(&pMemoryRequirements_host, params->pMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageSparseMemoryRequirements(void *args)
{
    struct vkGetImageSparseMemoryRequirements_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageSparseMemoryRequirements(wine_device_from_handle(params->device)->device, params->image, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageSparseMemoryRequirements(void *args)
{
    struct vkGetImageSparseMemoryRequirements_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageSparseMemoryRequirements(wine_device_from_handle(params->device)->device, params->image, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageSparseMemoryRequirements2(void *args)
{
    struct vkGetImageSparseMemoryRequirements2_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageSparseMemoryRequirements2(wine_device_from_handle(params->device)->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageSparseMemoryRequirements2(void *args)
{
    struct vkGetImageSparseMemoryRequirements2_params *params = args;
    VkImageSparseMemoryRequirementsInfo2_host pInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    convert_VkImageSparseMemoryRequirementsInfo2_win32_to_host(params->pInfo, &pInfo_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetImageSparseMemoryRequirements2(wine_device_from_handle(params->device)->device, &pInfo_host, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageSparseMemoryRequirements2KHR(void *args)
{
    struct vkGetImageSparseMemoryRequirements2KHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageSparseMemoryRequirements2KHR(wine_device_from_handle(params->device)->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageSparseMemoryRequirements2KHR(void *args)
{
    struct vkGetImageSparseMemoryRequirements2KHR_params *params = args;
    VkImageSparseMemoryRequirementsInfo2_host pInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pInfo, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);

    convert_VkImageSparseMemoryRequirementsInfo2_win32_to_host(params->pInfo, &pInfo_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetImageSparseMemoryRequirements2KHR(wine_device_from_handle(params->device)->device, &pInfo_host, params->pSparseMemoryRequirementCount, params->pSparseMemoryRequirements);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageSubresourceLayout(void *args)
{
    struct vkGetImageSubresourceLayout_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageSubresourceLayout(wine_device_from_handle(params->device)->device, params->image, params->pSubresource, params->pLayout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageSubresourceLayout(void *args)
{
    struct vkGetImageSubresourceLayout_params *params = args;
    VkSubresourceLayout_host pLayout_host;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageSubresourceLayout(wine_device_from_handle(params->device)->device, params->image, params->pSubresource, &pLayout_host);
    convert_VkSubresourceLayout_host_to_win32(&pLayout_host, params->pLayout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageSubresourceLayout2EXT(void *args)
{
    struct vkGetImageSubresourceLayout2EXT_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);

    wine_device_from_handle(params->device)->funcs.p_vkGetImageSubresourceLayout2EXT(wine_device_from_handle(params->device)->device, params->image, params->pSubresource, params->pLayout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageSubresourceLayout2EXT(void *args)
{
    struct vkGetImageSubresourceLayout2EXT_params *params = args;
    VkSubresourceLayout2EXT_host pLayout_host;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->image), params->pSubresource, params->pLayout);

    convert_VkSubresourceLayout2EXT_win32_to_host(params->pLayout, &pLayout_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetImageSubresourceLayout2EXT(wine_device_from_handle(params->device)->device, params->image, params->pSubresource, &pLayout_host);
    convert_VkSubresourceLayout2EXT_host_to_win32(&pLayout_host, params->pLayout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageViewAddressNVX(void *args)
{
    struct vkGetImageViewAddressNVX_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->imageView), params->pProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetImageViewAddressNVX(wine_device_from_handle(params->device)->device, params->imageView, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageViewAddressNVX(void *args)
{
    struct vkGetImageViewAddressNVX_params *params = args;
    VkImageViewAddressPropertiesNVX_host pProperties_host;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->imageView), params->pProperties);

    convert_VkImageViewAddressPropertiesNVX_win32_to_host(params->pProperties, &pProperties_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetImageViewAddressNVX(wine_device_from_handle(params->device)->device, params->imageView, &pProperties_host);
    convert_VkImageViewAddressPropertiesNVX_host_to_win32(&pProperties_host, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetImageViewHandleNVX(void *args)
{
    struct vkGetImageViewHandleNVX_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetImageViewHandleNVX(wine_device_from_handle(params->device)->device, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetImageViewHandleNVX(void *args)
{
    struct vkGetImageViewHandleNVX_params *params = args;
    VkImageViewHandleInfoNVX_host pInfo_host;
    TRACE("%p, %p\n", params->device, params->pInfo);

    convert_VkImageViewHandleInfoNVX_win32_to_host(params->pInfo, &pInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetImageViewHandleNVX(wine_device_from_handle(params->device)->device, &pInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetMemoryHostPointerPropertiesEXT(void *args)
{
    struct vkGetMemoryHostPointerPropertiesEXT_params *params = args;
    TRACE("%p, %#x, %p, %p\n", params->device, params->handleType, params->pHostPointer, params->pMemoryHostPointerProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetMemoryHostPointerPropertiesEXT(wine_device_from_handle(params->device)->device, params->handleType, params->pHostPointer, params->pMemoryHostPointerProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetMemoryHostPointerPropertiesEXT(void *args)
{
    struct vkGetMemoryHostPointerPropertiesEXT_params *params = args;
    TRACE("%p, %#x, %p, %p\n", params->device, params->handleType, params->pHostPointer, params->pMemoryHostPointerProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetMemoryHostPointerPropertiesEXT(wine_device_from_handle(params->device)->device, params->handleType, params->pHostPointer, params->pMemoryHostPointerProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetMicromapBuildSizesEXT(void *args)
{
    struct vkGetMicromapBuildSizesEXT_params *params = args;
    TRACE("%p, %#x, %p, %p\n", params->device, params->buildType, params->pBuildInfo, params->pSizeInfo);

    wine_device_from_handle(params->device)->funcs.p_vkGetMicromapBuildSizesEXT(wine_device_from_handle(params->device)->device, params->buildType, params->pBuildInfo, params->pSizeInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetMicromapBuildSizesEXT(void *args)
{
    struct vkGetMicromapBuildSizesEXT_params *params = args;
    VkMicromapBuildInfoEXT_host pBuildInfo_host;
    VkMicromapBuildSizesInfoEXT_host pSizeInfo_host;
    TRACE("%p, %#x, %p, %p\n", params->device, params->buildType, params->pBuildInfo, params->pSizeInfo);

    convert_VkMicromapBuildInfoEXT_win32_to_host(params->pBuildInfo, &pBuildInfo_host);
    convert_VkMicromapBuildSizesInfoEXT_win32_to_host(params->pSizeInfo, &pSizeInfo_host);
    wine_device_from_handle(params->device)->funcs.p_vkGetMicromapBuildSizesEXT(wine_device_from_handle(params->device)->device, params->buildType, &pBuildInfo_host, &pSizeInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPerformanceParameterINTEL(void *args)
{
    struct vkGetPerformanceParameterINTEL_params *params = args;
    TRACE("%p, %#x, %p\n", params->device, params->parameter, params->pValue);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPerformanceParameterINTEL(wine_device_from_handle(params->device)->device, params->parameter, params->pValue);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPerformanceParameterINTEL(void *args)
{
    struct vkGetPerformanceParameterINTEL_params *params = args;
    TRACE("%p, %#x, %p\n", params->device, params->parameter, params->pValue);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPerformanceParameterINTEL(wine_device_from_handle(params->device)->device, params->parameter, params->pValue);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(void *args)
{
    struct vkGetPhysicalDeviceCalibrateableTimeDomainsEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pTimeDomainCount, params->pTimeDomains);

    params->result = wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(params->physicalDevice, params->pTimeDomainCount, params->pTimeDomains);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(void *args)
{
    struct vkGetPhysicalDeviceCalibrateableTimeDomainsEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pTimeDomainCount, params->pTimeDomains);

    params->result = wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(params->physicalDevice, params->pTimeDomainCount, params->pTimeDomains);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(void *args)
{
    struct vkGetPhysicalDeviceCooperativeMatrixPropertiesNV_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPropertyCount, params->pProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(void *args)
{
    struct vkGetPhysicalDeviceCooperativeMatrixPropertiesNV_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPropertyCount, params->pProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceExternalBufferProperties(void *args)
{
    struct vkGetPhysicalDeviceExternalBufferProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);

    wine_vkGetPhysicalDeviceExternalBufferProperties(params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceExternalBufferProperties(void *args)
{
    struct vkGetPhysicalDeviceExternalBufferProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);

    wine_vkGetPhysicalDeviceExternalBufferProperties(params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceExternalBufferPropertiesKHR(void *args)
{
    struct vkGetPhysicalDeviceExternalBufferPropertiesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);

    wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR(params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceExternalBufferPropertiesKHR(void *args)
{
    struct vkGetPhysicalDeviceExternalBufferPropertiesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);

    wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR(params->physicalDevice, params->pExternalBufferInfo, params->pExternalBufferProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceExternalFenceProperties(void *args)
{
    struct vkGetPhysicalDeviceExternalFenceProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);

    wine_vkGetPhysicalDeviceExternalFenceProperties(params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceExternalFenceProperties(void *args)
{
    struct vkGetPhysicalDeviceExternalFenceProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);

    wine_vkGetPhysicalDeviceExternalFenceProperties(params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceExternalFencePropertiesKHR(void *args)
{
    struct vkGetPhysicalDeviceExternalFencePropertiesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);

    wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceExternalFencePropertiesKHR(void *args)
{
    struct vkGetPhysicalDeviceExternalFencePropertiesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);

    wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(params->physicalDevice, params->pExternalFenceInfo, params->pExternalFenceProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceExternalSemaphoreProperties(void *args)
{
    struct vkGetPhysicalDeviceExternalSemaphoreProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);

    wine_vkGetPhysicalDeviceExternalSemaphoreProperties(params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceExternalSemaphoreProperties(void *args)
{
    struct vkGetPhysicalDeviceExternalSemaphoreProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);

    wine_vkGetPhysicalDeviceExternalSemaphoreProperties(params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(void *args)
{
    struct vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);

    wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(void *args)
{
    struct vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);

    wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(params->physicalDevice, params->pExternalSemaphoreInfo, params->pExternalSemaphoreProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceFeatures(void *args)
{
    struct vkGetPhysicalDeviceFeatures_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFeatures(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceFeatures(void *args)
{
    struct vkGetPhysicalDeviceFeatures_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFeatures(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceFeatures2(void *args)
{
    struct vkGetPhysicalDeviceFeatures2_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFeatures2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceFeatures2(void *args)
{
    struct vkGetPhysicalDeviceFeatures2_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFeatures2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceFeatures2KHR(void *args)
{
    struct vkGetPhysicalDeviceFeatures2KHR_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFeatures2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceFeatures2KHR(void *args)
{
    struct vkGetPhysicalDeviceFeatures2KHR_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pFeatures);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFeatures2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFeatures);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFormatProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFormatProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties2_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties2_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties2KHR_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceFormatProperties2KHR_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->format, params->pFormatProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFormatProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->pFormatProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceFragmentShadingRatesKHR(void *args)
{
    struct vkGetPhysicalDeviceFragmentShadingRatesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pFragmentShadingRateCount, params->pFragmentShadingRates);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFragmentShadingRatesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFragmentShadingRateCount, params->pFragmentShadingRates);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceFragmentShadingRatesKHR(void *args)
{
    struct vkGetPhysicalDeviceFragmentShadingRatesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pFragmentShadingRateCount, params->pFragmentShadingRates);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceFragmentShadingRatesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFragmentShadingRateCount, params->pFragmentShadingRates);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceImageFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceImageFormatProperties_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p\n", params->physicalDevice, params->format, params->type, params->tiling, params->usage, params->flags, params->pImageFormatProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->type, params->tiling, params->usage, params->flags, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceImageFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceImageFormatProperties_params *params = args;
    VkImageFormatProperties_host pImageFormatProperties_host;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p\n", params->physicalDevice, params->format, params->type, params->tiling, params->usage, params->flags, params->pImageFormatProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->type, params->tiling, params->usage, params->flags, &pImageFormatProperties_host);
    convert_VkImageFormatProperties_host_to_win32(&pImageFormatProperties_host, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    VkResult result;
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2(wine_phys_dev_from_handle(physicalDevice)->phys_dev, pImageFormatInfo, pImageFormatProperties);
    return result;
}

static NTSTATUS thunk64_vkGetPhysicalDeviceImageFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceImageFormatProperties2_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);

    params->result = wine_vkGetPhysicalDeviceImageFormatProperties2(params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    VkResult result;
    VkImageFormatProperties2_host pImageFormatProperties_host;
    convert_VkImageFormatProperties2_win32_to_host(pImageFormatProperties, &pImageFormatProperties_host);
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2(wine_phys_dev_from_handle(physicalDevice)->phys_dev, pImageFormatInfo, &pImageFormatProperties_host);
    convert_VkImageFormatProperties2_host_to_win32(&pImageFormatProperties_host, pImageFormatProperties);
    return result;
}

static NTSTATUS thunk32_vkGetPhysicalDeviceImageFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceImageFormatProperties2_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);

    params->result = wine_vkGetPhysicalDeviceImageFormatProperties2(params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    VkResult result;
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2KHR(wine_phys_dev_from_handle(physicalDevice)->phys_dev, pImageFormatInfo, pImageFormatProperties);
    return result;
}

static NTSTATUS thunk64_vkGetPhysicalDeviceImageFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceImageFormatProperties2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);

    params->result = wine_vkGetPhysicalDeviceImageFormatProperties2KHR(params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    VkResult result;
    VkImageFormatProperties2_host pImageFormatProperties_host;
    convert_VkImageFormatProperties2_win32_to_host(pImageFormatProperties, &pImageFormatProperties_host);
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2KHR(wine_phys_dev_from_handle(physicalDevice)->phys_dev, pImageFormatInfo, &pImageFormatProperties_host);
    convert_VkImageFormatProperties2_host_to_win32(&pImageFormatProperties_host, pImageFormatProperties);
    return result;
}

static NTSTATUS thunk32_vkGetPhysicalDeviceImageFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceImageFormatProperties2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);

    params->result = wine_vkGetPhysicalDeviceImageFormatProperties2KHR(params->physicalDevice, params->pImageFormatInfo, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceMemoryProperties(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pMemoryProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceMemoryProperties(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties_params *params = args;
    VkPhysicalDeviceMemoryProperties_host pMemoryProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pMemoryProperties_host);
    convert_VkPhysicalDeviceMemoryProperties_host_to_win32(&pMemoryProperties_host, params->pMemoryProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceMemoryProperties2(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties2_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pMemoryProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceMemoryProperties2(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties2_params *params = args;
    VkPhysicalDeviceMemoryProperties2_host pMemoryProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    convert_VkPhysicalDeviceMemoryProperties2_win32_to_host(params->pMemoryProperties, &pMemoryProperties_host);
    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pMemoryProperties_host);
    convert_VkPhysicalDeviceMemoryProperties2_host_to_win32(&pMemoryProperties_host, params->pMemoryProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceMemoryProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties2KHR_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pMemoryProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceMemoryProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceMemoryProperties2KHR_params *params = args;
    VkPhysicalDeviceMemoryProperties2_host pMemoryProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pMemoryProperties);

    convert_VkPhysicalDeviceMemoryProperties2_win32_to_host(params->pMemoryProperties, &pMemoryProperties_host);
    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMemoryProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pMemoryProperties_host);
    convert_VkPhysicalDeviceMemoryProperties2_host_to_win32(&pMemoryProperties_host, params->pMemoryProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceMultisamplePropertiesEXT(void *args)
{
    struct vkGetPhysicalDeviceMultisamplePropertiesEXT_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->samples, params->pMultisampleProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMultisamplePropertiesEXT(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->samples, params->pMultisampleProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceMultisamplePropertiesEXT(void *args)
{
    struct vkGetPhysicalDeviceMultisamplePropertiesEXT_params *params = args;
    TRACE("%p, %#x, %p\n", params->physicalDevice, params->samples, params->pMultisampleProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceMultisamplePropertiesEXT(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->samples, params->pMultisampleProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceOpticalFlowImageFormatsNV(void *args)
{
    struct vkGetPhysicalDeviceOpticalFlowImageFormatsNV_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pOpticalFlowImageFormatInfo, params->pFormatCount, params->pImageFormatProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceOpticalFlowImageFormatsNV(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pOpticalFlowImageFormatInfo, params->pFormatCount, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceOpticalFlowImageFormatsNV(void *args)
{
    struct vkGetPhysicalDeviceOpticalFlowImageFormatsNV_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pOpticalFlowImageFormatInfo, params->pFormatCount, params->pImageFormatProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceOpticalFlowImageFormatsNV(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pOpticalFlowImageFormatInfo, params->pFormatCount, params->pImageFormatProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDevicePresentRectanglesKHR(void *args)
{
    struct vkGetPhysicalDevicePresentRectanglesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pRectCount, params->pRects);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDevicePresentRectanglesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pRectCount, params->pRects);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDevicePresentRectanglesKHR(void *args)
{
    struct vkGetPhysicalDevicePresentRectanglesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pRectCount, params->pRects);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDevicePresentRectanglesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pRectCount, params->pRects);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceProperties(void *args)
{
    struct vkGetPhysicalDeviceProperties_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceProperties(void *args)
{
    struct vkGetPhysicalDeviceProperties_params *params = args;
    VkPhysicalDeviceProperties_host pProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pProperties_host);
    convert_VkPhysicalDeviceProperties_host_to_win32(&pProperties_host, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceProperties2(void *args)
{
    struct vkGetPhysicalDeviceProperties2_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceProperties2(void *args)
{
    struct vkGetPhysicalDeviceProperties2_params *params = args;
    VkPhysicalDeviceProperties2_host pProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    convert_VkPhysicalDeviceProperties2_win32_to_host(params->pProperties, &pProperties_host);
    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pProperties_host);
    convert_VkPhysicalDeviceProperties2_host_to_win32(&pProperties_host, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceProperties2KHR_params *params = args;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceProperties2KHR_params *params = args;
    VkPhysicalDeviceProperties2_host pProperties_host;
    TRACE("%p, %p\n", params->physicalDevice, params->pProperties);

    convert_VkPhysicalDeviceProperties2_win32_to_host(params->pProperties, &pProperties_host);
    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pProperties_host);
    convert_VkPhysicalDeviceProperties2_host_to_win32(&pProperties_host, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPerformanceQueryCreateInfo, params->pNumPasses);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pPerformanceQueryCreateInfo, params->pNumPasses);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pPerformanceQueryCreateInfo, params->pNumPasses);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pPerformanceQueryCreateInfo, params->pNumPasses);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceQueueFamilyProperties(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceQueueFamilyProperties(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceQueueFamilyProperties2(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceQueueFamilyProperties2(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceQueueFamilyProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceQueueFamilyProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pQueueFamilyPropertyCount, params->pQueueFamilyProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSparseImageFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p, %p\n", params->physicalDevice, params->format, params->type, params->samples, params->usage, params->tiling, params->pPropertyCount, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->type, params->samples, params->usage, params->tiling, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSparseImageFormatProperties(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties_params *params = args;
    TRACE("%p, %#x, %#x, %#x, %#x, %#x, %p, %p\n", params->physicalDevice, params->format, params->type, params->samples, params->usage, params->tiling, params->pPropertyCount, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->format, params->type, params->samples, params->usage, params->tiling, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSparseImageFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pFormatInfo, params->pPropertyCount, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSparseImageFormatProperties2(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pFormatInfo, params->pPropertyCount, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2KHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pFormatInfo, params->pPropertyCount, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(void *args)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2KHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pFormatInfo, params->pPropertyCount, params->pProperties);

    wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pFormatInfo, params->pPropertyCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(void *args)
{
    struct vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pCombinationCount, params->pCombinations);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pCombinationCount, params->pCombinations);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(void *args)
{
    struct vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pCombinationCount, params->pCombinations);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pCombinationCount, params->pCombinations);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR pSurfaceInfo_host;
    convert_VkPhysicalDeviceSurfaceInfo2KHR_win64_to_host(pSurfaceInfo, &pSurfaceInfo_host);
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(wine_phys_dev_from_handle(physicalDevice)->phys_dev, &pSurfaceInfo_host, pSurfaceCapabilities);
    return result;
}

static NTSTATUS thunk64_vkGetPhysicalDeviceSurfaceCapabilities2KHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceCapabilities2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pSurfaceInfo, params->pSurfaceCapabilities);

    params->result = wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(params->physicalDevice, params->pSurfaceInfo, params->pSurfaceCapabilities);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR_host pSurfaceInfo_host;
    convert_VkPhysicalDeviceSurfaceInfo2KHR_win32_to_host(pSurfaceInfo, &pSurfaceInfo_host);
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(wine_phys_dev_from_handle(physicalDevice)->phys_dev, &pSurfaceInfo_host, pSurfaceCapabilities);
    return result;
}

static NTSTATUS thunk32_vkGetPhysicalDeviceSurfaceCapabilities2KHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceCapabilities2KHR_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pSurfaceInfo, params->pSurfaceCapabilities);

    params->result = wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(params->physicalDevice, params->pSurfaceInfo, params->pSurfaceCapabilities);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    VkResult result;
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(wine_phys_dev_from_handle(physicalDevice)->phys_dev, wine_surface_from_handle(surface)->driver_surface, pSurfaceCapabilities);
    return result;
}

static NTSTATUS thunk64_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceCapabilitiesKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pSurfaceCapabilities);

    params->result = wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(params->physicalDevice, params->surface, params->pSurfaceCapabilities);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    VkResult result;
    result = wine_phys_dev_from_handle(physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(wine_phys_dev_from_handle(physicalDevice)->phys_dev, wine_surface_from_handle(surface)->driver_surface, pSurfaceCapabilities);
    return result;
}

static NTSTATUS thunk32_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceCapabilitiesKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pSurfaceCapabilities);

    params->result = wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(params->physicalDevice, params->surface, params->pSurfaceCapabilities);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSurfaceFormats2KHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceFormats2KHR_params *params = args;
    VkPhysicalDeviceSurfaceInfo2KHR pSurfaceInfo_host;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pSurfaceInfo, params->pSurfaceFormatCount, params->pSurfaceFormats);

    convert_VkPhysicalDeviceSurfaceInfo2KHR_win64_to_host(params->pSurfaceInfo, &pSurfaceInfo_host);
    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pSurfaceInfo_host, params->pSurfaceFormatCount, params->pSurfaceFormats);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSurfaceFormats2KHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceFormats2KHR_params *params = args;
    VkPhysicalDeviceSurfaceInfo2KHR_host pSurfaceInfo_host;
    TRACE("%p, %p, %p, %p\n", params->physicalDevice, params->pSurfaceInfo, params->pSurfaceFormatCount, params->pSurfaceFormats);

    convert_VkPhysicalDeviceSurfaceInfo2KHR_win32_to_host(params->pSurfaceInfo, &pSurfaceInfo_host);
    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, &pSurfaceInfo_host, params->pSurfaceFormatCount, params->pSurfaceFormats);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSurfaceFormatsKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceFormatsKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pSurfaceFormatCount, params->pSurfaceFormats);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormatsKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pSurfaceFormatCount, params->pSurfaceFormats);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSurfaceFormatsKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceFormatsKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pSurfaceFormatCount, params->pSurfaceFormats);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormatsKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pSurfaceFormatCount, params->pSurfaceFormats);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSurfacePresentModesKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfacePresentModesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pPresentModeCount, params->pPresentModes);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfacePresentModesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pPresentModeCount, params->pPresentModes);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSurfacePresentModesKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfacePresentModesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->physicalDevice, wine_dbgstr_longlong(params->surface), params->pPresentModeCount, params->pPresentModes);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfacePresentModesKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, wine_surface_from_handle(params->surface)->driver_surface, params->pPresentModeCount, params->pPresentModes);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceSurfaceSupportKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceSupportKHR_params *params = args;
    TRACE("%p, %u, 0x%s, %p\n", params->physicalDevice, params->queueFamilyIndex, wine_dbgstr_longlong(params->surface), params->pSupported);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceSupportKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->queueFamilyIndex, wine_surface_from_handle(params->surface)->driver_surface, params->pSupported);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceSurfaceSupportKHR(void *args)
{
    struct vkGetPhysicalDeviceSurfaceSupportKHR_params *params = args;
    TRACE("%p, %u, 0x%s, %p\n", params->physicalDevice, params->queueFamilyIndex, wine_dbgstr_longlong(params->surface), params->pSupported);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceSurfaceSupportKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->queueFamilyIndex, wine_surface_from_handle(params->surface)->driver_surface, params->pSupported);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceToolProperties(void *args)
{
    struct vkGetPhysicalDeviceToolProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pToolCount, params->pToolProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceToolProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pToolCount, params->pToolProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceToolProperties(void *args)
{
    struct vkGetPhysicalDeviceToolProperties_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pToolCount, params->pToolProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceToolProperties(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pToolCount, params->pToolProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceToolPropertiesEXT(void *args)
{
    struct vkGetPhysicalDeviceToolPropertiesEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pToolCount, params->pToolProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceToolPropertiesEXT(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pToolCount, params->pToolProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceToolPropertiesEXT(void *args)
{
    struct vkGetPhysicalDeviceToolPropertiesEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->physicalDevice, params->pToolCount, params->pToolProperties);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceToolPropertiesEXT(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->pToolCount, params->pToolProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPhysicalDeviceWin32PresentationSupportKHR(void *args)
{
    struct vkGetPhysicalDeviceWin32PresentationSupportKHR_params *params = args;
    TRACE("%p, %u\n", params->physicalDevice, params->queueFamilyIndex);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceWin32PresentationSupportKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->queueFamilyIndex);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPhysicalDeviceWin32PresentationSupportKHR(void *args)
{
    struct vkGetPhysicalDeviceWin32PresentationSupportKHR_params *params = args;
    TRACE("%p, %u\n", params->physicalDevice, params->queueFamilyIndex);

    params->result = wine_phys_dev_from_handle(params->physicalDevice)->instance->funcs.p_vkGetPhysicalDeviceWin32PresentationSupportKHR(wine_phys_dev_from_handle(params->physicalDevice)->phys_dev, params->queueFamilyIndex);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPipelineCacheData(void *args)
{
    struct vkGetPipelineCacheData_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->pDataSize, params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineCacheData(wine_device_from_handle(params->device)->device, params->pipelineCache, params->pDataSize, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPipelineCacheData(void *args)
{
    struct vkGetPipelineCacheData_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipelineCache), params->pDataSize, params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineCacheData(wine_device_from_handle(params->device)->device, params->pipelineCache, params->pDataSize, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPipelineExecutableInternalRepresentationsKHR(void *args)
{
    struct vkGetPipelineExecutableInternalRepresentationsKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pInternalRepresentationCount, params->pInternalRepresentations);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineExecutableInternalRepresentationsKHR(wine_device_from_handle(params->device)->device, params->pExecutableInfo, params->pInternalRepresentationCount, params->pInternalRepresentations);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPipelineExecutableInternalRepresentationsKHR(void *args)
{
    struct vkGetPipelineExecutableInternalRepresentationsKHR_params *params = args;
    VkPipelineExecutableInfoKHR_host pExecutableInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pInternalRepresentationCount, params->pInternalRepresentations);

    convert_VkPipelineExecutableInfoKHR_win32_to_host(params->pExecutableInfo, &pExecutableInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineExecutableInternalRepresentationsKHR(wine_device_from_handle(params->device)->device, &pExecutableInfo_host, params->pInternalRepresentationCount, params->pInternalRepresentations);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPipelineExecutablePropertiesKHR(void *args)
{
    struct vkGetPipelineExecutablePropertiesKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pPipelineInfo, params->pExecutableCount, params->pProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineExecutablePropertiesKHR(wine_device_from_handle(params->device)->device, params->pPipelineInfo, params->pExecutableCount, params->pProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPipelineExecutablePropertiesKHR(void *args)
{
    struct vkGetPipelineExecutablePropertiesKHR_params *params = args;
    VkPipelineInfoKHR_host pPipelineInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pPipelineInfo, params->pExecutableCount, params->pProperties);

    convert_VkPipelineInfoKHR_win32_to_host(params->pPipelineInfo, &pPipelineInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineExecutablePropertiesKHR(wine_device_from_handle(params->device)->device, &pPipelineInfo_host, params->pExecutableCount, params->pProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPipelineExecutableStatisticsKHR(void *args)
{
    struct vkGetPipelineExecutableStatisticsKHR_params *params = args;
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pStatisticCount, params->pStatistics);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineExecutableStatisticsKHR(wine_device_from_handle(params->device)->device, params->pExecutableInfo, params->pStatisticCount, params->pStatistics);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPipelineExecutableStatisticsKHR(void *args)
{
    struct vkGetPipelineExecutableStatisticsKHR_params *params = args;
    VkPipelineExecutableInfoKHR_host pExecutableInfo_host;
    TRACE("%p, %p, %p, %p\n", params->device, params->pExecutableInfo, params->pStatisticCount, params->pStatistics);

    convert_VkPipelineExecutableInfoKHR_win32_to_host(params->pExecutableInfo, &pExecutableInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelineExecutableStatisticsKHR(wine_device_from_handle(params->device)->device, &pExecutableInfo_host, params->pStatisticCount, params->pStatistics);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPipelinePropertiesEXT(void *args)
{
    struct vkGetPipelinePropertiesEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pPipelineInfo, params->pPipelineProperties);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelinePropertiesEXT(wine_device_from_handle(params->device)->device, params->pPipelineInfo, params->pPipelineProperties);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPipelinePropertiesEXT(void *args)
{
    struct vkGetPipelinePropertiesEXT_params *params = args;
    VkPipelineInfoEXT_host pPipelineInfo_host;
    TRACE("%p, %p, %p\n", params->device, params->pPipelineInfo, params->pPipelineProperties);

    convert_VkPipelineInfoEXT_win32_to_host(params->pPipelineInfo, &pPipelineInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetPipelinePropertiesEXT(wine_device_from_handle(params->device)->device, &pPipelineInfo_host, params->pPipelineProperties);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPrivateData(void *args)
{
    struct vkGetPrivateData_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %p\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkGetPrivateData(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPrivateData(void *args)
{
    struct vkGetPrivateData_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %p\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkGetPrivateData(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetPrivateDataEXT(void *args)
{
    struct vkGetPrivateDataEXT_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %p\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkGetPrivateDataEXT(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetPrivateDataEXT(void *args)
{
    struct vkGetPrivateDataEXT_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, %p\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkGetPrivateDataEXT(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetQueryPoolResults(void *args)
{
    struct vkGetQueryPoolResults_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride), params->flags);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetQueryPoolResults(wine_device_from_handle(params->device)->device, params->queryPool, params->firstQuery, params->queryCount, params->dataSize, params->pData, params->stride, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetQueryPoolResults(void *args)
{
    struct vkGetQueryPoolResults_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride), params->flags);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetQueryPoolResults(wine_device_from_handle(params->device)->device, params->queryPool, params->firstQuery, params->queryCount, params->dataSize, params->pData, params->stride, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetQueueCheckpointData2NV(void *args)
{
    struct vkGetQueueCheckpointData2NV_params *params = args;
    TRACE("%p, %p, %p\n", params->queue, params->pCheckpointDataCount, params->pCheckpointData);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkGetQueueCheckpointData2NV(wine_queue_from_handle(params->queue)->queue, params->pCheckpointDataCount, params->pCheckpointData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetQueueCheckpointData2NV(void *args)
{
    struct vkGetQueueCheckpointData2NV_params *params = args;
    TRACE("%p, %p, %p\n", params->queue, params->pCheckpointDataCount, params->pCheckpointData);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkGetQueueCheckpointData2NV(wine_queue_from_handle(params->queue)->queue, params->pCheckpointDataCount, params->pCheckpointData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetQueueCheckpointDataNV(void *args)
{
    struct vkGetQueueCheckpointDataNV_params *params = args;
    TRACE("%p, %p, %p\n", params->queue, params->pCheckpointDataCount, params->pCheckpointData);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkGetQueueCheckpointDataNV(wine_queue_from_handle(params->queue)->queue, params->pCheckpointDataCount, params->pCheckpointData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetQueueCheckpointDataNV(void *args)
{
    struct vkGetQueueCheckpointDataNV_params *params = args;
    TRACE("%p, %p, %p\n", params->queue, params->pCheckpointDataCount, params->pCheckpointData);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkGetQueueCheckpointDataNV(wine_queue_from_handle(params->queue)->queue, params->pCheckpointDataCount, params->pCheckpointData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(void *args)
{
    struct vkGetRayTracingCaptureReplayShaderGroupHandlesKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(wine_device_from_handle(params->device)->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(void *args)
{
    struct vkGetRayTracingCaptureReplayShaderGroupHandlesKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(wine_device_from_handle(params->device)->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetRayTracingShaderGroupHandlesKHR(void *args)
{
    struct vkGetRayTracingShaderGroupHandlesKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingShaderGroupHandlesKHR(wine_device_from_handle(params->device)->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetRayTracingShaderGroupHandlesKHR(void *args)
{
    struct vkGetRayTracingShaderGroupHandlesKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingShaderGroupHandlesKHR(wine_device_from_handle(params->device)->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetRayTracingShaderGroupHandlesNV(void *args)
{
    struct vkGetRayTracingShaderGroupHandlesNV_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingShaderGroupHandlesNV(wine_device_from_handle(params->device)->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetRayTracingShaderGroupHandlesNV(void *args)
{
    struct vkGetRayTracingShaderGroupHandlesNV_params *params = args;
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->firstGroup, params->groupCount, wine_dbgstr_longlong(params->dataSize), params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingShaderGroupHandlesNV(wine_device_from_handle(params->device)->device, params->pipeline, params->firstGroup, params->groupCount, params->dataSize, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetRayTracingShaderGroupStackSizeKHR(void *args)
{
    struct vkGetRayTracingShaderGroupStackSizeKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %#x\n", params->device, wine_dbgstr_longlong(params->pipeline), params->group, params->groupShader);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingShaderGroupStackSizeKHR(wine_device_from_handle(params->device)->device, params->pipeline, params->group, params->groupShader);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetRayTracingShaderGroupStackSizeKHR(void *args)
{
    struct vkGetRayTracingShaderGroupStackSizeKHR_params *params = args;
    TRACE("%p, 0x%s, %u, %#x\n", params->device, wine_dbgstr_longlong(params->pipeline), params->group, params->groupShader);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetRayTracingShaderGroupStackSizeKHR(wine_device_from_handle(params->device)->device, params->pipeline, params->group, params->groupShader);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetRenderAreaGranularity(void *args)
{
    struct vkGetRenderAreaGranularity_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderPass), params->pGranularity);

    wine_device_from_handle(params->device)->funcs.p_vkGetRenderAreaGranularity(wine_device_from_handle(params->device)->device, params->renderPass, params->pGranularity);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetRenderAreaGranularity(void *args)
{
    struct vkGetRenderAreaGranularity_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->renderPass), params->pGranularity);

    wine_device_from_handle(params->device)->funcs.p_vkGetRenderAreaGranularity(wine_device_from_handle(params->device)->device, params->renderPass, params->pGranularity);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetSemaphoreCounterValue(void *args)
{
    struct vkGetSemaphoreCounterValue_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pValue);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetSemaphoreCounterValue(wine_device_from_handle(params->device)->device, params->semaphore, params->pValue);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetSemaphoreCounterValue(void *args)
{
    struct vkGetSemaphoreCounterValue_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pValue);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetSemaphoreCounterValue(wine_device_from_handle(params->device)->device, params->semaphore, params->pValue);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetSemaphoreCounterValueKHR(void *args)
{
    struct vkGetSemaphoreCounterValueKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pValue);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetSemaphoreCounterValueKHR(wine_device_from_handle(params->device)->device, params->semaphore, params->pValue);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetSemaphoreCounterValueKHR(void *args)
{
    struct vkGetSemaphoreCounterValueKHR_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->semaphore), params->pValue);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetSemaphoreCounterValueKHR(wine_device_from_handle(params->device)->device, params->semaphore, params->pValue);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetShaderInfoAMD(void *args)
{
    struct vkGetShaderInfoAMD_params *params = args;
    TRACE("%p, 0x%s, %#x, %#x, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->shaderStage, params->infoType, params->pInfoSize, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetShaderInfoAMD(wine_device_from_handle(params->device)->device, params->pipeline, params->shaderStage, params->infoType, params->pInfoSize, params->pInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetShaderInfoAMD(void *args)
{
    struct vkGetShaderInfoAMD_params *params = args;
    TRACE("%p, 0x%s, %#x, %#x, %p, %p\n", params->device, wine_dbgstr_longlong(params->pipeline), params->shaderStage, params->infoType, params->pInfoSize, params->pInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetShaderInfoAMD(wine_device_from_handle(params->device)->device, params->pipeline, params->shaderStage, params->infoType, params->pInfoSize, params->pInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetShaderModuleCreateInfoIdentifierEXT(void *args)
{
    struct vkGetShaderModuleCreateInfoIdentifierEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pIdentifier);

    wine_device_from_handle(params->device)->funcs.p_vkGetShaderModuleCreateInfoIdentifierEXT(wine_device_from_handle(params->device)->device, params->pCreateInfo, params->pIdentifier);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetShaderModuleCreateInfoIdentifierEXT(void *args)
{
    struct vkGetShaderModuleCreateInfoIdentifierEXT_params *params = args;
    TRACE("%p, %p, %p\n", params->device, params->pCreateInfo, params->pIdentifier);

    wine_device_from_handle(params->device)->funcs.p_vkGetShaderModuleCreateInfoIdentifierEXT(wine_device_from_handle(params->device)->device, params->pCreateInfo, params->pIdentifier);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetShaderModuleIdentifierEXT(void *args)
{
    struct vkGetShaderModuleIdentifierEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->shaderModule), params->pIdentifier);

    wine_device_from_handle(params->device)->funcs.p_vkGetShaderModuleIdentifierEXT(wine_device_from_handle(params->device)->device, params->shaderModule, params->pIdentifier);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetShaderModuleIdentifierEXT(void *args)
{
    struct vkGetShaderModuleIdentifierEXT_params *params = args;
    TRACE("%p, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->shaderModule), params->pIdentifier);

    wine_device_from_handle(params->device)->funcs.p_vkGetShaderModuleIdentifierEXT(wine_device_from_handle(params->device)->device, params->shaderModule, params->pIdentifier);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetSwapchainImagesKHR(void *args)
{
    struct vkGetSwapchainImagesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), params->pSwapchainImageCount, params->pSwapchainImages);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetSwapchainImagesKHR(wine_device_from_handle(params->device)->device, params->swapchain, params->pSwapchainImageCount, params->pSwapchainImages);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetSwapchainImagesKHR(void *args)
{
    struct vkGetSwapchainImagesKHR_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->swapchain), params->pSwapchainImageCount, params->pSwapchainImages);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetSwapchainImagesKHR(wine_device_from_handle(params->device)->device, params->swapchain, params->pSwapchainImageCount, params->pSwapchainImages);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkGetValidationCacheDataEXT(void *args)
{
    struct vkGetValidationCacheDataEXT_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->validationCache), params->pDataSize, params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetValidationCacheDataEXT(wine_device_from_handle(params->device)->device, params->validationCache, params->pDataSize, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkGetValidationCacheDataEXT(void *args)
{
    struct vkGetValidationCacheDataEXT_params *params = args;
    TRACE("%p, 0x%s, %p, %p\n", params->device, wine_dbgstr_longlong(params->validationCache), params->pDataSize, params->pData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkGetValidationCacheDataEXT(wine_device_from_handle(params->device)->device, params->validationCache, params->pDataSize, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkInitializePerformanceApiINTEL(void *args)
{
    struct vkInitializePerformanceApiINTEL_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInitializeInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkInitializePerformanceApiINTEL(wine_device_from_handle(params->device)->device, params->pInitializeInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkInitializePerformanceApiINTEL(void *args)
{
    struct vkInitializePerformanceApiINTEL_params *params = args;
    TRACE("%p, %p\n", params->device, params->pInitializeInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkInitializePerformanceApiINTEL(wine_device_from_handle(params->device)->device, params->pInitializeInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkInvalidateMappedMemoryRanges(void *args)
{
    struct vkInvalidateMappedMemoryRanges_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkInvalidateMappedMemoryRanges(wine_device_from_handle(params->device)->device, params->memoryRangeCount, params->pMemoryRanges);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkInvalidateMappedMemoryRanges(void *args)
{
    struct vkInvalidateMappedMemoryRanges_params *params = args;
    VkMappedMemoryRange_host *pMemoryRanges_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p\n", params->device, params->memoryRangeCount, params->pMemoryRanges);

    init_conversion_context(&ctx);
    pMemoryRanges_host = convert_VkMappedMemoryRange_array_win32_to_host(&ctx, params->pMemoryRanges, params->memoryRangeCount);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkInvalidateMappedMemoryRanges(wine_device_from_handle(params->device)->device, params->memoryRangeCount, pMemoryRanges_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkMapMemory(void *args)
{
    struct vkMapMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %#x, %p\n", params->device, wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->size), params->flags, params->ppData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkMapMemory(wine_device_from_handle(params->device)->device, params->memory, params->offset, params->size, params->flags, params->ppData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkMapMemory(void *args)
{
    struct vkMapMemory_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s, %#x, %p\n", params->device, wine_dbgstr_longlong(params->memory), wine_dbgstr_longlong(params->offset), wine_dbgstr_longlong(params->size), params->flags, params->ppData);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkMapMemory(wine_device_from_handle(params->device)->device, params->memory, params->offset, params->size, params->flags, params->ppData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkMergePipelineCaches(void *args)
{
    struct vkMergePipelineCaches_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->dstCache), params->srcCacheCount, params->pSrcCaches);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkMergePipelineCaches(wine_device_from_handle(params->device)->device, params->dstCache, params->srcCacheCount, params->pSrcCaches);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkMergePipelineCaches(void *args)
{
    struct vkMergePipelineCaches_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->dstCache), params->srcCacheCount, params->pSrcCaches);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkMergePipelineCaches(wine_device_from_handle(params->device)->device, params->dstCache, params->srcCacheCount, params->pSrcCaches);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkMergeValidationCachesEXT(void *args)
{
    struct vkMergeValidationCachesEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->dstCache), params->srcCacheCount, params->pSrcCaches);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkMergeValidationCachesEXT(wine_device_from_handle(params->device)->device, params->dstCache, params->srcCacheCount, params->pSrcCaches);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkMergeValidationCachesEXT(void *args)
{
    struct vkMergeValidationCachesEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %p\n", params->device, wine_dbgstr_longlong(params->dstCache), params->srcCacheCount, params->pSrcCaches);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkMergeValidationCachesEXT(wine_device_from_handle(params->device)->device, params->dstCache, params->srcCacheCount, params->pSrcCaches);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueBeginDebugUtilsLabelEXT(void *args)
{
    struct vkQueueBeginDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pLabelInfo);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueBeginDebugUtilsLabelEXT(wine_queue_from_handle(params->queue)->queue, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueBeginDebugUtilsLabelEXT(void *args)
{
    struct vkQueueBeginDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pLabelInfo);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueBeginDebugUtilsLabelEXT(wine_queue_from_handle(params->queue)->queue, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueBindSparse(void *args)
{
    struct vkQueueBindSparse_params *params = args;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->bindInfoCount, params->pBindInfo, wine_dbgstr_longlong(params->fence));

    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueBindSparse(wine_queue_from_handle(params->queue)->queue, params->bindInfoCount, params->pBindInfo, params->fence);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueBindSparse(void *args)
{
    struct vkQueueBindSparse_params *params = args;
    VkBindSparseInfo_host *pBindInfo_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->bindInfoCount, params->pBindInfo, wine_dbgstr_longlong(params->fence));

    init_conversion_context(&ctx);
    pBindInfo_host = convert_VkBindSparseInfo_array_win32_to_host(&ctx, params->pBindInfo, params->bindInfoCount);
    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueBindSparse(wine_queue_from_handle(params->queue)->queue, params->bindInfoCount, pBindInfo_host, params->fence);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueEndDebugUtilsLabelEXT(void *args)
{
    struct vkQueueEndDebugUtilsLabelEXT_params *params = args;
    TRACE("%p\n", params->queue);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueEndDebugUtilsLabelEXT(wine_queue_from_handle(params->queue)->queue);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueEndDebugUtilsLabelEXT(void *args)
{
    struct vkQueueEndDebugUtilsLabelEXT_params *params = args;
    TRACE("%p\n", params->queue);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueEndDebugUtilsLabelEXT(wine_queue_from_handle(params->queue)->queue);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueInsertDebugUtilsLabelEXT(void *args)
{
    struct vkQueueInsertDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pLabelInfo);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueInsertDebugUtilsLabelEXT(wine_queue_from_handle(params->queue)->queue, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueInsertDebugUtilsLabelEXT(void *args)
{
    struct vkQueueInsertDebugUtilsLabelEXT_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pLabelInfo);

    wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueInsertDebugUtilsLabelEXT(wine_queue_from_handle(params->queue)->queue, params->pLabelInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueuePresentKHR(void *args)
{
    struct vkQueuePresentKHR_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pPresentInfo);

    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueuePresentKHR(wine_queue_from_handle(params->queue)->queue, params->pPresentInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueuePresentKHR(void *args)
{
    struct vkQueuePresentKHR_params *params = args;
    TRACE("%p, %p\n", params->queue, params->pPresentInfo);

    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueuePresentKHR(wine_queue_from_handle(params->queue)->queue, params->pPresentInfo);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueSetPerformanceConfigurationINTEL(void *args)
{
    struct vkQueueSetPerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, 0x%s\n", params->queue, wine_dbgstr_longlong(params->configuration));

    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSetPerformanceConfigurationINTEL(wine_queue_from_handle(params->queue)->queue, params->configuration);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueSetPerformanceConfigurationINTEL(void *args)
{
    struct vkQueueSetPerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, 0x%s\n", params->queue, wine_dbgstr_longlong(params->configuration));

    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSetPerformanceConfigurationINTEL(wine_queue_from_handle(params->queue)->queue, params->configuration);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueSubmit(void *args)
{
    struct vkQueueSubmit_params *params = args;
    VkSubmitInfo *pSubmits_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    init_conversion_context(&ctx);
    pSubmits_host = convert_VkSubmitInfo_array_win64_to_host(&ctx, params->pSubmits, params->submitCount);
    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSubmit(wine_queue_from_handle(params->queue)->queue, params->submitCount, pSubmits_host, params->fence);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueSubmit(void *args)
{
    struct vkQueueSubmit_params *params = args;
    VkSubmitInfo *pSubmits_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    init_conversion_context(&ctx);
    pSubmits_host = convert_VkSubmitInfo_array_win32_to_host(&ctx, params->pSubmits, params->submitCount);
    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSubmit(wine_queue_from_handle(params->queue)->queue, params->submitCount, pSubmits_host, params->fence);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueSubmit2(void *args)
{
    struct vkQueueSubmit2_params *params = args;
    VkSubmitInfo2 *pSubmits_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    init_conversion_context(&ctx);
    pSubmits_host = convert_VkSubmitInfo2_array_win64_to_host(&ctx, params->pSubmits, params->submitCount);
    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSubmit2(wine_queue_from_handle(params->queue)->queue, params->submitCount, pSubmits_host, params->fence);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueSubmit2(void *args)
{
    struct vkQueueSubmit2_params *params = args;
    VkSubmitInfo2_host *pSubmits_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    init_conversion_context(&ctx);
    pSubmits_host = convert_VkSubmitInfo2_array_win32_to_host(&ctx, params->pSubmits, params->submitCount);
    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSubmit2(wine_queue_from_handle(params->queue)->queue, params->submitCount, pSubmits_host, params->fence);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueSubmit2KHR(void *args)
{
    struct vkQueueSubmit2KHR_params *params = args;
    VkSubmitInfo2 *pSubmits_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    init_conversion_context(&ctx);
    pSubmits_host = convert_VkSubmitInfo2_array_win64_to_host(&ctx, params->pSubmits, params->submitCount);
    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSubmit2KHR(wine_queue_from_handle(params->queue)->queue, params->submitCount, pSubmits_host, params->fence);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueSubmit2KHR(void *args)
{
    struct vkQueueSubmit2KHR_params *params = args;
    VkSubmitInfo2_host *pSubmits_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, 0x%s\n", params->queue, params->submitCount, params->pSubmits, wine_dbgstr_longlong(params->fence));

    init_conversion_context(&ctx);
    pSubmits_host = convert_VkSubmitInfo2_array_win32_to_host(&ctx, params->pSubmits, params->submitCount);
    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueSubmit2KHR(wine_queue_from_handle(params->queue)->queue, params->submitCount, pSubmits_host, params->fence);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkQueueWaitIdle(void *args)
{
    struct vkQueueWaitIdle_params *params = args;
    TRACE("%p\n", params->queue);

    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueWaitIdle(wine_queue_from_handle(params->queue)->queue);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkQueueWaitIdle(void *args)
{
    struct vkQueueWaitIdle_params *params = args;
    TRACE("%p\n", params->queue);

    params->result = wine_queue_from_handle(params->queue)->device->funcs.p_vkQueueWaitIdle(wine_queue_from_handle(params->queue)->queue);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkReleasePerformanceConfigurationINTEL(void *args)
{
    struct vkReleasePerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->configuration));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkReleasePerformanceConfigurationINTEL(wine_device_from_handle(params->device)->device, params->configuration);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkReleasePerformanceConfigurationINTEL(void *args)
{
    struct vkReleasePerformanceConfigurationINTEL_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->configuration));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkReleasePerformanceConfigurationINTEL(wine_device_from_handle(params->device)->device, params->configuration);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkReleaseProfilingLockKHR(void *args)
{
    struct vkReleaseProfilingLockKHR_params *params = args;
    TRACE("%p\n", params->device);

    wine_device_from_handle(params->device)->funcs.p_vkReleaseProfilingLockKHR(wine_device_from_handle(params->device)->device);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkReleaseProfilingLockKHR(void *args)
{
    struct vkReleaseProfilingLockKHR_params *params = args;
    TRACE("%p\n", params->device);

    wine_device_from_handle(params->device)->funcs.p_vkReleaseProfilingLockKHR(wine_device_from_handle(params->device)->device);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkResetCommandBuffer(void *args)
{
    struct vkResetCommandBuffer_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->flags);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkResetCommandBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkResetCommandBuffer(void *args)
{
    struct vkResetCommandBuffer_params *params = args;
    TRACE("%p, %#x\n", params->commandBuffer, params->flags);

    params->result = wine_cmd_buffer_from_handle(params->commandBuffer)->device->funcs.p_vkResetCommandBuffer(wine_cmd_buffer_from_handle(params->commandBuffer)->command_buffer, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkResetCommandPool(void *args)
{
    struct vkResetCommandPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetCommandPool(wine_device_from_handle(params->device)->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkResetCommandPool(void *args)
{
    struct vkResetCommandPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetCommandPool(wine_device_from_handle(params->device)->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkResetDescriptorPool(void *args)
{
    struct vkResetDescriptorPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->flags);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetDescriptorPool(wine_device_from_handle(params->device)->device, params->descriptorPool, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkResetDescriptorPool(void *args)
{
    struct vkResetDescriptorPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->descriptorPool), params->flags);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetDescriptorPool(wine_device_from_handle(params->device)->device, params->descriptorPool, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkResetEvent(void *args)
{
    struct vkResetEvent_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetEvent(wine_device_from_handle(params->device)->device, params->event);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkResetEvent(void *args)
{
    struct vkResetEvent_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetEvent(wine_device_from_handle(params->device)->device, params->event);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkResetFences(void *args)
{
    struct vkResetFences_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->fenceCount, params->pFences);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetFences(wine_device_from_handle(params->device)->device, params->fenceCount, params->pFences);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkResetFences(void *args)
{
    struct vkResetFences_params *params = args;
    TRACE("%p, %u, %p\n", params->device, params->fenceCount, params->pFences);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkResetFences(wine_device_from_handle(params->device)->device, params->fenceCount, params->pFences);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkResetQueryPool(void *args)
{
    struct vkResetQueryPool_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);

    wine_device_from_handle(params->device)->funcs.p_vkResetQueryPool(wine_device_from_handle(params->device)->device, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkResetQueryPool(void *args)
{
    struct vkResetQueryPool_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);

    wine_device_from_handle(params->device)->funcs.p_vkResetQueryPool(wine_device_from_handle(params->device)->device, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkResetQueryPoolEXT(void *args)
{
    struct vkResetQueryPoolEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);

    wine_device_from_handle(params->device)->funcs.p_vkResetQueryPoolEXT(wine_device_from_handle(params->device)->device, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkResetQueryPoolEXT(void *args)
{
    struct vkResetQueryPoolEXT_params *params = args;
    TRACE("%p, 0x%s, %u, %u\n", params->device, wine_dbgstr_longlong(params->queryPool), params->firstQuery, params->queryCount);

    wine_device_from_handle(params->device)->funcs.p_vkResetQueryPoolEXT(wine_device_from_handle(params->device)->device, params->queryPool, params->firstQuery, params->queryCount);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSetDebugUtilsObjectNameEXT(void *args)
{
    struct vkSetDebugUtilsObjectNameEXT_params *params = args;
    VkDebugUtilsObjectNameInfoEXT pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugUtilsObjectNameInfoEXT_win64_to_host(params->pNameInfo, &pNameInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetDebugUtilsObjectNameEXT(wine_device_from_handle(params->device)->device, &pNameInfo_host);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSetDebugUtilsObjectNameEXT(void *args)
{
    struct vkSetDebugUtilsObjectNameEXT_params *params = args;
    VkDebugUtilsObjectNameInfoEXT_host pNameInfo_host;
    TRACE("%p, %p\n", params->device, params->pNameInfo);

    convert_VkDebugUtilsObjectNameInfoEXT_win32_to_host(params->pNameInfo, &pNameInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetDebugUtilsObjectNameEXT(wine_device_from_handle(params->device)->device, &pNameInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSetDebugUtilsObjectTagEXT(void *args)
{
    struct vkSetDebugUtilsObjectTagEXT_params *params = args;
    VkDebugUtilsObjectTagInfoEXT pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugUtilsObjectTagInfoEXT_win64_to_host(params->pTagInfo, &pTagInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetDebugUtilsObjectTagEXT(wine_device_from_handle(params->device)->device, &pTagInfo_host);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSetDebugUtilsObjectTagEXT(void *args)
{
    struct vkSetDebugUtilsObjectTagEXT_params *params = args;
    VkDebugUtilsObjectTagInfoEXT_host pTagInfo_host;
    TRACE("%p, %p\n", params->device, params->pTagInfo);

    convert_VkDebugUtilsObjectTagInfoEXT_win32_to_host(params->pTagInfo, &pTagInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetDebugUtilsObjectTagEXT(wine_device_from_handle(params->device)->device, &pTagInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSetDeviceMemoryPriorityEXT(void *args)
{
    struct vkSetDeviceMemoryPriorityEXT_params *params = args;
    TRACE("%p, 0x%s, %f\n", params->device, wine_dbgstr_longlong(params->memory), params->priority);

    wine_device_from_handle(params->device)->funcs.p_vkSetDeviceMemoryPriorityEXT(wine_device_from_handle(params->device)->device, params->memory, params->priority);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSetDeviceMemoryPriorityEXT(void *args)
{
    struct vkSetDeviceMemoryPriorityEXT_params *params = args;
    TRACE("%p, 0x%s, %f\n", params->device, wine_dbgstr_longlong(params->memory), params->priority);

    wine_device_from_handle(params->device)->funcs.p_vkSetDeviceMemoryPriorityEXT(wine_device_from_handle(params->device)->device, params->memory, params->priority);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSetEvent(void *args)
{
    struct vkSetEvent_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetEvent(wine_device_from_handle(params->device)->device, params->event);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSetEvent(void *args)
{
    struct vkSetEvent_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->event));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetEvent(wine_device_from_handle(params->device)->device, params->event);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSetPrivateData(void *args)
{
    struct vkSetPrivateData_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, 0x%s\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), wine_dbgstr_longlong(params->data));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetPrivateData(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->data);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSetPrivateData(void *args)
{
    struct vkSetPrivateData_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, 0x%s\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), wine_dbgstr_longlong(params->data));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetPrivateData(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->data);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSetPrivateDataEXT(void *args)
{
    struct vkSetPrivateDataEXT_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, 0x%s\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), wine_dbgstr_longlong(params->data));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetPrivateDataEXT(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->data);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSetPrivateDataEXT(void *args)
{
    struct vkSetPrivateDataEXT_params *params = args;
    TRACE("%p, %#x, 0x%s, 0x%s, 0x%s\n", params->device, params->objectType, wine_dbgstr_longlong(params->objectHandle), wine_dbgstr_longlong(params->privateDataSlot), wine_dbgstr_longlong(params->data));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSetPrivateDataEXT(wine_device_from_handle(params->device)->device, params->objectType, wine_vk_unwrap_handle(params->objectType, params->objectHandle), params->privateDataSlot, params->data);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSignalSemaphore(void *args)
{
    struct vkSignalSemaphore_params *params = args;
    TRACE("%p, %p\n", params->device, params->pSignalInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSignalSemaphore(wine_device_from_handle(params->device)->device, params->pSignalInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSignalSemaphore(void *args)
{
    struct vkSignalSemaphore_params *params = args;
    VkSemaphoreSignalInfo_host pSignalInfo_host;
    TRACE("%p, %p\n", params->device, params->pSignalInfo);

    convert_VkSemaphoreSignalInfo_win32_to_host(params->pSignalInfo, &pSignalInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkSignalSemaphore(wine_device_from_handle(params->device)->device, &pSignalInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSignalSemaphoreKHR(void *args)
{
    struct vkSignalSemaphoreKHR_params *params = args;
    TRACE("%p, %p\n", params->device, params->pSignalInfo);

    params->result = wine_device_from_handle(params->device)->funcs.p_vkSignalSemaphoreKHR(wine_device_from_handle(params->device)->device, params->pSignalInfo);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSignalSemaphoreKHR(void *args)
{
    struct vkSignalSemaphoreKHR_params *params = args;
    VkSemaphoreSignalInfo_host pSignalInfo_host;
    TRACE("%p, %p\n", params->device, params->pSignalInfo);

    convert_VkSemaphoreSignalInfo_win32_to_host(params->pSignalInfo, &pSignalInfo_host);
    params->result = wine_device_from_handle(params->device)->funcs.p_vkSignalSemaphoreKHR(wine_device_from_handle(params->device)->device, &pSignalInfo_host);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkSubmitDebugUtilsMessageEXT(void *args)
{
    struct vkSubmitDebugUtilsMessageEXT_params *params = args;
    VkDebugUtilsMessengerCallbackDataEXT pCallbackData_host;
    struct conversion_context ctx;
    TRACE("%p, %#x, %#x, %p\n", params->instance, params->messageSeverity, params->messageTypes, params->pCallbackData);

    init_conversion_context(&ctx);
    convert_VkDebugUtilsMessengerCallbackDataEXT_win64_to_host(&ctx, params->pCallbackData, &pCallbackData_host);
    wine_instance_from_handle(params->instance)->funcs.p_vkSubmitDebugUtilsMessageEXT(wine_instance_from_handle(params->instance)->instance, params->messageSeverity, params->messageTypes, &pCallbackData_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkSubmitDebugUtilsMessageEXT(void *args)
{
    struct vkSubmitDebugUtilsMessageEXT_params *params = args;
    VkDebugUtilsMessengerCallbackDataEXT_host pCallbackData_host;
    struct conversion_context ctx;
    TRACE("%p, %#x, %#x, %p\n", params->instance, params->messageSeverity, params->messageTypes, params->pCallbackData);

    init_conversion_context(&ctx);
    convert_VkDebugUtilsMessengerCallbackDataEXT_win32_to_host(&ctx, params->pCallbackData, &pCallbackData_host);
    wine_instance_from_handle(params->instance)->funcs.p_vkSubmitDebugUtilsMessageEXT(wine_instance_from_handle(params->instance)->instance, params->messageSeverity, params->messageTypes, &pCallbackData_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkTrimCommandPool(void *args)
{
    struct vkTrimCommandPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);

    wine_device_from_handle(params->device)->funcs.p_vkTrimCommandPool(wine_device_from_handle(params->device)->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkTrimCommandPool(void *args)
{
    struct vkTrimCommandPool_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);

    wine_device_from_handle(params->device)->funcs.p_vkTrimCommandPool(wine_device_from_handle(params->device)->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkTrimCommandPoolKHR(void *args)
{
    struct vkTrimCommandPoolKHR_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);

    wine_device_from_handle(params->device)->funcs.p_vkTrimCommandPoolKHR(wine_device_from_handle(params->device)->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkTrimCommandPoolKHR(void *args)
{
    struct vkTrimCommandPoolKHR_params *params = args;
    TRACE("%p, 0x%s, %#x\n", params->device, wine_dbgstr_longlong(params->commandPool), params->flags);

    wine_device_from_handle(params->device)->funcs.p_vkTrimCommandPoolKHR(wine_device_from_handle(params->device)->device, wine_cmd_pool_from_handle(params->commandPool)->command_pool, params->flags);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkUninitializePerformanceApiINTEL(void *args)
{
    struct vkUninitializePerformanceApiINTEL_params *params = args;
    TRACE("%p\n", params->device);

    wine_device_from_handle(params->device)->funcs.p_vkUninitializePerformanceApiINTEL(wine_device_from_handle(params->device)->device);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkUninitializePerformanceApiINTEL(void *args)
{
    struct vkUninitializePerformanceApiINTEL_params *params = args;
    TRACE("%p\n", params->device);

    wine_device_from_handle(params->device)->funcs.p_vkUninitializePerformanceApiINTEL(wine_device_from_handle(params->device)->device);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkUnmapMemory(void *args)
{
    struct vkUnmapMemory_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->memory));

    wine_device_from_handle(params->device)->funcs.p_vkUnmapMemory(wine_device_from_handle(params->device)->device, params->memory);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkUnmapMemory(void *args)
{
    struct vkUnmapMemory_params *params = args;
    TRACE("%p, 0x%s\n", params->device, wine_dbgstr_longlong(params->memory));

    wine_device_from_handle(params->device)->funcs.p_vkUnmapMemory(wine_device_from_handle(params->device)->device, params->memory);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkUpdateDescriptorSetWithTemplate(void *args)
{
    struct vkUpdateDescriptorSetWithTemplate_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkUpdateDescriptorSetWithTemplate(wine_device_from_handle(params->device)->device, params->descriptorSet, params->descriptorUpdateTemplate, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkUpdateDescriptorSetWithTemplate(void *args)
{
    struct vkUpdateDescriptorSetWithTemplate_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkUpdateDescriptorSetWithTemplate(wine_device_from_handle(params->device)->device, params->descriptorSet, params->descriptorUpdateTemplate, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkUpdateDescriptorSetWithTemplateKHR(void *args)
{
    struct vkUpdateDescriptorSetWithTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkUpdateDescriptorSetWithTemplateKHR(wine_device_from_handle(params->device)->device, params->descriptorSet, params->descriptorUpdateTemplate, params->pData);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkUpdateDescriptorSetWithTemplateKHR(void *args)
{
    struct vkUpdateDescriptorSetWithTemplateKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, %p\n", params->device, wine_dbgstr_longlong(params->descriptorSet), wine_dbgstr_longlong(params->descriptorUpdateTemplate), params->pData);

    wine_device_from_handle(params->device)->funcs.p_vkUpdateDescriptorSetWithTemplateKHR(wine_device_from_handle(params->device)->device, params->descriptorSet, params->descriptorUpdateTemplate, params->pData);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkUpdateDescriptorSets(void *args)
{
    struct vkUpdateDescriptorSets_params *params = args;
    TRACE("%p, %u, %p, %u, %p\n", params->device, params->descriptorWriteCount, params->pDescriptorWrites, params->descriptorCopyCount, params->pDescriptorCopies);

    wine_device_from_handle(params->device)->funcs.p_vkUpdateDescriptorSets(wine_device_from_handle(params->device)->device, params->descriptorWriteCount, params->pDescriptorWrites, params->descriptorCopyCount, params->pDescriptorCopies);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkUpdateDescriptorSets(void *args)
{
    struct vkUpdateDescriptorSets_params *params = args;
    VkWriteDescriptorSet_host *pDescriptorWrites_host;
    VkCopyDescriptorSet_host *pDescriptorCopies_host;
    struct conversion_context ctx;
    TRACE("%p, %u, %p, %u, %p\n", params->device, params->descriptorWriteCount, params->pDescriptorWrites, params->descriptorCopyCount, params->pDescriptorCopies);

    init_conversion_context(&ctx);
    pDescriptorWrites_host = convert_VkWriteDescriptorSet_array_win32_to_host(&ctx, params->pDescriptorWrites, params->descriptorWriteCount);
    pDescriptorCopies_host = convert_VkCopyDescriptorSet_array_win32_to_host(&ctx, params->pDescriptorCopies, params->descriptorCopyCount);
    wine_device_from_handle(params->device)->funcs.p_vkUpdateDescriptorSets(wine_device_from_handle(params->device)->device, params->descriptorWriteCount, pDescriptorWrites_host, params->descriptorCopyCount, pDescriptorCopies_host);
    free_conversion_context(&ctx);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkWaitForFences(void *args)
{
    struct vkWaitForFences_params *params = args;
    TRACE("%p, %u, %p, %u, 0x%s\n", params->device, params->fenceCount, params->pFences, params->waitAll, wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitForFences(wine_device_from_handle(params->device)->device, params->fenceCount, params->pFences, params->waitAll, params->timeout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkWaitForFences(void *args)
{
    struct vkWaitForFences_params *params = args;
    TRACE("%p, %u, %p, %u, 0x%s\n", params->device, params->fenceCount, params->pFences, params->waitAll, wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitForFences(wine_device_from_handle(params->device)->device, params->fenceCount, params->pFences, params->waitAll, params->timeout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkWaitForPresentKHR(void *args)
{
    struct vkWaitForPresentKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->swapchain), wine_dbgstr_longlong(params->presentId), wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitForPresentKHR(wine_device_from_handle(params->device)->device, params->swapchain, params->presentId, params->timeout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkWaitForPresentKHR(void *args)
{
    struct vkWaitForPresentKHR_params *params = args;
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", params->device, wine_dbgstr_longlong(params->swapchain), wine_dbgstr_longlong(params->presentId), wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitForPresentKHR(wine_device_from_handle(params->device)->device, params->swapchain, params->presentId, params->timeout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkWaitSemaphores(void *args)
{
    struct vkWaitSemaphores_params *params = args;
    TRACE("%p, %p, 0x%s\n", params->device, params->pWaitInfo, wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitSemaphores(wine_device_from_handle(params->device)->device, params->pWaitInfo, params->timeout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkWaitSemaphores(void *args)
{
    struct vkWaitSemaphores_params *params = args;
    TRACE("%p, %p, 0x%s\n", params->device, params->pWaitInfo, wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitSemaphores(wine_device_from_handle(params->device)->device, params->pWaitInfo, params->timeout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkWaitSemaphoresKHR(void *args)
{
    struct vkWaitSemaphoresKHR_params *params = args;
    TRACE("%p, %p, 0x%s\n", params->device, params->pWaitInfo, wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitSemaphoresKHR(wine_device_from_handle(params->device)->device, params->pWaitInfo, params->timeout);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkWaitSemaphoresKHR(void *args)
{
    struct vkWaitSemaphoresKHR_params *params = args;
    TRACE("%p, %p, 0x%s\n", params->device, params->pWaitInfo, wine_dbgstr_longlong(params->timeout));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWaitSemaphoresKHR(wine_device_from_handle(params->device)->device, params->pWaitInfo, params->timeout);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkWriteAccelerationStructuresPropertiesKHR(void *args)
{
    struct vkWriteAccelerationStructuresPropertiesKHR_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %p, 0x%s\n", params->device, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWriteAccelerationStructuresPropertiesKHR(wine_device_from_handle(params->device)->device, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->dataSize, params->pData, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkWriteAccelerationStructuresPropertiesKHR(void *args)
{
    struct vkWriteAccelerationStructuresPropertiesKHR_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %p, 0x%s\n", params->device, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWriteAccelerationStructuresPropertiesKHR(wine_device_from_handle(params->device)->device, params->accelerationStructureCount, params->pAccelerationStructures, params->queryType, params->dataSize, params->pData, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

#if !defined(USE_STRUCT_CONVERSION)

static NTSTATUS thunk64_vkWriteMicromapsPropertiesEXT(void *args)
{
    struct vkWriteMicromapsPropertiesEXT_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %p, 0x%s\n", params->device, params->micromapCount, params->pMicromaps, params->queryType, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWriteMicromapsPropertiesEXT(wine_device_from_handle(params->device)->device, params->micromapCount, params->pMicromaps, params->queryType, params->dataSize, params->pData, params->stride);
    return STATUS_SUCCESS;
}

#else /* USE_STRUCT_CONVERSION */

static NTSTATUS thunk32_vkWriteMicromapsPropertiesEXT(void *args)
{
    struct vkWriteMicromapsPropertiesEXT_params *params = args;
    TRACE("%p, %u, %p, %#x, 0x%s, %p, 0x%s\n", params->device, params->micromapCount, params->pMicromaps, params->queryType, wine_dbgstr_longlong(params->dataSize), params->pData, wine_dbgstr_longlong(params->stride));

    params->result = wine_device_from_handle(params->device)->funcs.p_vkWriteMicromapsPropertiesEXT(wine_device_from_handle(params->device)->device, params->micromapCount, params->pMicromaps, params->queryType, params->dataSize, params->pData, params->stride);
    return STATUS_SUCCESS;
}

#endif /* USE_STRUCT_CONVERSION */

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
    "VK_EXT_depth_clamp_zero_one",
    "VK_EXT_depth_clip_control",
    "VK_EXT_depth_clip_enable",
    "VK_EXT_depth_range_unrestricted",
    "VK_EXT_descriptor_indexing",
    "VK_EXT_device_address_binding_report",
    "VK_EXT_device_fault",
    "VK_EXT_discard_rectangles",
    "VK_EXT_extended_dynamic_state",
    "VK_EXT_extended_dynamic_state2",
    "VK_EXT_extended_dynamic_state3",
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
    "VK_EXT_legacy_dithering",
    "VK_EXT_line_rasterization",
    "VK_EXT_load_store_op_none",
    "VK_EXT_memory_budget",
    "VK_EXT_memory_priority",
    "VK_EXT_mesh_shader",
    "VK_EXT_multi_draw",
    "VK_EXT_multisampled_render_to_single_sampled",
    "VK_EXT_mutable_descriptor_type",
    "VK_EXT_non_seamless_cube_map",
    "VK_EXT_opacity_micromap",
    "VK_EXT_pageable_device_local_memory",
    "VK_EXT_pci_bus_info",
    "VK_EXT_pipeline_creation_cache_control",
    "VK_EXT_pipeline_creation_feedback",
    "VK_EXT_pipeline_properties",
    "VK_EXT_pipeline_protected_access",
    "VK_EXT_pipeline_robustness",
    "VK_EXT_post_depth_coverage",
    "VK_EXT_primitive_topology_list_restart",
    "VK_EXT_primitives_generated_query",
    "VK_EXT_private_data",
    "VK_EXT_provoking_vertex",
    "VK_EXT_queue_family_foreign",
    "VK_EXT_rasterization_order_attachment_access",
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
    "VK_NV_optical_flow",
    "VK_NV_present_barrier",
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
        return (uint64_t) (uintptr_t) wine_cmd_buffer_from_handle(((VkCommandBuffer) (uintptr_t) handle))->command_buffer;
    case VK_OBJECT_TYPE_COMMAND_POOL:
        return (uint64_t) wine_cmd_pool_from_handle(handle)->command_pool;
    case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
        return (uint64_t) wine_debug_report_callback_from_handle(handle)->debug_callback;
    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
        return (uint64_t) wine_debug_utils_messenger_from_handle(handle)->debug_messenger;
    case VK_OBJECT_TYPE_DEVICE:
        return (uint64_t) (uintptr_t) wine_device_from_handle(((VkDevice) (uintptr_t) handle))->device;
    case VK_OBJECT_TYPE_INSTANCE:
        return (uint64_t) (uintptr_t) wine_instance_from_handle(((VkInstance) (uintptr_t) handle))->instance;
    case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
        return (uint64_t) (uintptr_t) wine_phys_dev_from_handle(((VkPhysicalDevice) (uintptr_t) handle))->phys_dev;
    case VK_OBJECT_TYPE_QUEUE:
        return (uint64_t) (uintptr_t) wine_queue_from_handle(((VkQueue) (uintptr_t) handle))->queue;
    case VK_OBJECT_TYPE_SURFACE_KHR:
        return (uint64_t) wine_surface_from_handle(handle)->surface;
    default:
       return handle;
    }
}

#if !defined(USE_STRUCT_CONVERSION)

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    init_vulkan,
    vk_is_available_instance_function,
    vk_is_available_device_function,
    thunk64_vkAcquireNextImage2KHR,
    thunk64_vkAcquireNextImageKHR,
    thunk64_vkAcquirePerformanceConfigurationINTEL,
    thunk64_vkAcquireProfilingLockKHR,
    thunk64_vkAllocateCommandBuffers,
    thunk64_vkAllocateDescriptorSets,
    thunk64_vkAllocateMemory,
    thunk64_vkBeginCommandBuffer,
    thunk64_vkBindAccelerationStructureMemoryNV,
    thunk64_vkBindBufferMemory,
    thunk64_vkBindBufferMemory2,
    thunk64_vkBindBufferMemory2KHR,
    thunk64_vkBindImageMemory,
    thunk64_vkBindImageMemory2,
    thunk64_vkBindImageMemory2KHR,
    thunk64_vkBindOpticalFlowSessionImageNV,
    thunk64_vkBuildAccelerationStructuresKHR,
    thunk64_vkBuildMicromapsEXT,
    thunk64_vkCmdBeginConditionalRenderingEXT,
    thunk64_vkCmdBeginDebugUtilsLabelEXT,
    thunk64_vkCmdBeginQuery,
    thunk64_vkCmdBeginQueryIndexedEXT,
    thunk64_vkCmdBeginRenderPass,
    thunk64_vkCmdBeginRenderPass2,
    thunk64_vkCmdBeginRenderPass2KHR,
    thunk64_vkCmdBeginRendering,
    thunk64_vkCmdBeginRenderingKHR,
    thunk64_vkCmdBeginTransformFeedbackEXT,
    thunk64_vkCmdBindDescriptorSets,
    thunk64_vkCmdBindIndexBuffer,
    thunk64_vkCmdBindInvocationMaskHUAWEI,
    thunk64_vkCmdBindPipeline,
    thunk64_vkCmdBindPipelineShaderGroupNV,
    thunk64_vkCmdBindShadingRateImageNV,
    thunk64_vkCmdBindTransformFeedbackBuffersEXT,
    thunk64_vkCmdBindVertexBuffers,
    thunk64_vkCmdBindVertexBuffers2,
    thunk64_vkCmdBindVertexBuffers2EXT,
    thunk64_vkCmdBlitImage,
    thunk64_vkCmdBlitImage2,
    thunk64_vkCmdBlitImage2KHR,
    thunk64_vkCmdBuildAccelerationStructureNV,
    thunk64_vkCmdBuildAccelerationStructuresIndirectKHR,
    thunk64_vkCmdBuildAccelerationStructuresKHR,
    thunk64_vkCmdBuildMicromapsEXT,
    thunk64_vkCmdClearAttachments,
    thunk64_vkCmdClearColorImage,
    thunk64_vkCmdClearDepthStencilImage,
    thunk64_vkCmdCopyAccelerationStructureKHR,
    thunk64_vkCmdCopyAccelerationStructureNV,
    thunk64_vkCmdCopyAccelerationStructureToMemoryKHR,
    thunk64_vkCmdCopyBuffer,
    thunk64_vkCmdCopyBuffer2,
    thunk64_vkCmdCopyBuffer2KHR,
    thunk64_vkCmdCopyBufferToImage,
    thunk64_vkCmdCopyBufferToImage2,
    thunk64_vkCmdCopyBufferToImage2KHR,
    thunk64_vkCmdCopyImage,
    thunk64_vkCmdCopyImage2,
    thunk64_vkCmdCopyImage2KHR,
    thunk64_vkCmdCopyImageToBuffer,
    thunk64_vkCmdCopyImageToBuffer2,
    thunk64_vkCmdCopyImageToBuffer2KHR,
    thunk64_vkCmdCopyMemoryToAccelerationStructureKHR,
    thunk64_vkCmdCopyMemoryToMicromapEXT,
    thunk64_vkCmdCopyMicromapEXT,
    thunk64_vkCmdCopyMicromapToMemoryEXT,
    thunk64_vkCmdCopyQueryPoolResults,
    thunk64_vkCmdCuLaunchKernelNVX,
    thunk64_vkCmdDebugMarkerBeginEXT,
    thunk64_vkCmdDebugMarkerEndEXT,
    thunk64_vkCmdDebugMarkerInsertEXT,
    thunk64_vkCmdDispatch,
    thunk64_vkCmdDispatchBase,
    thunk64_vkCmdDispatchBaseKHR,
    thunk64_vkCmdDispatchIndirect,
    thunk64_vkCmdDraw,
    thunk64_vkCmdDrawIndexed,
    thunk64_vkCmdDrawIndexedIndirect,
    thunk64_vkCmdDrawIndexedIndirectCount,
    thunk64_vkCmdDrawIndexedIndirectCountAMD,
    thunk64_vkCmdDrawIndexedIndirectCountKHR,
    thunk64_vkCmdDrawIndirect,
    thunk64_vkCmdDrawIndirectByteCountEXT,
    thunk64_vkCmdDrawIndirectCount,
    thunk64_vkCmdDrawIndirectCountAMD,
    thunk64_vkCmdDrawIndirectCountKHR,
    thunk64_vkCmdDrawMeshTasksEXT,
    thunk64_vkCmdDrawMeshTasksIndirectCountEXT,
    thunk64_vkCmdDrawMeshTasksIndirectCountNV,
    thunk64_vkCmdDrawMeshTasksIndirectEXT,
    thunk64_vkCmdDrawMeshTasksIndirectNV,
    thunk64_vkCmdDrawMeshTasksNV,
    thunk64_vkCmdDrawMultiEXT,
    thunk64_vkCmdDrawMultiIndexedEXT,
    thunk64_vkCmdEndConditionalRenderingEXT,
    thunk64_vkCmdEndDebugUtilsLabelEXT,
    thunk64_vkCmdEndQuery,
    thunk64_vkCmdEndQueryIndexedEXT,
    thunk64_vkCmdEndRenderPass,
    thunk64_vkCmdEndRenderPass2,
    thunk64_vkCmdEndRenderPass2KHR,
    thunk64_vkCmdEndRendering,
    thunk64_vkCmdEndRenderingKHR,
    thunk64_vkCmdEndTransformFeedbackEXT,
    thunk64_vkCmdExecuteCommands,
    thunk64_vkCmdExecuteGeneratedCommandsNV,
    thunk64_vkCmdFillBuffer,
    thunk64_vkCmdInsertDebugUtilsLabelEXT,
    thunk64_vkCmdNextSubpass,
    thunk64_vkCmdNextSubpass2,
    thunk64_vkCmdNextSubpass2KHR,
    thunk64_vkCmdOpticalFlowExecuteNV,
    thunk64_vkCmdPipelineBarrier,
    thunk64_vkCmdPipelineBarrier2,
    thunk64_vkCmdPipelineBarrier2KHR,
    thunk64_vkCmdPreprocessGeneratedCommandsNV,
    thunk64_vkCmdPushConstants,
    thunk64_vkCmdPushDescriptorSetKHR,
    thunk64_vkCmdPushDescriptorSetWithTemplateKHR,
    thunk64_vkCmdResetEvent,
    thunk64_vkCmdResetEvent2,
    thunk64_vkCmdResetEvent2KHR,
    thunk64_vkCmdResetQueryPool,
    thunk64_vkCmdResolveImage,
    thunk64_vkCmdResolveImage2,
    thunk64_vkCmdResolveImage2KHR,
    thunk64_vkCmdSetAlphaToCoverageEnableEXT,
    thunk64_vkCmdSetAlphaToOneEnableEXT,
    thunk64_vkCmdSetBlendConstants,
    thunk64_vkCmdSetCheckpointNV,
    thunk64_vkCmdSetCoarseSampleOrderNV,
    thunk64_vkCmdSetColorBlendAdvancedEXT,
    thunk64_vkCmdSetColorBlendEnableEXT,
    thunk64_vkCmdSetColorBlendEquationEXT,
    thunk64_vkCmdSetColorWriteEnableEXT,
    thunk64_vkCmdSetColorWriteMaskEXT,
    thunk64_vkCmdSetConservativeRasterizationModeEXT,
    thunk64_vkCmdSetCoverageModulationModeNV,
    thunk64_vkCmdSetCoverageModulationTableEnableNV,
    thunk64_vkCmdSetCoverageModulationTableNV,
    thunk64_vkCmdSetCoverageReductionModeNV,
    thunk64_vkCmdSetCoverageToColorEnableNV,
    thunk64_vkCmdSetCoverageToColorLocationNV,
    thunk64_vkCmdSetCullMode,
    thunk64_vkCmdSetCullModeEXT,
    thunk64_vkCmdSetDepthBias,
    thunk64_vkCmdSetDepthBiasEnable,
    thunk64_vkCmdSetDepthBiasEnableEXT,
    thunk64_vkCmdSetDepthBounds,
    thunk64_vkCmdSetDepthBoundsTestEnable,
    thunk64_vkCmdSetDepthBoundsTestEnableEXT,
    thunk64_vkCmdSetDepthClampEnableEXT,
    thunk64_vkCmdSetDepthClipEnableEXT,
    thunk64_vkCmdSetDepthClipNegativeOneToOneEXT,
    thunk64_vkCmdSetDepthCompareOp,
    thunk64_vkCmdSetDepthCompareOpEXT,
    thunk64_vkCmdSetDepthTestEnable,
    thunk64_vkCmdSetDepthTestEnableEXT,
    thunk64_vkCmdSetDepthWriteEnable,
    thunk64_vkCmdSetDepthWriteEnableEXT,
    thunk64_vkCmdSetDeviceMask,
    thunk64_vkCmdSetDeviceMaskKHR,
    thunk64_vkCmdSetDiscardRectangleEXT,
    thunk64_vkCmdSetEvent,
    thunk64_vkCmdSetEvent2,
    thunk64_vkCmdSetEvent2KHR,
    thunk64_vkCmdSetExclusiveScissorNV,
    thunk64_vkCmdSetExtraPrimitiveOverestimationSizeEXT,
    thunk64_vkCmdSetFragmentShadingRateEnumNV,
    thunk64_vkCmdSetFragmentShadingRateKHR,
    thunk64_vkCmdSetFrontFace,
    thunk64_vkCmdSetFrontFaceEXT,
    thunk64_vkCmdSetLineRasterizationModeEXT,
    thunk64_vkCmdSetLineStippleEXT,
    thunk64_vkCmdSetLineStippleEnableEXT,
    thunk64_vkCmdSetLineWidth,
    thunk64_vkCmdSetLogicOpEXT,
    thunk64_vkCmdSetLogicOpEnableEXT,
    thunk64_vkCmdSetPatchControlPointsEXT,
    thunk64_vkCmdSetPerformanceMarkerINTEL,
    thunk64_vkCmdSetPerformanceOverrideINTEL,
    thunk64_vkCmdSetPerformanceStreamMarkerINTEL,
    thunk64_vkCmdSetPolygonModeEXT,
    thunk64_vkCmdSetPrimitiveRestartEnable,
    thunk64_vkCmdSetPrimitiveRestartEnableEXT,
    thunk64_vkCmdSetPrimitiveTopology,
    thunk64_vkCmdSetPrimitiveTopologyEXT,
    thunk64_vkCmdSetProvokingVertexModeEXT,
    thunk64_vkCmdSetRasterizationSamplesEXT,
    thunk64_vkCmdSetRasterizationStreamEXT,
    thunk64_vkCmdSetRasterizerDiscardEnable,
    thunk64_vkCmdSetRasterizerDiscardEnableEXT,
    thunk64_vkCmdSetRayTracingPipelineStackSizeKHR,
    thunk64_vkCmdSetRepresentativeFragmentTestEnableNV,
    thunk64_vkCmdSetSampleLocationsEXT,
    thunk64_vkCmdSetSampleLocationsEnableEXT,
    thunk64_vkCmdSetSampleMaskEXT,
    thunk64_vkCmdSetScissor,
    thunk64_vkCmdSetScissorWithCount,
    thunk64_vkCmdSetScissorWithCountEXT,
    thunk64_vkCmdSetShadingRateImageEnableNV,
    thunk64_vkCmdSetStencilCompareMask,
    thunk64_vkCmdSetStencilOp,
    thunk64_vkCmdSetStencilOpEXT,
    thunk64_vkCmdSetStencilReference,
    thunk64_vkCmdSetStencilTestEnable,
    thunk64_vkCmdSetStencilTestEnableEXT,
    thunk64_vkCmdSetStencilWriteMask,
    thunk64_vkCmdSetTessellationDomainOriginEXT,
    thunk64_vkCmdSetVertexInputEXT,
    thunk64_vkCmdSetViewport,
    thunk64_vkCmdSetViewportShadingRatePaletteNV,
    thunk64_vkCmdSetViewportSwizzleNV,
    thunk64_vkCmdSetViewportWScalingEnableNV,
    thunk64_vkCmdSetViewportWScalingNV,
    thunk64_vkCmdSetViewportWithCount,
    thunk64_vkCmdSetViewportWithCountEXT,
    thunk64_vkCmdSubpassShadingHUAWEI,
    thunk64_vkCmdTraceRaysIndirect2KHR,
    thunk64_vkCmdTraceRaysIndirectKHR,
    thunk64_vkCmdTraceRaysKHR,
    thunk64_vkCmdTraceRaysNV,
    thunk64_vkCmdUpdateBuffer,
    thunk64_vkCmdWaitEvents,
    thunk64_vkCmdWaitEvents2,
    thunk64_vkCmdWaitEvents2KHR,
    thunk64_vkCmdWriteAccelerationStructuresPropertiesKHR,
    thunk64_vkCmdWriteAccelerationStructuresPropertiesNV,
    thunk64_vkCmdWriteBufferMarker2AMD,
    thunk64_vkCmdWriteBufferMarkerAMD,
    thunk64_vkCmdWriteMicromapsPropertiesEXT,
    thunk64_vkCmdWriteTimestamp,
    thunk64_vkCmdWriteTimestamp2,
    thunk64_vkCmdWriteTimestamp2KHR,
    thunk64_vkCompileDeferredNV,
    thunk64_vkCopyAccelerationStructureKHR,
    thunk64_vkCopyAccelerationStructureToMemoryKHR,
    thunk64_vkCopyMemoryToAccelerationStructureKHR,
    thunk64_vkCopyMemoryToMicromapEXT,
    thunk64_vkCopyMicromapEXT,
    thunk64_vkCopyMicromapToMemoryEXT,
    thunk64_vkCreateAccelerationStructureKHR,
    thunk64_vkCreateAccelerationStructureNV,
    thunk64_vkCreateBuffer,
    thunk64_vkCreateBufferView,
    thunk64_vkCreateCommandPool,
    thunk64_vkCreateComputePipelines,
    thunk64_vkCreateCuFunctionNVX,
    thunk64_vkCreateCuModuleNVX,
    thunk64_vkCreateDebugReportCallbackEXT,
    thunk64_vkCreateDebugUtilsMessengerEXT,
    thunk64_vkCreateDeferredOperationKHR,
    thunk64_vkCreateDescriptorPool,
    thunk64_vkCreateDescriptorSetLayout,
    thunk64_vkCreateDescriptorUpdateTemplate,
    thunk64_vkCreateDescriptorUpdateTemplateKHR,
    thunk64_vkCreateDevice,
    thunk64_vkCreateEvent,
    thunk64_vkCreateFence,
    thunk64_vkCreateFramebuffer,
    thunk64_vkCreateGraphicsPipelines,
    thunk64_vkCreateImage,
    thunk64_vkCreateImageView,
    thunk64_vkCreateIndirectCommandsLayoutNV,
    thunk64_vkCreateInstance,
    thunk64_vkCreateMicromapEXT,
    thunk64_vkCreateOpticalFlowSessionNV,
    thunk64_vkCreatePipelineCache,
    thunk64_vkCreatePipelineLayout,
    thunk64_vkCreatePrivateDataSlot,
    thunk64_vkCreatePrivateDataSlotEXT,
    thunk64_vkCreateQueryPool,
    thunk64_vkCreateRayTracingPipelinesKHR,
    thunk64_vkCreateRayTracingPipelinesNV,
    thunk64_vkCreateRenderPass,
    thunk64_vkCreateRenderPass2,
    thunk64_vkCreateRenderPass2KHR,
    thunk64_vkCreateSampler,
    thunk64_vkCreateSamplerYcbcrConversion,
    thunk64_vkCreateSamplerYcbcrConversionKHR,
    thunk64_vkCreateSemaphore,
    thunk64_vkCreateShaderModule,
    thunk64_vkCreateSwapchainKHR,
    thunk64_vkCreateValidationCacheEXT,
    thunk64_vkCreateWin32SurfaceKHR,
    thunk64_vkDebugMarkerSetObjectNameEXT,
    thunk64_vkDebugMarkerSetObjectTagEXT,
    thunk64_vkDebugReportMessageEXT,
    thunk64_vkDeferredOperationJoinKHR,
    thunk64_vkDestroyAccelerationStructureKHR,
    thunk64_vkDestroyAccelerationStructureNV,
    thunk64_vkDestroyBuffer,
    thunk64_vkDestroyBufferView,
    thunk64_vkDestroyCommandPool,
    thunk64_vkDestroyCuFunctionNVX,
    thunk64_vkDestroyCuModuleNVX,
    thunk64_vkDestroyDebugReportCallbackEXT,
    thunk64_vkDestroyDebugUtilsMessengerEXT,
    thunk64_vkDestroyDeferredOperationKHR,
    thunk64_vkDestroyDescriptorPool,
    thunk64_vkDestroyDescriptorSetLayout,
    thunk64_vkDestroyDescriptorUpdateTemplate,
    thunk64_vkDestroyDescriptorUpdateTemplateKHR,
    thunk64_vkDestroyDevice,
    thunk64_vkDestroyEvent,
    thunk64_vkDestroyFence,
    thunk64_vkDestroyFramebuffer,
    thunk64_vkDestroyImage,
    thunk64_vkDestroyImageView,
    thunk64_vkDestroyIndirectCommandsLayoutNV,
    thunk64_vkDestroyInstance,
    thunk64_vkDestroyMicromapEXT,
    thunk64_vkDestroyOpticalFlowSessionNV,
    thunk64_vkDestroyPipeline,
    thunk64_vkDestroyPipelineCache,
    thunk64_vkDestroyPipelineLayout,
    thunk64_vkDestroyPrivateDataSlot,
    thunk64_vkDestroyPrivateDataSlotEXT,
    thunk64_vkDestroyQueryPool,
    thunk64_vkDestroyRenderPass,
    thunk64_vkDestroySampler,
    thunk64_vkDestroySamplerYcbcrConversion,
    thunk64_vkDestroySamplerYcbcrConversionKHR,
    thunk64_vkDestroySemaphore,
    thunk64_vkDestroyShaderModule,
    thunk64_vkDestroySurfaceKHR,
    thunk64_vkDestroySwapchainKHR,
    thunk64_vkDestroyValidationCacheEXT,
    thunk64_vkDeviceWaitIdle,
    thunk64_vkEndCommandBuffer,
    thunk64_vkEnumerateDeviceExtensionProperties,
    thunk64_vkEnumerateDeviceLayerProperties,
    thunk64_vkEnumerateInstanceExtensionProperties,
    thunk64_vkEnumerateInstanceVersion,
    thunk64_vkEnumeratePhysicalDeviceGroups,
    thunk64_vkEnumeratePhysicalDeviceGroupsKHR,
    thunk64_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR,
    thunk64_vkEnumeratePhysicalDevices,
    thunk64_vkFlushMappedMemoryRanges,
    thunk64_vkFreeCommandBuffers,
    thunk64_vkFreeDescriptorSets,
    thunk64_vkFreeMemory,
    thunk64_vkGetAccelerationStructureBuildSizesKHR,
    thunk64_vkGetAccelerationStructureDeviceAddressKHR,
    thunk64_vkGetAccelerationStructureHandleNV,
    thunk64_vkGetAccelerationStructureMemoryRequirementsNV,
    thunk64_vkGetBufferDeviceAddress,
    thunk64_vkGetBufferDeviceAddressEXT,
    thunk64_vkGetBufferDeviceAddressKHR,
    thunk64_vkGetBufferMemoryRequirements,
    thunk64_vkGetBufferMemoryRequirements2,
    thunk64_vkGetBufferMemoryRequirements2KHR,
    thunk64_vkGetBufferOpaqueCaptureAddress,
    thunk64_vkGetBufferOpaqueCaptureAddressKHR,
    thunk64_vkGetCalibratedTimestampsEXT,
    thunk64_vkGetDeferredOperationMaxConcurrencyKHR,
    thunk64_vkGetDeferredOperationResultKHR,
    thunk64_vkGetDescriptorSetHostMappingVALVE,
    thunk64_vkGetDescriptorSetLayoutHostMappingInfoVALVE,
    thunk64_vkGetDescriptorSetLayoutSupport,
    thunk64_vkGetDescriptorSetLayoutSupportKHR,
    thunk64_vkGetDeviceAccelerationStructureCompatibilityKHR,
    thunk64_vkGetDeviceBufferMemoryRequirements,
    thunk64_vkGetDeviceBufferMemoryRequirementsKHR,
    thunk64_vkGetDeviceFaultInfoEXT,
    thunk64_vkGetDeviceGroupPeerMemoryFeatures,
    thunk64_vkGetDeviceGroupPeerMemoryFeaturesKHR,
    thunk64_vkGetDeviceGroupPresentCapabilitiesKHR,
    thunk64_vkGetDeviceGroupSurfacePresentModesKHR,
    thunk64_vkGetDeviceImageMemoryRequirements,
    thunk64_vkGetDeviceImageMemoryRequirementsKHR,
    thunk64_vkGetDeviceImageSparseMemoryRequirements,
    thunk64_vkGetDeviceImageSparseMemoryRequirementsKHR,
    thunk64_vkGetDeviceMemoryCommitment,
    thunk64_vkGetDeviceMemoryOpaqueCaptureAddress,
    thunk64_vkGetDeviceMemoryOpaqueCaptureAddressKHR,
    thunk64_vkGetDeviceMicromapCompatibilityEXT,
    thunk64_vkGetDeviceQueue,
    thunk64_vkGetDeviceQueue2,
    thunk64_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI,
    thunk64_vkGetDynamicRenderingTilePropertiesQCOM,
    thunk64_vkGetEventStatus,
    thunk64_vkGetFenceStatus,
    thunk64_vkGetFramebufferTilePropertiesQCOM,
    thunk64_vkGetGeneratedCommandsMemoryRequirementsNV,
    thunk64_vkGetImageMemoryRequirements,
    thunk64_vkGetImageMemoryRequirements2,
    thunk64_vkGetImageMemoryRequirements2KHR,
    thunk64_vkGetImageSparseMemoryRequirements,
    thunk64_vkGetImageSparseMemoryRequirements2,
    thunk64_vkGetImageSparseMemoryRequirements2KHR,
    thunk64_vkGetImageSubresourceLayout,
    thunk64_vkGetImageSubresourceLayout2EXT,
    thunk64_vkGetImageViewAddressNVX,
    thunk64_vkGetImageViewHandleNVX,
    thunk64_vkGetMemoryHostPointerPropertiesEXT,
    thunk64_vkGetMicromapBuildSizesEXT,
    thunk64_vkGetPerformanceParameterINTEL,
    thunk64_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
    thunk64_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV,
    thunk64_vkGetPhysicalDeviceExternalBufferProperties,
    thunk64_vkGetPhysicalDeviceExternalBufferPropertiesKHR,
    thunk64_vkGetPhysicalDeviceExternalFenceProperties,
    thunk64_vkGetPhysicalDeviceExternalFencePropertiesKHR,
    thunk64_vkGetPhysicalDeviceExternalSemaphoreProperties,
    thunk64_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR,
    thunk64_vkGetPhysicalDeviceFeatures,
    thunk64_vkGetPhysicalDeviceFeatures2,
    thunk64_vkGetPhysicalDeviceFeatures2KHR,
    thunk64_vkGetPhysicalDeviceFormatProperties,
    thunk64_vkGetPhysicalDeviceFormatProperties2,
    thunk64_vkGetPhysicalDeviceFormatProperties2KHR,
    thunk64_vkGetPhysicalDeviceFragmentShadingRatesKHR,
    thunk64_vkGetPhysicalDeviceImageFormatProperties,
    thunk64_vkGetPhysicalDeviceImageFormatProperties2,
    thunk64_vkGetPhysicalDeviceImageFormatProperties2KHR,
    thunk64_vkGetPhysicalDeviceMemoryProperties,
    thunk64_vkGetPhysicalDeviceMemoryProperties2,
    thunk64_vkGetPhysicalDeviceMemoryProperties2KHR,
    thunk64_vkGetPhysicalDeviceMultisamplePropertiesEXT,
    thunk64_vkGetPhysicalDeviceOpticalFlowImageFormatsNV,
    thunk64_vkGetPhysicalDevicePresentRectanglesKHR,
    thunk64_vkGetPhysicalDeviceProperties,
    thunk64_vkGetPhysicalDeviceProperties2,
    thunk64_vkGetPhysicalDeviceProperties2KHR,
    thunk64_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR,
    thunk64_vkGetPhysicalDeviceQueueFamilyProperties,
    thunk64_vkGetPhysicalDeviceQueueFamilyProperties2,
    thunk64_vkGetPhysicalDeviceQueueFamilyProperties2KHR,
    thunk64_vkGetPhysicalDeviceSparseImageFormatProperties,
    thunk64_vkGetPhysicalDeviceSparseImageFormatProperties2,
    thunk64_vkGetPhysicalDeviceSparseImageFormatProperties2KHR,
    thunk64_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV,
    thunk64_vkGetPhysicalDeviceSurfaceCapabilities2KHR,
    thunk64_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
    thunk64_vkGetPhysicalDeviceSurfaceFormats2KHR,
    thunk64_vkGetPhysicalDeviceSurfaceFormatsKHR,
    thunk64_vkGetPhysicalDeviceSurfacePresentModesKHR,
    thunk64_vkGetPhysicalDeviceSurfaceSupportKHR,
    thunk64_vkGetPhysicalDeviceToolProperties,
    thunk64_vkGetPhysicalDeviceToolPropertiesEXT,
    thunk64_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    thunk64_vkGetPipelineCacheData,
    thunk64_vkGetPipelineExecutableInternalRepresentationsKHR,
    thunk64_vkGetPipelineExecutablePropertiesKHR,
    thunk64_vkGetPipelineExecutableStatisticsKHR,
    thunk64_vkGetPipelinePropertiesEXT,
    thunk64_vkGetPrivateData,
    thunk64_vkGetPrivateDataEXT,
    thunk64_vkGetQueryPoolResults,
    thunk64_vkGetQueueCheckpointData2NV,
    thunk64_vkGetQueueCheckpointDataNV,
    thunk64_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR,
    thunk64_vkGetRayTracingShaderGroupHandlesKHR,
    thunk64_vkGetRayTracingShaderGroupHandlesNV,
    thunk64_vkGetRayTracingShaderGroupStackSizeKHR,
    thunk64_vkGetRenderAreaGranularity,
    thunk64_vkGetSemaphoreCounterValue,
    thunk64_vkGetSemaphoreCounterValueKHR,
    thunk64_vkGetShaderInfoAMD,
    thunk64_vkGetShaderModuleCreateInfoIdentifierEXT,
    thunk64_vkGetShaderModuleIdentifierEXT,
    thunk64_vkGetSwapchainImagesKHR,
    thunk64_vkGetValidationCacheDataEXT,
    thunk64_vkInitializePerformanceApiINTEL,
    thunk64_vkInvalidateMappedMemoryRanges,
    thunk64_vkMapMemory,
    thunk64_vkMergePipelineCaches,
    thunk64_vkMergeValidationCachesEXT,
    thunk64_vkQueueBeginDebugUtilsLabelEXT,
    thunk64_vkQueueBindSparse,
    thunk64_vkQueueEndDebugUtilsLabelEXT,
    thunk64_vkQueueInsertDebugUtilsLabelEXT,
    thunk64_vkQueuePresentKHR,
    thunk64_vkQueueSetPerformanceConfigurationINTEL,
    thunk64_vkQueueSubmit,
    thunk64_vkQueueSubmit2,
    thunk64_vkQueueSubmit2KHR,
    thunk64_vkQueueWaitIdle,
    thunk64_vkReleasePerformanceConfigurationINTEL,
    thunk64_vkReleaseProfilingLockKHR,
    thunk64_vkResetCommandBuffer,
    thunk64_vkResetCommandPool,
    thunk64_vkResetDescriptorPool,
    thunk64_vkResetEvent,
    thunk64_vkResetFences,
    thunk64_vkResetQueryPool,
    thunk64_vkResetQueryPoolEXT,
    thunk64_vkSetDebugUtilsObjectNameEXT,
    thunk64_vkSetDebugUtilsObjectTagEXT,
    thunk64_vkSetDeviceMemoryPriorityEXT,
    thunk64_vkSetEvent,
    thunk64_vkSetPrivateData,
    thunk64_vkSetPrivateDataEXT,
    thunk64_vkSignalSemaphore,
    thunk64_vkSignalSemaphoreKHR,
    thunk64_vkSubmitDebugUtilsMessageEXT,
    thunk64_vkTrimCommandPool,
    thunk64_vkTrimCommandPoolKHR,
    thunk64_vkUninitializePerformanceApiINTEL,
    thunk64_vkUnmapMemory,
    thunk64_vkUpdateDescriptorSetWithTemplate,
    thunk64_vkUpdateDescriptorSetWithTemplateKHR,
    thunk64_vkUpdateDescriptorSets,
    thunk64_vkWaitForFences,
    thunk64_vkWaitForPresentKHR,
    thunk64_vkWaitSemaphores,
    thunk64_vkWaitSemaphoresKHR,
    thunk64_vkWriteAccelerationStructuresPropertiesKHR,
    thunk64_vkWriteMicromapsPropertiesEXT,
};
C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_count);

#else /* USE_STRUCT_CONVERSION) */

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    init_vulkan,
    vk_is_available_instance_function,
    vk_is_available_device_function,
    thunk32_vkAcquireNextImage2KHR,
    thunk32_vkAcquireNextImageKHR,
    thunk32_vkAcquirePerformanceConfigurationINTEL,
    thunk32_vkAcquireProfilingLockKHR,
    thunk32_vkAllocateCommandBuffers,
    thunk32_vkAllocateDescriptorSets,
    thunk32_vkAllocateMemory,
    thunk32_vkBeginCommandBuffer,
    thunk32_vkBindAccelerationStructureMemoryNV,
    thunk32_vkBindBufferMemory,
    thunk32_vkBindBufferMemory2,
    thunk32_vkBindBufferMemory2KHR,
    thunk32_vkBindImageMemory,
    thunk32_vkBindImageMemory2,
    thunk32_vkBindImageMemory2KHR,
    thunk32_vkBindOpticalFlowSessionImageNV,
    thunk32_vkBuildAccelerationStructuresKHR,
    thunk32_vkBuildMicromapsEXT,
    thunk32_vkCmdBeginConditionalRenderingEXT,
    thunk32_vkCmdBeginDebugUtilsLabelEXT,
    thunk32_vkCmdBeginQuery,
    thunk32_vkCmdBeginQueryIndexedEXT,
    thunk32_vkCmdBeginRenderPass,
    thunk32_vkCmdBeginRenderPass2,
    thunk32_vkCmdBeginRenderPass2KHR,
    thunk32_vkCmdBeginRendering,
    thunk32_vkCmdBeginRenderingKHR,
    thunk32_vkCmdBeginTransformFeedbackEXT,
    thunk32_vkCmdBindDescriptorSets,
    thunk32_vkCmdBindIndexBuffer,
    thunk32_vkCmdBindInvocationMaskHUAWEI,
    thunk32_vkCmdBindPipeline,
    thunk32_vkCmdBindPipelineShaderGroupNV,
    thunk32_vkCmdBindShadingRateImageNV,
    thunk32_vkCmdBindTransformFeedbackBuffersEXT,
    thunk32_vkCmdBindVertexBuffers,
    thunk32_vkCmdBindVertexBuffers2,
    thunk32_vkCmdBindVertexBuffers2EXT,
    thunk32_vkCmdBlitImage,
    thunk32_vkCmdBlitImage2,
    thunk32_vkCmdBlitImage2KHR,
    thunk32_vkCmdBuildAccelerationStructureNV,
    thunk32_vkCmdBuildAccelerationStructuresIndirectKHR,
    thunk32_vkCmdBuildAccelerationStructuresKHR,
    thunk32_vkCmdBuildMicromapsEXT,
    thunk32_vkCmdClearAttachments,
    thunk32_vkCmdClearColorImage,
    thunk32_vkCmdClearDepthStencilImage,
    thunk32_vkCmdCopyAccelerationStructureKHR,
    thunk32_vkCmdCopyAccelerationStructureNV,
    thunk32_vkCmdCopyAccelerationStructureToMemoryKHR,
    thunk32_vkCmdCopyBuffer,
    thunk32_vkCmdCopyBuffer2,
    thunk32_vkCmdCopyBuffer2KHR,
    thunk32_vkCmdCopyBufferToImage,
    thunk32_vkCmdCopyBufferToImage2,
    thunk32_vkCmdCopyBufferToImage2KHR,
    thunk32_vkCmdCopyImage,
    thunk32_vkCmdCopyImage2,
    thunk32_vkCmdCopyImage2KHR,
    thunk32_vkCmdCopyImageToBuffer,
    thunk32_vkCmdCopyImageToBuffer2,
    thunk32_vkCmdCopyImageToBuffer2KHR,
    thunk32_vkCmdCopyMemoryToAccelerationStructureKHR,
    thunk32_vkCmdCopyMemoryToMicromapEXT,
    thunk32_vkCmdCopyMicromapEXT,
    thunk32_vkCmdCopyMicromapToMemoryEXT,
    thunk32_vkCmdCopyQueryPoolResults,
    thunk32_vkCmdCuLaunchKernelNVX,
    thunk32_vkCmdDebugMarkerBeginEXT,
    thunk32_vkCmdDebugMarkerEndEXT,
    thunk32_vkCmdDebugMarkerInsertEXT,
    thunk32_vkCmdDispatch,
    thunk32_vkCmdDispatchBase,
    thunk32_vkCmdDispatchBaseKHR,
    thunk32_vkCmdDispatchIndirect,
    thunk32_vkCmdDraw,
    thunk32_vkCmdDrawIndexed,
    thunk32_vkCmdDrawIndexedIndirect,
    thunk32_vkCmdDrawIndexedIndirectCount,
    thunk32_vkCmdDrawIndexedIndirectCountAMD,
    thunk32_vkCmdDrawIndexedIndirectCountKHR,
    thunk32_vkCmdDrawIndirect,
    thunk32_vkCmdDrawIndirectByteCountEXT,
    thunk32_vkCmdDrawIndirectCount,
    thunk32_vkCmdDrawIndirectCountAMD,
    thunk32_vkCmdDrawIndirectCountKHR,
    thunk32_vkCmdDrawMeshTasksEXT,
    thunk32_vkCmdDrawMeshTasksIndirectCountEXT,
    thunk32_vkCmdDrawMeshTasksIndirectCountNV,
    thunk32_vkCmdDrawMeshTasksIndirectEXT,
    thunk32_vkCmdDrawMeshTasksIndirectNV,
    thunk32_vkCmdDrawMeshTasksNV,
    thunk32_vkCmdDrawMultiEXT,
    thunk32_vkCmdDrawMultiIndexedEXT,
    thunk32_vkCmdEndConditionalRenderingEXT,
    thunk32_vkCmdEndDebugUtilsLabelEXT,
    thunk32_vkCmdEndQuery,
    thunk32_vkCmdEndQueryIndexedEXT,
    thunk32_vkCmdEndRenderPass,
    thunk32_vkCmdEndRenderPass2,
    thunk32_vkCmdEndRenderPass2KHR,
    thunk32_vkCmdEndRendering,
    thunk32_vkCmdEndRenderingKHR,
    thunk32_vkCmdEndTransformFeedbackEXT,
    thunk32_vkCmdExecuteCommands,
    thunk32_vkCmdExecuteGeneratedCommandsNV,
    thunk32_vkCmdFillBuffer,
    thunk32_vkCmdInsertDebugUtilsLabelEXT,
    thunk32_vkCmdNextSubpass,
    thunk32_vkCmdNextSubpass2,
    thunk32_vkCmdNextSubpass2KHR,
    thunk32_vkCmdOpticalFlowExecuteNV,
    thunk32_vkCmdPipelineBarrier,
    thunk32_vkCmdPipelineBarrier2,
    thunk32_vkCmdPipelineBarrier2KHR,
    thunk32_vkCmdPreprocessGeneratedCommandsNV,
    thunk32_vkCmdPushConstants,
    thunk32_vkCmdPushDescriptorSetKHR,
    thunk32_vkCmdPushDescriptorSetWithTemplateKHR,
    thunk32_vkCmdResetEvent,
    thunk32_vkCmdResetEvent2,
    thunk32_vkCmdResetEvent2KHR,
    thunk32_vkCmdResetQueryPool,
    thunk32_vkCmdResolveImage,
    thunk32_vkCmdResolveImage2,
    thunk32_vkCmdResolveImage2KHR,
    thunk32_vkCmdSetAlphaToCoverageEnableEXT,
    thunk32_vkCmdSetAlphaToOneEnableEXT,
    thunk32_vkCmdSetBlendConstants,
    thunk32_vkCmdSetCheckpointNV,
    thunk32_vkCmdSetCoarseSampleOrderNV,
    thunk32_vkCmdSetColorBlendAdvancedEXT,
    thunk32_vkCmdSetColorBlendEnableEXT,
    thunk32_vkCmdSetColorBlendEquationEXT,
    thunk32_vkCmdSetColorWriteEnableEXT,
    thunk32_vkCmdSetColorWriteMaskEXT,
    thunk32_vkCmdSetConservativeRasterizationModeEXT,
    thunk32_vkCmdSetCoverageModulationModeNV,
    thunk32_vkCmdSetCoverageModulationTableEnableNV,
    thunk32_vkCmdSetCoverageModulationTableNV,
    thunk32_vkCmdSetCoverageReductionModeNV,
    thunk32_vkCmdSetCoverageToColorEnableNV,
    thunk32_vkCmdSetCoverageToColorLocationNV,
    thunk32_vkCmdSetCullMode,
    thunk32_vkCmdSetCullModeEXT,
    thunk32_vkCmdSetDepthBias,
    thunk32_vkCmdSetDepthBiasEnable,
    thunk32_vkCmdSetDepthBiasEnableEXT,
    thunk32_vkCmdSetDepthBounds,
    thunk32_vkCmdSetDepthBoundsTestEnable,
    thunk32_vkCmdSetDepthBoundsTestEnableEXT,
    thunk32_vkCmdSetDepthClampEnableEXT,
    thunk32_vkCmdSetDepthClipEnableEXT,
    thunk32_vkCmdSetDepthClipNegativeOneToOneEXT,
    thunk32_vkCmdSetDepthCompareOp,
    thunk32_vkCmdSetDepthCompareOpEXT,
    thunk32_vkCmdSetDepthTestEnable,
    thunk32_vkCmdSetDepthTestEnableEXT,
    thunk32_vkCmdSetDepthWriteEnable,
    thunk32_vkCmdSetDepthWriteEnableEXT,
    thunk32_vkCmdSetDeviceMask,
    thunk32_vkCmdSetDeviceMaskKHR,
    thunk32_vkCmdSetDiscardRectangleEXT,
    thunk32_vkCmdSetEvent,
    thunk32_vkCmdSetEvent2,
    thunk32_vkCmdSetEvent2KHR,
    thunk32_vkCmdSetExclusiveScissorNV,
    thunk32_vkCmdSetExtraPrimitiveOverestimationSizeEXT,
    thunk32_vkCmdSetFragmentShadingRateEnumNV,
    thunk32_vkCmdSetFragmentShadingRateKHR,
    thunk32_vkCmdSetFrontFace,
    thunk32_vkCmdSetFrontFaceEXT,
    thunk32_vkCmdSetLineRasterizationModeEXT,
    thunk32_vkCmdSetLineStippleEXT,
    thunk32_vkCmdSetLineStippleEnableEXT,
    thunk32_vkCmdSetLineWidth,
    thunk32_vkCmdSetLogicOpEXT,
    thunk32_vkCmdSetLogicOpEnableEXT,
    thunk32_vkCmdSetPatchControlPointsEXT,
    thunk32_vkCmdSetPerformanceMarkerINTEL,
    thunk32_vkCmdSetPerformanceOverrideINTEL,
    thunk32_vkCmdSetPerformanceStreamMarkerINTEL,
    thunk32_vkCmdSetPolygonModeEXT,
    thunk32_vkCmdSetPrimitiveRestartEnable,
    thunk32_vkCmdSetPrimitiveRestartEnableEXT,
    thunk32_vkCmdSetPrimitiveTopology,
    thunk32_vkCmdSetPrimitiveTopologyEXT,
    thunk32_vkCmdSetProvokingVertexModeEXT,
    thunk32_vkCmdSetRasterizationSamplesEXT,
    thunk32_vkCmdSetRasterizationStreamEXT,
    thunk32_vkCmdSetRasterizerDiscardEnable,
    thunk32_vkCmdSetRasterizerDiscardEnableEXT,
    thunk32_vkCmdSetRayTracingPipelineStackSizeKHR,
    thunk32_vkCmdSetRepresentativeFragmentTestEnableNV,
    thunk32_vkCmdSetSampleLocationsEXT,
    thunk32_vkCmdSetSampleLocationsEnableEXT,
    thunk32_vkCmdSetSampleMaskEXT,
    thunk32_vkCmdSetScissor,
    thunk32_vkCmdSetScissorWithCount,
    thunk32_vkCmdSetScissorWithCountEXT,
    thunk32_vkCmdSetShadingRateImageEnableNV,
    thunk32_vkCmdSetStencilCompareMask,
    thunk32_vkCmdSetStencilOp,
    thunk32_vkCmdSetStencilOpEXT,
    thunk32_vkCmdSetStencilReference,
    thunk32_vkCmdSetStencilTestEnable,
    thunk32_vkCmdSetStencilTestEnableEXT,
    thunk32_vkCmdSetStencilWriteMask,
    thunk32_vkCmdSetTessellationDomainOriginEXT,
    thunk32_vkCmdSetVertexInputEXT,
    thunk32_vkCmdSetViewport,
    thunk32_vkCmdSetViewportShadingRatePaletteNV,
    thunk32_vkCmdSetViewportSwizzleNV,
    thunk32_vkCmdSetViewportWScalingEnableNV,
    thunk32_vkCmdSetViewportWScalingNV,
    thunk32_vkCmdSetViewportWithCount,
    thunk32_vkCmdSetViewportWithCountEXT,
    thunk32_vkCmdSubpassShadingHUAWEI,
    thunk32_vkCmdTraceRaysIndirect2KHR,
    thunk32_vkCmdTraceRaysIndirectKHR,
    thunk32_vkCmdTraceRaysKHR,
    thunk32_vkCmdTraceRaysNV,
    thunk32_vkCmdUpdateBuffer,
    thunk32_vkCmdWaitEvents,
    thunk32_vkCmdWaitEvents2,
    thunk32_vkCmdWaitEvents2KHR,
    thunk32_vkCmdWriteAccelerationStructuresPropertiesKHR,
    thunk32_vkCmdWriteAccelerationStructuresPropertiesNV,
    thunk32_vkCmdWriteBufferMarker2AMD,
    thunk32_vkCmdWriteBufferMarkerAMD,
    thunk32_vkCmdWriteMicromapsPropertiesEXT,
    thunk32_vkCmdWriteTimestamp,
    thunk32_vkCmdWriteTimestamp2,
    thunk32_vkCmdWriteTimestamp2KHR,
    thunk32_vkCompileDeferredNV,
    thunk32_vkCopyAccelerationStructureKHR,
    thunk32_vkCopyAccelerationStructureToMemoryKHR,
    thunk32_vkCopyMemoryToAccelerationStructureKHR,
    thunk32_vkCopyMemoryToMicromapEXT,
    thunk32_vkCopyMicromapEXT,
    thunk32_vkCopyMicromapToMemoryEXT,
    thunk32_vkCreateAccelerationStructureKHR,
    thunk32_vkCreateAccelerationStructureNV,
    thunk32_vkCreateBuffer,
    thunk32_vkCreateBufferView,
    thunk32_vkCreateCommandPool,
    thunk32_vkCreateComputePipelines,
    thunk32_vkCreateCuFunctionNVX,
    thunk32_vkCreateCuModuleNVX,
    thunk32_vkCreateDebugReportCallbackEXT,
    thunk32_vkCreateDebugUtilsMessengerEXT,
    thunk32_vkCreateDeferredOperationKHR,
    thunk32_vkCreateDescriptorPool,
    thunk32_vkCreateDescriptorSetLayout,
    thunk32_vkCreateDescriptorUpdateTemplate,
    thunk32_vkCreateDescriptorUpdateTemplateKHR,
    thunk32_vkCreateDevice,
    thunk32_vkCreateEvent,
    thunk32_vkCreateFence,
    thunk32_vkCreateFramebuffer,
    thunk32_vkCreateGraphicsPipelines,
    thunk32_vkCreateImage,
    thunk32_vkCreateImageView,
    thunk32_vkCreateIndirectCommandsLayoutNV,
    thunk32_vkCreateInstance,
    thunk32_vkCreateMicromapEXT,
    thunk32_vkCreateOpticalFlowSessionNV,
    thunk32_vkCreatePipelineCache,
    thunk32_vkCreatePipelineLayout,
    thunk32_vkCreatePrivateDataSlot,
    thunk32_vkCreatePrivateDataSlotEXT,
    thunk32_vkCreateQueryPool,
    thunk32_vkCreateRayTracingPipelinesKHR,
    thunk32_vkCreateRayTracingPipelinesNV,
    thunk32_vkCreateRenderPass,
    thunk32_vkCreateRenderPass2,
    thunk32_vkCreateRenderPass2KHR,
    thunk32_vkCreateSampler,
    thunk32_vkCreateSamplerYcbcrConversion,
    thunk32_vkCreateSamplerYcbcrConversionKHR,
    thunk32_vkCreateSemaphore,
    thunk32_vkCreateShaderModule,
    thunk32_vkCreateSwapchainKHR,
    thunk32_vkCreateValidationCacheEXT,
    thunk32_vkCreateWin32SurfaceKHR,
    thunk32_vkDebugMarkerSetObjectNameEXT,
    thunk32_vkDebugMarkerSetObjectTagEXT,
    thunk32_vkDebugReportMessageEXT,
    thunk32_vkDeferredOperationJoinKHR,
    thunk32_vkDestroyAccelerationStructureKHR,
    thunk32_vkDestroyAccelerationStructureNV,
    thunk32_vkDestroyBuffer,
    thunk32_vkDestroyBufferView,
    thunk32_vkDestroyCommandPool,
    thunk32_vkDestroyCuFunctionNVX,
    thunk32_vkDestroyCuModuleNVX,
    thunk32_vkDestroyDebugReportCallbackEXT,
    thunk32_vkDestroyDebugUtilsMessengerEXT,
    thunk32_vkDestroyDeferredOperationKHR,
    thunk32_vkDestroyDescriptorPool,
    thunk32_vkDestroyDescriptorSetLayout,
    thunk32_vkDestroyDescriptorUpdateTemplate,
    thunk32_vkDestroyDescriptorUpdateTemplateKHR,
    thunk32_vkDestroyDevice,
    thunk32_vkDestroyEvent,
    thunk32_vkDestroyFence,
    thunk32_vkDestroyFramebuffer,
    thunk32_vkDestroyImage,
    thunk32_vkDestroyImageView,
    thunk32_vkDestroyIndirectCommandsLayoutNV,
    thunk32_vkDestroyInstance,
    thunk32_vkDestroyMicromapEXT,
    thunk32_vkDestroyOpticalFlowSessionNV,
    thunk32_vkDestroyPipeline,
    thunk32_vkDestroyPipelineCache,
    thunk32_vkDestroyPipelineLayout,
    thunk32_vkDestroyPrivateDataSlot,
    thunk32_vkDestroyPrivateDataSlotEXT,
    thunk32_vkDestroyQueryPool,
    thunk32_vkDestroyRenderPass,
    thunk32_vkDestroySampler,
    thunk32_vkDestroySamplerYcbcrConversion,
    thunk32_vkDestroySamplerYcbcrConversionKHR,
    thunk32_vkDestroySemaphore,
    thunk32_vkDestroyShaderModule,
    thunk32_vkDestroySurfaceKHR,
    thunk32_vkDestroySwapchainKHR,
    thunk32_vkDestroyValidationCacheEXT,
    thunk32_vkDeviceWaitIdle,
    thunk32_vkEndCommandBuffer,
    thunk32_vkEnumerateDeviceExtensionProperties,
    thunk32_vkEnumerateDeviceLayerProperties,
    thunk32_vkEnumerateInstanceExtensionProperties,
    thunk32_vkEnumerateInstanceVersion,
    thunk32_vkEnumeratePhysicalDeviceGroups,
    thunk32_vkEnumeratePhysicalDeviceGroupsKHR,
    thunk32_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR,
    thunk32_vkEnumeratePhysicalDevices,
    thunk32_vkFlushMappedMemoryRanges,
    thunk32_vkFreeCommandBuffers,
    thunk32_vkFreeDescriptorSets,
    thunk32_vkFreeMemory,
    thunk32_vkGetAccelerationStructureBuildSizesKHR,
    thunk32_vkGetAccelerationStructureDeviceAddressKHR,
    thunk32_vkGetAccelerationStructureHandleNV,
    thunk32_vkGetAccelerationStructureMemoryRequirementsNV,
    thunk32_vkGetBufferDeviceAddress,
    thunk32_vkGetBufferDeviceAddressEXT,
    thunk32_vkGetBufferDeviceAddressKHR,
    thunk32_vkGetBufferMemoryRequirements,
    thunk32_vkGetBufferMemoryRequirements2,
    thunk32_vkGetBufferMemoryRequirements2KHR,
    thunk32_vkGetBufferOpaqueCaptureAddress,
    thunk32_vkGetBufferOpaqueCaptureAddressKHR,
    thunk32_vkGetCalibratedTimestampsEXT,
    thunk32_vkGetDeferredOperationMaxConcurrencyKHR,
    thunk32_vkGetDeferredOperationResultKHR,
    thunk32_vkGetDescriptorSetHostMappingVALVE,
    thunk32_vkGetDescriptorSetLayoutHostMappingInfoVALVE,
    thunk32_vkGetDescriptorSetLayoutSupport,
    thunk32_vkGetDescriptorSetLayoutSupportKHR,
    thunk32_vkGetDeviceAccelerationStructureCompatibilityKHR,
    thunk32_vkGetDeviceBufferMemoryRequirements,
    thunk32_vkGetDeviceBufferMemoryRequirementsKHR,
    thunk32_vkGetDeviceFaultInfoEXT,
    thunk32_vkGetDeviceGroupPeerMemoryFeatures,
    thunk32_vkGetDeviceGroupPeerMemoryFeaturesKHR,
    thunk32_vkGetDeviceGroupPresentCapabilitiesKHR,
    thunk32_vkGetDeviceGroupSurfacePresentModesKHR,
    thunk32_vkGetDeviceImageMemoryRequirements,
    thunk32_vkGetDeviceImageMemoryRequirementsKHR,
    thunk32_vkGetDeviceImageSparseMemoryRequirements,
    thunk32_vkGetDeviceImageSparseMemoryRequirementsKHR,
    thunk32_vkGetDeviceMemoryCommitment,
    thunk32_vkGetDeviceMemoryOpaqueCaptureAddress,
    thunk32_vkGetDeviceMemoryOpaqueCaptureAddressKHR,
    thunk32_vkGetDeviceMicromapCompatibilityEXT,
    thunk32_vkGetDeviceQueue,
    thunk32_vkGetDeviceQueue2,
    thunk32_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI,
    thunk32_vkGetDynamicRenderingTilePropertiesQCOM,
    thunk32_vkGetEventStatus,
    thunk32_vkGetFenceStatus,
    thunk32_vkGetFramebufferTilePropertiesQCOM,
    thunk32_vkGetGeneratedCommandsMemoryRequirementsNV,
    thunk32_vkGetImageMemoryRequirements,
    thunk32_vkGetImageMemoryRequirements2,
    thunk32_vkGetImageMemoryRequirements2KHR,
    thunk32_vkGetImageSparseMemoryRequirements,
    thunk32_vkGetImageSparseMemoryRequirements2,
    thunk32_vkGetImageSparseMemoryRequirements2KHR,
    thunk32_vkGetImageSubresourceLayout,
    thunk32_vkGetImageSubresourceLayout2EXT,
    thunk32_vkGetImageViewAddressNVX,
    thunk32_vkGetImageViewHandleNVX,
    thunk32_vkGetMemoryHostPointerPropertiesEXT,
    thunk32_vkGetMicromapBuildSizesEXT,
    thunk32_vkGetPerformanceParameterINTEL,
    thunk32_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
    thunk32_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV,
    thunk32_vkGetPhysicalDeviceExternalBufferProperties,
    thunk32_vkGetPhysicalDeviceExternalBufferPropertiesKHR,
    thunk32_vkGetPhysicalDeviceExternalFenceProperties,
    thunk32_vkGetPhysicalDeviceExternalFencePropertiesKHR,
    thunk32_vkGetPhysicalDeviceExternalSemaphoreProperties,
    thunk32_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR,
    thunk32_vkGetPhysicalDeviceFeatures,
    thunk32_vkGetPhysicalDeviceFeatures2,
    thunk32_vkGetPhysicalDeviceFeatures2KHR,
    thunk32_vkGetPhysicalDeviceFormatProperties,
    thunk32_vkGetPhysicalDeviceFormatProperties2,
    thunk32_vkGetPhysicalDeviceFormatProperties2KHR,
    thunk32_vkGetPhysicalDeviceFragmentShadingRatesKHR,
    thunk32_vkGetPhysicalDeviceImageFormatProperties,
    thunk32_vkGetPhysicalDeviceImageFormatProperties2,
    thunk32_vkGetPhysicalDeviceImageFormatProperties2KHR,
    thunk32_vkGetPhysicalDeviceMemoryProperties,
    thunk32_vkGetPhysicalDeviceMemoryProperties2,
    thunk32_vkGetPhysicalDeviceMemoryProperties2KHR,
    thunk32_vkGetPhysicalDeviceMultisamplePropertiesEXT,
    thunk32_vkGetPhysicalDeviceOpticalFlowImageFormatsNV,
    thunk32_vkGetPhysicalDevicePresentRectanglesKHR,
    thunk32_vkGetPhysicalDeviceProperties,
    thunk32_vkGetPhysicalDeviceProperties2,
    thunk32_vkGetPhysicalDeviceProperties2KHR,
    thunk32_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR,
    thunk32_vkGetPhysicalDeviceQueueFamilyProperties,
    thunk32_vkGetPhysicalDeviceQueueFamilyProperties2,
    thunk32_vkGetPhysicalDeviceQueueFamilyProperties2KHR,
    thunk32_vkGetPhysicalDeviceSparseImageFormatProperties,
    thunk32_vkGetPhysicalDeviceSparseImageFormatProperties2,
    thunk32_vkGetPhysicalDeviceSparseImageFormatProperties2KHR,
    thunk32_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV,
    thunk32_vkGetPhysicalDeviceSurfaceCapabilities2KHR,
    thunk32_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
    thunk32_vkGetPhysicalDeviceSurfaceFormats2KHR,
    thunk32_vkGetPhysicalDeviceSurfaceFormatsKHR,
    thunk32_vkGetPhysicalDeviceSurfacePresentModesKHR,
    thunk32_vkGetPhysicalDeviceSurfaceSupportKHR,
    thunk32_vkGetPhysicalDeviceToolProperties,
    thunk32_vkGetPhysicalDeviceToolPropertiesEXT,
    thunk32_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    thunk32_vkGetPipelineCacheData,
    thunk32_vkGetPipelineExecutableInternalRepresentationsKHR,
    thunk32_vkGetPipelineExecutablePropertiesKHR,
    thunk32_vkGetPipelineExecutableStatisticsKHR,
    thunk32_vkGetPipelinePropertiesEXT,
    thunk32_vkGetPrivateData,
    thunk32_vkGetPrivateDataEXT,
    thunk32_vkGetQueryPoolResults,
    thunk32_vkGetQueueCheckpointData2NV,
    thunk32_vkGetQueueCheckpointDataNV,
    thunk32_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR,
    thunk32_vkGetRayTracingShaderGroupHandlesKHR,
    thunk32_vkGetRayTracingShaderGroupHandlesNV,
    thunk32_vkGetRayTracingShaderGroupStackSizeKHR,
    thunk32_vkGetRenderAreaGranularity,
    thunk32_vkGetSemaphoreCounterValue,
    thunk32_vkGetSemaphoreCounterValueKHR,
    thunk32_vkGetShaderInfoAMD,
    thunk32_vkGetShaderModuleCreateInfoIdentifierEXT,
    thunk32_vkGetShaderModuleIdentifierEXT,
    thunk32_vkGetSwapchainImagesKHR,
    thunk32_vkGetValidationCacheDataEXT,
    thunk32_vkInitializePerformanceApiINTEL,
    thunk32_vkInvalidateMappedMemoryRanges,
    thunk32_vkMapMemory,
    thunk32_vkMergePipelineCaches,
    thunk32_vkMergeValidationCachesEXT,
    thunk32_vkQueueBeginDebugUtilsLabelEXT,
    thunk32_vkQueueBindSparse,
    thunk32_vkQueueEndDebugUtilsLabelEXT,
    thunk32_vkQueueInsertDebugUtilsLabelEXT,
    thunk32_vkQueuePresentKHR,
    thunk32_vkQueueSetPerformanceConfigurationINTEL,
    thunk32_vkQueueSubmit,
    thunk32_vkQueueSubmit2,
    thunk32_vkQueueSubmit2KHR,
    thunk32_vkQueueWaitIdle,
    thunk32_vkReleasePerformanceConfigurationINTEL,
    thunk32_vkReleaseProfilingLockKHR,
    thunk32_vkResetCommandBuffer,
    thunk32_vkResetCommandPool,
    thunk32_vkResetDescriptorPool,
    thunk32_vkResetEvent,
    thunk32_vkResetFences,
    thunk32_vkResetQueryPool,
    thunk32_vkResetQueryPoolEXT,
    thunk32_vkSetDebugUtilsObjectNameEXT,
    thunk32_vkSetDebugUtilsObjectTagEXT,
    thunk32_vkSetDeviceMemoryPriorityEXT,
    thunk32_vkSetEvent,
    thunk32_vkSetPrivateData,
    thunk32_vkSetPrivateDataEXT,
    thunk32_vkSignalSemaphore,
    thunk32_vkSignalSemaphoreKHR,
    thunk32_vkSubmitDebugUtilsMessageEXT,
    thunk32_vkTrimCommandPool,
    thunk32_vkTrimCommandPoolKHR,
    thunk32_vkUninitializePerformanceApiINTEL,
    thunk32_vkUnmapMemory,
    thunk32_vkUpdateDescriptorSetWithTemplate,
    thunk32_vkUpdateDescriptorSetWithTemplateKHR,
    thunk32_vkUpdateDescriptorSets,
    thunk32_vkWaitForFences,
    thunk32_vkWaitForPresentKHR,
    thunk32_vkWaitSemaphores,
    thunk32_vkWaitSemaphoresKHR,
    thunk32_vkWriteAccelerationStructuresPropertiesKHR,
    thunk32_vkWriteMicromapsPropertiesEXT,
};
C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_count);

#endif /* USE_STRUCT_CONVERSION) */

NTSTATUS WINAPI vk_direct_unix_call(unixlib_handle_t handle, unsigned int code, void *params)
{
    return __wine_unix_call_funcs[code](params);
}
