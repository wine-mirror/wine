/* Automatically generated from Vulkan vk.xml; DO NOT EDIT! */

#include "config.h"
#include "wine/port.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"
#include "vulkan_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

#if defined(USE_STRUCT_CONVERSION)
static inline void convert_VkCommandBufferAllocateInfo_win_to_host(const VkCommandBufferAllocateInfo *in, VkCommandBufferAllocateInfo_host *out)
{
    if (!in) return;

    out->sType = in->sType;
    out->pNext = in->pNext;
    out->commandPool = in->commandPool;
    out->level = in->level;
    out->commandBufferCount = in->commandBufferCount;
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

static inline void convert_VkMemoryRequirements_host_to_win(const VkMemoryRequirements_host *in, VkMemoryRequirements *out)
{
    if (!in) return;

    out->size = in->size;
    out->alignment = in->alignment;
    out->memoryTypeBits = in->memoryTypeBits;
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

static VkResult WINAPI wine_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets)
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

static VkResult WINAPI wine_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory)
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

static VkResult WINAPI wine_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo)
{
    FIXME("stub: %p, %p\n", commandBuffer, pBeginInfo);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult WINAPI wine_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", device, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(memory), wine_dbgstr_longlong(memoryOffset));
    return device->funcs.p_vkBindBufferMemory(device->device, buffer, memory, memoryOffset);
}

static VkResult WINAPI wine_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s\n", device, wine_dbgstr_longlong(image), wine_dbgstr_longlong(memory), wine_dbgstr_longlong(memoryOffset));
    return device->funcs.p_vkBindImageMemory(device->device, image, memory, memoryOffset);
}

static void WINAPI wine_vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    FIXME("stub: %p, 0x%s, %u, %#x\n", commandBuffer, wine_dbgstr_longlong(queryPool), query, flags);
}

static void WINAPI wine_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    FIXME("stub: %p, %p, %d\n", commandBuffer, pRenderPassBegin, contents);
}

static void WINAPI wine_vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
{
    FIXME("stub: %p, %d, 0x%s, %u, %u, %p, %u, %p\n", commandBuffer, pipelineBindPoint, wine_dbgstr_longlong(layout), firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

static void WINAPI wine_vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    FIXME("stub: %p, 0x%s, 0x%s, %d\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), indexType);
}

static void WINAPI wine_vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    FIXME("stub: %p, %d, 0x%s\n", commandBuffer, pipelineBindPoint, wine_dbgstr_longlong(pipeline));
}

static void WINAPI wine_vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
{
    FIXME("stub: %p, %u, %u, %p, %p\n", commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

static void WINAPI wine_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter)
{
    FIXME("stub: %p, 0x%s, %d, 0x%s, %d, %u, %p, %d\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions, filter);
}

static void WINAPI wine_vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects)
{
    FIXME("stub: %p, %u, %p, %u, %p\n", commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

static void WINAPI wine_vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    FIXME("stub: %p, 0x%s, %d, %p, %u, %p\n", commandBuffer, wine_dbgstr_longlong(image), imageLayout, pColor, rangeCount, pRanges);
}

static void WINAPI wine_vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges)
{
    FIXME("stub: %p, 0x%s, %d, %p, %u, %p\n", commandBuffer, wine_dbgstr_longlong(image), imageLayout, pDepthStencil, rangeCount, pRanges);
}

static void WINAPI wine_vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
{
    FIXME("stub: %p, 0x%s, 0x%s, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcBuffer), wine_dbgstr_longlong(dstBuffer), regionCount, pRegions);
}

static void WINAPI wine_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    FIXME("stub: %p, 0x%s, 0x%s, %d, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcBuffer), wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions);
}

static void WINAPI wine_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions)
{
    FIXME("stub: %p, 0x%s, %d, 0x%s, %d, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions);
}

static void WINAPI wine_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions)
{
    FIXME("stub: %p, 0x%s, %d, 0x%s, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstBuffer), regionCount, pRegions);
}

static void WINAPI wine_vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    FIXME("stub: %p, 0x%s, %u, %u, 0x%s, 0x%s, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(queryPool), firstQuery, queryCount, wine_dbgstr_longlong(dstBuffer), wine_dbgstr_longlong(dstOffset), wine_dbgstr_longlong(stride), flags);
}

static void WINAPI wine_vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    FIXME("stub: %p, %u, %u, %u\n", commandBuffer, groupCountX, groupCountY, groupCountZ);
}

static void WINAPI wine_vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    FIXME("stub: %p, 0x%s, 0x%s\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset));
}

static void WINAPI wine_vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    FIXME("stub: %p, %u, %u, %u, %u\n", commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

static void WINAPI wine_vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    FIXME("stub: %p, %u, %u, %u, %d, %u\n", commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

static void WINAPI wine_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    FIXME("stub: %p, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), drawCount, stride);
}

static void WINAPI wine_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    FIXME("stub: %p, 0x%s, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(buffer), wine_dbgstr_longlong(offset), drawCount, stride);
}

static void WINAPI wine_vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    FIXME("stub: %p, 0x%s, %u\n", commandBuffer, wine_dbgstr_longlong(queryPool), query);
}

static void WINAPI wine_vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    FIXME("stub: %p\n", commandBuffer);
}

static void WINAPI wine_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    FIXME("stub: %p, 0x%s, 0x%s, 0x%s, %u\n", commandBuffer, wine_dbgstr_longlong(dstBuffer), wine_dbgstr_longlong(dstOffset), wine_dbgstr_longlong(size), data);
}

static void WINAPI wine_vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    FIXME("stub: %p, %d\n", commandBuffer, contents);
}

