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

#define DEFINE_MEDIATYPE_GUID(name, format) \
    DEFINE_GUID(name, format, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

#ifndef DIRECT3D_VERSION
#define D3DFMT_R8G8B8         20
#define D3DFMT_A8R8G8B8       21
#define D3DFMT_X8R8G8B8       22
#define D3DFMT_R5G6B5         23
#define D3DFMT_X1R5G5B5       24
#define D3DFMT_A2B10G10R10    31
#define D3DFMT_P8             41
#define D3DFMT_A16B16G16R16F 113
#endif

DEFINE_MEDIATYPE_GUID(MFVideoFormat_WMV3,          MAKEFOURCC('W','M','V','3'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB8,          D3DFMT_P8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB555,        D3DFMT_X1R5G5B5);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB565,        D3DFMT_R5G6B5);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB24,         D3DFMT_R8G8B8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB32,         D3DFMT_X8R8G8B8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ARGB32,        D3DFMT_A8R8G8B8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_A2R10G10B10,   D3DFMT_A2B10G10R10);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_A16B16G16R16F, D3DFMT_A16B16G16R16F);

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

DEFINE_GUID(MF_MT_AVG_BITRATE,             0x20332624, 0xfb0d, 0x4d9e, 0xbd, 0x0d, 0xcb, 0xf6, 0x78, 0x6c, 0x10, 0x2e);
DEFINE_GUID(MF_MT_FRAME_RATE,              0xc459a2e8, 0x3d2c, 0x4e44, 0xb1, 0x32, 0xfe, 0xe5, 0x15, 0x6c, 0x7b, 0xb0);
DEFINE_GUID(MF_MT_FRAME_SIZE,              0x1652c33d, 0xd6b2, 0x4012, 0xb8, 0x34, 0x72, 0x03, 0x08, 0x49, 0xa3, 0x7d);
DEFINE_GUID(MF_MT_INTERLACE_MODE,          0xe2724bb8, 0xe676, 0x4806, 0xb4, 0xb2, 0xa8, 0xd6, 0xef, 0xb4, 0x4c, 0xcd);
DEFINE_GUID(MF_MT_MAJOR_TYPE,              0x48eba18e, 0xf8c9, 0x4687, 0xbf, 0x11, 0x0a, 0x74, 0xc9, 0xf9, 0x6a, 0x8f);
DEFINE_GUID(MF_MT_PIXEL_ASPECT_RATIO,      0xc6376a1e, 0x8d0a, 0x4027, 0xbe, 0x45, 0x6d, 0x9a, 0x0a, 0xd3, 0x9b, 0xb6);
DEFINE_GUID(MF_MT_SUBTYPE,                 0xf7e34c9a, 0x42e8, 0x4714, 0xb7, 0x4b, 0xcb, 0x29, 0xd7, 0x2c, 0x35, 0xe5);
DEFINE_GUID(MF_MT_ALL_SAMPLES_INDEPENDENT, 0xc9173739, 0x5e56, 0x461c, 0xb7, 0x13, 0x46, 0xfb, 0x99, 0x5c, 0xb9, 0x5f);
DEFINE_GUID(MF_MT_USER_DATA,               0xb6bc765f, 0x4c3b, 0x40a4, 0xbd, 0x51, 0x25, 0x35, 0xb6, 0x6f, 0xe0, 0x9d);
DEFINE_GUID(MF_MT_FRAME_RATE_RANGE_MIN,    0xd2e7558c, 0xdc1f, 0x403f, 0x9a, 0x72, 0xd2, 0x8b, 0xb1, 0xeb, 0x3b, 0x5e);
DEFINE_GUID(MF_MT_FRAME_RATE_RANGE_MAX,    0xe3371d41, 0xb4cf, 0x4a05, 0xbd, 0x4e, 0x20, 0xb8, 0x8b, 0xb2, 0xc4, 0xd6);

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

DEFINE_GUID(MFSampleExtension_DecodeTimestamp, 0x73a954d4, 0x09e2, 0x4861, 0xbe, 0xfc, 0x94, 0xbd, 0x97, 0xc0, 0x8e, 0x6e);
DEFINE_GUID(MFSampleExtension_Timestamp,       0x1e436999, 0x69be, 0x4c7a, 0x93, 0x69, 0x70, 0x06, 0x8c, 0x02, 0x60, 0xcb);
DEFINE_GUID(MFSampleExtension_Token,           0x8294da66, 0xf328, 0x4805, 0xb5, 0x51, 0x00, 0xde, 0xb4, 0xc5, 0x7a, 0x61);

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

