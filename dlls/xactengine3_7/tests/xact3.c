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

#define COBJMACROS
#include "wine/test.h"

#include "x3daudio.h"

#include "initguid.h"
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

static WCHAR *gen_xwb_file(void)
{
    static const WAVEBANKENTRY entry =
    {
        .Format =
        {
            .wFormatTag = WAVEBANKMINIFORMAT_TAG_ADPCM,
            .nChannels = 2,
            .nSamplesPerSec = 22051,
            .wBlockAlign = 48,
        },
    };
    static const WAVEBANKDATA bank_data =
    {
        .dwFlags = WAVEBANK_TYPE_STREAMING,
        .dwEntryCount = 1,
        .szBankName = "test",
        .dwEntryMetaDataElementSize = sizeof(entry),
        .dwEntryNameElementSize = WAVEBANK_ENTRYNAME_LENGTH,
        .dwAlignment = 0x800,
    };
    static const WAVEBANKHEADER header =
    {
        .dwSignature = WAVEBANK_HEADER_SIGNATURE,
        .dwVersion = XACT_CONTENT_VERSION,
        .dwHeaderVersion = WAVEBANK_HEADER_VERSION,
        .Segments =
        {
            [WAVEBANK_SEGIDX_BANKDATA] =
            {
                .dwOffset = sizeof(header),
                .dwLength = sizeof(bank_data),
            },
            [WAVEBANK_SEGIDX_ENTRYMETADATA] =
            {
                .dwOffset = sizeof(header) + sizeof(bank_data),
                .dwLength = sizeof(entry),
            },
        },
    };
    static WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;

    GetTempPathW(ARRAY_SIZE(path), path);
    lstrcatW(path, L"test.xwb");

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Cannot create file %s, error %ld\n",
            wine_dbgstr_w(path), GetLastError());

    WriteFile(file, &header, sizeof(header), &written, NULL);
    ok(written == sizeof(header), "Cannot write header\n");

    WriteFile(file, &bank_data, sizeof(bank_data), &written, NULL);
    ok(written == sizeof(bank_data), "Cannot write bank data\n");

    WriteFile(file, &entry, sizeof(entry), &written, NULL);
    ok(written == sizeof(entry), "Cannot write entry\n");

    CloseHandle(file);

    return path;
}

struct notification_cb_data
{
    XACTNOTIFICATIONTYPE type;
    IXACT3WaveBank *wave_bank;
    BOOL received;
    DWORD thread_id;
};

static void WINAPI notification_cb(const XACT_NOTIFICATION *notification)
{
    struct notification_cb_data *data = notification->pvContext;
    DWORD thread_id = GetCurrentThreadId();

    data->received = TRUE;
    ok(notification->type == data->type,
            "Unexpected notification type %u\n", notification->type);
    if(notification->type == XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED)
    {
        ok(notification->waveBank.pWaveBank == data->wave_bank, "Unexpected wave bank %p instead of %p\n",
            notification->waveBank.pWaveBank, data->wave_bank);
    }
    ok(thread_id == data->thread_id, "Unexpected thread id %#lx instead of %#lx\n", thread_id, data->thread_id);
}

static void test_notifications(void)
{
    struct notification_cb_data prepared_data = { 0 }, destroyed_data = { 0 };
    XACT_NOTIFICATION_DESCRIPTION notification_desc = { 0 };
    XACT_STREAMING_PARAMETERS streaming_params = { 0 };
    XACT_RUNTIME_PARAMETERS params = { 0 };
    IXACT3Engine *engine;
    WCHAR *filename;
    unsigned int i;
    HANDLE file;
    DWORD state;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACT3Engine, (void **)&engine);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /* Win 10 v1507 */, "Cannot create engine, hr %#lx\n", hr);

    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("XACT not supported\n");
        return;
    }

    params.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;
    params.fnNotificationCallback = notification_cb;

    hr = IXACT3Engine_Initialize(engine, &params);
    ok(hr == S_OK || broken(hr == XAUDIO2_E_INVALID_CALL), "Cannot initialize engine, hr %#lx\n", hr);
    if(FAILED(hr))
    {
        win_skip("Unable to Initialize XACT. No speakers attached?\n");
        IXACT3Engine_Release(engine);
        return;
    }

    notification_desc.type = 0;
    notification_desc.flags = 0;
    notification_desc.pvContext = &prepared_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);

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
    notification_desc.pvContext = NULL;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Cannot register notification, hr %#lx\n", hr);

    /* Registering again overrides pvContext, but each notification
     * class has its own pvContext. */
    notification_desc.pvContext = &destroyed_data;
    hr = IXACT3Engine_RegisterNotification(engine, &notification_desc);
    ok(hr == S_OK, "Cannot register notification, hr %#lx\n", hr);

    filename = gen_xwb_file();

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Cannot open file\n");

    streaming_params.file = file;
    streaming_params.packetSize = 0x800;
    hr = IXACT3Engine_CreateStreamingWaveBank(engine, &streaming_params, &prepared_data.wave_bank);
    ok(hr == S_OK, "Cannot create a streaming wave bank, hr %#lx\n", hr);
    destroyed_data.wave_bank = prepared_data.wave_bank;

    for (i = 0; i < 10 && !prepared_data.received; i++)
    {
        IXACT3Engine_DoWork(engine);
        Sleep(1);
    }

    hr = IXACT3WaveBank_GetState(prepared_data.wave_bank, &state);
    ok(hr == S_OK, "Cannot query wave bank state, hr %#lx\n", hr);
    ok(state == XACT_WAVEBANKSTATE_PREPARED, "Wave bank is not in prepared state, but in %#lx\n", state);

    ok(prepared_data.received, "The 'wave bank prepared' notification was never received\n");
    ok(!destroyed_data.received, "The 'wave bank destroyed' notification was received too early\n");

    IXACT3WaveBank_Destroy(prepared_data.wave_bank);
    ok(destroyed_data.received, "The 'wave bank destroyed' notification was never received\n");

    CloseHandle(file);
    IXACT3Engine_Release(engine);

    DeleteFileW(filename);
}

START_TEST(xact3)
{
    CoInitialize(NULL);

    test_interfaces();
    test_notifications();

    CoUninitialize();
}