static void WINAPI wine_vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    FIXME("stub: %p, %#x, %#x, %#x, %u, %p, %u, %p, %u, %p\n", commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

static void WINAPI wine_vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues)
{
    FIXME("stub: %p, 0x%s, %#x, %u, %u, %p\n", commandBuffer, wine_dbgstr_longlong(layout), stageFlags, offset, size, pValues);
}

static void WINAPI wine_vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    FIXME("stub: %p, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(event), stageMask);
}

static void WINAPI wine_vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    FIXME("stub: %p, 0x%s, %u, %u\n", commandBuffer, wine_dbgstr_longlong(queryPool), firstQuery, queryCount);
}

static void WINAPI wine_vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions)
{
    FIXME("stub: %p, 0x%s, %d, 0x%s, %d, %u, %p\n", commandBuffer, wine_dbgstr_longlong(srcImage), srcImageLayout, wine_dbgstr_longlong(dstImage), dstImageLayout, regionCount, pRegions);
}

static void WINAPI wine_vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    FIXME("stub: %p, %p\n", commandBuffer, blendConstants);
}

static void WINAPI wine_vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    FIXME("stub: %p, %f, %f, %f\n", commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

static void WINAPI wine_vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    FIXME("stub: %p, %f, %f\n", commandBuffer, minDepthBounds, maxDepthBounds);
}

static void WINAPI wine_vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    FIXME("stub: %p, 0x%s, %#x\n", commandBuffer, wine_dbgstr_longlong(event), stageMask);
}

static void WINAPI wine_vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    FIXME("stub: %p, %f\n", commandBuffer, lineWidth);
}

static void WINAPI wine_vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors)
{
    FIXME("stub: %p, %u, %u, %p\n", commandBuffer, firstScissor, scissorCount, pScissors);
}

static void WINAPI wine_vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    FIXME("stub: %p, %#x, %u\n", commandBuffer, faceMask, compareMask);
}

static void WINAPI wine_vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    FIXME("stub: %p, %#x, %u\n", commandBuffer, faceMask, reference);
}

static void WINAPI wine_vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    FIXME("stub: %p, %#x, %u\n", commandBuffer, faceMask, writeMask);
}

static void WINAPI wine_vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports)
{
    FIXME("stub: %p, %u, %u, %p\n", commandBuffer, firstViewport, viewportCount, pViewports);
}

static void WINAPI wine_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData)
{
    FIXME("stub: %p, 0x%s, 0x%s, 0x%s, %p\n", commandBuffer, wine_dbgstr_longlong(dstBuffer), wine_dbgstr_longlong(dstOffset), wine_dbgstr_longlong(dataSize), pData);
}

