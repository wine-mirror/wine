/* Automatically generated from Vulkan vk.xml and video.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2015-2025 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * and from Vulkan video.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2021-2025 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "vulkan_loader.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

VkResult WINAPI vkAcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex)
{
    struct vkAcquireNextImage2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pAcquireInfo = pAcquireInfo;
    params.pImageIndex = pImageIndex;
    status = UNIX_CALL(vkAcquireNextImage2KHR, &params);
    assert(!status && "vkAcquireNextImage2KHR");
    return params.result;
}

VkResult WINAPI vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
{
    struct vkAcquireNextImageKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.timeout = timeout;
    params.semaphore = semaphore;
    params.fence = fence;
    params.pImageIndex = pImageIndex;
    status = UNIX_CALL(vkAcquireNextImageKHR, &params);
    assert(!status && "vkAcquireNextImageKHR");
    return params.result;
}

VkResult WINAPI vkAcquirePerformanceConfigurationINTEL(VkDevice device, const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo, VkPerformanceConfigurationINTEL *pConfiguration)
{
    struct vkAcquirePerformanceConfigurationINTEL_params params;
    NTSTATUS status;
    params.device = device;
    params.pAcquireInfo = pAcquireInfo;
    params.pConfiguration = pConfiguration;
    status = UNIX_CALL(vkAcquirePerformanceConfigurationINTEL, &params);
    assert(!status && "vkAcquirePerformanceConfigurationINTEL");
    return params.result;
}

VkResult WINAPI vkAcquireProfilingLockKHR(VkDevice device, const VkAcquireProfilingLockInfoKHR *pInfo)
{
    struct vkAcquireProfilingLockKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkAcquireProfilingLockKHR, &params);
    assert(!status && "vkAcquireProfilingLockKHR");
    return params.result;
}

VkResult WINAPI vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets)
{
    struct vkAllocateDescriptorSets_params params;
    NTSTATUS status;
    params.device = device;
    params.pAllocateInfo = pAllocateInfo;
    params.pDescriptorSets = pDescriptorSets;
    status = UNIX_CALL(vkAllocateDescriptorSets, &params);
    assert(!status && "vkAllocateDescriptorSets");
    return params.result;
}

VkResult WINAPI vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory)
{
    struct vkAllocateMemory_params params;
    NTSTATUS status;
    params.device = device;
    params.pAllocateInfo = pAllocateInfo;
    params.pAllocator = pAllocator;
    params.pMemory = pMemory;
    status = UNIX_CALL(vkAllocateMemory, &params);
    assert(!status && "vkAllocateMemory");
    return params.result;
}

void WINAPI vkAntiLagUpdateAMD(VkDevice device, const VkAntiLagDataAMD *pData)
{
    struct vkAntiLagUpdateAMD_params params;
    NTSTATUS status;
    params.device = device;
    params.pData = pData;
    status = UNIX_CALL(vkAntiLagUpdateAMD, &params);
    assert(!status && "vkAntiLagUpdateAMD");
}

VkResult WINAPI vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)
{
    struct vkBeginCommandBuffer_params params;
    NTSTATUS status;
    params.commandBuffer = commandBuffer;
    params.pBeginInfo = pBeginInfo;
    status = UNIX_CALL(vkBeginCommandBuffer, &params);
    assert(!status && "vkBeginCommandBuffer");
    return params.result;
}

VkResult WINAPI vkBindAccelerationStructureMemoryNV(VkDevice device, uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV *pBindInfos)
{
    struct vkBindAccelerationStructureMemoryNV_params params;
    NTSTATUS status;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    status = UNIX_CALL(vkBindAccelerationStructureMemoryNV, &params);
    assert(!status && "vkBindAccelerationStructureMemoryNV");
    return params.result;
}

VkResult WINAPI vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    struct vkBindBufferMemory_params params;
    NTSTATUS status;
    params.device = device;
    params.buffer = buffer;
    params.memory = memory;
    params.memoryOffset = memoryOffset;
    status = UNIX_CALL(vkBindBufferMemory, &params);
    assert(!status && "vkBindBufferMemory");
    return params.result;
}

VkResult WINAPI vkBindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    struct vkBindBufferMemory2_params params;
    NTSTATUS status;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    status = UNIX_CALL(vkBindBufferMemory2, &params);
    assert(!status && "vkBindBufferMemory2");
    return params.result;
}

VkResult WINAPI vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos)
{
    struct vkBindBufferMemory2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    status = UNIX_CALL(vkBindBufferMemory2KHR, &params);
    assert(!status && "vkBindBufferMemory2KHR");
    return params.result;
}

VkResult WINAPI vkBindDataGraphPipelineSessionMemoryARM(VkDevice device, uint32_t bindInfoCount, const VkBindDataGraphPipelineSessionMemoryInfoARM *pBindInfos)
{
    struct vkBindDataGraphPipelineSessionMemoryARM_params params;
    NTSTATUS status;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    status = UNIX_CALL(vkBindDataGraphPipelineSessionMemoryARM, &params);
    assert(!status && "vkBindDataGraphPipelineSessionMemoryARM");
    return params.result;
}

VkResult WINAPI vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    struct vkBindImageMemory_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.memory = memory;
    params.memoryOffset = memoryOffset;
    status = UNIX_CALL(vkBindImageMemory, &params);
    assert(!status && "vkBindImageMemory");
    return params.result;
}

VkResult WINAPI vkBindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    struct vkBindImageMemory2_params params;
    NTSTATUS status;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    status = UNIX_CALL(vkBindImageMemory2, &params);
    assert(!status && "vkBindImageMemory2");
    return params.result;
}

VkResult WINAPI vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
{
    struct vkBindImageMemory2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    status = UNIX_CALL(vkBindImageMemory2KHR, &params);
    assert(!status && "vkBindImageMemory2KHR");
    return params.result;
}

VkResult WINAPI vkBindOpticalFlowSessionImageNV(VkDevice device, VkOpticalFlowSessionNV session, VkOpticalFlowSessionBindingPointNV bindingPoint, VkImageView view, VkImageLayout layout)
{
    struct vkBindOpticalFlowSessionImageNV_params params;
    NTSTATUS status;
    params.device = device;
    params.session = session;
    params.bindingPoint = bindingPoint;
    params.view = view;
    params.layout = layout;
    status = UNIX_CALL(vkBindOpticalFlowSessionImageNV, &params);
    assert(!status && "vkBindOpticalFlowSessionImageNV");
    return params.result;
}

VkResult WINAPI vkBindTensorMemoryARM(VkDevice device, uint32_t bindInfoCount, const VkBindTensorMemoryInfoARM *pBindInfos)
{
    struct vkBindTensorMemoryARM_params params;
    NTSTATUS status;
    params.device = device;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfos = pBindInfos;
    status = UNIX_CALL(vkBindTensorMemoryARM, &params);
    assert(!status && "vkBindTensorMemoryARM");
    return params.result;
}

VkResult WINAPI vkBindVideoSessionMemoryKHR(VkDevice device, VkVideoSessionKHR videoSession, uint32_t bindSessionMemoryInfoCount, const VkBindVideoSessionMemoryInfoKHR *pBindSessionMemoryInfos)
{
    struct vkBindVideoSessionMemoryKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.videoSession = videoSession;
    params.bindSessionMemoryInfoCount = bindSessionMemoryInfoCount;
    params.pBindSessionMemoryInfos = pBindSessionMemoryInfos;
    status = UNIX_CALL(vkBindVideoSessionMemoryKHR, &params);
    assert(!status && "vkBindVideoSessionMemoryKHR");
    return params.result;
}

VkResult WINAPI vkBuildAccelerationStructuresKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    struct vkBuildAccelerationStructuresKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    params.ppBuildRangeInfos = ppBuildRangeInfos;
    status = UNIX_CALL(vkBuildAccelerationStructuresKHR, &params);
    assert(!status && "vkBuildAccelerationStructuresKHR");
    return params.result;
}

VkResult WINAPI vkBuildMicromapsEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount, const VkMicromapBuildInfoEXT *pInfos)
{
    struct vkBuildMicromapsEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    status = UNIX_CALL(vkBuildMicromapsEXT, &params);
    assert(!status && "vkBuildMicromapsEXT");
    return params.result;
}

void WINAPI vkCmdBeginConditionalRenderingEXT(VkCommandBuffer commandBuffer, const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin)
{
    struct vkCmdBeginConditionalRenderingEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pConditionalRenderingBegin = pConditionalRenderingBegin;
    UNIX_CALL(vkCmdBeginConditionalRenderingEXT, &params);
}

void WINAPI vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkCmdBeginDebugUtilsLabelEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pLabelInfo = pLabelInfo;
    UNIX_CALL(vkCmdBeginDebugUtilsLabelEXT, &params);
}

void WINAPI vkCmdBeginPerTileExecutionQCOM(VkCommandBuffer commandBuffer, const VkPerTileBeginInfoQCOM *pPerTileBeginInfo)
{
    struct vkCmdBeginPerTileExecutionQCOM_params params;
    params.commandBuffer = commandBuffer;
    params.pPerTileBeginInfo = pPerTileBeginInfo;
    UNIX_CALL(vkCmdBeginPerTileExecutionQCOM, &params);
}

void WINAPI vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    struct vkCmdBeginQuery_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    params.flags = flags;
    UNIX_CALL(vkCmdBeginQuery, &params);
}

void WINAPI vkCmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index)
{
    struct vkCmdBeginQueryIndexedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    params.flags = flags;
    params.index = index;
    UNIX_CALL(vkCmdBeginQueryIndexedEXT, &params);
}

void WINAPI vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    struct vkCmdBeginRenderPass_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderPassBegin = pRenderPassBegin;
    params.contents = contents;
    UNIX_CALL(vkCmdBeginRenderPass, &params);
}

void WINAPI vkCmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    struct vkCmdBeginRenderPass2_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderPassBegin = pRenderPassBegin;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    UNIX_CALL(vkCmdBeginRenderPass2, &params);
}

void WINAPI vkCmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo)
{
    struct vkCmdBeginRenderPass2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderPassBegin = pRenderPassBegin;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    UNIX_CALL(vkCmdBeginRenderPass2KHR, &params);
}

void WINAPI vkCmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo)
{
    struct vkCmdBeginRendering_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderingInfo = pRenderingInfo;
    UNIX_CALL(vkCmdBeginRendering, &params);
}

void WINAPI vkCmdBeginRenderingKHR(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo)
{
    struct vkCmdBeginRenderingKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderingInfo = pRenderingInfo;
    UNIX_CALL(vkCmdBeginRenderingKHR, &params);
}

void WINAPI vkCmdBeginTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    struct vkCmdBeginTransformFeedbackEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstCounterBuffer = firstCounterBuffer;
    params.counterBufferCount = counterBufferCount;
    params.pCounterBuffers = pCounterBuffers;
    params.pCounterBufferOffsets = pCounterBufferOffsets;
    UNIX_CALL(vkCmdBeginTransformFeedbackEXT, &params);
}

void WINAPI vkCmdBeginVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoBeginCodingInfoKHR *pBeginInfo)
{
    struct vkCmdBeginVideoCodingKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pBeginInfo = pBeginInfo;
    UNIX_CALL(vkCmdBeginVideoCodingKHR, &params);
}

void WINAPI vkCmdBindDescriptorBufferEmbeddedSamplers2EXT(VkCommandBuffer commandBuffer, const VkBindDescriptorBufferEmbeddedSamplersInfoEXT *pBindDescriptorBufferEmbeddedSamplersInfo)
{
    struct vkCmdBindDescriptorBufferEmbeddedSamplers2EXT_params params;
    params.commandBuffer = commandBuffer;
    params.pBindDescriptorBufferEmbeddedSamplersInfo = pBindDescriptorBufferEmbeddedSamplersInfo;
    UNIX_CALL(vkCmdBindDescriptorBufferEmbeddedSamplers2EXT, &params);
}

void WINAPI vkCmdBindDescriptorBufferEmbeddedSamplersEXT(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set)
{
    struct vkCmdBindDescriptorBufferEmbeddedSamplersEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.layout = layout;
    params.set = set;
    UNIX_CALL(vkCmdBindDescriptorBufferEmbeddedSamplersEXT, &params);
}

void WINAPI vkCmdBindDescriptorBuffersEXT(VkCommandBuffer commandBuffer, uint32_t bufferCount, const VkDescriptorBufferBindingInfoEXT *pBindingInfos)
{
    struct vkCmdBindDescriptorBuffersEXT_params params;
    params.commandBuffer = commandBuffer;
    params.bufferCount = bufferCount;
    params.pBindingInfos = pBindingInfos;
    UNIX_CALL(vkCmdBindDescriptorBuffersEXT, &params);
}

void WINAPI vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
{
    struct vkCmdBindDescriptorSets_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.layout = layout;
    params.firstSet = firstSet;
    params.descriptorSetCount = descriptorSetCount;
    params.pDescriptorSets = pDescriptorSets;
    params.dynamicOffsetCount = dynamicOffsetCount;
    params.pDynamicOffsets = pDynamicOffsets;
    UNIX_CALL(vkCmdBindDescriptorSets, &params);
}

void WINAPI vkCmdBindDescriptorSets2(VkCommandBuffer commandBuffer, const VkBindDescriptorSetsInfo *pBindDescriptorSetsInfo)
{
    struct vkCmdBindDescriptorSets2_params params;
    params.commandBuffer = commandBuffer;
    params.pBindDescriptorSetsInfo = pBindDescriptorSetsInfo;
    UNIX_CALL(vkCmdBindDescriptorSets2, &params);
}

void WINAPI vkCmdBindDescriptorSets2KHR(VkCommandBuffer commandBuffer, const VkBindDescriptorSetsInfo *pBindDescriptorSetsInfo)
{
    struct vkCmdBindDescriptorSets2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pBindDescriptorSetsInfo = pBindDescriptorSetsInfo;
    UNIX_CALL(vkCmdBindDescriptorSets2KHR, &params);
}

void WINAPI vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    struct vkCmdBindIndexBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.indexType = indexType;
    UNIX_CALL(vkCmdBindIndexBuffer, &params);
}

void WINAPI vkCmdBindIndexBuffer2(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkIndexType indexType)
{
    struct vkCmdBindIndexBuffer2_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.size = size;
    params.indexType = indexType;
    UNIX_CALL(vkCmdBindIndexBuffer2, &params);
}

void WINAPI vkCmdBindIndexBuffer2KHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkIndexType indexType)
{
    struct vkCmdBindIndexBuffer2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.size = size;
    params.indexType = indexType;
    UNIX_CALL(vkCmdBindIndexBuffer2KHR, &params);
}

void WINAPI vkCmdBindInvocationMaskHUAWEI(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout)
{
    struct vkCmdBindInvocationMaskHUAWEI_params params;
    params.commandBuffer = commandBuffer;
    params.imageView = imageView;
    params.imageLayout = imageLayout;
    UNIX_CALL(vkCmdBindInvocationMaskHUAWEI, &params);
}

void WINAPI vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    struct vkCmdBindPipeline_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.pipeline = pipeline;
    UNIX_CALL(vkCmdBindPipeline, &params);
}

void WINAPI vkCmdBindPipelineShaderGroupNV(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline, uint32_t groupIndex)
{
    struct vkCmdBindPipelineShaderGroupNV_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.pipeline = pipeline;
    params.groupIndex = groupIndex;
    UNIX_CALL(vkCmdBindPipelineShaderGroupNV, &params);
}

void WINAPI vkCmdBindShadersEXT(VkCommandBuffer commandBuffer, uint32_t stageCount, const VkShaderStageFlagBits *pStages, const VkShaderEXT *pShaders)
{
    struct vkCmdBindShadersEXT_params params;
    params.commandBuffer = commandBuffer;
    params.stageCount = stageCount;
    params.pStages = pStages;
    params.pShaders = pShaders;
    UNIX_CALL(vkCmdBindShadersEXT, &params);
}

void WINAPI vkCmdBindShadingRateImageNV(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout)
{
    struct vkCmdBindShadingRateImageNV_params params;
    params.commandBuffer = commandBuffer;
    params.imageView = imageView;
    params.imageLayout = imageLayout;
    UNIX_CALL(vkCmdBindShadingRateImageNV, &params);
}

void WINAPI vkCmdBindTileMemoryQCOM(VkCommandBuffer commandBuffer, const VkTileMemoryBindInfoQCOM *pTileMemoryBindInfo)
{
    struct vkCmdBindTileMemoryQCOM_params params;
    params.commandBuffer = commandBuffer;
    params.pTileMemoryBindInfo = pTileMemoryBindInfo;
    UNIX_CALL(vkCmdBindTileMemoryQCOM, &params);
}

void WINAPI vkCmdBindTransformFeedbackBuffersEXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes)
{
    struct vkCmdBindTransformFeedbackBuffersEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    params.pSizes = pSizes;
    UNIX_CALL(vkCmdBindTransformFeedbackBuffersEXT, &params);
}

void WINAPI vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
{
    struct vkCmdBindVertexBuffers_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    UNIX_CALL(vkCmdBindVertexBuffers, &params);
}

void WINAPI vkCmdBindVertexBuffers2(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides)
{
    struct vkCmdBindVertexBuffers2_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    params.pSizes = pSizes;
    params.pStrides = pStrides;
    UNIX_CALL(vkCmdBindVertexBuffers2, &params);
}

void WINAPI vkCmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides)
{
    struct vkCmdBindVertexBuffers2EXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstBinding = firstBinding;
    params.bindingCount = bindingCount;
    params.pBuffers = pBuffers;
    params.pOffsets = pOffsets;
    params.pSizes = pSizes;
    params.pStrides = pStrides;
    UNIX_CALL(vkCmdBindVertexBuffers2EXT, &params);
}

void WINAPI vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter)
{
    struct vkCmdBlitImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    params.filter = filter;
    UNIX_CALL(vkCmdBlitImage, &params);
}

void WINAPI vkCmdBlitImage2(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo)
{
    struct vkCmdBlitImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pBlitImageInfo = pBlitImageInfo;
    UNIX_CALL(vkCmdBlitImage2, &params);
}

void WINAPI vkCmdBlitImage2KHR(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo)
{
    struct vkCmdBlitImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pBlitImageInfo = pBlitImageInfo;
    UNIX_CALL(vkCmdBlitImage2KHR, &params);
}

void WINAPI vkCmdBuildAccelerationStructureNV(VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV *pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset)
{
    struct vkCmdBuildAccelerationStructureNV_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    params.instanceData = instanceData;
    params.instanceOffset = instanceOffset;
    params.update = update;
    params.dst = dst;
    params.src = src;
    params.scratch = scratch;
    params.scratchOffset = scratchOffset;
    UNIX_CALL(vkCmdBuildAccelerationStructureNV, &params);
}

void WINAPI vkCmdBuildAccelerationStructuresIndirectKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkDeviceAddress *pIndirectDeviceAddresses, const uint32_t *pIndirectStrides, const uint32_t * const*ppMaxPrimitiveCounts)
{
    struct vkCmdBuildAccelerationStructuresIndirectKHR_params params;
    params.commandBuffer = commandBuffer;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    params.pIndirectDeviceAddresses = pIndirectDeviceAddresses;
    params.pIndirectStrides = pIndirectStrides;
    params.ppMaxPrimitiveCounts = ppMaxPrimitiveCounts;
    UNIX_CALL(vkCmdBuildAccelerationStructuresIndirectKHR, &params);
}

void WINAPI vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos)
{
    struct vkCmdBuildAccelerationStructuresKHR_params params;
    params.commandBuffer = commandBuffer;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    params.ppBuildRangeInfos = ppBuildRangeInfos;
    UNIX_CALL(vkCmdBuildAccelerationStructuresKHR, &params);
}

void WINAPI vkCmdBuildClusterAccelerationStructureIndirectNV(VkCommandBuffer commandBuffer, const VkClusterAccelerationStructureCommandsInfoNV *pCommandInfos)
{
    struct vkCmdBuildClusterAccelerationStructureIndirectNV_params params;
    params.commandBuffer = commandBuffer;
    params.pCommandInfos = pCommandInfos;
    UNIX_CALL(vkCmdBuildClusterAccelerationStructureIndirectNV, &params);
}

void WINAPI vkCmdBuildMicromapsEXT(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkMicromapBuildInfoEXT *pInfos)
{
    struct vkCmdBuildMicromapsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    UNIX_CALL(vkCmdBuildMicromapsEXT, &params);
}

void WINAPI vkCmdBuildPartitionedAccelerationStructuresNV(VkCommandBuffer commandBuffer, const VkBuildPartitionedAccelerationStructureInfoNV *pBuildInfo)
{
    struct vkCmdBuildPartitionedAccelerationStructuresNV_params params;
    params.commandBuffer = commandBuffer;
    params.pBuildInfo = pBuildInfo;
    UNIX_CALL(vkCmdBuildPartitionedAccelerationStructuresNV, &params);
}

void WINAPI vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects)
{
    struct vkCmdClearAttachments_params params;
    params.commandBuffer = commandBuffer;
    params.attachmentCount = attachmentCount;
    params.pAttachments = pAttachments;
    params.rectCount = rectCount;
    params.pRects = pRects;
    UNIX_CALL(vkCmdClearAttachments, &params);
}

void WINAPI vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    struct vkCmdClearColorImage_params params;
    params.commandBuffer = commandBuffer;
    params.image = image;
    params.imageLayout = imageLayout;
    params.pColor = pColor;
    params.rangeCount = rangeCount;
    params.pRanges = pRanges;
    UNIX_CALL(vkCmdClearColorImage, &params);
}

void WINAPI vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    struct vkCmdClearDepthStencilImage_params params;
    params.commandBuffer = commandBuffer;
    params.image = image;
    params.imageLayout = imageLayout;
    params.pDepthStencil = pDepthStencil;
    params.rangeCount = rangeCount;
    params.pRanges = pRanges;
    UNIX_CALL(vkCmdClearDepthStencilImage, &params);
}

void WINAPI vkCmdControlVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoCodingControlInfoKHR *pCodingControlInfo)
{
    struct vkCmdControlVideoCodingKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCodingControlInfo = pCodingControlInfo;
    UNIX_CALL(vkCmdControlVideoCodingKHR, &params);
}

void WINAPI vkCmdConvertCooperativeVectorMatrixNV(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkConvertCooperativeVectorMatrixInfoNV *pInfos)
{
    struct vkCmdConvertCooperativeVectorMatrixNV_params params;
    params.commandBuffer = commandBuffer;
    params.infoCount = infoCount;
    params.pInfos = pInfos;
    UNIX_CALL(vkCmdConvertCooperativeVectorMatrixNV, &params);
}

void WINAPI vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    struct vkCmdCopyAccelerationStructureKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    UNIX_CALL(vkCmdCopyAccelerationStructureKHR, &params);
}

void WINAPI vkCmdCopyAccelerationStructureNV(VkCommandBuffer commandBuffer, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeKHR mode)
{
    struct vkCmdCopyAccelerationStructureNV_params params;
    params.commandBuffer = commandBuffer;
    params.dst = dst;
    params.src = src;
    params.mode = mode;
    UNIX_CALL(vkCmdCopyAccelerationStructureNV, &params);
}

void WINAPI vkCmdCopyAccelerationStructureToMemoryKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    struct vkCmdCopyAccelerationStructureToMemoryKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    UNIX_CALL(vkCmdCopyAccelerationStructureToMemoryKHR, &params);
}

void WINAPI vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
{
    struct vkCmdCopyBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.srcBuffer = srcBuffer;
    params.dstBuffer = dstBuffer;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    UNIX_CALL(vkCmdCopyBuffer, &params);
}

void WINAPI vkCmdCopyBuffer2(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo)
{
    struct vkCmdCopyBuffer2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferInfo = pCopyBufferInfo;
    UNIX_CALL(vkCmdCopyBuffer2, &params);
}

void WINAPI vkCmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo)
{
    struct vkCmdCopyBuffer2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferInfo = pCopyBufferInfo;
    UNIX_CALL(vkCmdCopyBuffer2KHR, &params);
}

void WINAPI vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    struct vkCmdCopyBufferToImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcBuffer = srcBuffer;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    UNIX_CALL(vkCmdCopyBufferToImage, &params);
}

void WINAPI vkCmdCopyBufferToImage2(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo)
{
    struct vkCmdCopyBufferToImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferToImageInfo = pCopyBufferToImageInfo;
    UNIX_CALL(vkCmdCopyBufferToImage2, &params);
}

void WINAPI vkCmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo)
{
    struct vkCmdCopyBufferToImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyBufferToImageInfo = pCopyBufferToImageInfo;
    UNIX_CALL(vkCmdCopyBufferToImage2KHR, &params);
}

void WINAPI vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions)
{
    struct vkCmdCopyImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    UNIX_CALL(vkCmdCopyImage, &params);
}

void WINAPI vkCmdCopyImage2(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 *pCopyImageInfo)
{
    struct vkCmdCopyImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageInfo = pCopyImageInfo;
    UNIX_CALL(vkCmdCopyImage2, &params);
}

void WINAPI vkCmdCopyImage2KHR(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 *pCopyImageInfo)
{
    struct vkCmdCopyImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageInfo = pCopyImageInfo;
    UNIX_CALL(vkCmdCopyImage2KHR, &params);
}

void WINAPI vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    struct vkCmdCopyImageToBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstBuffer = dstBuffer;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    UNIX_CALL(vkCmdCopyImageToBuffer, &params);
}

void WINAPI vkCmdCopyImageToBuffer2(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo)
{
    struct vkCmdCopyImageToBuffer2_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageToBufferInfo = pCopyImageToBufferInfo;
    UNIX_CALL(vkCmdCopyImageToBuffer2, &params);
}

void WINAPI vkCmdCopyImageToBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo)
{
    struct vkCmdCopyImageToBuffer2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyImageToBufferInfo = pCopyImageToBufferInfo;
    UNIX_CALL(vkCmdCopyImageToBuffer2KHR, &params);
}

void WINAPI vkCmdCopyMemoryIndirectNV(VkCommandBuffer commandBuffer, VkDeviceAddress copyBufferAddress, uint32_t copyCount, uint32_t stride)
{
    struct vkCmdCopyMemoryIndirectNV_params params;
    params.commandBuffer = commandBuffer;
    params.copyBufferAddress = copyBufferAddress;
    params.copyCount = copyCount;
    params.stride = stride;
    UNIX_CALL(vkCmdCopyMemoryIndirectNV, &params);
}

void WINAPI vkCmdCopyMemoryToAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    struct vkCmdCopyMemoryToAccelerationStructureKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    UNIX_CALL(vkCmdCopyMemoryToAccelerationStructureKHR, &params);
}

void WINAPI vkCmdCopyMemoryToImageIndirectNV(VkCommandBuffer commandBuffer, VkDeviceAddress copyBufferAddress, uint32_t copyCount, uint32_t stride, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageSubresourceLayers *pImageSubresources)
{
    struct vkCmdCopyMemoryToImageIndirectNV_params params;
    params.commandBuffer = commandBuffer;
    params.copyBufferAddress = copyBufferAddress;
    params.copyCount = copyCount;
    params.stride = stride;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.pImageSubresources = pImageSubresources;
    UNIX_CALL(vkCmdCopyMemoryToImageIndirectNV, &params);
}

void WINAPI vkCmdCopyMemoryToMicromapEXT(VkCommandBuffer commandBuffer, const VkCopyMemoryToMicromapInfoEXT *pInfo)
{
    struct vkCmdCopyMemoryToMicromapEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    UNIX_CALL(vkCmdCopyMemoryToMicromapEXT, &params);
}

void WINAPI vkCmdCopyMicromapEXT(VkCommandBuffer commandBuffer, const VkCopyMicromapInfoEXT *pInfo)
{
    struct vkCmdCopyMicromapEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    UNIX_CALL(vkCmdCopyMicromapEXT, &params);
}

void WINAPI vkCmdCopyMicromapToMemoryEXT(VkCommandBuffer commandBuffer, const VkCopyMicromapToMemoryInfoEXT *pInfo)
{
    struct vkCmdCopyMicromapToMemoryEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pInfo = pInfo;
    UNIX_CALL(vkCmdCopyMicromapToMemoryEXT, &params);
}

void WINAPI vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    struct vkCmdCopyQueryPoolResults_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.stride = stride;
    params.flags = flags;
    UNIX_CALL(vkCmdCopyQueryPoolResults, &params);
}

void WINAPI vkCmdCopyTensorARM(VkCommandBuffer commandBuffer, const VkCopyTensorInfoARM *pCopyTensorInfo)
{
    struct vkCmdCopyTensorARM_params params;
    params.commandBuffer = commandBuffer;
    params.pCopyTensorInfo = pCopyTensorInfo;
    UNIX_CALL(vkCmdCopyTensorARM, &params);
}

void WINAPI vkCmdCuLaunchKernelNVX(VkCommandBuffer commandBuffer, const VkCuLaunchInfoNVX *pLaunchInfo)
{
    struct vkCmdCuLaunchKernelNVX_params params;
    params.commandBuffer = commandBuffer;
    params.pLaunchInfo = pLaunchInfo;
    UNIX_CALL(vkCmdCuLaunchKernelNVX, &params);
}

void WINAPI vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    struct vkCmdDebugMarkerBeginEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    UNIX_CALL(vkCmdDebugMarkerBeginEXT, &params);
}

void WINAPI vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
{
    struct vkCmdDebugMarkerEndEXT_params params;
    params.commandBuffer = commandBuffer;
    UNIX_CALL(vkCmdDebugMarkerEndEXT, &params);
}

void WINAPI vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
{
    struct vkCmdDebugMarkerInsertEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    UNIX_CALL(vkCmdDebugMarkerInsertEXT, &params);
}

void WINAPI vkCmdDecodeVideoKHR(VkCommandBuffer commandBuffer, const VkVideoDecodeInfoKHR *pDecodeInfo)
{
    struct vkCmdDecodeVideoKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pDecodeInfo = pDecodeInfo;
    UNIX_CALL(vkCmdDecodeVideoKHR, &params);
}

void WINAPI vkCmdDecompressMemoryIndirectCountNV(VkCommandBuffer commandBuffer, VkDeviceAddress indirectCommandsAddress, VkDeviceAddress indirectCommandsCountAddress, uint32_t stride)
{
    struct vkCmdDecompressMemoryIndirectCountNV_params params;
    params.commandBuffer = commandBuffer;
    params.indirectCommandsAddress = indirectCommandsAddress;
    params.indirectCommandsCountAddress = indirectCommandsCountAddress;
    params.stride = stride;
    UNIX_CALL(vkCmdDecompressMemoryIndirectCountNV, &params);
}

void WINAPI vkCmdDecompressMemoryNV(VkCommandBuffer commandBuffer, uint32_t decompressRegionCount, const VkDecompressMemoryRegionNV *pDecompressMemoryRegions)
{
    struct vkCmdDecompressMemoryNV_params params;
    params.commandBuffer = commandBuffer;
    params.decompressRegionCount = decompressRegionCount;
    params.pDecompressMemoryRegions = pDecompressMemoryRegions;
    UNIX_CALL(vkCmdDecompressMemoryNV, &params);
}

void WINAPI vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDispatch_params params;
    params.commandBuffer = commandBuffer;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    UNIX_CALL(vkCmdDispatch, &params);
}

void WINAPI vkCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDispatchBase_params params;
    params.commandBuffer = commandBuffer;
    params.baseGroupX = baseGroupX;
    params.baseGroupY = baseGroupY;
    params.baseGroupZ = baseGroupZ;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    UNIX_CALL(vkCmdDispatchBase, &params);
}

void WINAPI vkCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDispatchBaseKHR_params params;
    params.commandBuffer = commandBuffer;
    params.baseGroupX = baseGroupX;
    params.baseGroupY = baseGroupY;
    params.baseGroupZ = baseGroupZ;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    UNIX_CALL(vkCmdDispatchBaseKHR, &params);
}

void WINAPI vkCmdDispatchDataGraphARM(VkCommandBuffer commandBuffer, VkDataGraphPipelineSessionARM session, const VkDataGraphPipelineDispatchInfoARM *pInfo)
{
    struct vkCmdDispatchDataGraphARM_params params;
    params.commandBuffer = commandBuffer;
    params.session = session;
    params.pInfo = pInfo;
    UNIX_CALL(vkCmdDispatchDataGraphARM, &params);
}

void WINAPI vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    struct vkCmdDispatchIndirect_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    UNIX_CALL(vkCmdDispatchIndirect, &params);
}

void WINAPI vkCmdDispatchTileQCOM(VkCommandBuffer commandBuffer, const VkDispatchTileInfoQCOM *pDispatchTileInfo)
{
    struct vkCmdDispatchTileQCOM_params params;
    params.commandBuffer = commandBuffer;
    params.pDispatchTileInfo = pDispatchTileInfo;
    UNIX_CALL(vkCmdDispatchTileQCOM, &params);
}

void WINAPI vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    struct vkCmdDraw_params params;
    params.commandBuffer = commandBuffer;
    params.vertexCount = vertexCount;
    params.instanceCount = instanceCount;
    params.firstVertex = firstVertex;
    params.firstInstance = firstInstance;
    UNIX_CALL(vkCmdDraw, &params);
}

void WINAPI vkCmdDrawClusterHUAWEI(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDrawClusterHUAWEI_params params;
    params.commandBuffer = commandBuffer;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    UNIX_CALL(vkCmdDrawClusterHUAWEI, &params);
}

void WINAPI vkCmdDrawClusterIndirectHUAWEI(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    struct vkCmdDrawClusterIndirectHUAWEI_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    UNIX_CALL(vkCmdDrawClusterIndirectHUAWEI, &params);
}

void WINAPI vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    struct vkCmdDrawIndexed_params params;
    params.commandBuffer = commandBuffer;
    params.indexCount = indexCount;
    params.instanceCount = instanceCount;
    params.firstIndex = firstIndex;
    params.vertexOffset = vertexOffset;
    params.firstInstance = firstInstance;
    UNIX_CALL(vkCmdDrawIndexed, &params);
}

void WINAPI vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirect_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndexedIndirect, &params);
}

void WINAPI vkCmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirectCount_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndexedIndirectCount, &params);
}

void WINAPI vkCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirectCountAMD_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndexedIndirectCountAMD, &params);
}

void WINAPI vkCmdDrawIndexedIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndexedIndirectCountKHR_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndexedIndirectCountKHR, &params);
}

void WINAPI vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawIndirect_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndirect, &params);
}

void WINAPI vkCmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride)
{
    struct vkCmdDrawIndirectByteCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.instanceCount = instanceCount;
    params.firstInstance = firstInstance;
    params.counterBuffer = counterBuffer;
    params.counterBufferOffset = counterBufferOffset;
    params.counterOffset = counterOffset;
    params.vertexStride = vertexStride;
    UNIX_CALL(vkCmdDrawIndirectByteCountEXT, &params);
}

void WINAPI vkCmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndirectCount_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndirectCount, &params);
}

void WINAPI vkCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndirectCountAMD_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndirectCountAMD, &params);
}

void WINAPI vkCmdDrawIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawIndirectCountKHR_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawIndirectCountKHR, &params);
}

void WINAPI vkCmdDrawMeshTasksEXT(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    struct vkCmdDrawMeshTasksEXT_params params;
    params.commandBuffer = commandBuffer;
    params.groupCountX = groupCountX;
    params.groupCountY = groupCountY;
    params.groupCountZ = groupCountZ;
    UNIX_CALL(vkCmdDrawMeshTasksEXT, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectCountEXT(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawMeshTasksIndirectCountEXT, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectCountNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectCountNV_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.countBuffer = countBuffer;
    params.countBufferOffset = countBufferOffset;
    params.maxDrawCount = maxDrawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawMeshTasksIndirectCountNV, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectEXT(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectEXT_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawMeshTasksIndirectEXT, &params);
}

void WINAPI vkCmdDrawMeshTasksIndirectNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    struct vkCmdDrawMeshTasksIndirectNV_params params;
    params.commandBuffer = commandBuffer;
    params.buffer = buffer;
    params.offset = offset;
    params.drawCount = drawCount;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawMeshTasksIndirectNV, &params);
}

void WINAPI vkCmdDrawMeshTasksNV(VkCommandBuffer commandBuffer, uint32_t taskCount, uint32_t firstTask)
{
    struct vkCmdDrawMeshTasksNV_params params;
    params.commandBuffer = commandBuffer;
    params.taskCount = taskCount;
    params.firstTask = firstTask;
    UNIX_CALL(vkCmdDrawMeshTasksNV, &params);
}

void WINAPI vkCmdDrawMultiEXT(VkCommandBuffer commandBuffer, uint32_t drawCount, const VkMultiDrawInfoEXT *pVertexInfo, uint32_t instanceCount, uint32_t firstInstance, uint32_t stride)
{
    struct vkCmdDrawMultiEXT_params params;
    params.commandBuffer = commandBuffer;
    params.drawCount = drawCount;
    params.pVertexInfo = pVertexInfo;
    params.instanceCount = instanceCount;
    params.firstInstance = firstInstance;
    params.stride = stride;
    UNIX_CALL(vkCmdDrawMultiEXT, &params);
}

void WINAPI vkCmdDrawMultiIndexedEXT(VkCommandBuffer commandBuffer, uint32_t drawCount, const VkMultiDrawIndexedInfoEXT *pIndexInfo, uint32_t instanceCount, uint32_t firstInstance, uint32_t stride, const int32_t *pVertexOffset)
{
    struct vkCmdDrawMultiIndexedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.drawCount = drawCount;
    params.pIndexInfo = pIndexInfo;
    params.instanceCount = instanceCount;
    params.firstInstance = firstInstance;
    params.stride = stride;
    params.pVertexOffset = pVertexOffset;
    UNIX_CALL(vkCmdDrawMultiIndexedEXT, &params);
}

void WINAPI vkCmdEncodeVideoKHR(VkCommandBuffer commandBuffer, const VkVideoEncodeInfoKHR *pEncodeInfo)
{
    struct vkCmdEncodeVideoKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pEncodeInfo = pEncodeInfo;
    UNIX_CALL(vkCmdEncodeVideoKHR, &params);
}

void WINAPI vkCmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndConditionalRenderingEXT_params params;
    params.commandBuffer = commandBuffer;
    UNIX_CALL(vkCmdEndConditionalRenderingEXT, &params);
}

void WINAPI vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndDebugUtilsLabelEXT_params params;
    params.commandBuffer = commandBuffer;
    UNIX_CALL(vkCmdEndDebugUtilsLabelEXT, &params);
}

void WINAPI vkCmdEndPerTileExecutionQCOM(VkCommandBuffer commandBuffer, const VkPerTileEndInfoQCOM *pPerTileEndInfo)
{
    struct vkCmdEndPerTileExecutionQCOM_params params;
    params.commandBuffer = commandBuffer;
    params.pPerTileEndInfo = pPerTileEndInfo;
    UNIX_CALL(vkCmdEndPerTileExecutionQCOM, &params);
}

void WINAPI vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdEndQuery_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    UNIX_CALL(vkCmdEndQuery, &params);
}

void WINAPI vkCmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index)
{
    struct vkCmdEndQueryIndexedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.query = query;
    params.index = index;
    UNIX_CALL(vkCmdEndQueryIndexedEXT, &params);
}

void WINAPI vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndRenderPass_params params;
    params.commandBuffer = commandBuffer;
    UNIX_CALL(vkCmdEndRenderPass, &params);
}

void WINAPI vkCmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdEndRenderPass2_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassEndInfo = pSubpassEndInfo;
    UNIX_CALL(vkCmdEndRenderPass2, &params);
}

void WINAPI vkCmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdEndRenderPass2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassEndInfo = pSubpassEndInfo;
    UNIX_CALL(vkCmdEndRenderPass2KHR, &params);
}

void WINAPI vkCmdEndRendering(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndRendering_params params;
    params.commandBuffer = commandBuffer;
    UNIX_CALL(vkCmdEndRendering, &params);
}

void WINAPI vkCmdEndRendering2EXT(VkCommandBuffer commandBuffer, const VkRenderingEndInfoEXT *pRenderingEndInfo)
{
    struct vkCmdEndRendering2EXT_params params;
    params.commandBuffer = commandBuffer;
    params.pRenderingEndInfo = pRenderingEndInfo;
    UNIX_CALL(vkCmdEndRendering2EXT, &params);
}

void WINAPI vkCmdEndRenderingKHR(VkCommandBuffer commandBuffer)
{
    struct vkCmdEndRenderingKHR_params params;
    params.commandBuffer = commandBuffer;
    UNIX_CALL(vkCmdEndRenderingKHR, &params);
}

void WINAPI vkCmdEndTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets)
{
    struct vkCmdEndTransformFeedbackEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstCounterBuffer = firstCounterBuffer;
    params.counterBufferCount = counterBufferCount;
    params.pCounterBuffers = pCounterBuffers;
    params.pCounterBufferOffsets = pCounterBufferOffsets;
    UNIX_CALL(vkCmdEndTransformFeedbackEXT, &params);
}

void WINAPI vkCmdEndVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoEndCodingInfoKHR *pEndCodingInfo)
{
    struct vkCmdEndVideoCodingKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pEndCodingInfo = pEndCodingInfo;
    UNIX_CALL(vkCmdEndVideoCodingKHR, &params);
}

void WINAPI vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers)
{
    struct vkCmdExecuteCommands_params params;
    params.commandBuffer = commandBuffer;
    params.commandBufferCount = commandBufferCount;
    params.pCommandBuffers = pCommandBuffers;
    UNIX_CALL(vkCmdExecuteCommands, &params);
}

void WINAPI vkCmdExecuteGeneratedCommandsEXT(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed, const VkGeneratedCommandsInfoEXT *pGeneratedCommandsInfo)
{
    struct vkCmdExecuteGeneratedCommandsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.isPreprocessed = isPreprocessed;
    params.pGeneratedCommandsInfo = pGeneratedCommandsInfo;
    UNIX_CALL(vkCmdExecuteGeneratedCommandsEXT, &params);
}

void WINAPI vkCmdExecuteGeneratedCommandsNV(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    struct vkCmdExecuteGeneratedCommandsNV_params params;
    params.commandBuffer = commandBuffer;
    params.isPreprocessed = isPreprocessed;
    params.pGeneratedCommandsInfo = pGeneratedCommandsInfo;
    UNIX_CALL(vkCmdExecuteGeneratedCommandsNV, &params);
}

void WINAPI vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    struct vkCmdFillBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.size = size;
    params.data = data;
    UNIX_CALL(vkCmdFillBuffer, &params);
}

void WINAPI vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkCmdInsertDebugUtilsLabelEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pLabelInfo = pLabelInfo;
    UNIX_CALL(vkCmdInsertDebugUtilsLabelEXT, &params);
}

void WINAPI vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    struct vkCmdNextSubpass_params params;
    params.commandBuffer = commandBuffer;
    params.contents = contents;
    UNIX_CALL(vkCmdNextSubpass, &params);
}

void WINAPI vkCmdNextSubpass2(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdNextSubpass2_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    params.pSubpassEndInfo = pSubpassEndInfo;
    UNIX_CALL(vkCmdNextSubpass2, &params);
}

void WINAPI vkCmdNextSubpass2KHR(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo)
{
    struct vkCmdNextSubpass2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pSubpassBeginInfo = pSubpassBeginInfo;
    params.pSubpassEndInfo = pSubpassEndInfo;
    UNIX_CALL(vkCmdNextSubpass2KHR, &params);
}

void WINAPI vkCmdOpticalFlowExecuteNV(VkCommandBuffer commandBuffer, VkOpticalFlowSessionNV session, const VkOpticalFlowExecuteInfoNV *pExecuteInfo)
{
    struct vkCmdOpticalFlowExecuteNV_params params;
    params.commandBuffer = commandBuffer;
    params.session = session;
    params.pExecuteInfo = pExecuteInfo;
    UNIX_CALL(vkCmdOpticalFlowExecuteNV, &params);
}

void WINAPI vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    struct vkCmdPipelineBarrier_params params;
    params.commandBuffer = commandBuffer;
    params.srcStageMask = srcStageMask;
    params.dstStageMask = dstStageMask;
    params.dependencyFlags = dependencyFlags;
    params.memoryBarrierCount = memoryBarrierCount;
    params.pMemoryBarriers = pMemoryBarriers;
    params.bufferMemoryBarrierCount = bufferMemoryBarrierCount;
    params.pBufferMemoryBarriers = pBufferMemoryBarriers;
    params.imageMemoryBarrierCount = imageMemoryBarrierCount;
    params.pImageMemoryBarriers = pImageMemoryBarriers;
    UNIX_CALL(vkCmdPipelineBarrier, &params);
}

void WINAPI vkCmdPipelineBarrier2(VkCommandBuffer commandBuffer, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdPipelineBarrier2_params params;
    params.commandBuffer = commandBuffer;
    params.pDependencyInfo = pDependencyInfo;
    UNIX_CALL(vkCmdPipelineBarrier2, &params);
}

void WINAPI vkCmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdPipelineBarrier2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pDependencyInfo = pDependencyInfo;
    UNIX_CALL(vkCmdPipelineBarrier2KHR, &params);
}

void WINAPI vkCmdPreprocessGeneratedCommandsEXT(VkCommandBuffer commandBuffer, const VkGeneratedCommandsInfoEXT *pGeneratedCommandsInfo, VkCommandBuffer stateCommandBuffer)
{
    struct vkCmdPreprocessGeneratedCommandsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pGeneratedCommandsInfo = pGeneratedCommandsInfo;
    params.stateCommandBuffer = stateCommandBuffer;
    UNIX_CALL(vkCmdPreprocessGeneratedCommandsEXT, &params);
}

void WINAPI vkCmdPreprocessGeneratedCommandsNV(VkCommandBuffer commandBuffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo)
{
    struct vkCmdPreprocessGeneratedCommandsNV_params params;
    params.commandBuffer = commandBuffer;
    params.pGeneratedCommandsInfo = pGeneratedCommandsInfo;
    UNIX_CALL(vkCmdPreprocessGeneratedCommandsNV, &params);
}

void WINAPI vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues)
{
    struct vkCmdPushConstants_params params;
    params.commandBuffer = commandBuffer;
    params.layout = layout;
    params.stageFlags = stageFlags;
    params.offset = offset;
    params.size = size;
    params.pValues = pValues;
    UNIX_CALL(vkCmdPushConstants, &params);
}

void WINAPI vkCmdPushConstants2(VkCommandBuffer commandBuffer, const VkPushConstantsInfo *pPushConstantsInfo)
{
    struct vkCmdPushConstants2_params params;
    params.commandBuffer = commandBuffer;
    params.pPushConstantsInfo = pPushConstantsInfo;
    UNIX_CALL(vkCmdPushConstants2, &params);
}

void WINAPI vkCmdPushConstants2KHR(VkCommandBuffer commandBuffer, const VkPushConstantsInfo *pPushConstantsInfo)
{
    struct vkCmdPushConstants2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pPushConstantsInfo = pPushConstantsInfo;
    UNIX_CALL(vkCmdPushConstants2KHR, &params);
}

void WINAPI vkCmdPushDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites)
{
    struct vkCmdPushDescriptorSet_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.layout = layout;
    params.set = set;
    params.descriptorWriteCount = descriptorWriteCount;
    params.pDescriptorWrites = pDescriptorWrites;
    UNIX_CALL(vkCmdPushDescriptorSet, &params);
}

void WINAPI vkCmdPushDescriptorSet2(VkCommandBuffer commandBuffer, const VkPushDescriptorSetInfo *pPushDescriptorSetInfo)
{
    struct vkCmdPushDescriptorSet2_params params;
    params.commandBuffer = commandBuffer;
    params.pPushDescriptorSetInfo = pPushDescriptorSetInfo;
    UNIX_CALL(vkCmdPushDescriptorSet2, &params);
}

void WINAPI vkCmdPushDescriptorSet2KHR(VkCommandBuffer commandBuffer, const VkPushDescriptorSetInfo *pPushDescriptorSetInfo)
{
    struct vkCmdPushDescriptorSet2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pPushDescriptorSetInfo = pPushDescriptorSetInfo;
    UNIX_CALL(vkCmdPushDescriptorSet2KHR, &params);
}

void WINAPI vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites)
{
    struct vkCmdPushDescriptorSetKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.layout = layout;
    params.set = set;
    params.descriptorWriteCount = descriptorWriteCount;
    params.pDescriptorWrites = pDescriptorWrites;
    UNIX_CALL(vkCmdPushDescriptorSetKHR, &params);
}

void WINAPI vkCmdPushDescriptorSetWithTemplate(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void *pData)
{
    struct vkCmdPushDescriptorSetWithTemplate_params params;
    params.commandBuffer = commandBuffer;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.layout = layout;
    params.set = set;
    params.pData = pData;
    UNIX_CALL(vkCmdPushDescriptorSetWithTemplate, &params);
}

void WINAPI vkCmdPushDescriptorSetWithTemplate2(VkCommandBuffer commandBuffer, const VkPushDescriptorSetWithTemplateInfo *pPushDescriptorSetWithTemplateInfo)
{
    struct vkCmdPushDescriptorSetWithTemplate2_params params;
    params.commandBuffer = commandBuffer;
    params.pPushDescriptorSetWithTemplateInfo = pPushDescriptorSetWithTemplateInfo;
    UNIX_CALL(vkCmdPushDescriptorSetWithTemplate2, &params);
}

void WINAPI vkCmdPushDescriptorSetWithTemplate2KHR(VkCommandBuffer commandBuffer, const VkPushDescriptorSetWithTemplateInfo *pPushDescriptorSetWithTemplateInfo)
{
    struct vkCmdPushDescriptorSetWithTemplate2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pPushDescriptorSetWithTemplateInfo = pPushDescriptorSetWithTemplateInfo;
    UNIX_CALL(vkCmdPushDescriptorSetWithTemplate2KHR, &params);
}

void WINAPI vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void *pData)
{
    struct vkCmdPushDescriptorSetWithTemplateKHR_params params;
    params.commandBuffer = commandBuffer;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.layout = layout;
    params.set = set;
    params.pData = pData;
    UNIX_CALL(vkCmdPushDescriptorSetWithTemplateKHR, &params);
}

void WINAPI vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    struct vkCmdResetEvent_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    UNIX_CALL(vkCmdResetEvent, &params);
}

void WINAPI vkCmdResetEvent2(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask)
{
    struct vkCmdResetEvent2_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    UNIX_CALL(vkCmdResetEvent2, &params);
}

void WINAPI vkCmdResetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask)
{
    struct vkCmdResetEvent2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    UNIX_CALL(vkCmdResetEvent2KHR, &params);
}

void WINAPI vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    struct vkCmdResetQueryPool_params params;
    params.commandBuffer = commandBuffer;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    UNIX_CALL(vkCmdResetQueryPool, &params);
}

void WINAPI vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions)
{
    struct vkCmdResolveImage_params params;
    params.commandBuffer = commandBuffer;
    params.srcImage = srcImage;
    params.srcImageLayout = srcImageLayout;
    params.dstImage = dstImage;
    params.dstImageLayout = dstImageLayout;
    params.regionCount = regionCount;
    params.pRegions = pRegions;
    UNIX_CALL(vkCmdResolveImage, &params);
}

void WINAPI vkCmdResolveImage2(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo)
{
    struct vkCmdResolveImage2_params params;
    params.commandBuffer = commandBuffer;
    params.pResolveImageInfo = pResolveImageInfo;
    UNIX_CALL(vkCmdResolveImage2, &params);
}

void WINAPI vkCmdResolveImage2KHR(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo)
{
    struct vkCmdResolveImage2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.pResolveImageInfo = pResolveImageInfo;
    UNIX_CALL(vkCmdResolveImage2KHR, &params);
}

void WINAPI vkCmdSetAlphaToCoverageEnableEXT(VkCommandBuffer commandBuffer, VkBool32 alphaToCoverageEnable)
{
    struct vkCmdSetAlphaToCoverageEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.alphaToCoverageEnable = alphaToCoverageEnable;
    UNIX_CALL(vkCmdSetAlphaToCoverageEnableEXT, &params);
}

void WINAPI vkCmdSetAlphaToOneEnableEXT(VkCommandBuffer commandBuffer, VkBool32 alphaToOneEnable)
{
    struct vkCmdSetAlphaToOneEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.alphaToOneEnable = alphaToOneEnable;
    UNIX_CALL(vkCmdSetAlphaToOneEnableEXT, &params);
}

void WINAPI vkCmdSetAttachmentFeedbackLoopEnableEXT(VkCommandBuffer commandBuffer, VkImageAspectFlags aspectMask)
{
    struct vkCmdSetAttachmentFeedbackLoopEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.aspectMask = aspectMask;
    UNIX_CALL(vkCmdSetAttachmentFeedbackLoopEnableEXT, &params);
}

void WINAPI vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    struct vkCmdSetBlendConstants_params params;
    params.commandBuffer = commandBuffer;
    params.blendConstants = blendConstants;
    UNIX_CALL(vkCmdSetBlendConstants, &params);
}

void WINAPI vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    struct vkCmdSetCheckpointNV_params params;
    params.commandBuffer = commandBuffer;
    params.pCheckpointMarker = pCheckpointMarker;
    UNIX_CALL(vkCmdSetCheckpointNV, &params);
}

void WINAPI vkCmdSetCoarseSampleOrderNV(VkCommandBuffer commandBuffer, VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV *pCustomSampleOrders)
{
    struct vkCmdSetCoarseSampleOrderNV_params params;
    params.commandBuffer = commandBuffer;
    params.sampleOrderType = sampleOrderType;
    params.customSampleOrderCount = customSampleOrderCount;
    params.pCustomSampleOrders = pCustomSampleOrders;
    UNIX_CALL(vkCmdSetCoarseSampleOrderNV, &params);
}

void WINAPI vkCmdSetColorBlendAdvancedEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorBlendAdvancedEXT *pColorBlendAdvanced)
{
    struct vkCmdSetColorBlendAdvancedEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorBlendAdvanced = pColorBlendAdvanced;
    UNIX_CALL(vkCmdSetColorBlendAdvancedEXT, &params);
}

void WINAPI vkCmdSetColorBlendEnableEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkBool32 *pColorBlendEnables)
{
    struct vkCmdSetColorBlendEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorBlendEnables = pColorBlendEnables;
    UNIX_CALL(vkCmdSetColorBlendEnableEXT, &params);
}

void WINAPI vkCmdSetColorBlendEquationEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorBlendEquationEXT *pColorBlendEquations)
{
    struct vkCmdSetColorBlendEquationEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorBlendEquations = pColorBlendEquations;
    UNIX_CALL(vkCmdSetColorBlendEquationEXT, &params);
}

void WINAPI vkCmdSetColorWriteEnableEXT(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkBool32 *pColorWriteEnables)
{
    struct vkCmdSetColorWriteEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.attachmentCount = attachmentCount;
    params.pColorWriteEnables = pColorWriteEnables;
    UNIX_CALL(vkCmdSetColorWriteEnableEXT, &params);
}

void WINAPI vkCmdSetColorWriteMaskEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorComponentFlags *pColorWriteMasks)
{
    struct vkCmdSetColorWriteMaskEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstAttachment = firstAttachment;
    params.attachmentCount = attachmentCount;
    params.pColorWriteMasks = pColorWriteMasks;
    UNIX_CALL(vkCmdSetColorWriteMaskEXT, &params);
}

void WINAPI vkCmdSetConservativeRasterizationModeEXT(VkCommandBuffer commandBuffer, VkConservativeRasterizationModeEXT conservativeRasterizationMode)
{
    struct vkCmdSetConservativeRasterizationModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.conservativeRasterizationMode = conservativeRasterizationMode;
    UNIX_CALL(vkCmdSetConservativeRasterizationModeEXT, &params);
}

void WINAPI vkCmdSetCoverageModulationModeNV(VkCommandBuffer commandBuffer, VkCoverageModulationModeNV coverageModulationMode)
{
    struct vkCmdSetCoverageModulationModeNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageModulationMode = coverageModulationMode;
    UNIX_CALL(vkCmdSetCoverageModulationModeNV, &params);
}

void WINAPI vkCmdSetCoverageModulationTableEnableNV(VkCommandBuffer commandBuffer, VkBool32 coverageModulationTableEnable)
{
    struct vkCmdSetCoverageModulationTableEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageModulationTableEnable = coverageModulationTableEnable;
    UNIX_CALL(vkCmdSetCoverageModulationTableEnableNV, &params);
}

void WINAPI vkCmdSetCoverageModulationTableNV(VkCommandBuffer commandBuffer, uint32_t coverageModulationTableCount, const float *pCoverageModulationTable)
{
    struct vkCmdSetCoverageModulationTableNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageModulationTableCount = coverageModulationTableCount;
    params.pCoverageModulationTable = pCoverageModulationTable;
    UNIX_CALL(vkCmdSetCoverageModulationTableNV, &params);
}

void WINAPI vkCmdSetCoverageReductionModeNV(VkCommandBuffer commandBuffer, VkCoverageReductionModeNV coverageReductionMode)
{
    struct vkCmdSetCoverageReductionModeNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageReductionMode = coverageReductionMode;
    UNIX_CALL(vkCmdSetCoverageReductionModeNV, &params);
}

void WINAPI vkCmdSetCoverageToColorEnableNV(VkCommandBuffer commandBuffer, VkBool32 coverageToColorEnable)
{
    struct vkCmdSetCoverageToColorEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageToColorEnable = coverageToColorEnable;
    UNIX_CALL(vkCmdSetCoverageToColorEnableNV, &params);
}

void WINAPI vkCmdSetCoverageToColorLocationNV(VkCommandBuffer commandBuffer, uint32_t coverageToColorLocation)
{
    struct vkCmdSetCoverageToColorLocationNV_params params;
    params.commandBuffer = commandBuffer;
    params.coverageToColorLocation = coverageToColorLocation;
    UNIX_CALL(vkCmdSetCoverageToColorLocationNV, &params);
}

void WINAPI vkCmdSetCullMode(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
{
    struct vkCmdSetCullMode_params params;
    params.commandBuffer = commandBuffer;
    params.cullMode = cullMode;
    UNIX_CALL(vkCmdSetCullMode, &params);
}

void WINAPI vkCmdSetCullModeEXT(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
{
    struct vkCmdSetCullModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.cullMode = cullMode;
    UNIX_CALL(vkCmdSetCullModeEXT, &params);
}

void WINAPI vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    struct vkCmdSetDepthBias_params params;
    params.commandBuffer = commandBuffer;
    params.depthBiasConstantFactor = depthBiasConstantFactor;
    params.depthBiasClamp = depthBiasClamp;
    params.depthBiasSlopeFactor = depthBiasSlopeFactor;
    UNIX_CALL(vkCmdSetDepthBias, &params);
}

void WINAPI vkCmdSetDepthBias2EXT(VkCommandBuffer commandBuffer, const VkDepthBiasInfoEXT *pDepthBiasInfo)
{
    struct vkCmdSetDepthBias2EXT_params params;
    params.commandBuffer = commandBuffer;
    params.pDepthBiasInfo = pDepthBiasInfo;
    UNIX_CALL(vkCmdSetDepthBias2EXT, &params);
}

void WINAPI vkCmdSetDepthBiasEnable(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable)
{
    struct vkCmdSetDepthBiasEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthBiasEnable = depthBiasEnable;
    UNIX_CALL(vkCmdSetDepthBiasEnable, &params);
}

void WINAPI vkCmdSetDepthBiasEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable)
{
    struct vkCmdSetDepthBiasEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthBiasEnable = depthBiasEnable;
    UNIX_CALL(vkCmdSetDepthBiasEnableEXT, &params);
}

void WINAPI vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    struct vkCmdSetDepthBounds_params params;
    params.commandBuffer = commandBuffer;
    params.minDepthBounds = minDepthBounds;
    params.maxDepthBounds = maxDepthBounds;
    UNIX_CALL(vkCmdSetDepthBounds, &params);
}

void WINAPI vkCmdSetDepthBoundsTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable)
{
    struct vkCmdSetDepthBoundsTestEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthBoundsTestEnable = depthBoundsTestEnable;
    UNIX_CALL(vkCmdSetDepthBoundsTestEnable, &params);
}

void WINAPI vkCmdSetDepthBoundsTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable)
{
    struct vkCmdSetDepthBoundsTestEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthBoundsTestEnable = depthBoundsTestEnable;
    UNIX_CALL(vkCmdSetDepthBoundsTestEnableEXT, &params);
}

void WINAPI vkCmdSetDepthClampEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthClampEnable)
{
    struct vkCmdSetDepthClampEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthClampEnable = depthClampEnable;
    UNIX_CALL(vkCmdSetDepthClampEnableEXT, &params);
}

void WINAPI vkCmdSetDepthClampRangeEXT(VkCommandBuffer commandBuffer, VkDepthClampModeEXT depthClampMode, const VkDepthClampRangeEXT *pDepthClampRange)
{
    struct vkCmdSetDepthClampRangeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthClampMode = depthClampMode;
    params.pDepthClampRange = pDepthClampRange;
    UNIX_CALL(vkCmdSetDepthClampRangeEXT, &params);
}

void WINAPI vkCmdSetDepthClipEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthClipEnable)
{
    struct vkCmdSetDepthClipEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthClipEnable = depthClipEnable;
    UNIX_CALL(vkCmdSetDepthClipEnableEXT, &params);
}

void WINAPI vkCmdSetDepthClipNegativeOneToOneEXT(VkCommandBuffer commandBuffer, VkBool32 negativeOneToOne)
{
    struct vkCmdSetDepthClipNegativeOneToOneEXT_params params;
    params.commandBuffer = commandBuffer;
    params.negativeOneToOne = negativeOneToOne;
    UNIX_CALL(vkCmdSetDepthClipNegativeOneToOneEXT, &params);
}

void WINAPI vkCmdSetDepthCompareOp(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp)
{
    struct vkCmdSetDepthCompareOp_params params;
    params.commandBuffer = commandBuffer;
    params.depthCompareOp = depthCompareOp;
    UNIX_CALL(vkCmdSetDepthCompareOp, &params);
}

void WINAPI vkCmdSetDepthCompareOpEXT(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp)
{
    struct vkCmdSetDepthCompareOpEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthCompareOp = depthCompareOp;
    UNIX_CALL(vkCmdSetDepthCompareOpEXT, &params);
}

void WINAPI vkCmdSetDepthTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    struct vkCmdSetDepthTestEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthTestEnable = depthTestEnable;
    UNIX_CALL(vkCmdSetDepthTestEnable, &params);
}

void WINAPI vkCmdSetDepthTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    struct vkCmdSetDepthTestEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthTestEnable = depthTestEnable;
    UNIX_CALL(vkCmdSetDepthTestEnableEXT, &params);
}

void WINAPI vkCmdSetDepthWriteEnable(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable)
{
    struct vkCmdSetDepthWriteEnable_params params;
    params.commandBuffer = commandBuffer;
    params.depthWriteEnable = depthWriteEnable;
    UNIX_CALL(vkCmdSetDepthWriteEnable, &params);
}

void WINAPI vkCmdSetDepthWriteEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable)
{
    struct vkCmdSetDepthWriteEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.depthWriteEnable = depthWriteEnable;
    UNIX_CALL(vkCmdSetDepthWriteEnableEXT, &params);
}

void WINAPI vkCmdSetDescriptorBufferOffsets2EXT(VkCommandBuffer commandBuffer, const VkSetDescriptorBufferOffsetsInfoEXT *pSetDescriptorBufferOffsetsInfo)
{
    struct vkCmdSetDescriptorBufferOffsets2EXT_params params;
    params.commandBuffer = commandBuffer;
    params.pSetDescriptorBufferOffsetsInfo = pSetDescriptorBufferOffsetsInfo;
    UNIX_CALL(vkCmdSetDescriptorBufferOffsets2EXT, &params);
}

void WINAPI vkCmdSetDescriptorBufferOffsetsEXT(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const uint32_t *pBufferIndices, const VkDeviceSize *pOffsets)
{
    struct vkCmdSetDescriptorBufferOffsetsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.layout = layout;
    params.firstSet = firstSet;
    params.setCount = setCount;
    params.pBufferIndices = pBufferIndices;
    params.pOffsets = pOffsets;
    UNIX_CALL(vkCmdSetDescriptorBufferOffsetsEXT, &params);
}

void WINAPI vkCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    struct vkCmdSetDeviceMask_params params;
    params.commandBuffer = commandBuffer;
    params.deviceMask = deviceMask;
    UNIX_CALL(vkCmdSetDeviceMask, &params);
}

void WINAPI vkCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    struct vkCmdSetDeviceMaskKHR_params params;
    params.commandBuffer = commandBuffer;
    params.deviceMask = deviceMask;
    UNIX_CALL(vkCmdSetDeviceMaskKHR, &params);
}

void WINAPI vkCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D *pDiscardRectangles)
{
    struct vkCmdSetDiscardRectangleEXT_params params;
    params.commandBuffer = commandBuffer;
    params.firstDiscardRectangle = firstDiscardRectangle;
    params.discardRectangleCount = discardRectangleCount;
    params.pDiscardRectangles = pDiscardRectangles;
    UNIX_CALL(vkCmdSetDiscardRectangleEXT, &params);
}

void WINAPI vkCmdSetDiscardRectangleEnableEXT(VkCommandBuffer commandBuffer, VkBool32 discardRectangleEnable)
{
    struct vkCmdSetDiscardRectangleEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.discardRectangleEnable = discardRectangleEnable;
    UNIX_CALL(vkCmdSetDiscardRectangleEnableEXT, &params);
}

void WINAPI vkCmdSetDiscardRectangleModeEXT(VkCommandBuffer commandBuffer, VkDiscardRectangleModeEXT discardRectangleMode)
{
    struct vkCmdSetDiscardRectangleModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.discardRectangleMode = discardRectangleMode;
    UNIX_CALL(vkCmdSetDiscardRectangleModeEXT, &params);
}

void WINAPI vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    struct vkCmdSetEvent_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.stageMask = stageMask;
    UNIX_CALL(vkCmdSetEvent, &params);
}

void WINAPI vkCmdSetEvent2(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdSetEvent2_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.pDependencyInfo = pDependencyInfo;
    UNIX_CALL(vkCmdSetEvent2, &params);
}

void WINAPI vkCmdSetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo)
{
    struct vkCmdSetEvent2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.event = event;
    params.pDependencyInfo = pDependencyInfo;
    UNIX_CALL(vkCmdSetEvent2KHR, &params);
}

void WINAPI vkCmdSetExclusiveScissorEnableNV(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkBool32 *pExclusiveScissorEnables)
{
    struct vkCmdSetExclusiveScissorEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstExclusiveScissor = firstExclusiveScissor;
    params.exclusiveScissorCount = exclusiveScissorCount;
    params.pExclusiveScissorEnables = pExclusiveScissorEnables;
    UNIX_CALL(vkCmdSetExclusiveScissorEnableNV, &params);
}

void WINAPI vkCmdSetExclusiveScissorNV(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D *pExclusiveScissors)
{
    struct vkCmdSetExclusiveScissorNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstExclusiveScissor = firstExclusiveScissor;
    params.exclusiveScissorCount = exclusiveScissorCount;
    params.pExclusiveScissors = pExclusiveScissors;
    UNIX_CALL(vkCmdSetExclusiveScissorNV, &params);
}

void WINAPI vkCmdSetExtraPrimitiveOverestimationSizeEXT(VkCommandBuffer commandBuffer, float extraPrimitiveOverestimationSize)
{
    struct vkCmdSetExtraPrimitiveOverestimationSizeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.extraPrimitiveOverestimationSize = extraPrimitiveOverestimationSize;
    UNIX_CALL(vkCmdSetExtraPrimitiveOverestimationSizeEXT, &params);
}

void WINAPI vkCmdSetFragmentShadingRateEnumNV(VkCommandBuffer commandBuffer, VkFragmentShadingRateNV shadingRate, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    struct vkCmdSetFragmentShadingRateEnumNV_params params;
    params.commandBuffer = commandBuffer;
    params.shadingRate = shadingRate;
    params.combinerOps = combinerOps;
    UNIX_CALL(vkCmdSetFragmentShadingRateEnumNV, &params);
}

void WINAPI vkCmdSetFragmentShadingRateKHR(VkCommandBuffer commandBuffer, const VkExtent2D *pFragmentSize, const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    struct vkCmdSetFragmentShadingRateKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pFragmentSize = pFragmentSize;
    params.combinerOps = combinerOps;
    UNIX_CALL(vkCmdSetFragmentShadingRateKHR, &params);
}

void WINAPI vkCmdSetFrontFace(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
{
    struct vkCmdSetFrontFace_params params;
    params.commandBuffer = commandBuffer;
    params.frontFace = frontFace;
    UNIX_CALL(vkCmdSetFrontFace, &params);
}

void WINAPI vkCmdSetFrontFaceEXT(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
{
    struct vkCmdSetFrontFaceEXT_params params;
    params.commandBuffer = commandBuffer;
    params.frontFace = frontFace;
    UNIX_CALL(vkCmdSetFrontFaceEXT, &params);
}

void WINAPI vkCmdSetLineRasterizationModeEXT(VkCommandBuffer commandBuffer, VkLineRasterizationModeEXT lineRasterizationMode)
{
    struct vkCmdSetLineRasterizationModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.lineRasterizationMode = lineRasterizationMode;
    UNIX_CALL(vkCmdSetLineRasterizationModeEXT, &params);
}

void WINAPI vkCmdSetLineStipple(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern)
{
    struct vkCmdSetLineStipple_params params;
    params.commandBuffer = commandBuffer;
    params.lineStippleFactor = lineStippleFactor;
    params.lineStipplePattern = lineStipplePattern;
    UNIX_CALL(vkCmdSetLineStipple, &params);
}

void WINAPI vkCmdSetLineStippleEXT(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern)
{
    struct vkCmdSetLineStippleEXT_params params;
    params.commandBuffer = commandBuffer;
    params.lineStippleFactor = lineStippleFactor;
    params.lineStipplePattern = lineStipplePattern;
    UNIX_CALL(vkCmdSetLineStippleEXT, &params);
}

void WINAPI vkCmdSetLineStippleEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stippledLineEnable)
{
    struct vkCmdSetLineStippleEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.stippledLineEnable = stippledLineEnable;
    UNIX_CALL(vkCmdSetLineStippleEnableEXT, &params);
}

void WINAPI vkCmdSetLineStippleKHR(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern)
{
    struct vkCmdSetLineStippleKHR_params params;
    params.commandBuffer = commandBuffer;
    params.lineStippleFactor = lineStippleFactor;
    params.lineStipplePattern = lineStipplePattern;
    UNIX_CALL(vkCmdSetLineStippleKHR, &params);
}

void WINAPI vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    struct vkCmdSetLineWidth_params params;
    params.commandBuffer = commandBuffer;
    params.lineWidth = lineWidth;
    UNIX_CALL(vkCmdSetLineWidth, &params);
}

void WINAPI vkCmdSetLogicOpEXT(VkCommandBuffer commandBuffer, VkLogicOp logicOp)
{
    struct vkCmdSetLogicOpEXT_params params;
    params.commandBuffer = commandBuffer;
    params.logicOp = logicOp;
    UNIX_CALL(vkCmdSetLogicOpEXT, &params);
}

void WINAPI vkCmdSetLogicOpEnableEXT(VkCommandBuffer commandBuffer, VkBool32 logicOpEnable)
{
    struct vkCmdSetLogicOpEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.logicOpEnable = logicOpEnable;
    UNIX_CALL(vkCmdSetLogicOpEnableEXT, &params);
}

void WINAPI vkCmdSetPatchControlPointsEXT(VkCommandBuffer commandBuffer, uint32_t patchControlPoints)
{
    struct vkCmdSetPatchControlPointsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.patchControlPoints = patchControlPoints;
    UNIX_CALL(vkCmdSetPatchControlPointsEXT, &params);
}

VkResult WINAPI vkCmdSetPerformanceMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceMarkerInfoINTEL *pMarkerInfo)
{
    struct vkCmdSetPerformanceMarkerINTEL_params params;
    NTSTATUS status;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    status = UNIX_CALL(vkCmdSetPerformanceMarkerINTEL, &params);
    assert(!status && "vkCmdSetPerformanceMarkerINTEL");
    return params.result;
}

VkResult WINAPI vkCmdSetPerformanceOverrideINTEL(VkCommandBuffer commandBuffer, const VkPerformanceOverrideInfoINTEL *pOverrideInfo)
{
    struct vkCmdSetPerformanceOverrideINTEL_params params;
    NTSTATUS status;
    params.commandBuffer = commandBuffer;
    params.pOverrideInfo = pOverrideInfo;
    status = UNIX_CALL(vkCmdSetPerformanceOverrideINTEL, &params);
    assert(!status && "vkCmdSetPerformanceOverrideINTEL");
    return params.result;
}

VkResult WINAPI vkCmdSetPerformanceStreamMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo)
{
    struct vkCmdSetPerformanceStreamMarkerINTEL_params params;
    NTSTATUS status;
    params.commandBuffer = commandBuffer;
    params.pMarkerInfo = pMarkerInfo;
    status = UNIX_CALL(vkCmdSetPerformanceStreamMarkerINTEL, &params);
    assert(!status && "vkCmdSetPerformanceStreamMarkerINTEL");
    return params.result;
}

void WINAPI vkCmdSetPolygonModeEXT(VkCommandBuffer commandBuffer, VkPolygonMode polygonMode)
{
    struct vkCmdSetPolygonModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.polygonMode = polygonMode;
    UNIX_CALL(vkCmdSetPolygonModeEXT, &params);
}

void WINAPI vkCmdSetPrimitiveRestartEnable(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable)
{
    struct vkCmdSetPrimitiveRestartEnable_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveRestartEnable = primitiveRestartEnable;
    UNIX_CALL(vkCmdSetPrimitiveRestartEnable, &params);
}

void WINAPI vkCmdSetPrimitiveRestartEnableEXT(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable)
{
    struct vkCmdSetPrimitiveRestartEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveRestartEnable = primitiveRestartEnable;
    UNIX_CALL(vkCmdSetPrimitiveRestartEnableEXT, &params);
}

void WINAPI vkCmdSetPrimitiveTopology(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
    struct vkCmdSetPrimitiveTopology_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveTopology = primitiveTopology;
    UNIX_CALL(vkCmdSetPrimitiveTopology, &params);
}

void WINAPI vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
    struct vkCmdSetPrimitiveTopologyEXT_params params;
    params.commandBuffer = commandBuffer;
    params.primitiveTopology = primitiveTopology;
    UNIX_CALL(vkCmdSetPrimitiveTopologyEXT, &params);
}

void WINAPI vkCmdSetProvokingVertexModeEXT(VkCommandBuffer commandBuffer, VkProvokingVertexModeEXT provokingVertexMode)
{
    struct vkCmdSetProvokingVertexModeEXT_params params;
    params.commandBuffer = commandBuffer;
    params.provokingVertexMode = provokingVertexMode;
    UNIX_CALL(vkCmdSetProvokingVertexModeEXT, &params);
}

void WINAPI vkCmdSetRasterizationSamplesEXT(VkCommandBuffer commandBuffer, VkSampleCountFlagBits rasterizationSamples)
{
    struct vkCmdSetRasterizationSamplesEXT_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizationSamples = rasterizationSamples;
    UNIX_CALL(vkCmdSetRasterizationSamplesEXT, &params);
}

void WINAPI vkCmdSetRasterizationStreamEXT(VkCommandBuffer commandBuffer, uint32_t rasterizationStream)
{
    struct vkCmdSetRasterizationStreamEXT_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizationStream = rasterizationStream;
    UNIX_CALL(vkCmdSetRasterizationStreamEXT, &params);
}

void WINAPI vkCmdSetRasterizerDiscardEnable(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable)
{
    struct vkCmdSetRasterizerDiscardEnable_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizerDiscardEnable = rasterizerDiscardEnable;
    UNIX_CALL(vkCmdSetRasterizerDiscardEnable, &params);
}

void WINAPI vkCmdSetRasterizerDiscardEnableEXT(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable)
{
    struct vkCmdSetRasterizerDiscardEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.rasterizerDiscardEnable = rasterizerDiscardEnable;
    UNIX_CALL(vkCmdSetRasterizerDiscardEnableEXT, &params);
}

void WINAPI vkCmdSetRayTracingPipelineStackSizeKHR(VkCommandBuffer commandBuffer, uint32_t pipelineStackSize)
{
    struct vkCmdSetRayTracingPipelineStackSizeKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineStackSize = pipelineStackSize;
    UNIX_CALL(vkCmdSetRayTracingPipelineStackSizeKHR, &params);
}

void WINAPI vkCmdSetRenderingAttachmentLocations(VkCommandBuffer commandBuffer, const VkRenderingAttachmentLocationInfo *pLocationInfo)
{
    struct vkCmdSetRenderingAttachmentLocations_params params;
    params.commandBuffer = commandBuffer;
    params.pLocationInfo = pLocationInfo;
    UNIX_CALL(vkCmdSetRenderingAttachmentLocations, &params);
}

void WINAPI vkCmdSetRenderingAttachmentLocationsKHR(VkCommandBuffer commandBuffer, const VkRenderingAttachmentLocationInfo *pLocationInfo)
{
    struct vkCmdSetRenderingAttachmentLocationsKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pLocationInfo = pLocationInfo;
    UNIX_CALL(vkCmdSetRenderingAttachmentLocationsKHR, &params);
}

void WINAPI vkCmdSetRenderingInputAttachmentIndices(VkCommandBuffer commandBuffer, const VkRenderingInputAttachmentIndexInfo *pInputAttachmentIndexInfo)
{
    struct vkCmdSetRenderingInputAttachmentIndices_params params;
    params.commandBuffer = commandBuffer;
    params.pInputAttachmentIndexInfo = pInputAttachmentIndexInfo;
    UNIX_CALL(vkCmdSetRenderingInputAttachmentIndices, &params);
}

void WINAPI vkCmdSetRenderingInputAttachmentIndicesKHR(VkCommandBuffer commandBuffer, const VkRenderingInputAttachmentIndexInfo *pInputAttachmentIndexInfo)
{
    struct vkCmdSetRenderingInputAttachmentIndicesKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pInputAttachmentIndexInfo = pInputAttachmentIndexInfo;
    UNIX_CALL(vkCmdSetRenderingInputAttachmentIndicesKHR, &params);
}

void WINAPI vkCmdSetRepresentativeFragmentTestEnableNV(VkCommandBuffer commandBuffer, VkBool32 representativeFragmentTestEnable)
{
    struct vkCmdSetRepresentativeFragmentTestEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.representativeFragmentTestEnable = representativeFragmentTestEnable;
    UNIX_CALL(vkCmdSetRepresentativeFragmentTestEnableNV, &params);
}

void WINAPI vkCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT *pSampleLocationsInfo)
{
    struct vkCmdSetSampleLocationsEXT_params params;
    params.commandBuffer = commandBuffer;
    params.pSampleLocationsInfo = pSampleLocationsInfo;
    UNIX_CALL(vkCmdSetSampleLocationsEXT, &params);
}

void WINAPI vkCmdSetSampleLocationsEnableEXT(VkCommandBuffer commandBuffer, VkBool32 sampleLocationsEnable)
{
    struct vkCmdSetSampleLocationsEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.sampleLocationsEnable = sampleLocationsEnable;
    UNIX_CALL(vkCmdSetSampleLocationsEnableEXT, &params);
}

void WINAPI vkCmdSetSampleMaskEXT(VkCommandBuffer commandBuffer, VkSampleCountFlagBits samples, const VkSampleMask *pSampleMask)
{
    struct vkCmdSetSampleMaskEXT_params params;
    params.commandBuffer = commandBuffer;
    params.samples = samples;
    params.pSampleMask = pSampleMask;
    UNIX_CALL(vkCmdSetSampleMaskEXT, &params);
}

void WINAPI vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors)
{
    struct vkCmdSetScissor_params params;
    params.commandBuffer = commandBuffer;
    params.firstScissor = firstScissor;
    params.scissorCount = scissorCount;
    params.pScissors = pScissors;
    UNIX_CALL(vkCmdSetScissor, &params);
}

void WINAPI vkCmdSetScissorWithCount(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors)
{
    struct vkCmdSetScissorWithCount_params params;
    params.commandBuffer = commandBuffer;
    params.scissorCount = scissorCount;
    params.pScissors = pScissors;
    UNIX_CALL(vkCmdSetScissorWithCount, &params);
}

void WINAPI vkCmdSetScissorWithCountEXT(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors)
{
    struct vkCmdSetScissorWithCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.scissorCount = scissorCount;
    params.pScissors = pScissors;
    UNIX_CALL(vkCmdSetScissorWithCountEXT, &params);
}

void WINAPI vkCmdSetShadingRateImageEnableNV(VkCommandBuffer commandBuffer, VkBool32 shadingRateImageEnable)
{
    struct vkCmdSetShadingRateImageEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.shadingRateImageEnable = shadingRateImageEnable;
    UNIX_CALL(vkCmdSetShadingRateImageEnableNV, &params);
}

void WINAPI vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    struct vkCmdSetStencilCompareMask_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.compareMask = compareMask;
    UNIX_CALL(vkCmdSetStencilCompareMask, &params);
}

void WINAPI vkCmdSetStencilOp(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
    struct vkCmdSetStencilOp_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.failOp = failOp;
    params.passOp = passOp;
    params.depthFailOp = depthFailOp;
    params.compareOp = compareOp;
    UNIX_CALL(vkCmdSetStencilOp, &params);
}

void WINAPI vkCmdSetStencilOpEXT(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
    struct vkCmdSetStencilOpEXT_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.failOp = failOp;
    params.passOp = passOp;
    params.depthFailOp = depthFailOp;
    params.compareOp = compareOp;
    UNIX_CALL(vkCmdSetStencilOpEXT, &params);
}

void WINAPI vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    struct vkCmdSetStencilReference_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.reference = reference;
    UNIX_CALL(vkCmdSetStencilReference, &params);
}

void WINAPI vkCmdSetStencilTestEnable(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable)
{
    struct vkCmdSetStencilTestEnable_params params;
    params.commandBuffer = commandBuffer;
    params.stencilTestEnable = stencilTestEnable;
    UNIX_CALL(vkCmdSetStencilTestEnable, &params);
}

void WINAPI vkCmdSetStencilTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable)
{
    struct vkCmdSetStencilTestEnableEXT_params params;
    params.commandBuffer = commandBuffer;
    params.stencilTestEnable = stencilTestEnable;
    UNIX_CALL(vkCmdSetStencilTestEnableEXT, &params);
}

void WINAPI vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    struct vkCmdSetStencilWriteMask_params params;
    params.commandBuffer = commandBuffer;
    params.faceMask = faceMask;
    params.writeMask = writeMask;
    UNIX_CALL(vkCmdSetStencilWriteMask, &params);
}

void WINAPI vkCmdSetTessellationDomainOriginEXT(VkCommandBuffer commandBuffer, VkTessellationDomainOrigin domainOrigin)
{
    struct vkCmdSetTessellationDomainOriginEXT_params params;
    params.commandBuffer = commandBuffer;
    params.domainOrigin = domainOrigin;
    UNIX_CALL(vkCmdSetTessellationDomainOriginEXT, &params);
}

void WINAPI vkCmdSetVertexInputEXT(VkCommandBuffer commandBuffer, uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT *pVertexAttributeDescriptions)
{
    struct vkCmdSetVertexInputEXT_params params;
    params.commandBuffer = commandBuffer;
    params.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
    params.pVertexBindingDescriptions = pVertexBindingDescriptions;
    params.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
    params.pVertexAttributeDescriptions = pVertexAttributeDescriptions;
    UNIX_CALL(vkCmdSetVertexInputEXT, &params);
}

void WINAPI vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports)
{
    struct vkCmdSetViewport_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pViewports = pViewports;
    UNIX_CALL(vkCmdSetViewport, &params);
}

void WINAPI vkCmdSetViewportShadingRatePaletteNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV *pShadingRatePalettes)
{
    struct vkCmdSetViewportShadingRatePaletteNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pShadingRatePalettes = pShadingRatePalettes;
    UNIX_CALL(vkCmdSetViewportShadingRatePaletteNV, &params);
}

void WINAPI vkCmdSetViewportSwizzleNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportSwizzleNV *pViewportSwizzles)
{
    struct vkCmdSetViewportSwizzleNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pViewportSwizzles = pViewportSwizzles;
    UNIX_CALL(vkCmdSetViewportSwizzleNV, &params);
}

void WINAPI vkCmdSetViewportWScalingEnableNV(VkCommandBuffer commandBuffer, VkBool32 viewportWScalingEnable)
{
    struct vkCmdSetViewportWScalingEnableNV_params params;
    params.commandBuffer = commandBuffer;
    params.viewportWScalingEnable = viewportWScalingEnable;
    UNIX_CALL(vkCmdSetViewportWScalingEnableNV, &params);
}

void WINAPI vkCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV *pViewportWScalings)
{
    struct vkCmdSetViewportWScalingNV_params params;
    params.commandBuffer = commandBuffer;
    params.firstViewport = firstViewport;
    params.viewportCount = viewportCount;
    params.pViewportWScalings = pViewportWScalings;
    UNIX_CALL(vkCmdSetViewportWScalingNV, &params);
}

void WINAPI vkCmdSetViewportWithCount(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports)
{
    struct vkCmdSetViewportWithCount_params params;
    params.commandBuffer = commandBuffer;
    params.viewportCount = viewportCount;
    params.pViewports = pViewports;
    UNIX_CALL(vkCmdSetViewportWithCount, &params);
}

void WINAPI vkCmdSetViewportWithCountEXT(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports)
{
    struct vkCmdSetViewportWithCountEXT_params params;
    params.commandBuffer = commandBuffer;
    params.viewportCount = viewportCount;
    params.pViewports = pViewports;
    UNIX_CALL(vkCmdSetViewportWithCountEXT, &params);
}

void WINAPI vkCmdSubpassShadingHUAWEI(VkCommandBuffer commandBuffer)
{
    struct vkCmdSubpassShadingHUAWEI_params params;
    params.commandBuffer = commandBuffer;
    UNIX_CALL(vkCmdSubpassShadingHUAWEI, &params);
}

void WINAPI vkCmdTraceRaysIndirect2KHR(VkCommandBuffer commandBuffer, VkDeviceAddress indirectDeviceAddress)
{
    struct vkCmdTraceRaysIndirect2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.indirectDeviceAddress = indirectDeviceAddress;
    UNIX_CALL(vkCmdTraceRaysIndirect2KHR, &params);
}

void WINAPI vkCmdTraceRaysIndirectKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, VkDeviceAddress indirectDeviceAddress)
{
    struct vkCmdTraceRaysIndirectKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRaygenShaderBindingTable = pRaygenShaderBindingTable;
    params.pMissShaderBindingTable = pMissShaderBindingTable;
    params.pHitShaderBindingTable = pHitShaderBindingTable;
    params.pCallableShaderBindingTable = pCallableShaderBindingTable;
    params.indirectDeviceAddress = indirectDeviceAddress;
    UNIX_CALL(vkCmdTraceRaysIndirectKHR, &params);
}

void WINAPI vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
{
    struct vkCmdTraceRaysKHR_params params;
    params.commandBuffer = commandBuffer;
    params.pRaygenShaderBindingTable = pRaygenShaderBindingTable;
    params.pMissShaderBindingTable = pMissShaderBindingTable;
    params.pHitShaderBindingTable = pHitShaderBindingTable;
    params.pCallableShaderBindingTable = pCallableShaderBindingTable;
    params.width = width;
    params.height = height;
    params.depth = depth;
    UNIX_CALL(vkCmdTraceRaysKHR, &params);
}

void WINAPI vkCmdTraceRaysNV(VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth)
{
    struct vkCmdTraceRaysNV_params params;
    params.commandBuffer = commandBuffer;
    params.raygenShaderBindingTableBuffer = raygenShaderBindingTableBuffer;
    params.raygenShaderBindingOffset = raygenShaderBindingOffset;
    params.missShaderBindingTableBuffer = missShaderBindingTableBuffer;
    params.missShaderBindingOffset = missShaderBindingOffset;
    params.missShaderBindingStride = missShaderBindingStride;
    params.hitShaderBindingTableBuffer = hitShaderBindingTableBuffer;
    params.hitShaderBindingOffset = hitShaderBindingOffset;
    params.hitShaderBindingStride = hitShaderBindingStride;
    params.callableShaderBindingTableBuffer = callableShaderBindingTableBuffer;
    params.callableShaderBindingOffset = callableShaderBindingOffset;
    params.callableShaderBindingStride = callableShaderBindingStride;
    params.width = width;
    params.height = height;
    params.depth = depth;
    UNIX_CALL(vkCmdTraceRaysNV, &params);
}

void WINAPI vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData)
{
    struct vkCmdUpdateBuffer_params params;
    params.commandBuffer = commandBuffer;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.dataSize = dataSize;
    params.pData = pData;
    UNIX_CALL(vkCmdUpdateBuffer, &params);
}

void WINAPI vkCmdUpdatePipelineIndirectBufferNV(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    struct vkCmdUpdatePipelineIndirectBufferNV_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineBindPoint = pipelineBindPoint;
    params.pipeline = pipeline;
    UNIX_CALL(vkCmdUpdatePipelineIndirectBufferNV, &params);
}

void WINAPI vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    struct vkCmdWaitEvents_params params;
    params.commandBuffer = commandBuffer;
    params.eventCount = eventCount;
    params.pEvents = pEvents;
    params.srcStageMask = srcStageMask;
    params.dstStageMask = dstStageMask;
    params.memoryBarrierCount = memoryBarrierCount;
    params.pMemoryBarriers = pMemoryBarriers;
    params.bufferMemoryBarrierCount = bufferMemoryBarrierCount;
    params.pBufferMemoryBarriers = pBufferMemoryBarriers;
    params.imageMemoryBarrierCount = imageMemoryBarrierCount;
    params.pImageMemoryBarriers = pImageMemoryBarriers;
    UNIX_CALL(vkCmdWaitEvents, &params);
}

void WINAPI vkCmdWaitEvents2(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos)
{
    struct vkCmdWaitEvents2_params params;
    params.commandBuffer = commandBuffer;
    params.eventCount = eventCount;
    params.pEvents = pEvents;
    params.pDependencyInfos = pDependencyInfos;
    UNIX_CALL(vkCmdWaitEvents2, &params);
}

void WINAPI vkCmdWaitEvents2KHR(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos)
{
    struct vkCmdWaitEvents2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.eventCount = eventCount;
    params.pEvents = pEvents;
    params.pDependencyInfos = pDependencyInfos;
    UNIX_CALL(vkCmdWaitEvents2KHR, &params);
}

void WINAPI vkCmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    struct vkCmdWriteAccelerationStructuresPropertiesKHR_params params;
    params.commandBuffer = commandBuffer;
    params.accelerationStructureCount = accelerationStructureCount;
    params.pAccelerationStructures = pAccelerationStructures;
    params.queryType = queryType;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    UNIX_CALL(vkCmdWriteAccelerationStructuresPropertiesKHR, &params);
}

void WINAPI vkCmdWriteAccelerationStructuresPropertiesNV(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureNV *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    struct vkCmdWriteAccelerationStructuresPropertiesNV_params params;
    params.commandBuffer = commandBuffer;
    params.accelerationStructureCount = accelerationStructureCount;
    params.pAccelerationStructures = pAccelerationStructures;
    params.queryType = queryType;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    UNIX_CALL(vkCmdWriteAccelerationStructuresPropertiesNV, &params);
}

void WINAPI vkCmdWriteBufferMarker2AMD(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    struct vkCmdWriteBufferMarker2AMD_params params;
    params.commandBuffer = commandBuffer;
    params.stage = stage;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.marker = marker;
    UNIX_CALL(vkCmdWriteBufferMarker2AMD, &params);
}

void WINAPI vkCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker)
{
    struct vkCmdWriteBufferMarkerAMD_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineStage = pipelineStage;
    params.dstBuffer = dstBuffer;
    params.dstOffset = dstOffset;
    params.marker = marker;
    UNIX_CALL(vkCmdWriteBufferMarkerAMD, &params);
}

void WINAPI vkCmdWriteMicromapsPropertiesEXT(VkCommandBuffer commandBuffer, uint32_t micromapCount, const VkMicromapEXT *pMicromaps, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
{
    struct vkCmdWriteMicromapsPropertiesEXT_params params;
    params.commandBuffer = commandBuffer;
    params.micromapCount = micromapCount;
    params.pMicromaps = pMicromaps;
    params.queryType = queryType;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    UNIX_CALL(vkCmdWriteMicromapsPropertiesEXT, &params);
}

void WINAPI vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdWriteTimestamp_params params;
    params.commandBuffer = commandBuffer;
    params.pipelineStage = pipelineStage;
    params.queryPool = queryPool;
    params.query = query;
    UNIX_CALL(vkCmdWriteTimestamp, &params);
}

void WINAPI vkCmdWriteTimestamp2(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdWriteTimestamp2_params params;
    params.commandBuffer = commandBuffer;
    params.stage = stage;
    params.queryPool = queryPool;
    params.query = query;
    UNIX_CALL(vkCmdWriteTimestamp2, &params);
}

void WINAPI vkCmdWriteTimestamp2KHR(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query)
{
    struct vkCmdWriteTimestamp2KHR_params params;
    params.commandBuffer = commandBuffer;
    params.stage = stage;
    params.queryPool = queryPool;
    params.query = query;
    UNIX_CALL(vkCmdWriteTimestamp2KHR, &params);
}

VkResult WINAPI vkCompileDeferredNV(VkDevice device, VkPipeline pipeline, uint32_t shader)
{
    struct vkCompileDeferredNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pipeline = pipeline;
    params.shader = shader;
    status = UNIX_CALL(vkCompileDeferredNV, &params);
    assert(!status && "vkCompileDeferredNV");
    return params.result;
}

VkResult WINAPI vkConvertCooperativeVectorMatrixNV(VkDevice device, const VkConvertCooperativeVectorMatrixInfoNV *pInfo)
{
    struct vkConvertCooperativeVectorMatrixNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkConvertCooperativeVectorMatrixNV, &params);
    assert(!status && "vkConvertCooperativeVectorMatrixNV");
    return params.result;
}

VkResult WINAPI vkCopyAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureInfoKHR *pInfo)
{
    struct vkCopyAccelerationStructureKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkCopyAccelerationStructureKHR, &params);
    assert(!status && "vkCopyAccelerationStructureKHR");
    return params.result;
}

VkResult WINAPI vkCopyAccelerationStructureToMemoryKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo)
{
    struct vkCopyAccelerationStructureToMemoryKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkCopyAccelerationStructureToMemoryKHR, &params);
    assert(!status && "vkCopyAccelerationStructureToMemoryKHR");
    return params.result;
}

VkResult WINAPI vkCopyImageToImage(VkDevice device, const VkCopyImageToImageInfo *pCopyImageToImageInfo)
{
    struct vkCopyImageToImage_params params;
    NTSTATUS status;
    params.device = device;
    params.pCopyImageToImageInfo = pCopyImageToImageInfo;
    status = UNIX_CALL(vkCopyImageToImage, &params);
    assert(!status && "vkCopyImageToImage");
    return params.result;
}

VkResult WINAPI vkCopyImageToImageEXT(VkDevice device, const VkCopyImageToImageInfo *pCopyImageToImageInfo)
{
    struct vkCopyImageToImageEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCopyImageToImageInfo = pCopyImageToImageInfo;
    status = UNIX_CALL(vkCopyImageToImageEXT, &params);
    assert(!status && "vkCopyImageToImageEXT");
    return params.result;
}

VkResult WINAPI vkCopyImageToMemory(VkDevice device, const VkCopyImageToMemoryInfo *pCopyImageToMemoryInfo)
{
    struct vkCopyImageToMemory_params params;
    NTSTATUS status;
    params.device = device;
    params.pCopyImageToMemoryInfo = pCopyImageToMemoryInfo;
    status = UNIX_CALL(vkCopyImageToMemory, &params);
    assert(!status && "vkCopyImageToMemory");
    return params.result;
}

VkResult WINAPI vkCopyImageToMemoryEXT(VkDevice device, const VkCopyImageToMemoryInfo *pCopyImageToMemoryInfo)
{
    struct vkCopyImageToMemoryEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCopyImageToMemoryInfo = pCopyImageToMemoryInfo;
    status = UNIX_CALL(vkCopyImageToMemoryEXT, &params);
    assert(!status && "vkCopyImageToMemoryEXT");
    return params.result;
}

VkResult WINAPI vkCopyMemoryToAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo)
{
    struct vkCopyMemoryToAccelerationStructureKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkCopyMemoryToAccelerationStructureKHR, &params);
    assert(!status && "vkCopyMemoryToAccelerationStructureKHR");
    return params.result;
}

VkResult WINAPI vkCopyMemoryToImage(VkDevice device, const VkCopyMemoryToImageInfo *pCopyMemoryToImageInfo)
{
    struct vkCopyMemoryToImage_params params;
    NTSTATUS status;
    params.device = device;
    params.pCopyMemoryToImageInfo = pCopyMemoryToImageInfo;
    status = UNIX_CALL(vkCopyMemoryToImage, &params);
    assert(!status && "vkCopyMemoryToImage");
    return params.result;
}

VkResult WINAPI vkCopyMemoryToImageEXT(VkDevice device, const VkCopyMemoryToImageInfo *pCopyMemoryToImageInfo)
{
    struct vkCopyMemoryToImageEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCopyMemoryToImageInfo = pCopyMemoryToImageInfo;
    status = UNIX_CALL(vkCopyMemoryToImageEXT, &params);
    assert(!status && "vkCopyMemoryToImageEXT");
    return params.result;
}

VkResult WINAPI vkCopyMemoryToMicromapEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToMicromapInfoEXT *pInfo)
{
    struct vkCopyMemoryToMicromapEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkCopyMemoryToMicromapEXT, &params);
    assert(!status && "vkCopyMemoryToMicromapEXT");
    return params.result;
}

VkResult WINAPI vkCopyMicromapEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMicromapInfoEXT *pInfo)
{
    struct vkCopyMicromapEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkCopyMicromapEXT, &params);
    assert(!status && "vkCopyMicromapEXT");
    return params.result;
}

VkResult WINAPI vkCopyMicromapToMemoryEXT(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMicromapToMemoryInfoEXT *pInfo)
{
    struct vkCopyMicromapToMemoryEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkCopyMicromapToMemoryEXT, &params);
    assert(!status && "vkCopyMicromapToMemoryEXT");
    return params.result;
}

VkResult WINAPI vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureKHR *pAccelerationStructure)
{
    struct vkCreateAccelerationStructureKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pAccelerationStructure = pAccelerationStructure;
    status = UNIX_CALL(vkCreateAccelerationStructureKHR, &params);
    assert(!status && "vkCreateAccelerationStructureKHR");
    return params.result;
}

VkResult WINAPI vkCreateAccelerationStructureNV(VkDevice device, const VkAccelerationStructureCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureNV *pAccelerationStructure)
{
    struct vkCreateAccelerationStructureNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pAccelerationStructure = pAccelerationStructure;
    status = UNIX_CALL(vkCreateAccelerationStructureNV, &params);
    assert(!status && "vkCreateAccelerationStructureNV");
    return params.result;
}

VkResult WINAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer)
{
    struct vkCreateBuffer_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pBuffer = pBuffer;
    status = UNIX_CALL(vkCreateBuffer, &params);
    assert(!status && "vkCreateBuffer");
    return params.result;
}

VkResult WINAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
{
    struct vkCreateBufferView_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pView = pView;
    status = UNIX_CALL(vkCreateBufferView, &params);
    assert(!status && "vkCreateBufferView");
    return params.result;
}

VkResult WINAPI vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateComputePipelines_params params;
    NTSTATUS status;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    status = UNIX_CALL(vkCreateComputePipelines, &params);
    assert(!status && "vkCreateComputePipelines");
    return params.result;
}

VkResult WINAPI vkCreateCuFunctionNVX(VkDevice device, const VkCuFunctionCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuFunctionNVX *pFunction)
{
    struct vkCreateCuFunctionNVX_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pFunction = pFunction;
    status = UNIX_CALL(vkCreateCuFunctionNVX, &params);
    assert(!status && "vkCreateCuFunctionNVX");
    return params.result;
}

VkResult WINAPI vkCreateCuModuleNVX(VkDevice device, const VkCuModuleCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuModuleNVX *pModule)
{
    struct vkCreateCuModuleNVX_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pModule = pModule;
    status = UNIX_CALL(vkCreateCuModuleNVX, &params);
    assert(!status && "vkCreateCuModuleNVX");
    return params.result;
}

VkResult WINAPI vkCreateDataGraphPipelineSessionARM(VkDevice device, const VkDataGraphPipelineSessionCreateInfoARM *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDataGraphPipelineSessionARM *pSession)
{
    struct vkCreateDataGraphPipelineSessionARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSession = pSession;
    status = UNIX_CALL(vkCreateDataGraphPipelineSessionARM, &params);
    assert(!status && "vkCreateDataGraphPipelineSessionARM");
    return params.result;
}

VkResult WINAPI vkCreateDataGraphPipelinesARM(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkDataGraphPipelineCreateInfoARM *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateDataGraphPipelinesARM_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    status = UNIX_CALL(vkCreateDataGraphPipelinesARM, &params);
    assert(!status && "vkCreateDataGraphPipelinesARM");
    return params.result;
}

VkResult WINAPI vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
    struct vkCreateDebugReportCallbackEXT_params params;
    NTSTATUS status;
    params.instance = instance;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pCallback = pCallback;
    status = UNIX_CALL(vkCreateDebugReportCallbackEXT, &params);
    assert(!status && "vkCreateDebugReportCallbackEXT");
    return params.result;
}

VkResult WINAPI vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    struct vkCreateDebugUtilsMessengerEXT_params params;
    NTSTATUS status;
    params.instance = instance;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pMessenger = pMessenger;
    status = UNIX_CALL(vkCreateDebugUtilsMessengerEXT, &params);
    assert(!status && "vkCreateDebugUtilsMessengerEXT");
    return params.result;
}

VkResult WINAPI vkCreateDeferredOperationKHR(VkDevice device, const VkAllocationCallbacks *pAllocator, VkDeferredOperationKHR *pDeferredOperation)
{
    struct vkCreateDeferredOperationKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pAllocator = pAllocator;
    params.pDeferredOperation = pDeferredOperation;
    status = UNIX_CALL(vkCreateDeferredOperationKHR, &params);
    assert(!status && "vkCreateDeferredOperationKHR");
    return params.result;
}

VkResult WINAPI vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
    struct vkCreateDescriptorPool_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pDescriptorPool = pDescriptorPool;
    status = UNIX_CALL(vkCreateDescriptorPool, &params);
    assert(!status && "vkCreateDescriptorPool");
    return params.result;
}

VkResult WINAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout)
{
    struct vkCreateDescriptorSetLayout_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSetLayout = pSetLayout;
    status = UNIX_CALL(vkCreateDescriptorSetLayout, &params);
    assert(!status && "vkCreateDescriptorSetLayout");
    return params.result;
}

VkResult WINAPI vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    struct vkCreateDescriptorUpdateTemplate_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pDescriptorUpdateTemplate = pDescriptorUpdateTemplate;
    status = UNIX_CALL(vkCreateDescriptorUpdateTemplate, &params);
    assert(!status && "vkCreateDescriptorUpdateTemplate");
    return params.result;
}

VkResult WINAPI vkCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate)
{
    struct vkCreateDescriptorUpdateTemplateKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pDescriptorUpdateTemplate = pDescriptorUpdateTemplate;
    status = UNIX_CALL(vkCreateDescriptorUpdateTemplateKHR, &params);
    assert(!status && "vkCreateDescriptorUpdateTemplateKHR");
    return params.result;
}

VkResult WINAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent)
{
    struct vkCreateEvent_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pEvent = pEvent;
    status = UNIX_CALL(vkCreateEvent, &params);
    assert(!status && "vkCreateEvent");
    return params.result;
}

VkResult WINAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence)
{
    struct vkCreateFence_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pFence = pFence;
    status = UNIX_CALL(vkCreateFence, &params);
    assert(!status && "vkCreateFence");
    return params.result;
}

VkResult WINAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer)
{
    struct vkCreateFramebuffer_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pFramebuffer = pFramebuffer;
    status = UNIX_CALL(vkCreateFramebuffer, &params);
    assert(!status && "vkCreateFramebuffer");
    return params.result;
}

VkResult WINAPI vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateGraphicsPipelines_params params;
    NTSTATUS status;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    status = UNIX_CALL(vkCreateGraphicsPipelines, &params);
    assert(!status && "vkCreateGraphicsPipelines");
    return params.result;
}

VkResult WINAPI vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
    struct vkCreateImage_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pImage = pImage;
    status = UNIX_CALL(vkCreateImage, &params);
    assert(!status && "vkCreateImage");
    return params.result;
}

VkResult WINAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
    struct vkCreateImageView_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pView = pView;
    status = UNIX_CALL(vkCreateImageView, &params);
    assert(!status && "vkCreateImageView");
    return params.result;
}

VkResult WINAPI vkCreateIndirectCommandsLayoutEXT(VkDevice device, const VkIndirectCommandsLayoutCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutEXT *pIndirectCommandsLayout)
{
    struct vkCreateIndirectCommandsLayoutEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pIndirectCommandsLayout = pIndirectCommandsLayout;
    status = UNIX_CALL(vkCreateIndirectCommandsLayoutEXT, &params);
    assert(!status && "vkCreateIndirectCommandsLayoutEXT");
    return params.result;
}

VkResult WINAPI vkCreateIndirectCommandsLayoutNV(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutNV *pIndirectCommandsLayout)
{
    struct vkCreateIndirectCommandsLayoutNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pIndirectCommandsLayout = pIndirectCommandsLayout;
    status = UNIX_CALL(vkCreateIndirectCommandsLayoutNV, &params);
    assert(!status && "vkCreateIndirectCommandsLayoutNV");
    return params.result;
}

VkResult WINAPI vkCreateIndirectExecutionSetEXT(VkDevice device, const VkIndirectExecutionSetCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectExecutionSetEXT *pIndirectExecutionSet)
{
    struct vkCreateIndirectExecutionSetEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pIndirectExecutionSet = pIndirectExecutionSet;
    status = UNIX_CALL(vkCreateIndirectExecutionSetEXT, &params);
    assert(!status && "vkCreateIndirectExecutionSetEXT");
    return params.result;
}

VkResult WINAPI vkCreateMicromapEXT(VkDevice device, const VkMicromapCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkMicromapEXT *pMicromap)
{
    struct vkCreateMicromapEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pMicromap = pMicromap;
    status = UNIX_CALL(vkCreateMicromapEXT, &params);
    assert(!status && "vkCreateMicromapEXT");
    return params.result;
}

VkResult WINAPI vkCreateOpticalFlowSessionNV(VkDevice device, const VkOpticalFlowSessionCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkOpticalFlowSessionNV *pSession)
{
    struct vkCreateOpticalFlowSessionNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSession = pSession;
    status = UNIX_CALL(vkCreateOpticalFlowSessionNV, &params);
    assert(!status && "vkCreateOpticalFlowSessionNV");
    return params.result;
}

VkResult WINAPI vkCreatePipelineBinariesKHR(VkDevice device, const VkPipelineBinaryCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineBinaryHandlesInfoKHR *pBinaries)
{
    struct vkCreatePipelineBinariesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pBinaries = pBinaries;
    status = UNIX_CALL(vkCreatePipelineBinariesKHR, &params);
    assert(!status && "vkCreatePipelineBinariesKHR");
    return params.result;
}

VkResult WINAPI vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
    struct vkCreatePipelineCache_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPipelineCache = pPipelineCache;
    status = UNIX_CALL(vkCreatePipelineCache, &params);
    assert(!status && "vkCreatePipelineCache");
    return params.result;
}

VkResult WINAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout)
{
    struct vkCreatePipelineLayout_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPipelineLayout = pPipelineLayout;
    status = UNIX_CALL(vkCreatePipelineLayout, &params);
    assert(!status && "vkCreatePipelineLayout");
    return params.result;
}

VkResult WINAPI vkCreatePrivateDataSlot(VkDevice device, const VkPrivateDataSlotCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlot *pPrivateDataSlot)
{
    struct vkCreatePrivateDataSlot_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPrivateDataSlot = pPrivateDataSlot;
    status = UNIX_CALL(vkCreatePrivateDataSlot, &params);
    assert(!status && "vkCreatePrivateDataSlot");
    return params.result;
}

VkResult WINAPI vkCreatePrivateDataSlotEXT(VkDevice device, const VkPrivateDataSlotCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlot *pPrivateDataSlot)
{
    struct vkCreatePrivateDataSlotEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pPrivateDataSlot = pPrivateDataSlot;
    status = UNIX_CALL(vkCreatePrivateDataSlotEXT, &params);
    assert(!status && "vkCreatePrivateDataSlotEXT");
    return params.result;
}

VkResult WINAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool)
{
    struct vkCreateQueryPool_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pQueryPool = pQueryPool;
    status = UNIX_CALL(vkCreateQueryPool, &params);
    assert(!status && "vkCreateQueryPool");
    return params.result;
}

VkResult WINAPI vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateRayTracingPipelinesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.deferredOperation = deferredOperation;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    status = UNIX_CALL(vkCreateRayTracingPipelinesKHR, &params);
    assert(!status && "vkCreateRayTracingPipelinesKHR");
    return params.result;
}

VkResult WINAPI vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
{
    struct vkCreateRayTracingPipelinesNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pPipelines = pPipelines;
    status = UNIX_CALL(vkCreateRayTracingPipelinesNV, &params);
    assert(!status && "vkCreateRayTracingPipelinesNV");
    return params.result;
}

VkResult WINAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    struct vkCreateRenderPass_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pRenderPass = pRenderPass;
    status = UNIX_CALL(vkCreateRenderPass, &params);
    assert(!status && "vkCreateRenderPass");
    return params.result;
}

VkResult WINAPI vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    struct vkCreateRenderPass2_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pRenderPass = pRenderPass;
    status = UNIX_CALL(vkCreateRenderPass2, &params);
    assert(!status && "vkCreateRenderPass2");
    return params.result;
}

VkResult WINAPI vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    struct vkCreateRenderPass2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pRenderPass = pRenderPass;
    status = UNIX_CALL(vkCreateRenderPass2KHR, &params);
    assert(!status && "vkCreateRenderPass2KHR");
    return params.result;
}

VkResult WINAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler)
{
    struct vkCreateSampler_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSampler = pSampler;
    status = UNIX_CALL(vkCreateSampler, &params);
    assert(!status && "vkCreateSampler");
    return params.result;
}

VkResult WINAPI vkCreateSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    struct vkCreateSamplerYcbcrConversion_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pYcbcrConversion = pYcbcrConversion;
    status = UNIX_CALL(vkCreateSamplerYcbcrConversion, &params);
    assert(!status && "vkCreateSamplerYcbcrConversion");
    return params.result;
}

VkResult WINAPI vkCreateSamplerYcbcrConversionKHR(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion)
{
    struct vkCreateSamplerYcbcrConversionKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pYcbcrConversion = pYcbcrConversion;
    status = UNIX_CALL(vkCreateSamplerYcbcrConversionKHR, &params);
    assert(!status && "vkCreateSamplerYcbcrConversionKHR");
    return params.result;
}

VkResult WINAPI vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore)
{
    struct vkCreateSemaphore_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSemaphore = pSemaphore;
    status = UNIX_CALL(vkCreateSemaphore, &params);
    assert(!status && "vkCreateSemaphore");
    return params.result;
}

VkResult WINAPI vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule)
{
    struct vkCreateShaderModule_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pShaderModule = pShaderModule;
    status = UNIX_CALL(vkCreateShaderModule, &params);
    assert(!status && "vkCreateShaderModule");
    return params.result;
}

VkResult WINAPI vkCreateShadersEXT(VkDevice device, uint32_t createInfoCount, const VkShaderCreateInfoEXT *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkShaderEXT *pShaders)
{
    struct vkCreateShadersEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.createInfoCount = createInfoCount;
    params.pCreateInfos = pCreateInfos;
    params.pAllocator = pAllocator;
    params.pShaders = pShaders;
    status = UNIX_CALL(vkCreateShadersEXT, &params);
    assert(!status && "vkCreateShadersEXT");
    return params.result;
}

VkResult WINAPI vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain)
{
    struct vkCreateSwapchainKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSwapchain = pSwapchain;
    status = UNIX_CALL(vkCreateSwapchainKHR, &params);
    assert(!status && "vkCreateSwapchainKHR");
    return params.result;
}

VkResult WINAPI vkCreateTensorARM(VkDevice device, const VkTensorCreateInfoARM *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkTensorARM *pTensor)
{
    struct vkCreateTensorARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pTensor = pTensor;
    status = UNIX_CALL(vkCreateTensorARM, &params);
    assert(!status && "vkCreateTensorARM");
    return params.result;
}

VkResult WINAPI vkCreateTensorViewARM(VkDevice device, const VkTensorViewCreateInfoARM *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkTensorViewARM *pView)
{
    struct vkCreateTensorViewARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pView = pView;
    status = UNIX_CALL(vkCreateTensorViewARM, &params);
    assert(!status && "vkCreateTensorViewARM");
    return params.result;
}

VkResult WINAPI vkCreateValidationCacheEXT(VkDevice device, const VkValidationCacheCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkValidationCacheEXT *pValidationCache)
{
    struct vkCreateValidationCacheEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pValidationCache = pValidationCache;
    status = UNIX_CALL(vkCreateValidationCacheEXT, &params);
    assert(!status && "vkCreateValidationCacheEXT");
    return params.result;
}

VkResult WINAPI vkCreateVideoSessionKHR(VkDevice device, const VkVideoSessionCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkVideoSessionKHR *pVideoSession)
{
    struct vkCreateVideoSessionKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pVideoSession = pVideoSession;
    status = UNIX_CALL(vkCreateVideoSessionKHR, &params);
    assert(!status && "vkCreateVideoSessionKHR");
    return params.result;
}

VkResult WINAPI vkCreateVideoSessionParametersKHR(VkDevice device, const VkVideoSessionParametersCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkVideoSessionParametersKHR *pVideoSessionParameters)
{
    struct vkCreateVideoSessionParametersKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pVideoSessionParameters = pVideoSessionParameters;
    status = UNIX_CALL(vkCreateVideoSessionParametersKHR, &params);
    assert(!status && "vkCreateVideoSessionParametersKHR");
    return params.result;
}

VkResult WINAPI vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
    struct vkCreateWin32SurfaceKHR_params params;
    NTSTATUS status;
    params.instance = instance;
    params.pCreateInfo = pCreateInfo;
    params.pAllocator = pAllocator;
    params.pSurface = pSurface;
    status = UNIX_CALL(vkCreateWin32SurfaceKHR, &params);
    assert(!status && "vkCreateWin32SurfaceKHR");
    return params.result;
}

VkResult WINAPI vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo)
{
    struct vkDebugMarkerSetObjectNameEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pNameInfo = pNameInfo;
    status = UNIX_CALL(vkDebugMarkerSetObjectNameEXT, &params);
    assert(!status && "vkDebugMarkerSetObjectNameEXT");
    return params.result;
}

VkResult WINAPI vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo)
{
    struct vkDebugMarkerSetObjectTagEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pTagInfo = pTagInfo;
    status = UNIX_CALL(vkDebugMarkerSetObjectTagEXT, &params);
    assert(!status && "vkDebugMarkerSetObjectTagEXT");
    return params.result;
}

void WINAPI vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage)
{
    struct vkDebugReportMessageEXT_params params;
    NTSTATUS status;
    params.instance = instance;
    params.flags = flags;
    params.objectType = objectType;
    params.object = object;
    params.location = location;
    params.messageCode = messageCode;
    params.pLayerPrefix = pLayerPrefix;
    params.pMessage = pMessage;
    status = UNIX_CALL(vkDebugReportMessageEXT, &params);
    assert(!status && "vkDebugReportMessageEXT");
}

VkResult WINAPI vkDeferredOperationJoinKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    struct vkDeferredOperationJoinKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.operation = operation;
    status = UNIX_CALL(vkDeferredOperationJoinKHR, &params);
    assert(!status && "vkDeferredOperationJoinKHR");
    return params.result;
}

void WINAPI vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyAccelerationStructureKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.accelerationStructure = accelerationStructure;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyAccelerationStructureKHR, &params);
    assert(!status && "vkDestroyAccelerationStructureKHR");
}

void WINAPI vkDestroyAccelerationStructureNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyAccelerationStructureNV_params params;
    NTSTATUS status;
    params.device = device;
    params.accelerationStructure = accelerationStructure;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyAccelerationStructureNV, &params);
    assert(!status && "vkDestroyAccelerationStructureNV");
}

void WINAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyBuffer_params params;
    NTSTATUS status;
    params.device = device;
    params.buffer = buffer;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyBuffer, &params);
    assert(!status && "vkDestroyBuffer");
}

void WINAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyBufferView_params params;
    NTSTATUS status;
    params.device = device;
    params.bufferView = bufferView;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyBufferView, &params);
    assert(!status && "vkDestroyBufferView");
}

void WINAPI vkDestroyCuFunctionNVX(VkDevice device, VkCuFunctionNVX function, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyCuFunctionNVX_params params;
    NTSTATUS status;
    params.device = device;
    params.function = function;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyCuFunctionNVX, &params);
    assert(!status && "vkDestroyCuFunctionNVX");
}

void WINAPI vkDestroyCuModuleNVX(VkDevice device, VkCuModuleNVX module, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyCuModuleNVX_params params;
    NTSTATUS status;
    params.device = device;
    params.module = module;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyCuModuleNVX, &params);
    assert(!status && "vkDestroyCuModuleNVX");
}

void WINAPI vkDestroyDataGraphPipelineSessionARM(VkDevice device, VkDataGraphPipelineSessionARM session, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDataGraphPipelineSessionARM_params params;
    NTSTATUS status;
    params.device = device;
    params.session = session;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDataGraphPipelineSessionARM, &params);
    assert(!status && "vkDestroyDataGraphPipelineSessionARM");
}

void WINAPI vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDebugReportCallbackEXT_params params;
    NTSTATUS status;
    params.instance = instance;
    params.callback = callback;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDebugReportCallbackEXT, &params);
    assert(!status && "vkDestroyDebugReportCallbackEXT");
}

void WINAPI vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDebugUtilsMessengerEXT_params params;
    NTSTATUS status;
    params.instance = instance;
    params.messenger = messenger;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDebugUtilsMessengerEXT, &params);
    assert(!status && "vkDestroyDebugUtilsMessengerEXT");
}

void WINAPI vkDestroyDeferredOperationKHR(VkDevice device, VkDeferredOperationKHR operation, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDeferredOperationKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.operation = operation;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDeferredOperationKHR, &params);
    assert(!status && "vkDestroyDeferredOperationKHR");
}

void WINAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorPool_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorPool = descriptorPool;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDescriptorPool, &params);
    assert(!status && "vkDestroyDescriptorPool");
}

void WINAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorSetLayout_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorSetLayout = descriptorSetLayout;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDescriptorSetLayout, &params);
    assert(!status && "vkDestroyDescriptorSetLayout");
}

void WINAPI vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorUpdateTemplate_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDescriptorUpdateTemplate, &params);
    assert(!status && "vkDestroyDescriptorUpdateTemplate");
}

void WINAPI vkDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyDescriptorUpdateTemplateKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyDescriptorUpdateTemplateKHR, &params);
    assert(!status && "vkDestroyDescriptorUpdateTemplateKHR");
}

void WINAPI vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyEvent_params params;
    NTSTATUS status;
    params.device = device;
    params.event = event;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyEvent, &params);
    assert(!status && "vkDestroyEvent");
}

void WINAPI vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyFence_params params;
    NTSTATUS status;
    params.device = device;
    params.fence = fence;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyFence, &params);
    assert(!status && "vkDestroyFence");
}

void WINAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyFramebuffer_params params;
    NTSTATUS status;
    params.device = device;
    params.framebuffer = framebuffer;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyFramebuffer, &params);
    assert(!status && "vkDestroyFramebuffer");
}

void WINAPI vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyImage_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyImage, &params);
    assert(!status && "vkDestroyImage");
}

void WINAPI vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyImageView_params params;
    NTSTATUS status;
    params.device = device;
    params.imageView = imageView;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyImageView, &params);
    assert(!status && "vkDestroyImageView");
}

void WINAPI vkDestroyIndirectCommandsLayoutEXT(VkDevice device, VkIndirectCommandsLayoutEXT indirectCommandsLayout, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyIndirectCommandsLayoutEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.indirectCommandsLayout = indirectCommandsLayout;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyIndirectCommandsLayoutEXT, &params);
    assert(!status && "vkDestroyIndirectCommandsLayoutEXT");
}

void WINAPI vkDestroyIndirectCommandsLayoutNV(VkDevice device, VkIndirectCommandsLayoutNV indirectCommandsLayout, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyIndirectCommandsLayoutNV_params params;
    NTSTATUS status;
    params.device = device;
    params.indirectCommandsLayout = indirectCommandsLayout;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyIndirectCommandsLayoutNV, &params);
    assert(!status && "vkDestroyIndirectCommandsLayoutNV");
}

void WINAPI vkDestroyIndirectExecutionSetEXT(VkDevice device, VkIndirectExecutionSetEXT indirectExecutionSet, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyIndirectExecutionSetEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.indirectExecutionSet = indirectExecutionSet;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyIndirectExecutionSetEXT, &params);
    assert(!status && "vkDestroyIndirectExecutionSetEXT");
}

void WINAPI vkDestroyMicromapEXT(VkDevice device, VkMicromapEXT micromap, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyMicromapEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.micromap = micromap;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyMicromapEXT, &params);
    assert(!status && "vkDestroyMicromapEXT");
}

void WINAPI vkDestroyOpticalFlowSessionNV(VkDevice device, VkOpticalFlowSessionNV session, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyOpticalFlowSessionNV_params params;
    NTSTATUS status;
    params.device = device;
    params.session = session;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyOpticalFlowSessionNV, &params);
    assert(!status && "vkDestroyOpticalFlowSessionNV");
}

void WINAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPipeline_params params;
    NTSTATUS status;
    params.device = device;
    params.pipeline = pipeline;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyPipeline, &params);
    assert(!status && "vkDestroyPipeline");
}

void WINAPI vkDestroyPipelineBinaryKHR(VkDevice device, VkPipelineBinaryKHR pipelineBinary, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPipelineBinaryKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pipelineBinary = pipelineBinary;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyPipelineBinaryKHR, &params);
    assert(!status && "vkDestroyPipelineBinaryKHR");
}

void WINAPI vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPipelineCache_params params;
    NTSTATUS status;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyPipelineCache, &params);
    assert(!status && "vkDestroyPipelineCache");
}

void WINAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPipelineLayout_params params;
    NTSTATUS status;
    params.device = device;
    params.pipelineLayout = pipelineLayout;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyPipelineLayout, &params);
    assert(!status && "vkDestroyPipelineLayout");
}

void WINAPI vkDestroyPrivateDataSlot(VkDevice device, VkPrivateDataSlot privateDataSlot, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPrivateDataSlot_params params;
    NTSTATUS status;
    params.device = device;
    params.privateDataSlot = privateDataSlot;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyPrivateDataSlot, &params);
    assert(!status && "vkDestroyPrivateDataSlot");
}

void WINAPI vkDestroyPrivateDataSlotEXT(VkDevice device, VkPrivateDataSlot privateDataSlot, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyPrivateDataSlotEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.privateDataSlot = privateDataSlot;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyPrivateDataSlotEXT, &params);
    assert(!status && "vkDestroyPrivateDataSlotEXT");
}

void WINAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyQueryPool_params params;
    NTSTATUS status;
    params.device = device;
    params.queryPool = queryPool;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyQueryPool, &params);
    assert(!status && "vkDestroyQueryPool");
}

void WINAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyRenderPass_params params;
    NTSTATUS status;
    params.device = device;
    params.renderPass = renderPass;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyRenderPass, &params);
    assert(!status && "vkDestroyRenderPass");
}

void WINAPI vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySampler_params params;
    NTSTATUS status;
    params.device = device;
    params.sampler = sampler;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroySampler, &params);
    assert(!status && "vkDestroySampler");
}

void WINAPI vkDestroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySamplerYcbcrConversion_params params;
    NTSTATUS status;
    params.device = device;
    params.ycbcrConversion = ycbcrConversion;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroySamplerYcbcrConversion, &params);
    assert(!status && "vkDestroySamplerYcbcrConversion");
}

void WINAPI vkDestroySamplerYcbcrConversionKHR(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySamplerYcbcrConversionKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.ycbcrConversion = ycbcrConversion;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroySamplerYcbcrConversionKHR, &params);
    assert(!status && "vkDestroySamplerYcbcrConversionKHR");
}

void WINAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySemaphore_params params;
    NTSTATUS status;
    params.device = device;
    params.semaphore = semaphore;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroySemaphore, &params);
    assert(!status && "vkDestroySemaphore");
}

void WINAPI vkDestroyShaderEXT(VkDevice device, VkShaderEXT shader, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyShaderEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.shader = shader;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyShaderEXT, &params);
    assert(!status && "vkDestroyShaderEXT");
}

void WINAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyShaderModule_params params;
    NTSTATUS status;
    params.device = device;
    params.shaderModule = shaderModule;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyShaderModule, &params);
    assert(!status && "vkDestroyShaderModule");
}

void WINAPI vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySurfaceKHR_params params;
    NTSTATUS status;
    params.instance = instance;
    params.surface = surface;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroySurfaceKHR, &params);
    assert(!status && "vkDestroySurfaceKHR");
}

void WINAPI vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroySwapchainKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroySwapchainKHR, &params);
    assert(!status && "vkDestroySwapchainKHR");
}

void WINAPI vkDestroyTensorARM(VkDevice device, VkTensorARM tensor, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyTensorARM_params params;
    NTSTATUS status;
    params.device = device;
    params.tensor = tensor;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyTensorARM, &params);
    assert(!status && "vkDestroyTensorARM");
}

void WINAPI vkDestroyTensorViewARM(VkDevice device, VkTensorViewARM tensorView, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyTensorViewARM_params params;
    NTSTATUS status;
    params.device = device;
    params.tensorView = tensorView;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyTensorViewARM, &params);
    assert(!status && "vkDestroyTensorViewARM");
}

void WINAPI vkDestroyValidationCacheEXT(VkDevice device, VkValidationCacheEXT validationCache, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyValidationCacheEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.validationCache = validationCache;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyValidationCacheEXT, &params);
    assert(!status && "vkDestroyValidationCacheEXT");
}

void WINAPI vkDestroyVideoSessionKHR(VkDevice device, VkVideoSessionKHR videoSession, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyVideoSessionKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.videoSession = videoSession;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyVideoSessionKHR, &params);
    assert(!status && "vkDestroyVideoSessionKHR");
}

void WINAPI vkDestroyVideoSessionParametersKHR(VkDevice device, VkVideoSessionParametersKHR videoSessionParameters, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyVideoSessionParametersKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.videoSessionParameters = videoSessionParameters;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyVideoSessionParametersKHR, &params);
    assert(!status && "vkDestroyVideoSessionParametersKHR");
}

VkResult WINAPI vkDeviceWaitIdle(VkDevice device)
{
    struct vkDeviceWaitIdle_params params;
    NTSTATUS status;
    params.device = device;
    status = UNIX_CALL(vkDeviceWaitIdle, &params);
    assert(!status && "vkDeviceWaitIdle");
    return params.result;
}

VkResult WINAPI vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    struct vkEndCommandBuffer_params params;
    NTSTATUS status;
    params.commandBuffer = commandBuffer;
    status = UNIX_CALL(vkEndCommandBuffer, &params);
    assert(!status && "vkEndCommandBuffer");
    return params.result;
}

VkResult WINAPI vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
    struct vkEnumerateDeviceExtensionProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pLayerName = pLayerName;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkEnumerateDeviceExtensionProperties, &params);
    assert(!status && "vkEnumerateDeviceExtensionProperties");
    return params.result;
}

VkResult WINAPI vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    struct vkEnumerateDeviceLayerProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkEnumerateDeviceLayerProperties, &params);
    assert(!status && "vkEnumerateDeviceLayerProperties");
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    struct vkEnumeratePhysicalDeviceGroups_params params;
    NTSTATUS status;
    params.instance = instance;
    params.pPhysicalDeviceGroupCount = pPhysicalDeviceGroupCount;
    params.pPhysicalDeviceGroupProperties = pPhysicalDeviceGroupProperties;
    status = UNIX_CALL(vkEnumeratePhysicalDeviceGroups, &params);
    assert(!status && "vkEnumeratePhysicalDeviceGroups");
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    struct vkEnumeratePhysicalDeviceGroupsKHR_params params;
    NTSTATUS status;
    params.instance = instance;
    params.pPhysicalDeviceGroupCount = pPhysicalDeviceGroupCount;
    params.pPhysicalDeviceGroupProperties = pPhysicalDeviceGroupProperties;
    status = UNIX_CALL(vkEnumeratePhysicalDeviceGroupsKHR, &params);
    assert(!status && "vkEnumeratePhysicalDeviceGroupsKHR");
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pCounterCount, VkPerformanceCounterKHR *pCounters, VkPerformanceCounterDescriptionKHR *pCounterDescriptions)
{
    struct vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.queueFamilyIndex = queueFamilyIndex;
    params.pCounterCount = pCounterCount;
    params.pCounters = pCounters;
    params.pCounterDescriptions = pCounterDescriptions;
    status = UNIX_CALL(vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR, &params);
    assert(!status && "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR");
    return params.result;
}

VkResult WINAPI vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices)
{
    struct vkEnumeratePhysicalDevices_params params;
    NTSTATUS status;
    params.instance = instance;
    params.pPhysicalDeviceCount = pPhysicalDeviceCount;
    params.pPhysicalDevices = pPhysicalDevices;
    status = UNIX_CALL(vkEnumeratePhysicalDevices, &params);
    assert(!status && "vkEnumeratePhysicalDevices");
    return params.result;
}

VkResult WINAPI vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    struct vkFlushMappedMemoryRanges_params params;
    NTSTATUS status;
    params.device = device;
    params.memoryRangeCount = memoryRangeCount;
    params.pMemoryRanges = pMemoryRanges;
    status = UNIX_CALL(vkFlushMappedMemoryRanges, &params);
    assert(!status && "vkFlushMappedMemoryRanges");
    return params.result;
}

VkResult WINAPI vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets)
{
    struct vkFreeDescriptorSets_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorPool = descriptorPool;
    params.descriptorSetCount = descriptorSetCount;
    params.pDescriptorSets = pDescriptorSets;
    status = UNIX_CALL(vkFreeDescriptorSets, &params);
    assert(!status && "vkFreeDescriptorSets");
    return params.result;
}

void WINAPI vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator)
{
    struct vkFreeMemory_params params;
    NTSTATUS status;
    params.device = device;
    params.memory = memory;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkFreeMemory, &params);
    assert(!status && "vkFreeMemory");
}

void WINAPI vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo, const uint32_t *pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo)
{
    struct vkGetAccelerationStructureBuildSizesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.buildType = buildType;
    params.pBuildInfo = pBuildInfo;
    params.pMaxPrimitiveCounts = pMaxPrimitiveCounts;
    params.pSizeInfo = pSizeInfo;
    status = UNIX_CALL(vkGetAccelerationStructureBuildSizesKHR, &params);
    assert(!status && "vkGetAccelerationStructureBuildSizesKHR");
}

VkDeviceAddress WINAPI vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR *pInfo)
{
    struct vkGetAccelerationStructureDeviceAddressKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetAccelerationStructureDeviceAddressKHR, &params);
    assert(!status && "vkGetAccelerationStructureDeviceAddressKHR");
    return params.result;
}

VkResult WINAPI vkGetAccelerationStructureHandleNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, size_t dataSize, void *pData)
{
    struct vkGetAccelerationStructureHandleNV_params params;
    NTSTATUS status;
    params.device = device;
    params.accelerationStructure = accelerationStructure;
    params.dataSize = dataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetAccelerationStructureHandleNV, &params);
    assert(!status && "vkGetAccelerationStructureHandleNV");
    return params.result;
}

void WINAPI vkGetAccelerationStructureMemoryRequirementsNV(VkDevice device, const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2KHR *pMemoryRequirements)
{
    struct vkGetAccelerationStructureMemoryRequirementsNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetAccelerationStructureMemoryRequirementsNV, &params);
    assert(!status && "vkGetAccelerationStructureMemoryRequirementsNV");
}

VkResult WINAPI vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(VkDevice device, const VkAccelerationStructureCaptureDescriptorDataInfoEXT *pInfo, void *pData)
{
    struct vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pData = pData;
    status = UNIX_CALL(vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT, &params);
    assert(!status && "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT");
    return params.result;
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferDeviceAddress_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetBufferDeviceAddress, &params);
    assert(!status && "vkGetBufferDeviceAddress");
    return params.result;
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddressEXT(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferDeviceAddressEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetBufferDeviceAddressEXT, &params);
    assert(!status && "vkGetBufferDeviceAddressEXT");
    return params.result;
}

VkDeviceAddress WINAPI vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferDeviceAddressKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetBufferDeviceAddressKHR, &params);
    assert(!status && "vkGetBufferDeviceAddressKHR");
    return params.result;
}

void WINAPI vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements)
{
    struct vkGetBufferMemoryRequirements_params params;
    NTSTATUS status;
    params.device = device;
    params.buffer = buffer;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetBufferMemoryRequirements, &params);
    assert(!status && "vkGetBufferMemoryRequirements");
}

void WINAPI vkGetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetBufferMemoryRequirements2_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetBufferMemoryRequirements2, &params);
    assert(!status && "vkGetBufferMemoryRequirements2");
}

void WINAPI vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetBufferMemoryRequirements2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetBufferMemoryRequirements2KHR, &params);
    assert(!status && "vkGetBufferMemoryRequirements2KHR");
}

uint64_t WINAPI vkGetBufferOpaqueCaptureAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferOpaqueCaptureAddress_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetBufferOpaqueCaptureAddress, &params);
    assert(!status && "vkGetBufferOpaqueCaptureAddress");
    return params.result;
}

uint64_t WINAPI vkGetBufferOpaqueCaptureAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo)
{
    struct vkGetBufferOpaqueCaptureAddressKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetBufferOpaqueCaptureAddressKHR, &params);
    assert(!status && "vkGetBufferOpaqueCaptureAddressKHR");
    return params.result;
}

VkResult WINAPI vkGetBufferOpaqueCaptureDescriptorDataEXT(VkDevice device, const VkBufferCaptureDescriptorDataInfoEXT *pInfo, void *pData)
{
    struct vkGetBufferOpaqueCaptureDescriptorDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pData = pData;
    status = UNIX_CALL(vkGetBufferOpaqueCaptureDescriptorDataEXT, &params);
    assert(!status && "vkGetBufferOpaqueCaptureDescriptorDataEXT");
    return params.result;
}

VkResult WINAPI vkGetCalibratedTimestampsEXT(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoKHR *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation)
{
    struct vkGetCalibratedTimestampsEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.timestampCount = timestampCount;
    params.pTimestampInfos = pTimestampInfos;
    params.pTimestamps = pTimestamps;
    params.pMaxDeviation = pMaxDeviation;
    status = UNIX_CALL(vkGetCalibratedTimestampsEXT, &params);
    assert(!status && "vkGetCalibratedTimestampsEXT");
    return params.result;
}

VkResult WINAPI vkGetCalibratedTimestampsKHR(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoKHR *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation)
{
    struct vkGetCalibratedTimestampsKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.timestampCount = timestampCount;
    params.pTimestampInfos = pTimestampInfos;
    params.pTimestamps = pTimestamps;
    params.pMaxDeviation = pMaxDeviation;
    status = UNIX_CALL(vkGetCalibratedTimestampsKHR, &params);
    assert(!status && "vkGetCalibratedTimestampsKHR");
    return params.result;
}

void WINAPI vkGetClusterAccelerationStructureBuildSizesNV(VkDevice device, const VkClusterAccelerationStructureInputInfoNV *pInfo, VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo)
{
    struct vkGetClusterAccelerationStructureBuildSizesNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pSizeInfo = pSizeInfo;
    status = UNIX_CALL(vkGetClusterAccelerationStructureBuildSizesNV, &params);
    assert(!status && "vkGetClusterAccelerationStructureBuildSizesNV");
}

VkResult WINAPI vkGetDataGraphPipelineAvailablePropertiesARM(VkDevice device, const VkDataGraphPipelineInfoARM *pPipelineInfo, uint32_t *pPropertiesCount, VkDataGraphPipelinePropertyARM *pProperties)
{
    struct vkGetDataGraphPipelineAvailablePropertiesARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pPipelineInfo = pPipelineInfo;
    params.pPropertiesCount = pPropertiesCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetDataGraphPipelineAvailablePropertiesARM, &params);
    assert(!status && "vkGetDataGraphPipelineAvailablePropertiesARM");
    return params.result;
}

VkResult WINAPI vkGetDataGraphPipelinePropertiesARM(VkDevice device, const VkDataGraphPipelineInfoARM *pPipelineInfo, uint32_t propertiesCount, VkDataGraphPipelinePropertyQueryResultARM *pProperties)
{
    struct vkGetDataGraphPipelinePropertiesARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pPipelineInfo = pPipelineInfo;
    params.propertiesCount = propertiesCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetDataGraphPipelinePropertiesARM, &params);
    assert(!status && "vkGetDataGraphPipelinePropertiesARM");
    return params.result;
}

VkResult WINAPI vkGetDataGraphPipelineSessionBindPointRequirementsARM(VkDevice device, const VkDataGraphPipelineSessionBindPointRequirementsInfoARM *pInfo, uint32_t *pBindPointRequirementCount, VkDataGraphPipelineSessionBindPointRequirementARM *pBindPointRequirements)
{
    struct vkGetDataGraphPipelineSessionBindPointRequirementsARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pBindPointRequirementCount = pBindPointRequirementCount;
    params.pBindPointRequirements = pBindPointRequirements;
    status = UNIX_CALL(vkGetDataGraphPipelineSessionBindPointRequirementsARM, &params);
    assert(!status && "vkGetDataGraphPipelineSessionBindPointRequirementsARM");
    return params.result;
}

void WINAPI vkGetDataGraphPipelineSessionMemoryRequirementsARM(VkDevice device, const VkDataGraphPipelineSessionMemoryRequirementsInfoARM *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDataGraphPipelineSessionMemoryRequirementsARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetDataGraphPipelineSessionMemoryRequirementsARM, &params);
    assert(!status && "vkGetDataGraphPipelineSessionMemoryRequirementsARM");
}

uint32_t WINAPI vkGetDeferredOperationMaxConcurrencyKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    struct vkGetDeferredOperationMaxConcurrencyKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.operation = operation;
    status = UNIX_CALL(vkGetDeferredOperationMaxConcurrencyKHR, &params);
    assert(!status && "vkGetDeferredOperationMaxConcurrencyKHR");
    return params.result;
}

VkResult WINAPI vkGetDeferredOperationResultKHR(VkDevice device, VkDeferredOperationKHR operation)
{
    struct vkGetDeferredOperationResultKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.operation = operation;
    status = UNIX_CALL(vkGetDeferredOperationResultKHR, &params);
    assert(!status && "vkGetDeferredOperationResultKHR");
    return params.result;
}

void WINAPI vkGetDescriptorEXT(VkDevice device, const VkDescriptorGetInfoEXT *pDescriptorInfo, size_t dataSize, void *pDescriptor)
{
    struct vkGetDescriptorEXT_params params;
    params.device = device;
    params.pDescriptorInfo = pDescriptorInfo;
    params.dataSize = dataSize;
    params.pDescriptor = pDescriptor;
    UNIX_CALL(vkGetDescriptorEXT, &params);
}

void WINAPI vkGetDescriptorSetHostMappingVALVE(VkDevice device, VkDescriptorSet descriptorSet, void **ppData)
{
    struct vkGetDescriptorSetHostMappingVALVE_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorSet = descriptorSet;
    params.ppData = ppData;
    status = UNIX_CALL(vkGetDescriptorSetHostMappingVALVE, &params);
    assert(!status && "vkGetDescriptorSetHostMappingVALVE");
}

void WINAPI vkGetDescriptorSetLayoutBindingOffsetEXT(VkDevice device, VkDescriptorSetLayout layout, uint32_t binding, VkDeviceSize *pOffset)
{
    struct vkGetDescriptorSetLayoutBindingOffsetEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.layout = layout;
    params.binding = binding;
    params.pOffset = pOffset;
    status = UNIX_CALL(vkGetDescriptorSetLayoutBindingOffsetEXT, &params);
    assert(!status && "vkGetDescriptorSetLayoutBindingOffsetEXT");
}

void WINAPI vkGetDescriptorSetLayoutHostMappingInfoVALVE(VkDevice device, const VkDescriptorSetBindingReferenceVALVE *pBindingReference, VkDescriptorSetLayoutHostMappingInfoVALVE *pHostMapping)
{
    struct vkGetDescriptorSetLayoutHostMappingInfoVALVE_params params;
    NTSTATUS status;
    params.device = device;
    params.pBindingReference = pBindingReference;
    params.pHostMapping = pHostMapping;
    status = UNIX_CALL(vkGetDescriptorSetLayoutHostMappingInfoVALVE, &params);
    assert(!status && "vkGetDescriptorSetLayoutHostMappingInfoVALVE");
}

void WINAPI vkGetDescriptorSetLayoutSizeEXT(VkDevice device, VkDescriptorSetLayout layout, VkDeviceSize *pLayoutSizeInBytes)
{
    struct vkGetDescriptorSetLayoutSizeEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.layout = layout;
    params.pLayoutSizeInBytes = pLayoutSizeInBytes;
    status = UNIX_CALL(vkGetDescriptorSetLayoutSizeEXT, &params);
    assert(!status && "vkGetDescriptorSetLayoutSizeEXT");
}

void WINAPI vkGetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    struct vkGetDescriptorSetLayoutSupport_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pSupport = pSupport;
    status = UNIX_CALL(vkGetDescriptorSetLayoutSupport, &params);
    assert(!status && "vkGetDescriptorSetLayoutSupport");
}

void WINAPI vkGetDescriptorSetLayoutSupportKHR(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport)
{
    struct vkGetDescriptorSetLayoutSupportKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pSupport = pSupport;
    status = UNIX_CALL(vkGetDescriptorSetLayoutSupportKHR, &params);
    assert(!status && "vkGetDescriptorSetLayoutSupportKHR");
}

void WINAPI vkGetDeviceAccelerationStructureCompatibilityKHR(VkDevice device, const VkAccelerationStructureVersionInfoKHR *pVersionInfo, VkAccelerationStructureCompatibilityKHR *pCompatibility)
{
    struct vkGetDeviceAccelerationStructureCompatibilityKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pVersionInfo = pVersionInfo;
    params.pCompatibility = pCompatibility;
    status = UNIX_CALL(vkGetDeviceAccelerationStructureCompatibilityKHR, &params);
    assert(!status && "vkGetDeviceAccelerationStructureCompatibilityKHR");
}

void WINAPI vkGetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceBufferMemoryRequirements_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetDeviceBufferMemoryRequirements, &params);
    assert(!status && "vkGetDeviceBufferMemoryRequirements");
}

void WINAPI vkGetDeviceBufferMemoryRequirementsKHR(VkDevice device, const VkDeviceBufferMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceBufferMemoryRequirementsKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetDeviceBufferMemoryRequirementsKHR, &params);
    assert(!status && "vkGetDeviceBufferMemoryRequirementsKHR");
}

VkResult WINAPI vkGetDeviceFaultInfoEXT(VkDevice device, VkDeviceFaultCountsEXT *pFaultCounts, VkDeviceFaultInfoEXT *pFaultInfo)
{
    struct vkGetDeviceFaultInfoEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pFaultCounts = pFaultCounts;
    params.pFaultInfo = pFaultInfo;
    status = UNIX_CALL(vkGetDeviceFaultInfoEXT, &params);
    assert(!status && "vkGetDeviceFaultInfoEXT");
    return params.result;
}

void WINAPI vkGetDeviceGroupPeerMemoryFeatures(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    struct vkGetDeviceGroupPeerMemoryFeatures_params params;
    NTSTATUS status;
    params.device = device;
    params.heapIndex = heapIndex;
    params.localDeviceIndex = localDeviceIndex;
    params.remoteDeviceIndex = remoteDeviceIndex;
    params.pPeerMemoryFeatures = pPeerMemoryFeatures;
    status = UNIX_CALL(vkGetDeviceGroupPeerMemoryFeatures, &params);
    assert(!status && "vkGetDeviceGroupPeerMemoryFeatures");
}

void WINAPI vkGetDeviceGroupPeerMemoryFeaturesKHR(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures)
{
    struct vkGetDeviceGroupPeerMemoryFeaturesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.heapIndex = heapIndex;
    params.localDeviceIndex = localDeviceIndex;
    params.remoteDeviceIndex = remoteDeviceIndex;
    params.pPeerMemoryFeatures = pPeerMemoryFeatures;
    status = UNIX_CALL(vkGetDeviceGroupPeerMemoryFeaturesKHR, &params);
    assert(!status && "vkGetDeviceGroupPeerMemoryFeaturesKHR");
}

VkResult WINAPI vkGetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pDeviceGroupPresentCapabilities)
{
    struct vkGetDeviceGroupPresentCapabilitiesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pDeviceGroupPresentCapabilities = pDeviceGroupPresentCapabilities;
    status = UNIX_CALL(vkGetDeviceGroupPresentCapabilitiesKHR, &params);
    assert(!status && "vkGetDeviceGroupPresentCapabilitiesKHR");
    return params.result;
}

VkResult WINAPI vkGetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes)
{
    struct vkGetDeviceGroupSurfacePresentModesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.surface = surface;
    params.pModes = pModes;
    status = UNIX_CALL(vkGetDeviceGroupSurfacePresentModesKHR, &params);
    assert(!status && "vkGetDeviceGroupSurfacePresentModesKHR");
    return params.result;
}

void WINAPI vkGetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceImageMemoryRequirements_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetDeviceImageMemoryRequirements, &params);
    assert(!status && "vkGetDeviceImageMemoryRequirements");
}

void WINAPI vkGetDeviceImageMemoryRequirementsKHR(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceImageMemoryRequirementsKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetDeviceImageMemoryRequirementsKHR, &params);
    assert(!status && "vkGetDeviceImageMemoryRequirementsKHR");
}

void WINAPI vkGetDeviceImageSparseMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetDeviceImageSparseMemoryRequirements_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    status = UNIX_CALL(vkGetDeviceImageSparseMemoryRequirements, &params);
    assert(!status && "vkGetDeviceImageSparseMemoryRequirements");
}

void WINAPI vkGetDeviceImageSparseMemoryRequirementsKHR(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetDeviceImageSparseMemoryRequirementsKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    status = UNIX_CALL(vkGetDeviceImageSparseMemoryRequirementsKHR, &params);
    assert(!status && "vkGetDeviceImageSparseMemoryRequirementsKHR");
}

void WINAPI vkGetDeviceImageSubresourceLayout(VkDevice device, const VkDeviceImageSubresourceInfo *pInfo, VkSubresourceLayout2 *pLayout)
{
    struct vkGetDeviceImageSubresourceLayout_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pLayout = pLayout;
    status = UNIX_CALL(vkGetDeviceImageSubresourceLayout, &params);
    assert(!status && "vkGetDeviceImageSubresourceLayout");
}

void WINAPI vkGetDeviceImageSubresourceLayoutKHR(VkDevice device, const VkDeviceImageSubresourceInfo *pInfo, VkSubresourceLayout2 *pLayout)
{
    struct vkGetDeviceImageSubresourceLayoutKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pLayout = pLayout;
    status = UNIX_CALL(vkGetDeviceImageSubresourceLayoutKHR, &params);
    assert(!status && "vkGetDeviceImageSubresourceLayoutKHR");
}

void WINAPI vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes)
{
    struct vkGetDeviceMemoryCommitment_params params;
    NTSTATUS status;
    params.device = device;
    params.memory = memory;
    params.pCommittedMemoryInBytes = pCommittedMemoryInBytes;
    status = UNIX_CALL(vkGetDeviceMemoryCommitment, &params);
    assert(!status && "vkGetDeviceMemoryCommitment");
}

uint64_t WINAPI vkGetDeviceMemoryOpaqueCaptureAddress(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddress_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetDeviceMemoryOpaqueCaptureAddress, &params);
    assert(!status && "vkGetDeviceMemoryOpaqueCaptureAddress");
    return params.result;
}

uint64_t WINAPI vkGetDeviceMemoryOpaqueCaptureAddressKHR(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo)
{
    struct vkGetDeviceMemoryOpaqueCaptureAddressKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetDeviceMemoryOpaqueCaptureAddressKHR, &params);
    assert(!status && "vkGetDeviceMemoryOpaqueCaptureAddressKHR");
    return params.result;
}

void WINAPI vkGetDeviceMicromapCompatibilityEXT(VkDevice device, const VkMicromapVersionInfoEXT *pVersionInfo, VkAccelerationStructureCompatibilityKHR *pCompatibility)
{
    struct vkGetDeviceMicromapCompatibilityEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pVersionInfo = pVersionInfo;
    params.pCompatibility = pCompatibility;
    status = UNIX_CALL(vkGetDeviceMicromapCompatibilityEXT, &params);
    assert(!status && "vkGetDeviceMicromapCompatibilityEXT");
}

void WINAPI vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue)
{
    struct vkGetDeviceQueue_params params;
    NTSTATUS status;
    params.device = device;
    params.queueFamilyIndex = queueFamilyIndex;
    params.queueIndex = queueIndex;
    params.pQueue = pQueue;
    status = UNIX_CALL(vkGetDeviceQueue, &params);
    assert(!status && "vkGetDeviceQueue");
}

void WINAPI vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue)
{
    struct vkGetDeviceQueue2_params params;
    NTSTATUS status;
    params.device = device;
    params.pQueueInfo = pQueueInfo;
    params.pQueue = pQueue;
    status = UNIX_CALL(vkGetDeviceQueue2, &params);
    assert(!status && "vkGetDeviceQueue2");
}

VkResult WINAPI vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(VkDevice device, VkRenderPass renderpass, VkExtent2D *pMaxWorkgroupSize)
{
    struct vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI_params params;
    NTSTATUS status;
    params.device = device;
    params.renderpass = renderpass;
    params.pMaxWorkgroupSize = pMaxWorkgroupSize;
    status = UNIX_CALL(vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI, &params);
    assert(!status && "vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI");
    return params.result;
}

void WINAPI vkGetDeviceTensorMemoryRequirementsARM(VkDevice device, const VkDeviceTensorMemoryRequirementsARM *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetDeviceTensorMemoryRequirementsARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetDeviceTensorMemoryRequirementsARM, &params);
    assert(!status && "vkGetDeviceTensorMemoryRequirementsARM");
}

VkResult WINAPI vkGetDynamicRenderingTilePropertiesQCOM(VkDevice device, const VkRenderingInfo *pRenderingInfo, VkTilePropertiesQCOM *pProperties)
{
    struct vkGetDynamicRenderingTilePropertiesQCOM_params params;
    NTSTATUS status;
    params.device = device;
    params.pRenderingInfo = pRenderingInfo;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetDynamicRenderingTilePropertiesQCOM, &params);
    assert(!status && "vkGetDynamicRenderingTilePropertiesQCOM");
    return params.result;
}

VkResult WINAPI vkGetEncodedVideoSessionParametersKHR(VkDevice device, const VkVideoEncodeSessionParametersGetInfoKHR *pVideoSessionParametersInfo, VkVideoEncodeSessionParametersFeedbackInfoKHR *pFeedbackInfo, size_t *pDataSize, void *pData)
{
    struct vkGetEncodedVideoSessionParametersKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pVideoSessionParametersInfo = pVideoSessionParametersInfo;
    params.pFeedbackInfo = pFeedbackInfo;
    params.pDataSize = pDataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetEncodedVideoSessionParametersKHR, &params);
    assert(!status && "vkGetEncodedVideoSessionParametersKHR");
    return params.result;
}

VkResult WINAPI vkGetEventStatus(VkDevice device, VkEvent event)
{
    struct vkGetEventStatus_params params;
    NTSTATUS status;
    params.device = device;
    params.event = event;
    status = UNIX_CALL(vkGetEventStatus, &params);
    assert(!status && "vkGetEventStatus");
    return params.result;
}

VkResult WINAPI vkGetFenceStatus(VkDevice device, VkFence fence)
{
    struct vkGetFenceStatus_params params;
    NTSTATUS status;
    params.device = device;
    params.fence = fence;
    status = UNIX_CALL(vkGetFenceStatus, &params);
    assert(!status && "vkGetFenceStatus");
    return params.result;
}

VkResult WINAPI vkGetFramebufferTilePropertiesQCOM(VkDevice device, VkFramebuffer framebuffer, uint32_t *pPropertiesCount, VkTilePropertiesQCOM *pProperties)
{
    struct vkGetFramebufferTilePropertiesQCOM_params params;
    NTSTATUS status;
    params.device = device;
    params.framebuffer = framebuffer;
    params.pPropertiesCount = pPropertiesCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetFramebufferTilePropertiesQCOM, &params);
    assert(!status && "vkGetFramebufferTilePropertiesQCOM");
    return params.result;
}

void WINAPI vkGetGeneratedCommandsMemoryRequirementsEXT(VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoEXT *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetGeneratedCommandsMemoryRequirementsEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetGeneratedCommandsMemoryRequirementsEXT, &params);
    assert(!status && "vkGetGeneratedCommandsMemoryRequirementsEXT");
}

void WINAPI vkGetGeneratedCommandsMemoryRequirementsNV(VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetGeneratedCommandsMemoryRequirementsNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetGeneratedCommandsMemoryRequirementsNV, &params);
    assert(!status && "vkGetGeneratedCommandsMemoryRequirementsNV");
}

void WINAPI vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements)
{
    struct vkGetImageMemoryRequirements_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetImageMemoryRequirements, &params);
    assert(!status && "vkGetImageMemoryRequirements");
}

void WINAPI vkGetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetImageMemoryRequirements2_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetImageMemoryRequirements2, &params);
    assert(!status && "vkGetImageMemoryRequirements2");
}

void WINAPI vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetImageMemoryRequirements2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetImageMemoryRequirements2KHR, &params);
    assert(!status && "vkGetImageMemoryRequirements2KHR");
}

VkResult WINAPI vkGetImageOpaqueCaptureDescriptorDataEXT(VkDevice device, const VkImageCaptureDescriptorDataInfoEXT *pInfo, void *pData)
{
    struct vkGetImageOpaqueCaptureDescriptorDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pData = pData;
    status = UNIX_CALL(vkGetImageOpaqueCaptureDescriptorDataEXT, &params);
    assert(!status && "vkGetImageOpaqueCaptureDescriptorDataEXT");
    return params.result;
}

void WINAPI vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements *pSparseMemoryRequirements)
{
    struct vkGetImageSparseMemoryRequirements_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    status = UNIX_CALL(vkGetImageSparseMemoryRequirements, &params);
    assert(!status && "vkGetImageSparseMemoryRequirements");
}

void WINAPI vkGetImageSparseMemoryRequirements2(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetImageSparseMemoryRequirements2_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    status = UNIX_CALL(vkGetImageSparseMemoryRequirements2, &params);
    assert(!status && "vkGetImageSparseMemoryRequirements2");
}

void WINAPI vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements)
{
    struct vkGetImageSparseMemoryRequirements2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pSparseMemoryRequirementCount = pSparseMemoryRequirementCount;
    params.pSparseMemoryRequirements = pSparseMemoryRequirements;
    status = UNIX_CALL(vkGetImageSparseMemoryRequirements2KHR, &params);
    assert(!status && "vkGetImageSparseMemoryRequirements2KHR");
}

void WINAPI vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout)
{
    struct vkGetImageSubresourceLayout_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.pSubresource = pSubresource;
    params.pLayout = pLayout;
    status = UNIX_CALL(vkGetImageSubresourceLayout, &params);
    assert(!status && "vkGetImageSubresourceLayout");
}

void WINAPI vkGetImageSubresourceLayout2(VkDevice device, VkImage image, const VkImageSubresource2 *pSubresource, VkSubresourceLayout2 *pLayout)
{
    struct vkGetImageSubresourceLayout2_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.pSubresource = pSubresource;
    params.pLayout = pLayout;
    status = UNIX_CALL(vkGetImageSubresourceLayout2, &params);
    assert(!status && "vkGetImageSubresourceLayout2");
}

void WINAPI vkGetImageSubresourceLayout2EXT(VkDevice device, VkImage image, const VkImageSubresource2 *pSubresource, VkSubresourceLayout2 *pLayout)
{
    struct vkGetImageSubresourceLayout2EXT_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.pSubresource = pSubresource;
    params.pLayout = pLayout;
    status = UNIX_CALL(vkGetImageSubresourceLayout2EXT, &params);
    assert(!status && "vkGetImageSubresourceLayout2EXT");
}

void WINAPI vkGetImageSubresourceLayout2KHR(VkDevice device, VkImage image, const VkImageSubresource2 *pSubresource, VkSubresourceLayout2 *pLayout)
{
    struct vkGetImageSubresourceLayout2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.image = image;
    params.pSubresource = pSubresource;
    params.pLayout = pLayout;
    status = UNIX_CALL(vkGetImageSubresourceLayout2KHR, &params);
    assert(!status && "vkGetImageSubresourceLayout2KHR");
}

VkResult WINAPI vkGetImageViewAddressNVX(VkDevice device, VkImageView imageView, VkImageViewAddressPropertiesNVX *pProperties)
{
    struct vkGetImageViewAddressNVX_params params;
    NTSTATUS status;
    params.device = device;
    params.imageView = imageView;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetImageViewAddressNVX, &params);
    assert(!status && "vkGetImageViewAddressNVX");
    return params.result;
}

uint64_t WINAPI vkGetImageViewHandle64NVX(VkDevice device, const VkImageViewHandleInfoNVX *pInfo)
{
    struct vkGetImageViewHandle64NVX_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetImageViewHandle64NVX, &params);
    assert(!status && "vkGetImageViewHandle64NVX");
    return params.result;
}

uint32_t WINAPI vkGetImageViewHandleNVX(VkDevice device, const VkImageViewHandleInfoNVX *pInfo)
{
    struct vkGetImageViewHandleNVX_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetImageViewHandleNVX, &params);
    assert(!status && "vkGetImageViewHandleNVX");
    return params.result;
}

VkResult WINAPI vkGetImageViewOpaqueCaptureDescriptorDataEXT(VkDevice device, const VkImageViewCaptureDescriptorDataInfoEXT *pInfo, void *pData)
{
    struct vkGetImageViewOpaqueCaptureDescriptorDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pData = pData;
    status = UNIX_CALL(vkGetImageViewOpaqueCaptureDescriptorDataEXT, &params);
    assert(!status && "vkGetImageViewOpaqueCaptureDescriptorDataEXT");
    return params.result;
}

void WINAPI vkGetLatencyTimingsNV(VkDevice device, VkSwapchainKHR swapchain, VkGetLatencyMarkerInfoNV *pLatencyMarkerInfo)
{
    struct vkGetLatencyTimingsNV_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.pLatencyMarkerInfo = pLatencyMarkerInfo;
    status = UNIX_CALL(vkGetLatencyTimingsNV, &params);
    assert(!status && "vkGetLatencyTimingsNV");
}

VkResult WINAPI vkGetMemoryHostPointerPropertiesEXT(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, const void *pHostPointer, VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties)
{
    struct vkGetMemoryHostPointerPropertiesEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.handleType = handleType;
    params.pHostPointer = pHostPointer;
    params.pMemoryHostPointerProperties = pMemoryHostPointerProperties;
    status = UNIX_CALL(vkGetMemoryHostPointerPropertiesEXT, &params);
    assert(!status && "vkGetMemoryHostPointerPropertiesEXT");
    return params.result;
}

VkResult WINAPI vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR *pGetWin32HandleInfo, HANDLE *pHandle)
{
    struct vkGetMemoryWin32HandleKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pGetWin32HandleInfo = pGetWin32HandleInfo;
    params.pHandle = pHandle;
    status = UNIX_CALL(vkGetMemoryWin32HandleKHR, &params);
    assert(!status && "vkGetMemoryWin32HandleKHR");
    return params.result;
}

VkResult WINAPI vkGetMemoryWin32HandlePropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR *pMemoryWin32HandleProperties)
{
    struct vkGetMemoryWin32HandlePropertiesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.handleType = handleType;
    params.handle = handle;
    params.pMemoryWin32HandleProperties = pMemoryWin32HandleProperties;
    status = UNIX_CALL(vkGetMemoryWin32HandlePropertiesKHR, &params);
    assert(!status && "vkGetMemoryWin32HandlePropertiesKHR");
    return params.result;
}

void WINAPI vkGetMicromapBuildSizesEXT(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkMicromapBuildInfoEXT *pBuildInfo, VkMicromapBuildSizesInfoEXT *pSizeInfo)
{
    struct vkGetMicromapBuildSizesEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.buildType = buildType;
    params.pBuildInfo = pBuildInfo;
    params.pSizeInfo = pSizeInfo;
    status = UNIX_CALL(vkGetMicromapBuildSizesEXT, &params);
    assert(!status && "vkGetMicromapBuildSizesEXT");
}

void WINAPI vkGetPartitionedAccelerationStructuresBuildSizesNV(VkDevice device, const VkPartitionedAccelerationStructureInstancesInputNV *pInfo, VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo)
{
    struct vkGetPartitionedAccelerationStructuresBuildSizesNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pSizeInfo = pSizeInfo;
    status = UNIX_CALL(vkGetPartitionedAccelerationStructuresBuildSizesNV, &params);
    assert(!status && "vkGetPartitionedAccelerationStructuresBuildSizesNV");
}

VkResult WINAPI vkGetPerformanceParameterINTEL(VkDevice device, VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL *pValue)
{
    struct vkGetPerformanceParameterINTEL_params params;
    NTSTATUS status;
    params.device = device;
    params.parameter = parameter;
    params.pValue = pValue;
    status = UNIX_CALL(vkGetPerformanceParameterINTEL, &params);
    assert(!status && "vkGetPerformanceParameterINTEL");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainKHR *pTimeDomains)
{
    struct vkGetPhysicalDeviceCalibrateableTimeDomainsEXT_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pTimeDomainCount = pTimeDomainCount;
    params.pTimeDomains = pTimeDomains;
    status = UNIX_CALL(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, &params);
    assert(!status && "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCalibrateableTimeDomainsKHR(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainKHR *pTimeDomains)
{
    struct vkGetPhysicalDeviceCalibrateableTimeDomainsKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pTimeDomainCount = pTimeDomainCount;
    params.pTimeDomains = pTimeDomains;
    status = UNIX_CALL(vkGetPhysicalDeviceCalibrateableTimeDomainsKHR, &params);
    assert(!status && "vkGetPhysicalDeviceCalibrateableTimeDomainsKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixFlexibleDimensionsPropertiesNV *pProperties)
{
    struct vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV, &params);
    assert(!status && "vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixPropertiesKHR *pProperties)
{
    struct vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCooperativeMatrixPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixPropertiesNV *pProperties)
{
    struct vkGetPhysicalDeviceCooperativeMatrixPropertiesNV_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceCooperativeMatrixPropertiesNV, &params);
    assert(!status && "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceCooperativeVectorPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeVectorPropertiesNV *pProperties)
{
    struct vkGetPhysicalDeviceCooperativeVectorPropertiesNV_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceCooperativeVectorPropertiesNV, &params);
    assert(!status && "vkGetPhysicalDeviceCooperativeVectorPropertiesNV");
    return params.result;
}

void WINAPI vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    struct vkGetPhysicalDeviceExternalBufferProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pExternalBufferInfo = pExternalBufferInfo;
    params.pExternalBufferProperties = pExternalBufferProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceExternalBufferProperties, &params);
    assert(!status && "vkGetPhysicalDeviceExternalBufferProperties");
}

void WINAPI vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties)
{
    struct vkGetPhysicalDeviceExternalBufferPropertiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pExternalBufferInfo = pExternalBufferInfo;
    params.pExternalBufferProperties = pExternalBufferProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceExternalBufferPropertiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
}

void WINAPI vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    struct vkGetPhysicalDeviceExternalFenceProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pExternalFenceInfo = pExternalFenceInfo;
    params.pExternalFenceProperties = pExternalFenceProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceExternalFenceProperties, &params);
    assert(!status && "vkGetPhysicalDeviceExternalFenceProperties");
}

void WINAPI vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties)
{
    struct vkGetPhysicalDeviceExternalFencePropertiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pExternalFenceInfo = pExternalFenceInfo;
    params.pExternalFenceProperties = pExternalFenceProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceExternalFencePropertiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceExternalFencePropertiesKHR");
}

void WINAPI vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    struct vkGetPhysicalDeviceExternalSemaphoreProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pExternalSemaphoreInfo = pExternalSemaphoreInfo;
    params.pExternalSemaphoreProperties = pExternalSemaphoreProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceExternalSemaphoreProperties, &params);
    assert(!status && "vkGetPhysicalDeviceExternalSemaphoreProperties");
}

void WINAPI vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties)
{
    struct vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pExternalSemaphoreInfo = pExternalSemaphoreInfo;
    params.pExternalSemaphoreProperties = pExternalSemaphoreProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceExternalSemaphorePropertiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
}

void WINAPI vkGetPhysicalDeviceExternalTensorPropertiesARM(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalTensorInfoARM *pExternalTensorInfo, VkExternalTensorPropertiesARM *pExternalTensorProperties)
{
    struct vkGetPhysicalDeviceExternalTensorPropertiesARM_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pExternalTensorInfo = pExternalTensorInfo;
    params.pExternalTensorProperties = pExternalTensorProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceExternalTensorPropertiesARM, &params);
    assert(!status && "vkGetPhysicalDeviceExternalTensorPropertiesARM");
}

void WINAPI vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
    struct vkGetPhysicalDeviceFeatures_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pFeatures = pFeatures;
    status = UNIX_CALL(vkGetPhysicalDeviceFeatures, &params);
    assert(!status && "vkGetPhysicalDeviceFeatures");
}

void WINAPI vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    struct vkGetPhysicalDeviceFeatures2_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pFeatures = pFeatures;
    status = UNIX_CALL(vkGetPhysicalDeviceFeatures2, &params);
    assert(!status && "vkGetPhysicalDeviceFeatures2");
}

void WINAPI vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures)
{
    struct vkGetPhysicalDeviceFeatures2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pFeatures = pFeatures;
    status = UNIX_CALL(vkGetPhysicalDeviceFeatures2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceFeatures2KHR");
}

void WINAPI vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties)
{
    struct vkGetPhysicalDeviceFormatProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.pFormatProperties = pFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceFormatProperties, &params);
    assert(!status && "vkGetPhysicalDeviceFormatProperties");
}

void WINAPI vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    struct vkGetPhysicalDeviceFormatProperties2_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.pFormatProperties = pFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceFormatProperties2, &params);
    assert(!status && "vkGetPhysicalDeviceFormatProperties2");
}

void WINAPI vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties)
{
    struct vkGetPhysicalDeviceFormatProperties2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.pFormatProperties = pFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceFormatProperties2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceFormatProperties2KHR");
}

VkResult WINAPI vkGetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t *pFragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR *pFragmentShadingRates)
{
    struct vkGetPhysicalDeviceFragmentShadingRatesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pFragmentShadingRateCount = pFragmentShadingRateCount;
    params.pFragmentShadingRates = pFragmentShadingRates;
    status = UNIX_CALL(vkGetPhysicalDeviceFragmentShadingRatesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceFragmentShadingRatesKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceImageFormatProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.type = type;
    params.tiling = tiling;
    params.usage = usage;
    params.flags = flags;
    params.pImageFormatProperties = pImageFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceImageFormatProperties, &params);
    assert(!status && "vkGetPhysicalDeviceImageFormatProperties");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceImageFormatProperties2_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pImageFormatInfo = pImageFormatInfo;
    params.pImageFormatProperties = pImageFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceImageFormatProperties2, &params);
    assert(!status && "vkGetPhysicalDeviceImageFormatProperties2");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceImageFormatProperties2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pImageFormatInfo = pImageFormatInfo;
    params.pImageFormatProperties = pImageFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceImageFormatProperties2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceImageFormatProperties2KHR");
    return params.result;
}

void WINAPI vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
    struct vkGetPhysicalDeviceMemoryProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pMemoryProperties = pMemoryProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceMemoryProperties, &params);
    assert(!status && "vkGetPhysicalDeviceMemoryProperties");
}

void WINAPI vkGetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    struct vkGetPhysicalDeviceMemoryProperties2_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pMemoryProperties = pMemoryProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceMemoryProperties2, &params);
    assert(!status && "vkGetPhysicalDeviceMemoryProperties2");
}

void WINAPI vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties)
{
    struct vkGetPhysicalDeviceMemoryProperties2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pMemoryProperties = pMemoryProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceMemoryProperties2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceMemoryProperties2KHR");
}

void WINAPI vkGetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT *pMultisampleProperties)
{
    struct vkGetPhysicalDeviceMultisamplePropertiesEXT_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.samples = samples;
    params.pMultisampleProperties = pMultisampleProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceMultisamplePropertiesEXT, &params);
    assert(!status && "vkGetPhysicalDeviceMultisamplePropertiesEXT");
}

VkResult WINAPI vkGetPhysicalDeviceOpticalFlowImageFormatsNV(VkPhysicalDevice physicalDevice, const VkOpticalFlowImageFormatInfoNV *pOpticalFlowImageFormatInfo, uint32_t *pFormatCount, VkOpticalFlowImageFormatPropertiesNV *pImageFormatProperties)
{
    struct vkGetPhysicalDeviceOpticalFlowImageFormatsNV_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pOpticalFlowImageFormatInfo = pOpticalFlowImageFormatInfo;
    params.pFormatCount = pFormatCount;
    params.pImageFormatProperties = pImageFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceOpticalFlowImageFormatsNV, &params);
    assert(!status && "vkGetPhysicalDeviceOpticalFlowImageFormatsNV");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects)
{
    struct vkGetPhysicalDevicePresentRectanglesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pRectCount = pRectCount;
    params.pRects = pRects;
    status = UNIX_CALL(vkGetPhysicalDevicePresentRectanglesKHR, &params);
    assert(!status && "vkGetPhysicalDevicePresentRectanglesKHR");
    return params.result;
}

void WINAPI vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
    struct vkGetPhysicalDeviceProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceProperties, &params);
    assert(!status && "vkGetPhysicalDeviceProperties");
}

void WINAPI vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceQueueFamilyDataGraphProcessingEngineInfoARM *pQueueFamilyDataGraphProcessingEngineInfo, VkQueueFamilyDataGraphProcessingEnginePropertiesARM *pQueueFamilyDataGraphProcessingEngineProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pQueueFamilyDataGraphProcessingEngineInfo = pQueueFamilyDataGraphProcessingEngineInfo;
    params.pQueueFamilyDataGraphProcessingEngineProperties = pQueueFamilyDataGraphProcessingEngineProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM, &params);
    assert(!status && "vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM");
}

VkResult WINAPI vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pQueueFamilyDataGraphPropertyCount, VkQueueFamilyDataGraphPropertiesARM *pQueueFamilyDataGraphProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.queueFamilyIndex = queueFamilyIndex;
    params.pQueueFamilyDataGraphPropertyCount = pQueueFamilyDataGraphPropertyCount;
    params.pQueueFamilyDataGraphProperties = pQueueFamilyDataGraphProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM, &params);
    assert(!status && "vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM");
    return params.result;
}

void WINAPI vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(VkPhysicalDevice physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo, uint32_t *pNumPasses)
{
    struct vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pPerformanceQueryCreateInfo = pPerformanceQueryCreateInfo;
    params.pNumPasses = pNumPasses;
    status = UNIX_CALL(vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR");
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pQueueFamilyPropertyCount = pQueueFamilyPropertyCount;
    params.pQueueFamilyProperties = pQueueFamilyProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceQueueFamilyProperties, &params);
    assert(!status && "vkGetPhysicalDeviceQueueFamilyProperties");
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pQueueFamilyPropertyCount = pQueueFamilyPropertyCount;
    params.pQueueFamilyProperties = pQueueFamilyProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, &params);
    assert(!status && "vkGetPhysicalDeviceQueueFamilyProperties2");
}

void WINAPI vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties)
{
    struct vkGetPhysicalDeviceQueueFamilyProperties2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pQueueFamilyPropertyCount = pQueueFamilyPropertyCount;
    params.pQueueFamilyProperties = pQueueFamilyProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceQueueFamilyProperties2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.format = format;
    params.type = type;
    params.samples = samples;
    params.usage = usage;
    params.tiling = tiling;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceSparseImageFormatProperties, &params);
    assert(!status && "vkGetPhysicalDeviceSparseImageFormatProperties");
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pFormatInfo = pFormatInfo;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceSparseImageFormatProperties2, &params);
    assert(!status && "vkGetPhysicalDeviceSparseImageFormatProperties2");
}

void WINAPI vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties)
{
    struct vkGetPhysicalDeviceSparseImageFormatProperties2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pFormatInfo = pFormatInfo;
    params.pPropertyCount = pPropertyCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceSparseImageFormatProperties2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
}

VkResult WINAPI vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(VkPhysicalDevice physicalDevice, uint32_t *pCombinationCount, VkFramebufferMixedSamplesCombinationNV *pCombinations)
{
    struct vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pCombinationCount = pCombinationCount;
    params.pCombinations = pCombinations;
    status = UNIX_CALL(vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV, &params);
    assert(!status && "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
    struct vkGetPhysicalDeviceSurfaceCapabilities2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pSurfaceInfo = pSurfaceInfo;
    params.pSurfaceCapabilities = pSurfaceCapabilities;
    status = UNIX_CALL(vkGetPhysicalDeviceSurfaceCapabilities2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    struct vkGetPhysicalDeviceSurfaceCapabilitiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pSurfaceCapabilities = pSurfaceCapabilities;
    status = UNIX_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats)
{
    struct vkGetPhysicalDeviceSurfaceFormats2KHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pSurfaceInfo = pSurfaceInfo;
    params.pSurfaceFormatCount = pSurfaceFormatCount;
    params.pSurfaceFormats = pSurfaceFormats;
    status = UNIX_CALL(vkGetPhysicalDeviceSurfaceFormats2KHR, &params);
    assert(!status && "vkGetPhysicalDeviceSurfaceFormats2KHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats)
{
    struct vkGetPhysicalDeviceSurfaceFormatsKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pSurfaceFormatCount = pSurfaceFormatCount;
    params.pSurfaceFormats = pSurfaceFormats;
    status = UNIX_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR, &params);
    assert(!status && "vkGetPhysicalDeviceSurfaceFormatsKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes)
{
    struct vkGetPhysicalDeviceSurfacePresentModesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.surface = surface;
    params.pPresentModeCount = pPresentModeCount;
    params.pPresentModes = pPresentModes;
    status = UNIX_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceSurfacePresentModesKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported)
{
    struct vkGetPhysicalDeviceSurfaceSupportKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.queueFamilyIndex = queueFamilyIndex;
    params.surface = surface;
    params.pSupported = pSupported;
    status = UNIX_CALL(vkGetPhysicalDeviceSurfaceSupportKHR, &params);
    assert(!status && "vkGetPhysicalDeviceSurfaceSupportKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolProperties *pToolProperties)
{
    struct vkGetPhysicalDeviceToolProperties_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pToolCount = pToolCount;
    params.pToolProperties = pToolProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceToolProperties, &params);
    assert(!status && "vkGetPhysicalDeviceToolProperties");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolProperties *pToolProperties)
{
    struct vkGetPhysicalDeviceToolPropertiesEXT_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pToolCount = pToolCount;
    params.pToolProperties = pToolProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceToolPropertiesEXT, &params);
    assert(!status && "vkGetPhysicalDeviceToolPropertiesEXT");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceVideoCapabilitiesKHR(VkPhysicalDevice physicalDevice, const VkVideoProfileInfoKHR *pVideoProfile, VkVideoCapabilitiesKHR *pCapabilities)
{
    struct vkGetPhysicalDeviceVideoCapabilitiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pVideoProfile = pVideoProfile;
    params.pCapabilities = pCapabilities;
    status = UNIX_CALL(vkGetPhysicalDeviceVideoCapabilitiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceVideoCapabilitiesKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceVideoEncodeQualityLevelInfoKHR *pQualityLevelInfo, VkVideoEncodeQualityLevelPropertiesKHR *pQualityLevelProperties)
{
    struct vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pQualityLevelInfo = pQualityLevelInfo;
    params.pQualityLevelProperties = pQualityLevelProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR");
    return params.result;
}

VkResult WINAPI vkGetPhysicalDeviceVideoFormatPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceVideoFormatInfoKHR *pVideoFormatInfo, uint32_t *pVideoFormatPropertyCount, VkVideoFormatPropertiesKHR *pVideoFormatProperties)
{
    struct vkGetPhysicalDeviceVideoFormatPropertiesKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.pVideoFormatInfo = pVideoFormatInfo;
    params.pVideoFormatPropertyCount = pVideoFormatPropertyCount;
    params.pVideoFormatProperties = pVideoFormatProperties;
    status = UNIX_CALL(vkGetPhysicalDeviceVideoFormatPropertiesKHR, &params);
    assert(!status && "vkGetPhysicalDeviceVideoFormatPropertiesKHR");
    return params.result;
}

VkBool32 WINAPI vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
{
    struct vkGetPhysicalDeviceWin32PresentationSupportKHR_params params;
    NTSTATUS status;
    params.physicalDevice = physicalDevice;
    params.queueFamilyIndex = queueFamilyIndex;
    status = UNIX_CALL(vkGetPhysicalDeviceWin32PresentationSupportKHR, &params);
    assert(!status && "vkGetPhysicalDeviceWin32PresentationSupportKHR");
    return params.result;
}

VkResult WINAPI vkGetPipelineBinaryDataKHR(VkDevice device, const VkPipelineBinaryDataInfoKHR *pInfo, VkPipelineBinaryKeyKHR *pPipelineBinaryKey, size_t *pPipelineBinaryDataSize, void *pPipelineBinaryData)
{
    struct vkGetPipelineBinaryDataKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pPipelineBinaryKey = pPipelineBinaryKey;
    params.pPipelineBinaryDataSize = pPipelineBinaryDataSize;
    params.pPipelineBinaryData = pPipelineBinaryData;
    status = UNIX_CALL(vkGetPipelineBinaryDataKHR, &params);
    assert(!status && "vkGetPipelineBinaryDataKHR");
    return params.result;
}

VkResult WINAPI vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData)
{
    struct vkGetPipelineCacheData_params params;
    NTSTATUS status;
    params.device = device;
    params.pipelineCache = pipelineCache;
    params.pDataSize = pDataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetPipelineCacheData, &params);
    assert(!status && "vkGetPipelineCacheData");
    return params.result;
}

VkResult WINAPI vkGetPipelineExecutableInternalRepresentationsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations)
{
    struct vkGetPipelineExecutableInternalRepresentationsKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pExecutableInfo = pExecutableInfo;
    params.pInternalRepresentationCount = pInternalRepresentationCount;
    params.pInternalRepresentations = pInternalRepresentations;
    status = UNIX_CALL(vkGetPipelineExecutableInternalRepresentationsKHR, &params);
    assert(!status && "vkGetPipelineExecutableInternalRepresentationsKHR");
    return params.result;
}

VkResult WINAPI vkGetPipelineExecutablePropertiesKHR(VkDevice device, const VkPipelineInfoKHR *pPipelineInfo, uint32_t *pExecutableCount, VkPipelineExecutablePropertiesKHR *pProperties)
{
    struct vkGetPipelineExecutablePropertiesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pPipelineInfo = pPipelineInfo;
    params.pExecutableCount = pExecutableCount;
    params.pProperties = pProperties;
    status = UNIX_CALL(vkGetPipelineExecutablePropertiesKHR, &params);
    assert(!status && "vkGetPipelineExecutablePropertiesKHR");
    return params.result;
}

VkResult WINAPI vkGetPipelineExecutableStatisticsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pStatisticCount, VkPipelineExecutableStatisticKHR *pStatistics)
{
    struct vkGetPipelineExecutableStatisticsKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pExecutableInfo = pExecutableInfo;
    params.pStatisticCount = pStatisticCount;
    params.pStatistics = pStatistics;
    status = UNIX_CALL(vkGetPipelineExecutableStatisticsKHR, &params);
    assert(!status && "vkGetPipelineExecutableStatisticsKHR");
    return params.result;
}

VkDeviceAddress WINAPI vkGetPipelineIndirectDeviceAddressNV(VkDevice device, const VkPipelineIndirectDeviceAddressInfoNV *pInfo)
{
    struct vkGetPipelineIndirectDeviceAddressNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetPipelineIndirectDeviceAddressNV, &params);
    assert(!status && "vkGetPipelineIndirectDeviceAddressNV");
    return params.result;
}

void WINAPI vkGetPipelineIndirectMemoryRequirementsNV(VkDevice device, const VkComputePipelineCreateInfo *pCreateInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetPipelineIndirectMemoryRequirementsNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetPipelineIndirectMemoryRequirementsNV, &params);
    assert(!status && "vkGetPipelineIndirectMemoryRequirementsNV");
}

VkResult WINAPI vkGetPipelineKeyKHR(VkDevice device, const VkPipelineCreateInfoKHR *pPipelineCreateInfo, VkPipelineBinaryKeyKHR *pPipelineKey)
{
    struct vkGetPipelineKeyKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pPipelineCreateInfo = pPipelineCreateInfo;
    params.pPipelineKey = pPipelineKey;
    status = UNIX_CALL(vkGetPipelineKeyKHR, &params);
    assert(!status && "vkGetPipelineKeyKHR");
    return params.result;
}

VkResult WINAPI vkGetPipelinePropertiesEXT(VkDevice device, const VkPipelineInfoEXT *pPipelineInfo, VkBaseOutStructure *pPipelineProperties)
{
    struct vkGetPipelinePropertiesEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pPipelineInfo = pPipelineInfo;
    params.pPipelineProperties = pPipelineProperties;
    status = UNIX_CALL(vkGetPipelinePropertiesEXT, &params);
    assert(!status && "vkGetPipelinePropertiesEXT");
    return params.result;
}

void WINAPI vkGetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t *pData)
{
    struct vkGetPrivateData_params params;
    NTSTATUS status;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.pData = pData;
    status = UNIX_CALL(vkGetPrivateData, &params);
    assert(!status && "vkGetPrivateData");
}

void WINAPI vkGetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t *pData)
{
    struct vkGetPrivateDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.pData = pData;
    status = UNIX_CALL(vkGetPrivateDataEXT, &params);
    assert(!status && "vkGetPrivateDataEXT");
}

VkResult WINAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    struct vkGetQueryPoolResults_params params;
    NTSTATUS status;
    params.device = device;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    params.dataSize = dataSize;
    params.pData = pData;
    params.stride = stride;
    params.flags = flags;
    status = UNIX_CALL(vkGetQueryPoolResults, &params);
    assert(!status && "vkGetQueryPoolResults");
    return params.result;
}

void WINAPI vkGetQueueCheckpointData2NV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointData2NV *pCheckpointData)
{
    struct vkGetQueueCheckpointData2NV_params params;
    NTSTATUS status;
    params.queue = queue;
    params.pCheckpointDataCount = pCheckpointDataCount;
    params.pCheckpointData = pCheckpointData;
    status = UNIX_CALL(vkGetQueueCheckpointData2NV, &params);
    assert(!status && "vkGetQueueCheckpointData2NV");
}

void WINAPI vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    struct vkGetQueueCheckpointDataNV_params params;
    NTSTATUS status;
    params.queue = queue;
    params.pCheckpointDataCount = pCheckpointDataCount;
    params.pCheckpointData = pCheckpointData;
    status = UNIX_CALL(vkGetQueueCheckpointDataNV, &params);
    assert(!status && "vkGetQueueCheckpointDataNV");
}

VkResult WINAPI vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    struct vkGetRayTracingCaptureReplayShaderGroupHandlesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pipeline = pipeline;
    params.firstGroup = firstGroup;
    params.groupCount = groupCount;
    params.dataSize = dataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetRayTracingCaptureReplayShaderGroupHandlesKHR, &params);
    assert(!status && "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
    return params.result;
}

VkResult WINAPI vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    struct vkGetRayTracingShaderGroupHandlesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pipeline = pipeline;
    params.firstGroup = firstGroup;
    params.groupCount = groupCount;
    params.dataSize = dataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetRayTracingShaderGroupHandlesKHR, &params);
    assert(!status && "vkGetRayTracingShaderGroupHandlesKHR");
    return params.result;
}

VkResult WINAPI vkGetRayTracingShaderGroupHandlesNV(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData)
{
    struct vkGetRayTracingShaderGroupHandlesNV_params params;
    NTSTATUS status;
    params.device = device;
    params.pipeline = pipeline;
    params.firstGroup = firstGroup;
    params.groupCount = groupCount;
    params.dataSize = dataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetRayTracingShaderGroupHandlesNV, &params);
    assert(!status && "vkGetRayTracingShaderGroupHandlesNV");
    return params.result;
}

VkDeviceSize WINAPI vkGetRayTracingShaderGroupStackSizeKHR(VkDevice device, VkPipeline pipeline, uint32_t group, VkShaderGroupShaderKHR groupShader)
{
    struct vkGetRayTracingShaderGroupStackSizeKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pipeline = pipeline;
    params.group = group;
    params.groupShader = groupShader;
    status = UNIX_CALL(vkGetRayTracingShaderGroupStackSizeKHR, &params);
    assert(!status && "vkGetRayTracingShaderGroupStackSizeKHR");
    return params.result;
}

void WINAPI vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity)
{
    struct vkGetRenderAreaGranularity_params params;
    NTSTATUS status;
    params.device = device;
    params.renderPass = renderPass;
    params.pGranularity = pGranularity;
    status = UNIX_CALL(vkGetRenderAreaGranularity, &params);
    assert(!status && "vkGetRenderAreaGranularity");
}

void WINAPI vkGetRenderingAreaGranularity(VkDevice device, const VkRenderingAreaInfo *pRenderingAreaInfo, VkExtent2D *pGranularity)
{
    struct vkGetRenderingAreaGranularity_params params;
    NTSTATUS status;
    params.device = device;
    params.pRenderingAreaInfo = pRenderingAreaInfo;
    params.pGranularity = pGranularity;
    status = UNIX_CALL(vkGetRenderingAreaGranularity, &params);
    assert(!status && "vkGetRenderingAreaGranularity");
}

void WINAPI vkGetRenderingAreaGranularityKHR(VkDevice device, const VkRenderingAreaInfo *pRenderingAreaInfo, VkExtent2D *pGranularity)
{
    struct vkGetRenderingAreaGranularityKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pRenderingAreaInfo = pRenderingAreaInfo;
    params.pGranularity = pGranularity;
    status = UNIX_CALL(vkGetRenderingAreaGranularityKHR, &params);
    assert(!status && "vkGetRenderingAreaGranularityKHR");
}

VkResult WINAPI vkGetSamplerOpaqueCaptureDescriptorDataEXT(VkDevice device, const VkSamplerCaptureDescriptorDataInfoEXT *pInfo, void *pData)
{
    struct vkGetSamplerOpaqueCaptureDescriptorDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pData = pData;
    status = UNIX_CALL(vkGetSamplerOpaqueCaptureDescriptorDataEXT, &params);
    assert(!status && "vkGetSamplerOpaqueCaptureDescriptorDataEXT");
    return params.result;
}

VkResult WINAPI vkGetSemaphoreCounterValue(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    struct vkGetSemaphoreCounterValue_params params;
    NTSTATUS status;
    params.device = device;
    params.semaphore = semaphore;
    params.pValue = pValue;
    status = UNIX_CALL(vkGetSemaphoreCounterValue, &params);
    assert(!status && "vkGetSemaphoreCounterValue");
    return params.result;
}

VkResult WINAPI vkGetSemaphoreCounterValueKHR(VkDevice device, VkSemaphore semaphore, uint64_t *pValue)
{
    struct vkGetSemaphoreCounterValueKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.semaphore = semaphore;
    params.pValue = pValue;
    status = UNIX_CALL(vkGetSemaphoreCounterValueKHR, &params);
    assert(!status && "vkGetSemaphoreCounterValueKHR");
    return params.result;
}

VkResult WINAPI vkGetShaderBinaryDataEXT(VkDevice device, VkShaderEXT shader, size_t *pDataSize, void *pData)
{
    struct vkGetShaderBinaryDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.shader = shader;
    params.pDataSize = pDataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetShaderBinaryDataEXT, &params);
    assert(!status && "vkGetShaderBinaryDataEXT");
    return params.result;
}

VkResult WINAPI vkGetShaderInfoAMD(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t *pInfoSize, void *pInfo)
{
    struct vkGetShaderInfoAMD_params params;
    NTSTATUS status;
    params.device = device;
    params.pipeline = pipeline;
    params.shaderStage = shaderStage;
    params.infoType = infoType;
    params.pInfoSize = pInfoSize;
    params.pInfo = pInfo;
    status = UNIX_CALL(vkGetShaderInfoAMD, &params);
    assert(!status && "vkGetShaderInfoAMD");
    return params.result;
}

void WINAPI vkGetShaderModuleCreateInfoIdentifierEXT(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, VkShaderModuleIdentifierEXT *pIdentifier)
{
    struct vkGetShaderModuleCreateInfoIdentifierEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pCreateInfo = pCreateInfo;
    params.pIdentifier = pIdentifier;
    status = UNIX_CALL(vkGetShaderModuleCreateInfoIdentifierEXT, &params);
    assert(!status && "vkGetShaderModuleCreateInfoIdentifierEXT");
}

void WINAPI vkGetShaderModuleIdentifierEXT(VkDevice device, VkShaderModule shaderModule, VkShaderModuleIdentifierEXT *pIdentifier)
{
    struct vkGetShaderModuleIdentifierEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.shaderModule = shaderModule;
    params.pIdentifier = pIdentifier;
    status = UNIX_CALL(vkGetShaderModuleIdentifierEXT, &params);
    assert(!status && "vkGetShaderModuleIdentifierEXT");
}

VkResult WINAPI vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages)
{
    struct vkGetSwapchainImagesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.pSwapchainImageCount = pSwapchainImageCount;
    params.pSwapchainImages = pSwapchainImages;
    status = UNIX_CALL(vkGetSwapchainImagesKHR, &params);
    assert(!status && "vkGetSwapchainImagesKHR");
    return params.result;
}

void WINAPI vkGetTensorMemoryRequirementsARM(VkDevice device, const VkTensorMemoryRequirementsInfoARM *pInfo, VkMemoryRequirements2 *pMemoryRequirements)
{
    struct vkGetTensorMemoryRequirementsARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetTensorMemoryRequirementsARM, &params);
    assert(!status && "vkGetTensorMemoryRequirementsARM");
}

VkResult WINAPI vkGetTensorOpaqueCaptureDescriptorDataARM(VkDevice device, const VkTensorCaptureDescriptorDataInfoARM *pInfo, void *pData)
{
    struct vkGetTensorOpaqueCaptureDescriptorDataARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pData = pData;
    status = UNIX_CALL(vkGetTensorOpaqueCaptureDescriptorDataARM, &params);
    assert(!status && "vkGetTensorOpaqueCaptureDescriptorDataARM");
    return params.result;
}

VkResult WINAPI vkGetTensorViewOpaqueCaptureDescriptorDataARM(VkDevice device, const VkTensorViewCaptureDescriptorDataInfoARM *pInfo, void *pData)
{
    struct vkGetTensorViewOpaqueCaptureDescriptorDataARM_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pData = pData;
    status = UNIX_CALL(vkGetTensorViewOpaqueCaptureDescriptorDataARM, &params);
    assert(!status && "vkGetTensorViewOpaqueCaptureDescriptorDataARM");
    return params.result;
}

VkResult WINAPI vkGetValidationCacheDataEXT(VkDevice device, VkValidationCacheEXT validationCache, size_t *pDataSize, void *pData)
{
    struct vkGetValidationCacheDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.validationCache = validationCache;
    params.pDataSize = pDataSize;
    params.pData = pData;
    status = UNIX_CALL(vkGetValidationCacheDataEXT, &params);
    assert(!status && "vkGetValidationCacheDataEXT");
    return params.result;
}

VkResult WINAPI vkGetVideoSessionMemoryRequirementsKHR(VkDevice device, VkVideoSessionKHR videoSession, uint32_t *pMemoryRequirementsCount, VkVideoSessionMemoryRequirementsKHR *pMemoryRequirements)
{
    struct vkGetVideoSessionMemoryRequirementsKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.videoSession = videoSession;
    params.pMemoryRequirementsCount = pMemoryRequirementsCount;
    params.pMemoryRequirements = pMemoryRequirements;
    status = UNIX_CALL(vkGetVideoSessionMemoryRequirementsKHR, &params);
    assert(!status && "vkGetVideoSessionMemoryRequirementsKHR");
    return params.result;
}

VkResult WINAPI vkInitializePerformanceApiINTEL(VkDevice device, const VkInitializePerformanceApiInfoINTEL *pInitializeInfo)
{
    struct vkInitializePerformanceApiINTEL_params params;
    NTSTATUS status;
    params.device = device;
    params.pInitializeInfo = pInitializeInfo;
    status = UNIX_CALL(vkInitializePerformanceApiINTEL, &params);
    assert(!status && "vkInitializePerformanceApiINTEL");
    return params.result;
}

VkResult WINAPI vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
{
    struct vkInvalidateMappedMemoryRanges_params params;
    NTSTATUS status;
    params.device = device;
    params.memoryRangeCount = memoryRangeCount;
    params.pMemoryRanges = pMemoryRanges;
    status = UNIX_CALL(vkInvalidateMappedMemoryRanges, &params);
    assert(!status && "vkInvalidateMappedMemoryRanges");
    return params.result;
}

VkResult WINAPI vkLatencySleepNV(VkDevice device, VkSwapchainKHR swapchain, const VkLatencySleepInfoNV *pSleepInfo)
{
    struct vkLatencySleepNV_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.pSleepInfo = pSleepInfo;
    status = UNIX_CALL(vkLatencySleepNV, &params);
    assert(!status && "vkLatencySleepNV");
    return params.result;
}

VkResult WINAPI vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
    struct vkMapMemory_params params;
    NTSTATUS status;
    params.device = device;
    params.memory = memory;
    params.offset = offset;
    params.size = size;
    params.flags = flags;
    params.ppData = ppData;
    status = UNIX_CALL(vkMapMemory, &params);
    assert(!status && "vkMapMemory");
    return params.result;
}

VkResult WINAPI vkMapMemory2(VkDevice device, const VkMemoryMapInfo *pMemoryMapInfo, void **ppData)
{
    struct vkMapMemory2_params params;
    NTSTATUS status;
    params.device = device;
    params.pMemoryMapInfo = pMemoryMapInfo;
    params.ppData = ppData;
    status = UNIX_CALL(vkMapMemory2, &params);
    assert(!status && "vkMapMemory2");
    return params.result;
}

VkResult WINAPI vkMapMemory2KHR(VkDevice device, const VkMemoryMapInfo *pMemoryMapInfo, void **ppData)
{
    struct vkMapMemory2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pMemoryMapInfo = pMemoryMapInfo;
    params.ppData = ppData;
    status = UNIX_CALL(vkMapMemory2KHR, &params);
    assert(!status && "vkMapMemory2KHR");
    return params.result;
}

VkResult WINAPI vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
    struct vkMergePipelineCaches_params params;
    NTSTATUS status;
    params.device = device;
    params.dstCache = dstCache;
    params.srcCacheCount = srcCacheCount;
    params.pSrcCaches = pSrcCaches;
    status = UNIX_CALL(vkMergePipelineCaches, &params);
    assert(!status && "vkMergePipelineCaches");
    return params.result;
}

VkResult WINAPI vkMergeValidationCachesEXT(VkDevice device, VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT *pSrcCaches)
{
    struct vkMergeValidationCachesEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.dstCache = dstCache;
    params.srcCacheCount = srcCacheCount;
    params.pSrcCaches = pSrcCaches;
    status = UNIX_CALL(vkMergeValidationCachesEXT, &params);
    assert(!status && "vkMergeValidationCachesEXT");
    return params.result;
}

void WINAPI vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkQueueBeginDebugUtilsLabelEXT_params params;
    NTSTATUS status;
    params.queue = queue;
    params.pLabelInfo = pLabelInfo;
    status = UNIX_CALL(vkQueueBeginDebugUtilsLabelEXT, &params);
    assert(!status && "vkQueueBeginDebugUtilsLabelEXT");
}

VkResult WINAPI vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence)
{
    struct vkQueueBindSparse_params params;
    NTSTATUS status;
    params.queue = queue;
    params.bindInfoCount = bindInfoCount;
    params.pBindInfo = pBindInfo;
    params.fence = fence;
    status = UNIX_CALL(vkQueueBindSparse, &params);
    assert(!status && "vkQueueBindSparse");
    return params.result;
}

void WINAPI vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
{
    struct vkQueueEndDebugUtilsLabelEXT_params params;
    NTSTATUS status;
    params.queue = queue;
    status = UNIX_CALL(vkQueueEndDebugUtilsLabelEXT, &params);
    assert(!status && "vkQueueEndDebugUtilsLabelEXT");
}

void WINAPI vkQueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo)
{
    struct vkQueueInsertDebugUtilsLabelEXT_params params;
    NTSTATUS status;
    params.queue = queue;
    params.pLabelInfo = pLabelInfo;
    status = UNIX_CALL(vkQueueInsertDebugUtilsLabelEXT, &params);
    assert(!status && "vkQueueInsertDebugUtilsLabelEXT");
}

void WINAPI vkQueueNotifyOutOfBandNV(VkQueue queue, const VkOutOfBandQueueTypeInfoNV *pQueueTypeInfo)
{
    struct vkQueueNotifyOutOfBandNV_params params;
    NTSTATUS status;
    params.queue = queue;
    params.pQueueTypeInfo = pQueueTypeInfo;
    status = UNIX_CALL(vkQueueNotifyOutOfBandNV, &params);
    assert(!status && "vkQueueNotifyOutOfBandNV");
}

VkResult WINAPI vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    struct vkQueuePresentKHR_params params;
    NTSTATUS status;
    params.queue = queue;
    params.pPresentInfo = pPresentInfo;
    status = UNIX_CALL(vkQueuePresentKHR, &params);
    assert(!status && "vkQueuePresentKHR");
    return params.result;
}

VkResult WINAPI vkQueueSetPerformanceConfigurationINTEL(VkQueue queue, VkPerformanceConfigurationINTEL configuration)
{
    struct vkQueueSetPerformanceConfigurationINTEL_params params;
    NTSTATUS status;
    params.queue = queue;
    params.configuration = configuration;
    status = UNIX_CALL(vkQueueSetPerformanceConfigurationINTEL, &params);
    assert(!status && "vkQueueSetPerformanceConfigurationINTEL");
    return params.result;
}

VkResult WINAPI vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
{
    struct vkQueueSubmit_params params;
    NTSTATUS status;
    params.queue = queue;
    params.submitCount = submitCount;
    params.pSubmits = pSubmits;
    params.fence = fence;
    status = UNIX_CALL(vkQueueSubmit, &params);
    assert(!status && "vkQueueSubmit");
    return params.result;
}

VkResult WINAPI vkQueueSubmit2(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence)
{
    struct vkQueueSubmit2_params params;
    NTSTATUS status;
    params.queue = queue;
    params.submitCount = submitCount;
    params.pSubmits = pSubmits;
    params.fence = fence;
    status = UNIX_CALL(vkQueueSubmit2, &params);
    assert(!status && "vkQueueSubmit2");
    return params.result;
}

VkResult WINAPI vkQueueSubmit2KHR(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence)
{
    struct vkQueueSubmit2KHR_params params;
    NTSTATUS status;
    params.queue = queue;
    params.submitCount = submitCount;
    params.pSubmits = pSubmits;
    params.fence = fence;
    status = UNIX_CALL(vkQueueSubmit2KHR, &params);
    assert(!status && "vkQueueSubmit2KHR");
    return params.result;
}

VkResult WINAPI vkQueueWaitIdle(VkQueue queue)
{
    struct vkQueueWaitIdle_params params;
    NTSTATUS status;
    params.queue = queue;
    status = UNIX_CALL(vkQueueWaitIdle, &params);
    assert(!status && "vkQueueWaitIdle");
    return params.result;
}

VkResult WINAPI vkReleaseCapturedPipelineDataKHR(VkDevice device, const VkReleaseCapturedPipelineDataInfoKHR *pInfo, const VkAllocationCallbacks *pAllocator)
{
    struct vkReleaseCapturedPipelineDataKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pInfo = pInfo;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkReleaseCapturedPipelineDataKHR, &params);
    assert(!status && "vkReleaseCapturedPipelineDataKHR");
    return params.result;
}

VkResult WINAPI vkReleasePerformanceConfigurationINTEL(VkDevice device, VkPerformanceConfigurationINTEL configuration)
{
    struct vkReleasePerformanceConfigurationINTEL_params params;
    NTSTATUS status;
    params.device = device;
    params.configuration = configuration;
    status = UNIX_CALL(vkReleasePerformanceConfigurationINTEL, &params);
    assert(!status && "vkReleasePerformanceConfigurationINTEL");
    return params.result;
}

void WINAPI vkReleaseProfilingLockKHR(VkDevice device)
{
    struct vkReleaseProfilingLockKHR_params params;
    NTSTATUS status;
    params.device = device;
    status = UNIX_CALL(vkReleaseProfilingLockKHR, &params);
    assert(!status && "vkReleaseProfilingLockKHR");
}

VkResult WINAPI vkReleaseSwapchainImagesEXT(VkDevice device, const VkReleaseSwapchainImagesInfoKHR *pReleaseInfo)
{
    struct vkReleaseSwapchainImagesEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pReleaseInfo = pReleaseInfo;
    status = UNIX_CALL(vkReleaseSwapchainImagesEXT, &params);
    assert(!status && "vkReleaseSwapchainImagesEXT");
    return params.result;
}

VkResult WINAPI vkReleaseSwapchainImagesKHR(VkDevice device, const VkReleaseSwapchainImagesInfoKHR *pReleaseInfo)
{
    struct vkReleaseSwapchainImagesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pReleaseInfo = pReleaseInfo;
    status = UNIX_CALL(vkReleaseSwapchainImagesKHR, &params);
    assert(!status && "vkReleaseSwapchainImagesKHR");
    return params.result;
}

VkResult WINAPI vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    struct vkResetCommandBuffer_params params;
    NTSTATUS status;
    params.commandBuffer = commandBuffer;
    params.flags = flags;
    status = UNIX_CALL(vkResetCommandBuffer, &params);
    assert(!status && "vkResetCommandBuffer");
    return params.result;
}

VkResult WINAPI vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    struct vkResetCommandPool_params params;
    NTSTATUS status;
    params.device = device;
    params.commandPool = commandPool;
    params.flags = flags;
    status = UNIX_CALL(vkResetCommandPool, &params);
    assert(!status && "vkResetCommandPool");
    return params.result;
}

VkResult WINAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    struct vkResetDescriptorPool_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorPool = descriptorPool;
    params.flags = flags;
    status = UNIX_CALL(vkResetDescriptorPool, &params);
    assert(!status && "vkResetDescriptorPool");
    return params.result;
}

VkResult WINAPI vkResetEvent(VkDevice device, VkEvent event)
{
    struct vkResetEvent_params params;
    NTSTATUS status;
    params.device = device;
    params.event = event;
    status = UNIX_CALL(vkResetEvent, &params);
    assert(!status && "vkResetEvent");
    return params.result;
}

VkResult WINAPI vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
{
    struct vkResetFences_params params;
    NTSTATUS status;
    params.device = device;
    params.fenceCount = fenceCount;
    params.pFences = pFences;
    status = UNIX_CALL(vkResetFences, &params);
    assert(!status && "vkResetFences");
    return params.result;
}

void WINAPI vkResetQueryPool(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    struct vkResetQueryPool_params params;
    NTSTATUS status;
    params.device = device;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    status = UNIX_CALL(vkResetQueryPool, &params);
    assert(!status && "vkResetQueryPool");
}

void WINAPI vkResetQueryPoolEXT(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    struct vkResetQueryPoolEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.queryPool = queryPool;
    params.firstQuery = firstQuery;
    params.queryCount = queryCount;
    status = UNIX_CALL(vkResetQueryPoolEXT, &params);
    assert(!status && "vkResetQueryPoolEXT");
}

VkResult WINAPI vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
{
    struct vkSetDebugUtilsObjectNameEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pNameInfo = pNameInfo;
    status = UNIX_CALL(vkSetDebugUtilsObjectNameEXT, &params);
    assert(!status && "vkSetDebugUtilsObjectNameEXT");
    return params.result;
}

VkResult WINAPI vkSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo)
{
    struct vkSetDebugUtilsObjectTagEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.pTagInfo = pTagInfo;
    status = UNIX_CALL(vkSetDebugUtilsObjectTagEXT, &params);
    assert(!status && "vkSetDebugUtilsObjectTagEXT");
    return params.result;
}

void WINAPI vkSetDeviceMemoryPriorityEXT(VkDevice device, VkDeviceMemory memory, float priority)
{
    struct vkSetDeviceMemoryPriorityEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.memory = memory;
    params.priority = priority;
    status = UNIX_CALL(vkSetDeviceMemoryPriorityEXT, &params);
    assert(!status && "vkSetDeviceMemoryPriorityEXT");
}

VkResult WINAPI vkSetEvent(VkDevice device, VkEvent event)
{
    struct vkSetEvent_params params;
    NTSTATUS status;
    params.device = device;
    params.event = event;
    status = UNIX_CALL(vkSetEvent, &params);
    assert(!status && "vkSetEvent");
    return params.result;
}

void WINAPI vkSetHdrMetadataEXT(VkDevice device, uint32_t swapchainCount, const VkSwapchainKHR *pSwapchains, const VkHdrMetadataEXT *pMetadata)
{
    struct vkSetHdrMetadataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchainCount = swapchainCount;
    params.pSwapchains = pSwapchains;
    params.pMetadata = pMetadata;
    status = UNIX_CALL(vkSetHdrMetadataEXT, &params);
    assert(!status && "vkSetHdrMetadataEXT");
}

void WINAPI vkSetLatencyMarkerNV(VkDevice device, VkSwapchainKHR swapchain, const VkSetLatencyMarkerInfoNV *pLatencyMarkerInfo)
{
    struct vkSetLatencyMarkerNV_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.pLatencyMarkerInfo = pLatencyMarkerInfo;
    status = UNIX_CALL(vkSetLatencyMarkerNV, &params);
    assert(!status && "vkSetLatencyMarkerNV");
}

VkResult WINAPI vkSetLatencySleepModeNV(VkDevice device, VkSwapchainKHR swapchain, const VkLatencySleepModeInfoNV *pSleepModeInfo)
{
    struct vkSetLatencySleepModeNV_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.pSleepModeInfo = pSleepModeInfo;
    status = UNIX_CALL(vkSetLatencySleepModeNV, &params);
    assert(!status && "vkSetLatencySleepModeNV");
    return params.result;
}

VkResult WINAPI vkSetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t data)
{
    struct vkSetPrivateData_params params;
    NTSTATUS status;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.data = data;
    status = UNIX_CALL(vkSetPrivateData, &params);
    assert(!status && "vkSetPrivateData");
    return params.result;
}

VkResult WINAPI vkSetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t data)
{
    struct vkSetPrivateDataEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.objectType = objectType;
    params.objectHandle = objectHandle;
    params.privateDataSlot = privateDataSlot;
    params.data = data;
    status = UNIX_CALL(vkSetPrivateDataEXT, &params);
    assert(!status && "vkSetPrivateDataEXT");
    return params.result;
}

VkResult WINAPI vkSignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    struct vkSignalSemaphore_params params;
    NTSTATUS status;
    params.device = device;
    params.pSignalInfo = pSignalInfo;
    status = UNIX_CALL(vkSignalSemaphore, &params);
    assert(!status && "vkSignalSemaphore");
    return params.result;
}

VkResult WINAPI vkSignalSemaphoreKHR(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
{
    struct vkSignalSemaphoreKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pSignalInfo = pSignalInfo;
    status = UNIX_CALL(vkSignalSemaphoreKHR, &params);
    assert(!status && "vkSignalSemaphoreKHR");
    return params.result;
}

void WINAPI vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
{
    struct vkSubmitDebugUtilsMessageEXT_params params;
    NTSTATUS status;
    params.instance = instance;
    params.messageSeverity = messageSeverity;
    params.messageTypes = messageTypes;
    params.pCallbackData = pCallbackData;
    status = UNIX_CALL(vkSubmitDebugUtilsMessageEXT, &params);
    assert(!status && "vkSubmitDebugUtilsMessageEXT");
}

VkResult WINAPI vkTransitionImageLayout(VkDevice device, uint32_t transitionCount, const VkHostImageLayoutTransitionInfo *pTransitions)
{
    struct vkTransitionImageLayout_params params;
    NTSTATUS status;
    params.device = device;
    params.transitionCount = transitionCount;
    params.pTransitions = pTransitions;
    status = UNIX_CALL(vkTransitionImageLayout, &params);
    assert(!status && "vkTransitionImageLayout");
    return params.result;
}

VkResult WINAPI vkTransitionImageLayoutEXT(VkDevice device, uint32_t transitionCount, const VkHostImageLayoutTransitionInfo *pTransitions)
{
    struct vkTransitionImageLayoutEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.transitionCount = transitionCount;
    params.pTransitions = pTransitions;
    status = UNIX_CALL(vkTransitionImageLayoutEXT, &params);
    assert(!status && "vkTransitionImageLayoutEXT");
    return params.result;
}

void WINAPI vkTrimCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    struct vkTrimCommandPool_params params;
    NTSTATUS status;
    params.device = device;
    params.commandPool = commandPool;
    params.flags = flags;
    status = UNIX_CALL(vkTrimCommandPool, &params);
    assert(!status && "vkTrimCommandPool");
}

void WINAPI vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags)
{
    struct vkTrimCommandPoolKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.commandPool = commandPool;
    params.flags = flags;
    status = UNIX_CALL(vkTrimCommandPoolKHR, &params);
    assert(!status && "vkTrimCommandPoolKHR");
}

void WINAPI vkUninitializePerformanceApiINTEL(VkDevice device)
{
    struct vkUninitializePerformanceApiINTEL_params params;
    NTSTATUS status;
    params.device = device;
    status = UNIX_CALL(vkUninitializePerformanceApiINTEL, &params);
    assert(!status && "vkUninitializePerformanceApiINTEL");
}

void WINAPI vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    struct vkUnmapMemory_params params;
    NTSTATUS status;
    params.device = device;
    params.memory = memory;
    status = UNIX_CALL(vkUnmapMemory, &params);
    assert(!status && "vkUnmapMemory");
}

VkResult WINAPI vkUnmapMemory2(VkDevice device, const VkMemoryUnmapInfo *pMemoryUnmapInfo)
{
    struct vkUnmapMemory2_params params;
    NTSTATUS status;
    params.device = device;
    params.pMemoryUnmapInfo = pMemoryUnmapInfo;
    status = UNIX_CALL(vkUnmapMemory2, &params);
    assert(!status && "vkUnmapMemory2");
    return params.result;
}

VkResult WINAPI vkUnmapMemory2KHR(VkDevice device, const VkMemoryUnmapInfo *pMemoryUnmapInfo)
{
    struct vkUnmapMemory2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pMemoryUnmapInfo = pMemoryUnmapInfo;
    status = UNIX_CALL(vkUnmapMemory2KHR, &params);
    assert(!status && "vkUnmapMemory2KHR");
    return params.result;
}

void WINAPI vkUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    struct vkUpdateDescriptorSetWithTemplate_params params;
    params.device = device;
    params.descriptorSet = descriptorSet;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pData = pData;
    UNIX_CALL(vkUpdateDescriptorSetWithTemplate, &params);
}

void WINAPI vkUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData)
{
    struct vkUpdateDescriptorSetWithTemplateKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.descriptorSet = descriptorSet;
    params.descriptorUpdateTemplate = descriptorUpdateTemplate;
    params.pData = pData;
    status = UNIX_CALL(vkUpdateDescriptorSetWithTemplateKHR, &params);
    assert(!status && "vkUpdateDescriptorSetWithTemplateKHR");
}

void WINAPI vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies)
{
    struct vkUpdateDescriptorSets_params params;
    params.device = device;
    params.descriptorWriteCount = descriptorWriteCount;
    params.pDescriptorWrites = pDescriptorWrites;
    params.descriptorCopyCount = descriptorCopyCount;
    params.pDescriptorCopies = pDescriptorCopies;
    UNIX_CALL(vkUpdateDescriptorSets, &params);
}

void WINAPI vkUpdateIndirectExecutionSetPipelineEXT(VkDevice device, VkIndirectExecutionSetEXT indirectExecutionSet, uint32_t executionSetWriteCount, const VkWriteIndirectExecutionSetPipelineEXT *pExecutionSetWrites)
{
    struct vkUpdateIndirectExecutionSetPipelineEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.indirectExecutionSet = indirectExecutionSet;
    params.executionSetWriteCount = executionSetWriteCount;
    params.pExecutionSetWrites = pExecutionSetWrites;
    status = UNIX_CALL(vkUpdateIndirectExecutionSetPipelineEXT, &params);
    assert(!status && "vkUpdateIndirectExecutionSetPipelineEXT");
}

void WINAPI vkUpdateIndirectExecutionSetShaderEXT(VkDevice device, VkIndirectExecutionSetEXT indirectExecutionSet, uint32_t executionSetWriteCount, const VkWriteIndirectExecutionSetShaderEXT *pExecutionSetWrites)
{
    struct vkUpdateIndirectExecutionSetShaderEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.indirectExecutionSet = indirectExecutionSet;
    params.executionSetWriteCount = executionSetWriteCount;
    params.pExecutionSetWrites = pExecutionSetWrites;
    status = UNIX_CALL(vkUpdateIndirectExecutionSetShaderEXT, &params);
    assert(!status && "vkUpdateIndirectExecutionSetShaderEXT");
}

VkResult WINAPI vkUpdateVideoSessionParametersKHR(VkDevice device, VkVideoSessionParametersKHR videoSessionParameters, const VkVideoSessionParametersUpdateInfoKHR *pUpdateInfo)
{
    struct vkUpdateVideoSessionParametersKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.videoSessionParameters = videoSessionParameters;
    params.pUpdateInfo = pUpdateInfo;
    status = UNIX_CALL(vkUpdateVideoSessionParametersKHR, &params);
    assert(!status && "vkUpdateVideoSessionParametersKHR");
    return params.result;
}

VkResult WINAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout)
{
    struct vkWaitForFences_params params;
    NTSTATUS status;
    params.device = device;
    params.fenceCount = fenceCount;
    params.pFences = pFences;
    params.waitAll = waitAll;
    params.timeout = timeout;
    status = UNIX_CALL(vkWaitForFences, &params);
    assert(!status && "vkWaitForFences");
    return params.result;
}

VkResult WINAPI vkWaitForPresent2KHR(VkDevice device, VkSwapchainKHR swapchain, const VkPresentWait2InfoKHR *pPresentWait2Info)
{
    struct vkWaitForPresent2KHR_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.pPresentWait2Info = pPresentWait2Info;
    status = UNIX_CALL(vkWaitForPresent2KHR, &params);
    assert(!status && "vkWaitForPresent2KHR");
    return params.result;
}

VkResult WINAPI vkWaitForPresentKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t presentId, uint64_t timeout)
{
    struct vkWaitForPresentKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.swapchain = swapchain;
    params.presentId = presentId;
    params.timeout = timeout;
    status = UNIX_CALL(vkWaitForPresentKHR, &params);
    assert(!status && "vkWaitForPresentKHR");
    return params.result;
}

VkResult WINAPI vkWaitSemaphores(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    struct vkWaitSemaphores_params params;
    NTSTATUS status;
    params.device = device;
    params.pWaitInfo = pWaitInfo;
    params.timeout = timeout;
    status = UNIX_CALL(vkWaitSemaphores, &params);
    assert(!status && "vkWaitSemaphores");
    return params.result;
}

VkResult WINAPI vkWaitSemaphoresKHR(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout)
{
    struct vkWaitSemaphoresKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.pWaitInfo = pWaitInfo;
    params.timeout = timeout;
    status = UNIX_CALL(vkWaitSemaphoresKHR, &params);
    assert(!status && "vkWaitSemaphoresKHR");
    return params.result;
}

VkResult WINAPI vkWriteAccelerationStructuresPropertiesKHR(VkDevice device, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, size_t dataSize, void *pData, size_t stride)
{
    struct vkWriteAccelerationStructuresPropertiesKHR_params params;
    NTSTATUS status;
    params.device = device;
    params.accelerationStructureCount = accelerationStructureCount;
    params.pAccelerationStructures = pAccelerationStructures;
    params.queryType = queryType;
    params.dataSize = dataSize;
    params.pData = pData;
    params.stride = stride;
    status = UNIX_CALL(vkWriteAccelerationStructuresPropertiesKHR, &params);
    assert(!status && "vkWriteAccelerationStructuresPropertiesKHR");
    return params.result;
}

VkResult WINAPI vkWriteMicromapsPropertiesEXT(VkDevice device, uint32_t micromapCount, const VkMicromapEXT *pMicromaps, VkQueryType queryType, size_t dataSize, void *pData, size_t stride)
{
    struct vkWriteMicromapsPropertiesEXT_params params;
    NTSTATUS status;
    params.device = device;
    params.micromapCount = micromapCount;
    params.pMicromaps = pMicromaps;
    params.queryType = queryType;
    params.dataSize = dataSize;
    params.pData = pData;
    params.stride = stride;
    status = UNIX_CALL(vkWriteMicromapsPropertiesEXT, &params);
    assert(!status && "vkWriteMicromapsPropertiesEXT");
    return params.result;
}

static const struct vulkan_func vk_device_dispatch_table[] =
{
    {"vkAcquireNextImage2KHR", vkAcquireNextImage2KHR},
    {"vkAcquireNextImageKHR", vkAcquireNextImageKHR},
    {"vkAcquirePerformanceConfigurationINTEL", vkAcquirePerformanceConfigurationINTEL},
    {"vkAcquireProfilingLockKHR", vkAcquireProfilingLockKHR},
    {"vkAllocateCommandBuffers", vkAllocateCommandBuffers},
    {"vkAllocateDescriptorSets", vkAllocateDescriptorSets},
    {"vkAllocateMemory", vkAllocateMemory},
    {"vkAntiLagUpdateAMD", vkAntiLagUpdateAMD},
    {"vkBeginCommandBuffer", vkBeginCommandBuffer},
    {"vkBindAccelerationStructureMemoryNV", vkBindAccelerationStructureMemoryNV},
    {"vkBindBufferMemory", vkBindBufferMemory},
    {"vkBindBufferMemory2", vkBindBufferMemory2},
    {"vkBindBufferMemory2KHR", vkBindBufferMemory2KHR},
    {"vkBindDataGraphPipelineSessionMemoryARM", vkBindDataGraphPipelineSessionMemoryARM},
    {"vkBindImageMemory", vkBindImageMemory},
    {"vkBindImageMemory2", vkBindImageMemory2},
    {"vkBindImageMemory2KHR", vkBindImageMemory2KHR},
    {"vkBindOpticalFlowSessionImageNV", vkBindOpticalFlowSessionImageNV},
    {"vkBindTensorMemoryARM", vkBindTensorMemoryARM},
    {"vkBindVideoSessionMemoryKHR", vkBindVideoSessionMemoryKHR},
    {"vkBuildAccelerationStructuresKHR", vkBuildAccelerationStructuresKHR},
    {"vkBuildMicromapsEXT", vkBuildMicromapsEXT},
    {"vkCmdBeginConditionalRenderingEXT", vkCmdBeginConditionalRenderingEXT},
    {"vkCmdBeginDebugUtilsLabelEXT", vkCmdBeginDebugUtilsLabelEXT},
    {"vkCmdBeginPerTileExecutionQCOM", vkCmdBeginPerTileExecutionQCOM},
    {"vkCmdBeginQuery", vkCmdBeginQuery},
    {"vkCmdBeginQueryIndexedEXT", vkCmdBeginQueryIndexedEXT},
    {"vkCmdBeginRenderPass", vkCmdBeginRenderPass},
    {"vkCmdBeginRenderPass2", vkCmdBeginRenderPass2},
    {"vkCmdBeginRenderPass2KHR", vkCmdBeginRenderPass2KHR},
    {"vkCmdBeginRendering", vkCmdBeginRendering},
    {"vkCmdBeginRenderingKHR", vkCmdBeginRenderingKHR},
    {"vkCmdBeginTransformFeedbackEXT", vkCmdBeginTransformFeedbackEXT},
    {"vkCmdBeginVideoCodingKHR", vkCmdBeginVideoCodingKHR},
    {"vkCmdBindDescriptorBufferEmbeddedSamplers2EXT", vkCmdBindDescriptorBufferEmbeddedSamplers2EXT},
    {"vkCmdBindDescriptorBufferEmbeddedSamplersEXT", vkCmdBindDescriptorBufferEmbeddedSamplersEXT},
    {"vkCmdBindDescriptorBuffersEXT", vkCmdBindDescriptorBuffersEXT},
    {"vkCmdBindDescriptorSets", vkCmdBindDescriptorSets},
    {"vkCmdBindDescriptorSets2", vkCmdBindDescriptorSets2},
    {"vkCmdBindDescriptorSets2KHR", vkCmdBindDescriptorSets2KHR},
    {"vkCmdBindIndexBuffer", vkCmdBindIndexBuffer},
    {"vkCmdBindIndexBuffer2", vkCmdBindIndexBuffer2},
    {"vkCmdBindIndexBuffer2KHR", vkCmdBindIndexBuffer2KHR},
    {"vkCmdBindInvocationMaskHUAWEI", vkCmdBindInvocationMaskHUAWEI},
    {"vkCmdBindPipeline", vkCmdBindPipeline},
    {"vkCmdBindPipelineShaderGroupNV", vkCmdBindPipelineShaderGroupNV},
    {"vkCmdBindShadersEXT", vkCmdBindShadersEXT},
    {"vkCmdBindShadingRateImageNV", vkCmdBindShadingRateImageNV},
    {"vkCmdBindTileMemoryQCOM", vkCmdBindTileMemoryQCOM},
    {"vkCmdBindTransformFeedbackBuffersEXT", vkCmdBindTransformFeedbackBuffersEXT},
    {"vkCmdBindVertexBuffers", vkCmdBindVertexBuffers},
    {"vkCmdBindVertexBuffers2", vkCmdBindVertexBuffers2},
    {"vkCmdBindVertexBuffers2EXT", vkCmdBindVertexBuffers2EXT},
    {"vkCmdBlitImage", vkCmdBlitImage},
    {"vkCmdBlitImage2", vkCmdBlitImage2},
    {"vkCmdBlitImage2KHR", vkCmdBlitImage2KHR},
    {"vkCmdBuildAccelerationStructureNV", vkCmdBuildAccelerationStructureNV},
    {"vkCmdBuildAccelerationStructuresIndirectKHR", vkCmdBuildAccelerationStructuresIndirectKHR},
    {"vkCmdBuildAccelerationStructuresKHR", vkCmdBuildAccelerationStructuresKHR},
    {"vkCmdBuildClusterAccelerationStructureIndirectNV", vkCmdBuildClusterAccelerationStructureIndirectNV},
    {"vkCmdBuildMicromapsEXT", vkCmdBuildMicromapsEXT},
    {"vkCmdBuildPartitionedAccelerationStructuresNV", vkCmdBuildPartitionedAccelerationStructuresNV},
    {"vkCmdClearAttachments", vkCmdClearAttachments},
    {"vkCmdClearColorImage", vkCmdClearColorImage},
    {"vkCmdClearDepthStencilImage", vkCmdClearDepthStencilImage},
    {"vkCmdControlVideoCodingKHR", vkCmdControlVideoCodingKHR},
    {"vkCmdConvertCooperativeVectorMatrixNV", vkCmdConvertCooperativeVectorMatrixNV},
    {"vkCmdCopyAccelerationStructureKHR", vkCmdCopyAccelerationStructureKHR},
    {"vkCmdCopyAccelerationStructureNV", vkCmdCopyAccelerationStructureNV},
    {"vkCmdCopyAccelerationStructureToMemoryKHR", vkCmdCopyAccelerationStructureToMemoryKHR},
    {"vkCmdCopyBuffer", vkCmdCopyBuffer},
    {"vkCmdCopyBuffer2", vkCmdCopyBuffer2},
    {"vkCmdCopyBuffer2KHR", vkCmdCopyBuffer2KHR},
    {"vkCmdCopyBufferToImage", vkCmdCopyBufferToImage},
    {"vkCmdCopyBufferToImage2", vkCmdCopyBufferToImage2},
    {"vkCmdCopyBufferToImage2KHR", vkCmdCopyBufferToImage2KHR},
    {"vkCmdCopyImage", vkCmdCopyImage},
    {"vkCmdCopyImage2", vkCmdCopyImage2},
    {"vkCmdCopyImage2KHR", vkCmdCopyImage2KHR},
    {"vkCmdCopyImageToBuffer", vkCmdCopyImageToBuffer},
    {"vkCmdCopyImageToBuffer2", vkCmdCopyImageToBuffer2},
    {"vkCmdCopyImageToBuffer2KHR", vkCmdCopyImageToBuffer2KHR},
    {"vkCmdCopyMemoryIndirectNV", vkCmdCopyMemoryIndirectNV},
    {"vkCmdCopyMemoryToAccelerationStructureKHR", vkCmdCopyMemoryToAccelerationStructureKHR},
    {"vkCmdCopyMemoryToImageIndirectNV", vkCmdCopyMemoryToImageIndirectNV},
    {"vkCmdCopyMemoryToMicromapEXT", vkCmdCopyMemoryToMicromapEXT},
    {"vkCmdCopyMicromapEXT", vkCmdCopyMicromapEXT},
    {"vkCmdCopyMicromapToMemoryEXT", vkCmdCopyMicromapToMemoryEXT},
    {"vkCmdCopyQueryPoolResults", vkCmdCopyQueryPoolResults},
    {"vkCmdCopyTensorARM", vkCmdCopyTensorARM},
    {"vkCmdCuLaunchKernelNVX", vkCmdCuLaunchKernelNVX},
    {"vkCmdDebugMarkerBeginEXT", vkCmdDebugMarkerBeginEXT},
    {"vkCmdDebugMarkerEndEXT", vkCmdDebugMarkerEndEXT},
    {"vkCmdDebugMarkerInsertEXT", vkCmdDebugMarkerInsertEXT},
    {"vkCmdDecodeVideoKHR", vkCmdDecodeVideoKHR},
    {"vkCmdDecompressMemoryIndirectCountNV", vkCmdDecompressMemoryIndirectCountNV},
    {"vkCmdDecompressMemoryNV", vkCmdDecompressMemoryNV},
    {"vkCmdDispatch", vkCmdDispatch},
    {"vkCmdDispatchBase", vkCmdDispatchBase},
    {"vkCmdDispatchBaseKHR", vkCmdDispatchBaseKHR},
    {"vkCmdDispatchDataGraphARM", vkCmdDispatchDataGraphARM},
    {"vkCmdDispatchIndirect", vkCmdDispatchIndirect},
    {"vkCmdDispatchTileQCOM", vkCmdDispatchTileQCOM},
    {"vkCmdDraw", vkCmdDraw},
    {"vkCmdDrawClusterHUAWEI", vkCmdDrawClusterHUAWEI},
    {"vkCmdDrawClusterIndirectHUAWEI", vkCmdDrawClusterIndirectHUAWEI},
    {"vkCmdDrawIndexed", vkCmdDrawIndexed},
    {"vkCmdDrawIndexedIndirect", vkCmdDrawIndexedIndirect},
    {"vkCmdDrawIndexedIndirectCount", vkCmdDrawIndexedIndirectCount},
    {"vkCmdDrawIndexedIndirectCountAMD", vkCmdDrawIndexedIndirectCountAMD},
    {"vkCmdDrawIndexedIndirectCountKHR", vkCmdDrawIndexedIndirectCountKHR},
    {"vkCmdDrawIndirect", vkCmdDrawIndirect},
    {"vkCmdDrawIndirectByteCountEXT", vkCmdDrawIndirectByteCountEXT},
    {"vkCmdDrawIndirectCount", vkCmdDrawIndirectCount},
    {"vkCmdDrawIndirectCountAMD", vkCmdDrawIndirectCountAMD},
    {"vkCmdDrawIndirectCountKHR", vkCmdDrawIndirectCountKHR},
    {"vkCmdDrawMeshTasksEXT", vkCmdDrawMeshTasksEXT},
    {"vkCmdDrawMeshTasksIndirectCountEXT", vkCmdDrawMeshTasksIndirectCountEXT},
    {"vkCmdDrawMeshTasksIndirectCountNV", vkCmdDrawMeshTasksIndirectCountNV},
    {"vkCmdDrawMeshTasksIndirectEXT", vkCmdDrawMeshTasksIndirectEXT},
    {"vkCmdDrawMeshTasksIndirectNV", vkCmdDrawMeshTasksIndirectNV},
    {"vkCmdDrawMeshTasksNV", vkCmdDrawMeshTasksNV},
    {"vkCmdDrawMultiEXT", vkCmdDrawMultiEXT},
    {"vkCmdDrawMultiIndexedEXT", vkCmdDrawMultiIndexedEXT},
    {"vkCmdEncodeVideoKHR", vkCmdEncodeVideoKHR},
    {"vkCmdEndConditionalRenderingEXT", vkCmdEndConditionalRenderingEXT},
    {"vkCmdEndDebugUtilsLabelEXT", vkCmdEndDebugUtilsLabelEXT},
    {"vkCmdEndPerTileExecutionQCOM", vkCmdEndPerTileExecutionQCOM},
    {"vkCmdEndQuery", vkCmdEndQuery},
    {"vkCmdEndQueryIndexedEXT", vkCmdEndQueryIndexedEXT},
    {"vkCmdEndRenderPass", vkCmdEndRenderPass},
    {"vkCmdEndRenderPass2", vkCmdEndRenderPass2},
    {"vkCmdEndRenderPass2KHR", vkCmdEndRenderPass2KHR},
    {"vkCmdEndRendering", vkCmdEndRendering},
    {"vkCmdEndRendering2EXT", vkCmdEndRendering2EXT},
    {"vkCmdEndRenderingKHR", vkCmdEndRenderingKHR},
    {"vkCmdEndTransformFeedbackEXT", vkCmdEndTransformFeedbackEXT},
    {"vkCmdEndVideoCodingKHR", vkCmdEndVideoCodingKHR},
    {"vkCmdExecuteCommands", vkCmdExecuteCommands},
    {"vkCmdExecuteGeneratedCommandsEXT", vkCmdExecuteGeneratedCommandsEXT},
    {"vkCmdExecuteGeneratedCommandsNV", vkCmdExecuteGeneratedCommandsNV},
    {"vkCmdFillBuffer", vkCmdFillBuffer},
    {"vkCmdInsertDebugUtilsLabelEXT", vkCmdInsertDebugUtilsLabelEXT},
    {"vkCmdNextSubpass", vkCmdNextSubpass},
    {"vkCmdNextSubpass2", vkCmdNextSubpass2},
    {"vkCmdNextSubpass2KHR", vkCmdNextSubpass2KHR},
    {"vkCmdOpticalFlowExecuteNV", vkCmdOpticalFlowExecuteNV},
    {"vkCmdPipelineBarrier", vkCmdPipelineBarrier},
    {"vkCmdPipelineBarrier2", vkCmdPipelineBarrier2},
    {"vkCmdPipelineBarrier2KHR", vkCmdPipelineBarrier2KHR},
    {"vkCmdPreprocessGeneratedCommandsEXT", vkCmdPreprocessGeneratedCommandsEXT},
    {"vkCmdPreprocessGeneratedCommandsNV", vkCmdPreprocessGeneratedCommandsNV},
    {"vkCmdPushConstants", vkCmdPushConstants},
    {"vkCmdPushConstants2", vkCmdPushConstants2},
    {"vkCmdPushConstants2KHR", vkCmdPushConstants2KHR},
    {"vkCmdPushDescriptorSet", vkCmdPushDescriptorSet},
    {"vkCmdPushDescriptorSet2", vkCmdPushDescriptorSet2},
    {"vkCmdPushDescriptorSet2KHR", vkCmdPushDescriptorSet2KHR},
    {"vkCmdPushDescriptorSetKHR", vkCmdPushDescriptorSetKHR},
    {"vkCmdPushDescriptorSetWithTemplate", vkCmdPushDescriptorSetWithTemplate},
    {"vkCmdPushDescriptorSetWithTemplate2", vkCmdPushDescriptorSetWithTemplate2},
    {"vkCmdPushDescriptorSetWithTemplate2KHR", vkCmdPushDescriptorSetWithTemplate2KHR},
    {"vkCmdPushDescriptorSetWithTemplateKHR", vkCmdPushDescriptorSetWithTemplateKHR},
    {"vkCmdResetEvent", vkCmdResetEvent},
    {"vkCmdResetEvent2", vkCmdResetEvent2},
    {"vkCmdResetEvent2KHR", vkCmdResetEvent2KHR},
    {"vkCmdResetQueryPool", vkCmdResetQueryPool},
    {"vkCmdResolveImage", vkCmdResolveImage},
    {"vkCmdResolveImage2", vkCmdResolveImage2},
    {"vkCmdResolveImage2KHR", vkCmdResolveImage2KHR},
    {"vkCmdSetAlphaToCoverageEnableEXT", vkCmdSetAlphaToCoverageEnableEXT},
    {"vkCmdSetAlphaToOneEnableEXT", vkCmdSetAlphaToOneEnableEXT},
    {"vkCmdSetAttachmentFeedbackLoopEnableEXT", vkCmdSetAttachmentFeedbackLoopEnableEXT},
    {"vkCmdSetBlendConstants", vkCmdSetBlendConstants},
    {"vkCmdSetCheckpointNV", vkCmdSetCheckpointNV},
    {"vkCmdSetCoarseSampleOrderNV", vkCmdSetCoarseSampleOrderNV},
    {"vkCmdSetColorBlendAdvancedEXT", vkCmdSetColorBlendAdvancedEXT},
    {"vkCmdSetColorBlendEnableEXT", vkCmdSetColorBlendEnableEXT},
    {"vkCmdSetColorBlendEquationEXT", vkCmdSetColorBlendEquationEXT},
    {"vkCmdSetColorWriteEnableEXT", vkCmdSetColorWriteEnableEXT},
    {"vkCmdSetColorWriteMaskEXT", vkCmdSetColorWriteMaskEXT},
    {"vkCmdSetConservativeRasterizationModeEXT", vkCmdSetConservativeRasterizationModeEXT},
    {"vkCmdSetCoverageModulationModeNV", vkCmdSetCoverageModulationModeNV},
    {"vkCmdSetCoverageModulationTableEnableNV", vkCmdSetCoverageModulationTableEnableNV},
    {"vkCmdSetCoverageModulationTableNV", vkCmdSetCoverageModulationTableNV},
    {"vkCmdSetCoverageReductionModeNV", vkCmdSetCoverageReductionModeNV},
    {"vkCmdSetCoverageToColorEnableNV", vkCmdSetCoverageToColorEnableNV},
    {"vkCmdSetCoverageToColorLocationNV", vkCmdSetCoverageToColorLocationNV},
    {"vkCmdSetCullMode", vkCmdSetCullMode},
    {"vkCmdSetCullModeEXT", vkCmdSetCullModeEXT},
    {"vkCmdSetDepthBias", vkCmdSetDepthBias},
    {"vkCmdSetDepthBias2EXT", vkCmdSetDepthBias2EXT},
    {"vkCmdSetDepthBiasEnable", vkCmdSetDepthBiasEnable},
    {"vkCmdSetDepthBiasEnableEXT", vkCmdSetDepthBiasEnableEXT},
    {"vkCmdSetDepthBounds", vkCmdSetDepthBounds},
    {"vkCmdSetDepthBoundsTestEnable", vkCmdSetDepthBoundsTestEnable},
    {"vkCmdSetDepthBoundsTestEnableEXT", vkCmdSetDepthBoundsTestEnableEXT},
    {"vkCmdSetDepthClampEnableEXT", vkCmdSetDepthClampEnableEXT},
    {"vkCmdSetDepthClampRangeEXT", vkCmdSetDepthClampRangeEXT},
    {"vkCmdSetDepthClipEnableEXT", vkCmdSetDepthClipEnableEXT},
    {"vkCmdSetDepthClipNegativeOneToOneEXT", vkCmdSetDepthClipNegativeOneToOneEXT},
    {"vkCmdSetDepthCompareOp", vkCmdSetDepthCompareOp},
    {"vkCmdSetDepthCompareOpEXT", vkCmdSetDepthCompareOpEXT},
    {"vkCmdSetDepthTestEnable", vkCmdSetDepthTestEnable},
    {"vkCmdSetDepthTestEnableEXT", vkCmdSetDepthTestEnableEXT},
    {"vkCmdSetDepthWriteEnable", vkCmdSetDepthWriteEnable},
    {"vkCmdSetDepthWriteEnableEXT", vkCmdSetDepthWriteEnableEXT},
    {"vkCmdSetDescriptorBufferOffsets2EXT", vkCmdSetDescriptorBufferOffsets2EXT},
    {"vkCmdSetDescriptorBufferOffsetsEXT", vkCmdSetDescriptorBufferOffsetsEXT},
    {"vkCmdSetDeviceMask", vkCmdSetDeviceMask},
    {"vkCmdSetDeviceMaskKHR", vkCmdSetDeviceMaskKHR},
    {"vkCmdSetDiscardRectangleEXT", vkCmdSetDiscardRectangleEXT},
    {"vkCmdSetDiscardRectangleEnableEXT", vkCmdSetDiscardRectangleEnableEXT},
    {"vkCmdSetDiscardRectangleModeEXT", vkCmdSetDiscardRectangleModeEXT},
    {"vkCmdSetEvent", vkCmdSetEvent},
    {"vkCmdSetEvent2", vkCmdSetEvent2},
    {"vkCmdSetEvent2KHR", vkCmdSetEvent2KHR},
    {"vkCmdSetExclusiveScissorEnableNV", vkCmdSetExclusiveScissorEnableNV},
    {"vkCmdSetExclusiveScissorNV", vkCmdSetExclusiveScissorNV},
    {"vkCmdSetExtraPrimitiveOverestimationSizeEXT", vkCmdSetExtraPrimitiveOverestimationSizeEXT},
    {"vkCmdSetFragmentShadingRateEnumNV", vkCmdSetFragmentShadingRateEnumNV},
    {"vkCmdSetFragmentShadingRateKHR", vkCmdSetFragmentShadingRateKHR},
    {"vkCmdSetFrontFace", vkCmdSetFrontFace},
    {"vkCmdSetFrontFaceEXT", vkCmdSetFrontFaceEXT},
    {"vkCmdSetLineRasterizationModeEXT", vkCmdSetLineRasterizationModeEXT},
    {"vkCmdSetLineStipple", vkCmdSetLineStipple},
    {"vkCmdSetLineStippleEXT", vkCmdSetLineStippleEXT},
    {"vkCmdSetLineStippleEnableEXT", vkCmdSetLineStippleEnableEXT},
    {"vkCmdSetLineStippleKHR", vkCmdSetLineStippleKHR},
    {"vkCmdSetLineWidth", vkCmdSetLineWidth},
    {"vkCmdSetLogicOpEXT", vkCmdSetLogicOpEXT},
    {"vkCmdSetLogicOpEnableEXT", vkCmdSetLogicOpEnableEXT},
    {"vkCmdSetPatchControlPointsEXT", vkCmdSetPatchControlPointsEXT},
    {"vkCmdSetPerformanceMarkerINTEL", vkCmdSetPerformanceMarkerINTEL},
    {"vkCmdSetPerformanceOverrideINTEL", vkCmdSetPerformanceOverrideINTEL},
    {"vkCmdSetPerformanceStreamMarkerINTEL", vkCmdSetPerformanceStreamMarkerINTEL},
    {"vkCmdSetPolygonModeEXT", vkCmdSetPolygonModeEXT},
    {"vkCmdSetPrimitiveRestartEnable", vkCmdSetPrimitiveRestartEnable},
    {"vkCmdSetPrimitiveRestartEnableEXT", vkCmdSetPrimitiveRestartEnableEXT},
    {"vkCmdSetPrimitiveTopology", vkCmdSetPrimitiveTopology},
    {"vkCmdSetPrimitiveTopologyEXT", vkCmdSetPrimitiveTopologyEXT},
    {"vkCmdSetProvokingVertexModeEXT", vkCmdSetProvokingVertexModeEXT},
    {"vkCmdSetRasterizationSamplesEXT", vkCmdSetRasterizationSamplesEXT},
    {"vkCmdSetRasterizationStreamEXT", vkCmdSetRasterizationStreamEXT},
    {"vkCmdSetRasterizerDiscardEnable", vkCmdSetRasterizerDiscardEnable},
    {"vkCmdSetRasterizerDiscardEnableEXT", vkCmdSetRasterizerDiscardEnableEXT},
    {"vkCmdSetRayTracingPipelineStackSizeKHR", vkCmdSetRayTracingPipelineStackSizeKHR},
    {"vkCmdSetRenderingAttachmentLocations", vkCmdSetRenderingAttachmentLocations},
    {"vkCmdSetRenderingAttachmentLocationsKHR", vkCmdSetRenderingAttachmentLocationsKHR},
    {"vkCmdSetRenderingInputAttachmentIndices", vkCmdSetRenderingInputAttachmentIndices},
    {"vkCmdSetRenderingInputAttachmentIndicesKHR", vkCmdSetRenderingInputAttachmentIndicesKHR},
    {"vkCmdSetRepresentativeFragmentTestEnableNV", vkCmdSetRepresentativeFragmentTestEnableNV},
    {"vkCmdSetSampleLocationsEXT", vkCmdSetSampleLocationsEXT},
    {"vkCmdSetSampleLocationsEnableEXT", vkCmdSetSampleLocationsEnableEXT},
    {"vkCmdSetSampleMaskEXT", vkCmdSetSampleMaskEXT},
    {"vkCmdSetScissor", vkCmdSetScissor},
    {"vkCmdSetScissorWithCount", vkCmdSetScissorWithCount},
    {"vkCmdSetScissorWithCountEXT", vkCmdSetScissorWithCountEXT},
    {"vkCmdSetShadingRateImageEnableNV", vkCmdSetShadingRateImageEnableNV},
    {"vkCmdSetStencilCompareMask", vkCmdSetStencilCompareMask},
    {"vkCmdSetStencilOp", vkCmdSetStencilOp},
    {"vkCmdSetStencilOpEXT", vkCmdSetStencilOpEXT},
    {"vkCmdSetStencilReference", vkCmdSetStencilReference},
    {"vkCmdSetStencilTestEnable", vkCmdSetStencilTestEnable},
    {"vkCmdSetStencilTestEnableEXT", vkCmdSetStencilTestEnableEXT},
    {"vkCmdSetStencilWriteMask", vkCmdSetStencilWriteMask},
    {"vkCmdSetTessellationDomainOriginEXT", vkCmdSetTessellationDomainOriginEXT},
    {"vkCmdSetVertexInputEXT", vkCmdSetVertexInputEXT},
    {"vkCmdSetViewport", vkCmdSetViewport},
    {"vkCmdSetViewportShadingRatePaletteNV", vkCmdSetViewportShadingRatePaletteNV},
    {"vkCmdSetViewportSwizzleNV", vkCmdSetViewportSwizzleNV},
    {"vkCmdSetViewportWScalingEnableNV", vkCmdSetViewportWScalingEnableNV},
    {"vkCmdSetViewportWScalingNV", vkCmdSetViewportWScalingNV},
    {"vkCmdSetViewportWithCount", vkCmdSetViewportWithCount},
    {"vkCmdSetViewportWithCountEXT", vkCmdSetViewportWithCountEXT},
    {"vkCmdSubpassShadingHUAWEI", vkCmdSubpassShadingHUAWEI},
    {"vkCmdTraceRaysIndirect2KHR", vkCmdTraceRaysIndirect2KHR},
    {"vkCmdTraceRaysIndirectKHR", vkCmdTraceRaysIndirectKHR},
    {"vkCmdTraceRaysKHR", vkCmdTraceRaysKHR},
    {"vkCmdTraceRaysNV", vkCmdTraceRaysNV},
    {"vkCmdUpdateBuffer", vkCmdUpdateBuffer},
    {"vkCmdUpdatePipelineIndirectBufferNV", vkCmdUpdatePipelineIndirectBufferNV},
    {"vkCmdWaitEvents", vkCmdWaitEvents},
    {"vkCmdWaitEvents2", vkCmdWaitEvents2},
    {"vkCmdWaitEvents2KHR", vkCmdWaitEvents2KHR},
    {"vkCmdWriteAccelerationStructuresPropertiesKHR", vkCmdWriteAccelerationStructuresPropertiesKHR},
    {"vkCmdWriteAccelerationStructuresPropertiesNV", vkCmdWriteAccelerationStructuresPropertiesNV},
    {"vkCmdWriteBufferMarker2AMD", vkCmdWriteBufferMarker2AMD},
    {"vkCmdWriteBufferMarkerAMD", vkCmdWriteBufferMarkerAMD},
    {"vkCmdWriteMicromapsPropertiesEXT", vkCmdWriteMicromapsPropertiesEXT},
    {"vkCmdWriteTimestamp", vkCmdWriteTimestamp},
    {"vkCmdWriteTimestamp2", vkCmdWriteTimestamp2},
    {"vkCmdWriteTimestamp2KHR", vkCmdWriteTimestamp2KHR},
    {"vkCompileDeferredNV", vkCompileDeferredNV},
    {"vkConvertCooperativeVectorMatrixNV", vkConvertCooperativeVectorMatrixNV},
    {"vkCopyAccelerationStructureKHR", vkCopyAccelerationStructureKHR},
    {"vkCopyAccelerationStructureToMemoryKHR", vkCopyAccelerationStructureToMemoryKHR},
    {"vkCopyImageToImage", vkCopyImageToImage},
    {"vkCopyImageToImageEXT", vkCopyImageToImageEXT},
    {"vkCopyImageToMemory", vkCopyImageToMemory},
    {"vkCopyImageToMemoryEXT", vkCopyImageToMemoryEXT},
    {"vkCopyMemoryToAccelerationStructureKHR", vkCopyMemoryToAccelerationStructureKHR},
    {"vkCopyMemoryToImage", vkCopyMemoryToImage},
    {"vkCopyMemoryToImageEXT", vkCopyMemoryToImageEXT},
    {"vkCopyMemoryToMicromapEXT", vkCopyMemoryToMicromapEXT},
    {"vkCopyMicromapEXT", vkCopyMicromapEXT},
    {"vkCopyMicromapToMemoryEXT", vkCopyMicromapToMemoryEXT},
    {"vkCreateAccelerationStructureKHR", vkCreateAccelerationStructureKHR},
    {"vkCreateAccelerationStructureNV", vkCreateAccelerationStructureNV},
    {"vkCreateBuffer", vkCreateBuffer},
    {"vkCreateBufferView", vkCreateBufferView},
    {"vkCreateCommandPool", vkCreateCommandPool},
    {"vkCreateComputePipelines", vkCreateComputePipelines},
    {"vkCreateCuFunctionNVX", vkCreateCuFunctionNVX},
    {"vkCreateCuModuleNVX", vkCreateCuModuleNVX},
    {"vkCreateDataGraphPipelineSessionARM", vkCreateDataGraphPipelineSessionARM},
    {"vkCreateDataGraphPipelinesARM", vkCreateDataGraphPipelinesARM},
    {"vkCreateDeferredOperationKHR", vkCreateDeferredOperationKHR},
    {"vkCreateDescriptorPool", vkCreateDescriptorPool},
    {"vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout},
    {"vkCreateDescriptorUpdateTemplate", vkCreateDescriptorUpdateTemplate},
    {"vkCreateDescriptorUpdateTemplateKHR", vkCreateDescriptorUpdateTemplateKHR},
    {"vkCreateEvent", vkCreateEvent},
    {"vkCreateFence", vkCreateFence},
    {"vkCreateFramebuffer", vkCreateFramebuffer},
    {"vkCreateGraphicsPipelines", vkCreateGraphicsPipelines},
    {"vkCreateImage", vkCreateImage},
    {"vkCreateImageView", vkCreateImageView},
    {"vkCreateIndirectCommandsLayoutEXT", vkCreateIndirectCommandsLayoutEXT},
    {"vkCreateIndirectCommandsLayoutNV", vkCreateIndirectCommandsLayoutNV},
    {"vkCreateIndirectExecutionSetEXT", vkCreateIndirectExecutionSetEXT},
    {"vkCreateMicromapEXT", vkCreateMicromapEXT},
    {"vkCreateOpticalFlowSessionNV", vkCreateOpticalFlowSessionNV},
    {"vkCreatePipelineBinariesKHR", vkCreatePipelineBinariesKHR},
    {"vkCreatePipelineCache", vkCreatePipelineCache},
    {"vkCreatePipelineLayout", vkCreatePipelineLayout},
    {"vkCreatePrivateDataSlot", vkCreatePrivateDataSlot},
    {"vkCreatePrivateDataSlotEXT", vkCreatePrivateDataSlotEXT},
    {"vkCreateQueryPool", vkCreateQueryPool},
    {"vkCreateRayTracingPipelinesKHR", vkCreateRayTracingPipelinesKHR},
    {"vkCreateRayTracingPipelinesNV", vkCreateRayTracingPipelinesNV},
    {"vkCreateRenderPass", vkCreateRenderPass},
    {"vkCreateRenderPass2", vkCreateRenderPass2},
    {"vkCreateRenderPass2KHR", vkCreateRenderPass2KHR},
    {"vkCreateSampler", vkCreateSampler},
    {"vkCreateSamplerYcbcrConversion", vkCreateSamplerYcbcrConversion},
    {"vkCreateSamplerYcbcrConversionKHR", vkCreateSamplerYcbcrConversionKHR},
    {"vkCreateSemaphore", vkCreateSemaphore},
    {"vkCreateShaderModule", vkCreateShaderModule},
    {"vkCreateShadersEXT", vkCreateShadersEXT},
    {"vkCreateSwapchainKHR", vkCreateSwapchainKHR},
    {"vkCreateTensorARM", vkCreateTensorARM},
    {"vkCreateTensorViewARM", vkCreateTensorViewARM},
    {"vkCreateValidationCacheEXT", vkCreateValidationCacheEXT},
    {"vkCreateVideoSessionKHR", vkCreateVideoSessionKHR},
    {"vkCreateVideoSessionParametersKHR", vkCreateVideoSessionParametersKHR},
    {"vkDebugMarkerSetObjectNameEXT", vkDebugMarkerSetObjectNameEXT},
    {"vkDebugMarkerSetObjectTagEXT", vkDebugMarkerSetObjectTagEXT},
    {"vkDeferredOperationJoinKHR", vkDeferredOperationJoinKHR},
    {"vkDestroyAccelerationStructureKHR", vkDestroyAccelerationStructureKHR},
    {"vkDestroyAccelerationStructureNV", vkDestroyAccelerationStructureNV},
    {"vkDestroyBuffer", vkDestroyBuffer},
    {"vkDestroyBufferView", vkDestroyBufferView},
    {"vkDestroyCommandPool", vkDestroyCommandPool},
    {"vkDestroyCuFunctionNVX", vkDestroyCuFunctionNVX},
    {"vkDestroyCuModuleNVX", vkDestroyCuModuleNVX},
    {"vkDestroyDataGraphPipelineSessionARM", vkDestroyDataGraphPipelineSessionARM},
    {"vkDestroyDeferredOperationKHR", vkDestroyDeferredOperationKHR},
    {"vkDestroyDescriptorPool", vkDestroyDescriptorPool},
    {"vkDestroyDescriptorSetLayout", vkDestroyDescriptorSetLayout},
    {"vkDestroyDescriptorUpdateTemplate", vkDestroyDescriptorUpdateTemplate},
    {"vkDestroyDescriptorUpdateTemplateKHR", vkDestroyDescriptorUpdateTemplateKHR},
    {"vkDestroyDevice", vkDestroyDevice},
    {"vkDestroyEvent", vkDestroyEvent},
    {"vkDestroyFence", vkDestroyFence},
    {"vkDestroyFramebuffer", vkDestroyFramebuffer},
    {"vkDestroyImage", vkDestroyImage},
    {"vkDestroyImageView", vkDestroyImageView},
    {"vkDestroyIndirectCommandsLayoutEXT", vkDestroyIndirectCommandsLayoutEXT},
    {"vkDestroyIndirectCommandsLayoutNV", vkDestroyIndirectCommandsLayoutNV},
    {"vkDestroyIndirectExecutionSetEXT", vkDestroyIndirectExecutionSetEXT},
    {"vkDestroyMicromapEXT", vkDestroyMicromapEXT},
    {"vkDestroyOpticalFlowSessionNV", vkDestroyOpticalFlowSessionNV},
    {"vkDestroyPipeline", vkDestroyPipeline},
    {"vkDestroyPipelineBinaryKHR", vkDestroyPipelineBinaryKHR},
    {"vkDestroyPipelineCache", vkDestroyPipelineCache},
    {"vkDestroyPipelineLayout", vkDestroyPipelineLayout},
    {"vkDestroyPrivateDataSlot", vkDestroyPrivateDataSlot},
    {"vkDestroyPrivateDataSlotEXT", vkDestroyPrivateDataSlotEXT},
    {"vkDestroyQueryPool", vkDestroyQueryPool},
    {"vkDestroyRenderPass", vkDestroyRenderPass},
    {"vkDestroySampler", vkDestroySampler},
    {"vkDestroySamplerYcbcrConversion", vkDestroySamplerYcbcrConversion},
    {"vkDestroySamplerYcbcrConversionKHR", vkDestroySamplerYcbcrConversionKHR},
    {"vkDestroySemaphore", vkDestroySemaphore},
    {"vkDestroyShaderEXT", vkDestroyShaderEXT},
    {"vkDestroyShaderModule", vkDestroyShaderModule},
    {"vkDestroySwapchainKHR", vkDestroySwapchainKHR},
    {"vkDestroyTensorARM", vkDestroyTensorARM},
    {"vkDestroyTensorViewARM", vkDestroyTensorViewARM},
    {"vkDestroyValidationCacheEXT", vkDestroyValidationCacheEXT},
    {"vkDestroyVideoSessionKHR", vkDestroyVideoSessionKHR},
    {"vkDestroyVideoSessionParametersKHR", vkDestroyVideoSessionParametersKHR},
    {"vkDeviceWaitIdle", vkDeviceWaitIdle},
    {"vkEndCommandBuffer", vkEndCommandBuffer},
    {"vkFlushMappedMemoryRanges", vkFlushMappedMemoryRanges},
    {"vkFreeCommandBuffers", vkFreeCommandBuffers},
    {"vkFreeDescriptorSets", vkFreeDescriptorSets},
    {"vkFreeMemory", vkFreeMemory},
    {"vkGetAccelerationStructureBuildSizesKHR", vkGetAccelerationStructureBuildSizesKHR},
    {"vkGetAccelerationStructureDeviceAddressKHR", vkGetAccelerationStructureDeviceAddressKHR},
    {"vkGetAccelerationStructureHandleNV", vkGetAccelerationStructureHandleNV},
    {"vkGetAccelerationStructureMemoryRequirementsNV", vkGetAccelerationStructureMemoryRequirementsNV},
    {"vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT", vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT},
    {"vkGetBufferDeviceAddress", vkGetBufferDeviceAddress},
    {"vkGetBufferDeviceAddressEXT", vkGetBufferDeviceAddressEXT},
    {"vkGetBufferDeviceAddressKHR", vkGetBufferDeviceAddressKHR},
    {"vkGetBufferMemoryRequirements", vkGetBufferMemoryRequirements},
    {"vkGetBufferMemoryRequirements2", vkGetBufferMemoryRequirements2},
    {"vkGetBufferMemoryRequirements2KHR", vkGetBufferMemoryRequirements2KHR},
    {"vkGetBufferOpaqueCaptureAddress", vkGetBufferOpaqueCaptureAddress},
    {"vkGetBufferOpaqueCaptureAddressKHR", vkGetBufferOpaqueCaptureAddressKHR},
    {"vkGetBufferOpaqueCaptureDescriptorDataEXT", vkGetBufferOpaqueCaptureDescriptorDataEXT},
    {"vkGetCalibratedTimestampsEXT", vkGetCalibratedTimestampsEXT},
    {"vkGetCalibratedTimestampsKHR", vkGetCalibratedTimestampsKHR},
    {"vkGetClusterAccelerationStructureBuildSizesNV", vkGetClusterAccelerationStructureBuildSizesNV},
    {"vkGetDataGraphPipelineAvailablePropertiesARM", vkGetDataGraphPipelineAvailablePropertiesARM},
    {"vkGetDataGraphPipelinePropertiesARM", vkGetDataGraphPipelinePropertiesARM},
    {"vkGetDataGraphPipelineSessionBindPointRequirementsARM", vkGetDataGraphPipelineSessionBindPointRequirementsARM},
    {"vkGetDataGraphPipelineSessionMemoryRequirementsARM", vkGetDataGraphPipelineSessionMemoryRequirementsARM},
    {"vkGetDeferredOperationMaxConcurrencyKHR", vkGetDeferredOperationMaxConcurrencyKHR},
    {"vkGetDeferredOperationResultKHR", vkGetDeferredOperationResultKHR},
    {"vkGetDescriptorEXT", vkGetDescriptorEXT},
    {"vkGetDescriptorSetHostMappingVALVE", vkGetDescriptorSetHostMappingVALVE},
    {"vkGetDescriptorSetLayoutBindingOffsetEXT", vkGetDescriptorSetLayoutBindingOffsetEXT},
    {"vkGetDescriptorSetLayoutHostMappingInfoVALVE", vkGetDescriptorSetLayoutHostMappingInfoVALVE},
    {"vkGetDescriptorSetLayoutSizeEXT", vkGetDescriptorSetLayoutSizeEXT},
    {"vkGetDescriptorSetLayoutSupport", vkGetDescriptorSetLayoutSupport},
    {"vkGetDescriptorSetLayoutSupportKHR", vkGetDescriptorSetLayoutSupportKHR},
    {"vkGetDeviceAccelerationStructureCompatibilityKHR", vkGetDeviceAccelerationStructureCompatibilityKHR},
    {"vkGetDeviceBufferMemoryRequirements", vkGetDeviceBufferMemoryRequirements},
    {"vkGetDeviceBufferMemoryRequirementsKHR", vkGetDeviceBufferMemoryRequirementsKHR},
    {"vkGetDeviceFaultInfoEXT", vkGetDeviceFaultInfoEXT},
    {"vkGetDeviceGroupPeerMemoryFeatures", vkGetDeviceGroupPeerMemoryFeatures},
    {"vkGetDeviceGroupPeerMemoryFeaturesKHR", vkGetDeviceGroupPeerMemoryFeaturesKHR},
    {"vkGetDeviceGroupPresentCapabilitiesKHR", vkGetDeviceGroupPresentCapabilitiesKHR},
    {"vkGetDeviceGroupSurfacePresentModesKHR", vkGetDeviceGroupSurfacePresentModesKHR},
    {"vkGetDeviceImageMemoryRequirements", vkGetDeviceImageMemoryRequirements},
    {"vkGetDeviceImageMemoryRequirementsKHR", vkGetDeviceImageMemoryRequirementsKHR},
    {"vkGetDeviceImageSparseMemoryRequirements", vkGetDeviceImageSparseMemoryRequirements},
    {"vkGetDeviceImageSparseMemoryRequirementsKHR", vkGetDeviceImageSparseMemoryRequirementsKHR},
    {"vkGetDeviceImageSubresourceLayout", vkGetDeviceImageSubresourceLayout},
    {"vkGetDeviceImageSubresourceLayoutKHR", vkGetDeviceImageSubresourceLayoutKHR},
    {"vkGetDeviceMemoryCommitment", vkGetDeviceMemoryCommitment},
    {"vkGetDeviceMemoryOpaqueCaptureAddress", vkGetDeviceMemoryOpaqueCaptureAddress},
    {"vkGetDeviceMemoryOpaqueCaptureAddressKHR", vkGetDeviceMemoryOpaqueCaptureAddressKHR},
    {"vkGetDeviceMicromapCompatibilityEXT", vkGetDeviceMicromapCompatibilityEXT},
    {"vkGetDeviceProcAddr", vkGetDeviceProcAddr},
    {"vkGetDeviceQueue", vkGetDeviceQueue},
    {"vkGetDeviceQueue2", vkGetDeviceQueue2},
    {"vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI", vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI},
    {"vkGetDeviceTensorMemoryRequirementsARM", vkGetDeviceTensorMemoryRequirementsARM},
    {"vkGetDynamicRenderingTilePropertiesQCOM", vkGetDynamicRenderingTilePropertiesQCOM},
    {"vkGetEncodedVideoSessionParametersKHR", vkGetEncodedVideoSessionParametersKHR},
    {"vkGetEventStatus", vkGetEventStatus},
    {"vkGetFenceStatus", vkGetFenceStatus},
    {"vkGetFramebufferTilePropertiesQCOM", vkGetFramebufferTilePropertiesQCOM},
    {"vkGetGeneratedCommandsMemoryRequirementsEXT", vkGetGeneratedCommandsMemoryRequirementsEXT},
    {"vkGetGeneratedCommandsMemoryRequirementsNV", vkGetGeneratedCommandsMemoryRequirementsNV},
    {"vkGetImageMemoryRequirements", vkGetImageMemoryRequirements},
    {"vkGetImageMemoryRequirements2", vkGetImageMemoryRequirements2},
    {"vkGetImageMemoryRequirements2KHR", vkGetImageMemoryRequirements2KHR},
    {"vkGetImageOpaqueCaptureDescriptorDataEXT", vkGetImageOpaqueCaptureDescriptorDataEXT},
    {"vkGetImageSparseMemoryRequirements", vkGetImageSparseMemoryRequirements},
    {"vkGetImageSparseMemoryRequirements2", vkGetImageSparseMemoryRequirements2},
    {"vkGetImageSparseMemoryRequirements2KHR", vkGetImageSparseMemoryRequirements2KHR},
    {"vkGetImageSubresourceLayout", vkGetImageSubresourceLayout},
    {"vkGetImageSubresourceLayout2", vkGetImageSubresourceLayout2},
    {"vkGetImageSubresourceLayout2EXT", vkGetImageSubresourceLayout2EXT},
    {"vkGetImageSubresourceLayout2KHR", vkGetImageSubresourceLayout2KHR},
    {"vkGetImageViewAddressNVX", vkGetImageViewAddressNVX},
    {"vkGetImageViewHandle64NVX", vkGetImageViewHandle64NVX},
    {"vkGetImageViewHandleNVX", vkGetImageViewHandleNVX},
    {"vkGetImageViewOpaqueCaptureDescriptorDataEXT", vkGetImageViewOpaqueCaptureDescriptorDataEXT},
    {"vkGetLatencyTimingsNV", vkGetLatencyTimingsNV},
    {"vkGetMemoryHostPointerPropertiesEXT", vkGetMemoryHostPointerPropertiesEXT},
    {"vkGetMemoryWin32HandleKHR", vkGetMemoryWin32HandleKHR},
    {"vkGetMemoryWin32HandlePropertiesKHR", vkGetMemoryWin32HandlePropertiesKHR},
    {"vkGetMicromapBuildSizesEXT", vkGetMicromapBuildSizesEXT},
    {"vkGetPartitionedAccelerationStructuresBuildSizesNV", vkGetPartitionedAccelerationStructuresBuildSizesNV},
    {"vkGetPerformanceParameterINTEL", vkGetPerformanceParameterINTEL},
    {"vkGetPipelineBinaryDataKHR", vkGetPipelineBinaryDataKHR},
    {"vkGetPipelineCacheData", vkGetPipelineCacheData},
    {"vkGetPipelineExecutableInternalRepresentationsKHR", vkGetPipelineExecutableInternalRepresentationsKHR},
    {"vkGetPipelineExecutablePropertiesKHR", vkGetPipelineExecutablePropertiesKHR},
    {"vkGetPipelineExecutableStatisticsKHR", vkGetPipelineExecutableStatisticsKHR},
    {"vkGetPipelineIndirectDeviceAddressNV", vkGetPipelineIndirectDeviceAddressNV},
    {"vkGetPipelineIndirectMemoryRequirementsNV", vkGetPipelineIndirectMemoryRequirementsNV},
    {"vkGetPipelineKeyKHR", vkGetPipelineKeyKHR},
    {"vkGetPipelinePropertiesEXT", vkGetPipelinePropertiesEXT},
    {"vkGetPrivateData", vkGetPrivateData},
    {"vkGetPrivateDataEXT", vkGetPrivateDataEXT},
    {"vkGetQueryPoolResults", vkGetQueryPoolResults},
    {"vkGetQueueCheckpointData2NV", vkGetQueueCheckpointData2NV},
    {"vkGetQueueCheckpointDataNV", vkGetQueueCheckpointDataNV},
    {"vkGetRayTracingCaptureReplayShaderGroupHandlesKHR", vkGetRayTracingCaptureReplayShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesKHR", vkGetRayTracingShaderGroupHandlesKHR},
    {"vkGetRayTracingShaderGroupHandlesNV", vkGetRayTracingShaderGroupHandlesNV},
    {"vkGetRayTracingShaderGroupStackSizeKHR", vkGetRayTracingShaderGroupStackSizeKHR},
    {"vkGetRenderAreaGranularity", vkGetRenderAreaGranularity},
    {"vkGetRenderingAreaGranularity", vkGetRenderingAreaGranularity},
    {"vkGetRenderingAreaGranularityKHR", vkGetRenderingAreaGranularityKHR},
    {"vkGetSamplerOpaqueCaptureDescriptorDataEXT", vkGetSamplerOpaqueCaptureDescriptorDataEXT},
    {"vkGetSemaphoreCounterValue", vkGetSemaphoreCounterValue},
    {"vkGetSemaphoreCounterValueKHR", vkGetSemaphoreCounterValueKHR},
    {"vkGetShaderBinaryDataEXT", vkGetShaderBinaryDataEXT},
    {"vkGetShaderInfoAMD", vkGetShaderInfoAMD},
    {"vkGetShaderModuleCreateInfoIdentifierEXT", vkGetShaderModuleCreateInfoIdentifierEXT},
    {"vkGetShaderModuleIdentifierEXT", vkGetShaderModuleIdentifierEXT},
    {"vkGetSwapchainImagesKHR", vkGetSwapchainImagesKHR},
    {"vkGetTensorMemoryRequirementsARM", vkGetTensorMemoryRequirementsARM},
    {"vkGetTensorOpaqueCaptureDescriptorDataARM", vkGetTensorOpaqueCaptureDescriptorDataARM},
    {"vkGetTensorViewOpaqueCaptureDescriptorDataARM", vkGetTensorViewOpaqueCaptureDescriptorDataARM},
    {"vkGetValidationCacheDataEXT", vkGetValidationCacheDataEXT},
    {"vkGetVideoSessionMemoryRequirementsKHR", vkGetVideoSessionMemoryRequirementsKHR},
    {"vkInitializePerformanceApiINTEL", vkInitializePerformanceApiINTEL},
    {"vkInvalidateMappedMemoryRanges", vkInvalidateMappedMemoryRanges},
    {"vkLatencySleepNV", vkLatencySleepNV},
    {"vkMapMemory", vkMapMemory},
    {"vkMapMemory2", vkMapMemory2},
    {"vkMapMemory2KHR", vkMapMemory2KHR},
    {"vkMergePipelineCaches", vkMergePipelineCaches},
    {"vkMergeValidationCachesEXT", vkMergeValidationCachesEXT},
    {"vkQueueBeginDebugUtilsLabelEXT", vkQueueBeginDebugUtilsLabelEXT},
    {"vkQueueBindSparse", vkQueueBindSparse},
    {"vkQueueEndDebugUtilsLabelEXT", vkQueueEndDebugUtilsLabelEXT},
    {"vkQueueInsertDebugUtilsLabelEXT", vkQueueInsertDebugUtilsLabelEXT},
    {"vkQueueNotifyOutOfBandNV", vkQueueNotifyOutOfBandNV},
    {"vkQueuePresentKHR", vkQueuePresentKHR},
    {"vkQueueSetPerformanceConfigurationINTEL", vkQueueSetPerformanceConfigurationINTEL},
    {"vkQueueSubmit", vkQueueSubmit},
    {"vkQueueSubmit2", vkQueueSubmit2},
    {"vkQueueSubmit2KHR", vkQueueSubmit2KHR},
    {"vkQueueWaitIdle", vkQueueWaitIdle},
    {"vkReleaseCapturedPipelineDataKHR", vkReleaseCapturedPipelineDataKHR},
    {"vkReleasePerformanceConfigurationINTEL", vkReleasePerformanceConfigurationINTEL},
    {"vkReleaseProfilingLockKHR", vkReleaseProfilingLockKHR},
    {"vkReleaseSwapchainImagesEXT", vkReleaseSwapchainImagesEXT},
    {"vkReleaseSwapchainImagesKHR", vkReleaseSwapchainImagesKHR},
    {"vkResetCommandBuffer", vkResetCommandBuffer},
    {"vkResetCommandPool", vkResetCommandPool},
    {"vkResetDescriptorPool", vkResetDescriptorPool},
    {"vkResetEvent", vkResetEvent},
    {"vkResetFences", vkResetFences},
    {"vkResetQueryPool", vkResetQueryPool},
    {"vkResetQueryPoolEXT", vkResetQueryPoolEXT},
    {"vkSetDebugUtilsObjectNameEXT", vkSetDebugUtilsObjectNameEXT},
    {"vkSetDebugUtilsObjectTagEXT", vkSetDebugUtilsObjectTagEXT},
    {"vkSetDeviceMemoryPriorityEXT", vkSetDeviceMemoryPriorityEXT},
    {"vkSetEvent", vkSetEvent},
    {"vkSetHdrMetadataEXT", vkSetHdrMetadataEXT},
    {"vkSetLatencyMarkerNV", vkSetLatencyMarkerNV},
    {"vkSetLatencySleepModeNV", vkSetLatencySleepModeNV},
    {"vkSetPrivateData", vkSetPrivateData},
    {"vkSetPrivateDataEXT", vkSetPrivateDataEXT},
    {"vkSignalSemaphore", vkSignalSemaphore},
    {"vkSignalSemaphoreKHR", vkSignalSemaphoreKHR},
    {"vkTransitionImageLayout", vkTransitionImageLayout},
    {"vkTransitionImageLayoutEXT", vkTransitionImageLayoutEXT},
    {"vkTrimCommandPool", vkTrimCommandPool},
    {"vkTrimCommandPoolKHR", vkTrimCommandPoolKHR},
    {"vkUninitializePerformanceApiINTEL", vkUninitializePerformanceApiINTEL},
    {"vkUnmapMemory", vkUnmapMemory},
    {"vkUnmapMemory2", vkUnmapMemory2},
    {"vkUnmapMemory2KHR", vkUnmapMemory2KHR},
    {"vkUpdateDescriptorSetWithTemplate", vkUpdateDescriptorSetWithTemplate},
    {"vkUpdateDescriptorSetWithTemplateKHR", vkUpdateDescriptorSetWithTemplateKHR},
    {"vkUpdateDescriptorSets", vkUpdateDescriptorSets},
    {"vkUpdateIndirectExecutionSetPipelineEXT", vkUpdateIndirectExecutionSetPipelineEXT},
    {"vkUpdateIndirectExecutionSetShaderEXT", vkUpdateIndirectExecutionSetShaderEXT},
    {"vkUpdateVideoSessionParametersKHR", vkUpdateVideoSessionParametersKHR},
    {"vkWaitForFences", vkWaitForFences},
    {"vkWaitForPresent2KHR", vkWaitForPresent2KHR},
    {"vkWaitForPresentKHR", vkWaitForPresentKHR},
    {"vkWaitSemaphores", vkWaitSemaphores},
    {"vkWaitSemaphoresKHR", vkWaitSemaphoresKHR},
    {"vkWriteAccelerationStructuresPropertiesKHR", vkWriteAccelerationStructuresPropertiesKHR},
    {"vkWriteMicromapsPropertiesEXT", vkWriteMicromapsPropertiesEXT},
};

static const struct vulkan_func vk_phys_dev_dispatch_table[] =
{
    {"vkCreateDevice", vkCreateDevice},
    {"vkEnumerateDeviceExtensionProperties", vkEnumerateDeviceExtensionProperties},
    {"vkEnumerateDeviceLayerProperties", vkEnumerateDeviceLayerProperties},
    {"vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR", vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR},
    {"vkGetPhysicalDeviceCalibrateableTimeDomainsEXT", vkGetPhysicalDeviceCalibrateableTimeDomainsEXT},
    {"vkGetPhysicalDeviceCalibrateableTimeDomainsKHR", vkGetPhysicalDeviceCalibrateableTimeDomainsKHR},
    {"vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV", vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV},
    {"vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR", vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR},
    {"vkGetPhysicalDeviceCooperativeMatrixPropertiesNV", vkGetPhysicalDeviceCooperativeMatrixPropertiesNV},
    {"vkGetPhysicalDeviceCooperativeVectorPropertiesNV", vkGetPhysicalDeviceCooperativeVectorPropertiesNV},
    {"vkGetPhysicalDeviceExternalBufferProperties", vkGetPhysicalDeviceExternalBufferProperties},
    {"vkGetPhysicalDeviceExternalBufferPropertiesKHR", vkGetPhysicalDeviceExternalBufferPropertiesKHR},
    {"vkGetPhysicalDeviceExternalFenceProperties", vkGetPhysicalDeviceExternalFenceProperties},
    {"vkGetPhysicalDeviceExternalFencePropertiesKHR", vkGetPhysicalDeviceExternalFencePropertiesKHR},
    {"vkGetPhysicalDeviceExternalSemaphoreProperties", vkGetPhysicalDeviceExternalSemaphoreProperties},
    {"vkGetPhysicalDeviceExternalSemaphorePropertiesKHR", vkGetPhysicalDeviceExternalSemaphorePropertiesKHR},
    {"vkGetPhysicalDeviceExternalTensorPropertiesARM", vkGetPhysicalDeviceExternalTensorPropertiesARM},
    {"vkGetPhysicalDeviceFeatures", vkGetPhysicalDeviceFeatures},
    {"vkGetPhysicalDeviceFeatures2", vkGetPhysicalDeviceFeatures2},
    {"vkGetPhysicalDeviceFeatures2KHR", vkGetPhysicalDeviceFeatures2KHR},
    {"vkGetPhysicalDeviceFormatProperties", vkGetPhysicalDeviceFormatProperties},
    {"vkGetPhysicalDeviceFormatProperties2", vkGetPhysicalDeviceFormatProperties2},
    {"vkGetPhysicalDeviceFormatProperties2KHR", vkGetPhysicalDeviceFormatProperties2KHR},
    {"vkGetPhysicalDeviceFragmentShadingRatesKHR", vkGetPhysicalDeviceFragmentShadingRatesKHR},
    {"vkGetPhysicalDeviceImageFormatProperties", vkGetPhysicalDeviceImageFormatProperties},
    {"vkGetPhysicalDeviceImageFormatProperties2", vkGetPhysicalDeviceImageFormatProperties2},
    {"vkGetPhysicalDeviceImageFormatProperties2KHR", vkGetPhysicalDeviceImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceMemoryProperties", vkGetPhysicalDeviceMemoryProperties},
    {"vkGetPhysicalDeviceMemoryProperties2", vkGetPhysicalDeviceMemoryProperties2},
    {"vkGetPhysicalDeviceMemoryProperties2KHR", vkGetPhysicalDeviceMemoryProperties2KHR},
    {"vkGetPhysicalDeviceMultisamplePropertiesEXT", vkGetPhysicalDeviceMultisamplePropertiesEXT},
    {"vkGetPhysicalDeviceOpticalFlowImageFormatsNV", vkGetPhysicalDeviceOpticalFlowImageFormatsNV},
    {"vkGetPhysicalDevicePresentRectanglesKHR", vkGetPhysicalDevicePresentRectanglesKHR},
    {"vkGetPhysicalDeviceProperties", vkGetPhysicalDeviceProperties},
    {"vkGetPhysicalDeviceProperties2", vkGetPhysicalDeviceProperties2},
    {"vkGetPhysicalDeviceProperties2KHR", vkGetPhysicalDeviceProperties2KHR},
    {"vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM", vkGetPhysicalDeviceQueueFamilyDataGraphProcessingEnginePropertiesARM},
    {"vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM", vkGetPhysicalDeviceQueueFamilyDataGraphPropertiesARM},
    {"vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR", vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR},
    {"vkGetPhysicalDeviceQueueFamilyProperties", vkGetPhysicalDeviceQueueFamilyProperties},
    {"vkGetPhysicalDeviceQueueFamilyProperties2", vkGetPhysicalDeviceQueueFamilyProperties2},
    {"vkGetPhysicalDeviceQueueFamilyProperties2KHR", vkGetPhysicalDeviceQueueFamilyProperties2KHR},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", vkGetPhysicalDeviceSparseImageFormatProperties},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2", vkGetPhysicalDeviceSparseImageFormatProperties2},
    {"vkGetPhysicalDeviceSparseImageFormatProperties2KHR", vkGetPhysicalDeviceSparseImageFormatProperties2KHR},
    {"vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV", vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV},
    {"vkGetPhysicalDeviceSurfaceCapabilities2KHR", vkGetPhysicalDeviceSurfaceCapabilities2KHR},
    {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", vkGetPhysicalDeviceSurfaceCapabilitiesKHR},
    {"vkGetPhysicalDeviceSurfaceFormats2KHR", vkGetPhysicalDeviceSurfaceFormats2KHR},
    {"vkGetPhysicalDeviceSurfaceFormatsKHR", vkGetPhysicalDeviceSurfaceFormatsKHR},
    {"vkGetPhysicalDeviceSurfacePresentModesKHR", vkGetPhysicalDeviceSurfacePresentModesKHR},
    {"vkGetPhysicalDeviceSurfaceSupportKHR", vkGetPhysicalDeviceSurfaceSupportKHR},
    {"vkGetPhysicalDeviceToolProperties", vkGetPhysicalDeviceToolProperties},
    {"vkGetPhysicalDeviceToolPropertiesEXT", vkGetPhysicalDeviceToolPropertiesEXT},
    {"vkGetPhysicalDeviceVideoCapabilitiesKHR", vkGetPhysicalDeviceVideoCapabilitiesKHR},
    {"vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR", vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR},
    {"vkGetPhysicalDeviceVideoFormatPropertiesKHR", vkGetPhysicalDeviceVideoFormatPropertiesKHR},
    {"vkGetPhysicalDeviceWin32PresentationSupportKHR", vkGetPhysicalDeviceWin32PresentationSupportKHR},
};

static const struct vulkan_func vk_instance_dispatch_table[] =
{
    {"vkCreateDebugReportCallbackEXT", vkCreateDebugReportCallbackEXT},
    {"vkCreateDebugUtilsMessengerEXT", vkCreateDebugUtilsMessengerEXT},
    {"vkCreateWin32SurfaceKHR", vkCreateWin32SurfaceKHR},
    {"vkDebugReportMessageEXT", vkDebugReportMessageEXT},
    {"vkDestroyDebugReportCallbackEXT", vkDestroyDebugReportCallbackEXT},
    {"vkDestroyDebugUtilsMessengerEXT", vkDestroyDebugUtilsMessengerEXT},
    {"vkDestroyInstance", vkDestroyInstance},
    {"vkDestroySurfaceKHR", vkDestroySurfaceKHR},
    {"vkEnumeratePhysicalDeviceGroups", vkEnumeratePhysicalDeviceGroups},
    {"vkEnumeratePhysicalDeviceGroupsKHR", vkEnumeratePhysicalDeviceGroupsKHR},
    {"vkEnumeratePhysicalDevices", vkEnumeratePhysicalDevices},
    {"vkSubmitDebugUtilsMessageEXT", vkSubmitDebugUtilsMessageEXT},
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

void *wine_vk_get_phys_dev_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_phys_dev_dispatch_table); i++)
    {
        if (strcmp(vk_phys_dev_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found name=%s in physical device table\n", debugstr_a(name));
            return vk_phys_dev_dispatch_table[i].func;
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
