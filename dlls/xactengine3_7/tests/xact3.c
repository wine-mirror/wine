/*
 * Copyright (c) 2020 Alistair Leslie-Hughes
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

#include <windows.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <mmsystem.h>

#define COBJMACROS
#include "wine/test.h"

#include "initguid.h"
#include "xact_classes.h"
#define __xaudio2_h__
#include "x3daudio.h"

#include "xact3.h"
#include "xact3wb.h"

DEFINE_GUID(IID_IXACT3Engine30, 0x9e33f661, 0x2d07, 0x43ec, 0x97, 0x04, 0xbb, 0xcb, 0x71, 0xa5, 0x49, 0x72);
DEFINE_GUID(IID_IXACT3Engine31, 0xe72c1b9a, 0xd717, 0x41c0, 0x81, 0xa6, 0x50, 0xeb, 0x56, 0xe8, 0x06, 0x49);

DEFINE_GUID(CLSID_XACTEngine30, 0x3b80ee2a, 0xb0f5, 0x4780, 0x9e, 0x30, 0x90, 0xcb, 0x39, 0x68, 0x5b, 0x03);
DEFINE_GUID(CLSID_XACTEngine31, 0x962f5027, 0x99be, 0x4692, 0xa4, 0x68, 0x85, 0x80, 0x2c, 0xf8, 0xde, 0x61);
DEFINE_GUID(CLSID_XACTEngine32, 0xd3332f02, 0x3dd0, 0x4de9, 0x9a, 0xec, 0x20, 0xd8, 0x5c, 0x41, 0x11, 0xb6);
DEFINE_GUID(CLSID_XACTEngine33, 0x94c1affa, 0x66e7, 0x4961, 0x95, 0x21, 0xcf, 0xde, 0xf3, 0x12, 0x8d, 0x4f);
DEFINE_GUID(CLSID_XACTEngine34, 0x0977d092, 0x2d95, 0x4e43, 0x8d, 0x42, 0x9d, 0xdc, 0xc2, 0x54, 0x5e, 0xd5);
DEFINE_GUID(CLSID_XACTEngine35, 0x074b110f, 0x7f58, 0x4743, 0xae, 0xa5, 0x12, 0xf1, 0x5b, 0x50, 0x74, 0xed);
DEFINE_GUID(CLSID_XACTEngine36, 0x248d8a3b, 0x6256, 0x44d3, 0xa0, 0x18, 0x2a, 0xc9, 0x6c, 0x45, 0x9f, 0x47);

struct xact_interfaces
{
    REFGUID clsid;
    REFIID iid;
    HRESULT expected;
} xact_interfaces[] =
{
    {&CLSID_XACTEngine30, &IID_IXACT3Engine30, S_OK },
    {&CLSID_XACTEngine30, &IID_IXACT3Engine31, E_NOINTERFACE},
    {&CLSID_XACTEngine30, &IID_IXACT3Engine,   E_NOINTERFACE },

    /* Version 3.1 to 3.4 use the same interface */
    {&CLSID_XACTEngine31, &IID_IXACT3Engine31, S_OK },
    {&CLSID_XACTEngine32, &IID_IXACT3Engine31, S_OK },
    {&CLSID_XACTEngine33, &IID_IXACT3Engine31, S_OK },
    {&CLSID_XACTEngine34, &IID_IXACT3Engine31, S_OK },

    /* Version 3.5 to 3.7 use the same interface */
    {&CLSID_XACTEngine35, &IID_IXACT3Engine31, E_NOINTERFACE },
    {&CLSID_XACTEngine35, &IID_IXACT3Engine,   S_OK },

    {&CLSID_XACTEngine36, &IID_IXACT3Engine31, E_NOINTERFACE },
    {&CLSID_XACTEngine36, &IID_IXACT3Engine,   S_OK },

    {&CLSID_XACTEngine,   &IID_IXACT3Engine31, E_NOINTERFACE },
    {&CLSID_XACTEngine,   &IID_IXACT3Engine,   S_OK },
    {&CLSID_XACTEngine,   &IID_IUnknown,       S_OK },
};

static void test_interfaces(void)
{
    IUnknown *unk;
    HRESULT hr;
    int i;

    for (i = 0; i < ARRAY_SIZE(xact_interfaces); i++)
    {
        hr = CoCreateInstance(xact_interfaces[i].clsid, NULL, CLSCTX_INPROC_SERVER,
                    xact_interfaces[i].iid, (void**)&unk);
        if (hr == REGDB_E_CLASSNOTREG)
        {
            trace("%d %s not registered. Skipping\n", i, wine_dbgstr_guid(xact_interfaces[i].clsid) );
            continue;
        }
        ok(hr == xact_interfaces[i].expected, "%d, Unexpected value 0x%08lx\n", i, hr);
        if (hr == S_OK)
            IUnknown_Release(unk);
    }
}

/* See resource.rc for the source code. */

static const char soundbank_data[] =
{
        0x53, 0x44, 0x42, 0x4b, 0x2e, 0x00, 0x2b, 0x00, 0x38, 0x9a, 0x1d, 0x05, 0x92, 0xc6, 0x57, 0x0d,
        0xdc, 0x01, 0x01, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x02, 0x00, 0x09, 0x00,
        0x00, 0x00, 0x23, 0x01, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00, 0x54, 0x02, 0x00, 0x00, 0xff, 0xff,
        0xff, 0xff, 0x46, 0x01, 0x00, 0x00, 0x82, 0x01, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x22, 0x02,
        0x00, 0x00, 0x42, 0x02, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x73, 0x62, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x62, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x58, 0x42, 0x00,
        0x00, 0x4d, 0x00, 0x02, 0xa8, 0xe6, 0x00, 0x00, 0x00, 0xc0, 0x5d, 0xe8, 0x03, 0x96, 0x06, 0x01,
        0x00, 0x00, 0xc0, 0x5d, 0xe8, 0x03, 0x01, 0x03, 0x00, 0x00, 0x20, 0x00, 0x00, 0xff, 0x0c, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x40, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
        0x64, 0x01, 0x00, 0x00, 0x02, 0xc8, 0x01, 0x01, 0x00, 0x00, 0x20, 0x00, 0x00, 0xff, 0x0c, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xe9, 0xf8, 0xff, 0x00, 0x0c, 0x00,
        0x01, 0x00, 0x00, 0x04, 0x17, 0x01, 0x00, 0x00, 0x01, 0x46, 0x01, 0x00, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x5a, 0x01, 0x00, 0x00, 0x82, 0x01, 0x00, 0x00,
        0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x00, 0xff, 0xff, 0xff, 0xff, 0xca, 0x00,
        0x00, 0x00, 0x01, 0x14, 0x17, 0x01, 0x00, 0x00, 0x1e, 0x3c, 0x02, 0x00, 0x1b, 0x00, 0xff, 0xff,
        0x07, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0xa0, 0x40, 0x01, 0x00,
        0x00, 0x00, 0x17, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0xc0, 0x41, 0x01, 0x00,
        0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0xff, 0xff,
        0xff, 0xff, 0x54, 0x02, 0x00, 0x00, 0xff, 0xff, 0x57, 0x02, 0x00, 0x00, 0xff, 0xff, 0x5a, 0x02,
        0x00, 0x00, 0xff, 0xff, 0x63, 0x33, 0x00, 0x63, 0x31, 0x00, 0x63, 0x32, 0x00,
};