static void WINAPI wine_vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
{
    FIXME("stub: %p, %u, %p, %#x, %#x, %u, %p, %u, %p, %u, %p\n", commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

static void WINAPI wine_vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    FIXME("stub: %p, %d, 0x%s, %u\n", commandBuffer, pipelineStage, wine_dbgstr_longlong(queryPool), query);
}

static VkResult WINAPI wine_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer)
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

static VkResult WINAPI wine_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
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

static VkResult WINAPI wine_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pCommandPool);
    return device->funcs.p_vkCreateCommandPool(device->device, pCreateInfo, NULL, pCommandPool);
}

static VkResult WINAPI wine_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
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

static VkResult WINAPI wine_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pDescriptorPool);
    return device->funcs.p_vkCreateDescriptorPool(device->device, pCreateInfo, NULL, pDescriptorPool);
}

static VkResult WINAPI wine_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSetLayout);
    return device->funcs.p_vkCreateDescriptorSetLayout(device->device, pCreateInfo, NULL, pSetLayout);
}

static VkResult WINAPI wine_vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pEvent);
    return device->funcs.p_vkCreateEvent(device->device, pCreateInfo, NULL, pEvent);
}

static VkResult WINAPI wine_vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pFence);
    return device->funcs.p_vkCreateFence(device->device, pCreateInfo, NULL, pFence);
}

static VkResult WINAPI wine_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer)
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

static VkResult WINAPI wine_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
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

static VkResult WINAPI wine_vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pImage);
    return device->funcs.p_vkCreateImage(device->device, pCreateInfo, NULL, pImage);
}

static VkResult WINAPI wine_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView)
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

static VkResult WINAPI wine_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pPipelineCache);
    return device->funcs.p_vkCreatePipelineCache(device->device, pCreateInfo, NULL, pPipelineCache);
}

static VkResult WINAPI wine_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pPipelineLayout);
    return device->funcs.p_vkCreatePipelineLayout(device->device, pCreateInfo, NULL, pPipelineLayout);
}

static VkResult WINAPI wine_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pQueryPool);
    return device->funcs.p_vkCreateQueryPool(device->device, pCreateInfo, NULL, pQueryPool);
}

static VkResult WINAPI wine_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pRenderPass);
    return device->funcs.p_vkCreateRenderPass(device->device, pCreateInfo, NULL, pRenderPass);
}

static VkResult WINAPI wine_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSampler);
    return device->funcs.p_vkCreateSampler(device->device, pCreateInfo, NULL, pSampler);
}

static VkResult WINAPI wine_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pSemaphore);
    return device->funcs.p_vkCreateSemaphore(device->device, pCreateInfo, NULL, pSemaphore);
}

static VkResult WINAPI wine_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule)
{
    TRACE("%p, %p, %p, %p\n", device, pCreateInfo, pAllocator, pShaderModule);
    return device->funcs.p_vkCreateShaderModule(device->device, pCreateInfo, NULL, pShaderModule);
}

static void WINAPI wine_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(buffer), pAllocator);
    device->funcs.p_vkDestroyBuffer(device->device, buffer, NULL);
}

static void WINAPI wine_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(bufferView), pAllocator);
    device->funcs.p_vkDestroyBufferView(device->device, bufferView, NULL);
}

static void WINAPI wine_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(commandPool), pAllocator);
    device->funcs.p_vkDestroyCommandPool(device->device, commandPool, NULL);
}

static void WINAPI wine_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorPool), pAllocator);
    device->funcs.p_vkDestroyDescriptorPool(device->device, descriptorPool, NULL);
}

static void WINAPI wine_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(descriptorSetLayout), pAllocator);
    device->funcs.p_vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, NULL);
}

static void WINAPI wine_vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(event), pAllocator);
    device->funcs.p_vkDestroyEvent(device->device, event, NULL);
}

static void WINAPI wine_vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(fence), pAllocator);
    device->funcs.p_vkDestroyFence(device->device, fence, NULL);
}

static void WINAPI wine_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(framebuffer), pAllocator);
    device->funcs.p_vkDestroyFramebuffer(device->device, framebuffer, NULL);
}

static void WINAPI wine_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(image), pAllocator);
    device->funcs.p_vkDestroyImage(device->device, image, NULL);
}

static void WINAPI wine_vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(imageView), pAllocator);
    device->funcs.p_vkDestroyImageView(device->device, imageView, NULL);
}

