/*
 * Unit tests for mf.dll.
 *
 * Copyright 2017 Nikolay Sivov
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "mfapi.h"
#include "mfidl.h"
#include "mftransform.h"

extern HRESULT (WINAPI *pMFCreateSampleCopierMFT)(IMFTransform **copier);
extern HRESULT (WINAPI *pMFGetTopoNodeCurrentType)(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaType **type);
extern HRESULT (WINAPI *pMFCreateDXGIDeviceManager)(UINT *token, IMFDXGIDeviceManager **manager);

extern BOOL has_video_processor;
void init_functions(void);

static const BYTE test_h264_sequence_header[] =
{
    0x00, 0x00, 0x00, 0x01, 0x67, 0x4d, 0x40, 0x0b, 0x96, 0x56, 0x31, 0xb4,
    0x20, 0x00, 0x00, 0x7d, 0x20, 0x00, 0x1d, 0x4c, 0x01, 0xb4, 0x11, 0x08,
    0xa7, 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
};

const static BYTE test_aac_codec_data[] =
{
    0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x12, 0x08,
};

struct attribute_desc
{
    const GUID *key;
    const char *name;
    PROPVARIANT value;
    BOOL ratio;
    BOOL required;
    BOOL todo;
    BOOL todo_value;
};
typedef struct attribute_desc media_type_desc[32];

#define ATTR_GUID(k, g, ...)      {.key = &k, .name = #k, {.vt = VT_CLSID, .puuid = (GUID *)&g}, __VA_ARGS__ }
#define ATTR_UINT32(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI4, .ulVal = v}, __VA_ARGS__ }
#define ATTR_BLOB(k, p, n, ...)   {.key = &k, .name = #k, {.vt = VT_VECTOR | VT_UI1, .caub = {.pElems = (void *)p, .cElems = n}}, __VA_ARGS__ }
#define ATTR_RATIO(k, n, d, ...)  {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.HighPart = n, .LowPart = d}}, .ratio = TRUE, __VA_ARGS__ }
#define ATTR_UINT64(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.QuadPart = v}}, __VA_ARGS__ }

#define check_media_type(a, b, c) check_attributes_(__FILE__, __LINE__, (IMFAttributes *)a, b, c)
#define check_attributes(a, b, c) check_attributes_(__FILE__, __LINE__, a, b, c)
extern void check_attributes_(const char *file, int line, IMFAttributes *attributes,
        const struct attribute_desc *desc, ULONG limit);
extern void init_media_type(IMFMediaType *mediatype, const struct attribute_desc *desc, ULONG limit);

typedef DWORD (*compare_cb)(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect);
extern DWORD compare_nv12(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect);
extern DWORD compare_i420(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect);
extern DWORD compare_rgb32(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect);
extern DWORD compare_rgb24(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect);
extern DWORD compare_rgb16(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect);
extern DWORD compare_pcm16(const BYTE *data, DWORD *length, const RECT *rect, const BYTE *expect);

typedef void (*dump_cb)(const BYTE *data, DWORD length, const RECT *rect, HANDLE output);
extern void dump_rgb32(const BYTE *data, DWORD length, const RECT *rect, HANDLE output);
extern void dump_rgb24(const BYTE *data, DWORD length, const RECT *rect, HANDLE output);
extern void dump_rgb16(const BYTE *data, DWORD length, const RECT *rect, HANDLE output);
extern void dump_nv12(const BYTE *data, DWORD length, const RECT *rect, HANDLE output);
extern void dump_i420(const BYTE *data, DWORD length, const RECT *rect, HANDLE output);

struct buffer_desc
{
    DWORD length;
    BOOL todo_length;
    compare_cb compare;
    dump_cb dump;
    RECT rect;
};

struct sample_desc
{
    const struct attribute_desc *attributes;
    LONGLONG sample_time;
    LONGLONG sample_duration;
    DWORD buffer_count;
    const struct buffer_desc *buffers;
    DWORD repeat_count;
    BOOL todo_length;
    BOOL todo_duration;
    BOOL todo_time;
};

#define check_mf_sample_collection(a, b, c) check_mf_sample_collection_(__FILE__, __LINE__, a, b, c)
extern DWORD check_mf_sample_collection_(const char *file, int line, IMFCollection *samples,
        const struct sample_desc *expect_samples, const WCHAR *expect_data_filename);