static const char global_settings_data[] =
{
        0x58, 0x47, 0x53, 0x46, 0x2e, 0x00, 0x2a, 0x00, 0x4c, 0xca, 0x37, 0x2f, 0xa2, 0x52, 0xb7, 0x0b,
        0xdc, 0x01, 0x03, 0x03, 0x00, 0x0b, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x4d, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0x1a, 0x01, 0x00,
        0x00, 0x41, 0x01, 0x00, 0x00, 0x61, 0x01, 0x00, 0x00, 0x2c, 0x01, 0x00, 0x00, 0xa3, 0x01, 0x00,
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0xb4, 0x02, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb4,
        0x02, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb4, 0x03, 0x0f, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x44, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x60, 0x6a, 0x46, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60,
        0x6a, 0x46, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0xc3, 0x00, 0x00, 0x34, 0x43, 0x0d,
        0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x09, 0x00, 0xc0, 0xab,
        0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x74, 0x49, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x24, 0x74, 0x49, 0x01, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xc8, 0x42, 0x05, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x42,
        0x00, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x42, 0x03, 0x00, 0x00,
        0x20, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x42, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02, 0x00, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x2c, 0x01, 0x00, 0x00, 0xff, 0xff,
        0x33, 0x01, 0x00, 0x00, 0xff, 0xff, 0x3b, 0x01, 0x00, 0x00, 0xff, 0xff, 0x47, 0x6c, 0x6f, 0x62,
        0x61, 0x6c, 0x00, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x00, 0x4d, 0x75, 0x73, 0x69, 0x63,
        0x00, 0x02, 0x00, 0x03, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x08,
        0x00, 0xa3, 0x01, 0x00, 0x00, 0xff, 0xff, 0xb3, 0x01, 0x00, 0x00, 0x04, 0x00, 0xbe, 0x01, 0x00,
        0x00, 0x09, 0x00, 0xca, 0x01, 0x00, 0x00, 0x0a, 0x00, 0xdb, 0x01, 0x00, 0x00, 0xff, 0xff, 0xee,
        0x01, 0x00, 0x00, 0x06, 0x00, 0xfb, 0x01, 0x00, 0x00, 0xff, 0xff, 0x04, 0x02, 0x00, 0x00, 0xff,
        0xff, 0x07, 0x02, 0x00, 0x00, 0xff, 0xff, 0x0a, 0x02, 0x00, 0x00, 0xff, 0xff, 0x0d, 0x02, 0x00,
        0x00, 0xff, 0xff, 0x4e, 0x75, 0x6d, 0x43, 0x75, 0x65, 0x49, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63,
        0x65, 0x73, 0x00, 0x41, 0x74, 0x74, 0x61, 0x63, 0x6b, 0x54, 0x69, 0x6d, 0x65, 0x00, 0x52, 0x65,
        0x6c, 0x65, 0x61, 0x73, 0x65, 0x54, 0x69, 0x6d, 0x65, 0x00, 0x4f, 0x72, 0x69, 0x65, 0x6e, 0x74,
        0x61, 0x74, 0x69, 0x6f, 0x6e, 0x41, 0x6e, 0x67, 0x6c, 0x65, 0x00, 0x44, 0x6f, 0x70, 0x70, 0x6c,
        0x65, 0x72, 0x50, 0x69, 0x74, 0x63, 0x68, 0x53, 0x63, 0x61, 0x6c, 0x61, 0x72, 0x00, 0x53, 0x70,
        0x65, 0x65, 0x64, 0x4f, 0x66, 0x53, 0x6f, 0x75, 0x6e, 0x64, 0x00, 0x44, 0x69, 0x73, 0x74, 0x61,
        0x6e, 0x63, 0x65, 0x00, 0x76, 0x31, 0x00, 0x76, 0x32, 0x00, 0x76, 0x33, 0x00, 0x76, 0x34, 0x00,
};

static const struct
{
    WAVEBANKHEADER header;
    WAVEBANKDATA data;
    WAVEBANKENTRY entry;
}
streaming_wavebank =
{
    .entry =
    {
        .Format =
        {
            .wFormatTag = WAVEBANKMINIFORMAT_TAG_ADPCM,
            .nChannels = 2,
            .nSamplesPerSec = 22051,
            .wBlockAlign = 48,
        },
    },
    .data =
    {
        .dwFlags = WAVEBANK_TYPE_STREAMING,
        .dwEntryCount = 1,
        .szBankName = "test",
        .dwEntryMetaDataElementSize = sizeof(WAVEBANKENTRY),
        .dwEntryNameElementSize = WAVEBANK_ENTRYNAME_LENGTH,
        .dwAlignment = 0x800,
    },
    .header =
    {
        .dwSignature = WAVEBANK_HEADER_SIGNATURE,
        .dwVersion = XACT_CONTENT_VERSION,
        .dwHeaderVersion = WAVEBANK_HEADER_VERSION,
        .Segments =
        {
            [WAVEBANK_SEGIDX_BANKDATA] =
            {
                .dwOffset = sizeof(WAVEBANKHEADER),
                .dwLength = sizeof(WAVEBANKDATA),
            },
            [WAVEBANK_SEGIDX_ENTRYMETADATA] =
            {
                .dwOffset = sizeof(WAVEBANKHEADER) + sizeof(WAVEBANKDATA),
                .dwLength = sizeof(WAVEBANKENTRY),
            },
        },
    },
};

struct notification_cb_data
{
    XACTNOTIFICATIONTYPE type;
    IXACT3WaveBank *wave_bank;
    unsigned int count;
    DWORD thread_id;
    DWORD timestamp_before;
    BOOL todo_wave_bank;
};

/* Current time as calculated by xactengine. */
static DWORD get_current_time(void)
{
    LARGE_INTEGER time, freq;

    QueryPerformanceCounter(&time);
    QueryPerformanceFrequency(&freq);

    return ((time.QuadPart * 1000) / freq.QuadPart) & INT_MAX;
}

static void WINAPI notification_cb(const XACT_NOTIFICATION *notification)
{
    struct notification_cb_data *data = notification->pvContext;
    DWORD timestamp_after = get_current_time();
    DWORD thread_id = GetCurrentThreadId();

    ++data->count;
    ok(notification->type == data->type, "Expected type %u, got %u.\n", data->type, notification->type);
    if (data->timestamp_before) /* ignore notifications we shouldn't even be getting */
        todo_wine ok(notification->timeStamp >= data->timestamp_before && notification->timeStamp <= timestamp_after,
                "Expected %lu <= timestamp <= %lu, got %lu.\n",
                data->timestamp_before, timestamp_after, notification->timeStamp);
    if (notification->type == XACTNOTIFICATIONTYPE_WAVEBANKPREPARED)
        data->wave_bank = notification->waveBank.pWaveBank;
    else if (notification->type == XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED)
        todo_wine_if (data->todo_wave_bank) ok(notification->waveBank.pWaveBank == data->wave_bank, "Unexpected wave bank %p instead of %p\n",
            notification->waveBank.pWaveBank, data->wave_bank);
    ok(thread_id == data->thread_id, "Unexpected thread id %#lx instead of %#lx\n", thread_id, data->thread_id);
}