static void WINAPI wine_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(pipeline), pAllocator);
    device->funcs.p_vkDestroyPipeline(device->device, pipeline, NULL);
}

static void WINAPI wine_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(pipelineCache), pAllocator);
    device->funcs.p_vkDestroyPipelineCache(device->device, pipelineCache, NULL);
}

static void WINAPI wine_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(pipelineLayout), pAllocator);
    device->funcs.p_vkDestroyPipelineLayout(device->device, pipelineLayout, NULL);
}

static void WINAPI wine_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(queryPool), pAllocator);
    device->funcs.p_vkDestroyQueryPool(device->device, queryPool, NULL);
}

static void WINAPI wine_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(renderPass), pAllocator);
    device->funcs.p_vkDestroyRenderPass(device->device, renderPass, NULL);
}

static void WINAPI wine_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(sampler), pAllocator);
    device->funcs.p_vkDestroySampler(device->device, sampler, NULL);
}

static void WINAPI wine_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(semaphore), pAllocator);
    device->funcs.p_vkDestroySemaphore(device->device, semaphore, NULL);
}

static void WINAPI wine_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(shaderModule), pAllocator);
    device->funcs.p_vkDestroyShaderModule(device->device, shaderModule, NULL);
}

static VkResult WINAPI wine_vkDeviceWaitIdle(VkDevice device)
{
    TRACE("%p\n", device);
    return device->funcs.p_vkDeviceWaitIdle(device->device);
}

static VkResult WINAPI wine_vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    FIXME("stub: %p\n", commandBuffer);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult WINAPI wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pPropertyCount, pProperties);
    return physicalDevice->instance->funcs.p_vkEnumerateDeviceLayerProperties(physicalDevice->phys_dev, pPropertyCount, pProperties);
}

static VkResult WINAPI wine_vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
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

static VkResult WINAPI wine_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets)
{
    TRACE("%p, 0x%s, %u, %p\n", device, wine_dbgstr_longlong(descriptorPool), descriptorSetCount, pDescriptorSets);
    return device->funcs.p_vkFreeDescriptorSets(device->device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

static void WINAPI wine_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(memory), pAllocator);
    device->funcs.p_vkFreeMemory(device->device, memory, NULL);
}

static void WINAPI wine_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements)
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

static void WINAPI wine_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(memory), pCommittedMemoryInBytes);
    device->funcs.p_vkGetDeviceMemoryCommitment(device->device, memory, pCommittedMemoryInBytes);
}

static VkResult WINAPI wine_vkGetEventStatus(VkDevice device, VkEvent event)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(event));
    return device->funcs.p_vkGetEventStatus(device->device, event);
}

static VkResult WINAPI wine_vkGetFenceStatus(VkDevice device, VkFence fence)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(fence));
    return device->funcs.p_vkGetFenceStatus(device->device, fence);
}

static void WINAPI wine_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements)
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

static void WINAPI wine_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements *pSparseMemoryRequirements)
{
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(image), pSparseMemoryRequirementCount, pSparseMemoryRequirements);
    device->funcs.p_vkGetImageSparseMemoryRequirements(device->device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

static void WINAPI wine_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout)
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

static void WINAPI wine_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
    TRACE("%p, %p\n", physicalDevice, pFeatures);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFeatures(physicalDevice->phys_dev, pFeatures);
}

static void WINAPI wine_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties)
{
    TRACE("%p, %d, %p\n", physicalDevice, format, pFormatProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceFormatProperties(physicalDevice->phys_dev, format, pFormatProperties);
}

static VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties)
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult result;
    VkImageFormatProperties_host pImageFormatProperties_host;
    TRACE("%p, %d, %d, %d, %#x, %#x, %p\n", physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);

    result = physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(physicalDevice->phys_dev, format, type, tiling, usage, flags, &pImageFormatProperties_host);

    convert_VkImageFormatProperties_host_to_win(&pImageFormatProperties_host, pImageFormatProperties);
    return result;
#else
    TRACE("%p, %d, %d, %d, %#x, %#x, %p\n", physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    return physicalDevice->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties(physicalDevice->phys_dev, format, type, tiling, usage, flags, pImageFormatProperties);
#endif
}

static void WINAPI wine_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties)
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

static void WINAPI wine_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
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

