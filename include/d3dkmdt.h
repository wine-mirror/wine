/*
 * Copyright 2020 Brendan Shanks for CodeWeavers
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

#ifndef __WINE_D3DKMDT_H
#define __WINE_D3DKMDT_H

#define DXGK_FEATURE_SUPPORT_ALWAYS_OFF   ((UINT)0)
#define DXGK_FEATURE_SUPPORT_EXPERIMENTAL ((UINT)1)
#define DXGK_FEATURE_SUPPORT_STABLE       ((UINT)2)
#define DXGK_FEATURE_SUPPORT_ALWAYS_ON    ((UINT)3)

typedef enum _D3DKMDT_VIDEO_SIGNAL_STANDARD
{
    D3DKMDT_VSS_UNINITIALIZED =  0,
    D3DKMDT_VSS_VESA_DMT      =  1,
    D3DKMDT_VSS_VESA_GTF      =  2,
    D3DKMDT_VSS_VESA_CVT      =  3,
    D3DKMDT_VSS_IBM           =  4,
    D3DKMDT_VSS_APPLE         =  5,
    D3DKMDT_VSS_NTSC_M        =  6,
    D3DKMDT_VSS_NTSC_J        =  7,
    D3DKMDT_VSS_NTSC_443      =  8,
    D3DKMDT_VSS_PAL_B         =  9,
    D3DKMDT_VSS_PAL_B1        = 10,
    D3DKMDT_VSS_PAL_G         = 11,
    D3DKMDT_VSS_PAL_H         = 12,
    D3DKMDT_VSS_PAL_I         = 13,
    D3DKMDT_VSS_PAL_D         = 14,
    D3DKMDT_VSS_PAL_N         = 15,
    D3DKMDT_VSS_PAL_NC        = 16,
    D3DKMDT_VSS_SECAM_B       = 17,
    D3DKMDT_VSS_SECAM_D       = 18,
    D3DKMDT_VSS_SECAM_G       = 19,
    D3DKMDT_VSS_SECAM_H       = 20,
    D3DKMDT_VSS_SECAM_K       = 21,
    D3DKMDT_VSS_SECAM_K1      = 22,
    D3DKMDT_VSS_SECAM_L       = 23,
    D3DKMDT_VSS_SECAM_L1      = 24,
    D3DKMDT_VSS_EIA_861       = 25,
    D3DKMDT_VSS_EIA_861A      = 26,
    D3DKMDT_VSS_EIA_861B      = 27,
    D3DKMDT_VSS_PAL_K         = 28,
    D3DKMDT_VSS_PAL_K1        = 29,
    D3DKMDT_VSS_PAL_L         = 30,
    D3DKMDT_VSS_PAL_M         = 31,
    D3DKMDT_VSS_OTHER         = 255
} D3DKMDT_VIDEO_SIGNAL_STANDARD;

typedef enum _D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY
{
    D3DKMDT_GRAPHICS_PREEMPTION_NONE                = 0,
    D3DKMDT_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY = 100,
    D3DKMDT_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY  = 200,
    D3DKMDT_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY   = 300,
    D3DKMDT_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY      = 400,
    D3DKMDT_GRAPHICS_PREEMPTION_SHADER_BOUNDARY     = 500,
} D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY;

typedef enum _D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY
{
    D3DKMDT_COMPUTE_PREEMPTION_NONE                  = 0,
    D3DKMDT_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY   = 100,
    D3DKMDT_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY     = 200,
    D3DKMDT_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY = 300,
    D3DKMDT_COMPUTE_PREEMPTION_THREAD_BOUNDARY       = 400,
    D3DKMDT_COMPUTE_PREEMPTION_SHADER_BOUNDARY       = 500,
} D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY;

typedef struct _D3DKMDT_PREEMPTION_CAPS
{
    D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
    D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY  ComputePreemptionGranularity;
} D3DKMDT_PREEMPTION_CAPS;

typedef struct _D3DKMT_WDDM_1_2_CAPS
{
    D3DKMDT_PREEMPTION_CAPS PreemptionCaps;
    union
    {
        struct
        {
            UINT SupportNonVGA                       :  1;
            UINT SupportSmoothRotation               :  1;
            UINT SupportPerEngineTDR                 :  1;
            UINT SupportKernelModeCommandBuffer      :  1;
            UINT SupportCCD                          :  1;
            UINT SupportSoftwareDeviceBitmaps        :  1;
            UINT SupportGammaRamp                    :  1;
            UINT SupportHWCursor                     :  1;
            UINT SupportHWVSync                      :  1;
            UINT SupportSurpriseRemovalInHibernation :  1;
            UINT Reserved                            : 22;
        };
        UINT Value;
    };
} D3DKMT_WDDM_1_2_CAPS;

typedef struct _D3DKMT_WDDM_1_3_CAPS
{
    union
    {
        struct
        {
            UINT SupportMiracast               :  1;
            UINT IsHybridIntegratedGPU         :  1;
            UINT IsHybridDiscreteGPU           :  1;
            UINT SupportPowerManagementPStates :  1;
            UINT SupportVirtualModes           :  1;
            UINT SupportCrossAdapterResource   :  1;
            UINT Reserved                      : 26;
        };
        UINT Value;
    };
} D3DKMT_WDDM_1_3_CAPS;

typedef struct _D3DKMT_WDDM_2_0_CAPS
{
    union
    {
        struct
        {
            UINT Support64BitAtomics       :  1;
            UINT GpuMmuSupported           :  1;
            UINT IoMmuSupported            :  1;
            UINT FlipOverwriteSupported    :  1;
            UINT SupportContextlessPresent :  1;
            UINT SupportSurpriseRemoval    :  1;
            UINT Reserved                  : 26;
        };
        UINT Value;
    };
} D3DKMT_WDDM_2_0_CAPS;

typedef struct _D3DKMT_WDDM_2_7_CAPS
{
    union
    {
        struct
        {
            UINT HwSchSupported               :  1;
            UINT HwSchEnabled                 :  1;
            UINT HwSchEnabledByDefault        :  1;
            UINT IndependentVidPnVSyncControl :  1;
            UINT Reserved                     : 28;
        };
        UINT Value;
    };
} D3DKMT_WDDM_2_7_CAPS;

typedef struct _D3DKMT_WDDM_2_9_CAPS
{
    union
    {
        struct
        {
            UINT    HwSchSupportState          :  2;
            UINT    HwSchEnabled               :  1;
            UINT    SelfRefreshMemorySupported :  1;
            UINT    Reserved                   : 28;
        };
        UINT Value;
    };
} D3DKMT_WDDM_2_9_CAPS;

typedef struct _D3DKMT_WDDM_3_0_CAPS
{
    union
    {
        struct
        {
            UINT    HwFlipQueueSupportState :  2;
            UINT    HwFlipQueueEnabled      :  1;
            UINT    DisplayableSupported    :  1;
            UINT    Reserved                : 28;
        };
        UINT Value;
    };
} D3DKMT_WDDM_3_0_CAPS;

typedef struct _D3DKMT_WDDM_3_1_CAPS
{
    union
    {
        struct
        {
            UINT    NativeGpuFenceSupported :   1;
            UINT    Reserved                :  31;
        };
        UINT Value;
    };
} D3DKMT_WDDM_3_1_CAPS;

#endif /* __WINE_D3DKMDT_H */