static void test_notifications(void)
{
    struct notification_cb_data prepared_data = {0}, destroyed_data = {0}, destroyed_data2 = {0};
    XACT_RUNTIME_PARAMETERS params = {.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT};
    struct notification_cb_data prepared_data_once = {0}, destroyed_data_once = {0};
    XACT_NOTIFICATION_DESCRIPTION notification_desc = { 0 };
    XACT_STREAMING_PARAMETERS streaming_params = { 0 };
    IXACT3WaveBank *wavebank, *wavebank2;
    IXACT3SoundBank *soundbank;
    void *soundbank_data_copy;
    IXACT3Engine *engine;
    WCHAR path[MAX_PATH];
    DWORD state, size;
    ULONG refcount;
    HANDLE file;
    HRESULT hr;
    BOOL ret;

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_Initialize(engine, &params);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

if (!winetest_platform_is_wine)
{
    hr = IXACT3Engine_RegisterNotification(engine, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
}

    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKPREPARED;
    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    notification_desc.pvContext = &prepared_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    todo_wine ok(hr == XACTENGINE_E_NONOTIFICATIONCALLBACK, "Got hr %#lx.\n", hr);

if (!winetest_platform_is_wine)
{
    hr = IXACT3Engine_UnRegisterNotification(engine, NULL);
    ok(hr == XACTENGINE_E_NONOTIFICATIONCALLBACK, "Got hr %#lx.\n", hr);
}
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    todo_wine ok(hr == XACTENGINE_E_NONOTIFICATIONCALLBACK, "Got hr %#lx.\n", hr);

    refcount = IXACT3Engine_Release(engine);
    todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    params.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;
    params.fnNotificationCallback = notification_cb;
    params.globalSettingsBufferSize = sizeof(global_settings_data);
    params.pGlobalSettingsBuffer = (void *)global_settings_data;

    hr = IXACT3Engine_Initialize(engine, &params);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    notification_desc.type = 0;
    notification_desc.flags = 0;
    notification_desc.pvContext = &prepared_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);

if (!winetest_platform_is_wine)
{
    hr = IXACT3Engine_UnRegisterNotification(engine, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT + 1;
    notification_desc.flags = 0;
    notification_desc.pvContext = &prepared_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);

    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    prepared_data.type = XACTNOTIFICATIONTYPE_WAVEBANKPREPARED;
    prepared_data.thread_id = GetCurrentThreadId();
    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKPREPARED;
    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    notification_desc.pvContext = &prepared_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Cannot register notification, hr %#lx\n", hr);

    destroyed_data.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    destroyed_data.thread_id = GetCurrentThreadId();
    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    notification_desc.pvContext = &destroyed_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Cannot register notification, hr %#lx\n", hr);

    /* Multiple registrations are held in a stack. A notification is delivered
     * to exactly one callback at a time, which is the most recent eligible
     * callback to be registered (cf. the filtering tests below).
     * UnRegisterCallback() unregisters the most recent callback. */
    destroyed_data2.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    destroyed_data2.thread_id = GetCurrentThreadId();
    notification_desc.pvContext = &destroyed_data2;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Cannot register notification, hr %#lx\n", hr);

    GetTempPathW(ARRAY_SIZE(path), path);
    wcscat(path, L"test.xwb");

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create %s, error %lu.\n", debugstr_w(path), GetLastError());

    ret = WriteFile(file, &streaming_wavebank, sizeof(streaming_wavebank), &size, NULL);
    ok(ret == TRUE, "Got error %lu.\n", GetLastError());

    /* Do this twice, to test that persistent notifications truly are persistent. */
    for (unsigned int i = 0; i < 3; ++i)
    {
        prepared_data.count = destroyed_data.count = destroyed_data2.count = 0;

        /* For some reason PREPARED notifications aren't sent synchronously.
         * They're instead sent (in the same thread) from DoWork(). */
        streaming_params.file = file;
        streaming_params.packetSize = 0x800;
        hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
        ok(hr == S_OK, "Cannot create a streaming wave bank, hr %#lx\n", hr);
        destroyed_data.wave_bank = destroyed_data2.wave_bank = wavebank;
        ok(!prepared_data.count, "Got %u notifications.\n", prepared_data.count);

        prepared_data.timestamp_before = get_current_time();
        hr = IXACT3Engine_DoWork(engine);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(prepared_data.count == 1, "Got %u notifications.\n", prepared_data.count);
        ok(prepared_data.wave_bank == wavebank, "Wave banks didn't match.\n");

        hr = IXACT3WaveBank_GetState(wavebank, &state);
        ok(hr == S_OK, "Cannot query wave bank state, hr %#lx\n", hr);
        ok(state == XACT_WAVEBANKSTATE_PREPARED, "Wave bank is not in prepared state, but in %#lx\n", state);

        ok(!destroyed_data.count, "Got %u notifications.\n", destroyed_data.count);
        ok(!destroyed_data2.count, "Got %u notifications.\n", destroyed_data2.count);

        destroyed_data.timestamp_before = get_current_time();
        hr = IXACT3WaveBank_Destroy(wavebank);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(destroyed_data.count == 0, "Got %u notifications.\n", destroyed_data.count);
        ok(destroyed_data2.count == 1, "Got %u notifications.\n", destroyed_data2.count);
    }

    /* UnRegisterNotification() searches for a notification matching both the
     * flags and context. If it doesn't find one, it helpfully returns S_OK
     * anyway. */
    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    notification_desc.pvContext = NULL;
    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
if (!winetest_platform_is_wine)
{
    notification_desc.pvContext = &destroyed_data2;
    notification_desc.flags = 0;
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

    destroyed_data.count = destroyed_data2.count = 0;
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    destroyed_data.wave_bank = destroyed_data2.wave_bank = wavebank;
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(destroyed_data.count == 0, "Got %u notifications.\n", destroyed_data.count);
    todo_wine ok(destroyed_data2.count == 1, "Got %u notifications.\n", destroyed_data2.count);

    notification_desc.pvContext = &destroyed_data2;
    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    destroyed_data.count = destroyed_data2.count = 0;
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    destroyed_data.wave_bank = destroyed_data2.wave_bank = wavebank;
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(destroyed_data.count == 1, "Got %u notifications.\n", destroyed_data.count);
    ok(destroyed_data2.count == 0, "Got %u notifications.\n", destroyed_data2.count);

    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Registering the exact notification twice overrides the first. */
    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    notification_desc.pvContext = &destroyed_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    destroyed_data.count = 0;
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    destroyed_data.wave_bank = wavebank;
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!destroyed_data.count, "Got %u notifications.\n", destroyed_data.count);

if (!winetest_platform_is_wine)
{
    /* Test non-persistent notifications.
     *
     * Incidentally, register PREPARED notifications after creating the wave
     * bank but before DoWork(), to show that they're still received in this
     * case. */

    prepared_data.count = destroyed_data2.count = 0;

    destroyed_data_once.thread_id = GetCurrentThreadId();
    destroyed_data_once.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    notification_desc.flags = 0;
    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    notification_desc.pvContext = &destroyed_data_once;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(wavebank2 != wavebank, "Expected different wavebanks.\n");
    ok(!prepared_data.count, "Got %u notifications.\n", prepared_data.count);

    prepared_data_once.thread_id = GetCurrentThreadId();
    prepared_data_once.type = XACTNOTIFICATIONTYPE_WAVEBANKPREPARED;
    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKPREPARED;
    notification_desc.pvContext = &prepared_data_once;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    prepared_data.timestamp_before = get_current_time();
    prepared_data_once.timestamp_before = get_current_time();
    hr = IXACT3Engine_DoWork(engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(prepared_data.count == 1, "Got %u notifications.\n", prepared_data.count);
    ok(prepared_data.wave_bank == wavebank2, "Wavebanks didn't match.\n");
    ok(prepared_data_once.count == 1, "Got %u notifications.\n", prepared_data_once.count);
    ok(prepared_data_once.wave_bank == wavebank, "Wavebanks didn't match.\n");

    destroyed_data_once.timestamp_before = get_current_time();
    destroyed_data_once.wave_bank = wavebank2;
    hr = IXACT3WaveBank_Destroy(wavebank2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(destroyed_data_once.count == 1, "Got %u notifications.\n", destroyed_data_once.count);
    ok(!destroyed_data2.count, "Got %u notifications.\n", destroyed_data2.count);

    /* Non-persistent notifications are not given priority. */

    destroyed_data.count = destroyed_data_once.count = 0;

    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    notification_desc.flags = 0;
    notification_desc.pvContext = &destroyed_data_once;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    notification_desc.pvContext = &destroyed_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    destroyed_data.wave_bank = destroyed_data_once.wave_bank = wavebank;
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(destroyed_data.count == 1, "Got %u notifications.\n", destroyed_data.count);
    ok(destroyed_data_once.count == 0, "Got %u notifications.\n", destroyed_data_once.count);

    /* UnregisterNotification(NULL) is undocumented, but actually unregisters
     * all notifications, both persistent and non-persistent. */

    destroyed_data.count = destroyed_data_once.count = 0;

    hr = IXACT3Engine_UnRegisterNotification(engine, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(destroyed_data.count == 0, "Got %u notifications.\n", destroyed_data.count);
    ok(destroyed_data_once.count == 0, "Got %u notifications.\n", destroyed_data_once.count);
}

    /* Test filtering.
     *
     * We set pSoundBank on the wavebank notification to show that irrelevant
     * members are simply ignored. */

    /* Despite the parameter here being const, native will attempt to overwrite
     * some of this data. */
    soundbank_data_copy = malloc(sizeof(soundbank_data));
    memcpy(soundbank_data_copy, soundbank_data, sizeof(soundbank_data));
    hr = IXACT3Engine_CreateSoundBank(engine, soundbank_data_copy, sizeof(soundbank_data), 0, 0, &soundbank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    destroyed_data2.count = 0;
    destroyed_data2.wave_bank = wavebank2;
    destroyed_data.count = 0;
    destroyed_data.wave_bank = wavebank;

    destroyed_data2.todo_wave_bank = TRUE;

    notification_desc.type = XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED;
    notification_desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    notification_desc.pvContext = &destroyed_data;
    notification_desc.pSoundBank = soundbank;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    notification_desc.pvContext = &destroyed_data2;
    notification_desc.pWaveBank = wavebank2;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_DoWork(engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    destroyed_data.timestamp_before = get_current_time();
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(destroyed_data.count == 1, "Got %u notifications.\n", destroyed_data.count);
    todo_wine ok(!destroyed_data2.count, "Got %u notifications.\n", destroyed_data2.count);
    destroyed_data2.todo_wave_bank = FALSE;
    hr = IXACT3WaveBank_Destroy(wavebank2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(destroyed_data.count == 1, "Got %u notifications.\n", destroyed_data.count);
    todo_wine ok(destroyed_data2.count == 1, "Got %u notifications.\n", destroyed_data2.count);

    /* UnRegisterNotification requires that the filters match too, including
     * the ones that don't matter. */
    destroyed_data.count = destroyed_data2.count = 0;

    notification_desc.pWaveBank = NULL;
    notification_desc.pvContext = &destroyed_data;
    notification_desc.pSoundBank = NULL;
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    destroyed_data.wave_bank = wavebank;
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(destroyed_data.count == 1, "Got %u notifications.\n", destroyed_data.count);
    ok(!destroyed_data2.count, "Got %u notifications.\n", destroyed_data2.count);

    destroyed_data.count = destroyed_data2.count = 0;

    notification_desc.pSoundBank = soundbank;
    hr = IXACT3Engine_UnRegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3WaveBank_Destroy(wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!destroyed_data.count, "Got %u notifications.\n", destroyed_data.count);
    ok(!destroyed_data2.count, "Got %u notifications.\n", destroyed_data2.count);

    CloseHandle(file);
    refcount = IXACT3Engine_Release(engine);
    todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);

    ret = DeleteFileW(path);
    ok(ret == TRUE, "Got error %lu.\n", GetLastError());
}

static void test_renderer_details(void)
{
    XAUDIO2_DEVICE_DETAILS xaudio_details;
    XACT_RENDERER_DETAILS details;
    WAVEFORMATEXTENSIBLE format;
    IXACT3Engine *engine;
    UINT32 xaudio_count;
    IXAudio2 *xaudio;
    XACTINDEX count;
    ULONG refcount;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XAudio2, NULL, CLSCTX_INPROC_SERVER, &IID_IXAudio2, (void **)&xaudio);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXAudio2_GetDeviceCount(xaudio, &xaudio_count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_GetRendererCount(engine, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == xaudio_count, "Expected count %u, got %u.\n", xaudio_count, count);

    for (XACTINDEX i = 0; i < count; ++i)
    {
        winetest_push_context("renderer %u", i);

        hr = IXACT3Engine_GetRendererDetails(engine, i, &details);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IXAudio2_GetDeviceDetails(xaudio, i, &xaudio_details);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(!wcscmp(details.displayName, xaudio_details.DisplayName),
                "Expected %s, got %s.\n", debugstr_w(xaudio_details.DisplayName), debugstr_w(details.displayName));
        ok(!wcscmp(details.rendererID, xaudio_details.DeviceID),
                "Expected %s, got %s.\n", debugstr_w(xaudio_details.DeviceID), debugstr_w(details.rendererID));
        ok(details.defaultDevice == !i, "Got default device %d.\n", details.defaultDevice);

        winetest_pop_context();
    }

    hr = IXACT3Engine_GetRendererDetails(engine, count, &details);
    todo_wine ok(hr == XAUDIO2_E_INVALID_CALL, "Got hr %#lx.\n", hr);

    refcount = IXACT3Engine_Release(engine);
    todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);

    for (XACTINDEX i = 0; i < count; ++i)
    {
        XACT_RUNTIME_PARAMETERS params = {.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT};

        winetest_push_context("renderer %u", i);

        hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IXAudio2_GetDeviceDetails(xaudio, i, &xaudio_details);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        params.pRendererID = xaudio_details.DeviceID;
        hr = IXACT3Engine_Initialize(engine, &params);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IXACT3Engine_GetFinalMixFormat(engine, &format);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        todo_wine ok(!memcmp(&format, &xaudio_details.OutputFormat, sizeof(WAVEFORMATEXTENSIBLE)), "Formats didn't match.\n");

        refcount = IXACT3Engine_Release(engine);
        todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);

        winetest_pop_context();
    }

    refcount = IXAudio2_Release(xaudio);
    ok(!refcount, "Got outstanding refcount %ld.\n", refcount);
}

static void test_properties(void)
{
    XACT_RUNTIME_PARAMETERS params = {.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT};
    const XACT_VARIATION_PROPERTIES *var_props;
    const XACT_SOUND_PROPERTIES *sound_props;
    const XACT_TRACK_PROPERTIES *track_props;
    XACT_WAVE_INSTANCE_PROPERTIES wave_props;
    XACT_CUE_INSTANCE_PROPERTIES *props;
    XACT_CUE_PROPERTIES cue_props;
    IXACT3SoundBank *soundbank;
    void *soundbank_data_copy;
    IXACT3WaveBank *wavebank;
    XACTVARIABLEVALUE value;
    XACTINDEX count, index;
    IXACT3Cue *cue, *cue2;
    IXACT3Engine *engine;
    IXACT3Wave *wave;
    ULONG refcount;
    DWORD state;
    HRESULT hr;
    HRSRC res;

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    params.pGlobalSettingsBuffer = (void *)global_settings_data;
    params.globalSettingsBufferSize = sizeof(global_settings_data);
    hr = IXACT3Engine_Initialize(engine, &params);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    res = FindResourceW(NULL, L"test.xwb", (const WCHAR *)RT_RCDATA);
    ok(!!res, "Got error %lu.\n", GetLastError());

    hr = IXACT3Engine_CreateInMemoryWaveBank(engine,
            LockResource(LoadResource(GetModuleHandleA(NULL), res)),
            SizeofResource(GetModuleHandleA(NULL), res), 0, 0, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Despite the parameter here being const, native will attempt to overwrite
     * some of this data. */
    soundbank_data_copy = malloc(sizeof(soundbank_data));
    memcpy(soundbank_data_copy, soundbank_data, sizeof(soundbank_data));
    hr = IXACT3Engine_CreateSoundBank(engine, soundbank_data_copy, sizeof(soundbank_data), 0, 0, &soundbank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3SoundBank_GetState(soundbank, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(!state, "Got state %#lx.\n", state);

    hr = IXACT3SoundBank_GetNumCues(soundbank, &count);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(count == 3, "Got count %u.\n", count);

    memset(&cue_props, 0xcc, sizeof(cue_props));
    hr = IXACT3SoundBank_GetCueProperties(soundbank, 1, &cue_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!strcmp(cue_props.friendlyName, "c1"), "Got name %s.\n", debugstr_a(cue_props.friendlyName));
    ok(!cue_props.interactive, "Got interactive %d.\n", cue_props.interactive);
    todo_wine ok(cue_props.iaVariableIndex == XACTINDEX_INVALID, "Got variable index %u.\n", cue_props.iaVariableIndex);
    ok(cue_props.numVariations == 2, "Got %u variations.\n", cue_props.numVariations);
    ok(cue_props.maxInstances == XACTINSTANCELIMIT_INFINITE, "Got max instances %u.\n", cue_props.maxInstances);
    ok(!cue_props.currentInstances, "Got current instances %u.\n", cue_props.currentInstances);

    memset(&cue_props, 0xcc, sizeof(cue_props));
    hr = IXACT3SoundBank_GetCueProperties(soundbank, 2, &cue_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!strcmp(cue_props.friendlyName, "c2"), "Got name %s.\n", debugstr_a(cue_props.friendlyName));
    ok(cue_props.interactive == TRUE, "Got interactive %d.\n", cue_props.interactive);
    ok(cue_props.iaVariableIndex == 7, "Got variable index %u.\n", cue_props.iaVariableIndex);
    ok(cue_props.numVariations == 2, "Got %u variations.\n", cue_props.numVariations);
    ok(cue_props.maxInstances == 11, "Got max instances %u.\n", cue_props.maxInstances);
    ok(!cue_props.currentInstances, "Got current instances %u.\n", cue_props.currentInstances);

    memset(&cue_props, 0xcc, sizeof(cue_props));
    hr = IXACT3SoundBank_GetCueProperties(soundbank, 0, &cue_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!strcmp(cue_props.friendlyName, "c3"), "Got name %s.\n", debugstr_a(cue_props.friendlyName));
    ok(!cue_props.interactive, "Got interactive %d.\n", cue_props.interactive);
    todo_wine ok(cue_props.iaVariableIndex == XACTINDEX_INVALID, "Got variable index %u.\n", cue_props.iaVariableIndex);
    todo_wine ok(cue_props.numVariations == 1, "Got %u variations.\n", cue_props.numVariations);
    ok(cue_props.maxInstances == XACTINSTANCELIMIT_INFINITE, "Got max instances %u.\n", cue_props.maxInstances);
    ok(!cue_props.currentInstances, "Got current instances %u.\n", cue_props.currentInstances);

    hr = IXACT3SoundBank_Prepare(soundbank, 1, 0, 0, &cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3SoundBank_Prepare(soundbank, 1, 0, 0, &cue2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(cue2 != cue, "Expected different cues.\n");

    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_PREPARED, "Got state %#lx.\n", state);

    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(props->allocAttributes == 0x1, "Got alloc attributes %#lx.\n", props->allocAttributes);
    ok(!strcmp(props->cueProperties.friendlyName, "c1"),
            "Got name %s.\n", debugstr_a(props->cueProperties.friendlyName));
    ok(!props->cueProperties.interactive, "Got interactive %d.\n", props->cueProperties.interactive);
    todo_wine ok(props->cueProperties.iaVariableIndex == XACTINDEX_INVALID,
            "Got variable index %u.\n", props->cueProperties.iaVariableIndex);
    ok(props->cueProperties.numVariations == 2, "Got %u variations.\n", props->cueProperties.numVariations);
    ok(props->cueProperties.maxInstances == XACTINSTANCELIMIT_INFINITE,
            "Got max instances %u.\n", props->cueProperties.maxInstances);
    ok(!props->cueProperties.currentInstances, "Got current instances %u.\n", props->cueProperties.currentInstances);
    var_props = &props->activeVariationProperties.variationProperties;
    ok(!var_props->index, "Got variation index %u.\n", var_props->index);
    todo_wine ok(var_props->weight == 20, "Got variation weight %u.\n", var_props->weight);
    ok(!var_props->iaVariableMin, "Got variable min %.8e.\n", var_props->iaVariableMin);
    ok(!var_props->iaVariableMax, "Got variable max %.8e.\n", var_props->iaVariableMax);
    ok(!var_props->linger, "Got linger %u.\n", var_props->linger);
    /* The sound properties are already filled, for the first sound variation
     * which will play. Since our cue has the variation type 0 (ordered) this
     * will always be its first sound variation. */
    sound_props = &props->activeVariationProperties.soundProperties;
    todo_wine ok(sound_props->category == 1, "Got category %u.\n", sound_props->category);
    ok(!sound_props->priority, "Got priority %u.\n", sound_props->priority);
    /* Pitch isn't quite consistent, and probably varies due to floating point
     * accuracy. */
    todo_wine ok(sound_props->pitch == 65 || sound_props->pitch == 66, "Got pitch %d.\n", sound_props->pitch);
    todo_wine ok(sound_props->volume == 0.237621084f, "Got volume %.8e.\n", sound_props->volume);
    todo_wine ok(sound_props->numTracks == 2, "Got %u tracks.\n", sound_props->numTracks);
    if (sound_props->numTracks == 2)
    {
        track_props = &sound_props->arrTrackProperties[0];
        ok(track_props->numVariations == 2, "Got %u variations.\n", track_props->numVariations);
        ok(track_props->numChannels == 1, "Got %u channels.\n", track_props->numChannels);
        /* Similarly the wave variation is filled for the first wave variation
         * which will play, and we use the ordered type here. */
        ok(!track_props->waveVariation, "Got active variation %u.\n", track_props->waveVariation);
        /* The duration reflects the loop count multiplied by the track length.
         * This would seem to be a bug in native, since the event has the "new
         * variation on loop" flag set and so we should presumably get the
         * second wave variation on the second loop. That said, when manually
         * testing I was unable to get the track to actually play more than
         * once. */
        ok(track_props->duration == 300 * 3, "Got duration %lu.\n", track_props->duration);
        ok(track_props->loopCount == 2, "Got loop count %u.\n", track_props->loopCount);
        track_props = &sound_props->arrTrackProperties[1];
        ok(track_props->duration == 522, "Got duration %lu.\n", track_props->duration);
        ok(track_props->numVariations == 1, "Got %u variations.\n", track_props->numVariations);
        ok(track_props->numChannels == 2, "Got %u channels.\n", track_props->numChannels);
        ok(!track_props->waveVariation, "Got active variation %u.\n", track_props->waveVariation);
        ok(!track_props->loopCount, "Got loop count %u.\n", track_props->loopCount);
    }
    CoTaskMemFree(props);

    hr = IXACT3Cue_GetProperties(cue2, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!props->cueProperties.currentInstances, "Got current instances %u.\n", props->cueProperties.currentInstances);
    var_props = &props->activeVariationProperties.variationProperties;
    todo_wine ok(var_props->index == 1, "Got variation index %u.\n", var_props->index);
    todo_wine ok(var_props->weight == 60, "Got variation weight %u.\n", var_props->weight);
    ok(!var_props->iaVariableMin, "Got variable min %.8e.\n", var_props->iaVariableMin);
    ok(!var_props->iaVariableMax, "Got variable max %.8e.\n", var_props->iaVariableMax);
    ok(!var_props->linger, "Got linger %u.\n", var_props->linger);
    sound_props = &props->activeVariationProperties.soundProperties;
    todo_wine ok(sound_props->category == 2, "Got category %u.\n", sound_props->category);
    ok(!sound_props->priority, "Got priority %u.\n", sound_props->priority);
    todo_wine ok(sound_props->pitch == -8, "Got pitch %d.\n", sound_props->pitch);
    todo_wine ok(sound_props->volume == 1.66583312f, "Got volume %.8e.\n", sound_props->volume);
    todo_wine ok(sound_props->numTracks == 1, "Got %u tracks.\n", sound_props->numTracks);
    if (sound_props->numTracks)
    {
        track_props = &sound_props->arrTrackProperties[0];
        ok(track_props->numVariations == 1, "Got %u variations.\n", track_props->numVariations);
        ok(track_props->numChannels == 1, "Got %u channels.\n", track_props->numChannels);
        ok(!track_props->waveVariation, "Got active variation %u.\n", track_props->waveVariation);
        ok(track_props->duration == 400, "Got duration %lu.\n", track_props->duration);
        ok(!track_props->loopCount, "Got loop count %u.\n", track_props->loopCount);
    }
    CoTaskMemFree(props);

    hr = IXACT3Cue_Pause(cue, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == (XACT_STATE_PREPARED | XACT_STATE_PAUSED), "Got state %#lx.\n", state);

    hr = IXACT3Cue_Play(cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(state == (XACT_STATE_PLAYING | XACT_STATE_PAUSED), "Got state %#lx.\n", state);

    hr = IXACT3Cue_Play(cue);
    todo_wine ok(hr == XACTENGINE_E_INVALIDUSAGE, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(props->cueProperties.currentInstances == 1, "Got current instances %u.\n", props->cueProperties.currentInstances);
    var_props = &props->activeVariationProperties.variationProperties;
    ok(!var_props->index, "Got variation index %u.\n", var_props->index);
    CoTaskMemFree(props);

    hr = IXACT3Cue_GetProperties(cue2, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(props->cueProperties.currentInstances == 1, "Got current instances %u.\n", props->cueProperties.currentInstances);
    var_props = &props->activeVariationProperties.variationProperties;
    todo_wine ok(var_props->index == 1, "Got variation index %u.\n", var_props->index);
    CoTaskMemFree(props);

    hr = IXACT3Cue_Play(cue2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_Pause(cue2, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_Pause(cue2, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_GetState(cue2, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == (XACT_STATE_PLAYING | XACT_STATE_PAUSED), "Got state %#lx.\n", state);
    hr = IXACT3Cue_Play(cue2);
    todo_wine ok(hr == XACTENGINE_E_INVALIDUSAGE, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(props->cueProperties.currentInstances == 2, "Got current instances %u.\n", props->cueProperties.currentInstances);
    var_props = &props->activeVariationProperties.variationProperties;
    ok(!var_props->index, "Got variation index %u.\n", var_props->index);
    CoTaskMemFree(props);

    hr = IXACT3Cue_Destroy(cue2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(props->cueProperties.currentInstances == 1, "Got current instances %u.\n", props->cueProperties.currentInstances);
    CoTaskMemFree(props);

    index = IXACT3Engine_GetGlobalVariableIndex(engine, "v1");
    ok(index == 7, "Got index %u.\n", index);
    value = -1.0f;
    hr = IXACT3Engine_GetGlobalVariable(engine, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(value == 10.0f, "Got value %.8e.\n", value);

    hr = IXACT3Cue_Stop(cue, XACT_FLAG_STOP_IMMEDIATE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_Stop(cue, XACT_FLAG_STOP_IMMEDIATE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    /* Despite STOP_IMMEDIATE we're not STOPPED until the render thread notices. */
    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_STOPPING || state == XACT_STATE_STOPPED, "Got state %#lx.\n", state);

    hr = IXACT3Cue_Play(cue);
    todo_wine ok(hr == XACTENGINE_E_INVALIDUSAGE, "Got hr %#lx.\n", hr);

    /* Third instance of this cue: we've now alternated back to the first sound
     * variation, and since this is the second instance of that variation we
     * alternate to the second wave variation. */
    hr = IXACT3SoundBank_Prepare(soundbank, 1, 0, 0, &cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!strcmp(props->cueProperties.friendlyName, "c1"),
            "Got name %s.\n", debugstr_a(props->cueProperties.friendlyName));
    var_props = &props->activeVariationProperties.variationProperties;
    ok(!var_props->index, "Got variation index %u.\n", var_props->index);
    sound_props = &props->activeVariationProperties.soundProperties;
    todo_wine ok(sound_props->numTracks == 2, "Got %u tracks.\n", sound_props->numTracks);
    if (sound_props->numTracks == 2)
    {
        track_props = &sound_props->arrTrackProperties[0];
        ok(track_props->numVariations == 2, "Got %u variations.\n", track_props->numVariations);
        ok(track_props->numChannels == 1, "Got %u channels.\n", track_props->numChannels);
        ok(track_props->waveVariation == 1, "Got active variation %u.\n", track_props->waveVariation);
        ok(track_props->duration == 400 * 3, "Got duration %lu.\n", track_props->duration);
        ok(track_props->loopCount == 2, "Got loop count %u.\n", track_props->loopCount);
        track_props = &sound_props->arrTrackProperties[1];
        ok(track_props->duration == 522, "Got duration %lu.\n", track_props->duration);
        ok(track_props->numVariations == 1, "Got %u variations.\n", track_props->numVariations);
        ok(track_props->numChannels == 2, "Got %u channels.\n", track_props->numChannels);
        ok(!track_props->waveVariation, "Got active variation %u.\n", track_props->waveVariation);
        ok(!track_props->loopCount, "Got loop count %u.\n", track_props->loopCount);
    }
    CoTaskMemFree(props);

    hr = IXACT3SoundBank_Prepare(soundbank, 2, 0, 0, &cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_PREPARED, "Got state %#lx.\n", state);

    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(props->allocAttributes == 0x1, "Got alloc attributes %#lx.\n", props->allocAttributes);
    ok(!strcmp(props->cueProperties.friendlyName, "c2"),
            "Got name %s.\n", debugstr_a(props->cueProperties.friendlyName));
    ok(props->cueProperties.interactive == TRUE, "Got interactive %d.\n", props->cueProperties.interactive);
    ok(props->cueProperties.iaVariableIndex == 7,
            "Got variable index %u.\n", props->cueProperties.iaVariableIndex);
    ok(props->cueProperties.numVariations == 2, "Got %u variations.\n", props->cueProperties.numVariations);
    ok(props->cueProperties.maxInstances == 11, "Got max instances %u.\n", props->cueProperties.maxInstances);
    ok(!props->cueProperties.currentInstances, "Got current instances %u.\n", props->cueProperties.currentInstances);
    /* Unlike the non-interactive case, the variation properties are *not*
     * filled before playing. In fact, they're not filled immediately after
     * Play() either, but only at a delay. DoWork() seems to be necessary, but
     * unlike with wavebank notifications, isn't sufficient either.
     * Unfortunately this makes it pretty much impossible to reliably test.
     *
     * The actual returned properties are filled as one would expect.
     * "weight" remains zero.
     *
     * If the variable is set to a value that's not in any range, the variation
     * properties remain unchanged (INVALID and zeroes), but currentInstances
     * is still 1.
     */
    var_props = &props->activeVariationProperties.variationProperties;
    todo_wine ok(var_props->index == XACTVARIABLEINDEX_INVALID, "Got variation index %u.\n", var_props->index);
    ok(!var_props->weight, "Got variation weight %u.\n", var_props->weight);
    ok(!var_props->iaVariableMin, "Got variable min %.8e.\n", var_props->iaVariableMin);
    ok(!var_props->iaVariableMax, "Got variable max %.8e.\n", var_props->iaVariableMax);
    ok(!var_props->linger, "Got linger %u.\n", var_props->linger);
    sound_props = &props->activeVariationProperties.soundProperties;
    todo_wine ok(sound_props->category == XACTCATEGORY_INVALID, "Got category %u.\n", sound_props->category);
    ok(!sound_props->priority, "Got priority %u.\n", sound_props->priority);
    todo_wine ok(sound_props->pitch == XACTPITCH_MIN, "Got pitch %d.\n", sound_props->pitch);
    ok(!sound_props->volume, "Got volume %.8e.\n", sound_props->volume);
    ok(!sound_props->numTracks, "Got %u tracks.\n", sound_props->numTracks);
    CoTaskMemFree(props);

    /* The instance count at least is updated immediately. */
    hr = IXACT3Cue_Play(cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(props->cueProperties.currentInstances == 1, "Got current instances %u.\n", props->cueProperties.currentInstances);

    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_PLAYING, "Got state %#lx.\n", state);
    hr = IXACT3Cue_Play(cue);
    todo_wine ok(hr == XACTENGINE_E_INVALIDUSAGE, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_Stop(cue, XACT_FLAG_STOP_IMMEDIATE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_STOPPING || state == XACT_STATE_STOPPED, "Got state %#lx.\n", state);
    hr = IXACT3Cue_Play(cue);
    todo_wine ok(hr == XACTENGINE_E_INVALIDUSAGE, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_Destroy(cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (unsigned int i = 0; i < 12; ++i)
    {
        hr = IXACT3SoundBank_Prepare(soundbank, 2, 0, 0, &cue);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IXACT3Cue_Play(cue);
        todo_wine_if (i >= 10) ok(hr == (i < 11 ? S_OK : XACTENGINE_E_INSTANCELIMITFAILTOPLAY), "Got hr %#lx.\n", hr);
        hr = IXACT3Cue_GetProperties(cue, &props);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        todo_wine_if (i < 10) ok(props->cueProperties.currentInstances == min(i + 1, 11),
                "Got current instances %u.\n", props->cueProperties.currentInstances);
    }

    /* Test a cue which has only one sound, and is therefore encoded
     * differently. */
    hr = IXACT3SoundBank_Prepare(soundbank, 0, 0, 0, &cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Cue_GetState(cue, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_PREPARED, "Got state %#lx.\n", state);

    hr = IXACT3Cue_GetProperties(cue, &props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(props->allocAttributes == 0x1, "Got alloc attributes %#lx.\n", props->allocAttributes);
    ok(!strcmp(props->cueProperties.friendlyName, "c3"),
            "Got name %s.\n", debugstr_a(props->cueProperties.friendlyName));
    ok(!props->cueProperties.interactive, "Got interactive %d.\n", props->cueProperties.interactive);
    todo_wine ok(props->cueProperties.iaVariableIndex == XACTINDEX_INVALID,
            "Got variable index %u.\n", props->cueProperties.iaVariableIndex);
    todo_wine ok(props->cueProperties.numVariations == 1, "Got %u variations.\n", props->cueProperties.numVariations);
    ok(props->cueProperties.maxInstances == XACTINSTANCELIMIT_INFINITE,
            "Got max instances %u.\n", props->cueProperties.maxInstances);
    ok(!props->cueProperties.currentInstances, "Got current instances %u.\n", props->cueProperties.currentInstances);
    var_props = &props->activeVariationProperties.variationProperties;
    ok(!var_props->index, "Got variation index %u.\n", var_props->index);
    todo_wine ok(var_props->weight == 0xff, "Got variation weight %u.\n", var_props->weight);
    ok(!var_props->iaVariableMin, "Got variable min %.8e.\n", var_props->iaVariableMin);
    ok(!var_props->iaVariableMax, "Got variable max %.8e.\n", var_props->iaVariableMax);
    ok(!var_props->linger, "Got linger %u.\n", var_props->linger);
    sound_props = &props->activeVariationProperties.soundProperties;
    todo_wine ok(sound_props->category == 2, "Got category %u.\n", sound_props->category);
    ok(!sound_props->priority, "Got priority %u.\n", sound_props->priority);
    todo_wine ok(sound_props->pitch == -8, "Got pitch %d.\n", sound_props->pitch);
    todo_wine ok(sound_props->volume == 1.66583312f, "Got volume %.8e.\n", sound_props->volume);
    todo_wine ok(sound_props->numTracks == 1, "Got %u tracks.\n", sound_props->numTracks);
    if (sound_props->numTracks)
    {
        track_props = &sound_props->arrTrackProperties[0];
        ok(track_props->numVariations == 1, "Got %u variations.\n", track_props->numVariations);
        ok(track_props->numChannels == 1, "Got %u channels.\n", track_props->numChannels);
        ok(!track_props->waveVariation, "Got active variation %u.\n", track_props->waveVariation);
        ok(track_props->duration == 400, "Got duration %lu.\n", track_props->duration);
        ok(!track_props->loopCount, "Got loop count %u.\n", track_props->loopCount);
    }
    CoTaskMemFree(props);

    memset(&wave_props, 0xcc, sizeof(wave_props));
    hr = IXACT3WaveBank_GetWaveProperties(wavebank, 0, &wave_props.properties);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(wave_props.properties.friendlyName[0] == (char)0xcc,
            "Got name %s.\n", debugstr_a(wave_props.properties.friendlyName));
    ok(wave_props.properties.format.wFormatTag == WAVEBANKMINIFORMAT_TAG_PCM,
            "Got format %u.\n", wave_props.properties.format.wFormatTag);
    ok(wave_props.properties.format.nChannels == 1,
            "Got %u channels.\n", wave_props.properties.format.nChannels);
    ok(wave_props.properties.format.nSamplesPerSec == 44100,
            "Got sample rate %u.\n", wave_props.properties.format.nSamplesPerSec);
    ok(wave_props.properties.format.wBlockAlign == 1,
            "Got alignment %u.\n", wave_props.properties.format.wBlockAlign);
    ok(wave_props.properties.format.wBitsPerSample == WAVEBANKMINIFORMAT_BITDEPTH_8,
            "Got bit depth %u.\n", wave_props.properties.format.wBitsPerSample);
    ok(wave_props.properties.durationInSamples == 44100 * 3 / 10,
            "Got duration %lu.\n", wave_props.properties.durationInSamples);
    ok(!wave_props.properties.loopRegion.dwStartSample,
            "Got loop start %lu.\n", wave_props.properties.loopRegion.dwStartSample);
    ok(!wave_props.properties.loopRegion.dwTotalSamples,
            "Got loop length %lu.\n", wave_props.properties.loopRegion.dwTotalSamples);
    ok(!wave_props.properties.streaming, "Got streaming %d.\n", wave_props.properties.streaming);

    memset(&wave_props, 0xcc, sizeof(wave_props));
    hr = IXACT3WaveBank_GetWaveProperties(wavebank, 1, &wave_props.properties);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(wave_props.properties.friendlyName[0] == (char)0xcc,
            "Got name %s.\n", debugstr_a(wave_props.properties.friendlyName));
    ok(wave_props.properties.format.wFormatTag == WAVEBANKMINIFORMAT_TAG_ADPCM,
            "Got format %u.\n", wave_props.properties.format.wFormatTag);
    ok(wave_props.properties.format.nChannels == 1,
            "Got %u channels.\n", wave_props.properties.format.nChannels);
    ok(wave_props.properties.format.nSamplesPerSec == 44080,
            "Got sample rate %u.\n", wave_props.properties.format.nSamplesPerSec);
    ok(!wave_props.properties.format.wBlockAlign,
            "Got alignment %u.\n", wave_props.properties.format.wBlockAlign);
    ok(!wave_props.properties.format.wBitsPerSample,
            "Got bit depth %u.\n", wave_props.properties.format.wBitsPerSample);
    ok(wave_props.properties.durationInSamples == 44080 * 4 / 10,
            "Got duration %lu.\n", wave_props.properties.durationInSamples);
    ok(!wave_props.properties.loopRegion.dwStartSample,
            "Got loop start %lu.\n", wave_props.properties.loopRegion.dwStartSample);
    ok(wave_props.properties.loopRegion.dwTotalSamples == 44080 * 4 / 10,
            "Got loop length %lu.\n", wave_props.properties.loopRegion.dwTotalSamples);
    ok(!wave_props.properties.streaming, "Got streaming %d.\n", wave_props.properties.streaming);

    memset(&wave_props, 0xcc, sizeof(wave_props));
    hr = IXACT3WaveBank_GetWaveProperties(wavebank, 2, &wave_props.properties);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(wave_props.properties.friendlyName[0] == (char)0xcc,
            "Got name %s.\n", debugstr_a(wave_props.properties.friendlyName));
    ok(wave_props.properties.format.wFormatTag == WAVEBANKMINIFORMAT_TAG_WMA,
            "Got format %u.\n", wave_props.properties.format.wFormatTag);
    ok(wave_props.properties.format.nChannels == 2,
            "Got %u channels.\n", wave_props.properties.format.nChannels);
    ok(wave_props.properties.format.nSamplesPerSec == 44100,
            "Got sample rate %u.\n", wave_props.properties.format.nSamplesPerSec);
    ok(wave_props.properties.format.wBlockAlign == 99,
            "Got alignment %u.\n", wave_props.properties.format.wBlockAlign);
    ok(!wave_props.properties.format.wBitsPerSample,
            "Got bit depth %u.\n", wave_props.properties.format.wBitsPerSample);
    todo_wine ok(wave_props.properties.durationInSamples == 23040,
            "Got duration %lu.\n", wave_props.properties.durationInSamples);
    ok(!wave_props.properties.loopRegion.dwStartSample,
            "Got loop start %lu.\n", wave_props.properties.loopRegion.dwStartSample);
    ok(!wave_props.properties.loopRegion.dwTotalSamples,
            "Got loop length %lu.\n", wave_props.properties.loopRegion.dwTotalSamples);
    ok(!wave_props.properties.streaming, "Got streaming %d.\n", wave_props.properties.streaming);

    hr = IXACT3WaveBank_Prepare(wavebank, 0, XACT_FLAG_UNITS_MS, 100, 2, &wave);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetProperties(wave, &wave_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(wave_props.properties.friendlyName[0] == (char)0xcc,
            "Got name %s.\n", debugstr_a(wave_props.properties.friendlyName));
    ok(wave_props.properties.format.wFormatTag == WAVEBANKMINIFORMAT_TAG_PCM,
            "Got format %u.\n", wave_props.properties.format.wFormatTag);
    ok(wave_props.properties.format.nChannels == 1,
            "Got %u channels.\n", wave_props.properties.format.nChannels);
    ok(wave_props.properties.format.nSamplesPerSec == 44100,
            "Got sample rate %u.\n", wave_props.properties.format.nSamplesPerSec);
    ok(wave_props.properties.format.wBlockAlign == 1,
            "Got alignment %u.\n", wave_props.properties.format.wBlockAlign);
    ok(wave_props.properties.format.wBitsPerSample == WAVEBANKMINIFORMAT_BITDEPTH_8,
            "Got bit depth %u.\n", wave_props.properties.format.wBitsPerSample);
    ok(wave_props.properties.durationInSamples == 44100 * 3 / 10,
            "Got duration %lu.\n", wave_props.properties.durationInSamples);
    ok(!wave_props.properties.loopRegion.dwStartSample,
            "Got loop start %lu.\n", wave_props.properties.loopRegion.dwStartSample);
    ok(!wave_props.properties.loopRegion.dwTotalSamples,
            "Got loop length %lu.\n", wave_props.properties.loopRegion.dwTotalSamples);
    ok(!wave_props.properties.streaming, "Got streaming %d.\n", wave_props.properties.streaming);
    ok(!wave_props.backgroundMusic, "Got background music %d.\n", wave_props.backgroundMusic);

    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_PREPARED, "Got state %#lx.\n", state);

    hr = IXACT3Wave_Play(wave);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    /* Calling GetState() at this point might still yield PREPARED; it doesn't
     * switch to PLAYING until the render thread notices.
     * Calling Play() while PLAYING is illegal, same as for cues, but calling
     * Play() twice before the render thread notices is fine. */

if (!winetest_platform_is_wine)
{
    hr = IXACT3Wave_Stop(wave, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_STOPPING || state == XACT_STATE_STOPPED, "Got state %#lx.\n", state);
    hr = IXACT3Wave_Play(wave);
    ok(hr == XACTENGINE_E_INVALIDUSAGE, "Got hr %#lx.\n", hr);

    hr = IXACT3Wave_Stop(wave, XACT_FLAG_STOP_IMMEDIATE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_Stop(wave, XACT_FLAG_STOP_IMMEDIATE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_STOPPING || state == XACT_STATE_STOPPED, "Got state %#lx.\n", state);
}

    hr = IXACT3WaveBank_Prepare(wavebank, 1, XACT_FLAG_UNITS_MS, 100, 2, &wave);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetProperties(wave, &wave_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(wave_props.properties.format.wFormatTag == WAVEBANKMINIFORMAT_TAG_ADPCM,
            "Got format %u.\n", wave_props.properties.format.wFormatTag);
    ok(!wave_props.backgroundMusic, "Got background music %d.\n", wave_props.backgroundMusic);

    hr = IXACT3Wave_Pause(wave, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == (XACT_STATE_PREPARED | XACT_STATE_PAUSED), "Got state %#lx.\n", state);
    hr = IXACT3Wave_Pause(wave, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == XACT_STATE_PREPARED, "Got state %#lx.\n", state);

    hr = IXACT3Wave_Pause(wave, TRUE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == (XACT_STATE_PREPARED | XACT_STATE_PAUSED), "Got state %#lx.\n", state);
    /* Unlike with cues, Play() while PREPARED and PAUSED does nothing.
     * Even if you unpause and wait, it'll remain in PREPARED. */
    hr = IXACT3Wave_Play(wave);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(state == (XACT_STATE_PREPARED | XACT_STATE_PAUSED), "Got state %#lx.\n", state);
    hr = IXACT3Wave_Pause(wave, FALSE);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetState(wave, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(state == XACT_STATE_PREPARED, "Got state %#lx.\n", state);

    hr = IXACT3WaveBank_Prepare(wavebank, 2, XACT_FLAG_UNITS_MS, 100, 2, &wave);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetProperties(wave, &wave_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(wave_props.properties.format.wFormatTag == WAVEBANKMINIFORMAT_TAG_WMA,
            "Got format %u.\n", wave_props.properties.format.wFormatTag);
    ok(!wave_props.backgroundMusic, "Got background music %d.\n", wave_props.backgroundMusic);

    /* The background music flag is documented as being ignored, but it's at
     * least still recorded and propagated to the properties. */
    hr = IXACT3WaveBank_Prepare(wavebank, 2, XACT_FLAG_UNITS_MS | XACT_FLAG_BACKGROUND_MUSIC, 100, 2, &wave);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Wave_GetProperties(wave, &wave_props);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(wave_props.backgroundMusic == TRUE, "Got background music %d.\n", wave_props.backgroundMusic);

    refcount = IXACT3Engine_Release(engine);
    todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);
}

static void test_variables(void)
{
    XACT_RUNTIME_PARAMETERS params = {.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT};
    IXACT3SoundBank *soundbank;
    void *soundbank_data_copy;
    IXACT3WaveBank *wavebank;
    XACTVARIABLEVALUE value;
    IXACT3Engine *engine;
    XACTINDEX index;
    IXACT3Cue *cue;
    ULONG refcount;
    HRESULT hr;
    HRSRC res;

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    params.pGlobalSettingsBuffer = (void *)global_settings_data;
    params.globalSettingsBufferSize = sizeof(global_settings_data);
    hr = IXACT3Engine_Initialize(engine, &params);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    index = IXACT3Engine_GetGlobalVariableIndex(engine, "speedofsound");
    ok(index == XACTINDEX_INVALID, "Got index %u.\n", index);
    index = IXACT3Engine_GetGlobalVariableIndex(engine, "V1");
    ok(index == XACTINDEX_INVALID, "Got index %u.\n", index);

    index = IXACT3Engine_GetGlobalVariableIndex(engine, "SpeedOfSound");
    ok(index == 5, "Got index %u.\n", index);
    value = -1.0f;
    hr = IXACT3Engine_GetGlobalVariable(engine, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(value == 343.5f, "Got value %.8e.\n", value);

    index = IXACT3Engine_GetGlobalVariableIndex(engine, "v1");
    ok(index == 7, "Got index %u.\n", index);
    value = -1.0f;
    hr = IXACT3Engine_GetGlobalVariable(engine, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(value == 10.0f, "Got value %.8e.\n", value);

    hr = IXACT3Engine_SetGlobalVariable(engine, index, 11.0f);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_GetGlobalVariable(engine, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(value == 11.0f, "Got value %.8e.\n", value);

    hr = IXACT3Engine_SetGlobalVariable(engine, index, 101.0f);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_GetGlobalVariable(engine, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(value == 100.0f, "Got value %.8e.\n", value);

    value = -1.0f;
    hr = IXACT3Engine_GetGlobalVariable(engine, 8, &value);
    todo_wine ok(hr == XACTENGINE_E_INVALIDVARIABLEINDEX, "Got hr %#lx.\n", hr);
    todo_wine ok(value == -1.0f, "Got value %.8e.\n", value);

    hr = IXACT3Engine_SetGlobalVariable(engine, 8, 11.0f);
    todo_wine ok(hr == XACTENGINE_E_INVALIDVARIABLEINDEX, "Got hr %#lx.\n", hr);

    index = IXACT3Engine_GetGlobalVariableIndex(engine, "v2");
    ok(index == XACTINDEX_INVALID, "Got index %u.\n", index);

    index = IXACT3Engine_GetGlobalVariableIndex(engine, "v3");
    todo_wine ok(index == XACTINDEX_INVALID, "Got index %u.\n", index);

    index = IXACT3Engine_GetGlobalVariableIndex(engine, "v4");
    ok(index == 10, "Got index %u.\n", index);

    hr = IXACT3Engine_SetGlobalVariable(engine, index, 11.0f);
    todo_wine ok(hr == XACTENGINE_E_INVALIDVARIABLEINDEX, "Got hr %#lx.\n", hr);
    hr = IXACT3Engine_GetGlobalVariable(engine, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    todo_wine ok(value == 10.0f, "Got value %.8e.\n", value);

    res = FindResourceW(NULL, L"test.xwb", (const WCHAR *)RT_RCDATA);
    ok(!!res, "Got error %lu.\n", GetLastError());

    hr = IXACT3Engine_CreateInMemoryWaveBank(engine,
            LockResource(LoadResource(GetModuleHandleA(NULL), res)),
            SizeofResource(GetModuleHandleA(NULL), res), 0, 0, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Despite the parameter here being const, native will attempt to overwrite
     * some of this data. */
    soundbank_data_copy = malloc(sizeof(soundbank_data));
    memcpy(soundbank_data_copy, soundbank_data, sizeof(soundbank_data));
    hr = IXACT3Engine_CreateSoundBank(engine, soundbank_data_copy, sizeof(soundbank_data), 0, 0, &soundbank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3SoundBank_Prepare(soundbank, 1, 0, 0, &cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    index = IXACT3Cue_GetVariableIndex(cue, "v1");
    ok(index == XACTINDEX_INVALID, "Got index %u.\n", index);

    value = -1.0f;
    hr = IXACT3Cue_GetVariable(cue, 7, &value);
    todo_wine ok(hr == XACTENGINE_E_INVALIDVARIABLEINDEX, "Got hr %#lx.\n", hr);
    todo_wine ok(value == -1.0f, "Got value %.8e.\n", value);

    hr = IXACT3Cue_SetVariable(cue, 7, 11.0f);
    todo_wine ok(hr == XACTENGINE_E_INVALIDVARIABLEINDEX, "Got hr %#lx.\n", hr);

    index = IXACT3Cue_GetVariableIndex(cue, "v2");
    ok(index == 8, "Got index %u.\n", index);

    value = -1.0f;
    hr = IXACT3Cue_GetVariable(cue, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(value == 10.0f, "Got value %.8e.\n", value);
    hr = IXACT3Cue_SetVariable(cue, index, 11.0f);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IXACT3Cue_GetVariable(cue, index, &value);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(value == 11.0f, "Got value %.8e.\n", value);

    refcount = IXACT3Engine_Release(engine);
    todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);
}

static void test_matrices(void)
{
    XACT_RUNTIME_PARAMETERS params = {.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT};
    WAVEFORMATEXTENSIBLE format;
    IXACT3SoundBank *soundbank;
    void *soundbank_data_copy;
    IXACT3WaveBank *wavebank;
    float coefficients[20];
    IXACT3Engine *engine;
    IXACT3Cue *cue;
    ULONG refcount;
    HRESULT hr;
    HRSRC res;

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    params.pGlobalSettingsBuffer = (void *)global_settings_data;
    params.globalSettingsBufferSize = sizeof(global_settings_data);
    hr = IXACT3Engine_Initialize(engine, &params);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    res = FindResourceW(NULL, L"test.xwb", (const WCHAR *)RT_RCDATA);
    ok(!!res, "Got error %lu.\n", GetLastError());

    hr = IXACT3Engine_CreateInMemoryWaveBank(engine,
            LockResource(LoadResource(GetModuleHandleA(NULL), res)),
            SizeofResource(GetModuleHandleA(NULL), res), 0, 0, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3Engine_GetFinalMixFormat(engine, &format);

    /* Despite the parameter here being const, native will attempt to overwrite
     * some of this data. */
    soundbank_data_copy = malloc(sizeof(soundbank_data));
    memcpy(soundbank_data_copy, soundbank_data, sizeof(soundbank_data));
    hr = IXACT3Engine_CreateSoundBank(engine, soundbank_data_copy, sizeof(soundbank_data), 0, 0, &soundbank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IXACT3SoundBank_Play(soundbank, 1, 0, 0, &cue);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    for (unsigned int src_count = 0; src_count < 10; ++src_count)
    {
        hr = IXACT3Cue_SetMatrixCoefficients(cue, src_count, format.Format.nChannels, coefficients);
        todo_wine_if (src_count == 0 || src_count == 9)
            ok(hr == (src_count >= 1 && src_count <= 8 ? S_OK : E_FAIL),
                    "Got hr %#lx for source count %u.\n", hr, src_count);
    }
    for (unsigned int dst_count = 0; dst_count < 10; ++dst_count)
    {
        hr = IXACT3Cue_SetMatrixCoefficients(cue, 1, dst_count, coefficients);
        todo_wine_if (dst_count != format.Format.nChannels)
            ok(hr == (dst_count == format.Format.nChannels ? S_OK : E_INVALIDARG),
                    "Got hr %#lx for destination count %u.\n", hr, dst_count);
    }

    refcount = IXACT3Engine_Release(engine);
    todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);
}

static void test_create_wavebank(void)
{
    XACT_RUNTIME_PARAMETERS params = {.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT};
    XACT_STREAMING_PARAMETERS streaming_params = {0};
    IXACT3WaveBank *wavebank;
    IXACT3Engine *engine;
    WCHAR path[MAX_PATH];
    ULONG refcount;
    HANDLE file;
    HRESULT hr;
    DWORD size;
    HRSRC res;
    BOOL ret;

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    params.pGlobalSettingsBuffer = (void *)global_settings_data;
    params.globalSettingsBufferSize = sizeof(global_settings_data);
    hr = IXACT3Engine_Initialize(engine, &params);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test trying to create an in-memory wave bank from streaming wave bank data. This fails. */

    hr = IXACT3Engine_CreateInMemoryWaveBank(engine, &streaming_wavebank, sizeof(streaming_wavebank), 0, 0, &wavebank);
    todo_wine ok(hr == XACTENGINE_E_INVALIDUSAGE, "Got hr %#lx.\n", hr);

    /* Test trying to create a streaming wave bank from in-memory wave bank data.
     * This actually succeeds, but then trying to use the wavebank crashes. */

    GetTempPathW(ARRAY_SIZE(path), path);
    wcscat(path, L"test.xwb");

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create %s, error %lu.\n", debugstr_w(path), GetLastError());

    res = FindResourceW(NULL, L"test.xwb", (const WCHAR *)RT_RCDATA);
    ok(!!res, "Got error %lu.\n", GetLastError());
    ret = WriteFile(file, LockResource(LoadResource(GetModuleHandleA(NULL), res)),
            SizeofResource(GetModuleHandleA(NULL), res), &size, NULL);
    ok(ret == TRUE, "Got error %lu.\n", GetLastError());

    streaming_params.file = file;
    streaming_params.packetSize = 16;
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &wavebank);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    CloseHandle(file);

    ret = DeleteFileW(path);
    ok(ret == TRUE, "Got error %lu.\n", GetLastError());

    refcount = IXACT3Engine_Release(engine);
    todo_wine ok(!refcount, "Got outstanding refcount %ld.\n", refcount);
}

START_TEST(xact3)
{
    IXACT3Engine *engine;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /* Win 10 v1507 */, "Cannot create engine, hr %#lx\n", hr);

    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("XACT is not supported.\n");
        return;
    }
    IXACT3Engine_Release(engine);

    test_interfaces();
    test_notifications();
    test_renderer_details();
    test_properties();
    test_variables();
    test_matrices();
    test_create_wavebank();

    CoUninitialize();
}