static void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties)
{
    TRACE("%p, %p, %p\n", physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice->phys_dev, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

static void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties)
{
    TRACE("%p, %d, %d, %d, %#x, %d, %p, %p\n", physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
    physicalDevice->instance->funcs.p_vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice->phys_dev, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

static VkResult WINAPI wine_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData)
{
    TRACE("%p, 0x%s, %p, %p\n", device, wine_dbgstr_longlong(pipelineCache), pDataSize, pData);
    return device->funcs.p_vkGetPipelineCacheData(device->device, pipelineCache, pDataSize, pData);
}

static VkResult WINAPI wine_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    TRACE("%p, 0x%s, %u, %u, 0x%s, %p, 0x%s, %#x\n", device, wine_dbgstr_longlong(queryPool), firstQuery, queryCount, wine_dbgstr_longlong(dataSize), pData, wine_dbgstr_longlong(stride), flags);
    return device->funcs.p_vkGetQueryPoolResults(device->device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

static void WINAPI wine_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity)
{
    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(renderPass), pGranularity);
    device->funcs.p_vkGetRenderAreaGranularity(device->device, renderPass, pGranularity);
}

static VkResult WINAPI wine_vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges)
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

static VkResult WINAPI wine_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
    TRACE("%p, 0x%s, 0x%s, 0x%s, %#x, %p\n", device, wine_dbgstr_longlong(memory), wine_dbgstr_longlong(offset), wine_dbgstr_longlong(size), flags, ppData);
    return device->funcs.p_vkMapMemory(device->device, memory, offset, size, flags, ppData);
}

static VkResult WINAPI wine_vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches)
{
    TRACE("%p, 0x%s, %u, %p\n", device, wine_dbgstr_longlong(dstCache), srcCacheCount, pSrcCaches);
    return device->funcs.p_vkMergePipelineCaches(device->device, dstCache, srcCacheCount, pSrcCaches);
}

static VkResult WINAPI wine_vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence)
{
    FIXME("stub: %p, %u, %p, 0x%s\n", queue, bindInfoCount, pBindInfo, wine_dbgstr_longlong(fence));
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult WINAPI wine_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
{
    FIXME("stub: %p, %u, %p, 0x%s\n", queue, submitCount, pSubmits, wine_dbgstr_longlong(fence));
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult WINAPI wine_vkQueueWaitIdle(VkQueue queue)
{
    FIXME("stub: %p\n", queue);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult WINAPI wine_vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    FIXME("stub: %p, %#x\n", commandBuffer, flags);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult WINAPI wine_vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    TRACE("%p, 0x%s, %#x\n", device, wine_dbgstr_longlong(commandPool), flags);
    return device->funcs.p_vkResetCommandPool(device->device, commandPool, flags);
}

static VkResult WINAPI wine_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    TRACE("%p, 0x%s, %#x\n", device, wine_dbgstr_longlong(descriptorPool), flags);
    return device->funcs.p_vkResetDescriptorPool(device->device, descriptorPool, flags);
}

static VkResult WINAPI wine_vkResetEvent(VkDevice device, VkEvent event)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(event));
    return device->funcs.p_vkResetEvent(device->device, event);
}

static VkResult WINAPI wine_vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
{
    TRACE("%p, %u, %p\n", device, fenceCount, pFences);
    return device->funcs.p_vkResetFences(device->device, fenceCount, pFences);
}

static VkResult WINAPI wine_vkSetEvent(VkDevice device, VkEvent event)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(event));
    return device->funcs.p_vkSetEvent(device->device, event);
}

static void WINAPI wine_vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    TRACE("%p, 0x%s\n", device, wine_dbgstr_longlong(memory));
    device->funcs.p_vkUnmapMemory(device->device, memory);
}

static void WINAPI wine_vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies)
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

static VkResult WINAPI wine_vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout)
{
    TRACE("%p, %u, %p, %u, 0x%s\n", device, fenceCount, pFences, waitAll, wine_dbgstr_longlong(timeout));
    return device->funcs.p_vkWaitForFences(device->device, fenceCount, pFences, waitAll, timeout);
}