HRESULT WINAPI MFAddPeriodicCallback(MFPERIODICCALLBACK callback, IUnknown *context, DWORD *key);
HRESULT WINAPI MFAllocateWorkQueue(DWORD *queue);
HRESULT WINAPI MFAllocateWorkQueueEx(MFASYNC_WORKQUEUE_TYPE queue_type, DWORD *queue);
HRESULT WINAPI MFCalculateImageSize(REFGUID subtype, UINT32 width, UINT32 height, UINT32 *size);
HRESULT WINAPI MFCancelWorkItem(MFWORKITEM_KEY key);
BOOL    WINAPI MFCompareFullToPartialMediaType(IMFMediaType *full_type, IMFMediaType *partial_type);
HRESULT WINAPI MFCopyImage(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride, DWORD width, DWORD lines);
HRESULT WINAPI MFCreateAlignedMemoryBuffer(DWORD max_length, DWORD alignment, IMFMediaBuffer **buffer);
HRESULT WINAPI MFCreateAttributes(IMFAttributes **attributes, UINT32 size);
HRESULT WINAPI MFCreateAsyncResult(IUnknown *object, IMFAsyncCallback *callback, IUnknown *state, IMFAsyncResult **result);
HRESULT WINAPI MFCreateCollection(IMFCollection **collection);
HRESULT WINAPI MFCreateEventQueue(IMFMediaEventQueue **queue);
HRESULT WINAPI MFCreateFile(MF_FILE_ACCESSMODE accessmode, MF_FILE_OPENMODE openmode, MF_FILE_FLAGS flags,
                            LPCWSTR url, IMFByteStream **bytestream);
HRESULT WINAPI MFCreateMediaEvent(MediaEventType type, REFGUID extended_type, HRESULT status,
                                  const PROPVARIANT *value, IMFMediaEvent **event);
HRESULT WINAPI MFCreateMediaType(IMFMediaType **type);
HRESULT WINAPI MFCreateSample(IMFSample **sample);
HRESULT WINAPI MFCreateMemoryBuffer(DWORD max_length, IMFMediaBuffer **buffer);
void *  WINAPI MFHeapAlloc(SIZE_T size, ULONG flags, char *file, int line, EAllocationType type);
void    WINAPI MFHeapFree(void *ptr);
HRESULT WINAPI MFGetAttributesAsBlob(IMFAttributes *attributes, UINT8 *buffer, UINT size);
HRESULT WINAPI MFGetAttributesAsBlobSize(IMFAttributes *attributes, UINT32 *size);
HRESULT WINAPI MFGetTimerPeriodicity(DWORD *periodicity);
HRESULT WINAPI MFTEnum(GUID category, UINT32 flags, MFT_REGISTER_TYPE_INFO *input_type,
                       MFT_REGISTER_TYPE_INFO *output_type, IMFAttributes *attributes,
                       CLSID **pclsids, UINT32 *pcount);
HRESULT WINAPI MFTEnumEx(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
                         const MFT_REGISTER_TYPE_INFO *output_type, IMFActivate ***activate,
                         UINT32 *pcount);
HRESULT WINAPI MFInitAttributesFromBlob(IMFAttributes *attributes, const UINT8 *buffer, UINT size);
HRESULT WINAPI MFInvokeCallback(IMFAsyncResult *result);
HRESULT WINAPI MFLockPlatform(void);
HRESULT WINAPI MFPutWorkItem(DWORD queue, IMFAsyncCallback *callback, IUnknown *state);
HRESULT WINAPI MFPutWorkItem2(DWORD queue, LONG priority, IMFAsyncCallback *callback, IUnknown *state);
HRESULT WINAPI MFPutWorkItemEx(DWORD queue, IMFAsyncResult *result);
HRESULT WINAPI MFPutWorkItemEx2(DWORD queue, LONG priority, IMFAsyncResult *result);
HRESULT WINAPI MFScheduleWorkItem(IMFAsyncCallback *callback, IUnknown *state, INT64 timeout, MFWORKITEM_KEY *key);
HRESULT WINAPI MFScheduleWorkItemEx(IMFAsyncResult *result, INT64 timeout, MFWORKITEM_KEY *key);
HRESULT WINAPI MFTRegister(CLSID clsid, GUID category, LPWSTR name, UINT32 flags, UINT32 cinput,
                           MFT_REGISTER_TYPE_INFO *input_types, UINT32 coutput,
                           MFT_REGISTER_TYPE_INFO *output_types, IMFAttributes *attributes);
HRESULT WINAPI MFTRegisterLocal(IClassFactory *factory, REFGUID category, LPCWSTR name,
                           UINT32 flags, UINT32 cinput, const MFT_REGISTER_TYPE_INFO *input_types,
                           UINT32 coutput, const MFT_REGISTER_TYPE_INFO* output_types);
HRESULT WINAPI MFRemovePeriodicCallback(DWORD key);
HRESULT WINAPI MFShutdown(void);
HRESULT WINAPI MFStartup(ULONG version, DWORD flags);
HRESULT WINAPI MFUnlockPlatform(void);
HRESULT WINAPI MFUnlockWorkQueue(DWORD queue);
HRESULT WINAPI MFTUnregister(CLSID clsid);
HRESULT WINAPI MFTUnregisterLocal(IClassFactory *factory);
HRESULT WINAPI MFGetPluginControl(IMFPluginControl**);

#if defined(__cplusplus)
}
#endif

#endif /* __WINE_MFAPI_H */
