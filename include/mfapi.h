/*
 * Copyright (C) 2015 Austin English
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

#ifndef __WINE_MFAPI_H
#define __WINE_MFAPI_H

#include <mfobjects.h>
#include <mmreg.h>
#include <avrt.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(MF_VERSION)
/* Default to Windows XP */
#define MF_SDK_VERSION 0x0001
#define MF_API_VERSION 0x0070
#define MF_VERSION (MF_SDK_VERSION << 16 | MF_API_VERSION)
#endif

#define MFSTARTUP_NOSOCKET 0x1
#define MFSTARTUP_LITE     (MFSTARTUP_NOSOCKET)
#define MFSTARTUP_FULL     0

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

#ifndef DEFINE_MEDIATYPE_GUID
#define DEFINE_MEDIATYPE_GUID(name, format) \
    DEFINE_GUID(name, format, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
#endif

#ifndef DIRECT3D_VERSION
#define LOCAL_D3DFMT_DEFINES   1
#define D3DFMT_R8G8B8         20
#define D3DFMT_A8R8G8B8       21
#define D3DFMT_X8R8G8B8       22
#define D3DFMT_R5G6B5         23
#define D3DFMT_X1R5G5B5       24
#define D3DFMT_A2B10G10R10    31
#define D3DFMT_P8             41
#define D3DFMT_L8             50
#define D3DFMT_D16            80
#define D3DFMT_L16            81
#define D3DFMT_A16B16G16R16F 113
#endif

DEFINE_MEDIATYPE_GUID(MFVideoFormat_Base,          0);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB8,          D3DFMT_P8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB555,        D3DFMT_X1R5G5B5);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB565,        D3DFMT_R5G6B5);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB24,         D3DFMT_R8G8B8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB32,         D3DFMT_X8R8G8B8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ARGB32,        D3DFMT_A8R8G8B8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_A2R10G10B10,   D3DFMT_A2B10G10R10);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_A16B16G16R16F, D3DFMT_A16B16G16R16F);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_L8,            D3DFMT_L8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_L16,           D3DFMT_L16);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_D16,           D3DFMT_D16);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_420O,          MAKEFOURCC('4','2','0','O'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_DVSL,          MAKEFOURCC('d','v','s','l'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_DVSD,          MAKEFOURCC('d','v','s','d'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_DVHD,          MAKEFOURCC('d','v','h','d'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_DV25,          MAKEFOURCC('d','v','2','5'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_DV50,          MAKEFOURCC('d','v','5','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_DVH1,          MAKEFOURCC('d','v','h','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_DVC,           MAKEFOURCC('d','v','c',' '));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_v210,          MAKEFOURCC('v','2','1','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_v216,          MAKEFOURCC('v','2','1','6'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_v410,          MAKEFOURCC('v','4','1','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_AI44,          MAKEFOURCC('A','I','4','4'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_AV1,           MAKEFOURCC('A','V','0','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_AYUV,          MAKEFOURCC('A','Y','U','V'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_H263,          MAKEFOURCC('H','2','6','3'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_H264,          MAKEFOURCC('H','2','6','4'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_H265,          MAKEFOURCC('H','2','6','5'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_HEVC,          MAKEFOURCC('H','E','V','C'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_HEVC_ES,       MAKEFOURCC('H','E','V','S'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_I420,          MAKEFOURCC('I','4','2','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IYUV,          MAKEFOURCC('I','Y','U','V'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_M4S2,          MAKEFOURCC('M','4','S','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MJPG,          MAKEFOURCC('M','J','P','G'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MP43,          MAKEFOURCC('M','P','4','3'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MP4S,          MAKEFOURCC('M','P','4','S'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MP4V,          MAKEFOURCC('M','P','4','V'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MPG1,          MAKEFOURCC('M','P','G','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MSS1,          MAKEFOURCC('M','S','S','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MSS2,          MAKEFOURCC('M','S','S','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_NV11,          MAKEFOURCC('N','V','1','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_NV12,          MAKEFOURCC('N','V','1','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ORAW,          MAKEFOURCC('O','R','A','W'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_P010,          MAKEFOURCC('P','0','1','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_P016,          MAKEFOURCC('P','0','1','6'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_P210,          MAKEFOURCC('P','2','1','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_P216,          MAKEFOURCC('P','2','1','6'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_UYVY,          MAKEFOURCC('U','Y','V','Y'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP10,          MAKEFOURCC('V','P','1','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP80,          MAKEFOURCC('V','P','8','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VP90,          MAKEFOURCC('V','P','9','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_Y210,          MAKEFOURCC('Y','2','1','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_Y216,          MAKEFOURCC('Y','2','1','6'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_Y410,          MAKEFOURCC('Y','4','1','0'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_Y416,          MAKEFOURCC('Y','4','1','6'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_Y41P,          MAKEFOURCC('Y','4','1','P'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_Y41T,          MAKEFOURCC('Y','4','1','T'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_Y42T,          MAKEFOURCC('Y','4','2','T'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_YUY2,          MAKEFOURCC('Y','U','Y','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_YV12,          MAKEFOURCC('Y','V','1','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_YVYU,          MAKEFOURCC('Y','V','Y','U'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_YVU9,          MAKEFOURCC('Y','V','U','9'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_WMV1,          MAKEFOURCC('W','M','V','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_WMV2,          MAKEFOURCC('W','M','V','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_WMV3,          MAKEFOURCC('W','M','V','3'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_WVC1,          MAKEFOURCC('W','V','C','1'));

DEFINE_GUID(MFVideoFormat_Base_HDCP, 0xeac3b9d5, 0xbd14, 0x4237, 0x8f, 0x1f, 0xba, 0xb4, 0x28, 0xe4, 0x93, 0x12);
DEFINE_GUID(MFVideoFormat_H264_ES,   0x3f40f4f0, 0x5622, 0x4ff8, 0xb6, 0xd8, 0xa1, 0x7a, 0x58, 0x4b, 0xee, 0x5e);
DEFINE_GUID(MFVideoFormat_H264_HDCP, 0x5d0ce9dd, 0x9817, 0x49da, 0xbd, 0xfd, 0xf5, 0xf5, 0xb9, 0x8f, 0x18, 0xa6);
DEFINE_GUID(MFVideoFormat_HEVC_HDCP, 0x3cfe0fe6, 0x05c4, 0x47dc, 0x9d, 0x70, 0x4b, 0xdb, 0x29, 0x59, 0x72, 0x0f);
DEFINE_GUID(MFVideoFormat_MPEG2,     0xe06d8026, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea);

#define MFVideoFormat_MPG2 MFVideoFormat_MPEG2

#ifdef LOCAL_D3DFMT_DEFINES
#undef D3DFMT_R8G8B8
#undef D3DFMT_A8R8G8B8
#undef D3DFMT_X8R8G8B8
#undef D3DFMT_R5G6B5
#undef D3DFMT_X1R5G5B5
#undef D3DFMT_A2B10G10R10
#undef D3DFMT_P8
#undef D3DFMT_L8
#undef D3DFMT_D16
#undef D3DFMT_L16
#undef D3DFMT_A16B16G16R16F
#undef LOCAL_D3DFMT_DEFINES
#endif

DEFINE_MEDIATYPE_GUID(MFAudioFormat_Base,              0);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_AAC,               WAVE_FORMAT_MPEG_HEAAC);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_ADTS,              WAVE_FORMAT_MPEG_ADTS_AAC);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_ALAC,              WAVE_FORMAT_ALAC);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_AMR_NB,            WAVE_FORMAT_AMR_NB);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_AMR_WB,            WAVE_FORMAT_AMR_WB);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_AMR_WP,            WAVE_FORMAT_AMR_WP);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_Dolby_AC3_SPDIF,   WAVE_FORMAT_DOLBY_AC3_SPDIF);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_DRM,               WAVE_FORMAT_DRM);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_DTS,               WAVE_FORMAT_DTS);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_Float,             WAVE_FORMAT_IEEE_FLOAT);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_FLAC,              WAVE_FORMAT_FLAC);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_MP3,               WAVE_FORMAT_MPEGLAYER3);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_MPEG,              WAVE_FORMAT_MPEG);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_MSP1,              WAVE_FORMAT_WMAVOICE9);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_Opus,              WAVE_FORMAT_OPUS);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_PCM,               WAVE_FORMAT_PCM);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_WMAudioV8,         WAVE_FORMAT_WMAUDIO2);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_WMAudioV9,         WAVE_FORMAT_WMAUDIO3);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_WMAudio_Lossless,  WAVE_FORMAT_WMAUDIO_LOSSLESS);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_WMASPDIF,          WAVE_FORMAT_WMASPDIF);

DEFINE_GUID(MFAudioFormat_AAC_HDCP,             0x419bce76, 0x8b72, 0x400f, 0xad, 0xeb, 0x84, 0xb5, 0x7d, 0x63, 0x48, 0x4d);
DEFINE_GUID(MFAudioFormat_ADTS_HDCP,            0xda4963a3, 0x14d8, 0x4dcf, 0x92, 0xb7, 0x19, 0x3e, 0xb8, 0x43, 0x63, 0xdb);
DEFINE_GUID(MFAudioFormat_Base_HDCP,            0x3884b5bc, 0xe277, 0x43fd, 0x98, 0x3d, 0x03, 0x8a, 0xa8, 0xd9, 0xb6, 0x05);
DEFINE_GUID(MFAudioFormat_Dolby_AC3,            0xe06d802c, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea);
DEFINE_GUID(MFAudioFormat_Dolby_AC3_HDCP,       0x97663a80, 0x8ffb, 0x4445, 0xa6, 0xba, 0x79, 0x2d, 0x90, 0x8f, 0x49, 0x7f);
DEFINE_GUID(MFAudioFormat_Dolby_DDPlus,         0xa7fb87af, 0x2d02, 0x42fb, 0xa4, 0xd4, 0x05, 0xcd, 0x93, 0x84, 0x3b, 0xdd);
DEFINE_GUID(MFAudioFormat_Float_SpatialObjects, 0xfa39cd94, 0xbc64, 0x4ab1, 0x9b, 0x71, 0xdc, 0xd0, 0x9d, 0x5a, 0x7e, 0x7a);
DEFINE_GUID(MFAudioFormat_LPCM,                 0xe06d8032, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea);
DEFINE_GUID(MFAudioFormat_PCM_HDCP,             0xa5e7ff01, 0x8411, 0x4acc, 0xa8, 0x65, 0x5f, 0x49, 0x41, 0x28, 0x8d, 0x80);
DEFINE_GUID(MFAudioFormat_Vorbis,               0x8d2fd10b, 0x5841, 0x4a6b, 0x89, 0x05, 0x58, 0x8f, 0xec, 0x1a, 0xde, 0xd9);

DEFINE_GUID(MFImageFormat_JPEG,  0x19e4a5aa, 0x5662, 0x4fc5, 0xa0, 0xc0, 0x17, 0x58, 0x02, 0x8e, 0x10, 0x57);
DEFINE_GUID(MFImageFormat_RGB32, 0x00000016, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

DEFINE_GUID(MFStreamFormat_MPEG2Transport, 0xe06d8023, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea);
DEFINE_GUID(MFStreamFormat_MPEG2Program,   0x263067d1, 0xd330, 0x45dc, 0xb6, 0x69, 0x34, 0xd9, 0x86, 0xe4, 0xe3, 0xe1);

DEFINE_GUID(AM_MEDIA_TYPE_REPRESENTATION, 0xe2e42ad2, 0x132c, 0x491e, 0xa2, 0x68, 0x3c, 0x7c, 0x2d, 0xca, 0x18, 0x1f);
DEFINE_GUID(FORMAT_MFVideoFormat,         0xaed4ab2d, 0x7326, 0x43cb, 0x94, 0x64, 0xc8, 0x79, 0xca, 0xb9, 0xc4, 0x3d);

#if defined(__cplusplus) && !defined(CINTERFACE)
typedef struct tagMFASYNCRESULT : public IMFAsyncResult {
#else
typedef struct tagMFASYNCRESULT
{
    IMFAsyncResult AsyncResult;
#endif
    OVERLAPPED overlapped;
    IMFAsyncCallback *pCallback;
    HRESULT hrStatusResult;
    DWORD dwBytesTransferred;
    HANDLE hEvent;
} MFASYNCRESULT;

typedef enum _EAllocationType
{
    eAllocationTypeDynamic,
    eAllocationTypeRT,
    eAllocationTypePageable,
    eAllocationTypeIgnore
} EAllocationType;

#ifdef MF_INIT_GUIDS
#include <initguid.h>
#endif

DEFINE_GUID(MF_MT_AM_FORMAT_TYPE,                      0x73d1072d, 0x1870, 0x4174, 0xa0, 0x63, 0x29, 0xff, 0x4f, 0xf6, 0xc1, 0x1e);
DEFINE_GUID(MF_MT_AVG_BIT_ERROR_RATE,                  0x799cabd6, 0x3508, 0x4db4, 0xa3, 0xc7, 0x56, 0x9c, 0xd5, 0x33, 0xde, 0xb1);
DEFINE_GUID(MF_MT_AVG_BITRATE,                         0x20332624, 0xfb0d, 0x4d9e, 0xbd, 0x0d, 0xcb, 0xf6, 0x78, 0x6c, 0x10, 0x2e);
DEFINE_GUID(MF_MT_CUSTOM_VIDEO_PRIMARIES,              0x47537213, 0x8cfb, 0x4722, 0xaa, 0x34, 0xfb, 0xc9, 0xe2, 0x4d, 0x77, 0xb8);
DEFINE_GUID(MF_MT_DECODER_MAX_DPB_COUNT,               0x67be144c, 0x88b7, 0x4ca9, 0x96, 0x28, 0xc8, 0x08, 0xd5, 0x26, 0x22, 0x17);
DEFINE_GUID(MF_MT_DECODER_USE_MAX_RESOLUTION,          0x4c547c24, 0xaf9a, 0x4f38, 0x96, 0xad, 0x97, 0x87, 0x73, 0xcf, 0x53, 0xe7);
DEFINE_GUID(MF_MT_DRM_FLAGS,                           0x8772f323, 0x355a, 0x4cc7, 0xbb, 0x78, 0x6d, 0x61, 0xa0, 0x48, 0xae, 0x82);
DEFINE_GUID(MF_MT_FRAME_RATE,                          0xc459a2e8, 0x3d2c, 0x4e44, 0xb1, 0x32, 0xfe, 0xe5, 0x15, 0x6c, 0x7b, 0xb0);
DEFINE_GUID(MF_MT_FRAME_SIZE,                          0x1652c33d, 0xd6b2, 0x4012, 0xb8, 0x34, 0x72, 0x03, 0x08, 0x49, 0xa3, 0x7d);
DEFINE_GUID(MF_MT_GEOMETRIC_APERTURE,                  0x66758743, 0x7e5f, 0x400d, 0x98, 0x0a, 0xaa, 0x85, 0x96, 0xc8, 0x56, 0x96);
DEFINE_GUID(MF_MT_IMAGE_LOSS_TOLERANT,                 0xed062cf4, 0xe34e, 0x4922, 0xbe, 0x99, 0x93, 0x40, 0x32, 0x13, 0x3d, 0x7c);
DEFINE_GUID(MF_MT_INTERLACE_MODE,                      0xe2724bb8, 0xe676, 0x4806, 0xb4, 0xb2, 0xa8, 0xd6, 0xef, 0xb4, 0x4c, 0xcd);
DEFINE_GUID(MF_MT_MAJOR_TYPE,                          0x48eba18e, 0xf8c9, 0x4687, 0xbf, 0x11, 0x0a, 0x74, 0xc9, 0xf9, 0x6a, 0x8f);
DEFINE_GUID(MF_MT_MAX_FRAME_AVERAGE_LUMINANCE_LEVEL,   0x58d4bf57, 0x6f52, 0x4733, 0xa1, 0x95, 0xa9, 0xe2, 0x9e, 0xcf, 0x9e, 0x27);
DEFINE_GUID(MF_MT_MAX_KEYFRAME_SPACING,                0xc16eb52b, 0x73a1, 0x476f, 0x8d, 0x62, 0x83, 0x9d, 0x6a, 0x02, 0x06, 0x52);
DEFINE_GUID(MF_MT_MAX_LUMINANCE_LEVEL,                 0x50253128, 0xc110, 0x4de4, 0x98, 0xae, 0x46, 0xa3, 0x24, 0xfa, 0xe6, 0xda);
DEFINE_GUID(MF_MT_MAX_MASTERING_LUMINANCE,             0xd6c6b997, 0x272f, 0x4ca1, 0x8d, 0x00, 0x80, 0x42, 0x11, 0x1a, 0x0f, 0xf6);
DEFINE_GUID(MF_MT_MIN_MASTERING_LUMINANCE,             0x839a4460, 0x4e7e, 0x4b4f, 0xae, 0x79, 0xcc, 0x08, 0x90, 0x5c, 0x7b, 0x27);
DEFINE_GUID(MF_MT_MINIMUM_DISPLAY_APERTURE,            0xd7388766, 0x18fe, 0x48c6, 0xa1, 0x77, 0xee, 0x89, 0x48, 0x67, 0xc8, 0xc4);
DEFINE_GUID(MF_MT_MPEG4_SAMPLE_DESCRIPTION,            0x261e9d83, 0x9529, 0x4b8f, 0xa1, 0x11, 0x8b, 0x9c, 0x95, 0x0a, 0x81, 0xa9);
DEFINE_GUID(MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY,          0x9aa7e155, 0xb64a, 0x4c1d, 0xa5, 0x00, 0x45, 0x5d, 0x60, 0x0b, 0x65, 0x60);
DEFINE_GUID(MF_MT_PIXEL_ASPECT_RATIO,                  0xc6376a1e, 0x8d0a, 0x4027, 0xbe, 0x45, 0x6d, 0x9a, 0x0a, 0xd3, 0x9b, 0xb6);
DEFINE_GUID(MF_MT_REALTIME_CONTENT,                    0xbb12d222, 0x2bdb, 0x425e, 0x91, 0xec, 0x23, 0x08, 0xe1, 0x89, 0xa5, 0x8f);
DEFINE_GUID(MF_MT_OUTPUT_BUFFER_NUM,                   0xa505d3ac, 0xf930, 0x436e, 0x8e, 0xde, 0x93, 0xa5, 0x09, 0xce, 0x23, 0xb2);
DEFINE_GUID(MF_MT_SUBTYPE,                             0xf7e34c9a, 0x42e8, 0x4714, 0xb7, 0x4b, 0xcb, 0x29, 0xd7, 0x2c, 0x35, 0xe5);
DEFINE_GUID(MF_MT_ALL_SAMPLES_INDEPENDENT,             0xc9173739, 0x5e56, 0x461c, 0xb7, 0x13, 0x46, 0xfb, 0x99, 0x5c, 0xb9, 0x5f);
DEFINE_GUID(MF_MT_USER_DATA,                           0xb6bc765f, 0x4c3b, 0x40a4, 0xbd, 0x51, 0x25, 0x35, 0xb6, 0x6f, 0xe0, 0x9d);
DEFINE_GUID(MF_MT_FRAME_RATE_RANGE_MIN,                0xd2e7558c, 0xdc1f, 0x403f, 0x9a, 0x72, 0xd2, 0x8b, 0xb1, 0xeb, 0x3b, 0x5e);
DEFINE_GUID(MF_MT_FRAME_RATE_RANGE_MAX,                0xe3371d41, 0xb4cf, 0x4a05, 0xbd, 0x4e, 0x20, 0xb8, 0x8b, 0xb2, 0xc4, 0xd6);
DEFINE_GUID(MF_MT_WRAPPED_TYPE,                        0x4d3f7b23, 0xd02f, 0x4e6c, 0x9b, 0xee, 0xe4, 0xbf, 0x2c, 0x6c, 0x69, 0x5d);
DEFINE_GUID(MF_MT_FIXED_SIZE_SAMPLES,                  0xb8ebefaf, 0xb718, 0x4e04, 0xb0, 0xa9, 0x11, 0x67, 0x75, 0xe3, 0x32, 0x1b);
DEFINE_GUID(MF_MT_COMPRESSED,                          0x3afd0cee, 0x18f2, 0x4ba5, 0xa1, 0x10, 0x8b, 0xea, 0x50, 0x2e, 0x1f, 0x92);
DEFINE_GUID(MF_MT_SAMPLE_SIZE,                         0xdad3ab78, 0x1990, 0x408b, 0xbc, 0xe2, 0xeb, 0xa6, 0x73, 0xda, 0xcc, 0x10);
DEFINE_GUID(MF_MT_VIDEO_3D,                            0xcb5e88cf, 0x7b5b, 0x47b6, 0x85, 0xaa, 0x1c, 0xa5, 0xae, 0x18, 0x75, 0x55);
DEFINE_GUID(MF_MT_VIDEO_3D_FORMAT,                     0x5315d8a0, 0x87c5, 0x4697, 0xb7, 0x93, 0x66, 0x06, 0xc6, 0x7c, 0x04, 0x9b);
DEFINE_GUID(MF_MT_VIDEO_3D_NUM_VIEWS,                  0xbb077e8a, 0xdcbf, 0x42eb, 0xaf, 0x60, 0x41, 0x8d, 0xf9, 0x8a, 0xa4, 0x95);
DEFINE_GUID(MF_MT_VIDEO_3D_LEFT_IS_BASE,               0x6d4b7bff, 0x5629, 0x4404, 0x94, 0x8c, 0xc6, 0x34, 0xf4, 0xce, 0x26, 0xd4);
DEFINE_GUID(MF_MT_VIDEO_3D_FIRST_IS_LEFT,              0xec298493, 0x0ada, 0x4ea1, 0xa4, 0xfe, 0xcb, 0xbd, 0x36, 0xce, 0x93, 0x31);
DEFINE_GUID(MF_MT_VIDEO_CHROMA_SITING,                 0x65df2370, 0xc773, 0x4c33, 0xaa, 0x64, 0x84, 0x3e, 0x06, 0x8e, 0xfb, 0x0c);
DEFINE_GUID(MF_MT_VIDEO_LEVEL,                         0x96f66574, 0x11c5, 0x4015, 0x86, 0x66, 0xbf, 0xf5, 0x16, 0x43, 0x6d, 0xa7);
DEFINE_GUID(MF_MT_VIDEO_LIGHTING,                      0x53a0529c, 0x890b, 0x4216, 0x8b, 0xf9, 0x59, 0x93, 0x67, 0xad, 0x6d, 0x20);
DEFINE_GUID(MF_MT_VIDEO_NOMINAL_RANGE,                 0xc21b8ee5, 0xb956, 0x4071, 0x8d, 0xaf, 0x32, 0x5e, 0xdf, 0x5c, 0xab, 0x11);
DEFINE_GUID(MF_MT_VIDEO_PRIMARIES,                     0xdbfbe4d7, 0x0740, 0x4ee0, 0x81, 0x92, 0x85, 0x0a, 0xb0, 0xe2, 0x19, 0x35);
DEFINE_GUID(MF_MT_VIDEO_PROFILE,                       0xad76a80b, 0x2d5c, 0x4e0b, 0xb3, 0x75, 0x64, 0xe5, 0x20, 0x13, 0x70, 0x36);
DEFINE_GUID(MF_MT_VIDEO_ROTATION,                      0xc380465d, 0x2271, 0x428c, 0x9b, 0x83, 0xec, 0xea, 0x3b, 0x4a, 0x85, 0xc1);
DEFINE_GUID(MF_MT_VIDEO_RENDERER_EXTENSION_PROFILE,    0x8437d4b9, 0xd448, 0x4fcd, 0x9b, 0x6b, 0x83, 0x9b, 0xf9, 0x6c, 0x77, 0x98);
DEFINE_GUID(MF_MT_VIDEO_NO_FRAME_ORDERING,             0x3f5b106f, 0x6bc2, 0x4ee3, 0xb7, 0xed, 0x89, 0x02, 0xc1, 0x8f, 0x53, 0x51);
DEFINE_GUID(MF_MT_VIDEO_H264_NO_FMOASO,                0xed461cd6, 0xec9f, 0x416a, 0xa8, 0xa3, 0x26, 0xd7, 0xd3, 0x10, 0x18, 0xd7);
DEFINE_GUID(MF_MT_PAD_CONTROL_FLAGS,                   0x4d0e73e5, 0x80ea, 0x4354, 0xa9, 0xd0, 0x11, 0x76, 0xce, 0xb0, 0x28, 0xea);
DEFINE_GUID(MF_MT_PAN_SCAN_APERTURE,                   0x79614dde, 0x9187, 0x48fb, 0xb8, 0xc7, 0x4d, 0x52, 0x68, 0x9d, 0xe6, 0x49);
DEFINE_GUID(MF_MT_PAN_SCAN_ENABLED,                    0x4b7f6bc3, 0x8b13, 0x40b2, 0xa9, 0x93, 0xab, 0xf6, 0x30, 0xb8, 0x20, 0x4e);
DEFINE_GUID(MF_MT_SECURE,                              0xc5acc4fd, 0x0304, 0x4ecf, 0x80, 0x9f, 0x47, 0xbc, 0x97, 0xff, 0x63, 0xbd);
DEFINE_GUID(MF_MT_SOURCE_CONTENT_HINT,                 0x68aca3cc, 0x22d0, 0x44e6, 0x85, 0xf8, 0x28, 0x16, 0x71, 0x97, 0xfa, 0x38);
DEFINE_GUID(MF_MT_TIMESTAMP_CAN_BE_DTS,                0x24974215, 0x1b7b, 0x41e4, 0x86, 0x25, 0xac, 0x46, 0x9f, 0x2d, 0xed, 0xaa);
DEFINE_GUID(MF_MT_TRANSFER_FUNCTION,                   0x5fb0fce9, 0xbe5c, 0x4935, 0xa8, 0x11, 0xec, 0x83, 0x8f, 0x8e, 0xed, 0x93);
DEFINE_GUID(MF_MT_ALPHA_MODE,                          0x5d959b0d, 0x4cbf, 0x4d04, 0x91, 0x9f, 0x3f, 0x5f, 0x7f, 0x28, 0x42, 0x11);
DEFINE_GUID(MF_MT_DEPTH_MEASUREMENT,                   0xfd5ac489, 0x0917, 0x4bb6, 0x9d, 0x54, 0x31, 0x22, 0xbf, 0x70, 0x14, 0x4b);
DEFINE_GUID(MF_MT_DEPTH_VALUE_UNIT,                    0x21a800f5, 0x3189, 0x4797, 0xbe, 0xba, 0xf1, 0x3c, 0xd9, 0xa3, 0x1a, 0x5e);
DEFINE_GUID(MF_MT_FORWARD_CUSTOM_NALU,                 0xed336efd, 0x244f, 0x428d, 0x91, 0x53, 0x28, 0xf3, 0x99, 0x45, 0x88, 0x90);
DEFINE_GUID(MF_MT_FORWARD_CUSTOM_SEI,                  0xe27362f1, 0xb136, 0x41d1, 0x95, 0x94, 0x3a, 0x7e, 0x4f, 0xeb, 0xf2, 0xd1);
DEFINE_GUID(MF_MT_AUDIO_NUM_CHANNELS,                  0x37e48bf5, 0x645e, 0x4c5b, 0x89, 0xde, 0xad, 0xa9, 0xe2, 0x9b, 0x69, 0x6a);
DEFINE_GUID(MF_MT_AUDIO_SAMPLES_PER_SECOND,            0x5faeeae7, 0x0290, 0x4c31, 0x9e, 0x8a, 0xc5, 0x34, 0xf6, 0x8d, 0x9d, 0xba);
DEFINE_GUID(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND,      0xfb3b724a, 0xcfb5, 0x4319, 0xae, 0xfe, 0x6e, 0x42, 0xb2, 0x40, 0x61, 0x32);
DEFINE_GUID(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,          0x1aab75c8, 0xcfef, 0x451c, 0xab, 0x95, 0xac, 0x03, 0x4b, 0x8e, 0x17, 0x31);
DEFINE_GUID(MF_MT_AUDIO_BLOCK_ALIGNMENT,               0x322de230, 0x9eeb, 0x43bd, 0xab, 0x7a, 0xff, 0x41, 0x22, 0x51, 0x54, 0x1d);
DEFINE_GUID(MF_MT_AUDIO_BITS_PER_SAMPLE,               0xf2deb57f, 0x40fa, 0x4764, 0xaa, 0x33, 0xed, 0x4f, 0x2d, 0x1f, 0xf6, 0x69);
DEFINE_GUID(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE,         0xd9bf8d6a, 0x9530, 0x4b7c, 0x9d, 0xdf, 0xff, 0x6f, 0xd5, 0x8b, 0xbd, 0x06);
DEFINE_GUID(MF_MT_AUDIO_SAMPLES_PER_BLOCK,             0xaab15aac, 0xe13a, 0x4995, 0x92, 0x22, 0x50, 0x1e, 0xa1, 0x5c, 0x68, 0x77);
DEFINE_GUID(MF_MT_AUDIO_CHANNEL_MASK,                  0x55fb5765, 0x644a, 0x4caf, 0x84, 0x79, 0x93, 0x89, 0x83, 0xbb, 0x15, 0x88);
DEFINE_GUID(MF_MT_AUDIO_FOLDDOWN_MATRIX,               0x9d62927c, 0x36be, 0x4cf1, 0xb5, 0xc4, 0xa3, 0x92, 0x6e, 0x3e, 0x87, 0x11);
DEFINE_GUID(MF_MT_AUDIO_WMADRC_PEAKREF,                0x9d62927d, 0x36be, 0x4cf2, 0xb5, 0xc4, 0xa3, 0x92, 0x6e, 0x3e, 0x87, 0x11);
DEFINE_GUID(MF_MT_AUDIO_WMADRC_PEAKTARGET,             0x9d62927e, 0x36be, 0x4cf2, 0xb5, 0xc4, 0xa3, 0x92, 0x6e, 0x3e, 0x87, 0x11);
DEFINE_GUID(MF_MT_AUDIO_WMADRC_AVGREF,                 0x9d62927f, 0x36be, 0x4cf2, 0xb5, 0xc4, 0xa3, 0x92, 0x6e, 0x3e, 0x87, 0x11);
DEFINE_GUID(MF_MT_AUDIO_WMADRC_AVGTARGET,              0x9d629280, 0x36be, 0x4cf2, 0xb5, 0xc4, 0xa3, 0x92, 0x6e, 0x3e, 0x87, 0x11);
DEFINE_GUID(MF_MT_AUDIO_PREFER_WAVEFORMATEX,           0xa901aaba, 0xe037, 0x458a, 0xbd, 0xf6, 0x54, 0x5b, 0xe2, 0x07, 0x40, 0x42);
DEFINE_GUID(MF_MT_AUDIO_FLAC_MAX_BLOCK_SIZE,           0x8b81adae, 0x4b5a, 0x4d40, 0x80, 0x22, 0xf3, 0x8d, 0x09, 0xca, 0x3c, 0x5c);
DEFINE_GUID(MF_MT_ORIGINAL_4CC,                        0xd7be3fe0, 0x2bc7, 0x492d, 0xb8, 0x43, 0x61, 0xa1, 0x91, 0x9b, 0x70, 0xc3);
DEFINE_GUID(MF_MT_ORIGINAL_WAVE_FORMAT_TAG,            0x8cbbc843, 0x9fd9, 0x49c2, 0x88, 0x2f, 0xa7, 0x25, 0x86, 0xc4, 0x08, 0xad);
DEFINE_GUID(MF_MT_DEFAULT_STRIDE,                      0x644b4e48, 0x1e02, 0x4516, 0xb0, 0xeb, 0xc0, 0x1c, 0xa9, 0xd4, 0x9a, 0xc6);
DEFINE_GUID(MF_MT_PALETTE,                             0x6d283f42, 0x9846, 0x4410, 0xaf, 0xd9, 0x65, 0x4d, 0x50, 0x3b, 0x1a, 0x54);
DEFINE_GUID(MF_MT_YUV_MATRIX,                          0x3e23d450, 0x2c75, 0x4d25, 0xa0, 0x0e, 0xb9, 0x16, 0x70, 0xd1, 0x23, 0x27);

DEFINE_GUID(MF_MT_SPATIAL_AUDIO_MAX_DYNAMIC_OBJECTS,              0xdcfba24a, 0x2609, 0x4240, 0xa7, 0x21, 0x3f, 0xae, 0xa7, 0x6a, 0x4d, 0xf9);
DEFINE_GUID(MF_MT_SPATIAL_AUDIO_OBJECT_METADATA_FORMAT_ID,        0x2ab71bc0, 0x6223, 0x4ba7, 0xad, 0x64, 0x7b, 0x94, 0xb4, 0x7a, 0xe7, 0x92);
DEFINE_GUID(MF_MT_SPATIAL_AUDIO_OBJECT_METADATA_LENGTH,           0x094ba8be, 0xd723, 0x489f, 0x92, 0xfa, 0x76, 0x67, 0x77, 0xb3, 0x47, 0x26);
DEFINE_GUID(MF_MT_SPATIAL_AUDIO_MAX_METADATA_ITEMS,               0x11aa80b4, 0xe0da, 0x47c6, 0x80, 0x60, 0x96, 0xc1, 0x25, 0x9a, 0xe5, 0x0d);
DEFINE_GUID(MF_MT_SPATIAL_AUDIO_MIN_METADATA_ITEM_OFFSET_SPACING, 0x83e96ec9, 0x1184, 0x417e, 0x82, 0x54, 0x9f, 0x26, 0x91, 0x58, 0xfc, 0x06);
DEFINE_GUID(MF_MT_SPATIAL_AUDIO_DATA_PRESENT,                     0x6842f6e7, 0xd43e, 0x4ebb, 0x9c, 0x9c, 0xc9, 0x6f, 0x41, 0x78, 0x48, 0x63);

DEFINE_GUID(MF_MT_AAC_PAYLOAD_TYPE,                    0xbfbabe79, 0x7434, 0x4d1c, 0x94, 0xf0, 0x72, 0xa3, 0xb9, 0xe1, 0x71, 0x88);
DEFINE_GUID(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION,  0x7632f0e6, 0x9538, 0x4d61, 0xac, 0xda, 0xea, 0x29, 0xc8, 0xc1, 0x44, 0x56);

DEFINE_GUID(MF_MT_MPEG_START_TIME_CODE,                0x91f67885, 0x4333, 0x4280, 0x97, 0xcd, 0xbd, 0x5a, 0x6c, 0x03, 0xa0, 0x6e);
DEFINE_GUID(MF_MT_MPEG2_PROFILE,                       0xad76a80b, 0x2d5c, 0x4e0b, 0xb3, 0x75, 0x64, 0xe5, 0x20, 0x13, 0x70, 0x36);
DEFINE_GUID(MF_MT_MPEG2_LEVEL,                         0x96f66574, 0x11c5, 0x4015, 0x86, 0x66, 0xbf, 0xf5, 0x16, 0x43, 0x6d, 0xa7);
DEFINE_GUID(MF_MT_MPEG2_FLAGS,                         0x31e3991d, 0xf701, 0x4b2f, 0xb4, 0x26, 0x8a, 0xe3, 0xbd, 0xa9, 0xe0, 0x4b);
DEFINE_GUID(MF_MT_MPEG_SEQUENCE_HEADER,                0x3c036de7, 0x3ad0, 0x4c9e, 0x92, 0x16, 0xee, 0x6d, 0x6a, 0xc2, 0x1c, 0xb3);
DEFINE_GUID(MF_MT_MPEG2_STANDARD,                      0xa20af9e8, 0x928a, 0x4b26, 0xaa, 0xa9, 0xf0, 0x5c, 0x74, 0xca, 0xc4, 0x7c);
DEFINE_GUID(MF_MT_MPEG2_TIMECODE,                      0x5229ba10, 0xe29d, 0x4f80, 0xa5, 0x9c, 0xdf, 0x4f, 0x18, 0x02, 0x07, 0xd2);
DEFINE_GUID(MF_MT_MPEG2_CONTENT_PACKET,                0x825d55e4, 0x4f12, 0x4197, 0x9e, 0xb3, 0x59, 0xb6, 0xe4, 0x71, 0x0f, 0x06);
DEFINE_GUID(MF_MT_MPEG2_ONE_FRAME_PER_PACKET,          0x91a49eb5, 0x1d20, 0x4b42, 0xac, 0xe8, 0x80, 0x42, 0x69, 0xbf, 0x95, 0xed);
DEFINE_GUID(MF_MT_MPEG2_HDCP,                          0x168f1b4a, 0x3e91, 0x450f, 0xae, 0xa7, 0xe4, 0xba, 0xea, 0xda, 0xe5, 0xba);

DEFINE_GUID(MF_MT_H264_MAX_CODEC_CONFIG_DELAY,         0xf5929986, 0x4c45, 0x4fbb, 0xbb, 0x49, 0x6c, 0xc5, 0x34, 0xd0, 0x5b, 0x9b);
DEFINE_GUID(MF_MT_H264_SUPPORTED_SLICE_MODES,          0xc8be1937, 0x4d64, 0x4549, 0x83, 0x43, 0xa8, 0x08, 0x6c, 0x0b, 0xfd, 0xa5);
DEFINE_GUID(MF_MT_H264_SUPPORTED_SYNC_FRAME_TYPES,     0x89a52c01, 0xf282, 0x48d2, 0xb5, 0x22, 0x22, 0xe6, 0xae, 0x63, 0x31, 0x99);
DEFINE_GUID(MF_MT_H264_RESOLUTION_SCALING,             0xe3854272, 0xf715, 0x4757, 0xba, 0x90, 0x1b, 0x69, 0x6c, 0x77, 0x34, 0x57);
DEFINE_GUID(MF_MT_H264_SIMULCAST_SUPPORT,              0x9ea2d63d, 0x53f0, 0x4a34, 0xb9, 0x4e, 0x9d, 0xe4, 0x9a, 0x07, 0x8c, 0xb3);
DEFINE_GUID(MF_MT_H264_SUPPORTED_RATE_CONTROL_MODES,   0x6a8ac47e, 0x519c, 0x4f18, 0x9b, 0xb3, 0x7e, 0xea, 0xae, 0xa5, 0x59, 0x4d);
DEFINE_GUID(MF_MT_H264_MAX_MB_PER_SEC,                 0x45256d30, 0x7215, 0x4576, 0x93, 0x36, 0xb0, 0xf1, 0xbc, 0xd5, 0x9b, 0xb2);
DEFINE_GUID(MF_MT_H264_SUPPORTED_USAGES,               0x60b1a998, 0xdc01, 0x40ce, 0x97, 0x36, 0xab, 0xa8, 0x45, 0xa2, 0xdb, 0xdc);
DEFINE_GUID(MF_MT_H264_CAPABILITIES,                   0xbb3bd508, 0x490a, 0x11e0, 0x99, 0xe4, 0x13, 0x16, 0xdf, 0xd7, 0x20, 0x85);
DEFINE_GUID(MF_MT_H264_SVC_CAPABILITIES,               0xf8993abe, 0xd937, 0x4a8f, 0xbb, 0xca, 0x69, 0x66, 0xfe, 0x9e, 0x11, 0x52);

DEFINE_GUID(MF_MT_H264_USAGE,             0x359ce3a5, 0xaf00, 0x49ca, 0xa2, 0xf4, 0x2a, 0xc9, 0x4c, 0xa8, 0x2b, 0x61);
DEFINE_GUID(MF_MT_H264_RATE_CONTROL_MODES,0x705177d8, 0x45cb, 0x11e0, 0xac, 0x7d, 0xb9, 0x1c, 0xe0, 0xd7, 0x20, 0x85);
DEFINE_GUID(MF_MT_H264_LAYOUT_PER_STREAM, 0x85e299b2, 0x90e3, 0x4fe8, 0xb2, 0xf5, 0xc0, 0x67, 0xe0, 0xbf, 0xe5, 0x7a);
DEFINE_GUID(MF_MT_IN_BAND_PARAMETER_SET,  0x75da5090, 0x910b, 0x4a03, 0x89, 0x6c, 0x7b, 0x89, 0x8f, 0xee, 0xa5, 0xaf);
DEFINE_GUID(MF_MT_MPEG4_TRACK_TYPE,       0x54f486dd, 0x9327, 0x4f6d, 0x80, 0xab, 0x6f, 0x70, 0x9e, 0xbb, 0x4c, 0xce);

DEFINE_GUID(MF_MT_DV_AAUX_SRC_PACK_0,     0x84bd5d88, 0x0fb8, 0x4ac8, 0xbe, 0x4b, 0xa8, 0x84, 0x8b, 0xef, 0x98, 0xf3);
DEFINE_GUID(MF_MT_DV_AAUX_CTRL_PACK_0,    0xf731004e, 0x1dd1, 0x4515, 0xaa, 0xbe, 0xf0, 0xc0, 0x6a, 0xa5, 0x36, 0xac);
DEFINE_GUID(MF_MT_DV_AAUX_SRC_PACK_1,     0x720e6544, 0x0225, 0x4003, 0xa6, 0x51, 0x01, 0x96, 0x56, 0x3a, 0x95, 0x8e);
DEFINE_GUID(MF_MT_DV_AAUX_CTRL_PACK_1,    0xcd1f470d, 0x1f04, 0x4fe0, 0xbf, 0xb9, 0xd0, 0x7a, 0xe0, 0x38, 0x6a, 0xd8);
DEFINE_GUID(MF_MT_DV_VAUX_SRC_PACK,       0x41402d9d, 0x7b57, 0x43c6, 0xb1, 0x29, 0x2c, 0xb9, 0x97, 0xf1, 0x50, 0x09);
DEFINE_GUID(MF_MT_DV_VAUX_CTRL_PACK,      0x2f84e1c4, 0x0da1, 0x4788, 0x93, 0x8e, 0x0d, 0xfb, 0xfb, 0xb3, 0x4b, 0x48);

DEFINE_GUID(MF_MT_ARBITRARY_HEADER,       0x9e6bd6f5, 0x0109, 0x4f95, 0x84, 0xac, 0x93, 0x09, 0x15, 0x3a, 0x19, 0xfc);
DEFINE_GUID(MF_MT_ARBITRARY_FORMAT,       0x5a75b249, 0x0d7d, 0x49a1, 0xa1, 0xc3, 0xe0, 0xd8, 0x7f, 0x0c, 0xad, 0xe5);

DEFINE_GUID(MFT_CATEGORY_VIDEO_DECODER,   0xd6c02d4b, 0x6833, 0x45b4, 0x97, 0x1a, 0x05, 0xa4, 0xb0, 0x4b, 0xab, 0x91);
DEFINE_GUID(MFT_CATEGORY_VIDEO_ENCODER,   0xf79eac7d, 0xe545, 0x4387, 0xbd, 0xee, 0xd6, 0x47, 0xd7, 0xbd, 0xe4, 0x2a);
DEFINE_GUID(MFT_CATEGORY_VIDEO_EFFECT,    0x12e17c21, 0x532c, 0x4a6e, 0x8a, 0x1c, 0x40, 0x82, 0x5a, 0x73, 0x63, 0x97);
DEFINE_GUID(MFT_CATEGORY_MULTIPLEXER,     0x059c561e, 0x05ae, 0x4b61, 0xb6, 0x9d, 0x55, 0xb6, 0x1e, 0xe5, 0x4a, 0x7b);
DEFINE_GUID(MFT_CATEGORY_DEMULTIPLEXER,   0xa8700a7a, 0x939b, 0x44c5, 0x99, 0xd7, 0x76, 0x22, 0x6b, 0x23, 0xb3, 0xf1);
DEFINE_GUID(MFT_CATEGORY_AUDIO_DECODER,   0x9ea73fb4, 0xef7a, 0x4559, 0x8d, 0x5d, 0x71, 0x9d, 0x8f, 0x04, 0x26, 0xc7);
DEFINE_GUID(MFT_CATEGORY_AUDIO_ENCODER,   0x91c64bd0, 0xf91e, 0x4d8c, 0x92, 0x76, 0xdb, 0x24, 0x82, 0x79, 0xd9, 0x75);
DEFINE_GUID(MFT_CATEGORY_AUDIO_EFFECT,    0x11064c48, 0x3648, 0x4ed0, 0x93, 0x2e, 0x05, 0xce, 0x8a, 0xc8, 0x11, 0xb7);
DEFINE_GUID(MFT_CATEGORY_VIDEO_PROCESSOR, 0x302ea3fc, 0xaa5f, 0x47f9, 0x9f, 0x7a, 0xc2, 0x18, 0x8b, 0xb1, 0x63, 0x02);
DEFINE_GUID(MFT_CATEGORY_OTHER,           0x90175d57, 0xb7ea, 0x4901, 0xae, 0xb3, 0x93, 0x3a, 0x87, 0x47, 0x75, 0x6f);
DEFINE_GUID(MFT_ENUM_ADAPTER_LUID,        0x1d39518c, 0xe220, 0x4da8, 0xa0, 0x7f, 0xba, 0x17, 0x25, 0x52, 0xd6, 0xb1);
DEFINE_GUID(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, 0x53476a11, 0x3f13, 0x49fb, 0xac, 0x42, 0xee, 0x27, 0x33, 0xc9, 0x67, 0x41);

DEFINE_GUID(MFMediaType_Audio,             0x73647561, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(MFMediaType_Binary,            0x72178c25, 0xe45b, 0x11d5, 0xbc, 0x2a, 0x00, 0xb0, 0xd0, 0xf3, 0xf4, 0xab);
DEFINE_GUID(MFMediaType_Default,           0x81a412e6, 0x8103, 0x4b06, 0x85, 0x7f, 0x18, 0x62, 0x78, 0x10, 0x24, 0xac);
DEFINE_GUID(MFMediaType_FileTransfer,      0x72178c26, 0xe45b, 0x11d5, 0xbc, 0x2a, 0x00, 0xb0, 0xd0, 0xf3, 0xf4, 0xab);
DEFINE_GUID(MFMediaType_HTML,              0x72178c24, 0xe45b, 0x11d5, 0xbc, 0x2a, 0x00, 0xb0, 0xd0, 0xf3, 0xf4, 0xab);
DEFINE_GUID(MFMediaType_Image,             0x72178c23, 0xe45b, 0x11d5, 0xbc, 0x2a, 0x00, 0xb0, 0xd0, 0xf3, 0xf4, 0xab);
DEFINE_GUID(MFMediaType_MultiplexedFrames, 0x6ea542b0, 0x281f, 0x4231, 0xa4, 0x64, 0xfe, 0x2f, 0x50, 0x22, 0x50, 0x1c);
DEFINE_GUID(MFMediaType_Perception,        0x597ff6f9, 0x6ea2, 0x4670, 0x85, 0xb4, 0xea, 0x84, 0x07, 0x3f, 0xe9, 0x40);
DEFINE_GUID(MFMediaType_Protected,         0x7b4b6fe6, 0x9d04, 0x4494, 0xbe, 0x14, 0x7e, 0x0b, 0xd0, 0x76, 0xc8, 0xe4);
DEFINE_GUID(MFMediaType_SAMI,              0xe69669a0, 0x3dcd, 0x40cb, 0x9e, 0x2e, 0x37, 0x08, 0x38, 0x7c, 0x06, 0x16);
DEFINE_GUID(MFMediaType_Script,            0x72178c22, 0xe45b, 0x11d5, 0xbc, 0x2a, 0x00, 0xb0, 0xd0, 0xf3, 0xf4, 0xab);
DEFINE_GUID(MFMediaType_Stream,            0xe436eb83, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID(MFMediaType_Subtitle,          0xa6d13581, 0xed50, 0x4e65, 0xae, 0x08, 0x26, 0x06, 0x55, 0x76, 0xaa, 0xcc);
DEFINE_GUID(MFMediaType_Video,             0x73646976, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

DEFINE_GUID(MFSampleExtension_DecodeTimestamp,          0x73a954d4, 0x09e2, 0x4861, 0xbe, 0xfc, 0x94, 0xbd, 0x97, 0xc0, 0x8e, 0x6e);
DEFINE_GUID(MFSampleExtension_Timestamp,                0x1e436999, 0x69be, 0x4c7a, 0x93, 0x69, 0x70, 0x06, 0x8c, 0x02, 0x60, 0xcb);
DEFINE_GUID(MFSampleExtension_Token,                    0x8294da66, 0xf328, 0x4805, 0xb5, 0x51, 0x00, 0xde, 0xb4, 0xc5, 0x7a, 0x61);
DEFINE_GUID(MFSampleExtension_3DVideo,                  0xf86f97a4, 0xdd54, 0x4e2e, 0x9a, 0x5e, 0x55, 0xfc, 0x2d, 0x74, 0xa0, 0x05);
DEFINE_GUID(MFSampleExtension_3DVideo_SampleFormat,     0x08671772, 0xe36f, 0x4cff, 0x97, 0xb3, 0xd7, 0x2e, 0x20, 0x98, 0x7a, 0x48);
DEFINE_GUID(MFSampleExtension_TargetGlobalLuminance,    0x3f60ef36, 0x31ef, 0x4daf, 0x83, 0x60, 0x94, 0x03, 0x97, 0xe4, 0x1e, 0xf3);
DEFINE_GUID(MFSampleExtension_ForwardedDecodeUnits,     0x424c754c, 0x97c8, 0x48d6, 0x87, 0x77, 0xfc, 0x41, 0xf7, 0xb6, 0x08, 0x79);
DEFINE_GUID(MFSampleExtension_ForwardedDecodeUnitType,  0x089e57c7, 0x47d3, 0x4a26, 0xbf, 0x9c, 0x4b, 0x64, 0xfa, 0xfb, 0x5d, 0x1e);

DEFINE_GUID(MF_EVENT_DO_THINNING,                       0x321ea6fb, 0xdad9, 0x46e4, 0xb3, 0x1d, 0xd2, 0xea, 0xe7, 0x09, 0x0e, 0x30);
DEFINE_GUID(MF_EVENT_MFT_CONTEXT,                       0xb7cd31f1, 0x899e, 0x4b41, 0x80, 0xc9, 0x26, 0xa8, 0x96, 0xd3, 0x29, 0x77);
DEFINE_GUID(MF_EVENT_MFT_INPUT_STREAM_ID,               0xf29c2cca, 0x7ae6, 0x42d2, 0xb2, 0x84, 0xbf, 0x83, 0x7c, 0xc8, 0x74, 0xe2);
DEFINE_GUID(MF_EVENT_PRESENTATION_TIME_OFFSET,          0x5ad914d1, 0x9b45, 0x4a8d, 0xa2, 0xc0, 0x81, 0xd1, 0xe5, 0x0b, 0xfb, 0x07);
DEFINE_GUID(MF_EVENT_SCRUBSAMPLE_TIME,                  0x9ac712b3, 0xdcb8, 0x44d5, 0x8d, 0x0c, 0x37, 0x45, 0x5a, 0x27, 0x82, 0xe3);
DEFINE_GUID(MF_EVENT_SESSIONCAPS,                       0x7e5ebcd0, 0x11b8, 0x4abe, 0xaf, 0xad, 0x10, 0xf6, 0x59, 0x9a, 0x7f, 0x42);
DEFINE_GUID(MF_EVENT_SESSIONCAPS_DELTA,                 0x7e5ebcd1, 0x11b8, 0x4abe, 0xaf, 0xad, 0x10, 0xf6, 0x59, 0x9a, 0x7f, 0x42);
DEFINE_GUID(MF_EVENT_SOURCE_ACTUAL_START,               0xa8cc55a9, 0x6b31, 0x419f, 0x84, 0x5d, 0xff, 0xb3, 0x51, 0xa2, 0x43, 0x4b);
DEFINE_GUID(MF_EVENT_SOURCE_CHARACTERISTICS,            0x47db8490, 0x8b22, 0x4f52, 0xaf, 0xda, 0x9c, 0xe1, 0xb2, 0xd3, 0xcf, 0xa8);
DEFINE_GUID(MF_EVENT_SOURCE_CHARACTERISTICS_OLD,        0x47db8491, 0x8b22, 0x4f52, 0xaf, 0xda, 0x9c, 0xe1, 0xb2, 0xd3, 0xcf, 0xa8);
DEFINE_GUID(MF_EVENT_SOURCE_FAKE_START,                 0xa8cc55a7, 0x6b31, 0x419f, 0x84, 0x5d, 0xff, 0xb3, 0x51, 0xa2, 0x43, 0x4b);
DEFINE_GUID(MF_EVENT_SOURCE_PROJECTSTART,               0xa8cc55a8, 0x6b31, 0x419f, 0x84, 0x5d, 0xff, 0xb3, 0x51, 0xa2, 0x43, 0x4b);
DEFINE_GUID(MF_EVENT_SOURCE_TOPOLOGY_CANCELED,          0xdb62f650, 0x9a5e, 0x4704, 0xac, 0xf3, 0x56, 0x3b, 0xc6, 0xa7, 0x33, 0x64);
DEFINE_GUID(MF_EVENT_START_PRESENTATION_TIME,           0x5ad914d0, 0x9b45, 0x4a8d, 0xa2, 0xc0, 0x81, 0xd1, 0xe5, 0x0b, 0xfb, 0x07);
DEFINE_GUID(MF_EVENT_START_PRESENTATION_TIME_AT_OUTPUT, 0x5ad914d2, 0x9b45, 0x4a8d, 0xa2, 0xc0, 0x81, 0xd1, 0xe5, 0x0b, 0xfb, 0x07);
DEFINE_GUID(MF_EVENT_STREAM_METADATA_CONTENT_KEYIDS,    0x5063449d, 0xcc29, 0x4fc6, 0xa7, 0x5a, 0xd2, 0x47, 0xb3, 0x5a, 0xf8, 0x5c);
DEFINE_GUID(MF_EVENT_STREAM_METADATA_KEYDATA,           0xcd59a4a1, 0x4a3b, 0x4bbd, 0x86, 0x65, 0x72, 0xa4, 0x0f, 0xbe, 0xa7, 0x76);
DEFINE_GUID(MF_EVENT_STREAM_METADATA_SYSTEMID,          0x1ea2ef64, 0xba16, 0x4a36, 0x87, 0x19, 0xfe, 0x75, 0x60, 0xba, 0x32, 0xad);
DEFINE_GUID(MF_EVENT_TOPOLOGY_STATUS,                   0x30c5018d, 0x9a53, 0x454b, 0xad, 0x9e, 0x6d, 0x5f, 0x8f, 0xa7, 0xc4, 0x3b);
DEFINE_GUID(MF_EVENT_OUTPUT_NODE,                       0x830f1a8b, 0xc060, 0x46dd, 0xa8, 0x01, 0x1c, 0x95, 0xde, 0xc9, 0xb1, 0x07);

DEFINE_GUID(MF_LOW_LATENCY,                             0x9c27891a, 0xed7a, 0x40e1, 0x88, 0xe8, 0xb2, 0x27, 0x27, 0xa0, 0x24, 0xee);
DEFINE_GUID(MF_VIDEO_MAX_MB_PER_SEC,                    0xe3f2e203, 0xd445, 0x4b8c, 0x92, 0x11, 0xae, 0x39, 0x0d, 0x3b, 0xa0, 0x17);
DEFINE_GUID(MF_DISABLE_FRAME_CORRUPTION_INFO,           0x7086e16c, 0x49c5, 0x4201, 0x88, 0x2a, 0x85, 0x38, 0xf3, 0x8c, 0xf1, 0x3a);

typedef unsigned __int64 MFWORKITEM_KEY;

typedef enum
{
    MF_STANDARD_WORKQUEUE,
    MF_WINDOW_WORKQUEUE,
    MF_MULTITHREADED_WORKQUEUE,
} MFASYNC_WORKQUEUE_TYPE;

typedef void (CALLBACK *MFPERIODICCALLBACK)(IUnknown *context);

#define MF_1_BYTE_ALIGNMENT       0x00000000
#define MF_2_BYTE_ALIGNMENT       0x00000001
#define MF_4_BYTE_ALIGNMENT       0x00000003
#define MF_8_BYTE_ALIGNMENT       0x00000007
#define MF_16_BYTE_ALIGNMENT      0x0000000f
#define MF_32_BYTE_ALIGNMENT      0x0000001f
#define MF_64_BYTE_ALIGNMENT      0x0000003f
#define MF_128_BYTE_ALIGNMENT     0x0000007f
#define MF_256_BYTE_ALIGNMENT     0x000000ff
#define MF_512_BYTE_ALIGNMENT     0x000001ff
#define MF_1024_BYTE_ALIGNMENT    0x000003ff
#define MF_2048_BYTE_ALIGNMENT    0x000007ff
#define MF_4096_BYTE_ALIGNMENT    0x00000fff
#define MF_8192_BYTE_ALIGNMENT    0x00001fff

typedef enum _MFWaveFormatExConvertFlags
{
    MFWaveFormatExConvertFlag_Normal = 0,
    MFWaveFormatExConvertFlag_ForceExtensible = 1,
} MFWaveFormatExConvertFlags;

enum _MFT_ENUM_FLAG
{
    MFT_ENUM_FLAG_SYNCMFT                         = 0x00000001,
    MFT_ENUM_FLAG_ASYNCMFT                        = 0x00000002,
    MFT_ENUM_FLAG_HARDWARE                        = 0x00000004,
    MFT_ENUM_FLAG_FIELDOFUSE                      = 0x00000008,
    MFT_ENUM_FLAG_LOCALMFT                        = 0x00000010,
    MFT_ENUM_FLAG_TRANSCODE_ONLY                  = 0x00000020,
    MFT_ENUM_FLAG_ALL                             = 0x0000003f,
    MFT_ENUM_FLAG_SORTANDFILTER                   = 0x00000040,
    MFT_ENUM_FLAG_SORTANDFILTER_APPROVED_ONLY     = 0x000000c0,
    MFT_ENUM_FLAG_SORTANDFILTER_WEB_ONLY          = 0x00000140,
    MFT_ENUM_FLAG_SORTANDFILTER_WEB_ONLY_EDGEMODE = 0x00000240,
    MFT_ENUM_FLAG_UNTRUSTED_STOREMFT              = 0x00000400,
};

typedef enum
{
    MF_TOPOSTATUS_INVALID = 0,
    MF_TOPOSTATUS_READY = 100,
    MF_TOPOSTATUS_STARTED_SOURCE = 200,
    MF_TOPOSTATUS_DYNAMIC_CHANGED = 210,
    MF_TOPOSTATUS_SINK_SWITCHED = 300,
    MF_TOPOSTATUS_ENDED = 400,
} MF_TOPOSTATUS;

/* Media Session capabilities. */
#define MFSESSIONCAP_START                 0x00000001
#define MFSESSIONCAP_SEEK                  0x00000002
#define MFSESSIONCAP_PAUSE                 0x00000004
#define MFSESSIONCAP_RATE_FORWARD          0x00000010
#define MFSESSIONCAP_RATE_REVERSE          0x00000020
#define MFSESSIONCAP_DOES_NOT_USE_NETWORK  0x00000040

HRESULT WINAPI MFAddPeriodicCallback(MFPERIODICCALLBACK callback, IUnknown *context, DWORD *key);
HRESULT WINAPI MFAllocateSerialWorkQueue(DWORD target_queue, DWORD *queue);
HRESULT WINAPI MFAllocateWorkQueue(DWORD *queue);
HRESULT WINAPI MFAllocateWorkQueueEx(MFASYNC_WORKQUEUE_TYPE queue_type, DWORD *queue);
HRESULT WINAPI MFBeginCreateFile(MF_FILE_ACCESSMODE access_mode, MF_FILE_OPENMODE open_mode, MF_FILE_FLAGS flags,
        const WCHAR *path, IMFAsyncCallback *callback, IUnknown *state, IUnknown **cancel_cookie);
HRESULT WINAPI MFBeginRegisterWorkQueueWithMMCSS(DWORD queue, const WCHAR *usage_class, DWORD taskid,
        IMFAsyncCallback *callback, IUnknown *state);
HRESULT WINAPI MFBeginRegisterWorkQueueWithMMCSSEx(DWORD queue, const WCHAR *usage_class, DWORD taskid, LONG priority,
        IMFAsyncCallback *callback, IUnknown *state);
HRESULT WINAPI MFBeginUnregisterWorkQueueWithMMCSS(DWORD queue, IMFAsyncCallback *callback, IUnknown *state);
HRESULT WINAPI MFCalculateImageSize(REFGUID subtype, UINT32 width, UINT32 height, UINT32 *size);
HRESULT WINAPI MFCancelCreateFile(IUnknown *cancel_cookie);
HRESULT WINAPI MFCancelWorkItem(MFWORKITEM_KEY key);
BOOL    WINAPI MFCompareFullToPartialMediaType(IMFMediaType *full_type, IMFMediaType *partial_type);
HRESULT WINAPI MFConvertColorInfoToDXVA(DWORD *dxva_info, const MFVIDEOFORMAT *format);
HRESULT WINAPI MFCopyImage(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride, DWORD width, DWORD lines);
HRESULT WINAPI MFCreate2DMediaBuffer(DWORD width, DWORD height, DWORD fourcc, BOOL bottom_up, IMFMediaBuffer **buffer);
HRESULT WINAPI MFCreateAlignedMemoryBuffer(DWORD max_length, DWORD alignment, IMFMediaBuffer **buffer);
HRESULT WINAPI MFCreateAttributes(IMFAttributes **attributes, UINT32 size);
HRESULT WINAPI MFCreateAsyncResult(IUnknown *object, IMFAsyncCallback *callback, IUnknown *state, IMFAsyncResult **result);
HRESULT WINAPI MFCreateCollection(IMFCollection **collection);
HRESULT WINAPI MFCreateDXGIDeviceManager(UINT *token, IMFDXGIDeviceManager **manager);
HRESULT WINAPI MFCreateDXSurfaceBuffer(REFIID riid, IUnknown *surface, BOOL bottom_up, IMFMediaBuffer **buffer);
HRESULT WINAPI MFCreateEventQueue(IMFMediaEventQueue **queue);
HRESULT WINAPI MFCreateFile(MF_FILE_ACCESSMODE accessmode, MF_FILE_OPENMODE openmode, MF_FILE_FLAGS flags,
                            LPCWSTR url, IMFByteStream **bytestream);
HRESULT WINAPI MFCreateMediaBufferFromMediaType(IMFMediaType *media_type, LONGLONG duration, DWORD min_length,
        DWORD min_alignment, IMFMediaBuffer **buffer);
HRESULT WINAPI MFCreateMediaEvent(MediaEventType type, REFGUID extended_type, HRESULT status,
                                  const PROPVARIANT *value, IMFMediaEvent **event);
HRESULT WINAPI MFCreateMediaType(IMFMediaType **type);
HRESULT WINAPI MFCreateMFVideoFormatFromMFMediaType(IMFMediaType *media_type, MFVIDEOFORMAT **video_format, UINT32 *size);
HRESULT WINAPI MFCreateSample(IMFSample **sample);
HRESULT WINAPI MFCreateVideoMediaTypeFromSubtype(const GUID *subtype, IMFVideoMediaType **media_type);
HRESULT WINAPI MFCreateMemoryBuffer(DWORD max_length, IMFMediaBuffer **buffer);
HRESULT WINAPI MFCreateWaveFormatExFromMFMediaType(IMFMediaType *type, WAVEFORMATEX **format, UINT32 *size, UINT32 flags);
HRESULT WINAPI MFEndCreateFile(IMFAsyncResult *result, IMFByteStream **stream);
HRESULT WINAPI MFEndRegisterWorkQueueWithMMCSS(IMFAsyncResult *result, DWORD *taskid);
HRESULT WINAPI MFEndUnregisterWorkQueueWithMMCSS(IMFAsyncResult *result);
void *  WINAPI MFHeapAlloc(SIZE_T size, ULONG flags, char *file, int line, EAllocationType type);
void    WINAPI MFHeapFree(void *ptr);
HRESULT WINAPI MFGetAttributesAsBlob(IMFAttributes *attributes, UINT8 *buffer, UINT size);
HRESULT WINAPI MFGetAttributesAsBlobSize(IMFAttributes *attributes, UINT32 *size);
HRESULT WINAPI MFGetStrideForBitmapInfoHeader(DWORD format, DWORD width, LONG *stride);
HRESULT WINAPI MFGetPlaneSize(DWORD format, DWORD width, DWORD height, DWORD *size);
HRESULT WINAPI MFGetTimerPeriodicity(DWORD *periodicity);
HRESULT WINAPI MFGetWorkQueueMMCSSClass(DWORD queue, WCHAR *class, DWORD *length);
HRESULT WINAPI MFGetWorkQueueMMCSSPriority(DWORD queue, LONG *priority);
HRESULT WINAPI MFGetWorkQueueMMCSSTaskId(DWORD queue, DWORD *taskid);
HRESULT WINAPI MFTEnum(GUID category, UINT32 flags, MFT_REGISTER_TYPE_INFO *input_type,
                       MFT_REGISTER_TYPE_INFO *output_type, IMFAttributes *attributes,
                       CLSID **pclsids, UINT32 *pcount);
HRESULT WINAPI MFTEnum2(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type, IMFAttributes *attributes, IMFActivate ***activate, UINT32 *count);
HRESULT WINAPI MFTEnumEx(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
                         const MFT_REGISTER_TYPE_INFO *output_type, IMFActivate ***activate,
                         UINT32 *pcount);
HRESULT WINAPI MFInitAttributesFromBlob(IMFAttributes *attributes, const UINT8 *buffer, UINT size);
HRESULT WINAPI MFInitMediaTypeFromWaveFormatEx(IMFMediaType *mediatype, const WAVEFORMATEX *format, UINT32 size);
HRESULT WINAPI MFInvokeCallback(IMFAsyncResult *result);
HRESULT WINAPI MFLockPlatform(void);
HRESULT WINAPI MFPutWaitingWorkItem(HANDLE event, LONG priority, IMFAsyncResult *result, MFWORKITEM_KEY *key);
HRESULT WINAPI MFPutWorkItem(DWORD queue, IMFAsyncCallback *callback, IUnknown *state);
HRESULT WINAPI MFPutWorkItem2(DWORD queue, LONG priority, IMFAsyncCallback *callback, IUnknown *state);
HRESULT WINAPI MFPutWorkItemEx(DWORD queue, IMFAsyncResult *result);
HRESULT WINAPI MFPutWorkItemEx2(DWORD queue, LONG priority, IMFAsyncResult *result);
HRESULT WINAPI MFRegisterLocalByteStreamHandler(const WCHAR *extension, const WCHAR *mime, IMFActivate *activate);
HRESULT WINAPI MFRegisterLocalSchemeHandler(const WCHAR *scheme, IMFActivate *activate);
HRESULT WINAPI MFRegisterPlatformWithMMCSS(const WCHAR *usage_class, DWORD *taskid, LONG priority);
HRESULT WINAPI MFScheduleWorkItem(IMFAsyncCallback *callback, IUnknown *state, INT64 timeout, MFWORKITEM_KEY *key);
HRESULT WINAPI MFScheduleWorkItemEx(IMFAsyncResult *result, INT64 timeout, MFWORKITEM_KEY *key);
HRESULT WINAPI MFTRegister(CLSID clsid, GUID category, LPWSTR name, UINT32 flags, UINT32 cinput,
                           MFT_REGISTER_TYPE_INFO *input_types, UINT32 coutput,
                           MFT_REGISTER_TYPE_INFO *output_types, IMFAttributes *attributes);
HRESULT WINAPI MFTRegisterLocal(IClassFactory *factory, REFGUID category, LPCWSTR name,
                           UINT32 flags, UINT32 cinput, const MFT_REGISTER_TYPE_INFO *input_types,
                           UINT32 coutput, const MFT_REGISTER_TYPE_INFO* output_types);
HRESULT WINAPI MFTRegisterLocalByCLSID(REFCLSID clsid, REFGUID category, LPCWSTR name, UINT32 flags,
        UINT32 input_count, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 output_count,
        const MFT_REGISTER_TYPE_INFO *output_types);
HRESULT WINAPI MFRemovePeriodicCallback(DWORD key);
HRESULT WINAPI MFShutdown(void);
HRESULT WINAPI MFStartup(ULONG version, DWORD flags);
HRESULT WINAPI MFUnlockPlatform(void);
HRESULT WINAPI MFUnlockWorkQueue(DWORD queue);
HRESULT WINAPI MFUnregisterPlatformFromMMCSS(void);
HRESULT WINAPI MFTUnregister(CLSID clsid);
HRESULT WINAPI MFTUnregisterLocal(IClassFactory *factory);
HRESULT WINAPI MFTUnregisterLocalByCLSID(CLSID clsid);
HRESULT WINAPI MFGetPluginControl(IMFPluginControl**);
HRESULT WINAPI MFWrapMediaType(IMFMediaType *original, REFGUID major, REFGUID subtype, IMFMediaType **wrapped);
HRESULT WINAPI MFUnwrapMediaType(IMFMediaType *wrapped, IMFMediaType **original);

#if defined(__cplusplus)
}
#endif

#endif /* __WINE_MFAPI_H */