static const struct vulkan_func vk_device_dispatch_table[] =
{
    {"vkAcquireNextImageKHR", &wine_vkAcquireNextImageKHR},
    {"vkAllocateCommandBuffers", &wine_vkAllocateCommandBuffers},
    {"vkAllocateDescriptorSets", &wine_vkAllocateDescriptorSets},
    {"vkAllocateMemory", &wine_vkAllocateMemory},
    {"vkBeginCommandBuffer", &wine_vkBeginCommandBuffer},
    {"vkBindBufferMemory", &wine_vkBindBufferMemory},
    {"vkBindImageMemory", &wine_vkBindImageMemory},
    {"vkCmdBeginQuery", &wine_vkCmdBeginQuery},
    {"vkCmdBeginRenderPass", &wine_vkCmdBeginRenderPass},
    {"vkCmdBindDescriptorSets", &wine_vkCmdBindDescriptorSets},
    {"vkCmdBindIndexBuffer", &wine_vkCmdBindIndexBuffer},
    {"vkCmdBindPipeline", &wine_vkCmdBindPipeline},
    {"vkCmdBindVertexBuffers", &wine_vkCmdBindVertexBuffers},
    {"vkCmdBlitImage", &wine_vkCmdBlitImage},
    {"vkCmdClearAttachments", &wine_vkCmdClearAttachments},
    {"vkCmdClearColorImage", &wine_vkCmdClearColorImage},
    {"vkCmdClearDepthStencilImage", &wine_vkCmdClearDepthStencilImage},
    {"vkCmdCopyBuffer", &wine_vkCmdCopyBuffer},
    {"vkCmdCopyBufferToImage", &wine_vkCmdCopyBufferToImage},
    {"vkCmdCopyImage", &wine_vkCmdCopyImage},
    {"vkCmdCopyImageToBuffer", &wine_vkCmdCopyImageToBuffer},
    {"vkCmdCopyQueryPoolResults", &wine_vkCmdCopyQueryPoolResults},
    {"vkCmdDispatch", &wine_vkCmdDispatch},
    {"vkCmdDispatchIndirect", &wine_vkCmdDispatchIndirect},
    {"vkCmdDraw", &wine_vkCmdDraw},
    {"vkCmdDrawIndexed", &wine_vkCmdDrawIndexed},
    {"vkCmdDrawIndexedIndirect", &wine_vkCmdDrawIndexedIndirect},
    {"vkCmdDrawIndirect", &wine_vkCmdDrawIndirect},
    {"vkCmdEndQuery", &wine_vkCmdEndQuery},
    {"vkCmdEndRenderPass", &wine_vkCmdEndRenderPass},
    {"vkCmdExecuteCommands", &wine_vkCmdExecuteCommands},
    {"vkCmdFillBuffer", &wine_vkCmdFillBuffer},
    {"vkCmdNextSubpass", &wine_vkCmdNextSubpass},
    {"vkCmdPipelineBarrier", &wine_vkCmdPipelineBarrier},
    {"vkCmdPushConstants", &wine_vkCmdPushConstants},
    {"vkCmdResetEvent", &wine_vkCmdResetEvent},
    {"vkCmdResetQueryPool", &wine_vkCmdResetQueryPool},
    {"vkCmdResolveImage", &wine_vkCmdResolveImage},
    {"vkCmdSetBlendConstants", &wine_vkCmdSetBlendConstants},
    {"vkCmdSetDepthBias", &wine_vkCmdSetDepthBias},
    {"vkCmdSetDepthBounds", &wine_vkCmdSetDepthBounds},
    {"vkCmdSetEvent", &wine_vkCmdSetEvent},
    {"vkCmdSetLineWidth", &wine_vkCmdSetLineWidth},
    {"vkCmdSetScissor", &wine_vkCmdSetScissor},
    {"vkCmdSetStencilCompareMask", &wine_vkCmdSetStencilCompareMask},
    {"vkCmdSetStencilReference", &wine_vkCmdSetStencilReference},
    {"vkCmdSetStencilWriteMask", &wine_vkCmdSetStencilWriteMask},
    {"vkCmdSetViewport", &wine_vkCmdSetViewport},
    {"vkCmdUpdateBuffer", &wine_vkCmdUpdateBuffer},
    {"vkCmdWaitEvents", &wine_vkCmdWaitEvents},
    {"vkCmdWriteTimestamp", &wine_vkCmdWriteTimestamp},
    {"vkCreateBuffer", &wine_vkCreateBuffer},
    {"vkCreateBufferView", &wine_vkCreateBufferView},
    {"vkCreateCommandPool", &wine_vkCreateCommandPool},
    {"vkCreateComputePipelines", &wine_vkCreateComputePipelines},
    {"vkCreateDescriptorPool", &wine_vkCreateDescriptorPool},
    {"vkCreateDescriptorSetLayout", &wine_vkCreateDescriptorSetLayout},
    {"vkCreateEvent", &wine_vkCreateEvent},
    {"vkCreateFence", &wine_vkCreateFence},
    {"vkCreateFramebuffer", &wine_vkCreateFramebuffer},
    {"vkCreateGraphicsPipelines", &wine_vkCreateGraphicsPipelines},
    {"vkCreateImage", &wine_vkCreateImage},
    {"vkCreateImageView", &wine_vkCreateImageView},
    {"vkCreatePipelineCache", &wine_vkCreatePipelineCache},
    {"vkCreatePipelineLayout", &wine_vkCreatePipelineLayout},
    {"vkCreateQueryPool", &wine_vkCreateQueryPool},
    {"vkCreateRenderPass", &wine_vkCreateRenderPass},
    {"vkCreateSampler", &wine_vkCreateSampler},
    {"vkCreateSemaphore", &wine_vkCreateSemaphore},
    {"vkCreateShaderModule", &wine_vkCreateShaderModule},
    {"vkCreateSwapchainKHR", &wine_vkCreateSwapchainKHR},
    {"vkDestroyBuffer", &wine_vkDestroyBuffer},
    {"vkDestroyBufferView", &wine_vkDestroyBufferView},
    {"vkDestroyCommandPool", &wine_vkDestroyCommandPool},
    {"vkDestroyDescriptorPool", &wine_vkDestroyDescriptorPool},
    {"vkDestroyDescriptorSetLayout", &wine_vkDestroyDescriptorSetLayout},
    {"vkDestroyDevice", &wine_vkDestroyDevice},
    {"vkDestroyEvent", &wine_vkDestroyEvent},
    {"vkDestroyFence", &wine_vkDestroyFence},
    {"vkDestroyFramebuffer", &wine_vkDestroyFramebuffer},
    {"vkDestroyImage", &wine_vkDestroyImage},
    {"vkDestroyImageView", &wine_vkDestroyImageView},
    {"vkDestroyPipeline", &wine_vkDestroyPipeline},
    {"vkDestroyPipelineCache", &wine_vkDestroyPipelineCache},
    {"vkDestroyPipelineLayout", &wine_vkDestroyPipelineLayout},
    {"vkDestroyQueryPool", &wine_vkDestroyQueryPool},
    {"vkDestroyRenderPass", &wine_vkDestroyRenderPass},
    {"vkDestroySampler", &wine_vkDestroySampler},
    {"vkDestroySemaphore", &wine_vkDestroySemaphore},
    {"vkDestroyShaderModule", &wine_vkDestroyShaderModule},
    {"vkDestroySwapchainKHR", &wine_vkDestroySwapchainKHR},
    {"vkDeviceWaitIdle", &wine_vkDeviceWaitIdle},
    {"vkEndCommandBuffer", &wine_vkEndCommandBuffer},
    {"vkFlushMappedMemoryRanges", &wine_vkFlushMappedMemoryRanges},
    {"vkFreeCommandBuffers", &wine_vkFreeCommandBuffers},
    {"vkFreeDescriptorSets", &wine_vkFreeDescriptorSets},
    {"vkFreeMemory", &wine_vkFreeMemory},
    {"vkGetBufferMemoryRequirements", &wine_vkGetBufferMemoryRequirements},
    {"vkGetDeviceMemoryCommitment", &wine_vkGetDeviceMemoryCommitment},
    {"vkGetDeviceProcAddr", &wine_vkGetDeviceProcAddr},
    {"vkGetDeviceQueue", &wine_vkGetDeviceQueue},
    {"vkGetEventStatus", &wine_vkGetEventStatus},
    {"vkGetFenceStatus", &wine_vkGetFenceStatus},
    {"vkGetImageMemoryRequirements", &wine_vkGetImageMemoryRequirements},
    {"vkGetImageSparseMemoryRequirements", &wine_vkGetImageSparseMemoryRequirements},
    {"vkGetImageSubresourceLayout", &wine_vkGetImageSubresourceLayout},
    {"vkGetPipelineCacheData", &wine_vkGetPipelineCacheData},
    {"vkGetQueryPoolResults", &wine_vkGetQueryPoolResults},
    {"vkGetRenderAreaGranularity", &wine_vkGetRenderAreaGranularity},
    {"vkGetSwapchainImagesKHR", &wine_vkGetSwapchainImagesKHR},
    {"vkInvalidateMappedMemoryRanges", &wine_vkInvalidateMappedMemoryRanges},
    {"vkMapMemory", &wine_vkMapMemory},
    {"vkMergePipelineCaches", &wine_vkMergePipelineCaches},
    {"vkQueueBindSparse", &wine_vkQueueBindSparse},
    {"vkQueuePresentKHR", &wine_vkQueuePresentKHR},
    {"vkQueueSubmit", &wine_vkQueueSubmit},
    {"vkQueueWaitIdle", &wine_vkQueueWaitIdle},
    {"vkResetCommandBuffer", &wine_vkResetCommandBuffer},
    {"vkResetCommandPool", &wine_vkResetCommandPool},
    {"vkResetDescriptorPool", &wine_vkResetDescriptorPool},
    {"vkResetEvent", &wine_vkResetEvent},
    {"vkResetFences", &wine_vkResetFences},
    {"vkSetEvent", &wine_vkSetEvent},
    {"vkUnmapMemory", &wine_vkUnmapMemory},
    {"vkUpdateDescriptorSets", &wine_vkUpdateDescriptorSets},
    {"vkWaitForFences", &wine_vkWaitForFences},
};

static const struct vulkan_func vk_instance_dispatch_table[] =
{
    {"vkCreateDevice", &wine_vkCreateDevice},
    {"vkCreateWin32SurfaceKHR", &wine_vkCreateWin32SurfaceKHR},
    {"vkDestroyInstance", &wine_vkDestroyInstance},
    {"vkDestroySurfaceKHR", &wine_vkDestroySurfaceKHR},
    {"vkEnumerateDeviceExtensionProperties", &wine_vkEnumerateDeviceExtensionProperties},
    {"vkEnumerateDeviceLayerProperties", &wine_vkEnumerateDeviceLayerProperties},
    {"vkEnumeratePhysicalDevices", &wine_vkEnumeratePhysicalDevices},
    {"vkGetPhysicalDeviceFeatures", &wine_vkGetPhysicalDeviceFeatures},
    {"vkGetPhysicalDeviceFormatProperties", &wine_vkGetPhysicalDeviceFormatProperties},
    {"vkGetPhysicalDeviceImageFormatProperties", &wine_vkGetPhysicalDeviceImageFormatProperties},
    {"vkGetPhysicalDeviceMemoryProperties", &wine_vkGetPhysicalDeviceMemoryProperties},
    {"vkGetPhysicalDeviceProperties", &wine_vkGetPhysicalDeviceProperties},
    {"vkGetPhysicalDeviceQueueFamilyProperties", &wine_vkGetPhysicalDeviceQueueFamilyProperties},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", &wine_vkGetPhysicalDeviceSparseImageFormatProperties},
    {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", &wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR},
    {"vkGetPhysicalDeviceSurfaceFormatsKHR", &wine_vkGetPhysicalDeviceSurfaceFormatsKHR},
    {"vkGetPhysicalDeviceSurfacePresentModesKHR", &wine_vkGetPhysicalDeviceSurfacePresentModesKHR},
    {"vkGetPhysicalDeviceSurfaceSupportKHR", &wine_vkGetPhysicalDeviceSurfaceSupportKHR},
    {"vkGetPhysicalDeviceWin32PresentationSupportKHR", &wine_vkGetPhysicalDeviceWin32PresentationSupportKHR},
};

void *wine_vk_get_device_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_device_dispatch_table); i++)
    {
        if (strcmp(vk_device_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found name=%s in device table\n", name);
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
            TRACE("Found name=%s in instance table\n", name);
            return vk_instance_dispatch_table[i].func;
        }
    }
    return NULL;
}

static const char * const vk_device_extensions[] =
{
    "VK_KHR_swapchain",
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
